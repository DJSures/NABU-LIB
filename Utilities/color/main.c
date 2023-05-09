// ************************************************************************
// Color
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
#pragma output nofileio
#pragma output noprotectmsdos
#pragma output nogfxglobals
#define DISABLE_CURSOR
#define DISABLE_KEYBOARD_INT
//#define DISABLE_HCCA_RX_INT
#define DISABLE_VDP

#define BIN_TYPE BIN_CPM

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <conio.h>
#include "../../NABULIB/NABU-LIB.h"
#include "../../NABULIB/RetroNET-FileStore.h"

#define _FILENAME_SETTINGS_LEN 4
uint8_t * _FILENAME_SETTINGS = "CCPM";

__sfr __at 0xA1 IO_VDPLATCH;

__at (0xff25) uint8_t  _SCREEN_COLOR;

uint16_t strToInt(char *str) {

  uint16_t result = 0;

  while ((*str >= '0') && (*str <= '9')) {

      result = (result * 10) + ((*str) - '0');

      str++;
  }

  return result;
}

void displayHelp() {

  puts("");
  puts("Set screen color for Cloud CPM");
  puts("");
  puts("1  black             9  light red");
  puts("2  medium green     10  dark yellow");
  puts("3  light green      11  light yellow");
  puts("4  dark blue        12  dark green");
  puts("5  light blue       13  magenta");
  puts("6  dark red         14  gray");
  puts("7  cyan             15  white");  
  puts("8  medium red");
  puts("");
  puts("Usage:");
  puts(" interactive:         color i");
  puts(" argument:            color fg bg");
  puts(" example white/black: color 15 1");
  puts("");
}

#define SCRATCH_SIZE 128
uint8_t _scratch[SCRATCH_SIZE] = { 0 };

void saveSetting(uint8_t fg, uint8_t bg) {

  _SCREEN_COLOR = (fg << 4) | (bg & 0x0f);    

  uint8_t fileHandle = rn_fileOpen(_FILENAME_SETTINGS_LEN, _FILENAME_SETTINGS, OPEN_FILE_FLAG_READWRITE, 0xff);

  uint8_t r = rn_fileHandleRead(fileHandle, _scratch, 0, 0, SCRATCH_SIZE);

  rn_fileHandleEmptyFile(fileHandle);

  _scratch[0] = _SCREEN_COLOR;

  rn_fileHandleAppend(fileHandle, 0, SCRATCH_SIZE, _scratch);

  rn_fileHandleClose(fileHandle);
}

void vdp_setRegister(uint8_t registerIndex, uint8_t value) {

  IO_VDPLATCH = value;

  IO_VDPLATCH = 0x80 | registerIndex;
}

void setColor(uint8_t fgColor, uint8_t bgColor) {

  vdp_setRegister(7, (fgColor << 4) | (bgColor & 0x0f));
}

void interactive() {

  uint8_t origColor = _SCREEN_COLOR;

  puts("");
  puts("f - cycles foreground");
  puts("b - cycles background");
  puts("s - saves selection");
  puts("c - cancels");

  uint8_t fg = _SCREEN_COLOR >> 4;
  uint8_t bg = _SCREEN_COLOR & 0x0f;

  while (true) {

    uint8_t k = getch();

    switch (k) {
      case 'f':

        fg++;

        if (bg == fg)
          fg++;

        if (fg > 15)
          fg = 1;
        
        setColor(fg, bg);
        break;
      case 'b':

        bg++;

        if (bg == fg)
          bg++;

        if (bg > 15)
          bg = 1;
        
        setColor(fg, bg);
        break;
      case 's':

        saveSetting(fg, bg);

        puts("");
        puts("Duplicate these colors by typing:");
        printf("color %u %u\n\n", fg, bg);
        return;
        break;
      case 'c':
        return;
        break;
    }
  }
}

void main(int argc, char *argv[]) {

  if (argc <= 1) {

    displayHelp();

    return;
  }

  if (argc == 2) {

    interactive();

    return;
  }

  if (argc == 3) {

    uint8_t fg = strToInt(argv[1]);

    uint8_t bg = strToInt(argv[2]);

    saveSetting(fg, bg);
  }
}
