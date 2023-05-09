// ************************************************************************
// NABU TurtleBot
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

#define FONT_LM80C

#define DEBUG_VDP_INT

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../NABULIB/NABU-LIB.h"
//#include <math.h>
#include <string.h>
#include "../../NABULIB/patterns.h"
#include "../../NABULIB/RetroNET-FileStore.h"

#define DB_BUFFER_LEN 2048 * 2
uint8_t _dBuffer[DB_BUFFER_LEN];

#define IN_BUF_LEN 64
uint8_t _inBuf[IN_BUF_LEN] = { 0 };

#define OUT_BUF_LEN 128
uint8_t _outBuf[OUT_BUF_LEN] = { 0 };

uint8_t _tcp = 0xff;

int16_t _heading = 0;
int16_t _x        = 25600 / 2;
int16_t _y        = 6000;

void invalidateScreen() {

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

  uint16_t i;

  // blank top 2/3 patterns
  // ---------------------------------------------------------
  vdp_setWriteAddress(_vdpPatternGeneratorTableAddr);

  for (i = 0; i < 2048 * 2; i++) {

    IO_VDPDATA = 0x00;

    _dBuffer[i] = 0x00;
  }

  // load ascii font in bottom 3rd pattern
  // ---------------------------------------------------------
  vdp_setWriteAddress(_vdpPatternGeneratorTableAddr + (2048 * 2) + 0x100);

  uint8_t *start = ASCII;
  uint8_t *end = start + 768;

  do {

    IO_VDPDATA = *start;
    start++;
  } while (start != end);

  // colorize all patterns
  // ---------------------------------------------------------
  vdp_setWriteAddress(_vdpColorTableAddr);

  // Top turtle bitmap graphic
  for (i = 0; i < 4096; i++)
    IO_VDPDATA = VDP_BLACK << 4 | VDP_WHITE;

  // bottom text
  for (; i < 6144; i++)
    IO_VDPDATA = VDP_LIGHT_GREEN << 4 | VDP_BLACK;

  // add 2/3 patterns to screen
  // ---------------------------------------------------------
  vdp_setWriteAddress(_vdpPatternNameTableAddr);

  for (i = 0; i < 256 * 2; i++) {

    _vdp_textBuffer[i] = i % 256;

    IO_VDPDATA = i % 256;
  }
  
  vdp_enableVDPReadyInt();
}

void toupper(uint8_t *str) {

  while (*str) {

    if (*str >= 97)
      *str -= 32;

    str++;
  }
}

uint16_t strToInt(char *str) {

  uint16_t result = 0;

  while ((*str >= '0') && (*str <= '9')) {

      result = (result * 10) + ((*str) - '0');

      str++;
  }

  return result;
}

// for (int x = 0; x < 360; x++)
//   Console.Write(Math.Round(Math.Sin(x * Math.PI / 180) * 1, 2) * 100 + ", ");
// Console.WriteLine();
// for (int x = 0; x < 360; x++)
//   Console.Write(Math.Round(Math.Cos(x * Math.PI / 180) * -1, 2) * 100 + ", ");

