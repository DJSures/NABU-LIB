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
// - Almost every function is now hand written Z80 assembly (__naked). The C version of
//   each function is kept in the comments above it so the logic is still easy to follow.
// - Functions with one argument use __z88dk_fastcall (argument in L / HL / DEHL).
// - Functions with two or more arguments use __z88dk_callee: the function pops its own
//   arguments straight into registers. That is smaller and faster than reading them
//   from a stack frame, and the caller no longer needs cleanup code after every call,
//   which also makes every program that calls the library smaller.
// - The tiny helpers (ayWrite, ayRead, vdp_setRegister, the interrupt enable/disable,
//   etc) are macros in NABU-LIB.h, so they compile to a couple of IN/OUT instructions at
//   the call site and no unused callable copy is linked into every program.
//   vdp_setWriteAddress() stays an inline function because it uses its argument twice.
// - Bulk VRAM transfers go through the nl_outBlock / nl_fill loops (also reachable from C
//   as _vdp_outBlock / _vdp_fill). Every write to the VDP data port is at least 29
//   T-states after the previous VRAM access, which is the TMS9918A minimum access time
//   (about 8us in the graphics modes) at 3.58MHz.
// - Row offsets (row * columns) come from the _vdpRowOffset table that vdp_init() fills, so
//   cursor math never needs a software multiply.
//
// REGISTER RULES FOR THE ASSEMBLY
// -------------------------------
// - IX is the SDCC frame pointer and is never touched (or is saved and restored).
// - IY, AF, BC, DE, HL are scratch registers for SDCC in z88dk, so any function may use them.
// - The shadow registers (EXX, EX AF,AF') are not used, because a CP/M BIOS interrupt
//   handler is allowed to use them.
// - Internal helpers start with nl_ and are documented with their register contract.
//
// NOTE ON (void) PARAMETERS
// The __naked assembly functions read their arguments directly from the stack/registers.
// SDCC cannot see inside __asm blocks, so it would report every parameter as unused
// (warning 85). Each one starts with "(void)param;" which marks the parameter as used and
// generates no code. (#pragma disable_warning is not used because in this SDCC version it
// stays off for the rest of the file, which would also hide real warnings in the program
// that includes NABU-LIB.)
//
// **********************************************************************************************



// **************************************************************************
// System
// ------
// **************************************************************************

void initNABULib() __naked {

  // // Turn off the rom
  // IO_CONTROL = CONTROL_ROMSEL | CONTROL_VDOBUF;
  //
  // NABU_DisableInterrupts();
  //
  // _randomSeed = R register;
  // srand(_randomSeed);
  //
  // IM 2 with the vector table at INTERUPT_VECTOR_MAP_ADDRESS, then enable the
  // interrupts NABU-LIB uses in the AY port A interrupt mask.
  //
  // if homebrew, there are no existing interupts, so we start with a nice clean 0
  // if cpm, we get the previous interrupt settings that the BIOS set because we only override
  // what we want to use in NABULib.
  //
  // NABU_EnableInterrupts();
  //
  // initNABULIBAudio();
  __asm

      ; Turn off the rom
      ld a, CONTROL_ROMSEL | CONTROL_VDOBUF
      out (_IO_CONTROL), a

      di

      ; seed the random number generator from the refresh register
      ld a, r
      ld (__randomSeed), a
      ld l, a
      ld h, 0
      call _srand

      im 2

      ld a, INTERUPT_VECTOR_MAP_MSB
      ld i, a

    #ifndef DISABLE_HCCA_RX_INT
      ; HCCA Receive
      ld hl, _isrHCCARX
      ld (INTERUPT_VECTOR_MAP_ADDRESS), hl
    #endif

      ; // HCCA Send
      ; ld hl, _isrHCCATX
      ; ld (INTERUPT_VECTOR_MAP_ADDRESS + 2), hl

    #ifndef DISABLE_KEYBOARD_INT
      ; HCCA Keyboard
      ld hl, _isrKeyboard
      ld (INTERUPT_VECTOR_MAP_ADDRESS + 4), hl
    #endif

      ; // Video Frame Sync (see vdp_addISR)
      ; ld hl, _isrVideoSync
      ; ld (INTERUPT_VECTOR_MAP_ADDRESS + 6), hl

      ; // Option cards 0 - 3 would be at +8, +10, +12, +14

    #if BIN_TYPE == BIN_CPM
      ; keep the interrupts the CP/M BIOS already enabled
      ld a, IOPORTA
      out (_IO_AYLATCH), a
      in a, (_IO_AYDATA)
    #else
      ; a homebrew would not have existing interupts, so start with a nice clean 0
      xor a
    #endif

    #ifndef DISABLE_HCCA_RX_INT
      or INT_MASK_HCCARX
    #endif

    #ifndef DISABLE_KEYBOARD_INT
      or INT_MASK_KEYBOARD
    #endif

      ld l, a
      ld a, IOPORTA
      out (_IO_AYLATCH), a
      ld a, l
      out (_IO_AYDATA), a

      ei

      jp _initNABULIBAudio

  __endasm;
}

void initNABULIBAudio() __naked {

  // ayWrite(6, 0b00000000);   Noise envelope
  // ayWrite(8, 0b00000000);   Turn off all channels
  // ayWrite(9, 0b00000000);
  // ayWrite(10, 0b00000000);
  // ayWrite(7, 0b01111000);   Enable only the Tone generators on A B C
  // ayWrite(11, 0);
  // ayWrite(12, 0);
  // ayWrite(13, 0);
  __asm

      ld hl, nl_audioInitTable
      ld b, 16

    nl_audioInitLoop:

      ; register, then value. outi decrements b, so two per pair
      ld c, _IO_AYLATCH
      outi
      dec c
      outi
      jr nz, nl_audioInitLoop

      ret

    nl_audioInitTable:

      defb 6, 0x00
      defb 8, 0x00
      defb 9, 0x00
      defb 10, 0x00
      defb 7, 0x78
      defb 11, 0x00
      defb 12, 0x00
      defb 13, 0x00

  __endasm;
}

void RightShift(uint8_t *arr, uint16_t len, uint8_t n) __z88dk_callee __naked {

  // see NOTE ON (void) PARAMETERS
  (void)arr;
  (void)len;
  (void)n;

  // // Shifting by the whole length (or more) leaves nothing but padding. The old
  // // pointer loop never terminated in this case and walked through all of RAM.
  // if (n >= len) {
  //
  //   memset(arr, 0x20, len);
  //
  //   return;
  // }
  //
  // // memmove/memset are LDDR/LDIR and handle the overlap, which is both smaller
  // // and much faster than a byte-at-a-time C loop.
  // memmove(arr + n, arr, len - n);
  //
  // memset(arr, 0x20, n);
  __asm

      pop hl
      pop de                  ; de = arr
      pop bc                  ; bc = len
      dec sp
      pop af                  ; a = n
      push hl

      push de

      ; n >= len ? pad the whole array
      ld l, a
      ld h, 0
      or a
      sbc hl, bc
      jr nc, nl_rsFillAll

      ; LDDR from arr + len - n - 1 to arr + len - 1, len - n bytes
      ex de, hl
      add hl, bc
      dec hl
      ld d, h
      ld e, l                 ; de = arr + len - 1

      push bc
      ld c, a
      ld b, 0                 ; bc = n
      or a
      sbc hl, bc              ; hl = arr + len - n - 1
      ex (sp), hl             ; hl = len, stack = source end
      or a
      sbc hl, bc
      ld b, h
      ld c, l                 ; bc = len - n (never 0 here)
      pop hl
      lddr

      ld c, a
      ld b, 0                 ; bc = n
      pop hl                  ; hl = arr
      ld a, 0x20
      jp nl_memset

    nl_rsFillAll:

      pop hl                  ; hl = arr, bc = len
      ld a, 0x20
      jp nl_memset

  __endasm;
}

