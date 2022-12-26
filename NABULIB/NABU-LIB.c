// ****************************************************************************************
// NABU-LIB C Library
// DJ Sures (c) 2022
// https://nabu.ca
// 
// Read the NABU-LIB.h file for details of each function in this file.
// 
// *Note: Some TMS9918 graphic functions were from: https://github.com/michalin/TMS9918_Arduino/tree/main/examples
//        Modified by DJ Sures 2022 for NABU PC
//
// **********************************************************************************************





// **************************************************************************
// System
// ------
// **************************************************************************

inline void nop() {
  __asm
  NOP
    __endasm;
}

inline void NABU_DisableInterrupts() {

  __asm
  di
    __endasm;
}

inline void NABU_EnableInterrupts() {

  __asm
  ei
    __endasm;
}






// **************************************************************************
// Sound
// -----
// **************************************************************************

inline void ayWrite(uint8_t reg, uint8_t val) {

  IO_AYLATCH = reg;

  IO_AYDATA = val;
}

inline uint8_t ayRead(uint8_t reg) {

  IO_AYLATCH = reg;

  return IO_AYDATA;
}

void initAudio() {

  // Noise envelope
  ayWrite(6, 0b00000000);

  // Set the amplitude (volume) to enveoloe
  ayWrite(8, 0b00010000);
  ayWrite(9, 0b00010000);
  ayWrite(10, 0b00010000);

  // Enable only the Tone generators on A B C
  ayWrite(7, 0b01111000);
}

void playNoteDelay(uint8_t channel, uint8_t note, uint16_t delayLength) {

  // Set the envolope length
  ayWrite(11, delayLength >> 8);
  ayWrite(12, delayLength & 0xff);

  if (channel == 0) {

    ayWrite(0x00, _NOTES_FINE[note]);
    ayWrite(0x01, _NOTES_COURSE[note]);

    ayWrite(13, 0b00000000);
  } else if (channel == 1) {

    ayWrite(0x02, _NOTES_FINE[note]);
    ayWrite(0x03, _NOTES_COURSE[note]);

    ayWrite(13, 0b00000000);
  } else {

    ayWrite(0x04, _NOTES_FINE[note]);
    ayWrite(0x05, _NOTES_COURSE[note]);

    ayWrite(13, 0b00000000);
  }
}





// **************************************************************************
// Keyboard
// --------
// **************************************************************************

uint8_t isKeyPressed() {

  if (IO_KEYBOARD_STATUS & 0x02) {

    uint8_t inKey = IO_KEYBOARD;

    if (inKey >= 0x90 && inKey <= 0x95)
      return 0x00;

    LastKeyPressed = inKey;

    return inKey;
  }

  return false;
}

uint8_t getChar() {

  uint8_t retVal = 0;

  while (retVal == 0)
    retVal = isKeyPressed();

  return retVal;
}






// **************************************************************************
// HCCA Receive
// ------------
// **************************************************************************

IM2_DEFINE_ISR(isr) {

  if (hcca_isDataAvailable()) {

    _rxBuffer[_rxWritePos] = hcca_readByte();

    _rxWritePos++;
  }
}

void hcca_enableReceiveBufferInterrupt() {

  NABU_DisableInterrupts();

  ayWrite(IOPORTA, 0b10000000);          // Enable the interrupts that we want to raise (HCCA RX) 

  im2_init((void*)0xe300);               // init the IM2 to point to the start of the table at 0xd300
  memset((void*)0xe300, 0xe4, 257);      // init the table at 0xd300 to point to 0xd4d4 (table contains 127 16-bit addresses)
  z80_bpoke(0xe4e4, 195);                // at 0xd4d4, put a JP
  z80_wpoke(0xe4e5, (unsigned int)isr);  // at 0xd4d5, put the address to JP to

  NABU_EnableInterrupts();
}

bool hcca_isRxBufferAvailable() {

  return _rxWritePos != _rxReadPos;
}

uint8_t hcca_readFromBuffer() {

  uint8_t ret = _rxBuffer[_rxReadPos];

  _rxReadPos++;

  return ret;
}

