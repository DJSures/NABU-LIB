// ****************************************************************************************
// NABU-LIB C Library
// DJ Sures (c) 2024
// https://nabu.ca
// https://github.com/DJSures/NABU-LIB
//
// Read the NABU-LIB.h file for details of each function in this file.
//
// PERFORMANCE NOTES
// -----------------
// - The tiny helpers (ayWrite, ayRead, vdp_setRegister, vdp_setWriteAddress, the interrupt
//   enable/disable, etc) are defined "inline". SDCC expands them at every call site so they
//   compile down to a couple of OUT instructions instead of a push/call/pop sequence. A
//   callable copy is still emitted, so taking their address keeps working.
// - The hottest single-argument functions (hcca_writeByte, vdp_write, vdp_print...) use
//   __z88dk_fastcall so the argument is passed in a register instead of on the stack.
// - Bulk VRAM transfers go through the _vdp_outBlock / _vdp_fill assembly loops, which are
//   tuned to stay above the TMS9918A minimum VRAM access time (about 8us in graphics modes).
// - Row offsets (row * columns) come from the _vdpRowOffset table that vdp_init() fills, so
//   cursor math no longer needs a software 8x8 multiply on every character.
//
// **********************************************************************************************



// **************************************************************************
// System
// ------
// **************************************************************************

void initNABULib() {

  // Turn off the rom
  IO_CONTROL = CONTROL_ROMSEL | CONTROL_VDOBUF;

  NABU_DisableInterrupts();

  __asm

    push af;
    ld a, r;
    ld (__randomSeed), a;
    pop af;

  __endasm;

  srand(_randomSeed);

  __asm

    IM 2;

    ld a, INTERUPT_VECTOR_MAP_MSB;
    ld i, a;

    #ifndef DISABLE_HCCA_RX_INT
      // HCCA Receive
      ld hl, _isrHCCARX;
      ld (INTERUPT_VECTOR_MAP_ADDRESS), hl;
    #endif

      // // HCCA Send
      // ld hl, _isrHCCATX;
      // ld (INTERUPT_VECTOR_MAP_ADDRESS + 2), hl;

    #ifndef DISABLE_KEYBOARD_INT
      // HCCA Keyboard
      ld hl, _isrKeyboard;
      ld (INTERUPT_VECTOR_MAP_ADDRESS + 4), hl;
    #endif

    // // Video Frame Sync
    // ld hl, _isrVideoSync;
    // ld (INTERUPT_VECTOR_MAP_ADDRESS + 6), hl;

    // // Option card 0
    // ld hl, _isrFunc;
    // ld (INTERUPT_VECTOR_MAP_ADDRESS + 8), hl;

    // // Option card 1
    // ld hl, _isrFunc;
    // ld (INTERUPT_VECTOR_MAP_ADDRESS + 10), hl;

    // // Option card 2
    // ld hl, _isrFunc;
    // ld (INTERUPT_VECTOR_MAP_ADDRESS + 12), hl;

    // // Option card 3
    // ld hl, _isrFunc;
    // ld (INTERUPT_VECTOR_MAP_ADDRESS + 14), hl;

  __endasm;

  uint8_t origIntMask;

  #if BIN_TYPE == BIN_HOMEBREW

    // A homebrew would not have existing interupts, so we start with a nice clean 0
    origIntMask = 0;
  #elif BIN_TYPE == BIN_CPM

    // if cpm, we get the previous interrupt settings that the BIOS set because we only override
    // what we want to use in NABULib.
    origIntMask = ayRead(IOPORTA);
  #endif

  #ifndef DISABLE_HCCA_RX_INT
    origIntMask |= INT_MASK_HCCARX;
  #endif

  #ifndef DISABLE_KEYBOARD_INT
    origIntMask |= INT_MASK_KEYBOARD;
  #endif

  ayWrite(IOPORTA, origIntMask);

  NABU_EnableInterrupts();

  initNABULIBAudio();
}

void initNABULIBAudio() {

  // Noise envelope
  ayWrite(6, 0b00000000);

  // Turn off all channels
  ayWrite(8, 0b00000000);
  ayWrite(9, 0b00000000);
  ayWrite(10, 0b00000000);

  // Enable only the Tone generators on A B C
  ayWrite(7, 0b01111000);

  ayWrite(11, 0);
  ayWrite(12, 0);
  ayWrite(13, 0);
}

// Inline so each call compiles to a single NOP instead of a call/ret pair
inline void nop() {

  __asm__("nop");
}

// Inline so each call compiles to a single DI instead of a call/ret pair
inline void NABU_DisableInterrupts() {

  __asm__("di");
}

// Inline so each call compiles to a single EI instead of a call/ret pair
inline void NABU_EnableInterrupts() {

  __asm__("ei");
}

void RightShift(uint8_t *arr, uint16_t len, uint8_t n) {

  // Shifting by the whole length (or more) leaves nothing but padding. The old
  // pointer loop never terminated in this case and walked through all of RAM.
  if (n >= len) {

    memset(arr, 0x20, len);

    return;
  }

  // memmove/memset are z88dk assembly (LDDR/LDIR) and handle the overlap,
  // which is both smaller and much faster than a byte-at-a-time C loop.
  memmove(arr + n, arr, len - n);

  memset(arr, 0x20, n);
}

// **************************************************************************
// Interrupts
// -----
// **************************************************************************

#ifndef DISABLE_HCCA_RX_INT
  void isrHCCARX() __naked {

    // review LIS and only bc, hl, a registers are used for this function
    // (now pure assembly so only the registers actually used, AF and HL, are saved)
    //
    // _rxBuffer[_rxBufferWritePos] = IO_HCCA;
    // _rxBufferWritePos++;
    //
    // The write position is bumped before the byte is stored. That is safe because
    // the main program cannot run while this ISR is executing.
    __asm

      push af
      push hl

      ld hl, __rxBufferWritePos
      ld a, (hl)
      inc (hl)

      ld hl, __rxBuffer
      add a, l
      ld l, a
      adc a, h
      sub l
      ld h, a

      in a, (_IO_HCCA)
      ld (hl), a

      pop hl
      pop af

      ei
      reti

    __endasm;
  }
#endif

#ifndef DISABLE_KEYBOARD_INT
  void isrKeyboard() __naked {

    // Pure assembly version of the original C logic below. Writing it in assembly
    // means we know exactly which registers are touched (AF, DE, HL) instead of
    // hoping the C code generator never uses one we did not save.
    //
    // inKey = IO_KEYBOARD
    //
    // if (inKey >= 0x80 && inKey <= 0x83)
    //   _lastKeyboardIntVal = inKey;                   joystick header byte
    // else if (inKey < 0x90 || inKey > 0x95)
    //   if (_lastKeyboardIntVal is 0x80..0x83)
    //     _joyStatus[_lastKeyboardIntVal - 0x80] = inKey; _lastKeyboardIntVal = 0;
    //   else
    //     _kbdBuffer[_kbdBufferWritePos++] = inKey;
    __asm

      push af
      push de
      push hl

      in a, (_IO_KEYBOARD)
      ld e, a

      ; 0x80 - 0x83 is a joystick header, remember which joystick is next
      sub 0x80
      cp 4
      jr nc, isrKbd_notJoyHeader

      ld a, e
      ld (__lastKeyboardIntVal), a
      jr isrKbd_done

    isrKbd_notJoyHeader:

      ; 0x90 - 0x95 are keyboard status codes which are ignored
      ld a, e
      sub 0x90
      cp 6
      jr c, isrKbd_done

      ; was the previous byte a joystick header?
      ld a, (__lastKeyboardIntVal)
      sub 0x80
      cp 4
      jr nc, isrKbd_key

      ; _joyStatus[a] = inKey
      ld hl, __joyStatus
      add a, l
      ld l, a
      adc a, h
      sub l
      ld h, a
      ld (hl), e

      xor a
      ld (__lastKeyboardIntVal), a
      jr isrKbd_done

    isrKbd_key:

      ; _kbdBuffer[_kbdBufferWritePos++] = inKey
      ld hl, __kbdBufferWritePos
      ld a, (hl)
      inc (hl)

      ld hl, __kbdBuffer
      add a, l
      ld l, a
      adc a, h
      sub l
      ld h, a
      ld (hl), e

    isrKbd_done:

      pop hl
      pop de
      pop af

      ei
      reti

    __endasm;
  }
