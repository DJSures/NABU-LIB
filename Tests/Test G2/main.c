#define BIN_TYPE BIN_CPM

#define RX_BUFFER_SIZE 156

#define FONT_LM80C

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../NABULIB/patterns.h"
#include "../../NABULIB/NABU-LIB.h"
#include "../../NABULIB/RetroNET-FileStore.h"
#include <arch/z80.h>
#include "starpattern.c"

// SPRITE DATA
unsigned char SPRITE_DATA[]={
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x03,0x04,0x08,0x28,0x68,
0x7F,0x3F,0x0F,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x80,0x40,0x20,0x28,0x2C,
0xFC,0xF8,0xE0,0x00,0x00,0x00,0x00,0x00,
0x12,0x1B,0x1F,0x7F,0xFF,0xFF,0x7F,0x3F,
0x3F,0xFF,0xFF,0xDF,0x1F,0x3F,0x7C,0x60,
0x38,0x38,0xF8,0xFC,0xFF,0xFF,0xFF,0xFD,
0xFE,0xFE,0xFC,0xFC,0xFC,0xBE,0x18,0x18,
0x00,0x0F,0x3F,0x3F,0x3F,0x7F,0x7F,0x7F,
0x3F,0x3F,0x3F,0x3F,0x3F,0x1F,0x3F,0x00,
0x00,0xCC,0xFC,0xFE,0xFE,0xFE,0xFF,0xFF,
0xFE,0xFE,0xFE,0xFC,0xFC,0xC4,0x00,0x00,
0x00,0x17,0x3F,0x3F,0x1F,0x3F,0x7F,0xFF,
0x7F,0x3F,0x3F,0x1F,0x3F,0x3F,0x33,0x01,
0x00,0xC0,0xF8,0xFC,0xFC,0xFE,0xFE,0xFC,
0xFC,0xF8,0xFC,0xFE,0xFE,0xFE,0xB4,0x00,
0x00,0x00,0x03,0x01,0x09,0x1F,0x1F,0x1F,
0x0F,0x1F,0x1F,0x1F,0x07,0x03,0x03,0x00,
0x00,0x80,0xF8,0xF8,0xFC,0xFC,0xFC,0xFC,
0xFC,0xF8,0xFC,0xFE,0xFC,0xF8,0x30,0x00,
0x00,0x00,0x00,0x00,0x00,0x07,0x07,0x07,
0x0F,0x0F,0x07,0x07,0x03,0x01,0x00,0x00,
0x00,0x00,0x60,0xE0,0xF0,0xF8,0xF8,0xF8,
0xF8,0xE0,0xE0,0xE0,0xF0,0xE0,0x00,0x00,
0x00,0x00,0x19,0x1F,0x1F,0x0F,0x1F,0x1F,
0x1F,0x3F,0x3F,0x3F,0x1B,0x01,0x00,0x00,
0x00,0x00,0x60,0xF0,0xF8,0xF8,0xFC,0xFC,
0xFC,0xFC,0xF8,0xE0,0xF0,0xE0,0x00,0x00,
0x00,0x00,0x07,0x0F,0x1F,0x3F,0x7F,0x7F,
0x7F,0x7F,0x7F,0x3F,0x1B,0x0C,0x07,0x00,
0x00,0x00,0xC0,0xE0,0xB0,0xD8,0xDC,0xEC,
0xEC,0xFC,0xFC,0xF8,0xF0,0xE0,0xC0,0x00,
0x06,0x02,0x07,0x0D,0x0F,0x06,0x03,0x01,
0x07,0x09,0x09,0x01,0x02,0x04,0x04,0x0C,
0xC0,0x80,0xC0,0x60,0xE0,0xC0,0x80,0x00,
0xC0,0x20,0x20,0x00,0x80,0x40,0x40,0x60,
0x07,0x18,0x20,0x40,0x40,0x80,0x80,0x80,
0x80,0x80,0x40,0x40,0x20,0x18,0x07,0x00,
0xC0,0x30,0x08,0x04,0x04,0x02,0x02,0x02,
0x02,0x02,0x04,0x04,0x08,0x30,0xC0,0x00};

#define SPRITE_NAME_BLANK  0
#define SPRITE_NAME_UFO    1
#define SPRITE_NAME_ROCK1  2
#define SPRITE_NAME_ROCK2  3
#define SPRITE_NAME_ROCK3  4
#define SPRITE_NAME_ROCK4  5
#define SPRITE_NAME_ROCK5  6
#define SPRITE_NAME_ROCK6  7
#define SPRITE_NAME_PELLET 8
#define SPRITE_NAME_GUY    9
#define SPRITE_NAME_SHIELD 10

#define SCREEN_MAX_X 239
#define SCREEN_MIN_X 1

#define SCREEN_MAX_Y 176
#define SCREEN_MIN_Y 1


void loadImage(uint8_t filenameLen, uint8_t *filename) {

#define BUF_SIZE 128
  uint8_t buf[BUF_SIZE];
  
  uint8_t fileId = rn_fileOpen(filenameLen, filename, OPEN_FILE_FLAG_READONLY, 0xff);

  rn_fileHandleSeek(fileId, 7, RN_SEEK_SET);

  vdp_setWriteAddress(0);

  while (true) {

    uint16_t read = rn_fileHandleReadSeq(fileId, buf, 0, BUF_SIZE);

    if (read == 0)
      break;

    for (uint16_t i = 0; i < read; i++)
      IO_VDPDATA = buf[i];
  }

  rn_fileHandleClose(fileId);
}

void main() {

  initNABULib();
  
  vdp_initTextMode(2, 0, true);
  vdp_loadASCIIFont(ASCII);

  vdp_clearScreen();

  vdp_print("HELLO WORLD. press key to advance");

  getChar();

  vdp_clearVRAM();

  vdp_initMSXMode(0);

  loadImage(41, "https://cloud.nabu.ca/resources/maija.sc2");

  getChar();

  vdp_clearVRAM();
  vdp_initG2Mode(2, true, false, true);

  vdp_loadPatternTable(PATTERN, sizeof(PATTERN));
  vdp_loadColorTable(COLOR, sizeof(COLOR));

  vdp_loadSpritePatternNameTable(11, SPRITE_DATA);
  uint8_t spriteId = vdp_spriteInit(0, SPRITE_NAME_ROCK1, 50, 50, VDP_DARK_BLUE);

  vdp_print("HELLO WORLD"); vdp_newLine();
  
  getChar();

  for (uint8_t i = 0; i < 40; i++) {

    vdp_print("HELLO ");
    vdp_write(' ' + i);
    vdp_newLine();    

    z80_delay_ms(25);
  }

  vdp_clearScreen();
  
  while (true)  {

    if (isKeyPressed() && getChar() == ' ')
      break;

    vdp_putPattern(
      rand() % _vdpCursorMaxX,
      rand() % _vdpCursorMaxY,
      rand() % 6);

    z80_delay_ms(100);
  }
}