inline void hcca_receiveModeStart() {

  ayWrite(IOPORTA, 0b10000000);
}

inline bool hcca_isDataAvailable() {

  return !(ayRead(IOPORTB) & 0b00000010);
}

inline uint8_t hcca_readByte() {

  return IO_HCCA;
}






// **************************************************************************
// HCCA Transmit
// -------------
// **************************************************************************

inline void hcca_writeByte(uint8_t c) {

  //NABU_DisableInterrupts();

  //// Transmit start
  //ayWrite(IOPORTA, 0b01000000);

  IO_HCCA = c;
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();
  nop();

  //IO_AYLATCH = IOPORTB;
  //while ((IO_AYDATA & 0b00000100) == 0x04);

  //// Enable RX interrupt
  //ayWrite(IOPORTA, 0b10000000);

  //NABU_EnableInterrupts();
}

void hcca_writeString(uint8_t* str) {

  for (int i = 0; str[i] != 0x00; i++)
    hcca_writeByte(str[i]);
}

void hcca_writeBytes(uint8_t* str, uint8_t len) {

  for (int i = 0; i < len; i++)
    hcca_writeByte(str[i]);
}





// **************************************************************************
// VDP 
// ---
// **************************************************************************

inline void vdp_setRegister(unsigned char registerIndex, unsigned char value) {

  IO_VDPLATCH = value;

  IO_VDPLATCH = 0x80 | registerIndex;
}

inline void vdp_setWriteAddress(unsigned int address) {

  IO_VDPLATCH = address & 0xff;

  IO_VDPLATCH = 0x40 | (address >> 8) & 0x3f;
}

inline void vdp_setReadAddress(unsigned int address) {

  IO_VDPLATCH = address & 0xff;

  IO_VDPLATCH = (address >> 8) & 0x3f;
}

int vdp_init(uint8_t mode, uint8_t color, bool big_sprites, bool magnify) {

  _vdp_mode = mode;

  _vdp_sprite_size_sel = big_sprites;

  // Clear Ram
  vdp_setWriteAddress(0x0);

  for (int i = 0; i < 0x3FFF; i++)
    IO_VDPDATA = 0;

  switch (mode) {
  case VDP_MODE_G1:

    vdp_setRegister(0, 0x00);
    vdp_setRegister(1, 0xC0 | (big_sprites << 1) | magnify); // Ram size 16k, activate video output
    vdp_setRegister(2, 0x05); // Name table at 0x1400
    vdp_setRegister(3, 0x80); // Color, start at 0x2000
    vdp_setRegister(4, 0x01); // Pattern generator start at 0x800
    vdp_setRegister(5, 0x20); // Sprite attriutes start at 0x1000
    vdp_setRegister(6, 0x00); // Sprite pattern table at 0x000
    _vdp_sprite_pattern_table = 0;
    _vdp_pattern_table = 0x800;
    _vdp_sprite_attribute_table = 0x1000;
    _vdp_name_table = 0x1400;
    _vdp_color_table = 0x2000;
    _vdp_color_table_size = 32;

    // Initialize pattern table with ASCII patterns
    vdp_setWriteAddress(_vdp_pattern_table + 0x100);

    for (uint16_t i = 0; i < 768; i++)
      IO_VDPDATA = ASCII[i];

    break;

  case VDP_MODE_G2:

    vdp_setRegister(0, 0x02);
    vdp_setRegister(1, 0xC0 | (big_sprites << 1) | magnify); // Ram size 16k, Disable Int, 16x16 Sprites, mag off, activate video output
    vdp_setRegister(2, 0x0E); // Name table at 0x3800
    vdp_setRegister(3, 0xFF); // Color, start at 0x2000
    vdp_setRegister(4, 0x03); // Pattern generator start at 0x0
    vdp_setRegister(5, 0x76); // Sprite attriutes start at 0x3800
    vdp_setRegister(6, 0x03); // Sprite pattern table at 0x1800
    _vdp_pattern_table = 0x00;
    _vdp_sprite_pattern_table = 0x1800;
    _vdp_color_table = 0x2000;
    _vdp_name_table = 0x3800;
    _vdp_sprite_attribute_table = 0x3B00;
    _vdp_color_table_size = 0x1800;

    // Initialize pattern table with ASCII patterns
    vdp_setWriteAddress(_vdp_name_table);

    for (uint16_t i = 0; i < 768; i++)
      IO_VDPDATA = i;

    break;

  case VDP_MODE_TEXT:

    vdp_setRegister(0, 0x00);
    vdp_setRegister(1, 0xD2); // Ram size 16k, Disable Int
    vdp_setRegister(2, 0x02); // Name table at 0x800
    vdp_setRegister(4, 0x00); // Pattern table start at 0x0
    _vdp_pattern_table = 0x00;
    _vdp_name_table = 0x800;
    _vdp_crsr_max_x = 39;

    // Initialize pattern table with ASCII patterns
    vdp_setWriteAddress(_vdp_pattern_table + 0x100);

    for (uint16_t i = 0; i < 23 * 40; i++)
      _vdp_textBuffer[i] = 0x20;

    for (uint16_t i = 0; i < 768; i++)
      IO_VDPDATA = ASCII[i];

    vdp_setCursor2(0, 0);

    break;

  case VDP_MODE_MULTICOLOR:

    vdp_setRegister(0, 0x00);
    vdp_setRegister(1, 0xC8 | (big_sprites << 1) | magnify); // Ram size 16k, Multicolor
    vdp_setRegister(2, 0x05); // Name table at 0x1400
    // setRegister(3, 0xFF); // Color table not available
    vdp_setRegister(4, 0x01); // Pattern table start at 0x800
    vdp_setRegister(5, 0x76); // Sprite Attribute table at 0x1000
    vdp_setRegister(6, 0x03); // Sprites Pattern Table at 0x0
    _vdp_pattern_table = 0x800;
    _vdp_name_table = 0x1400;

    // Initialize pattern table with ASCII patterns
    vdp_setWriteAddress(_vdp_name_table);

    for (uint8_t j = 0; j < 24; j++)
      for (uint16_t i = 0; i < 32; i++)
        IO_VDPDATA = i + 32 * (j / 4);

    break;
  default:
    return VDP_ERROR; // Unsupported mode
  }

  vdp_setRegister(7, color);

  return VDP_OK;
}