#endif

#if BIN_TYPE == BIN_CPM

  // **************************************************************************
  // VT51 for CPM
  // ------------------
  // **************************************************************************

  // Sends ESC followed by one character. putchar() is used instead of printf()
  // so a program that never calls printf() itself does not link in the whole
  // printf formatter (several KB) just for these escape codes.
  static void _vt_esc(uint8_t c) {

    putchar(27);
    putchar(c);
  }

  void vt_clearToEndOfScreen() {

    _vt_esc('J');
  }

  void vt_clearToEndOfLine() {

    _vt_esc('K');
  }

  void vt_clearScreen() {

    if (_EMULATION_MODE == 0)
      putchar(26);
    else
      _vt_esc('E');
  }

  void vt_clearLine() {

    _vt_esc('l');
  }

  void vt_clearToStartOfLine() {

    _vt_esc('o');
  }

  void vt_clearToStartOfScreen() {

    _vt_esc('d');
  }

  void vt_moveCursorDown(uint8_t count) {

    for (; count != 0; count--) {

      if (_EMULATION_MODE == 0)
        putchar(0x0a);
      else
        _vt_esc('B');
    }
  }

  void vt_cursorHome() {

    _vt_esc('H');
  }

  void vt_moveCursorLeft(uint8_t count) {

    for (; count != 0; count--) {

      if (_EMULATION_MODE == 0)
        putchar(8);
      else
        _vt_esc('D');
    }
  }

  void vt_moveCursorRight(uint8_t count) {

    for (; count != 0; count--) {

      if (_EMULATION_MODE == 0)
        putchar(12);
      else
        _vt_esc('C');
    }
  }

  void vt_moveCursorUp(uint8_t count) {

    for (; count != 0; count--) {

      if (_EMULATION_MODE == 0)
        putchar(11);
      else
        _vt_esc('A');
    }
  }

  void vt_deleteLine() {

    if (_EMULATION_MODE)
      _vt_esc('R');
    else
      _vt_esc('M');
  }

  void vt_setCursor(uint8_t x, uint8_t y) {

    putchar(27);

    if (_EMULATION_MODE == 0)
      putchar('=');
    else
      putchar('Y');

    putchar(32 + y);
    putchar(32 + x);
  }

  void vt_insertLine() {

    if (_EMULATION_MODE == 0)
      _vt_esc('E');
    else
      _vt_esc('L');
  }

  void vt_restoreCursorPosition() {

    // Both emulation modes use the same sequence
    _vt_esc('k');
  }

  void vt_backgroundColor(uint8_t color) {

    putchar(27);
    putchar('c');
    putchar(color);
  }

  void vt_saveCursorPosition() {

    _vt_esc('j');
  }

  void vt_cursorUpAndInsert() {

    _vt_esc('I');
  }

  void vt_wrapOff() {

    _vt_esc('w');
  }

  void vt_wrapOn() {

    _vt_esc('v');
  }

  void vt_normalVideo() {

    _vt_esc('q');
  }

  void vt_reverseVideo() {

    _vt_esc('p');
  }

  bool isCloudCPM() {

    return _CLOUD_CPM_KEY == 0x55;
  }

#endif




// **************************************************************************
// Sound
// -----
// **************************************************************************

// Inline: compiles to two OUT instructions at every call site
inline void ayWrite(uint8_t reg, uint8_t val) {

  IO_AYLATCH = reg;

  IO_AYDATA = val;
}

// Inline: compiles to an OUT and an IN at every call site
inline uint8_t ayRead(uint8_t reg) {

  IO_AYLATCH = reg;

  return IO_AYDATA;
}

void playNoteDelay(uint8_t channel, uint8_t note, uint16_t delayLength) {

  // Set the envelope length
  // Note: R11 receives the high byte and R12 the low byte. That is swapped compared
  // to the AY datasheet (R11 = fine, R12 = coarse), but existing programs have their
  // delay values tuned to this behaviour, so it is intentionally left as is.
  ayWrite(11, delayLength >> 8);
  ayWrite(12, delayLength & 0xff);

  if (channel > 2)
    return;

  // Only the selected channel uses the envelope, the other two are silenced.
  // This replaces three copies of the same code in a switch statement.
  ayWrite(8,  (channel == 0) ? 0b00010000 : 0b00000000);
  ayWrite(9,  (channel == 1) ? 0b00010000 : 0b00000000);
  ayWrite(10, (channel == 2) ? 0b00010000 : 0b00000000);

  uint8_t toneReg = channel << 1;

  ayWrite(toneReg,     _NOTES_FINE[note]);
  ayWrite(toneReg + 1, _NOTES_COURSE[note]);

  ayWrite(13, 0b00000000);
}





// **************************************************************************
// Keyboard
// --------
// **************************************************************************

#ifndef DISABLE_KEYBOARD_INT

  uint8_t isKeyPressed() {

    return (_kbdBufferWritePos != _kbdBufferReadPos);
  }

  uint8_t getChar() {

    #if defined(DISABLE_CURSOR) || defined(DISABLE_VDP)

      while (_kbdBufferWritePos == _kbdBufferReadPos);

    #else

      uint16_t cursorCnt = 0;

      while (_kbdBufferWritePos == _kbdBufferReadPos)
        if (CURSOR_CHAR != 0) {

          if (cursorCnt == 1)
            vdp_writeCharAtLocation(vdp_cursor.x, vdp_cursor.y, CURSOR_CHAR);
          else if (cursorCnt == 15000)
            vdp_writeCharAtLocation(vdp_cursor.x, vdp_cursor.y, ' ');
          else if (cursorCnt > 30000)
            cursorCnt = 0;

          cursorCnt++;
        }

      if (CURSOR_CHAR != 0)
        vdp_writeCharAtLocation(vdp_cursor.x, vdp_cursor.y, ' ');

    #endif

    uint8_t key = _kbdBuffer[_kbdBufferReadPos];

    _kbdBufferReadPos++;

    return key;
  }

  // Inline: a single memory load at the call site
  inline uint8_t getJoyStatus(uint8_t joyNum) {

      return _joyStatus[joyNum];
  }

  #ifndef DISABLE_VDP

    uint8_t readLine(uint8_t *buffer, uint8_t maxInputLen) {

      uint8_t i = 0;

      while (true) {

        uint8_t c = getChar();

        if (c == 13) {

          break;
        } else if (i < maxInputLen && c >= 0x20 && c <= 126) {

          buffer[i] = c;

          vdp_write(c);

          i++;
        } else if (i == maxInputLen && c != 127) {

          playNoteDelay(0, 60, 10);
        } else if (c == 127 && i > 0) {

          if (vdp_cursor.x == 0) {

            vdp_cursor.x = 39;
            vdp_cursor.y--;
          } else {

            vdp_cursor.x--;
          }

          i--;

          vdp_writeCharAtLocation(vdp_cursor.x, vdp_cursor.y, ' ');
        }
      }

      return i;
    }
  #endif