const int8_t sinArray[360] = { 0, 2, 3, 5, 7, 9, 10, 12, 14, 16, 17, 19, 21, 22, 24, 26, 28, 29, 31, 33, 34, 36, 37, 39, 41, 42, 44, 45, 47, 48, 50, 52, 53, 54, 56, 57, 59, 60, 62, 63, 64, 66, 67, 68, 69, 71, 72, 73, 74, 75, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 87, 88, 89, 90, 91, 91, 92, 93, 93, 94, 95, 95, 96, 96, 97, 97, 97, 98, 98, 98, 99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 99, 99, 99, 99, 98, 98, 98, 97, 97, 97, 96, 96, 95, 95, 94, 93, 93, 92, 91, 91, 90, 89, 88, 87, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 75, 74, 73, 72, 71, 69, 68, 67, 66, 64, 63, 62, 60, 59, 57, 56, 54, 53, 52, 50, 48, 47, 45, 44, 42, 41, 39, 37, 36, 34, 33, 31, 29, 28, 26, 24, 22, 21, 19, 17, 16, 14, 12, 10, 9, 7, 5, 3, 2, 0, -2, -3, -5, -7, -9, -10, -12, -14, -16, -17, -19, -21, -22, -24, -26, -28, -29, -31, -33, -34, -36, -37, -39, -41, -42, -44, -45, -47, -48, -50, -52, -53, -54, -56, -57, -59, -60, -62, -63, -64, -66, -67, -68, -69, -71, -72, -73, -74, -75, -77, -78, -79, -80, -81, -82, -83, -84, -85, -86, -87, -87, -88, -89, -90, -91, -91, -92, -93, -93, -94, -95, -95, -96, -96, -97, -97, -97, -98, -98, -98, -99, -99, -99, -99, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -99, -99, -99, -99, -98, -98, -98, -97, -97, -97, -96, -96, -95, -95, -94, -93, -93, -92, -91, -91, -90, -89, -88, -87, -87, -86, -85, -84, -83, -82, -81, -80, -79, -78, -77, -75, -74, -73, -72, -71, -69, -68, -67, -66, -64, -63, -62, -60, -59, -57, -56, -54, -53, -52, -50, -48, -47, -45, -44, -42, -41, -39, -37, -36, -34, -33, -31, -29, -28, -26, -24, -22, -21, -19, -17, -16, -14, -12, -10, -9, -7, -5, -3, -2, };
const int8_t cosArray[360] = { -100, -100, -100, -100, -100, -100, -99, -99, -99, -99, -98, -98, -98, -97, -97, -97, -96, -96, -95, -95, -94, -93, -93, -92, -91, -91, -90, -89, -88, -87, -87, -86, -85, -84, -83, -82, -81, -80, -79, -78, -77, -75, -74, -73, -72, -71, -69, -68, -67, -66, -64, -63, -62, -60, -59, -57, -56, -54, -53, -52, -50, -48, -47, -45, -44, -42, -41, -39, -37, -36, -34, -33, -31, -29, -28, -26, -24, -22, -21, -19, -17, -16, -14, -12, -10, -9, -7, -5, -3, -2, 0, 2, 3, 5, 7, 9, 10, 12, 14, 16, 17, 19, 21, 22, 24, 26, 28, 29, 31, 33, 34, 36, 37, 39, 41, 42, 44, 45, 47, 48, 50, 52, 53, 54, 56, 57, 59, 60, 62, 63, 64, 66, 67, 68, 69, 71, 72, 73, 74, 75, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 87, 88, 89, 90, 91, 91, 92, 93, 93, 94, 95, 95, 96, 96, 97, 97, 97, 98, 98, 98, 99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 99, 99, 99, 99, 98, 98, 98, 97, 97, 97, 96, 96, 95, 95, 94, 93, 93, 92, 91, 91, 90, 89, 88, 87, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 75, 74, 73, 72, 71, 69, 68, 67, 66, 64, 63, 62, 60, 59, 57, 56, 54, 53, 52, 50, 48, 47, 45, 44, 42, 41, 39, 37, 36, 34, 33, 31, 29, 28, 26, 24, 22, 21, 19, 17, 16, 14, 12, 10, 9, 7, 5, 3, 2, 0, -2, -3, -5, -7, -9, -10, -12, -14, -16, -17, -19, -21, -22, -24, -26, -28, -29, -31, -33, -34, -36, -37, -39, -41, -42, -44, -45, -47, -48, -50, -52, -53, -54, -56, -57, -59, -60, -62, -63, -64, -66, -67, -68, -69, -71, -72, -73, -74, -75, -77, -78, -79, -80, -81, -82, -83, -84, -85, -86, -87, -87, -88, -89, -90, -91, -91, -92, -93, -93, -94, -95, -95, -96, -96, -97, -97, -97, -98, -98, -98, -99, -99, -99, -99, -100, -100, -100, -100, -100, };

void updateStatus() {

  sprintf(_outBuf, "X:%u  Y:%u  Heading: %d'      ", _x / 100, _y / 100, _heading);
  uint8_t cx = vdp_cursor.x;
  uint8_t cy = vdp_cursor.y;
  vdp_setCursor2(0, 16);
  vdp_print(_outBuf);
  vdp_setCursor2(cx, cy);
}

void move(uint8_t seconds) {

  for (uint s = 0; s < seconds * 10; s++) {

    if (s % 10 == 0) {

      _x += sinArray[_heading];
      _y += cosArray[_heading];
    }

    putPixel(true, _x / 100, _y / 100);
    invalidateScreen();    
    updateStatus();
  }

  sprintf(_outBuf, "stop()\r\n");
  rn_TCPHandleWrite(_tcp, 0, strlen(_outBuf), _outBuf);
}