void vdp_dumpFontToScreen() {

  for (uint8_t j = 0; j < 24; j++)
    for (uint16_t i = 0; i < 32; i++)
      IO_VDPDATA = i + 32 * (j / 4);
}

void vdp_colorize(uint8_t fg, uint8_t bg) {

  if (_vdp_mode != VDP_MODE_G2)
    return;

  uint16_t name_offset = vdp_cursor.y * (_vdp_crsr_max_x + 1) + vdp_cursor.x; // Position in name table
  uint16_t color_offset = name_offset << 3;                       // Offset of pattern in pattern table
  vdp_setWriteAddress(_vdp_color_table + color_offset);

  for (uint8_t i = 0; i < 8; i++)
    IO_VDPDATA = (fg << 4) + bg;
}

void vdp_plotHires(uint8_t x, uint8_t y, uint8_t color1, uint8_t color2) {

  uint16_t offset = 8 * (x / 8) + y % 8 + 256 * (y / 8);

  vdp_setReadAddress(_vdp_pattern_table + offset);
  uint8_t pixel = IO_VDPDATA;

  vdp_setReadAddress(_vdp_color_table + offset);
  uint8_t color = IO_VDPDATA;

  if (color1 != NULL) {

    pixel |= 0x80 >> (x % 8); //Set a "1"
    color = (color & 0x0F) | (color1 << 4);
  } else {

    pixel &= ~(0x80 >> (x % 8)); //Set bit as "0"
    color = (color & 0xF0) | (color2 & 0x0F);
  }

  vdp_setWriteAddress(_vdp_pattern_table + offset);
  IO_VDPDATA = pixel;

  vdp_setWriteAddress(_vdp_color_table + offset);
  IO_VDPDATA = color;
}

