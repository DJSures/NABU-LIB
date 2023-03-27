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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../NABULIB/NABU-LIB.h"
#include <math.h>
#include "../NABULIB/patterns.h"

uint8_t _dBuffer[6144];

void invalideArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {

  vdp_waitVDPReadyInt();

  uint8_t xStart = 8 * (x >> 3);

  while (y < h) {  

    uint16_t offset = ( y % 8 ) + ( 256 * (y >> 3) ) ;

    vdp_setWriteAddress(_vdpPatternGeneratorTableAddr + offset + xStart);

    for (uint8_t i = x; i < w; i++) 
      IO_VDPDATA = _dBuffer[offset + i];
    
    y++;
  }
}

void invalidateScreen() {

  vdp_waitVDPReadyInt();

  vdp_setWriteAddress(_vdpPatternGeneratorTableAddr);

  uint8_t *start   = _dBuffer;
  uint8_t *end  = _dBuffer + 6144;

  do {

    IO_VDPDATA = *start;

    start++;
  } while (start != end);    
}

void putPixel(bool c, uint8_t x, uint8_t y) {
  
  uint16_t offset = (8 * (x >> 3) ) + ( y % 8 ) + ( 256 * (y >> 3) ) ;

  if (c) 
    _dBuffer[offset] |= 0x80 >> (x % 8); // Set bit a "1"
  else 
    _dBuffer[offset] &= ~(0x80 >> (x % 8)); // Set bit as "0"
}

void line(bool color, int x0, int y0, int x1, int y1) {

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) >> 1;

  while (putPixel(color, x0, y0), x0 != x1 || y0 != y1) {

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
  
  for (uint8_t xx = x; xx < w; xx++) {

    putPixel(color, xx, y);
  
    putPixel(color, xx, y + 1);
    putPixel(color, xx, y + 2);
    putPixel(color, xx, y + 3);
    putPixel(color, xx, y + 4);
    putPixel(color, xx, y + 5);
    putPixel(color, xx, y + 6);
    putPixel(color, xx, y + 7);
    putPixel(color, xx, y + 8);
    putPixel(color, xx, y + 9);

    putPixel(color, xx, h);
  }

  for (uint8_t yy = y; yy < h; yy++) {

    putPixel(color, x, yy);
    putPixel(color, w, yy);
  }  
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
  for (; i < 150; i++) {

    window(true, i, 50 , i + 60, 80);

    line(true, 20, 20, 180, 100);

    invalidateScreen();

    window(false, i, 50, i + 60, 80);
  }
  
  window(true, i, 50 , i + 60, 80);
  invalidateScreen();

  getChar();
}
