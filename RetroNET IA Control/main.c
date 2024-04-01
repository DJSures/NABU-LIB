//#define BIN_TYPE BIN_HOMEBREW
#define BIN_TYPE BIN_CPM

#define RX_BUFFER_SIZE 1024

#define DISABLE_CURSOR

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../NABULIB/NABU-LIB.h"
#include "../../NABULIB/RetroNET-IAControl.h"
#include "../../NABULIB/RetroNET-FileStore.h"
#define byte uint8_t
#include "starpattern.c"

#define MENU_TOP_X 0
#define MENU_TOP_Y 2

uint8_t _tmpBuf[512] = { 0x00 };

void toupper(uint8_t *text) {

  while (*text != 0x00) {

    if (*text >= 'a' && *text <= 'z') 
      *text = *text - 32;
    
    text++;
  }
}

void tolower(uint8_t *text) {

  while (*text != 0x00) {

    if (*text >= 'A' && *text <= 'Z') 
      *text = *text + 32;
    
    text++;
  }
}

uint8_t waitForJoyStick() {

  while (true) {

    if (isKeyPressed()) {

      switch(getChar()) {
        case 'q':
          __asm
            rst 0x00;
          __endasm;
          break;
        case 0xe2:
          playNoteDelay(0, 12, 20);
          return Joy_Up;
          break;
        case 0xe3:
          playNoteDelay(0, 5, 20);
          return Joy_Down;
          break;
        case 13:        
          playNoteDelay(0, 0, 20);
          return Joy_Button;
          break;
      }
    }

    for (uint8_t p = 0; p < 4; p++) {

      // only allow down up and button on joystick
      switch (_joyStatus[p] & (Joy_Down | Joy_Up | Joy_Button) ) {

        case Joy_Up:
            playNoteDelay(0, 12, 20);
            while (_joyStatus[p] & Joy_Up);
            return Joy_Up;
          break;

        case Joy_Down:
            playNoteDelay(0, 5, 20);
            while (_joyStatus[p] & Joy_Down);
            return Joy_Down;
          break;

        case Joy_Button:
            playNoteDelay(0, 0, 20);
            while (_joyStatus[p] & Joy_Button);
            return Joy_Button;
          break;
      }
    }
  }
}

void putTitle() {

  vdp_clearScreen();

  vdp_colorizePattern(0x01, VDP_WHITE, VDP_LIGHT_BLUE);

  for (uint8_t i = 0; i < 32; i++) 
    vdp_putPattern(i, 0, 0x01);
  
  vdp_setCursor2(12, 0);
  vdp_printColorized("retronet", VDP_WHITE, VDP_LIGHT_BLUE);

  vdp_setCursor2(31 - 4, 23);
  vdp_print("v0.1");
}

void screenLog() {

  vdp_clearRows(MENU_TOP_Y, 21);

  ia_getLog(_tmpBuf);
  
  toupper(_tmpBuf);

  vdp_print(_tmpBuf);

  waitForJoyStick();
}

uint8_t screen1() {

  vdp_clearRows(MENU_TOP_Y, 24);

  // Display categories
  // ---------------------------------------------------
  uint8_t parentCnt = ia_getParentCount();

  for (uint8_t i = 0; i < parentCnt; i++) {

    vdp_setCursor2(MENU_TOP_X + 1, MENU_TOP_Y + i);

    ia_getParentName(i, _tmpBuf);

    toupper(_tmpBuf);

    vdp_print(_tmpBuf);
  }

  // Display News
  // ---------------------------------------------------
  ia_getNewsDate(_tmpBuf);
  toupper(_tmpBuf);
  vdp_setCursor2(0, 10);
  vdp_print(_tmpBuf); 

  ia_getNewsTitle(_tmpBuf);
  toupper(_tmpBuf);
  vdp_setCursor2(0, 11);
  vdp_print(_tmpBuf); 

  ia_getNewsContent(_tmpBuf);
  toupper(_tmpBuf);
  vdp_setCursor2(0, 12);
  vdp_print(_tmpBuf);

  // Allow selecting a category
  // ---------------------------------------------------
  uint8_t selectedId = 0;
  vdp_putPattern(MENU_TOP_X, MENU_TOP_Y + selectedId, '>'); 
  while (true) {

    uint8_t joy = waitForJoyStick();

    vdp_putPattern(MENU_TOP_X, MENU_TOP_Y + selectedId, ' ');

    if (joy == Joy_Up && selectedId > 0)
      selectedId--;
    else if (joy == Joy_Down && selectedId < parentCnt - 1)
      selectedId++;
    else if (joy == Joy_Button)
      return selectedId;

    vdp_putPattern(MENU_TOP_X, MENU_TOP_Y + selectedId, '>'); 
  }
}