void vdp_plotColor(uint8_t x, uint8_t y, uint8_t color) {

  if (_vdp_mode == VDP_MODE_MULTICOLOR) {

    uint16_t addr = _vdp_pattern_table + 8 * (x / 2) + y % 8 + 256 * (y / 8);

    vdp_setReadAddress(addr);
    uint8_t dot = IO_VDPDATA;

    vdp_setWriteAddress(addr);

    if (x & 1) // Odd columns
      IO_VDPDATA = (dot & 0xF0) + (color & 0x0f);
    else
      IO_VDPDATA = (dot & 0x0F) + (color << 4);
  } else if (_vdp_mode == VDP_MODE_G2) {

    // Draw bitmap
    uint16_t offset = 8 * (x / 2) + y % 8 + 256 * (y / 8);

    vdp_setReadAddress(_vdp_color_table + offset);
    uint8_t color_ = IO_VDPDATA;

    if ((x & 1) == 0) {
      // Even 

      color_ &= 0x0F;
      color_ |= (color << 4);
    } else {

      color_ &= 0xF0;
      color_ |= color & 0x0F;
    }

    vdp_setWriteAddress(_vdp_pattern_table + offset);
    IO_VDPDATA = 0xF0;

    vdp_setWriteAddress(_vdp_color_table + offset);
    IO_VDPDATA = color_;
  }
}

void vdp_setSpritePattern(uint8_t number, const uint8_t* sprite) {

  if (_vdp_sprite_size_sel) {

    vdp_setWriteAddress(_vdp_sprite_pattern_table + 32 * number);

    for (uint8_t i = 0; i < 32; i++) {
      IO_VDPDATA = sprite[i];
    }
  } else {

    vdp_setWriteAddress(_vdp_sprite_pattern_table + 8 * number);

    for (uint8_t i = 0; i < 8; i++) {
      IO_VDPDATA = sprite[i];
    }
  }

}

void vdp_setSpriteColor(uint16_t addr, uint8_t color) {

  vdp_setReadAddress(addr + 3);

  uint8_t ecclr = IO_VDPDATA & 0x80 | (color & 0x0F);

  vdp_setWriteAddress(addr + 3);

  IO_VDPDATA = ecclr;
}

//Sprite_attributes vdp_sprite_get_attributes(uint16_t addr) {
//
//  Sprite_attributes attrs;
//
//  setReadAddress(addr);
//
//  attrs.y = IO_VDPDATA;
//  attrs.x = IO_VDPDATA;
//  attrs.name_ptr = IO_VDPDATA;
//  attrs.ecclr = IO_VDPDATA;
//
//  return attrs;
//}

void vdp_getSpritePosition(uint16_t addr, uint16_t xpos, uint8_t ypos) {

  vdp_setReadAddress(addr);

  ypos = IO_VDPDATA;

  uint8_t x = IO_VDPDATA;

  uint8_t eccr = IO_VDPDATA;

  if ((eccr & 0x80) != 0)
    xpos = x;
  else
    xpos = x + 32;
}

uint16_t vdp_spriteInit(uint8_t name, uint8_t priority, uint8_t color) {

  uint16_t addr = _vdp_sprite_attribute_table + 4 * priority;

  vdp_setWriteAddress(addr);
  IO_VDPDATA = 0;
  IO_VDPDATA = 0;

  if (_vdp_sprite_size_sel)
    IO_VDPDATA = 4 * name;
  else
    IO_VDPDATA = 4 * name;

  IO_VDPDATA = 0x80 | (color & 0xF);

  return addr;
}

uint8_t vdp_setSpritePosition(uint16_t addr, uint16_t x, uint8_t y) {

  uint8_t ec, xpos;

  if (x < 144) {

    ec = 1;
    xpos = x;
  } else {

    ec = 0;
    xpos = x - 32;
  }

  vdp_setReadAddress(addr + 3);
  uint8_t color = IO_VDPDATA & 0x0f;

  vdp_setWriteAddress(addr);
  IO_VDPDATA = y;
  IO_VDPDATA = xpos;

  vdp_setWriteAddress(addr + 3);
  IO_VDPDATA = (ec << 7) | color;

  return IO_VDPLATCH;
}