#endif


#ifndef DISABLE_HCCA_RX_INT

  // **************************************************************************
  // HCCA Receive
  // ------------
  // **************************************************************************

  bool hcca_ping() {

    hcca_writeByte(0xa1);

    uint32_t timer = 0;

    while (_rxBufferWritePos == _rxBufferReadPos) {

      timer++;

      if (timer == 50000)
        return false;
    }

    uint8_t resp = hcca_readByte();

    if (resp == 0x55)
      return true;

    return false;
  }

  // Inline: a two byte compare at the call site
  inline bool hcca_isRxBufferAvailable() {

    return _rxBufferWritePos != _rxBufferReadPos;
  }

  uint8_t hcca_getSizeOfDataInBuffer() {

    // The rx ring buffer is exactly 256 bytes and the read/write positions
    // are uint8_t, so natural 8-bit wrap-around subtraction always yields
    // the correct number of bytes available, including across the wrap.
    // The previous (0xff - read) + write expression was off-by-one.
    return (uint8_t)(_rxBufferWritePos - _rxBufferReadPos);
  }

  uint8_t hcca_readByte() {

    while (_rxBufferWritePos == _rxBufferReadPos);

    uint8_t ret = _rxBuffer[_rxBufferReadPos];

    _rxBufferReadPos++;

    return ret;
  }

  uint16_t hcca_readUInt16() {

    // Two separate statements so the low byte is guaranteed to be read first.
    // In "a() + (b() << 8)" C does not define which call happens first.
    uint16_t ret = hcca_readByte();

    return ret | ((uint16_t)hcca_readByte() << 8);
  }

  int16_t hcca_readInt16() {

    return (int16_t)hcca_readUInt16();
  }

  uint32_t hcca_readUInt32() {

    // The Z80 is little-endian like the wire format, so read straight into the
    // variable. (The old array initializer did not guarantee the call order.)
    uint32_t ret;

    hcca_readBytes(0, 4, (uint8_t *)&ret);

    return ret;
  }

  int32_t hcca_readInt32() {

    return (int32_t)hcca_readUInt32();
  }

  void hcca_readBytes(uint16_t offset, uint16_t bufferLen, uint8_t *buffer) {

    uint8_t *start = buffer + offset;
    uint8_t *end   = start + bufferLen;

    // while (not do/while) so a bufferLen of 0 reads nothing. The do/while version
    // read 65536 bytes for a length of 0 and hung or overwrote memory, which
    // happened whenever the Internet Adapter replied with an empty string.
    while (start != end) {

      while (_rxBufferWritePos == _rxBufferReadPos);

      *start = _rxBuffer[_rxBufferReadPos];

      start++;

      _rxBufferReadPos++;
    }
  }


  // **************************************************************************
  // HCCA Transmit
  // -------------
  // **************************************************************************

  void hcca_writeByte(uint8_t c) __z88dk_fastcall {

    NABU_DisableInterrupts();

    uint8_t origIntMask = ayRead(IOPORTA);

    ayWrite(IOPORTA, INT_MASK_HCCATX);

    IO_AYLATCH = IOPORTB;
    while (IO_AYDATA & 0x04);

    IO_HCCA = c;

    ayWrite(IOPORTA, origIntMask);

    NABU_EnableInterrupts();
  }

  // The multi-byte writers send the variable straight from memory. The Z80 is
  // little-endian, which matches the LSB-first wire format, so there is no
  // shifting of 32-bit values.
  void hcca_writeUInt32(uint32_t val) __z88dk_fastcall {

    hcca_writeBytes(0, 4, (uint8_t *)&val);
  }

  void hcca_writeInt32(int32_t val) __z88dk_fastcall {

    hcca_writeBytes(0, 4, (uint8_t *)&val);
  }

  void hcca_writeUInt16(uint16_t val) __z88dk_fastcall {

    hcca_writeByte((uint8_t)val);
    hcca_writeByte((uint8_t)(val >> 8));
  }

  void hcca_writeInt16(int16_t val) __z88dk_fastcall {

    hcca_writeUInt16((uint16_t)val);
  }

  void hcca_writeString(uint8_t *str) __z88dk_fastcall {

    while (*str != 0x00) {

      hcca_writeByte(*str);
      str++;
    }
  }

  void hcca_writeBytes(uint16_t offset, uint16_t length, uint8_t *bytes) {

    uint8_t *start = bytes + offset;
    uint8_t *end   = start + length;

    while (start != end) {

      hcca_writeByte(*start);

      start++;
    }
  }
#endif


// **************************************************************************
// VDP
// ---
// **************************************************************************