int8_t screen2(uint8_t parentId) {

  uint8_t childCnt = ia_getChildCount(parentId);
  
  vdp_clearRows(MENU_TOP_Y, 24);

  vdp_setCursor2(MENU_TOP_X + 1, MENU_TOP_Y);
  vdp_printColorized("back", VDP_GRAY, 0); 

  for (uint8_t i = 0; i < childCnt; i++) {

    vdp_setCursor2(MENU_TOP_X + 1, MENU_TOP_Y + i + 1);

    ia_getChildName(parentId, i, _tmpBuf);
    toupper(_tmpBuf);
    vdp_print(_tmpBuf); 
  }

  int8_t selectedId = -1;
  vdp_putPattern(MENU_TOP_X, MENU_TOP_Y + selectedId + 1, '>'); 

  while (true) {

    uint8_t joy = waitForJoyStick();

    vdp_putPattern(MENU_TOP_X, MENU_TOP_Y + selectedId + 1, ' ');

    if (joy == Joy_Up && selectedId > -1)
      selectedId--;
    else if (joy == Joy_Down && selectedId < childCnt - 1)
      selectedId++;
    else if (joy == Joy_Button)
      return selectedId;

    vdp_putPattern(MENU_TOP_X, MENU_TOP_Y + selectedId + 1, '>'); 

    vdp_clearRows(15, 24);

    if (selectedId >= 0) {

      // icon box
      // -----------------------------------------------------------
      // left
      vdp_putPattern(0, 16, 0x80); // top
      vdp_putPattern(0, 17, 0x81); // bottom

      // top
      vdp_putPattern(1, 15, 0x82); // left
      vdp_putPattern(2, 15, 0x83); // right

      // right
      vdp_putPattern(3, 16, 0x84); // top
      vdp_putPattern(3, 17, 0x85); // bottom

      // bottom
      vdp_putPattern(1, 18, 0x86); // left
      vdp_putPattern(2, 18, 0x87); // right

      // Icon
      // -----------------------------------------------------------
      ia_getChildIconTilePattern(parentId, selectedId, _tmpBuf);
      vdp_loadPatternToId(0xf0, _tmpBuf);
      vdp_loadPatternToId(0xf1, _tmpBuf + 8);
      vdp_loadPatternToId(0xf2, _tmpBuf + 16);
      vdp_loadPatternToId(0xf3, _tmpBuf + 24);

      ia_getChildIconTileColor(parentId, selectedId, _tmpBuf);
      vdp_loadColorToId(0xf0, _tmpBuf);
      vdp_loadColorToId(0xf1, _tmpBuf + 8);
      vdp_loadColorToId(0xf2, _tmpBuf + 16);
      vdp_loadColorToId(0xf3, _tmpBuf + 24);

      vdp_putPattern(1, 16, 0xf0);    
      vdp_putPattern(1, 17, 0xf1);    
      vdp_putPattern(2, 16, 0xf2);
      vdp_putPattern(2, 17, 0xf3);    

      // Author
      // -----------------------------------------------------------
      vdp_setCursor2(5, 17);
      vdp_print("AUTHOR: "); 
      ia_getChildAuthor(parentId, selectedId, _tmpBuf);
      tolower(_tmpBuf);
      vdp_printColorized(_tmpBuf, VDP_GRAY, 0); 

      // Description
      // -----------------------------------------------------------
      vdp_setCursor2(0, 19);
      ia_getChildDescription(parentId, selectedId, _tmpBuf);
      tolower(_tmpBuf);
      vdp_printColorized(_tmpBuf, VDP_GRAY, 0); 
    }
  }
}

void restartWithNABUROM() {

    __asm
      // Disable the interrupts
      // --------------------------
      // di
      ld a, 0xf3;
      ld (0x8880), a;

      // Enable the ROM shadow
      // --------------------------
      // ld a, 0x02
      ld a, 0x3e;
      ld (0x8881), a;
      ld a, 0x02;
      ld (0x8882), a;

      // out (0x00), a
      ld a, 0xd3;
      ld (0x8883), a;
      ld a, 0x00;
      ld (0x8884), a;

      // Restart at 0x00
      // --------------------------
      // rst 0x00
      ld a, 0xc7;
      ld (0x8885), a;
     
      jp 0x8880;
    __endasm;
}

void main() {

  initNABULib();
    
  vdp_initG2Mode(0, false, false, false, false);
  vdp_loadPatternTable(PATTERN, sizeof(PATTERN));
  vdp_loadColorTable(COLOR, sizeof(COLOR));

  putTitle();

  while (true) {

    uint8_t parentId = screen1();    

    int8_t childId = screen2(parentId);

    if (childId >= 0) {

      ia_setSelection(parentId, childId);
      
      restartWithNABUROM();      
    }
  }
}