void vdp_print(uint8_t* text) {

  for (uint16_t i = 0; text[i] != 0x00; i++)
    vdp_write(text[i], true);
}

void vdp_printG2(uint8_t* text) {

  for (uint16_t i = 0; text[i] != 0x00; i++)
    vdp_writeG2(text[i], true);
}

void vdp_newLine() {

  vdp_setCursor2(0, ++vdp_cursor.y);
}

void vdp_setBackDropColor(uint8_t color) {

  vdp_setRegister(7, color);
}

void vdp_setPatternColor(uint16_t index, uint8_t fg, uint8_t bg) {

  if (_vdp_mode == VDP_MODE_G1)
    index &= 31;

  vdp_setWriteAddress(_vdp_color_table + index);
  IO_VDPDATA = (fg << 4) + bg;
}

void vdp_setCursor2(uint8_t col, uint8_t row) {

  if (col == 255) {

    col = _vdp_crsr_max_x;
    row--;
  } else if (col > _vdp_crsr_max_x) {

    col = 0;
    row++;
  }

  if (row == 255)
    row = _vdp_crsr_max_y;
  else if (row > _vdp_crsr_max_y)
    row = 0;

  vdp_cursor.x = col;
  vdp_cursor.y = row;
}

void vdp_setCursor(uint8_t direction) __z88dk_fastcall {

  switch (direction) {
  case VDP_CURSOR_UP:
    vdp_setCursor2(vdp_cursor.x, vdp_cursor.y - 1);
    break;
  case VDP_CURSOR_DOWN:
    vdp_setCursor2(vdp_cursor.x, vdp_cursor.y + 1);
    break;
  case VDP_CURSOR_LEFT:
    vdp_setCursor2(vdp_cursor.x - 1, vdp_cursor.y);
    break;
  case VDP_CURSOR_RIGHT:
    vdp_setCursor2(vdp_cursor.x + 1, vdp_cursor.y);
    break;
  }
}

void vdp_setTextColor(uint8_t fg, uint8_t bg) {

  _vdp_fgcolor = fg;

  _vdp_bgcolor = bg;

  if (_vdp_mode == VDP_MODE_TEXT)
    vdp_setRegister(7, (fg << 4) + bg);
}

void vdp_write(uint8_t chr, bool advanceNextChar) {

  // Position in name table
  uint16_t name_offset = vdp_cursor.y * (_vdp_crsr_max_x + 1) + vdp_cursor.x;

  // Offset of pattern in pattern table
  uint16_t pattern_offset = name_offset << 3;

  vdp_setWriteAddress(_vdp_name_table + name_offset);

  IO_VDPDATA = chr;

  _vdp_textBuffer[vdp_cursor.y * (_vdp_crsr_max_x + 1) + vdp_cursor.x] = chr;

  if (advanceNextChar)
    vdp_setCursor(VDP_CURSOR_RIGHT);
}

void vdp_writeG2(uint8_t chr, bool advanceNextChar) {

  if (chr < 0x20 || chr > 0x7d)
    return;

  uint16_t name_offset = vdp_cursor.y * (_vdp_crsr_max_x + 1) + vdp_cursor.x; // Position in name table
  uint16_t pattern_offset = name_offset << 3;                    // Offset of pattern in pattern table

  vdp_setWriteAddress(_vdp_pattern_table + pattern_offset);

  for (uint8_t i = 0; i < 8; i++)
    IO_VDPDATA = ASCII[((chr - 32) << 3) + i];

  if (advanceNextChar)
    vdp_setCursor(VDP_CURSOR_RIGHT);
}

void vdp_writeCharAtLocation(uint8_t x, uint8_t y, uint8_t c) {

  uint16_t name_offset = y * (_vdp_crsr_max_x + 1) + x; // Position in name table

  _vdp_textBuffer[y * (_vdp_crsr_max_x + 1) + x] = c;

  vdp_setWriteAddress(_vdp_name_table + name_offset);

  IO_VDPDATA = c;
}

