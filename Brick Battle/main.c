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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../NABULIB/NABU-LIB.h"
#include "../NABULIB/RetroNET-FileStore.h"
#include "patterns.c"
#include "../NABULIB/NabuTracker.h"
#include "./midis/baseattack-nt.h"

unsigned char SPRITE_DATA[]={
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x7F,0xFF,0xFF,0x7F,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xFE,0xFF,0xFF,0xFE,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x38,0x7C,0xFE,0xFE,0xFE,0x7C,0x38,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

#define SPRITE_NAME_BLANK  0
#define SPRITE_NAME_PADDLE 1
#define SPRITE_NAME_BALL   2

#define SCREEN_MAX_X 255
#define SCREEN_MIN_X 1

#define SCREEN_MAX_Y 190
#define SCREEN_MIN_Y 8

uint8_t tb[64] = { 0x00 };

uint8_t PLAYER_CNT = 1;
uint8_t ROCK_CNT   = 1;
uint8_t TIME       = 60;

// Main game loop counter used for player & rock movement inertia
uint16_t _loopCntr = 0;

uint8_t _timeCntr = 0;

bool _isGamePlaying = false;

#define PLAYER_MAX 2

uint8_t _playerColors[4] = { 2, 4, 6, 10 };

struct {
           uint8_t  x;
           uint8_t  y;
           uint8_t  score;
           uint8_t  spriteId;
           bool     vdpReady;
} _player[4] = 
  {
    { 0 },
    { 0 },
    { 0 },
    { 0 },
  };

#define ROCK_MAX 6

struct {
           uint8_t  x;
           uint8_t  y;
           bool     xDir;
           bool     yDir;    
           uint8_t  xSpeed;
           uint8_t  ySpeed;
           uint8_t  spriteId;
           uint8_t  lastTouchPlayerId;
           bool     vdpReady;
} _rock[ROCK_MAX] = 
  {
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
  };

void updateScore(uint8_t p) {
  
  sprintf(tb, "%03u", _player[p].score);
  
  vdp_setCursor2(p * 7, 0);
  vdp_print(tb);
}

void initBricks() {

  for (uint8_t y = 2; y < 6; y++) {

    uint8_t p = 0;

    for (uint8_t x = 1; x < 31; x++) {

      vdp_putPattern(x, y, 10 + (p % 15));

      p++;
    }
  }
}

// ********************************************************************************************
// INITIALIZE PLAYER
// Before the game starts, we need to initialize the player
// ********************************************************************************************
void initPlayers() {

  for (uint8_t p = 0; p < PLAYER_MAX;)
    if (p < PLAYER_CNT) {

      uint8_t x = (rand() % (SCREEN_MAX_X - 22)) + 2;

      bool ok = true;
  
      for (uint8_t pp = 0; pp < p; pp++)
        if (x > _player[pp].x - 16 && x < _player[pp].x + 16)
          ok = false;
      
      if (!ok)
        continue;
      
      _player[p].x        = x;
      _player[p].y        = SCREEN_MAX_Y - 14;
      _player[p].score    = 0;
      _player[p].vdpReady = true;
      _player[p].spriteId = vdp_spriteInit(p, SPRITE_NAME_PADDLE, _player[p].x, _player[p].y, _playerColors[p]);

      updateScore(p);

      p++;
    } else {

      _player[p].vdpReady = false;

      vdp_setCursor2(p * 7, 0);
      vdp_print("       ");

      vdp_disableSprite(p);

      p++;
    }
}

// ********************************************************************************************
// INITIALIZE PLAYER
// Before the game starts, we need to initialize the player
// ********************************************************************************************
void initRocks() {

  for (uint8_t p = 0; p < ROCK_MAX; p++)
    if (p < ROCK_CNT) {

      _rock[p].x = SCREEN_MAX_X / 2;
      _rock[p].y = SCREEN_MAX_Y / 2;
      _rock[p].xSpeed = (rand() % (5)) + 1;
      _rock[p].ySpeed = (rand() % (5)) + 1;    
      _rock[p].xDir = false;
      _rock[p].yDir = false;
      _rock[p].vdpReady = true;
      _rock[p].spriteId = vdp_spriteInit(4 + p, SPRITE_NAME_BALL, _rock[p].x, _rock[p].y, VDP_GRAY);
      _rock[p].lastTouchPlayerId = 0xff;
    } else {

      _rock[p].vdpReady = true;

      vdp_disableSprite(4 + p);
    }
}

// ********************************************************************************************
// PLAYER
// Handle the player's movement
// ********************************************************************************************
void doPlayer(uint8_t p) {

  // Joystick up/down
  // -------------------------------------------------------
  switch (_joyStatus[p] & 0b00001010) {

    case Joy_Up:

      if (_player[p].y > SCREEN_MIN_Y) {

        _player[p].vdpReady = true;      

        _player[p].y--;
      }

      break;
    case Joy_Down:

      if (_player[p].y < SCREEN_MAX_Y - 5) {

        _player[p].vdpReady = true;

        _player[p].y++;
      }

      break;
  }

  // Joystick left/right
  // -------------------------------------------------------
  switch (_joyStatus[p] & 0b00000101) {

    case Joy_Right:

      if (_player[p].x < SCREEN_MAX_X - 16) {

        _player[p].vdpReady = true;      

        _player[p].x++;
      }

      break;
    case Joy_Left:

      if (_player[p].x > SCREEN_MIN_X) {

        _player[p].vdpReady = true;      

        _player[p].x--;
      }

      break;
  }  
}


// ********************************************************************************************
// ROCK'n
// Handle the rock movement
// ********************************************************************************************
void doRock(uint8_t r) {

  bool needsRedraw = false;

  if (!(_loopCntr % _rock[r].xSpeed)) {

    if (_rock[r].xDir) {

      _rock[r].x++;

      if (_rock[r].x >= SCREEN_MAX_X - 8) {

        if (_isGamePlaying)
          playNoteDelay(2, 0, 10);

        _rock[r].xDir = false;
      }
    } else {

      _rock[r].x--;

      if (_rock[r].x <= SCREEN_MIN_X) {

        if (_isGamePlaying)
          playNoteDelay(2, 0, 10);
  
        _rock[r].xDir = true;
      }
    }

    needsRedraw = true;
  }

  if (!(_loopCntr % _rock[r].ySpeed)) {

    if (_rock[r].yDir) {

      _rock[r].y++;

      if (_rock[r].y >= SCREEN_MAX_Y - 6) {

        if (_isGamePlaying)
          playNoteDelay(2, 0, 10);

        _rock[r].yDir = false;

        // _rock[r].lastTouchPlayerId == 0xff;

        // vdp_setSpriteColor(_rock[r].spriteId, VDP_GRAY);
      }
    } else {

      _rock[r].y--;

      if (_rock[r].y <= SCREEN_MIN_Y) {

        if (_isGamePlaying)
          playNoteDelay(2, 0, 10);
  
        _rock[r].yDir = true;
      }

      if (_isGamePlaying) {

        uint8_t xCol = (_rock[r].x + 4) / 8;
        uint8_t yCol = (_rock[r].y + 4) / 8;

        if (vdp_getCharAtLocationBuf(xCol, yCol) >= 10 && _rock[r].lastTouchPlayerId != 0xff) {

          vdp_putPattern(xCol, yCol, 0x00);

          _rock[r].yDir = true;

          _player[_rock[r].lastTouchPlayerId].score += 2;

          updateScore(_rock[r].lastTouchPlayerId);

          playNoteDelay(0, 50, 10);
        }
      }
    }

    needsRedraw = true;
  }

  if (needsRedraw) {

    _rock[r].vdpReady = true;  
  }
}


// ********************************************************************************************
// COLLISION
// This is called only when the VDP status has collision detected
// ********************************************************************************************
void doCollision() {

  // Check player collisions
  for (uint8_t p = 0; p < PLAYER_CNT; p++) {

    // width and height of sprite
    uint8_t xd = _player[p].x + 16;
    uint8_t yd = _player[p].y + 4;

    // center of sprite
    uint8_t xc = _player[p].x + 8;
    uint8_t yc = _player[p].y + 2;

    // player against player
    for (uint8_t i = 0; i < PLAYER_CNT; i++) {

      if (i == p)
        continue;

      if ( (_player[p].x < _player[i].x + 16) && (xd > _player[i].x) && 
           (_player[p].y < _player[i].y + 4) && (yd > _player[i].y) ) {

        if (_isGamePlaying)
          playNoteDelay(2, 38, 5);
  
        if (xc < _player[i].x + 8) {

          _player[p].x -= 4;
          _player[i].x += 4;
        } else {

          _player[p].x += 4;
          _player[i].x -= 4;
        }

        if (yc < _player[i].y + 2) {

          _player[p].y -= 2;
          _player[i].y += 2;
        } else {

          _player[p].y += 2;
          _player[i].y -= 2;
        }

        _player[p].vdpReady = true;
        _player[i].vdpReady = true;

        break;
      }
    }

    // player against rock
    for (uint8_t i = 0; i < ROCK_CNT; i++) {

      if ( (_player[p].x < _rock[i].x + 16) && (xd > _rock[i].x) && 
           (_player[p].y < _rock[i].y + 8) && (yd > _rock[i].y) ) {

        if (_isGamePlaying)
          playNoteDelay(1, 0, 10);
  
        if (xc < _rock[i].x + 4) 
          _rock[i].xDir = true;
        else 
          _rock[i].xDir = false;  
        
        // if (yc < _rock[i].y + 4) 
        //   _rock[i].yDir = true;
        // else 
          _rock[i].yDir = false;
      
        _rock[i].xSpeed = (rand() % (3)) + 1;
        _rock[i].ySpeed = (rand() % (3)) + 1;    

        _rock[i].lastTouchPlayerId = p;

        vdp_setSpriteColor(_rock[i].spriteId, _playerColors[p]);

        break;
      }
    }
  }

  // check rock collisions
  for (uint8_t r = 0; r < ROCK_CNT; r++) {
    
    uint8_t xd = _rock[r].x + 8;
    uint8_t yd = _rock[r].y + 8;

    uint8_t xc = _rock[r].x + 4;
    uint8_t yc = _rock[r].y + 4;

    for (uint8_t i = 0; i < ROCK_CNT; i++) {

      if (i == r)
        continue;

      if ( (_rock[r].x < _rock[i].x + 8) && (xd > _rock[i].x) && 
           (_rock[r].y < _rock[i].y + 8) && (yd > _rock[i].y) ) {

        if (_isGamePlaying)
          playNoteDelay(2, 12, 10);
  
        if (xc < _rock[i].x + 4) {

          _rock[r].xDir = false;
          _rock[i].xDir = true;
        } else {

          _rock[r].xDir = true;
          _rock[i].xDir = false;
        }

        if (yc < _rock[i].y + 4) {

          _rock[r].yDir = false;
          _rock[i].yDir = true;
        } else {

          _rock[r].yDir = true;
          _rock[i].yDir = false;
        }

        _rock[r].xSpeed = (rand() % (20)) + 1;
        _rock[r].ySpeed = (rand() % (20)) + 1;    

        break;
      }
    }
  }
}

void renderRocks() {

    // ROCKS
    // ------------------------------------------------------------------
    if (_rock[0].vdpReady) {

      vdp_setSpritePosition(_rock[0].spriteId, _rock[0].x, _rock[0].y);  

      _rock[0].vdpReady = false;
    }

    if (_rock[1].vdpReady) {

      vdp_setSpritePosition(_rock[1].spriteId, _rock[1].x, _rock[1].y);  

      _rock[1].vdpReady = false;
    }

    if (_rock[2].vdpReady) {

      vdp_setSpritePosition(_rock[2].spriteId, _rock[2].x, _rock[2].y);  

      _rock[2].vdpReady = false;
    }

    if (_rock[3].vdpReady) {

      vdp_setSpritePosition(_rock[3].spriteId, _rock[3].x, _rock[3].y);  

      _rock[3].vdpReady = false;
    }

    if (_rock[4].vdpReady) {

      vdp_setSpritePosition(_rock[4].spriteId, _rock[4].x, _rock[4].y);  

      _rock[4].vdpReady = false;
    }

    if (_rock[5].vdpReady) {

      vdp_setSpritePosition(_rock[5].spriteId, _rock[5].x, _rock[5].y);  

      _rock[5].vdpReady = false;
    }
}

void clearScreen() {

  vdp_fillScreen(0x00);

  for (uint8_t x = 0; x < 32; x++) {

    vdp_putPattern(x, 1, 0x03);
    vdp_putPattern(x, 23, 0x04);
  }

  for (uint8_t y = 1; y < 23; y++) {

    vdp_putPattern(0, y, 0x02);
    vdp_putPattern(31, y, 0x08);
  }

  vdp_putPattern(0, 1, 0x01);
  vdp_putPattern(31, 1, 0x05);
  vdp_putPattern(31, 23, 0x07);
  vdp_putPattern(0, 23, 0x06);
}

// ********************************************************************************************
// PLAYING THE GAME YO!
// Sets up the graphic mode, vdp interrupt, initializes the game, plays it in a loop
// When q is pressed, we exit the loop and that returns to the calling function (ie main menu)
// ********************************************************************************************
void doGameLoop() {

  _isGamePlaying = true;

  _timeCntr = TIME;

  initNABULIBAudio();

  clearScreen();

  initPlayers();

  initRocks();

  initBricks();

  if (PLAYER_CNT == 0)
    for (uint8_t i = 0; i < ROCK_CNT; i++)
      _rock[i].lastTouchPlayerId = 0;

  while (true) {

    if (isKeyPressed() && getChar() == 'q')
      break;

    if (vdpStatusRegVal & VDP_FLAG_COLLISION)
      doCollision();

    for (uint8_t p = 0; p < PLAYER_CNT; p++)
      doPlayer(p);

    for (uint8_t r = 0; r < ROCK_CNT; r++) 
      doRock(r);

    // Wait for VSYNC
    // ------------------------------------------------------------------
    vdp_waitVDPReadyInt();

    // UFOs
    // ------------------------------------------------------------------
    if (_player[0].vdpReady) {

      vdp_setSpritePosition(_player[0].spriteId, _player[0].x, _player[0].y);

      _player[0].vdpReady = false;
    }

    if (_player[1].vdpReady) {

      vdp_setSpritePosition(_player[1].spriteId, _player[1].x, _player[1].y);

      _player[1].vdpReady = false;
    }

    if (_player[2].vdpReady) {

      vdp_setSpritePosition(_player[2].spriteId, _player[2].x, _player[2].y);

      _player[2].vdpReady = false;
    }

    if (_player[3].vdpReady) {

      vdp_setSpritePosition(_player[3].spriteId, _player[3].x, _player[3].y);

      _player[3].vdpReady = false;
    }

    renderRocks();

    if (PLAYER_CNT != 0 && _loopCntr % 60 == 0) {

      _timeCntr--;

      sprintf(tb, "% 3u", _timeCntr);
      vdp_setCursor2(28, 0);
      vdp_print(tb);

      if (_timeCntr < 5)
        playNoteDelay(0, 60, 10);

      if (_timeCntr == 0)
        break;
    }

    _loopCntr++;
  }

  for (uint p = 0; p < PLAYER_MAX; p++)
    vdp_disableSprite(p);

  _isGamePlaying = false;
}


// ********************************************************************************************
// LOAD IMAGE
// Loads a SC2 Image to Graphics Mode 2
// ********************************************************************************************
void loadImage(uint8_t filenameLen, uint8_t *filename) {

  #define BUF_SIZE 128
  uint8_t buf[BUF_SIZE];

  uint8_t fileId = rn_fileOpen(filenameLen, filename, OPEN_FILE_FLAG_READONLY, 0xff);

  rn_fileHandleSeek(fileId, 7, RN_SEEK_SET);

  vdp_setWriteAddress(0);

  playNoteDelay(0, 0, 100);        
  while (true) {

    uint16_t read = rn_fileHandleReadSeq(fileId, buf, 0, BUF_SIZE);

    if (read == 0)
      break;

    for (uint16_t i = 0; i < read; i++)
      IO_VDPDATA = buf[i];
  }

  rn_fileHandleClose(fileId);
}


// ********************************************************************************************
// INIT MENU
// Initializes the main menu and sets the text mode
// ********************************************************************************************
void initMenu() {

  vdp_setCursor2(27, 22);
  vdp_print("v0.3");

  vdp_setCursor2(5, 18);
  vdp_print("PRESS Q IN GAME TO EXIT");
  vdp_setCursor2(8, 20);
  vdp_print("DJ SURES (C)2023");

  vdp_setCursor2(9, 7);
  vdp_print("PLAYERS:");
  vdp_setCursor2(9, 8);
  vdp_print("BALLS:");
  vdp_setCursor2(9, 9);
  vdp_print("TIME:");
  vdp_setCursor2(9, 11);
  vdp_print("PLAY!");
  vdp_setCursor2(18, 11);
  vdp_print("QUIT!");
}

void introScreens() {

  vdp_clearVRAM();

  vdp_initMSXMode(0x00);
  
  loadImage(53, "https://cloud.nabu.ca/resources/BrickBattle/title.sc2");
  nt_init(baseattack_nt);

  _loopCntr++;
  while (true) {

    if (isKeyPressed() && getChar() == ' ')
      break;
    
    if (_joyStatus[0] & Joy_Button)
      break;
      
    if (_loopCntr % 550 == 0)
      nt_handleNote();    

    _loopCntr++;
  }
}

// ********************************************************************************************
// I AM THE MAIN and MENU
// ********************************************************************************************
void main() {

  initNABULib();
  
  introScreens();

  vdp_clearVRAM();

  vdp_initG2Mode(0, true, false, false, false);

  vdp_loadPatternTable(PATTERN, sizeof(PATTERN));
  vdp_loadColorTable(COLOR, sizeof(COLOR));
  vdp_loadSpritePatternNameTable(3, SPRITE_DATA);

  for (uint8_t i = 33; i < 91; i++)
    vdp_colorizePattern(i, VDP_WHITE, VDP_TRANSPARENT);

  vdp_colorizePattern('>', VDP_LIGHT_GREEN, VDP_TRANSPARENT);

  clearScreen();

  vdp_enableVDPReadyInt();

  initRocks();

  initMenu();
  
  initBricks();

  initPlayers();

  uint8_t menuPos = 0;

  while (true) {

    // PLAYERS
    // -------------------------------------------------
    sprintf(tb, "%u", PLAYER_CNT);
    vdp_setCursor2(18, 7);
    vdp_print(tb);
    if (menuPos == 0)
      vdp_writeCharAtLocation(7, 7, '>');
    else
      vdp_writeCharAtLocation(7, 7, ' ');

    // PELLETS
    // -------------------------------------------------
    sprintf(tb, "%u", ROCK_CNT);
    vdp_setCursor2(18, 8);
    vdp_print(tb);
    if (menuPos == 1)
      vdp_writeCharAtLocation(7, 8, '>');
    else
      vdp_writeCharAtLocation(7, 8, ' ');

    // TIME
    // -------------------------------------------------
    sprintf(tb, "%03u", TIME);
    vdp_setCursor2(18, 9);
    vdp_print(tb);
    if (menuPos == 2)
      vdp_writeCharAtLocation(7, 9, '>');
    else
      vdp_writeCharAtLocation(7, 9, ' ');

    // PLAY
    // -------------------------------------------------
    if (menuPos == 3)
      vdp_writeCharAtLocation(7, 11, '>');
    else
      vdp_writeCharAtLocation(7, 11, ' ');

    // QUIT
    // -------------------------------------------------
    if (menuPos == 4)
      vdp_writeCharAtLocation(16, 11, '>');
    else
      vdp_writeCharAtLocation(16, 11, ' ');

    while (!(_joyStatus[0] & 0b00011111)) {

      if ((_loopCntr % 3 == 0) && vdpStatusRegVal & 0b00100000)
        doCollision();

      if (_loopCntr % 9 == 0)
        nt_handleNote();    

      for (uint8_t r = 0; r < ROCK_CNT; r++) 
        doRock(r);

      vdp_waitVDPReadyInt();
        
      renderRocks();

      _loopCntr++;
    }

    switch (_joyStatus[0] & 0b00011111) {

      case Joy_Up:
          if (menuPos == 0)
            menuPos = 4;
          else
            menuPos--;            
        break;
      case Joy_Down:
          if (menuPos == 4)
            menuPos = 0;
          else
            menuPos++;            
        break;
      case Joy_Right:
        switch (menuPos) {
          case 0:
            if (PLAYER_CNT < PLAYER_MAX)
              PLAYER_CNT++;

            initPlayers();
            break;
          case 1:
            if (ROCK_CNT < ROCK_MAX)
              ROCK_CNT++;

            initRocks();
            break;
          case 2:
            if (TIME < 240)
              TIME+=10;
            break;
        }
        break;
      case Joy_Left:
        switch (menuPos) {
          case 0:
            if (PLAYER_CNT > 0)
              PLAYER_CNT--;

            initPlayers();
            break;
          case 1:
            if (ROCK_CNT > 1)
              ROCK_CNT--;

            initRocks();
            break;
          case 2:
            if (TIME > 10)
              TIME-=10;
            break;
        }
        break;
      case Joy_Button:
        switch (menuPos) {
          case 3:      

            doGameLoop();

            initMenu();

            initRocks();

            nt_init(baseattack_nt);
            break;
          case 4:

            vdp_disableVDPReadyInt();
            nt_stopSounds();
            initNABULIBAudio();    
            playNoteDelay(0, 0, 80);        

            puts("");
            puts("----------------------------------------");
            puts("Brick Battle            DJ Sures (c)2023");
            puts("https://nabu.ca");
            puts("");
            puts("Thank you for playing!");
            puts("");
            return;
        }
      break;
    }

    _joyStatus[0] = 0;
  }
}