#ifndef DISABLE_VDP

  // Name table offset of a column/row using the row table (no multiply)
  #define _VDP_NAME_OFFSET(x, y) (_vdpRowOffset[(y)] + (x))

  // Pixel masks for vdp_plotHires (replaces a variable 0x80 >> n shift loop)
  static const uint8_t _vdpBitMask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

  // Inline: compiles to two OUT instructions at every call site
  inline void vdp_setRegister(uint8_t registerIndex, uint8_t value) {

    IO_VDPLATCH = value;

    IO_VDPLATCH = 0x80 | registerIndex;
  }

  // Inline: compiles to two OUT instructions at every call site
  inline void vdp_setWriteAddress(uint16_t address) {

    IO_VDPLATCH = address;

    IO_VDPLATCH = (address >> 8) | 0x40;
  }

  // Deliberately NOT inlined. After the read address is set the VDP needs a few
  // microseconds to fetch the byte, and the call/return provides that delay
  // before the caller reads IO_VDPDATA.
  void vdp_setReadAddress(uint16_t address) {

    IO_VDPLATCH = address;

    IO_VDPLATCH = (address >> 8);
  }

  // NOTE ON (void) PARAMETERS
  // The __naked assembly functions below read their arguments directly from the
  // stack/registers. SDCC cannot see inside __asm blocks, so it would report every
  // parameter as unused (warning 85). Each one starts with "(void)param;" which marks
  // the parameter as used and generates no code. (#pragma disable_warning is not used
  // because in this SDCC version it stays off for the rest of the file, which would
  // also hide real warnings in the program that includes NABU-LIB.)

  // **************************************************************************
  // Send len bytes from src to VRAM at the current VDP write address.
  // 37 T-states per byte (about 10us at 3.58MHz), which is within the TMS9918A
  // access window in every mode. len may be 0.
  // **************************************************************************
  void _vdp_outBlock(uint8_t *src, uint16_t len) __naked {

    // see NOTE ON (void) PARAMETERS
    (void)src;
    (void)len;

    __asm

      ld hl, 2
      add hl, sp
      ld e, (hl)
      inc hl
      ld d, (hl)
      inc hl
      ld c, (hl)
      inc hl
      ld b, (hl)
      ex de, hl

      ; hl = src, bc = len
      ld a, b
      or c
      ret z

      ; d = outer passes, b = inner count (0 means 256)
      ld d, b
      ld b, c
      ld a, c
      or a
      jr z, vdpOutBlock_loop
      inc d

    vdpOutBlock_loop:

      ld a, (hl)
      out (_IO_VDPDATA), a
      inc hl
      djnz vdpOutBlock_loop

      dec d
      jr nz, vdpOutBlock_loop

      ret

    __endasm;
  }

  // **************************************************************************
  // Write value to VRAM len times at the current VDP write address.
  // 32 T-states per byte. len may be 0.
  // **************************************************************************
  void _vdp_fill(uint16_t len, uint8_t value) __naked {

    // see NOTE ON (void) PARAMETERS
    (void)len;
    (void)value;

    __asm

      ld hl, 2
      add hl, sp
      ld c, (hl)
      inc hl
      ld b, (hl)
      inc hl
      ld e, (hl)

      ; bc = len, e = value
      ld a, b
      or c
      ret z

      ld d, b
      ld b, c
      ld a, c
      or a
      jr z, vdpFill_loop
      inc d

    vdpFill_loop:

      ld a, e
      out (_IO_VDPDATA), a
      nop
      djnz vdpFill_loop

      dec d
      jr nz, vdpFill_loop

      ret

    __endasm;
  }


  // **************************************************************************
  // Write a block to VRAM at addr. When the pattern/color tables are split in
  // thirds (G2 bitmap mode) the same block is also written at +2048 and +4096.
  // datasheet 2-20 : screen is split into 3 and the tables are repeated 3 times
  // **************************************************************************
  static void _vdp_loadThirds(uint16_t addr, uint8_t *src, uint16_t len) {

    uint8_t copies = _vdpSplitThirds ? 3 : 1;

    do {

      vdp_setWriteAddress(addr);

      _vdp_outBlock(src, len);

      addr += 2048;
      copies--;
    } while (copies != 0);
  }

  // Same as _vdp_loadThirds() but fills with a single value
  static void _vdp_fillThirds(uint16_t addr, uint16_t len, uint8_t value) {

    uint8_t copies = _vdpSplitThirds ? 3 : 1;

    do {

      vdp_setWriteAddress(addr);

      _vdp_fill(len, value);

      addr += 2048;
      copies--;
    } while (copies != 0);
  }

  void waitVdpISR() __naked {

    __asm

      push af;

    // _vdpStatus = IO_VDPLATCH;
      in	a, (_IO_VDPLATCH)
      ld	(_vdpStatusRegVal), a

    // _vdpReady = true;
      ld  a, 0x01
      ld  (_vdpIsReady), a

      pop af;

      ei;
      reti;

    __endasm;
  }

  void vdp_enableVDPReadyInt() {

    // This is exactly vdp_addISR() with the built in "ready" ISR, so share the code
    vdp_addISR(waitVdpISR);
  }

  // Inline: clears the flag and spins on it at the call site
  inline void vdp_waitVDPReadyInt() {

    // uncomment this to enable debugging for the VDP to see if the vdpIsReady flag was
    // set prior to your program calling vdp_waitVDPReadyInt(). That means your program took
    // too long and missed the vertical screen refresh.
    // If you're using a frame buffer, it's okay if the program is taking too long because
    // you could use this function to wait for the vdp to be ready before updating the
    // frame buffer.

    #ifdef DEBUG_VDP_INT

      if (vdpIsReady)
        IO_CONTROL = CONTROL_ROMSEL | CONTROL_VDOBUF | CONTROL_LED_ALERT;
      else
        IO_CONTROL = CONTROL_ROMSEL | CONTROL_VDOBUF;

    #endif

    vdpIsReady = false;

    while (!vdpIsReady);
  }

  void vdp_disableVDPReadyInt() {

    vdp_setRegister(1, _vdpReg1Val);

    NABU_DisableInterrupts();

    _vdpInterruptEnabled = false;

    // clear any existing interrupts status
    // (reading the VDP status register clears its interrupt flag. This used to
    // read IO_AYLATCH, which is the write-only AY latch and did nothing.)
    vdpStatusRegVal = IO_VDPLATCH;

    ayWrite(IOPORTA, ayRead(IOPORTA) & ~INT_MASK_VDP);

    NABU_EnableInterrupts();
  }

  void vdp_addISR(void (*isr)()) {

    NABU_DisableInterrupts();

    _vdp_ISR = isr;

    __asm

      // Video Frame Sync
      ld hl, (__vdp_ISR);
      ld (INTERUPT_VECTOR_MAP_ADDRESS + 6), hl;

    __endasm;

    ayWrite(IOPORTA, ayRead(IOPORTA) | INT_MASK_VDP);

    NABU_EnableInterrupts();

    _vdpInterruptEnabled = true;
    vdp_setRegister(1, _vdpReg1Val | 0b00100000 );
  }

  void vdp_removeISR() {

    vdp_disableVDPReadyInt();
  }

  void vdp_init(uint8_t mode, uint8_t fgColor, uint8_t bgColor, bool big_sprites, bool magnify, bool autoScroll, bool splitThirds) {

    _vdpMode = mode;

    _vdpSplitThirds = splitThirds;

    _vdpSpriteSizeSelected = big_sprites;

    _autoScroll = autoScroll;

    _vdpInterruptEnabled = false;

    vdp_cursor.x = 0;
    vdp_cursor.y = 0;

    switch (mode) {

      case VDP_MODE_G2:

        vdp_setRegister(0, 0b00000010);

        _vdpReg1Val = 0b11000000 | (big_sprites << 1) | magnify;
        vdp_setRegister(1, _vdpReg1Val);

        vdp_setRegister(2, 0x06);
        _vdpPatternNameTableAddr = 0x1800;

        if (_vdpSplitThirds)
          vdp_setRegister(4, 0x03);
        else
          vdp_setRegister(4, 0x00);

        _vdpPatternGeneratorTableAddr = 0x00;

        fgColor = 0;
        _vdpCursorMaxX = 31;
        _vdpCursorMaxXFull = 32;
        _vdpTextBufferSize = 768;

        break;

      case VDP_MODE_TEXT40:

        vdp_setRegister(0, 0b00000000);

        _vdpReg1Val = 0b11010010;
        vdp_setRegister(1, _vdpReg1Val);

        vdp_setRegister(2, 0x06);
        _vdpPatternNameTableAddr = 0x1800;

        vdp_setRegister(4, 0x00);
        _vdpPatternGeneratorTableAddr = 0x00;

        _vdpCursorMaxX = 39;
        _vdpCursorMaxXFull = 40;
        _vdpTextBufferSize = 960;

        break;
      case VDP_MODE_TEXT80:

        vdp_setRegister(0, 0x04);

        _vdpReg1Val = 0xD2;
        vdp_setRegister(1, _vdpReg1Val);

        vdp_setRegister(2, 0b00000111); // pattern name table address (0x1000)
        _vdpPatternNameTableAddr = 0x1000;

        vdp_setRegister(4, 0x00);       // pattern generator address (0x0000)
        _vdpPatternGeneratorTableAddr = 0x00;

        _vdpCursorMaxX = 79;
        _vdpCursorMaxXFull = 80;
        _vdpTextBufferSize = 1920;

        break;
      case VDP_MODE_MULTICOLOR:

        vdp_setRegister(0, 0b00000000);

        _vdpReg1Val = 0b11001000 | (big_sprites << 1) | magnify;
        vdp_setRegister(1, _vdpReg1Val);

        vdp_setRegister(2, 0x06);
        _vdpPatternNameTableAddr = 0x1800;

        vdp_setRegister(4, 0x00);
        _vdpPatternGeneratorTableAddr = 0x00;

        fgColor = 0;
        _vdpCursorMaxX = 31;
        _vdpCursorMaxXFull = 32;

        _vdpTextBufferSize = 768;

        break;
    }

    // Pre-compute row * columns for every row (and one past the last row) so the
    // text functions can look up a name table offset instead of multiplying.
    uint16_t  rowOffset = 0;
    uint16_t *rowPtr    = _vdpRowOffset;
    uint16_t *rowEnd    = _vdpRowOffset + (VDP_ROWS + 1);

    do {

      *rowPtr = rowOffset;

      rowOffset += _vdpCursorMaxXFull;
      rowPtr++;
    } while (rowPtr != rowEnd);

    if (_vdpSplitThirds)
      vdp_setRegister(3, 0xff);
    else
      vdp_setRegister(3, 0x9f);
    _vdpColorTableAddr = 0x2000;

    vdp_setRegister(5, 0x36);
    _vdpSpriteAttributeTableAddr = 0x1b00;

    vdp_setRegister(6, 0x07);
    _vdpSpriteGeneratorTableAddr = 0x3800;

    vdp_setRegister(7, (fgColor << 4) | (bgColor & 0x0f));

    vdp_clearScreen();
  }

  void vdp_clearVRAM() {

    // 16 KB of VRAM (0x0000 - 0x3FFF). Loop bound was 0x3FFF which left
    // the final byte uncleared, so use 0x4000 (full 16384 bytes).
    vdp_setWriteAddress(0x00);

    _vdp_fill(0x4000, 0x00);
  }

  void vdp_initMSXMode(uint8_t bgColor) {

    // Configure TMS9918A for MSX SCREEN 2 (Graphics II) so .SC2 files display correctly.
    //
    // SC2 file layout (raw VRAM dump):
    //   0x0000 - 0x17FF : pattern table
    //   0x1800 - 0x1AFF : screen image table (name table)
    //   0x1B00 - 0x1B7F : sprite attribute table
    //   0x2000 - 0x37FF : color table
    //
    // On MSX you would do:
    //   10 SCREEN 2
    //   20 BLOAD "FILE.SC2",S
    //
    // This function mirrors that mode setup on the NABU's TMS9918A-compatible VDP.

    // R0: Mode bits, enable Graphic 2 (SCREEN 2)
    //   bit1 (M1) = 1, other mode bits 0
    vdp_setRegister(0, 0b00000010);

    // R1:
    //   bit7 = 1 -> 16K VRAM
    //   bit6 = 1 -> display ON (0 when we want to blank)
    //   bit5 = 1 -> blank leftmost 8 pixels (MSX style, can be 0 if not desired)
    //   other bits: 8x8 sprites, no magnification
    vdp_setRegister(1, 0b11100000);

    // R2: name (Screen Image) table base: 0x1800
    //   base = R2 * 0x400 -> 0x06 * 0x400 = 0x1800
    vdp_setRegister(2, 0x06);

    // R3: color table base for Graphics II.
    //   For G2, R3 = 0xFF selects the color table at 0x2000 with the proper mask.
    vdp_setRegister(3, 0xff);

    // R4: pattern generator base for Graphics II.
    //   In G2, R4 = 0x03 maps the pattern table to 0x0000 as used by SC2 files.
    vdp_setRegister(4, 0x03);

    // R5: sprite attribute table base: 0x1B00
    //   base = R5 * 0x80 -> 0x36 * 0x80 = 0x1B00
    vdp_setRegister(5, 0x36);

    // R6: sprite pattern table base: 0x3800
    //   base = R6 * 0x800 -> 0x07 * 0x800 = 0x3800
    vdp_setRegister(6, 0x07);

    // R7: background / border colour (low nibble)
    vdp_setRegister(7, bgColor & 0x0f);
  }

  void vdp_initTextMode80(uint8_t fgColor, uint8_t bgColor, bool autoScroll) {

    vdp_init(VDP_MODE_TEXT80, fgColor, bgColor , false, false, autoScroll, false);
  }

  void vdp_initTextMode(uint8_t fgColor, uint8_t bgColor, bool autoScroll) {

    vdp_init(VDP_MODE_TEXT40, fgColor, bgColor , false, false, autoScroll, false);
  }

  void vdp_initG2Mode(uint8_t bgColor, bool bigSprites, bool scaleSprites, bool autoScroll, bool splitThirds) {

    vdp_init(VDP_MODE_G2, 0, bgColor, bigSprites, scaleSprites, autoScroll, splitThirds);
  }

  void vdp_initMultiColorMode() {

    vdp_init(VDP_MODE_MULTICOLOR, 0, 0, false, false, false, false);
  }

  void vdp_clearScreen() {

    vdp_setWriteAddress(_vdpPatternNameTableAddr);

    uint8_t cr = 0x00;

    if (_vdpMode == VDP_MODE_TEXT40 || _vdpMode == VDP_MODE_TEXT80)
      cr = 0x20;

    // _vdpTextBufferSize is columns * 24 for the current mode. A size of 0 (vdp_init
    // never called) now does nothing instead of looping through 64KB.
    _vdp_fill(_vdpTextBufferSize, cr);

    memset(_vdp_textBuffer, 0x20, _vdpTextBufferSize);
  }

  void vdp_fillScreen(uint8_t c) __z88dk_fastcall {

    vdp_setWriteAddress(_vdpPatternNameTableAddr);

    _vdp_fill(_vdpTextBufferSize, c);

    memset(_vdp_textBuffer, c, _vdpTextBufferSize);
  }

  void vdp_clearRows(uint8_t topRow, uint8_t bottomRow) {

    // Clears topRow up to (but not including) bottomRow, so (0, 24) is the whole screen.
    if (bottomRow > VDP_ROWS)
      bottomRow = VDP_ROWS;

    // An empty range used to wrap the do/while and clear 64KB of RAM
    if (topRow >= bottomRow)
      return;

    uint16_t name_offset = _vdpRowOffset[topRow];
    uint16_t len         = _vdpRowOffset[bottomRow] - name_offset;

    vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);

    _vdp_fill(len, 0x20);

    memset(_vdp_textBuffer + name_offset, 0x20, len);
  }

  void vdp_loadASCIIFont(uint8_t *font) __z88dk_fastcall {

    vdp_setWriteAddress(_vdpPatternGeneratorTableAddr + 0x100);

    _vdp_outBlock(font, 768);
  }

  void vdp_loadASCIIFontWithInverse(uint8_t *font) __z88dk_fastcall {

    vdp_setWriteAddress(_vdpPatternGeneratorTableAddr + 0x100);

    _vdp_outBlock(font, 768);

    uint8_t *start = font;
    uint8_t *end   = start + 768;

    do {

      IO_VDPDATA = *start ^ 0xff;

      start++;
    } while (start != end);
  }

  void vdp_putPattern(uint8_t x, uint8_t y, uint8_t patternId) {

    uint16_t name_offset = _VDP_NAME_OFFSET(x, y);

    vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);
    _vdp_textBuffer[name_offset] = patternId;

    IO_VDPDATA = patternId;
  }

  void vdp_loadPatternToId(uint8_t patternId, uint8_t *pattern) {

    // datasheet 2-20 : screen is split into 3 and the pattern table therefore is repeated 3 times
    _vdp_loadThirds(_vdpPatternGeneratorTableAddr + ((uint16_t)patternId << 3), pattern, 8);
  }

  void vdp_loadPatternTable(uint8_t *patternTable, uint16_t len) {

    // datasheet 2-20 : screen is split into 3 and the pattern table therefore is repeated 3 times
    _vdp_loadThirds(_vdpPatternGeneratorTableAddr, patternTable, len);
  }

  void vdp_loadColorToId(uint8_t patternId,  uint8_t *color) {

    // datasheet 2-20 : screen is split into 3 and the color table therefore is repeated 3 times
    _vdp_loadThirds(_vdpColorTableAddr + ((uint16_t)patternId << 3), color, 8);
  }

  void vdp_loadColorTable(uint8_t *colorTable, uint16_t len) {

    // datasheet 2-20 : screen is split into 3 and the color table therefore is repeated 3 times
    _vdp_loadThirds(_vdpColorTableAddr, colorTable, len);
  }

  void vdp_colorizePattern(uint8_t patternId, uint8_t fg, uint8_t bg) {

    _vdp_fillThirds(_vdpColorTableAddr + ((uint16_t)patternId << 3), 8, (fg << 4) | (bg & 0x0f));
  }

  void vdp_plotHires(uint8_t x, uint8_t y, uint8_t color1, uint8_t color2) {

    // 8 * (x / 8) + y % 8 + 256 * (y / 8), done with masks: the low byte is
    // (x & 0xF8) | (y & 7) and the high byte is y / 8.
    uint16_t offset = ((uint16_t)(y >> 3) << 8) | (uint8_t)((x & 0xF8) | (y & 0x07));
    uint8_t  mask   = _vdpBitMask[x & 0x07];

    vdp_setReadAddress(_vdpPatternGeneratorTableAddr + offset);
    uint8_t pixel = IO_VDPDATA;

    vdp_setReadAddress(_vdpColorTableAddr + offset);
    uint8_t color = IO_VDPDATA;

    // color1 is a uint8_t color index (not a pointer). Pass 0 to plot
    // a background-only pixel using color2; any non-zero value sets the
    // foreground bit and uses color1 as the foreground color.
    if (color1 != 0) {

      pixel |= mask; // Set bit as "1"
      color = (color & 0x0F) | (color1 << 4);
    } else {

      pixel &= ~mask; // Set bit as "0"
      color = (color & 0xF0) | (color2 & 0x0F);
    }

    vdp_setWriteAddress(_vdpPatternGeneratorTableAddr + offset);
    IO_VDPDATA = pixel;

    vdp_setWriteAddress(_vdpColorTableAddr + offset);
    IO_VDPDATA = color;
  }

  void vdp_plotColor(uint8_t x, uint8_t y, uint8_t color) {

    // 8 * (x / 2) + y % 8 + 256 * (y / 8) without divides or multiplies
    uint16_t offset = ((uint16_t)(y >> 3) << 8) + ((uint16_t)(x >> 1) << 3) + (y & 0x07);

    if (_vdpMode == VDP_MODE_MULTICOLOR) {

      uint16_t addr = _vdpPatternGeneratorTableAddr + offset;

      vdp_setReadAddress(addr);
      uint8_t dot = IO_VDPDATA;

      vdp_setWriteAddress(addr);

      if (x & 1) // Odd columns
        IO_VDPDATA = (dot & 0xF0) + (color & 0x0f);
      else
        IO_VDPDATA = (dot & 0x0F) + (color << 4);
    } else if (_vdpMode == VDP_MODE_G2) {

      // Draw bitmap
      vdp_setReadAddress(_vdpColorTableAddr + offset);
      uint8_t color_ = IO_VDPDATA;

      if ((x & 1) == 0) {

        // Even
        color_ &= 0x0F;
        color_ |= (color << 4);
      } else {

        color_ &= 0xF0;
        color_ |= color & 0x0F;
      }

      vdp_setWriteAddress(_vdpPatternGeneratorTableAddr + offset);
      IO_VDPDATA = 0xF0;

      vdp_setWriteAddress(_vdpColorTableAddr + offset);
      IO_VDPDATA = color_;
    }
  }

  void vdp_disableSprite(uint8_t id) __z88dk_fastcall {

    uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);

    vdp_setWriteAddress(addr);
    IO_VDPDATA = 192;        // y
    IO_VDPDATA = 0;          // x
    IO_VDPDATA = 0;          // pattern name #0 (should always be empty so no false collisions)
    IO_VDPDATA = 0b10000000; // early clock to hide behind border
  }

  void vdp_loadSpritePatternNameTable(uint16_t numSprites, uint8_t *sprite) {

    for (uint8_t i = 0; i < 32; i++)
      vdp_disableSprite(i);

    vdp_setWriteAddress(_vdpSpriteGeneratorTableAddr);

    // 16x16 sprites are 32 bytes each, 8x8 sprites are 8 bytes each
    if (_vdpSpriteSizeSelected)
      _vdp_outBlock(sprite, numSprites << 5);
    else
      _vdp_outBlock(sprite, numSprites << 3);
  }

  uint8_t vdp_spriteInit(uint8_t id, uint8_t spritePatternNameId, uint8_t x, uint8_t y, uint8_t color) {

    uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);

    vdp_setWriteAddress(addr);

    IO_VDPDATA = y; // y

    IO_VDPDATA = x; // x

    if (_vdpSpriteSizeSelected)
      IO_VDPDATA = spritePatternNameId << 2; // 16x16 sprite location (name). big sprites take 4 8x8 blocks (datasheet 2-34)
    else
      IO_VDPDATA = spritePatternNameId;     // 8x8 sprite location (name

    // Set bit 7 to hide the sprite since we're not using it yet
    IO_VDPDATA = color & 0x0f;

    return id;
  }

  void vdp_setSpriteColor(uint8_t id, uint8_t color) {

    uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);

    vdp_setWriteAddress(addr + 3);

    IO_VDPDATA = color & 0x0f;
  }

  void vdp_setSpritePosition(uint8_t id, uint8_t x, uint8_t y) {

    uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);

    vdp_setWriteAddress(addr);
    IO_VDPDATA = y;
    IO_VDPDATA = x;
  }

  void vdp_setSpritePositionAndColor(uint8_t id, uint8_t x, uint8_t y, uint8_t color) {

    uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);

    vdp_setWriteAddress(addr);
    IO_VDPDATA = y;
    IO_VDPDATA = x;

    // Skip the pattern name byte so the pattern set by vdp_spriteInit() is kept.
    // This used to write the sprite id as the pattern name, which replaced the
    // sprite's pattern (and was always wrong for 16x16 sprites).
    vdp_setWriteAddress(addr + 3);
    IO_VDPDATA = color & 0x0f;
  }

  void vdp_getSpritePosition(uint8_t id, uint8_t *xpos, uint8_t *ypos) {

    uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);

    vdp_setReadAddress(addr);

    *ypos = IO_VDPDATA;

    *xpos = IO_VDPDATA;
  }

  // **************************************************************************
  // Internal helper used by vdp_print, vdp_printPart, and vdp_write when the
  // cursor advances past the right margin. Does one of three things:
  //
  //  1) Soft wrap to the next row. The TMS9918A auto-increments its internal
  //     write-address counter as bytes are pushed to IO_VDPDATA, and the name
  //     table is row-major contiguous in VRAM, so the chip is already pointing
  //     at the next row's first byte for free - no setWriteAddress needed.
  //
  //  2) Bottom-of-region with autoscroll on: scroll the region up, clamp y to
  //     the bottom row, and re-set the VDP write address (scrollTextUp leaves
  //     the address one byte past the region).
  //
  //  3) Bottom-of-screen with autoscroll off: clamp y to _vdpCursorMaxY and
  //     re-set the VDP write address.
  //
  // Returns a pointer to the new position in the text buffer so the caller can
  // keep its mirror pointer and the VDP's internal counter in sync.
  // **************************************************************************
  static uint8_t *_vdp_handleEOL() {

    vdp_cursor.x = 0;
    vdp_cursor.y++;

    if (_autoScroll && vdp_cursor.y > _autoScrollBottomRow) {

      vdp_scrollTextUp(_autoScrollTopRow, _autoScrollBottomRow);
      vdp_cursor.y = _autoScrollBottomRow;

      uint16_t off = _vdpRowOffset[vdp_cursor.y];

      vdp_setWriteAddress(_vdpPatternNameTableAddr + off);

      return _vdp_textBuffer + off;
    }

    if (vdp_cursor.y > VDP_MAX_Y) {

      vdp_cursor.y = VDP_MAX_Y;

      uint16_t off = _vdpRowOffset[VDP_MAX_Y];

      vdp_setWriteAddress(_vdpPatternNameTableAddr + off);

      return _vdp_textBuffer + off;
    }

    // Soft wrap. VDP auto-increment already advanced into the next row.
    return _vdp_textBuffer + _vdpRowOffset[vdp_cursor.y];
  }

  // **************************************************************************
  // Internal assembly helper shared by vdp_print and vdp_printPart.
  // Sets the VDP write address to the cursor position.
  //   out: de = pointer into _vdp_textBuffer at the cursor
  //        c  = vdp_cursor.x
  //        b  = _vdpCursorMaxX
  //   hl is preserved
  // **************************************************************************
  void _vdp_printSetup() __naked {

    __asm

      push hl

      ; de = _vdpRowOffset[vdp_cursor.y]
      ld a, (_vdp_cursor + 1)
      add a, a
      ld e, a
      ld d, 0
      ld hl, __vdpRowOffset
      add hl, de
      ld e, (hl)
      inc hl
      ld d, (hl)

      ; de = name offset = row offset + vdp_cursor.x
      ld a, (_vdp_cursor)
      ld c, a
      ld l, a
      ld h, 0
      add hl, de
      ex de, hl

      ; vdp_setWriteAddress(_vdpPatternNameTableAddr + name offset)
      ld hl, (__vdpPatternNameTableAddr)
      add hl, de
      ld a, l
      out (_IO_VDPLATCH), a
      ld a, h
      or 0x40
      out (_IO_VDPLATCH), a

      ; de = _vdp_textBuffer + name offset
      ld hl, __vdp_textBuffer
      add hl, de
      ex de, hl

      ld a, (__vdpCursorMaxX)
      ld b, a

      pop hl
      ret

    __endasm;
  }

  void vdp_print(uint8_t *text) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)text;

    // Fast streaming path: set the VDP write address once, then push bytes
    // and let the chip's auto-incrementing address counter advance through
    // VRAM. The cold path (right-margin wrap) is handled by _vdp_handleEOL.
    //
    // Written in assembly because the C version ran out of registers and kept
    // the text pointer, buffer pointer and column on the stack. Here everything
    // lives in registers: hl = text, de = text buffer, c = column, b = max column.
    // About 72 T-states per character, well above the VDP minimum access time.
    __asm

      ld a, (hl)
      or a
      ret z

      call __vdp_printSetup

    vdpPrint_loop:

      ld a, (hl)
      or a
      jr z, vdpPrint_done

      out (_IO_VDPDATA), a
      ld (de), a

      inc de
      inc hl
      inc c

      ; column past the right margin?
      ld a, b
      cp c
      jr nc, vdpPrint_loop

      push hl
      push bc
      call __vdp_handleEOL
      ex de, hl
      pop bc
      pop hl

      ld c, 0
      jr vdpPrint_loop

    vdpPrint_done:

      ld a, c
      ld (_vdp_cursor), a

      ret

    __endasm;
  }

  void vdp_printJustified(uint8_t *text, uint8_t leftMargin, uint8_t rightMargin) {

    while (*text != 0x00) {

      if (*text == ' ') {

        text++;

        uint8_t *startOfNextWord = text;

        // Find the length of the next word
        while (*startOfNextWord != ' '  &&
              *startOfNextWord != '.'  &&
              *startOfNextWord != ','  &&
              *startOfNextWord != '!'  &&
              *startOfNextWord != 0x00)
          startOfNextWord++;

        // Calculate the length of the next word
        uint8_t nextWordLength = startOfNextWord - text;

        // Check if the next word exceeds the screen width
        if (vdp_cursor.x + nextWordLength >= rightMargin) {

          vdp_newLine();
          vdp_cursor.x = leftMargin;

        } else if (vdp_cursor.x != leftMargin) {

          vdp_write(' ');
        }

        // The text ended right after the space. Writing *text here would have
        // printed the NUL and then read past the end of the string.
        if (*text == 0x00)
          break;
      }

      if (vdp_cursor.x >= rightMargin) {

        vdp_newLine();
        vdp_cursor.x = leftMargin;
      }

      vdp_write(*text);

      text++;
    }
  }

  void vdp_printColorized(uint8_t *text, uint8_t fgColor, uint8_t bgColor) {

    uint8_t *start = text;

    while (*start != 0x00) {

      vdp_colorizePattern(*start, fgColor, bgColor);

      vdp_write(*start);

      start++;
    }
  }

  void vdp_printPart(uint16_t offset, uint16_t textLength, uint8_t *text) __naked {

    // see NOTE ON (void) PARAMETERS
    (void)offset;
    (void)textLength;
    (void)text;

    // Same streaming pattern as vdp_print, terminating on length instead
    // of null. Sharing a helper for the per-character body would cost a
    // function call per byte on Z80 and undo the speedup, so the loop is
    // intentionally inlined twice. The shared cold path lives in
    // _vdp_handleEOL.
    //
    // Assembly: hl = text, iy = text buffer, de = bytes left, c = column,
    // b = max column. (IY is a scratch register for SDCC in z88dk.)
    __asm

      ld hl, 2
      add hl, sp
      ld c, (hl)
      inc hl
      ld b, (hl)
      inc hl
      ld e, (hl)
      inc hl
      ld d, (hl)
      inc hl
      ld a, (hl)
      inc hl
      ld h, (hl)
      ld l, a

      ; hl = text + offset, de = textLength
      add hl, bc

      ld a, d
      or e
      ret z

      push de
      call __vdp_printSetup
      push de
      pop iy
      pop de

    vdpPrintPart_loop:

      ld a, (hl)
      out (_IO_VDPDATA), a
      ld (iy+0), a

      inc iy
      inc hl
      inc c

      ; column past the right margin?
      ld a, b
      cp c
      jr nc, vdpPrintPart_next

      push hl
      push de
      push bc
      call __vdp_handleEOL
      push hl
      pop iy
      pop bc
      pop de
      pop hl

      ld c, 0

    vdpPrintPart_next:

      dec de
      ld a, d
      or e
      jr nz, vdpPrintPart_loop

      ld a, c
      ld (_vdp_cursor), a

      ret

    __endasm;
  }

  void vdp_newLine() {

    if (vdp_cursor.y == _autoScrollBottomRow) {

      if (_autoScroll)
        vdp_scrollTextUp(_autoScrollTopRow, _autoScrollBottomRow);

      vdp_cursor.x = 0;
    } else {

      vdp_setCursor2(0, ++vdp_cursor.y);
    }
  }

  // Inline: compiles to two OUT instructions at the call site
  inline void vdp_setBackDropColor(uint8_t color) {

    vdp_setRegister(7, color);
  }

  void vdp_setCursor2(uint8_t col, uint8_t row) {

    if (col > _vdpCursorMaxX) {

      col = 0;
      row++;
    }

    if (row > VDP_MAX_Y)
      row = VDP_MAX_Y;

    vdp_cursor.x = col;
    vdp_cursor.y = row;
  }

  void vdp_setCursor(uint8_t direction) {

    // UP and LEFT now stop at the top-left edge. Previously the unsigned
    // underflow made UP on row 0 jump to row 23, and LEFT on column 0 moved
    // the cursor DOWN a row.
    switch (direction) {
      case VDP_CURSOR_UP:

        if (vdp_cursor.y != 0)
          vdp_cursor.y--;

        break;
      case VDP_CURSOR_DOWN:
        vdp_setCursor2(vdp_cursor.x, vdp_cursor.y + 1);
        break;
      case VDP_CURSOR_LEFT:

        if (vdp_cursor.x != 0) {

          vdp_cursor.x--;
        } else if (vdp_cursor.y != 0) {

          // wrap to the end of the previous row
          vdp_cursor.x = _vdpCursorMaxX;
          vdp_cursor.y--;
        }

        break;
      case VDP_CURSOR_RIGHT:
        vdp_setCursor2(vdp_cursor.x + 1, vdp_cursor.y);
        break;
    }
  }

  void vdp_setTextColor(uint8_t fg, uint8_t bg) {

    vdp_setRegister(7, (fg << 4) + bg);
  }

  void vdp_write(uint8_t chr) __z88dk_fastcall {

    uint16_t name_offset = _VDP_NAME_OFFSET(vdp_cursor.x, vdp_cursor.y);

    vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);

    IO_VDPDATA = chr;

    _vdp_textBuffer[name_offset] = chr;

    vdp_cursor.x++;

    // Share the wrap/scroll/clamp logic with vdp_print and vdp_printPart so
    // all four print paths (write, print, printPart, printColorized) end up
    // with identical cursor behavior at the right margin.
    if (vdp_cursor.x > _vdpCursorMaxX)
      _vdp_handleEOL();
  }

  void vdp_writeCharAtLocation(uint8_t x, uint8_t y, uint8_t c) {

    uint16_t name_offset = _VDP_NAME_OFFSET(x, y);

    _vdp_textBuffer[name_offset] = c;

    vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);

    IO_VDPDATA = c;
  }

  uint8_t vdp_getCharAtLocationVRAM(uint8_t x, uint8_t y) {

    vdp_setReadAddress(_vdpPatternNameTableAddr + _VDP_NAME_OFFSET(x, y));

    return IO_VDPDATA;
  }

  // Inline: a table lookup and a load at the call site
  inline uint8_t vdp_getCharAtLocationBuf(uint8_t x, uint8_t y) {

    return _vdp_textBuffer[_VDP_NAME_OFFSET(x, y)];
  }

  void vdp_setCharAtLocationBuf(uint8_t x, uint8_t y, uint8_t c) {

    _vdp_textBuffer[_VDP_NAME_OFFSET(x, y)] = c;
  }

  void vdp_refreshViewPort() {

    vdp_setWriteAddress(_vdpPatternNameTableAddr);

    _vdp_outBlock(_vdp_textBuffer, _vdpTextBufferSize);
  }

  void vdp_scrollTextUp(uint8_t topRow, uint8_t bottomRow) {

    // Scrolls rows topRow..bottomRow (inclusive) up by one and blanks bottomRow.
    if (bottomRow > VDP_MAX_Y)
      bottomRow = VDP_MAX_Y;

    // topRow == bottomRow just blanks that row. Before, it looped through all of RAM.
    if (topRow > bottomRow)
      return;

    uint16_t name_offset = _vdpRowOffset[topRow];
    uint16_t moveLen     = _vdpRowOffset[bottomRow] - name_offset;
    uint8_t *to          = _vdp_textBuffer + name_offset;

    // Move the buffer with LDIR, blank the last row, then stream the whole
    // region to VRAM in one pass with the assembly block writer.
    memmove(to, to + _vdpCursorMaxXFull, moveLen);

    memset(to + moveLen, 0x20, _vdpCursorMaxXFull);

    vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);

    _vdp_outBlock(to, moveLen + _vdpCursorMaxXFull);
  }

  void vdp_scrollTextDown(uint8_t topRow, uint8_t bottomRow) {

    // Scrolls rows topRow..bottomRow (inclusive) down by one and blanks topRow.
    if (bottomRow > VDP_MAX_Y)
      bottomRow = VDP_MAX_Y;

    // topRow == bottomRow just blanks that row. Before, it looped through all of RAM.
    if (topRow > bottomRow)
      return;

    uint16_t name_offset = _vdpRowOffset[topRow];
    uint16_t moveLen     = _vdpRowOffset[bottomRow] - name_offset;
    uint8_t *from        = _vdp_textBuffer + name_offset;

    // memmove copies backwards (LDDR) because the regions overlap upwards
    memmove(from + _vdpCursorMaxXFull, from, moveLen);

    memset(from, 0x20, _vdpCursorMaxXFull);

    vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);

    _vdp_outBlock(from, moveLen + _vdpCursorMaxXFull);
  }

  // The binary writers shift the value left and test the top bit, instead of
  // shifting by a variable amount for every digit (very slow for 32-bit values).
  void vdp_writeUInt8ToBinary(uint8_t v) __z88dk_fastcall {

    uint8_t str[9];
    uint8_t *p = str;

    for (uint8_t i = 8; i != 0; i--) {

      *p = (v & 0x80) ? '1' : '0';

      p++;
      v <<= 1;
    }

    *p = 0x00;

    vdp_print(str);
  }

  void vdp_writeUInt16ToBinary(uint16_t v) __z88dk_fastcall {

    uint8_t str[17];
    uint8_t *p = str;

    for (uint8_t i = 16; i != 0; i--) {

      *p = (v & 0x8000) ? '1' : '0';

      p++;
      v <<= 1;
    }

    *p = 0x00;

    vdp_print(str);
  }

  void vdp_writeUInt32ToBinary(uint32_t v) __z88dk_fastcall {

    uint8_t str[33];
    uint8_t *p = str;

    for (uint8_t i = 32; i != 0; i--) {

      *p = (v & 0x80000000UL) ? '1' : '0';

      p++;
      v <<= 1;
    }

    *p = 0x00;

    vdp_print(str);
  }
#endif