uint8_t vdp_getCharAtLocationVRAM(uint8_t x, uint8_t y) {

  uint16_t name_offset = y * (_vdp_crsr_max_x + 1) + x; // Position in name table

  vdp_setReadAddress(_vdp_name_table + name_offset);

  return IO_VDPDATA;
}

inline uint8_t vdp_getCharAtLocationBuf(uint8_t x, uint8_t y) {

  return _vdp_textBuffer[y * (_vdp_crsr_max_x + 1) + x];
}

inline void vdp_setCharAtLocationBuf(uint8_t x, uint8_t y, uint8_t c) {

  _vdp_textBuffer[y * (_vdp_crsr_max_x + 1) + x] = c;
}

void vdp_scrollTextUp(uint8_t topRow, uint8_t bottomRow) {

  uint16_t name_offset = topRow * (_vdp_crsr_max_x + 1);

  vdp_setWriteAddress(_vdp_name_table + name_offset);

  for (uint8_t y = topRow; y < bottomRow; y++)
    for (uint8_t x = 0; x < 40; x++) {

      _vdp_textBuffer[name_offset] = _vdp_textBuffer[name_offset + _vdp_crsr_max_x + 1];

      IO_VDPDATA = _vdp_textBuffer[name_offset];

      name_offset++;
    }

  if (bottomRow <= _vdp_crsr_max_y)
    for (uint8_t x = 0; x < 40; x++) {

      _vdp_textBuffer[name_offset] = 0x20;

      IO_VDPDATA = 0x20;

      name_offset++;
    }
}

void vdp_clearRows(uint8_t topRow, uint8_t bottomRow) {

  uint16_t name_offset = topRow * (_vdp_crsr_max_x + 1);

  vdp_setWriteAddress(_vdp_name_table + name_offset);

  for (uint8_t y = topRow; y < bottomRow; y++)
    for (uint8_t x = 0; x < 40; x++) {

      _vdp_textBuffer[name_offset] = 0x20;

      IO_VDPDATA = 0x20;

      name_offset++;
    }
}

void vdp_writeUInt16(uint16_t v) __z88dk_fastcall {

  uint8_t tb[6];

  itoa(v, tb, 10);

  vdp_print(tb);
}

void vdp_writeInt16(int16_t v) __z88dk_fastcall {

  uint8_t tb[7];

  itoa(v, tb, 10);

  vdp_print(tb);
}

void vdp_writeUInt8(uint8_t v) __z88dk_fastcall {

  uint8_t tb[4];

  itoa(v, tb, 10);

  vdp_print(tb);
}

void vdp_writeInt8(int8_t v) __z88dk_fastcall {

  uint8_t tb[5];

  itoa(v, tb, 10);

  vdp_print(tb);
}

void vdp_writeUInt8ToBinary(uint8_t v) __z88dk_fastcall {

  uint8_t str[9];

  for (uint8_t i = 0; i < 8; i++)
    if (v >> i & 1)
      str[7 - i] = '1';
    else
      str[7 - i] = '0';

  str[8] = 0x00;

  vdp_print(str);
}

void vdp_writeUInt16ToBinary(uint16_t v) __z88dk_fastcall {

  uint8_t str[17];

  for (uint8_t i = 0; i < 16; i++)
    if (v >> i & 1)
      str[15 - i] = '1';
    else
      str[15 - i] = '0';

  str[16] = 0x00;

  vdp_print(str);
}

int vdp_initTextMode(uint8_t fgcolor, uint8_t bgcolor) {

  return vdp_init(VDP_MODE_TEXT, (fgcolor << 4) | (bgcolor & 0x0f), 0, 0);
}

int vdp_initG1Mode(uint8_t fgcolor, uint8_t bgcolor) {

  return vdp_init(VDP_MODE_G1, (fgcolor << 4) | (bgcolor & 0x0f), 0, 0);
}

int vdp_initG2Mode(bool big_sprites, bool scale_sprites) {

  return vdp_init(VDP_MODE_G2, 0x0, big_sprites, scale_sprites);
}

int vdp_initMultiColorMode() {

  return vdp_init(VDP_MODE_MULTICOLOR, 0, 0, 0);
}