// **************************************************************************
// Shared memory helpers (register contracts in the comments)
// **************************************************************************
void _nabulib_memCore() __naked {

  __asm

    ; ------------------------------------------------------------------------
    ; nl_memset: fill bc bytes at hl with a. bc may be 0.
    ;   in:  hl = destination, bc = count, a = value
    ;   out: a kept. bc, de, hl changed
    ; ------------------------------------------------------------------------
    nl_memset:

      ld e, a
      ld a, b
      or c
      ld a, e
      ret z

      ld (hl), a
      dec bc
      ld a, b
      or c
      ld a, e
      ret z

      ld d, h
      ld e, l
      inc de
      ldir

      ret

  __endasm;
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
  // These go through the CP/M BIOS console with putchar(), so they stay in C.
  // **************************************************************************

  // Sends ESC followed by one character. putchar() is used instead of printf()
  // so a program that never calls printf() itself does not link in the whole
  // printf formatter (several KB) just for these escape codes.
  // (fastcall so each caller is just "ld l, char / jp __vt_esc")
  static void _vt_esc(uint8_t c) __z88dk_fastcall {

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
// (ayWrite and ayRead are macros in NABU-LIB.h)
// **************************************************************************

void playNoteDelay(uint8_t channel, uint8_t note, uint16_t delayLength) __z88dk_callee __naked {

  // see NOTE ON (void) PARAMETERS
  (void)channel;
  (void)note;
  (void)delayLength;

  // // Set the envelope length
  // // Note: R11 receives the high byte and R12 the low byte. That is swapped compared
  // // to the AY datasheet (R11 = fine, R12 = coarse), but existing programs have their
  // // delay values tuned to this behaviour, so it is intentionally left as is.
  // ayWrite(11, delayLength >> 8);
  // ayWrite(12, delayLength & 0xff);
  //
  // if (channel > 2)
  //   return;
  //
  // // Only the selected channel uses the envelope, the other two are silenced.
  // ayWrite(8,  (channel == 0) ? 0b00010000 : 0b00000000);
  // ayWrite(9,  (channel == 1) ? 0b00010000 : 0b00000000);
  // ayWrite(10, (channel == 2) ? 0b00010000 : 0b00000000);
  //
  // uint8_t toneReg = channel << 1;
  //
  // ayWrite(toneReg,     _NOTES_FINE[note]);
  // ayWrite(toneReg + 1, _NOTES_COURSE[note]);
  //
  // ayWrite(13, 0b00000000);
  __asm

      pop hl
      pop de                  ; e = channel, d = note
      pop bc                  ; bc = delayLength
      push hl

    ; internal entry: e = channel, d = note, bc = delayLength
    nl_playNote:

      ld a, 11
      out (_IO_AYLATCH), a
      ld a, b
      out (_IO_AYDATA), a

      ld a, 12
      out (_IO_AYLATCH), a
      ld a, c
      out (_IO_AYDATA), a

      ld a, e
      cp 3
      ret nc

      ; volume registers 8, 9, 10: 0x10 (envelope) for the selected channel, 0 for the rest
      ld b, 8

    nl_pnVolume:

      ld a, b
      out (_IO_AYLATCH), a
      sub 8
      cp e
      ld a, 0
      jr nz, nl_pnSilent
      ld a, 0x10

    nl_pnSilent:

      out (_IO_AYDATA), a
      inc b
      ld a, b
      cp 11
      jr nz, nl_pnVolume

      ; tone register = channel * 2
      ld a, e
      add a, a
      ld c, a

      ld hl, __NOTES_FINE
      ld a, d
      add a, l
      ld l, a
      adc a, h
      sub l
      ld h, a

      ld a, c
      out (_IO_AYLATCH), a
      ld a, (hl)
      out (_IO_AYDATA), a

      ld hl, __NOTES_COURSE
      ld a, d
      add a, l
      ld l, a
      adc a, h
      sub l
      ld h, a

      ld a, c
      inc a
      out (_IO_AYLATCH), a
      ld a, (hl)
      out (_IO_AYDATA), a

      ld a, 13
      out (_IO_AYLATCH), a
      xor a
      out (_IO_AYDATA), a

      ret

  __endasm;
}





// **************************************************************************
// Keyboard
// --------
// **************************************************************************

#ifndef DISABLE_KEYBOARD_INT

  uint8_t isKeyPressed() __naked {

    // return (_kbdBufferWritePos != _kbdBufferReadPos);
    __asm

      ld hl, __kbdBufferReadPos
      ld a, (__kbdBufferWritePos)
      sub (hl)
      ld l, a
      ret z

      ld l, 1
      ret

    __endasm;
  }

  uint8_t getChar() __naked {

    // Blocks until a key is waiting. While waiting, the cursor character blinks at the
    // cursor position (unless DISABLE_CURSOR, DISABLE_VDP or CURSOR_CHAR is 0).
    //
    // uint16_t cursorCnt = 0;
    //
    // while (_kbdBufferWritePos == _kbdBufferReadPos)
    //   if (CURSOR_CHAR != 0) {
    //
    //     if (cursorCnt == 1)
    //       vdp_writeCharAtLocation(vdp_cursor.x, vdp_cursor.y, CURSOR_CHAR);
    //     else if (cursorCnt == 15000)
    //       vdp_writeCharAtLocation(vdp_cursor.x, vdp_cursor.y, ' ');
    //     else if (cursorCnt > 30000)
    //       cursorCnt = 0;
    //
    //     cursorCnt++;
    //   }
    //
    // if (CURSOR_CHAR != 0)
    //   vdp_writeCharAtLocation(vdp_cursor.x, vdp_cursor.y, ' ');
    //
    // uint8_t key = _kbdBuffer[_kbdBufferReadPos];
    //
    // _kbdBufferReadPos++;
    //
    // return key;
    //
    // The wait loop is padded to the same ~155 T-states per pass as the old C loop, so
    // the cursor still blinks at the same rate (about 1.3 seconds per blink).
    __asm

    #if defined(DISABLE_CURSOR) || defined(DISABLE_VDP) || (CURSOR_CHAR == 0)

      ld hl, __kbdBufferReadPos

    nl_gcWait:

      ld a, (__kbdBufferWritePos)
      cp (hl)
      jr z, nl_gcWait

      jr nl_kbdPop

    #else

      ld bc, 0                ; cursorCnt

    nl_gcLoop:

      ld hl, __kbdBufferReadPos
      ld a, (__kbdBufferWritePos)
      cp (hl)
      jr nz, nl_gcKey

      ; cursorCnt == 1 ?
      ld a, c
      dec a
      or b
      jr z, nl_gcShow

      ; cursorCnt == 15000 (0x3a98) ?
      ld a, c
      cp 0x98
      jr nz, nl_gcWrap
      ld a, b
      cp 0x3a
      jr z, nl_gcHide

    nl_gcWrap:

      ; cursorCnt > 30000 ?
      ld hl, 30000
      or a
      sbc hl, bc
      jr nc, nl_gcNext
      ld bc, 0

    nl_gcNext:

      inc bc

      ; padding so one pass takes as long as the old C loop (152 T-states)
      ld a, (hl)
      ld a, (hl)

      jr nl_gcLoop

    nl_gcShow:

      ld a, CURSOR_CHAR
      jr nl_gcPut

    nl_gcHide:

      ld a, ' '

    nl_gcPut:

      push bc
      ld de, (_vdp_cursor)
      call nl_writeCharAt
      pop bc
      jr nl_gcNext

    nl_gcKey:

      ; erase the cursor
      ld de, (_vdp_cursor)
      ld a, ' '
      call nl_writeCharAt

    #endif

      jr nl_kbdPop

    __endasm;
  }

  uint8_t getKey() __naked {

    // Emulates CP/M BDOS call #6 (direct console input), which returns 0 if no
    // char is waiting. Reads the buffer directly instead of calling getChar()
    // because getChar() also erases the cursor character, which would overwrite
    // whatever is on screen at the cursor position every time a key is polled.
    //
    // if (_kbdBufferWritePos == _kbdBufferReadPos)
    //   return 0;
    //
    // uint8_t key = _kbdBuffer[_kbdBufferReadPos];
    //
    // _kbdBufferReadPos++;
    //
    // return key;
    __asm

      ld hl, __kbdBufferReadPos
      ld a, (__kbdBufferWritePos)
      cp (hl)
      ld l, 0
      ret z

    ; ------------------------------------------------------------------------
    ; nl_kbdPop: remove the next key from the buffer (one must be waiting)
    ;   out: l = a = key. bc and de kept
    ; ------------------------------------------------------------------------
    nl_kbdPop:

      ld a, (__kbdBufferReadPos)
      ld hl, __kbdBuffer
      add a, l
      ld l, a
      adc a, h
      sub l
      ld h, a
      ld a, (hl)

      ld hl, __kbdBufferReadPos
      inc (hl)

      ld l, a
      ret

    __endasm;
  }

  // getJoyStatus() is a macro in NABU-LIB.h

  #ifndef DISABLE_VDP

    uint8_t readLine(uint8_t *buffer, uint8_t maxInputLen) __z88dk_callee __naked {

      // see NOTE ON (void) PARAMETERS
      (void)buffer;
      (void)maxInputLen;

      // uint8_t i = 0;
      //
      // while (true) {
      //
      //   uint8_t c = getChar();
      //
      //   if (c == 13) {
      //
      //     break;
      //   } else if (i < maxInputLen && c >= 0x20 && c <= 126) {
      //
      //     buffer[i] = c;
      //
      //     vdp_write(c);
      //
      //     i++;
      //   } else if (i == maxInputLen && c != 127) {
      //
      //     playNoteDelay(0, 60, 10);
      //   } else if (c == 127 && i > 0) {
      //
      //     if (vdp_cursor.x == 0) {
      //
      //       vdp_cursor.x = _vdpCursorMaxX;   (was a fixed 39, which was wrong in 32 and 80 column modes)
      //       vdp_cursor.y--;
      //     } else {
      //
      //       vdp_cursor.x--;
      //     }
      //
      //     i--;
      //
      //     vdp_writeCharAtLocation(vdp_cursor.x, vdp_cursor.y, ' ');
      //   }
      // }
      //
      // return i;
      __asm

        pop hl
        pop de                ; de = buffer
        dec sp
        pop af                ; a = maxInputLen
        push hl

        ld b, a               ; b = maxInputLen
        ld c, 0               ; c = i

      nl_rlLoop:

        push bc
        push de
        call _getChar
        pop de
        pop bc

        ld a, l
        cp 13
        jr z, nl_rlDone

        ld h, a               ; h = key

        ; room left?
        ld a, c
        cp b
        jr nc, nl_rlFull

        ; printable?
        ld a, h
        cp 0x20
        jr c, nl_rlBackspace
        cp 127
        jr nc, nl_rlBackspace

        ; buffer[i] = c
        push de
        ld a, e
        add a, c
        ld e, a
        adc a, d
        sub e
        ld d, a
        ld a, h
        ld (de), a
        pop de

        push bc
        push de
        ld l, h
        call _vdp_write
        pop de
        pop bc

        inc c
        jr nl_rlLoop

      nl_rlFull:

        ; full, any key but backspace beeps
        ld a, h
        cp 127
        jr z, nl_rlBackspace

        push bc
        push de
        ld de, 0x3c00         ; channel 0, note 60
        ld bc, 10
        call nl_playNote
        pop de
        pop bc
        jr nl_rlLoop

      nl_rlBackspace:

        ld a, h
        cp 127
        jr nz, nl_rlLoop

        ld a, c
        or a
        jr z, nl_rlLoop

        dec c

        push bc
        push de

        ld de, (_vdp_cursor)  ; e = x, d = y
        ld a, e
        or a
        jr nz, nl_rlLeft

        ld a, (__vdpCursorMaxX)
        ld e, a
        dec d
        jr nl_rlErase

      nl_rlLeft:

        dec e

      nl_rlErase:

        ld (_vdp_cursor), de
        ld a, ' '
        call nl_writeCharAt

        pop de
        pop bc
        jr nl_rlLoop

      nl_rlDone:

        ld l, c
        ret

      __endasm;
    }
  #endif

#endif


#ifndef DISABLE_HCCA_RX_INT

  // **************************************************************************
  // HCCA Receive
  // ------------
  // (hcca_isRxBufferAvailable() is a macro in NABU-LIB.h)
  // **************************************************************************

  bool hcca_ping() __naked {

    // hcca_writeByte(0xa1);
    //
    // uint32_t timer = 0;
    //
    // while (_rxBufferWritePos == _rxBufferReadPos) {
    //
    //   timer++;
    //
    //   if (timer == 50000)
    //     return false;
    // }
    //
    // uint8_t resp = hcca_readByte();
    //
    // if (resp == 0x55)
    //   return true;
    //
    // return false;
    //
    // The timeout loop is sized to wait the same ~1.3 seconds as the old C loop.
    __asm

      ld l, 0xa1
      call _hcca_writeByte

      ld bc, 54800

    nl_hpWait:

      ld hl, __rxBufferReadPos
      ld a, (__rxBufferWritePos)
      cp (hl)
      jr nz, nl_hpGot

      ; padding so the timeout matches the old C loop
      push hl
      pop hl

      dec bc
      ld a, b
      or c
      jr nz, nl_hpWait

      ld l, 0
      ret

    nl_hpGot:

      call nl_rxPop
      sub 0x55
      ld l, 1
      ret z

      ld l, 0
      ret

    __endasm;
  }

  uint8_t hcca_getSizeOfDataInBuffer() __naked {

    // The rx ring buffer is exactly 256 bytes and the read/write positions
    // are uint8_t, so natural 8-bit wrap-around subtraction always yields
    // the correct number of bytes available, including across the wrap.
    // The previous (0xff - read) + write expression was off-by-one.
    //
    // return (uint8_t)(_rxBufferWritePos - _rxBufferReadPos);
    __asm

      ld hl, __rxBufferReadPos
      ld a, (__rxBufferWritePos)
      sub (hl)
      ld l, a
      ret

    __endasm;
  }

  uint8_t hcca_readByte() __naked {

    // while (_rxBufferWritePos == _rxBufferReadPos);
    //
    // uint8_t ret = _rxBuffer[_rxBufferReadPos];
    //
    // _rxBufferReadPos++;
    //
    // return ret;
    __asm

    ; ------------------------------------------------------------------------
    ; nl_rxPop: wait for and remove the next received byte
    ;   out: l = a = byte. bc and de kept
    ; ------------------------------------------------------------------------
    nl_rxPop:

      ld hl, __rxBufferReadPos

    nl_rxWait:

      ld a, (__rxBufferWritePos)
      cp (hl)
      jr z, nl_rxWait

      ld a, (hl)
      ld hl, __rxBuffer
      add a, l
      ld l, a
      adc a, h
      sub l
      ld h, a
      ld a, (hl)

      ld hl, __rxBufferReadPos
      inc (hl)

      ld l, a
      ret

    __endasm;
  }

  uint16_t hcca_readUInt16() __naked {

    // The low byte is read first (the wire format is LSB first).
    //
    // hcca_readInt16() is the same function (an alias label), the bits are identical.
    __asm

    _hcca_readInt16:

      call nl_rxPop
      ld c, a
      call nl_rxPop
      ld h, a
      ld l, c
      ret

    __endasm;
  }

  uint32_t hcca_readUInt32() __naked {

    // The Z80 is little-endian like the wire format, so the 4 bytes are read
    // LSB first into L, H, E, D.
    //
    // hcca_readInt32() is the same function (an alias label), the bits are identical.
    __asm

    _hcca_readInt32:

      call _hcca_readUInt16
      push hl
      call _hcca_readUInt16
      ex de, hl
      pop hl
      ret

    __endasm;
  }

  void hcca_readBytes(uint16_t offset, uint16_t bufferLen, uint8_t *buffer) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)offset;
    (void)bufferLen;
    (void)buffer;

    // uint8_t *start = buffer + offset;
    // uint8_t *end   = start + bufferLen;
    //
    // // while (not do/while) so a bufferLen of 0 reads nothing. The do/while version
    // // read 65536 bytes for a length of 0 and hung or overwrote memory, which
    // // happened whenever the Internet Adapter replied with an empty string.
    // while (start != end) {
    //
    //   while (_rxBufferWritePos == _rxBufferReadPos);
    //
    //   *start = _rxBuffer[_rxBufferReadPos];
    //
    //   start++;
    //
    //   _rxBufferReadPos++;
    // }
    __asm

      pop hl
      pop bc                  ; bc = offset
      pop de                  ; de = bufferLen
      ex (sp), hl             ; hl = buffer (return address back on the stack)
      add hl, bc
      ex de, hl               ; de = destination
      ld b, h
      ld c, l                 ; bc = count

    nl_rbLoop:

      ld a, b
      or c
      ret z

      call nl_rxPop
      ld (de), a
      inc de
      dec bc
      jr nl_rbLoop

    __endasm;
  }


  // **************************************************************************
  // HCCA Transmit
  // -------------
  // **************************************************************************

  void hcca_writeByte(uint8_t c) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)c;

    // NABU_DisableInterrupts();
    //
    // uint8_t origIntMask = ayRead(IOPORTA);
    //
    // ayWrite(IOPORTA, INT_MASK_HCCATX);
    //
    // IO_AYLATCH = IOPORTB;
    // while (IO_AYDATA & 0x04);
    //
    // IO_HCCA = c;
    //
    // ayWrite(IOPORTA, origIntMask);
    //
    // NABU_EnableInterrupts();
    //
    // Only a, f and h are changed, the multi-byte writers depend on bc and de being kept.
    __asm

      di

      ld a, IOPORTA
      out (_IO_AYLATCH), a
      in a, (_IO_AYDATA)
      ld h, a                 ; h = original interrupt mask

      ld a, INT_MASK_HCCATX
      out (_IO_AYDATA), a

      ld a, IOPORTB
      out (_IO_AYLATCH), a

    nl_hwWait:

      in a, (_IO_AYDATA)
      and 0x04
      jr nz, nl_hwWait

      ld a, l
      out (_IO_HCCA), a

      ld a, IOPORTA
      out (_IO_AYLATCH), a
      ld a, h
      out (_IO_AYDATA), a

      ei
      ret

    __endasm;
  }

  // The multi-byte writers send the value LSB first, which matches the wire format.
  // hcca_writeInt32() and hcca_writeInt16() are alias labels of the unsigned versions,
  // the bits sent are identical.
  void hcca_writeUInt32(uint32_t val) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)val;

    __asm

    _hcca_writeInt32:

      call _hcca_writeUInt16  ; low word (hl), de is kept
      ex de, hl

    _hcca_writeInt16:
    _hcca_writeUInt16:

      push hl
      call _hcca_writeByte
      pop hl
      ld l, h
      jp _hcca_writeByte

    __endasm;
  }

  void hcca_writeString(uint8_t *str) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)str;

    // while (*str != 0x00) {
    //
    //   hcca_writeByte(*str);
    //   str++;
    // }
    __asm

    nl_hsLoop:

      ld a, (hl)
      or a
      ret z

      push hl
      ld l, a
      call _hcca_writeByte
      pop hl

      inc hl
      jr nl_hsLoop

    __endasm;
  }

  void hcca_writeBytes(uint16_t offset, uint16_t length, uint8_t *bytes) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)offset;
    (void)length;
    (void)bytes;

    // uint8_t *start = bytes + offset;
    // uint8_t *end   = start + length;
    //
    // while (start != end) {
    //
    //   hcca_writeByte(*start);
    //
    //   start++;
    // }
    __asm

      pop hl
      pop bc                  ; bc = offset
      pop de                  ; de = length
      ex (sp), hl             ; hl = bytes (return address back on the stack)
      add hl, bc              ; hl = source

    nl_hbLoop:

      ld a, d
      or e
      ret z

      push hl
      ld l, (hl)
      call _hcca_writeByte
      pop hl

      inc hl
      dec de
      jr nl_hbLoop

    __endasm;
  }
