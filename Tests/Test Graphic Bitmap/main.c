// ************************************************************************
// Brick Battle
// DJ Sures (c)2023
//
// This is the progress of a game I'm making for the NABU computer.
// There are bugs, and I know about them. So you don't need to tell me :)
// I'm enjoying making it myself, so hope you understand keeping comments to yourself xD
// I shared the code so others can build their own games and use this as a starting point.
// If you make something, please let me know! We're a small NABU community and need to stick together.
//
// Have fun!
// - DJ
// ************************************************************************

#define BIN_TYPE BIN_CPM

#define DISABLE_CURSOR

#define FONT_LM80C

#define DEBUG_VDP_INT

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../NABULIB/NABU-LIB.h"
#include <math.h>
#include "../../NABULIB/patterns.h"

#define DB_BUFFER_LEN 6144
uint8_t _dBuffer[DB_BUFFER_LEN];

void invalidateScreen() {

  // The main loop drawing takes too long (probably Line()) so we
  // reset the vdpIsReady flag and wait for the next interrupt
  vdpIsReady = false;
  vdp_waitVDPReadyInt();

  vdp_setWriteAddress(_vdpPatternGeneratorTableAddr);

  __asm

    push hl;
    push de;

    ld hl, __dBuffer;
    ld de, DB_BUFFER_LEN; 

    invalidateScreenLoop:

      ld a, (hl);
      out (0xa0), a;

      inc hl;
      dec de;

      ld A, D;                 
      or E;                   
      jp nz, invalidateScreenLoop;

    pop de;
    pop hl;
  __endasm;
}

void putPixel(bool c, uint8_t x, uint8_t y) {
  
  uint8_t *buf = _dBuffer;
  buf += (8 * (x >> 3) ) + ( y % 8 ) + ( 256 * (y >> 3) ) ;
  
  if (c) 
    *buf |= 0x80 >> (x % 8); // Set bit a "1"
  else 
    *buf &= ~(0x80 >> (x % 8)); // Set bit as "0"
}

void line(bool color, int x0, int y0, int x1, int y1) {

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) >> 1;

  while (x0 != x1 || y0 != y1) {

    putPixel(color, x0, y0);
    
    int e2 = err;

    if (e2 > -dx) {

      err -= dy; 
      x0 += sx; 
    }

    if (e2 < dy) {

      err += dx; 
      y0 += sy; 
    }
  }
}

void window(bool color, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  
  line(color, x, y, w, y);
  line(color, w, y, w, h);
  line(color, w, h, x, h);
  line(color, x, h, x, y);
}

void initDisplay() {

  // blank all patterns
  // ---------------------------------------------------------
  vdp_waitVDPReadyInt();

  vdp_setWriteAddress(_vdpPatternGeneratorTableAddr);

  for (int i = 0; i < 6144; i++) {

    IO_VDPDATA = 0x00;

    _dBuffer[i] = 0x00;
  }
  
  // colorize all patterns
  // ---------------------------------------------------------
  vdp_setWriteAddress(_vdpColorTableAddr);

  for (int i = 0; i < 6144; i++)
    IO_VDPDATA = 0xf0;

  // add patterns to screen
  // ---------------------------------------------------------
  vdp_setWriteAddress(_vdpPatternNameTableAddr);

  for (int i = 0; i < 768; i++) {

    _vdp_textBuffer[i] = i % 256;

    IO_VDPDATA = i % 256;
  }
}

void main() {

  initNABULib();

  vdp_initG2Mode(0, false, false, false, true);

  vdp_enableVDPReadyInt();

  initDisplay();

  uint8_t i = 20;
  for (; i < 100; i++) {

    window(true, i, 50 , i + 60, 80);

    line(true, 20, 20, 180, 100);

    invalidateScreen();

    window(false, i, 50, i + 60, 80);
    
    if (isKeyPressed())
      break;
  }
  
  window(true, i, 50 , i + 60, 80);
  invalidateScreen();

  getChar();
}