void doLoop() {

  vdp_print("NABU TurtleBot       (c)DJ Sures"); vdp_newLine();
  vdp_print("Type HELP for help"); vdp_newLine();

  while (true) {

    vdp_waitVDPReadyInt();
    putPixel(true, _x / 100, _y / 100);
    invalidateScreen();
    updateStatus();

    memset(_inBuf, 0x00, IN_BUF_LEN);
    memset(_outBuf, 0x00, OUT_BUF_LEN);
      
    vdp_print(">");
    uint8_t read = readLine(_inBuf, IN_BUF_LEN);    
    vdp_newLine();

    if (read == 0)
      continue;

    toupper(_inBuf);

    uint8_t *cmd    = strtok(_inBuf, " ");
    uint8_t *param  = strtok(NULL, "\0");
    
    if (strstr(cmd, "FORWARD") && param) {

      sprintf(_outBuf, "forward()\r\n");
      rn_TCPHandleWrite(_tcp, 0, strlen(_outBuf), _outBuf);

      uint8_t p = strToInt(param);      
      move(p);

    } else if (strstr(cmd, "RIGHT") && param) {

      sprintf(_outBuf, "right()\r\n");
      rn_TCPHandleWrite(_tcp, 0, strlen(_outBuf), _outBuf);

      uint8_t p = strToInt(param);      
      
      uint16_t oldHeading = _heading;
      
      _heading += p;
      
      uint16_t diff = _heading - oldHeading;
      
      _heading = _heading % 360;

      for (uint16_t i = 0; i < diff * 3; i++)
        vdp_waitVDPReadyInt();

      sprintf(_outBuf, "stop()\r\n");
      rn_TCPHandleWrite(_tcp, 0, strlen(_outBuf), _outBuf);
    } else if (strstr(cmd, "LEFT") && param) {

      sprintf(_outBuf, "left()\r\n");
      rn_TCPHandleWrite(_tcp, 0, strlen(_outBuf), _outBuf);

      uint8_t p = strToInt(param);      
      
      int16_t oldHeading = _heading;
      
      _heading -= p;
      
      uint16_t diff = abs(_heading - oldHeading);
      
      if (_heading < 0)
        _heading = 360 - abs(_heading);

      _heading = _heading % 360;

      for (uint16_t i = 0; i < diff * 3; i++)
        vdp_waitVDPReadyInt();

      sprintf(_outBuf, "stop()\r\n");
      rn_TCPHandleWrite(_tcp, 0, strlen(_outBuf), _outBuf);
    } else if (strstr(cmd, "STOP")) {

      sprintf(_outBuf, "stop()\r\n");
      rn_TCPHandleWrite(_tcp, 0, strlen(_outBuf), _outBuf);
    } else if (strstr(cmd, "DANCE")) {

      sprintf(_outBuf, "ControlCommand(\"Soundboard v4\", \"Track_1\")\r\n");
      rn_TCPHandleWrite(_tcp, 0, strlen(_outBuf), _outBuf);
    } else if (strstr(cmd, "WAVE")) {

      sprintf(_outBuf, "ControlCommand(\"Auto Position\", \"AutoPositionAction\", \"Wave\")\r\n");
      rn_TCPHandleWrite(_tcp, 0, strlen(_outBuf), _outBuf);

    } else if (strstr(cmd, "SAY") && param) {

      sprintf(_outBuf, "say(%s)\r\n", param);
      rn_TCPHandleWrite(_tcp, 0, strlen(_outBuf), _outBuf);
    } else if (strstr(cmd, "RESET")) {

      for (uint16_t i = 0; i < 2048 * 2; i++)
        _dBuffer[i] = 0x00;
      
      _heading = 0;
      _x        = 25600 / 2;
      _y        = 6000;
    } else if (strstr(cmd, "QUIT")) {

      return;
    } else if (strstr(cmd, "HELP")) {

      vdp_print("Forward x                  Stop"); vdp_newLine();
      vdp_print("Left x                  Right x"); vdp_newLine();
      vdp_print("Dance                      Wave"); vdp_newLine();
      vdp_print("Say \"text\" "); vdp_newLine();
      vdp_print("Reset                      Quit"); vdp_newLine();

    } else {

      vdp_print("Type HELP for help");
      vdp_newLine();
    }
  }
}

void main() {

  initNABULib();

  puts("\nNABU TurtleBot           2023(c)DJ Sures");
  puts(" - Connecting to ARC...");
  _tcp = rn_TCPOpen(9, "127.0.0.1", 6666, 0xff);

  if (_tcp == 0xff) {

    puts(" - Unable to connect to ARC!");

    return;
  }

  puts(" - Connected");

  vdp_initG2Mode(0, false, false, true, true);

  initDisplay();

  _autoScrollTopRow = 18;
  _autoScrollBottomRow = 23;
  vdp_setCursor2(0, _autoScrollTopRow);

  doLoop();

  puts(" - Disconnecting from ARC");

  rn_TCPHandleClose(_tcp);    

  puts(" - Exiting to CPM...");
}