#endif


// **************************************************************************
// VDP
// ---
// (vdp_setRegister, vdp_waitVDPReadyInt, vdp_setBackDropColor and
//  vdp_getCharAtLocationBuf are macros in NABU-LIB.h)
// **************************************************************************

#ifndef DISABLE_VDP

  // Inline: compiles to two OUT instructions at every call site.
  // (A function instead of a macro because the address is used twice.)
  inline void vdp_setWriteAddress(uint16_t address) {

    IO_VDPLATCH = address;

    IO_VDPLATCH = (address >> 8) | 0x40;
  }

  // Not inlined. After the read address is set the VDP needs a few microseconds to
  // fetch the byte, so this function waits long enough before returning that the
  // caller can read IO_VDPDATA straight away.
  void vdp_setReadAddress(uint16_t address) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)address;

    // IO_VDPLATCH = address;
    //
    // IO_VDPLATCH = (address >> 8);
    __asm

    ; ------------------------------------------------------------------------
    ; nl_rdAddr: set the VRAM read address, returning late enough for the VDP to
    ; have fetched the byte (the next IN from IO_VDPDATA is 29+ T-states away)
    ;   in:  hl = VRAM address
    ;   out: a changed. everything else kept
    ; ------------------------------------------------------------------------
    nl_rdAddr:

      ld a, l
      out (_IO_VDPLATCH), a
      ld a, h
      and 0x3f
      out (_IO_VDPLATCH), a

      push hl
      pop hl

      ret

    __endasm;
  }

  // **************************************************************************
  // Send len bytes from src to VRAM at the current VDP write address.
  // 30 T-states per byte, which is within the TMS9918A access window in every mode.
  // len may be 0. Callable from C with the normal (caller cleans up) convention.
  // **************************************************************************
  void _vdp_outBlock(uint8_t *src, uint16_t len) __naked {

    // see NOTE ON (void) PARAMETERS
    (void)src;
    (void)len;

    __asm

      pop bc
      pop hl
      pop de
      push de
      push hl
      push bc

    ; ------------------------------------------------------------------------
    ; nl_outBlock: stream bytes to VRAM at the current write address
    ;   in:  hl = source, de = count (0 is allowed)
    ;   out: a, bc, de, hl changed
    ; ------------------------------------------------------------------------
    nl_outBlock:

      ld a, d
      or e
      ret z

      ; b = inner count (0 means 256), d = outer passes
      ld b, e
      dec de
      inc d
      ld c, _IO_VDPDATA

    nl_obLoop:

      outi
      nop
      jp nz, nl_obLoop

      dec d
      jp nz, nl_obLoop

      ret

    __endasm;
  }

  // **************************************************************************
  // Write value to VRAM len times at the current VDP write address.
  // 32 T-states per byte. len may be 0. Callable from C with the normal convention.
  // **************************************************************************
  void _vdp_fill(uint16_t len, uint8_t value) __naked {

    // see NOTE ON (void) PARAMETERS
    (void)len;
    (void)value;

    __asm

      pop bc
      pop de
      pop hl
      push hl
      push de
      push bc
      ld a, l

    ; ------------------------------------------------------------------------
    ; nl_fill: write the same value to VRAM at the current write address
    ;   in:  de = count (0 is allowed), a = value
    ;   out: a and hl kept. bc, de changed
    ; ------------------------------------------------------------------------
    nl_fill:

      ld c, a
      ld a, d
      or e
      ld a, c
      ret z

      ld b, e
      dec de
      inc d
      ld c, _IO_VDPDATA

    nl_flLoop:

      out (c), a
      cp 0                    ; padding to 32 T-states per byte
      djnz nl_flLoop

      dec d
      jr nz, nl_flLoop

      ret

    __endasm;
  }

  // **************************************************************************
  // Shared VDP helpers (register contracts in the comments)
  // **************************************************************************
  void _nabulib_vdpCore() __naked {

    __asm

    ; ------------------------------------------------------------------------
    ; nl_wrAddr: set the VRAM write address
    ;   in:  hl = VRAM address
    ;   out: a changed. everything else kept
    ; ------------------------------------------------------------------------
    nl_wrAddr:

      ld a, l
      out (_IO_VDPLATCH), a
      ld a, h
      or 0x40
      out (_IO_VDPLATCH), a
      ret

    ; ------------------------------------------------------------------------
    ; nl_outA: write a to VRAM. The call/return keeps back to back writes
    ; far enough apart for the VDP.
    ; ------------------------------------------------------------------------
    nl_outA:

      out (_IO_VDPDATA), a
      ret

    ; ------------------------------------------------------------------------
    ; nl_vregInc: write a to VDP register c, then c++
    ;   out: a changed, c + 1
    ; ------------------------------------------------------------------------
    nl_vregInc:

      out (_IO_VDPLATCH), a
      ld a, c
      or 0x80
      out (_IO_VDPLATCH), a
      inc c
      ret

    ; ------------------------------------------------------------------------
    ; nl_rowOff: hl = _vdpRowOffset[a]
    ;   out: a, de changed. bc kept
    ; ------------------------------------------------------------------------
    nl_rowOff:

      ld l, a
      ld h, 0
      add hl, hl
      ld de, __vdpRowOffset
      add hl, de
      ld a, (hl)
      inc hl
      ld h, (hl)
      ld l, a
      ret

    ; ------------------------------------------------------------------------
    ; nl_nameOff: hl = _vdpRowOffset[y] + x (name table / text buffer offset)
    ;   in:  e = x, d = y
    ;   out: a, de changed. bc kept
    ; ------------------------------------------------------------------------
    nl_nameOff:

      ld a, e
      ld l, d
      ld h, 0
      add hl, hl
      ld de, __vdpRowOffset
      add hl, de
      ld e, (hl)
      inc hl
      ld d, (hl)
      ld l, a
      ld h, 0
      add hl, de
      ret

    ; ------------------------------------------------------------------------
    ; nl_writeCharAt: put a character in the text buffer and on screen at x, y
    ;   in:  e = x, d = y, a = character
    ;   out: c = character. a, de, hl changed. b kept
    ; ------------------------------------------------------------------------
    nl_writeCharAt:

      ld c, a
      call nl_nameOff
      ex de, hl               ; de = offset

      ld hl, (__vdpPatternNameTableAddr)
      add hl, de
      call nl_wrAddr
      ld a, c
      out (_IO_VDPDATA), a

      ld hl, __vdp_textBuffer
      add hl, de
      ld (hl), c
      ret

    ; ------------------------------------------------------------------------
    ; nl_spriteAddr: hl = sprite attribute table address of sprite a
    ;   out: bc, de kept
    ; ------------------------------------------------------------------------
    nl_spriteAddr:

      push de
      ld l, a
      ld h, 0
      add hl, hl
      add hl, hl
      ld de, (__vdpSpriteAttributeTableAddr)
      add hl, de
      pop de
      ret

    ; ------------------------------------------------------------------------
    ; nl_loadThirds: write a block to VRAM at hl. When the pattern/color tables are
    ; split in thirds (G2 bitmap mode) the same block is also written at +2048 and +4096.
    ; datasheet 2-20 : screen is split into 3 and the tables are repeated 3 times
    ;   in:  hl = VRAM address, de = source, bc = length
    ; ------------------------------------------------------------------------
    nl_loadThirds:

      ld a, (__vdpSplitThirds)
      or a
      ld a, 1
      jr z, nl_ltLoop
      ld a, 3

    nl_ltLoop:

      push af
      push hl
      push de
      push bc

      call nl_wrAddr
      ex de, hl               ; hl = source
      ld d, b
      ld e, c                 ; de = length
      call nl_outBlock

      pop bc
      pop de
      pop hl

      ld a, h
      add a, 8                ; + 2048
      ld h, a

      pop af
      dec a
      jr nz, nl_ltLoop

      ret

    ; ------------------------------------------------------------------------
    ; nl_handleEOL: called when the cursor advances past the right margin.
    ; Does one of three things:
    ;
    ;  1) Soft wrap to the next row. The TMS9918A auto-increments its internal
    ;     write-address counter as bytes are pushed to IO_VDPDATA, and the name
    ;     table is row-major contiguous in VRAM, so the chip is already pointing
    ;     at the next rows first byte for free - no setWriteAddress needed.
    ;
    ;  2) Bottom-of-region with autoscroll on: scroll the region up, clamp y to
    ;     the bottom row, and re-set the VDP write address (scrollTextUp leaves
    ;     the address one byte past the region).
    ;
    ;  3) Bottom-of-screen with autoscroll off: clamp y to _vdpCursorMaxY and
    ;     re-set the VDP write address.
    ;
    ;   out: hl = pointer to the new position in the text buffer, so the caller
    ;        can keep its mirror pointer and the VDP internal counter in sync.
    ;        Everything but ix, iy changed.
    ; ------------------------------------------------------------------------
    nl_handleEOL:

      ld hl, _vdp_cursor
      ld (hl), 0              ; x = 0
      inc hl
      inc (hl)                ; y++

      ld a, (__autoScroll)
      or a
      jr z, nl_eolNoScroll

      ld a, (__autoScrollBottomRow)
      cp (hl)
      jr nc, nl_eolNoScroll

      ; y passed the bottom of the scroll region
      ld (hl), a
      ld d, a
      ld a, (__autoScrollTopRow)
      ld e, a
      call nl_scrollUp

      ld a, (_vdp_cursor + 1)
      jr nl_eolSetAddr

    nl_eolNoScroll:

      ld a, (hl)
      cp 24
      jr c, nl_eolSoft

      ld a, 23
      ld (hl), a

    nl_eolSetAddr:

      call nl_rowOff
      push hl
      ld de, (__vdpPatternNameTableAddr)
      add hl, de
      call nl_wrAddr
      pop hl
      ld de, __vdp_textBuffer
      add hl, de
      ret

    nl_eolSoft:

      ; Soft wrap. VDP auto-increment already advanced into the next row.
      call nl_rowOff
      ld de, __vdp_textBuffer
      add hl, de
      ret

    ; ------------------------------------------------------------------------
    ; nl_printSetup: shared by vdp_print and vdp_printPart.
    ; Sets the VDP write address to the cursor position.
    ;   out: de = pointer into _vdp_textBuffer at the cursor
    ;        c  = vdp_cursor.x
    ;        b  = _vdpCursorMaxX
    ;   hl is kept
    ; ------------------------------------------------------------------------
    nl_printSetup:

      push hl

      ; de = _vdpRowOffset[vdp_cursor.y] + vdp_cursor.x
      ld de, (_vdp_cursor)
      ld c, e
      call nl_nameOff
      ex de, hl

      ; vdp_setWriteAddress(_vdpPatternNameTableAddr + name offset)
      ld hl, (__vdpPatternNameTableAddr)
      add hl, de
      call nl_wrAddr

      ; de = _vdp_textBuffer + name offset
      ld hl, __vdp_textBuffer
      add hl, de
      ex de, hl

      ld a, (__vdpCursorMaxX)
      ld b, a

      pop hl
      ret

    ; ------------------------------------------------------------------------
    ; nl_spriteOff: the 4 attribute bytes of a disabled sprite
    ;   y = 192, x = 0, pattern name #0 (should always be empty so no false
    ;   collisions), early clock to hide behind the border
    ; ------------------------------------------------------------------------
    nl_spriteOff:

      defb 192, 0, 0, 0x80

    __endasm;
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

  void vdp_enableVDPReadyInt() __naked {

    // This is exactly vdp_addISR() with the built in "ready" ISR, so share the code
    //
    // vdp_addISR(waitVdpISR);
    __asm

      ld hl, _waitVdpISR
      jp _vdp_addISR

    __endasm;
  }

  // vdp_waitVDPReadyInt() is a macro in NABU-LIB.h

  void vdp_disableVDPReadyInt() __naked {

    // vdp_setRegister(1, _vdpReg1Val);
    //
    // NABU_DisableInterrupts();
    //
    // _vdpInterruptEnabled = false;
    //
    // // clear any existing interrupts status
    // // (reading the VDP status register clears its interrupt flag. This used to
    // // read IO_AYLATCH, which is the write-only AY latch and did nothing.)
    // vdpStatusRegVal = IO_VDPLATCH;
    //
    // ayWrite(IOPORTA, ayRead(IOPORTA) & ~INT_MASK_VDP);
    //
    // NABU_EnableInterrupts();
    //
    // vdp_removeISR() is the same function (an alias label).
    __asm

    _vdp_removeISR:

      ld a, (__vdpReg1Val)
      out (_IO_VDPLATCH), a
      ld a, 0x81
      out (_IO_VDPLATCH), a

      di

      xor a
      ld (__vdpInterruptEnabled), a

      in a, (_IO_VDPLATCH)
      ld (_vdpStatusRegVal), a

      ld a, IOPORTA
      out (_IO_AYLATCH), a
      in a, (_IO_AYDATA)
      and ~INT_MASK_VDP & 0xff
      out (_IO_AYDATA), a

      ei
      ret

    __endasm;
  }

  void vdp_addISR(void (*isr)()) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)isr;

    // NABU_DisableInterrupts();
    //
    // _vdp_ISR = isr;
    //
    // // Video Frame Sync
    // INTERUPT_VECTOR_MAP_ADDRESS + 6 = isr
    //
    // ayWrite(IOPORTA, ayRead(IOPORTA) | INT_MASK_VDP);
    //
    // NABU_EnableInterrupts();
    //
    // _vdpInterruptEnabled = true;
    // vdp_setRegister(1, _vdpReg1Val | 0b00100000 );
    __asm

      di

      ld (__vdp_ISR), hl

      ; Video Frame Sync
      ld (INTERUPT_VECTOR_MAP_ADDRESS + 6), hl

      ld a, IOPORTA
      out (_IO_AYLATCH), a
      in a, (_IO_AYDATA)
      or INT_MASK_VDP
      out (_IO_AYDATA), a

      ei

      ld a, 1
      ld (__vdpInterruptEnabled), a

      ld a, (__vdpReg1Val)
      or 0x20
      out (_IO_VDPLATCH), a
      ld a, 0x81
      out (_IO_VDPLATCH), a

      ret

    __endasm;
  }

  void vdp_init(uint8_t mode, uint8_t fgColor, uint8_t bgColor, bool big_sprites, bool magnify, bool autoScroll, bool splitThirds) __naked {

    // see NOTE ON (void) PARAMETERS
    (void)mode;
    (void)fgColor;
    (void)bgColor;
    (void)big_sprites;
    (void)magnify;
    (void)autoScroll;
    (void)splitThirds;

    // The per-mode register values come from nl_vdpModeTable (one 8 byte row per mode)
    // instead of a switch statement. This is what each mode sets:
    //
    // VDP_MODE_G2:          R0 = 0x02, R1 = 0xC0 | big<<1 | magnify, R2 = 0x06 (names 0x1800),
    //                       R4 = 0x03 if splitThirds else 0x00 (patterns 0x0000), fgColor = 0, 32 columns
    // VDP_MODE_TEXT40:      R0 = 0x00, R1 = 0xD2, R2 = 0x06 (names 0x1800), R4 = 0x00, 40 columns
    // VDP_MODE_TEXT80:      R0 = 0x04, R1 = 0xD2, R2 = 0x07 (names 0x1000), R4 = 0x00, 80 columns
    // VDP_MODE_MULTICOLOR:  R0 = 0x00, R1 = 0xC8 | big<<1 | magnify, R2 = 0x06 (names 0x1800),
    //                       R4 = 0x00, fgColor = 0, 32 columns
    // VDP_MODE_G1:          G1 has no split tables. Leaving splitThirds on would make the pattern/color
    //                       loaders write copies at +2048/+4096, over the sprite attribute and name tables,
    //                       so splitThirds is forced off.
    //                       R0 = 0x00, R1 = 0xC0 | big<<1 | magnify,
    //                       R2 = 0x05 name table 0x05 * 0x400 = 0x1400
    //                       R3 = 0x80 color table 0x80 * 0x40 = 0x2000
    //                       R4 = 0x01 pattern generator 0x01 * 0x800 = 0x0800
    //                       R5 = 0x20 sprite attributes 0x20 * 0x80 = 0x1000
    //                       R6 = 0x00 sprite patterns 0x00 * 0x800 = 0x0000
    //                       fgColor = 0, 32 columns
    //
    // G1 sets its own color and sprite table locations, so these shared defaults are only
    // applied to the other modes:
    //   R3 = 0xff if splitThirds else 0x9f (colors 0x2000), R5 = 0x36 (sprite attributes 0x1b00),
    //   R6 = 0x07 (sprite patterns 0x3800)
    //
    // Then R7 = (fgColor << 4) | (bgColor & 0x0f), the _vdpRowOffset table is filled
    // (row * columns for every row, and one past the last row, so the text functions can
    // look up a name table offset instead of multiplying), and the screen is cleared.
    __asm

      ; copy the arguments into the library globals, keep fg/bg/magnify in registers
      ld hl, 2
      add hl, sp
      ld a, (hl)
      ld (__vdpMode), a
      inc hl
      ld c, (hl)              ; c = fgColor
      inc hl
      ld b, (hl)              ; b = bgColor
      inc hl
      ld a, (hl)
      ld (__vdpSpriteSizeSelected), a
      inc hl
      ld e, (hl)              ; e = magnify
      inc hl
      ld a, (hl)
      ld (__autoScroll), a
      inc hl
      ld a, (hl)
      ld (__vdpSplitThirds), a

    ; ------------------------------------------------------------------------
    ; nl_vdpInit: shared body of vdp_init and the vdp_initXXXMode functions
    ;   in: c = fgColor, b = bgColor, e = magnify
    ;       _vdpMode, _vdpSplitThirds, _vdpSpriteSizeSelected, _autoScroll already set
    ; ------------------------------------------------------------------------
    nl_vdpInit:

      push bc                 ; fg/bg for R7

      xor a
      ld (__vdpInterruptEnabled), a
      ld h, a
      ld l, a
      ld (_vdp_cursor), hl

      ld a, (__vdpMode)
      dec a
      cp 5
      jp nc, nl_viShared      ; not a known mode: only the shared part is done

      ; hl = nl_vdpModeTable + (mode - 1) * 8
      add a, a
      add a, a
      add a, a
      ld hl, nl_vdpModeTable
      add a, l
      ld l, a
      adc a, h
      sub l
      ld h, a

      ld d, (hl)              ; d = flags
      inc hl

      ; graphic modes ignore the foreground color
      bit 1, d
      jr z, nl_viKeepFg
      pop bc
      ld c, 0
      push bc

    nl_viKeepFg:

      ; R0
      ld c, 0
      ld a, (hl)
      inc hl
      call nl_vregInc

      ; R1 = base | (big_sprites << 1) | magnify in the modes that use sprites
      ld a, (__vdpSpriteSizeSelected)
      add a, a
      or e
      bit 0, d
      jr nz, nl_viSprites
      xor a

    nl_viSprites:

      or (hl)
      inc hl
      ld (__vdpReg1Val), a
      call nl_vregInc

      ; R2
      ld a, (hl)
      inc hl
      call nl_vregInc

      ; R4, or 0x03 in G2 with split thirds
      ld a, (hl)
      inc hl
      bit 2, d
      jr z, nl_viR4
      ld b, a
      ld a, (__vdpSplitThirds)
      or a
      ld a, b
      jr z, nl_viR4
      or 0x03

    nl_viR4:

      ld c, 4
      call nl_vregInc

      ; name table and pattern generator addresses (low bytes are always 0)
      xor a
      ld (__vdpPatternNameTableAddr), a
      ld (__vdpPatternGeneratorTableAddr), a
      ld a, (hl)
      inc hl
      ld (__vdpPatternNameTableAddr + 1), a
      ld a, (hl)
      inc hl
      ld (__vdpPatternGeneratorTableAddr + 1), a

      ; columns
      ld a, (hl)
      ld (__vdpCursorMaxXFull), a
      dec a
      ld (__vdpCursorMaxX), a

      bit 3, d
      jr z, nl_viShared

      ; G1 tables
      xor a
      ld (__vdpSplitThirds), a

      ld c, 3
      ld a, 0x80
      call nl_vregInc

      ld c, 5
      ld a, 0x20
      call nl_vregInc
      xor a
      call nl_vregInc

      ld hl, 0x1000
      ld (__vdpSpriteAttributeTableAddr), hl
      ld h, l
      ld (__vdpSpriteGeneratorTableAddr), hl

      jr nl_viColor

    nl_viShared:

      ld a, (__vdpSplitThirds)
      or a
      ld a, 0x9f
      jr z, nl_viR3
      ld a, 0xff

    nl_viR3:

      ld c, 3
      call nl_vregInc

      ld c, 5
      ld a, 0x36
      call nl_vregInc
      ld a, 0x07
      call nl_vregInc

      ld hl, 0x1b00
      ld (__vdpSpriteAttributeTableAddr), hl
      ld hl, 0x3800
      ld (__vdpSpriteGeneratorTableAddr), hl

    nl_viColor:

      ld hl, 0x2000
      ld (__vdpColorTableAddr), hl

      ; R7 = (fgColor << 4) | (bgColor & 0x0f)
      pop de                  ; e = fg, d = bg
      ld a, e
      add a, a
      add a, a
      add a, a
      add a, a
      ld e, a
      ld a, d
      and 0x0f
      or e
      ld c, 7
      call nl_vregInc

      ; _vdpRowOffset[row] = row * columns, rows 0 to 24
      ld a, (__vdpCursorMaxXFull)
      ld c, a
      ld hl, __vdpRowOffset
      ld de, 0
      ld b, 25

    nl_viRow:

      ld (hl), e
      inc hl
      ld (hl), d
      inc hl
      ld a, e
      add a, c
      ld e, a
      jr nc, nl_viRowNc
      inc d

    nl_viRowNc:

      djnz nl_viRow

      ; _vdpTextBufferSize = _vdpRowOffset[24] (24 rows * columns)
      dec hl
      ld d, (hl)
      dec hl
      ld e, (hl)
      ld (__vdpTextBufferSize), de

      jp _vdp_clearScreen

    ; ------------------------------------------------------------------------
    ; one row per mode (VDP_MODE_G2 = 1 ... VDP_MODE_G1 = 5)
    ;   flags (bit 0 = sprite bits in R1, bit 1 = fgColor forced to 0,
    ;          bit 2 = R4 0x03 when split, bit 3 = G1 tables), R0, R1, R2, R4,
    ;   name table address high byte, pattern generator address high byte, columns
    ; ------------------------------------------------------------------------
    nl_vdpModeTable:

      defb 0x07, 0x02, 0xC0, 0x06, 0x00, 0x18, 0x00, 32   ; G2
      defb 0x03, 0x00, 0xC8, 0x06, 0x00, 0x18, 0x00, 32   ; MULTICOLOR
      defb 0x00, 0x00, 0xD2, 0x06, 0x00, 0x18, 0x00, 40   ; TEXT40
      defb 0x00, 0x04, 0xD2, 0x07, 0x00, 0x10, 0x00, 80   ; TEXT80
      defb 0x0B, 0x00, 0xC0, 0x05, 0x01, 0x14, 0x08, 32   ; G1

    __endasm;
  }

  void vdp_initTextMode80(uint8_t fgColor, uint8_t bgColor, bool autoScroll) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)fgColor;
    (void)bgColor;
    (void)autoScroll;

    // vdp_init(VDP_MODE_TEXT80, fgColor, bgColor , false, false, autoScroll, false);
    __asm

      ld a, 4                 ; VDP_MODE_TEXT80
      jr nl_initText

    __endasm;
  }

  void vdp_initTextMode(uint8_t fgColor, uint8_t bgColor, bool autoScroll) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)fgColor;
    (void)bgColor;
    (void)autoScroll;

    // vdp_init(VDP_MODE_TEXT40, fgColor, bgColor , false, false, autoScroll, false);
    __asm

      ld a, 3                 ; VDP_MODE_TEXT40

    nl_initText:

      ld (__vdpMode), a

      pop hl
      pop bc                  ; c = fgColor, b = bgColor
      dec sp
      pop af                  ; a = autoScroll
      push hl

      ld (__autoScroll), a
      xor a
      ld (__vdpSplitThirds), a
      ld (__vdpSpriteSizeSelected), a
      ld e, a
      jp nl_vdpInit

    __endasm;
  }

  void vdp_initG2Mode(uint8_t bgColor, bool bigSprites, bool scaleSprites, bool autoScroll, bool splitThirds) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)bgColor;
    (void)bigSprites;
    (void)scaleSprites;
    (void)autoScroll;
    (void)splitThirds;

    // vdp_init(VDP_MODE_G2, 0, bgColor, bigSprites, scaleSprites, autoScroll, splitThirds);
    __asm

      ld a, 1                 ; VDP_MODE_G2
      jr nl_initG

    __endasm;
  }

  void vdp_initG1Mode(uint8_t bgColor, bool bigSprites, bool scaleSprites, bool autoScroll, bool splitThirds) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)bgColor;
    (void)bigSprites;
    (void)scaleSprites;
    (void)autoScroll;
    (void)splitThirds;

    // splitThirds is ignored by vdp_init() for G1, it is forced to false there
    //
    // vdp_init(VDP_MODE_G1, 0, bgColor, bigSprites, scaleSprites, autoScroll, splitThirds);
    __asm

      ld a, 5                 ; VDP_MODE_G1

    nl_initG:

      ld (__vdpMode), a

      pop hl
      pop bc                  ; c = bgColor, b = bigSprites
      pop de                  ; e = scaleSprites, d = autoScroll
      dec sp
      pop af                  ; a = splitThirds
      push hl

      ld (__vdpSplitThirds), a
      ld a, b
      ld (__vdpSpriteSizeSelected), a
      ld a, d
      ld (__autoScroll), a
      ld b, c                 ; bgColor
      ld c, 0                 ; fgColor
      jp nl_vdpInit

    __endasm;
  }

  void vdp_initMultiColorMode() __naked {

    // vdp_init(VDP_MODE_MULTICOLOR, 0, 0, false, false, false, false);
    __asm

      ld a, 2                 ; VDP_MODE_MULTICOLOR
      ld (__vdpMode), a
      xor a
      ld (__autoScroll), a
      ld (__vdpSplitThirds), a
      ld (__vdpSpriteSizeSelected), a
      ld e, a
      ld b, a
      ld c, a
      jp nl_vdpInit

    __endasm;
  }

  void vdp_clearVRAM() __naked {

    // 16 KB of VRAM (0x0000 - 0x3FFF). Loop bound was 0x3FFF which left
    // the final byte uncleared, so use 0x4000 (full 16384 bytes).
    //
    // vdp_setWriteAddress(0x00);
    //
    // _vdp_fill(0x4000, 0x00);
    __asm

      ld hl, 0
      call nl_wrAddr

      ld de, 0x4000
      xor a
      jp nl_fill

    __endasm;
  }

  void vdp_initMSXMode(uint8_t bgColor) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)bgColor;

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
    //
    // R0: Mode bits, enable Graphic 2 (SCREEN 2)
    //   bit1 (M1) = 1, other mode bits 0
    //
    // R1:
    //   bit7 = 1 -> 16K VRAM
    //   bit6 = 1 -> display ON (0 when we want to blank)
    //   bit5     -> VDP frame interrupt enable (IE). Left off here, the same as
    //               vdp_init(); vdp_addISR()/vdp_enableVDPReadyInt() ORs it in.
    //               Kept on if a VDP ISR is already running.
    //   other bits: 8x8 sprites, no magnification
    //
    // _vdpReg1Val must be recorded because vdp_addISR() and vdp_disableVDPReadyInt()
    // rewrite R1 from it. Left at its old value, enabling the VDP interrupt after
    // this call would write a stale R1 (e.g. 0x20 = display off).
    //
    // R2: name (Screen Image) table base: 0x1800
    //   base = R2 * 0x400 -> 0x06 * 0x400 = 0x1800
    //
    // R3: color table base for Graphics II.
    //   For G2, R3 = 0xFF selects the color table at 0x2000 with the proper mask.
    //
    // R4: pattern generator base for Graphics II.
    //   In G2, R4 = 0x03 maps the pattern table to 0x0000 as used by SC2 files.
    //
    // R5: sprite attribute table base: 0x1B00
    //   base = R5 * 0x80 -> 0x36 * 0x80 = 0x1B00
    //
    // R6: sprite pattern table base: 0x3800
    //   base = R6 * 0x800 -> 0x07 * 0x800 = 0x3800
    //
    // R7: background / border colour (low nibble)
    //
    // Record the mode and table addresses so the rest of the library (sprites,
    // vdp_loadPatternTable/ColorTable, plotting) writes to the SC2 locations.
    // splitThirds is true because R3/R4 = 0xFF/0x03 give each third of the screen
    // its own 2K of patterns and colors, exactly like vdp_init(..., splitThirds = true).
    __asm

      ld a, l
      and 0x0f
      ld e, a                 ; e = R7

      ld a, 0xC0
      ld (__vdpReg1Val), a

      ld hl, nl_msxRegs
      ld c, 0

    nl_msxLoop:

      ld a, (hl)
      inc hl

      ; R1 keeps the frame interrupt on if a VDP ISR is running
      dec c
      jr nz, nl_msxWrite
      ld a, (__vdpInterruptEnabled)
      or a
      ld a, 0xC0
      jr z, nl_msxWrite
      or 0x20

    nl_msxWrite:

      inc c
      call nl_vregInc
      ld a, c
      cp 7
      jr nz, nl_msxLoop

      ld a, e
      call nl_vregInc

      ld a, 1                 ; VDP_MODE_G2
      ld (__vdpMode), a
      ld (__vdpSplitThirds), a
      xor a
      ld (__vdpSpriteSizeSelected), a

      ld h, a
      ld l, a
      ld (__vdpPatternGeneratorTableAddr), hl
      ld h, 0x18
      ld (__vdpPatternNameTableAddr), hl
      ld h, 0x1b
      ld (__vdpSpriteAttributeTableAddr), hl
      ld h, 0x20
      ld (__vdpColorTableAddr), hl
      ld h, 0x38
      ld (__vdpSpriteGeneratorTableAddr), hl

      ret

    nl_msxRegs:

      defb 0x02, 0xC0, 0x06, 0xFF, 0x03, 0x36, 0x07

    __endasm;
  }

  void vdp_clearScreen() __naked {

    // vdp_setWriteAddress(_vdpPatternNameTableAddr);
    //
    // uint8_t cr = 0x00;
    //
    // if (_vdpMode == VDP_MODE_TEXT40 || _vdpMode == VDP_MODE_TEXT80)
    //   cr = 0x20;
    //
    // // _vdpTextBufferSize is columns * 24 for the current mode. A size of 0 (vdp_init
    // // never called) now does nothing instead of looping through 64KB.
    // _vdp_fill(_vdpTextBufferSize, cr);
    //
    // memset(_vdp_textBuffer, 0x20, _vdpTextBufferSize);
    __asm

      ; text modes (3 and 4) clear with spaces, graphic modes with pattern 0
      ld a, (__vdpMode)
      sub 3
      cp 2
      ld a, 0x20
      jr c, nl_fillScreenA

      xor a
      call nl_fillScreenVRAM
      ld a, 0x20
      jr nl_fillScreenBuf

    __endasm;
  }

  void vdp_fillScreen(uint8_t c) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)c;

    // vdp_setWriteAddress(_vdpPatternNameTableAddr);
    //
    // _vdp_fill(_vdpTextBufferSize, c);
    //
    // memset(_vdp_textBuffer, c, _vdpTextBufferSize);
    __asm

      ld a, l

    nl_fillScreenA:

      push af
      call nl_fillScreenVRAM
      pop af

    nl_fillScreenBuf:

      ld hl, __vdp_textBuffer
      ld bc, (__vdpTextBufferSize)
      jp nl_memset

    ; nl_fillScreenVRAM: fill the name table with a (a kept)
    nl_fillScreenVRAM:

      ld hl, (__vdpPatternNameTableAddr)
      ld c, a
      call nl_wrAddr
      ld a, c
      ld de, (__vdpTextBufferSize)
      jp nl_fill

    __endasm;
  }

  void vdp_clearRows(uint8_t topRow, uint8_t bottomRow) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)topRow;
    (void)bottomRow;

    // // Clears topRow up to (but not including) bottomRow, so (0, 24) is the whole screen.
    // if (bottomRow > VDP_ROWS)
    //   bottomRow = VDP_ROWS;
    //
    // // An empty range used to wrap the do/while and clear 64KB of RAM
    // if (topRow >= bottomRow)
    //   return;
    //
    // uint16_t name_offset = _vdpRowOffset[topRow];
    // uint16_t len         = _vdpRowOffset[bottomRow] - name_offset;
    //
    // vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);
    //
    // _vdp_fill(len, 0x20);
    //
    // memset(_vdp_textBuffer + name_offset, 0x20, len);
    __asm

      pop hl
      pop de                  ; e = topRow, d = bottomRow
      push hl

      ld a, d
      cp 25
      jr c, nl_crBottomOk
      ld d, 24

    nl_crBottomOk:

      ld a, e
      cp d
      ret nc

      ld b, d
      call nl_rowOff          ; hl = _vdpRowOffset[topRow]
      push hl
      ld a, b
      call nl_rowOff          ; hl = _vdpRowOffset[bottomRow]
      pop de                  ; de = name offset
      or a
      sbc hl, de              ; hl = len

      push hl
      push de

      ld hl, (__vdpPatternNameTableAddr)
      add hl, de
      call nl_wrAddr

      pop de
      pop bc
      push de
      push bc

      ld d, b
      ld e, c
      ld a, 0x20
      call nl_fill

      pop bc                  ; bc = len
      pop de                  ; de = name offset
      ld hl, __vdp_textBuffer
      add hl, de
      jp nl_memset            ; a is still 0x20

    __endasm;
  }

  void vdp_loadASCIIFont(uint8_t *font) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)font;

    // vdp_setWriteAddress(_vdpPatternGeneratorTableAddr + 0x100);
    //
    // _vdp_outBlock(font, 768);
    __asm

      push hl
      ld hl, (__vdpPatternGeneratorTableAddr)
      inc h
      call nl_wrAddr
      pop hl

      ld de, 768
      jp nl_outBlock

    __endasm;
  }

  void vdp_loadASCIIFontWithInverse(uint8_t *font) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)font;

    // vdp_loadASCIIFont(font);
    //
    // then the same 768 bytes again, inverted, right after it:
    //
    // do {
    //
    //   IO_VDPDATA = *start ^ 0xff;
    //
    //   start++;
    // } while (start != end);
    __asm

      push hl
      call _vdp_loadASCIIFont
      pop hl

      ld de, 768

    nl_lfiLoop:

      ld a, (hl)
      cpl
      out (_IO_VDPDATA), a
      inc hl
      dec de
      ld a, d
      or e
      jr nz, nl_lfiLoop

      ret

    __endasm;
  }

  void vdp_writeCharAtLocation(uint8_t x, uint8_t y, uint8_t c) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)x;
    (void)y;
    (void)c;

    // uint16_t name_offset = _VDP_NAME_OFFSET(x, y);
    //
    // _vdp_textBuffer[name_offset] = c;
    //
    // vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);
    //
    // IO_VDPDATA = c;
    //
    // vdp_putPattern() does exactly the same, so it is an alias label.
    __asm

    _vdp_putPattern:

      pop hl
      pop de                  ; e = x, d = y
      dec sp
      pop af                  ; a = c
      push hl

      jp nl_writeCharAt

    __endasm;
  }

  void vdp_loadPatternToId(uint8_t patternId, uint8_t *pattern) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)patternId;
    (void)pattern;

    // // datasheet 2-20 : screen is split into 3 and the pattern table therefore is repeated 3 times
    // _vdp_loadThirds(_vdpPatternGeneratorTableAddr + ((uint16_t)patternId << 3), pattern, 8);
    __asm

      ld bc, __vdpPatternGeneratorTableAddr
      jr nl_loadToId

    __endasm;
  }

  void vdp_loadColorToId(uint8_t patternId, uint8_t *color) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)patternId;
    (void)color;

    // // datasheet 2-20 : screen is split into 3 and the color table therefore is repeated 3 times
    // _vdp_loadThirds(_vdpColorTableAddr + ((uint16_t)patternId << 3), color, 8);
    __asm

      ld bc, __vdpColorTableAddr

    ; bc = address of the table base address variable
    nl_loadToId:

      pop hl
      pop de                  ; e = patternId, d = source low byte
      dec sp
      pop af                  ; a = source high byte
      push hl

      ld h, a
      ld l, d
      push hl                 ; source

      ; hl = table base + patternId * 8
      ld l, e
      ld h, 0
      add hl, hl
      add hl, hl
      add hl, hl
      ld a, (bc)
      add a, l
      ld l, a
      inc bc
      ld a, (bc)
      adc a, h
      ld h, a

      pop de
      ld bc, 8
      jp nl_loadThirds

    __endasm;
  }

  void vdp_loadPatternTable(uint8_t *patternTable, uint16_t len) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)patternTable;
    (void)len;

    // // datasheet 2-20 : screen is split into 3 and the pattern table therefore is repeated 3 times
    // _vdp_loadThirds(_vdpPatternGeneratorTableAddr, patternTable, len);
    __asm

      ld hl, (__vdpPatternGeneratorTableAddr)
      jr nl_loadTable

    __endasm;
  }

  void vdp_loadColorTable(uint8_t *colorTable, uint16_t len) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)colorTable;
    (void)len;

    // // datasheet 2-20 : screen is split into 3 and the color table therefore is repeated 3 times
    // _vdp_loadThirds(_vdpColorTableAddr, colorTable, len);
    __asm

      ld hl, (__vdpColorTableAddr)

    ; hl = VRAM address
    nl_loadTable:

      pop af                  ; return address
      pop de                  ; de = table
      pop bc                  ; bc = len
      push af

      jp nl_loadThirds

    __endasm;
  }

  void vdp_colorizePattern(uint8_t patternId, uint8_t fg, uint8_t bg) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)patternId;
    (void)fg;
    (void)bg;

    // // G1 has a 32 byte color table with one byte per group of 8 patterns, so this
    // // colors patternId and the 7 other patterns in its group
    // if (_vdpMode == VDP_MODE_G1) {
    //
    //   vdp_setWriteAddress(_vdpColorTableAddr + (patternId >> 3));
    //   IO_VDPDATA = (fg << 4) | (bg & 0x0f);
    //
    //   return;
    // }
    //
    // _vdp_fillThirds(_vdpColorTableAddr + ((uint16_t)patternId << 3), 8, (fg << 4) | (bg & 0x0f));
    //
    // The 8 color bytes are built on the stack and written with nl_loadThirds, so there
    // is no separate "fill thirds" routine.
    __asm

      pop hl
      pop de                  ; e = patternId, d = fg
      dec sp
      pop af                  ; a = bg
      push hl

    ; internal entry: e = patternId, d = fg, a = bg
    nl_colorize:

      and 0x0f
      ld b, a
      ld a, d
      add a, a
      add a, a
      add a, a
      add a, a
      or b
      ld b, a                 ; b = color byte

      ld a, (__vdpMode)
      cp 5                    ; VDP_MODE_G1
      jr nz, nl_czThirds

      ld a, e
      rrca
      rrca
      rrca
      and 0x1f
      ld hl, (__vdpColorTableAddr)
      add a, l
      ld l, a
      adc a, h
      sub l
      ld h, a
      call nl_wrAddr
      ld a, b
      out (_IO_VDPDATA), a
      ret

    nl_czThirds:

      ; hl = color table + patternId * 8
      ld l, e
      ld h, 0
      add hl, hl
      add hl, hl
      add hl, hl
      ld de, (__vdpColorTableAddr)
      add hl, de

      ; 8 copies of the color byte on the stack
      ld d, b
      ld e, b
      push de
      push de
      push de
      push de
      ld de, 0
      ex de, hl
      add hl, sp
      ex de, hl               ; de = stack copy, hl = VRAM address

      ld bc, 8
      call nl_loadThirds

      ld hl, 8
      add hl, sp
      ld sp, hl
      ret

    __endasm;
  }

  void vdp_plotHires(uint8_t x, uint8_t y, uint8_t color1, uint8_t color2) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)x;
    (void)y;
    (void)color1;
    (void)color2;

    // // 8 * (x / 8) + y % 8 + 256 * (y / 8), done with masks: the low byte is
    // // (x & 0xF8) | (y & 7) and the high byte is y / 8.
    // uint16_t offset = ((uint16_t)(y >> 3) << 8) | (uint8_t)((x & 0xF8) | (y & 0x07));
    // uint8_t  mask   = 0x80 >> (x & 0x07);
    //
    // vdp_setReadAddress(_vdpPatternGeneratorTableAddr + offset);
    // uint8_t pixel = IO_VDPDATA;
    //
    // vdp_setReadAddress(_vdpColorTableAddr + offset);
    // uint8_t color = IO_VDPDATA;
    //
    // // color1 is a uint8_t color index (not a pointer). Pass 0 to plot
    // // a background-only pixel using color2; any non-zero value sets the
    // // foreground bit and uses color1 as the foreground color.
    // if (color1 != 0) {
    //
    //   pixel |= mask; // Set bit as "1"
    //   color = (color & 0x0F) | (color1 << 4);
    // } else {
    //
    //   pixel &= ~mask; // Set bit as "0"
    //   color = (color & 0xF0) | (color2 & 0x0F);
    // }
    //
    // vdp_setWriteAddress(_vdpPatternGeneratorTableAddr + offset);
    // IO_VDPDATA = pixel;
    //
    // vdp_setWriteAddress(_vdpColorTableAddr + offset);
    // IO_VDPDATA = color;
    __asm

      pop hl
      pop de                  ; e = x, d = y
      pop bc                  ; c = color1, b = color2
      push hl

      ; mask = 0x80 >> (x & 7), kept on the stack
      ld a, e
      and 7
      inc a
      ld l, a
      xor a
      scf

    nl_phMask:

      rra
      dec l
      jr nz, nl_phMask

      push af

      ; hl = offset
      ld a, d
      and 7
      ld l, a
      ld a, e
      and 0xf8
      or l
      ld l, a
      ld a, d
      rrca
      rrca
      rrca
      and 0x1f
      ld h, a

      ; stack: color table address. hl = pattern address
      push hl
      ld de, (__vdpColorTableAddr)
      add hl, de
      ex (sp), hl
      ld de, (__vdpPatternGeneratorTableAddr)
      add hl, de

      call nl_rdAddr
      in a, (_IO_VDPDATA)
      ld e, a                 ; e = pixel byte

      ex (sp), hl             ; hl = color address, stack: pattern address
      call nl_rdAddr
      in a, (_IO_VDPDATA)
      ld d, a                 ; d = color byte

      ex (sp), hl             ; hl = pattern address
      pop iy                  ; iy = color address
      pop af                  ; a = mask

      inc c
      dec c
      jr z, nl_phClear

      ; Set bit as "1"
      or e
      ld e, a
      ld a, c
      add a, a
      add a, a
      add a, a
      add a, a
      ld c, a
      ld a, d
      and 0x0f
      or c
      jr nl_phWrite

    nl_phClear:

      ; Set bit as "0"
      cpl
      and e
      ld e, a
      ld a, b
      and 0x0f
      ld c, a
      ld a, d
      and 0xf0
      or c

    nl_phWrite:

      ld d, a                 ; d = new color byte

      call nl_wrAddr
      ld a, e
      out (_IO_VDPDATA), a

      push iy
      pop hl
      call nl_wrAddr
      ld a, d
      out (_IO_VDPDATA), a

      ret

    __endasm;
  }

  void vdp_plotColor(uint8_t x, uint8_t y, uint8_t color) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)x;
    (void)y;
    (void)color;

    // // 8 * (x / 2) + y % 8 + 256 * (y / 8) without divides or multiplies
    // uint16_t offset = ((uint16_t)(y >> 3) << 8) + ((uint16_t)(x >> 1) << 3) + (y & 0x07);
    //
    // if (_vdpMode == VDP_MODE_MULTICOLOR) {
    //
    //   uint16_t addr = _vdpPatternGeneratorTableAddr + offset;
    //
    //   vdp_setReadAddress(addr);
    //   uint8_t dot = IO_VDPDATA;
    //
    //   vdp_setWriteAddress(addr);
    //
    //   if (x & 1) // Odd columns
    //     IO_VDPDATA = (dot & 0xF0) + (color & 0x0f);
    //   else
    //     IO_VDPDATA = (dot & 0x0F) + (color << 4);
    // } else if (_vdpMode == VDP_MODE_G2) {
    //
    //   // Draw bitmap
    //   vdp_setReadAddress(_vdpColorTableAddr + offset);
    //   uint8_t color_ = IO_VDPDATA;
    //
    //   if ((x & 1) == 0) {
    //
    //     // Even
    //     color_ &= 0x0F;
    //     color_ |= (color << 4);
    //   } else {
    //
    //     color_ &= 0xF0;
    //     color_ |= color & 0x0F;
    //   }
    //
    //   vdp_setWriteAddress(_vdpPatternGeneratorTableAddr + offset);
    //   IO_VDPDATA = 0xF0;
    //
    //   vdp_setWriteAddress(_vdpColorTableAddr + offset);
    //   IO_VDPDATA = color_;
    // }
    //
    // Both modes merge the color into one nibble of a byte the same way, so that part
    // is shared: b = the bits kept, c = the new bits.
    __asm

      pop hl
      pop de                  ; e = x, d = y
      dec sp
      pop af                  ; a = color
      push hl

      ld c, a
      ld a, (__vdpMode)
      cp 2                    ; VDP_MODE_MULTICOLOR
      jr z, nl_pcMode
      cp 1                    ; VDP_MODE_G2
      ret nz

    nl_pcMode:

      ld a, c
      bit 0, e
      jr nz, nl_pcOdd

      ; Even: high nibble
      add a, a
      add a, a
      add a, a
      add a, a
      ld c, a
      ld b, 0x0f
      jr nl_pcOffset

    nl_pcOdd:

      ; Odd: low nibble
      and 0x0f
      ld c, a
      ld b, 0xf0

    nl_pcOffset:

      ; hl = ((y >> 3) << 8) + ((x >> 1) << 3) + (y & 7)
      ld l, e
      srl l
      ld h, 0
      add hl, hl
      add hl, hl
      add hl, hl
      ld a, d
      and 7
      or l
      ld l, a
      ld a, d
      rrca
      rrca
      rrca
      and 0x1f
      add a, h
      ld h, a

      push hl                 ; offset

      ; multicolor reads its pattern table, G2 its color table
      ld de, (__vdpPatternGeneratorTableAddr)
      ld a, (__vdpMode)
      cp 2
      jr z, nl_pcRead
      ld de, (__vdpColorTableAddr)

    nl_pcRead:

      add hl, de
      call nl_rdAddr
      in a, (_IO_VDPDATA)
      and b
      or c
      ld c, a                 ; c = merged byte

      pop de                  ; de = offset

      ld a, (__vdpMode)
      cp 2
      jr nz, nl_pcG2

      ; multicolor: write it back to the same place
      call nl_wrAddr
      ld a, c
      out (_IO_VDPDATA), a
      ret

    nl_pcG2:

      push hl                 ; color table address
      ld hl, (__vdpPatternGeneratorTableAddr)
      add hl, de
      call nl_wrAddr
      ld a, 0xF0
      out (_IO_VDPDATA), a

      pop hl
      call nl_wrAddr
      ld a, c
      out (_IO_VDPDATA), a
      ret

    __endasm;
  }

  void vdp_disableSprite(uint8_t id) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)id;

    // uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);
    //
    // vdp_setWriteAddress(addr);
    // IO_VDPDATA = 192;        // y
    // IO_VDPDATA = 0;          // x
    // IO_VDPDATA = 0;          // pattern name #0 (should always be empty so no false collisions)
    // IO_VDPDATA = 0b10000000; // early clock to hide behind border
    //
    // The 4 bytes come from nl_spriteOff through the paced block writer. (The old C
    // version wrote them only 18 T-states apart, too fast for the VDP during the display.)
    __asm

      ld a, l
      call nl_spriteAddr
      call nl_wrAddr

      ld hl, nl_spriteOff
      ld de, 4
      jp nl_outBlock

    __endasm;
  }

  void vdp_loadSpritePatternNameTable(uint16_t numSprites, uint8_t *sprite) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)numSprites;
    (void)sprite;

    // for (uint8_t i = 0; i < 32; i++)
    //   vdp_disableSprite(i);
    //
    // vdp_setWriteAddress(_vdpSpriteGeneratorTableAddr);
    //
    // // 16x16 sprites are 32 bytes each, 8x8 sprites are 8 bytes each
    // if (_vdpSpriteSizeSelected)
    //   _vdp_outBlock(sprite, numSprites << 5);
    // else
    //   _vdp_outBlock(sprite, numSprites << 3);
    //
    // The 32 sprite attribute entries are one continuous block, so the address is set
    // once and the disabled-sprite bytes are written 32 times.
    __asm

      pop hl
      pop bc                  ; bc = numSprites
      pop de                  ; de = sprite
      push hl

      push de
      push bc

      ld hl, (__vdpSpriteAttributeTableAddr)
      call nl_wrAddr

      ld b, 32

    nl_lspOff:

      push bc
      ld hl, nl_spriteOff
      ld de, 4
      call nl_outBlock
      pop bc
      djnz nl_lspOff

      ld hl, (__vdpSpriteGeneratorTableAddr)
      call nl_wrAddr

      pop hl                  ; numSprites
      add hl, hl
      add hl, hl
      add hl, hl
      ld a, (__vdpSpriteSizeSelected)
      or a
      jr z, nl_lspSmall
      add hl, hl
      add hl, hl

    nl_lspSmall:

      ex de, hl               ; de = length
      pop hl                  ; hl = sprite
      jp nl_outBlock

    __endasm;
  }

  uint8_t vdp_spriteInit(uint8_t id, uint8_t spritePatternNameId, uint8_t x, uint8_t y, uint8_t color) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)id;
    (void)spritePatternNameId;
    (void)x;
    (void)y;
    (void)color;

    // uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);
    //
    // vdp_setWriteAddress(addr);
    //
    // IO_VDPDATA = y; // y
    //
    // IO_VDPDATA = x; // x
    //
    // if (_vdpSpriteSizeSelected)
    //   IO_VDPDATA = spritePatternNameId << 2; // 16x16 sprite location (name). big sprites take 4 8x8 blocks (datasheet 2-34)
    // else
    //   IO_VDPDATA = spritePatternNameId;     // 8x8 sprite location (name
    //
    // IO_VDPDATA = color & 0x0f;
    //
    // return id;
    __asm

      pop hl
      pop bc                  ; c = id, b = spritePatternNameId
      pop de                  ; e = x, d = y
      dec sp
      pop af                  ; a = color
      push hl

      and 0x0f
      push af

      ld a, (__vdpSpriteSizeSelected)
      or a
      jr z, nl_siSmall
      sla b
      sla b

    nl_siSmall:

      ld a, c
      call nl_spriteAddr
      call nl_wrAddr

      ld a, d                 ; y
      call nl_outA
      ld a, e                 ; x
      call nl_outA
      ld a, b                 ; name
      call nl_outA
      pop af                  ; color
      out (_IO_VDPDATA), a

      ld l, c
      ret

    __endasm;
  }

  void vdp_setSpriteColor(uint8_t id, uint8_t color) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)id;
    (void)color;

    // uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);
    //
    // vdp_setWriteAddress(addr + 3);
    //
    // IO_VDPDATA = color & 0x0f;
    __asm

      pop hl
      pop de                  ; e = id, d = color
      push hl

      ld a, e
      call nl_spriteAddr
      inc hl
      inc hl
      inc hl
      call nl_wrAddr

      ld a, d
      and 0x0f
      out (_IO_VDPDATA), a
      ret

    __endasm;
  }

  void vdp_setSpritePosition(uint8_t id, uint8_t x, uint8_t y) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)id;
    (void)x;
    (void)y;

    // uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);
    //
    // vdp_setWriteAddress(addr);
    // IO_VDPDATA = y;
    // IO_VDPDATA = x;
    __asm

      pop hl
      pop de                  ; e = id, d = x
      dec sp
      pop af                  ; a = y
      push hl

      ld b, a
      ld a, e
      call nl_spriteAddr
      call nl_wrAddr

      ld a, b
      call nl_outA
      ld a, d
      jp nl_outA

    __endasm;
  }

  void vdp_setSpritePositionAndColor(uint8_t id, uint8_t x, uint8_t y, uint8_t color) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)id;
    (void)x;
    (void)y;
    (void)color;

    // uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);
    //
    // vdp_setWriteAddress(addr);
    // IO_VDPDATA = y;
    // IO_VDPDATA = x;
    //
    // // Skip the pattern name byte so the pattern set by vdp_spriteInit() is kept.
    // // This used to write the sprite id as the pattern name, which replaced the
    // // sprite's pattern (and was always wrong for 16x16 sprites).
    // vdp_setWriteAddress(addr + 3);
    // IO_VDPDATA = color & 0x0f;
    __asm

      pop hl
      pop de                  ; e = id, d = x
      pop bc                  ; c = y, b = color
      push hl

      ld a, e
      call nl_spriteAddr
      call nl_wrAddr

      ld a, c
      call nl_outA
      ld a, d
      call nl_outA

      inc hl
      inc hl
      inc hl
      call nl_wrAddr

      ld a, b
      and 0x0f
      out (_IO_VDPDATA), a
      ret

    __endasm;
  }

  void vdp_getSpritePosition(uint8_t id, uint8_t *xpos, uint8_t *ypos) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)id;
    (void)xpos;
    (void)ypos;

    // uint16_t addr = _vdpSpriteAttributeTableAddr + ((uint16_t)id << 2);
    //
    // vdp_setReadAddress(addr);
    //
    // *ypos = IO_VDPDATA;
    //
    // *xpos = IO_VDPDATA;
    __asm

      pop hl
      dec sp
      pop af                  ; a = id
      pop de                  ; de = xpos
      pop bc                  ; bc = ypos
      push hl

      call nl_spriteAddr
      call nl_rdAddr

      in a, (_IO_VDPDATA)
      ld (bc), a

      ; give the VDP time to fetch the next byte
      push hl
      pop hl

      in a, (_IO_VDPDATA)
      ld (de), a
      ret

    __endasm;
  }
  void vdp_print(uint8_t *text) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)text;

    // Fast streaming path: set the VDP write address once, then push bytes
    // and let the chip's auto-incrementing address counter advance through
    // VRAM. The cold path (right-margin wrap) is handled by nl_handleEOL.
    //
    // Everything lives in registers: hl = text, de = text buffer, c = column,
    // b = max column. About 72 T-states per character, well above the VDP
    // minimum access time.
    __asm

      ld a, (hl)
      or a
      ret z

      call nl_printSetup

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
      call nl_handleEOL
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

  void vdp_printJustified(uint8_t *text, uint8_t leftMargin, uint8_t rightMargin) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)text;
    (void)leftMargin;
    (void)rightMargin;

    // while (*text != 0x00) {
    //
    //   if (*text == ' ') {
    //
    //     text++;
    //
    //     uint8_t *startOfNextWord = text;
    //
    //     // Find the length of the next word
    //     while (*startOfNextWord != ' '  &&
    //           *startOfNextWord != '.'  &&
    //           *startOfNextWord != ','  &&
    //           *startOfNextWord != '!'  &&
    //           *startOfNextWord != 0x00)
    //       startOfNextWord++;
    //
    //     // Calculate the length of the next word
    //     uint8_t nextWordLength = startOfNextWord - text;
    //
    //     // Check if the next word exceeds the screen width
    //     if (vdp_cursor.x + nextWordLength >= rightMargin) {
    //
    //       vdp_newLine();
    //       vdp_cursor.x = leftMargin;
    //
    //     } else if (vdp_cursor.x != leftMargin) {
    //
    //       vdp_write(' ');
    //     }
    //
    //     // The text ended right after the space. Writing *text here would have
    //     // printed the NUL and then read past the end of the string.
    //     if (*text == 0x00)
    //       break;
    //   }
    //
    //   if (vdp_cursor.x >= rightMargin) {
    //
    //     vdp_newLine();
    //     vdp_cursor.x = leftMargin;
    //   }
    //
    //   vdp_write(*text);
    //
    //   text++;
    // }
    __asm

      pop hl
      pop de                  ; de = text
      pop bc                  ; c = leftMargin, b = rightMargin
      push hl

    nl_pjLoop:

      ld a, (de)
      or a
      ret z

      cp ' '
      jr nz, nl_pjChar

      inc de

      ; Find the length of the next word
      ld h, d
      ld l, e

    nl_pjScan:

      ld a, (hl)
      or a
      jr z, nl_pjScanned
      cp ' '
      jr z, nl_pjScanned
      cp '.'
      jr z, nl_pjScanned
      cp ','
      jr z, nl_pjScanned
      cp '!'
      jr z, nl_pjScanned
      inc hl
      jr nl_pjScan

    nl_pjScanned:

      ld a, l
      sub e
      ld h, a                 ; h = nextWordLength

      ; Check if the next word exceeds the screen width (9 bit compare)
      ld a, (_vdp_cursor)
      add a, h
      jr c, nl_pjWrap
      cp b
      jr nc, nl_pjWrap

      ld a, (_vdp_cursor)
      cp c
      jr z, nl_pjSpaced

      ld a, ' '
      call nl_pjWrite
      jr nl_pjSpaced

    nl_pjWrap:

      call nl_pjNewLine

    nl_pjSpaced:

      ; The text ended right after the space
      ld a, (de)
      or a
      ret z

    nl_pjChar:

      ld a, (_vdp_cursor)
      cp b
      call nc, nl_pjNewLine

      ld a, (de)
      call nl_pjWrite

      inc de
      jr nl_pjLoop

    ; vdp_newLine(); vdp_cursor.x = leftMargin (bc, de kept)
    nl_pjNewLine:

      push bc
      push de
      call _vdp_newLine
      pop de
      pop bc
      ld a, c
      ld (_vdp_cursor), a
      ret

    ; vdp_write(a) (bc, de kept)
    nl_pjWrite:

      push bc
      push de
      ld l, a
      call _vdp_write
      pop de
      pop bc
      ret

    __endasm;
  }

  void vdp_printColorized(uint8_t *text, uint8_t fgColor, uint8_t bgColor) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)text;
    (void)fgColor;
    (void)bgColor;

    // uint8_t *start = text;
    //
    // while (*start != 0x00) {
    //
    //   vdp_colorizePattern(*start, fgColor, bgColor);
    //
    //   vdp_write(*start);
    //
    //   start++;
    // }
    __asm

      pop hl
      pop de                  ; de = text
      pop bc                  ; c = fgColor, b = bgColor
      push hl

    nl_pczLoop:

      ld a, (de)
      or a
      ret z

      push de
      push bc

      ld e, a
      ld d, c
      ld a, b
      push de                 ; e = the character
      call nl_colorize
      pop hl                  ; l = the character
      call _vdp_write

      pop bc
      pop de
      inc de
      jr nl_pczLoop

    __endasm;
  }

  void vdp_printPart(uint16_t offset, uint16_t textLength, uint8_t *text) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)offset;
    (void)textLength;
    (void)text;

    // Same streaming pattern as vdp_print, terminating on length instead
    // of null. Sharing a helper for the per-character body would cost a
    // function call per byte on Z80 and undo the speedup, so the loop is
    // intentionally inlined twice. The shared cold path lives in
    // nl_handleEOL.
    //
    // Assembly: hl = text, iy = text buffer, de = bytes left, c = column,
    // b = max column. (IY is a scratch register for SDCC in z88dk.)
    __asm

      pop hl
      pop bc                  ; bc = offset
      pop de                  ; de = textLength
      ex (sp), hl             ; hl = text (return address back on the stack)

      ; hl = text + offset, de = textLength
      add hl, bc

      ld a, d
      or e
      ret z

      push de
      call nl_printSetup
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
      call nl_handleEOL
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

  void vdp_newLine() __naked {

    // if (vdp_cursor.y == _autoScrollBottomRow) {
    //
    //   if (_autoScroll)
    //     vdp_scrollTextUp(_autoScrollTopRow, _autoScrollBottomRow);
    //
    //   vdp_cursor.x = 0;
    // } else {
    //
    //   vdp_setCursor2(0, ++vdp_cursor.y);
    // }
    __asm

      ld hl, _vdp_cursor + 1
      ld a, (__autoScrollBottomRow)
      cp (hl)
      jr nz, nl_nlDown

      ld a, (__autoScroll)
      or a
      jr z, nl_nlHome

      ld a, (__autoScrollBottomRow)
      ld d, a
      ld a, (__autoScrollTopRow)
      ld e, a
      call nl_scrollUp

    nl_nlHome:

      xor a
      ld (_vdp_cursor), a
      ret

    nl_nlDown:

      ; next row, clamped to the last row
      ld a, (hl)
      inc a
      cp 24
      jr c, nl_nlRow
      ld a, 23

    nl_nlRow:

      ld (hl), a
      dec hl
      ld (hl), 0
      ret

    __endasm;
  }

  void vdp_setCursor2(uint8_t col, uint8_t row) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)col;
    (void)row;

    // if (col > _vdpCursorMaxX) {
    //
    //   col = 0;
    //   row++;
    // }
    //
    // if (row > VDP_MAX_Y)
    //   row = VDP_MAX_Y;
    //
    // vdp_cursor.x = col;
    // vdp_cursor.y = row;
    __asm

      pop hl
      pop de                  ; e = col, d = row
      push hl

    ; internal entry: e = col, d = row
    nl_setCursor2:

      ld a, (__vdpCursorMaxX)
      cp e
      jr nc, nl_sc2Col
      ld e, 0
      inc d

    nl_sc2Col:

      ld a, d
      cp 24
      jr c, nl_sc2Row
      ld d, 23

    nl_sc2Row:

      ld (_vdp_cursor), de
      ret

    __endasm;
  }

  void vdp_setCursor(uint8_t direction) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)direction;

    // UP and LEFT stop at the top-left edge. Previously the unsigned
    // underflow made UP on row 0 jump to row 23, and LEFT on column 0 moved
    // the cursor DOWN a row.
    //
    // switch (direction) {
    //   case VDP_CURSOR_UP:
    //     if (vdp_cursor.y != 0)
    //       vdp_cursor.y--;
    //     break;
    //   case VDP_CURSOR_DOWN:
    //     vdp_setCursor2(vdp_cursor.x, vdp_cursor.y + 1);
    //     break;
    //   case VDP_CURSOR_LEFT:
    //     if (vdp_cursor.x != 0) {
    //       vdp_cursor.x--;
    //     } else if (vdp_cursor.y != 0) {
    //       // wrap to the end of the previous row
    //       vdp_cursor.x = _vdpCursorMaxX;
    //       vdp_cursor.y--;
    //     }
    //     break;
    //   case VDP_CURSOR_RIGHT:
    //     vdp_setCursor2(vdp_cursor.x + 1, vdp_cursor.y);
    //     break;
    // }
    __asm

      ld de, (_vdp_cursor)    ; e = x, d = y

      ld a, l
      or a
      jr z, nl_scUp
      dec a
      jr z, nl_scDown
      dec a
      jr z, nl_scLeft
      dec a
      ret nz

      ; VDP_CURSOR_RIGHT
      inc e
      jr nl_setCursor2

    nl_scDown:

      inc d
      jr nl_setCursor2

    nl_scUp:

      ld a, d
      or a
      ret z
      dec d
      jr nl_scStore

    nl_scLeft:

      ld a, e
      or a
      jr z, nl_scLeftWrap
      dec e
      jr nl_scStore

    nl_scLeftWrap:

      ; wrap to the end of the previous row
      ld a, d
      or a
      ret z
      dec d
      ld a, (__vdpCursorMaxX)
      ld e, a

    nl_scStore:

      ld (_vdp_cursor), de
      ret

    __endasm;
  }

  void vdp_setTextColor(uint8_t fg, uint8_t bg) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)fg;
    (void)bg;

    // vdp_setRegister(7, (fg << 4) + bg);
    __asm

      pop hl
      pop de                  ; e = fg, d = bg
      push hl

      ld a, e
      add a, a
      add a, a
      add a, a
      add a, a
      add a, d
      out (_IO_VDPLATCH), a
      ld a, 0x87
      out (_IO_VDPLATCH), a
      ret

    __endasm;
  }

  void vdp_write(uint8_t chr) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)chr;

    // uint16_t name_offset = _VDP_NAME_OFFSET(vdp_cursor.x, vdp_cursor.y);
    //
    // vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);
    //
    // IO_VDPDATA = chr;
    //
    // _vdp_textBuffer[name_offset] = chr;
    //
    // vdp_cursor.x++;
    //
    // // Share the wrap/scroll/clamp logic with vdp_print and vdp_printPart so
    // // all four print paths (write, print, printPart, printColorized) end up
    // // with identical cursor behavior at the right margin.
    // if (vdp_cursor.x > _vdpCursorMaxX)
    //   _vdp_handleEOL();
    //
    // This is the hottest single character path, so the offset math and the VDP
    // write are inlined here instead of calling nl_writeCharAt.
    __asm

      ld c, l                 ; c = chr

      ; de = _vdpRowOffset[vdp_cursor.y] + vdp_cursor.x
      ld hl, (_vdp_cursor)    ; l = x, h = y
      ld a, l
      ld l, h
      ld h, 0
      add hl, hl
      ld de, __vdpRowOffset
      add hl, de
      ld e, (hl)
      inc hl
      ld d, (hl)
      ld l, a
      ld h, 0
      add hl, de
      ex de, hl

      ; vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset)
      ld hl, (__vdpPatternNameTableAddr)
      add hl, de
      ld a, l
      out (_IO_VDPLATCH), a
      ld a, h
      or 0x40
      out (_IO_VDPLATCH), a

      ld a, c
      out (_IO_VDPDATA), a

      ld hl, __vdp_textBuffer
      add hl, de
      ld (hl), c

      ; vdp_cursor.x++, past the right margin?
      ld hl, _vdp_cursor
      inc (hl)
      ld a, (__vdpCursorMaxX)
      cp (hl)
      ret nc

      jp nl_handleEOL

    __endasm;
  }

  uint8_t vdp_getCharAtLocationVRAM(uint8_t x, uint8_t y) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)x;
    (void)y;

    // vdp_setReadAddress(_vdpPatternNameTableAddr + _VDP_NAME_OFFSET(x, y));
    //
    // return IO_VDPDATA;
    __asm

      pop hl
      pop de                  ; e = x, d = y
      push hl

      call nl_nameOff
      ld de, (__vdpPatternNameTableAddr)
      add hl, de
      call nl_rdAddr

      in a, (_IO_VDPDATA)
      ld l, a
      ret

    __endasm;
  }

  void vdp_setCharAtLocationBuf(uint8_t x, uint8_t y, uint8_t c) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)x;
    (void)y;
    (void)c;

    // _vdp_textBuffer[_VDP_NAME_OFFSET(x, y)] = c;
    __asm

      pop hl
      pop de                  ; e = x, d = y
      dec sp
      pop af                  ; a = c
      push hl

      ld b, a
      call nl_nameOff
      ld de, __vdp_textBuffer
      add hl, de
      ld (hl), b
      ret

    __endasm;
  }

  void vdp_refreshViewPort() __naked {

    // vdp_setWriteAddress(_vdpPatternNameTableAddr);
    //
    // _vdp_outBlock(_vdp_textBuffer, _vdpTextBufferSize);
    __asm

      ld hl, (__vdpPatternNameTableAddr)
      call nl_wrAddr

      ld hl, __vdp_textBuffer
      ld de, (__vdpTextBufferSize)
      jp nl_outBlock

    __endasm;
  }

  void vdp_scrollTextUp(uint8_t topRow, uint8_t bottomRow) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)topRow;
    (void)bottomRow;

    // // Scrolls rows topRow..bottomRow (inclusive) up by one and blanks bottomRow.
    // if (bottomRow > VDP_MAX_Y)
    //   bottomRow = VDP_MAX_Y;
    //
    // // topRow == bottomRow just blanks that row. Before, it looped through all of RAM.
    // if (topRow > bottomRow)
    //   return;
    //
    // uint16_t name_offset = _vdpRowOffset[topRow];
    // uint16_t moveLen     = _vdpRowOffset[bottomRow] - name_offset;
    // uint8_t *to          = _vdp_textBuffer + name_offset;
    //
    // // Move the buffer with LDIR, blank the last row, then stream the whole
    // // region to VRAM in one pass with the block writer.
    // memmove(to, to + _vdpCursorMaxXFull, moveLen);
    //
    // memset(to + moveLen, 0x20, _vdpCursorMaxXFull);
    //
    // vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);
    //
    // _vdp_outBlock(to, moveLen + _vdpCursorMaxXFull);
    __asm

      pop hl
      pop de                  ; e = topRow, d = bottomRow
      push hl

    ; internal entry: e = topRow, d = bottomRow
    nl_scrollUp:

      call nl_scrollRange
      ret c

      push de                 ; name offset
      push bc                 ; moveLen

      ld hl, __vdp_textBuffer
      add hl, de
      ex de, hl               ; de = to
      ld a, (__vdpCursorMaxXFull)
      ld l, a
      ld h, 0
      add hl, de              ; hl = to + columns

      ld a, b
      or c
      jr z, nl_suNoMove
      ldir                    ; de ends at to + moveLen

    nl_suNoMove:

      ex de, hl               ; hl = to + moveLen

    ; ------------------------------------------------------------------------
    ; nl_scrollBlank: blank one row at hl, then send the region to VRAM
    ;   stack: name offset, moveLen (top)
    ; ------------------------------------------------------------------------
    nl_scrollBlank:

      ld a, (__vdpCursorMaxXFull)
      ld c, a
      ld b, 0
      ld a, 0x20
      call nl_memset

      pop bc                  ; bc = moveLen
      pop de                  ; de = name offset

      ld hl, (__vdpPatternNameTableAddr)
      add hl, de
      call nl_wrAddr

      ld hl, __vdp_textBuffer
      add hl, de              ; hl = first row of the region

      ld a, (__vdpCursorMaxXFull)
      add a, c
      ld e, a
      ld a, b
      adc a, 0
      ld d, a                 ; de = moveLen + columns
      jp nl_outBlock

    ; ------------------------------------------------------------------------
    ; nl_scrollRange: clamp bottomRow to the last row and find the region
    ;   in:  e = topRow, d = bottomRow
    ;   out: carry set if there is nothing to do (topRow > bottomRow)
    ;        else de = _vdpRowOffset[topRow], bc = _vdpRowOffset[bottomRow] - de
    ; ------------------------------------------------------------------------
    nl_scrollRange:

      ld a, d
      cp 24
      jr c, nl_srBottomOk
      ld d, 23

    nl_srBottomOk:

      ld a, d
      cp e
      ret c

      ld b, d
      ld a, e
      call nl_rowOff
      push hl
      ld a, b
      call nl_rowOff
      pop de
      or a
      sbc hl, de
      ld b, h
      ld c, l
      or a
      ret

    __endasm;
  }

  void vdp_scrollTextDown(uint8_t topRow, uint8_t bottomRow) __z88dk_callee __naked {

    // see NOTE ON (void) PARAMETERS
    (void)topRow;
    (void)bottomRow;

    // // Scrolls rows topRow..bottomRow (inclusive) down by one and blanks topRow.
    // if (bottomRow > VDP_MAX_Y)
    //   bottomRow = VDP_MAX_Y;
    //
    // // topRow == bottomRow just blanks that row. Before, it looped through all of RAM.
    // if (topRow > bottomRow)
    //   return;
    //
    // uint16_t name_offset = _vdpRowOffset[topRow];
    // uint16_t moveLen     = _vdpRowOffset[bottomRow] - name_offset;
    // uint8_t *from        = _vdp_textBuffer + name_offset;
    //
    // // memmove copies backwards (LDDR) because the regions overlap upwards
    // memmove(from + _vdpCursorMaxXFull, from, moveLen);
    //
    // memset(from, 0x20, _vdpCursorMaxXFull);
    //
    // vdp_setWriteAddress(_vdpPatternNameTableAddr + name_offset);
    //
    // _vdp_outBlock(from, moveLen + _vdpCursorMaxXFull);
    __asm

      pop hl
      pop de                  ; e = topRow, d = bottomRow
      push hl

      call nl_scrollRange
      ret c

      push de                 ; name offset
      push bc                 ; moveLen

      ld hl, __vdp_textBuffer
      add hl, de
      push hl                 ; from

      ld a, b
      or c
      jr z, nl_sdNoMove

      ; LDDR from the end: hl = from + moveLen - 1, de = hl + columns
      add hl, bc
      dec hl
      ld a, (__vdpCursorMaxXFull)
      ld e, a
      ld d, 0
      ex de, hl
      add hl, de
      ex de, hl
      lddr

    nl_sdNoMove:

      pop hl                  ; hl = from
      jr nl_scrollBlank

    __endasm;
  }

  // The binary writers shift the value left and write '0' or '1' for the bit that falls
  // out, most significant bit first.
  void vdp_writeUInt8ToBinary(uint8_t v) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)v;

    __asm

      ld h, l
      ld b, 8
      jr nl_writeBinary

    __endasm;
  }

  void vdp_writeUInt16ToBinary(uint16_t v) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)v;

    __asm

      ld b, 16

    ; ------------------------------------------------------------------------
    ; nl_writeBinary: print the top b bits (8 or 16) of hl as '0' / '1'.
    ; The digits are built in an 18 byte buffer on the stack and sent with vdp_print.
    ; ------------------------------------------------------------------------
    nl_writeBinary:

      ex de, hl               ; de = value
      ld hl, -18
      add hl, sp
      ld sp, hl               ; hl = buffer
      push hl

    nl_wbLoop:

      ex de, hl
      add hl, hl              ; carry = next bit
      ex de, hl
      ld a, '0'
      adc a, 0
      ld (hl), a
      inc hl
      djnz nl_wbLoop

      ld (hl), 0

      pop hl
      call _vdp_print

      ld hl, 18
      add hl, sp
      ld sp, hl
      ret

    __endasm;
  }

  void vdp_writeUInt32ToBinary(uint32_t v) __z88dk_fastcall __naked {

    // see NOTE ON (void) PARAMETERS
    (void)v;

    __asm

      push hl
      ex de, hl
      ld b, 16
      call nl_writeBinary     ; high word
      pop hl
      ld b, 16
      jr nl_writeBinary       ; low word

    __endasm;
  }
#endif
