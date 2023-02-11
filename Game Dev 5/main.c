// ************************************************************************
// GAME MAN YEAH!
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

#define RX_BUFFER_SIZE 156

#define DISABLE_CURSOR

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../NABULIB/NABU-LIB.h"
#include "../NABULIB/RetroNET-FileStore.h"
#include "starpattern.c"
#include "../NABULIB/NabuTracker.h"
#include "./midis/loop1-nt.h"

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

#define CHAR_PELLET 0x08
#define CHAR_MAN    0x09

#define SCREEN_MAX_X 239
#define SCREEN_MIN_X 1

#define SCREEN_MAX_Y 176
#define SCREEN_MIN_Y 8

uint8_t tb[64];
uint8_t PLAYER_LIVES = 3;
uint8_t PLAYER_CNT   = 1;
uint8_t PELLET_CNT   = 4;
uint8_t TIME         = 60;

// Main game loop counter used for player & rock movement inertia
uint16_t _loopCntr = 0;

uint8_t _timeCntr = 0;

bool _isGamePlaying = false;

#define SPEED_CURVES 7
uint8_t _speedCurve[SPEED_CURVES] = {
  0,
  15,
  10,
  8,
  5,
  3,
  1
};

#define PLAYER_MAX 4
struct {
           uint8_t  x;
           uint8_t  y;
           bool     xDir;
           bool     yDir;
           uint8_t  xSpeed;
           uint8_t  ySpeed;
           uint8_t  score;
           uint8_t  lives;
           uint8_t  spriteId;
           uint8_t  joyCnt;
           bool     vdpReady;
} _player[PLAYER_MAX] = 
  {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  };

#define ROCK_CNT 6
struct {
           uint8_t  x;
           uint8_t  y;
           bool     xDir;
           bool     yDir;    
           uint8_t  xSpeed;
           uint8_t  ySpeed;
           uint8_t  spriteId;
           bool     vdpReady;
} _rock[ROCK_CNT] = 
  {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  };

#define PELLET_MAX 10
struct {
    uint8_t x;
    uint8_t y;
} _pellets[PELLET_MAX] = { { 0 } };

void updateScore(uint8_t p) {
  
  sprintf(tb, "%03u", _player[p].score);
  
  vdp_setCursor2(p * 7, 0);
  vdp_print(tb);
}

void updateLives(uint8_t p) {

  uint8_t i;

  vdp_setCursor2(p * 7, 1);

  for (i = 0; i < _player[p].lives; i++)
    vdp_write(CHAR_MAN);

  for (; i < 6; i++)
    vdp_write(' ');
}

void updatePellets() {

  for (uint8_t i = 0; i < PELLET_MAX; i++)
    vdp_putPattern(_pellets[i].x, _pellets[i].y, ' ');

  for (uint8_t i = 0; i < PELLET_CNT;) {

    uint8_t x = rand() % 32;
    uint8_t y = (rand() % 21) + 2;

    if (vdp_getCharAtLocationBuf(x, y) != CHAR_PELLET) {

      _pellets[i].x = x;
      _pellets[i].y = y;

      vdp_putPattern(
        x,
        y,
        CHAR_PELLET);    

      i++;
    }
  }
}

// ********************************************************************************************
// STARS
// Flicker stars in the background
// ********************************************************************************************
void doStars() {
    
    uint8_t x = rand() % 32;
    uint8_t y = (rand() % 21) + 2;

    if (vdp_getCharAtLocationBuf(x, y) < 8)
      vdp_putPattern(
        x,
        y,
        rand() % 8);
}

// ********************************************************************************************
// INITIALIZE PLAYER
// Before the game starts, we need to initialize the player
// ********************************************************************************************
void initPlayers() {

  for (uint8_t p = 0; p < PLAYER_MAX; p++) 
    if (p < PLAYER_CNT) {

      _player[p].x = SCREEN_MIN_X;
      _player[p].y = rand() % SCREEN_MAX_Y;
      _player[p].xSpeed = 0;
      _player[p].ySpeed = 0;
      _player[p].xDir = true;
      _player[p].yDir = true;
      _player[p].lives = PLAYER_LIVES;
      _player[p].score = 0;
      _player[p].vdpReady = true;

      _player[p].spriteId = vdp_spriteInit(p, SPRITE_NAME_UFO, _player[p].x, _player[p].y, 3);

      updateScore(p);
      updateLives(p);
    } else {

      _player[p].lives = 0;
      _player[p].vdpReady = false;

      vdp_setCursor2(p * 7, 0);
      vdp_print("       ");

      vdp_setCursor2(p * 7, 1);
      vdp_print("       ");

      vdp_disableSprite(p);
    }
}

// ********************************************************************************************
// INITIALIZE PLAYER
// Before the game starts, we need to initialize the player
// ********************************************************************************************
void initRocks() {

  for (uint8_t p = 0; p < ROCK_CNT; p++) {

    _rock[p].x = (rand() % (SCREEN_MAX_X - SCREEN_MIN_X)) + SCREEN_MIN_X;
    _rock[p].y = (rand() % (SCREEN_MAX_Y - SCREEN_MIN_Y)) + SCREEN_MIN_Y;
    _rock[p].xSpeed = (rand() % (15)) + 1;
    _rock[p].ySpeed = (rand() % (15)) + 1;    
    _rock[p].xDir = rand() % 2;
    _rock[p].yDir = rand() % 2;
    _rock[p].vdpReady = true;
  }

  // ***************************************************
  // Init asteroids
  // ***************************************************
  _rock[0].spriteId = vdp_spriteInit(4, SPRITE_NAME_ROCK1, _rock[0].x, _rock[0].y, VDP_DARK_BLUE);

  _rock[1].spriteId = vdp_spriteInit(5, SPRITE_NAME_ROCK2, _rock[1].x, _rock[1].y, VDP_LIGHT_BLUE);

  _rock[2].spriteId = vdp_spriteInit(6, SPRITE_NAME_ROCK3, _rock[2].x, _rock[2].y, VDP_DARK_RED);

  _rock[3].spriteId = vdp_spriteInit(7, SPRITE_NAME_ROCK4, _rock[3].x, _rock[3].y, VDP_CYAN);

  _rock[4].spriteId = vdp_spriteInit(8, SPRITE_NAME_ROCK5, _rock[4].x, _rock[4].y, VDP_GRAY);

  _rock[5].spriteId = vdp_spriteInit(9, SPRITE_NAME_ROCK6, _rock[5].x, _rock[5].y, VDP_LIGHT_YELLOW);
}

// ********************************************************************************************
// PLAYER (UFO)
// Handle the player's movement and inertia
// ********************************************************************************************
void doPlayer(uint8_t p) {

  if (_player[p].lives == 0)
    return;

  if (_joyStatus[p] & 0b00001111)
    _player[p].joyCnt++;
  else
    _player[p].joyCnt = 0;

  if (_player[p].joyCnt == 10) {

    _player[p].joyCnt = 0;

    // Joystick up/down
    // -------------------------------------------------------
    switch (_joyStatus[p] & 0b00001010) {

      case Joy_Up:

        if (_player[p].ySpeed == 0) {

          _player[p].yDir = false;
          _player[p].ySpeed = 1;
        } else {

          if (_player[p].yDir) {

            _player[p].ySpeed--;
          } else if (_player[p].ySpeed < SPEED_CURVES - 1) {

            _player[p].ySpeed++;
          }
        }

        break;
      case Joy_Down:

        if (_player[p].ySpeed == 0) {

          _player[p].yDir = true;

          _player[p].ySpeed = 1;
        } else {

          if (!_player[p].yDir) {

            _player[p].ySpeed--;
          } else if (_player[p].ySpeed < SPEED_CURVES - 1) {

            _player[p].ySpeed++;
          }
        }

        break;
    }

    // Joystick left/right
    // -------------------------------------------------------
    switch (_joyStatus[p] & 0b00000101) {

      case Joy_Right:

        if (_player[p].xSpeed == 0) {    

          _player[p].xDir = true;

          _player[p].xSpeed = 1;
        } else {

          if (!_player[p].xDir) {

            _player[p].xSpeed--;
          } else if (_player[p].xSpeed < SPEED_CURVES - 1) {

            _player[p].xSpeed++;
          }
        }
        break;
      case Joy_Left:

        if (_player[p].xSpeed == 0) {

          _player[p].xDir = false;

          _player[p].xSpeed = 1;
        } else {

          if (_player[p].xDir) {

            _player[p].xSpeed--;
          } else if (_player[p].xSpeed < SPEED_CURVES - 1) {

            _player[p].xSpeed++;
          }
        }
        break;
    }
  }

  bool needsRedraw = false;

  if (_player[p].xSpeed != 0 && !(_loopCntr % _speedCurve[_player[p].xSpeed])) {

    if (_player[p].xDir) {

      _player[p].x++;

      if (_player[p].x >= SCREEN_MAX_X) {

        if (_isGamePlaying)
          playNoteDelay(2, 50, 5);

        _player[p].xDir = false;
      }
    } else {

      _player[p].x--;

      if (_player[p].x <= SCREEN_MIN_X) {

        if (_isGamePlaying)
          playNoteDelay(2, 50, 5);
  
        _player[p].xDir = true;
      }
    }

    needsRedraw = true;
  }

  if (_player[p].ySpeed != 0 && !(_loopCntr % _speedCurve[_player[p].ySpeed])) {

    if (_player[p].yDir) {

      _player[p].y++;

      if (_player[p].y >= SCREEN_MAX_Y) {

        if (_isGamePlaying)
          playNoteDelay(2, 50, 5);
  
        _player[p].yDir = false;
      }
    } else {

      _player[p].y--;

      if (_player[p].y <= SCREEN_MIN_Y) {

        if (_isGamePlaying)
          playNoteDelay(2, 50, 5);

        _player[p].yDir = true;
      }
    }

    needsRedraw = true;
  }

  if (needsRedraw) {

    if (vdp_getCharAtLocationBuf((_player[p].x + 8) / 8, (_player[p].y + 8) / 8) == CHAR_PELLET) {

      _player[p].score+=2;
      
      updateScore(p);
      updatePellets();
    }

    _player[p].vdpReady = true;      
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

      if (_rock[r].x >= SCREEN_MAX_X) {

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

      if (_rock[r].y >= SCREEN_MAX_Y) {

        if (_isGamePlaying)
          playNoteDelay(2, 0, 10);

        _rock[r].yDir = false;
      }
    } else {

      _rock[r].y--;

      if (_rock[r].y <= SCREEN_MIN_Y) {

        if (_isGamePlaying)
          playNoteDelay(2, 0, 10);
  
        _rock[r].yDir = true;
      }
    }

    needsRedraw = true;
  }

  if (needsRedraw)
    _rock[r].vdpReady = true;  
}


// ********************************************************************************************
// COLLISION
// This is called only when the VDP status has collision detected
// ********************************************************************************************
void doCollision() {

  // Check player collisions
  for (uint8_t p = 0; p < PLAYER_CNT; p++) {

    if (_player[p].lives == 0)
      continue;

    uint8_t xd = _player[p].x + 16;
    uint8_t yd = _player[p].y + 16;

    uint8_t xc = _player[p].x + 8;
    uint8_t yc = _player[p].y + 8;

    // player against player
    for (uint8_t i = 0; i < PLAYER_CNT; i++) {

      if (i == p || _player[i].lives == 0)
        continue;

      if ( (_player[p].x < _player[i].x + 16) && (xd > _player[i].x) && 
           (_player[p].y < _player[i].y + 16) && (yd > _player[i].y) ) {

        if (_isGamePlaying)
          playNoteDelay(2, 38, 5);
  
        if (xc < _player[i].x + 8) {

          _player[p].xDir = false;
          _player[i].xDir = true;
        } else {

          _player[p].xDir = true;
          _player[i].xDir = false;  
        }

        if (yc < _player[i].y + 8) {

          _player[p].yDir = false;
          _player[i].yDir = true;
        } else {

          _player[p].yDir = true;
          _player[i].yDir = false;
        }

        _player[p].xSpeed = SPEED_CURVES - 1;
        _player[p].ySpeed = SPEED_CURVES - 1;    

        _player[i].xSpeed = SPEED_CURVES - 1;
        _player[i].ySpeed = SPEED_CURVES - 1;    

        break;
      }
    }

    // player against rock
    for (uint8_t i = 0; i < ROCK_CNT; i++) {

      if ( (_player[p].x < _rock[i].x + 16) && (xd > _rock[i].x) && 
           (_player[p].y < _rock[i].y + 16) && (yd > _rock[i].y) ) {

        if (_isGamePlaying)
          playNoteDelay(1, 0, 30);
  
        if (xc < _rock[i].x + 8) {

          _player[p].xDir = false;
          _rock[i].xDir = true;
        } else {

          _player[p].xDir = true;
          _rock[i].xDir = false;  
        }

        if (yc < _rock[i].y + 8) {

          _player[p].yDir = false;
          _rock[i].yDir = true;
        } else {

          _player[p].yDir = true;
          _rock[i].yDir = false;
        }

        _player[p].lives--;

        updateLives(p);

        if (_player[p].lives == 0) {

          vdp_disableSprite(p);
         
          _player[p].vdpReady = false;
        } else {        

          _player[p].x = SCREEN_MIN_X;
          _player[p].y = rand() % SCREEN_MAX_Y;
          _player[p].xSpeed = 0;
          _player[p].ySpeed = 0;
          _player[p].xDir = true;
          _player[p].yDir = true;
          _player[p].vdpReady = true;
        }

        _rock[i].xSpeed = (rand() % (5)) + 1;
        _rock[i].ySpeed = (rand() % (5)) + 1;    

        break;
      }
    }
  }

  // check rock collisions
  for (uint8_t r = 0; r < ROCK_CNT; r++) {
    
    uint8_t xd = _rock[r].x + 16;
    uint8_t yd = _rock[r].y + 16;

    uint8_t xc = _rock[r].x + 8;
    uint8_t yc = _rock[r].y + 8;

    for (uint8_t i = 0; i < ROCK_CNT; i++) {

      if (i == r)
        continue;

      if ( (_rock[r].x < _rock[i].x + 16) && (xd > _rock[i].x) && 
           (_rock[r].y < _rock[i].y + 16) && (yd > _rock[i].y) ) {

        if (_isGamePlaying)
          playNoteDelay(2, 12, 10);
  
        if (xc < _rock[i].x + 8) {

          _rock[r].xDir = false;
          _rock[i].xDir = true;
        } else {

          _rock[r].xDir = true;
          _rock[i].xDir = false;
        }

        if (yc < _rock[i].y + 8) {

          _rock[r].yDir = false;
          _rock[i].yDir = true;
        } else {

          _rock[r].yDir = true;
          _rock[i].yDir = false;
        }

        _rock[r].xSpeed = (rand() % (20)) + 4;
        _rock[r].ySpeed = (rand() % (20)) + 4;    

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

// ********************************************************************************************
// PLAYING THE GAME YO!
// Sets up the graphic mode, vdp interrupt, initializes the game, plays it in a loop
// When q is pressed, we exit the loop and that returns to the calling function (ie main menu)
// ********************************************************************************************
void doGameLoop() {

  _isGamePlaying = true;

  _timeCntr = TIME;

  initNABULIBAudio();

  vdp_waitVDPReadyInt();

  vdp_fillScreen(0x00);

  for (uint8_t i = 0; i < 50; i++)
    doStars();

  initPlayers();

  if (PLAYER_CNT != 0)
    updatePellets();

  while (true) {

    if (isKeyPressed() && getChar() == 'q')
      break;

    if ((_loopCntr % 3 == 0) && vdpStatusRegVal & VDP_FLAG_COLLISION)
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

      doStars();

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

  vdp_waitVDPReadyInt();

  for (uint8_t i = 0; i < 50; i++)
    doStars();

  vdp_setCursor2(28, 23);
  vdp_print("v0.8");

  vdp_setCursor2(4, 20);
  vdp_print("(PRESS Q IN GAME TO EXIT)");
  vdp_setCursor2(8, 22);
  vdp_print("DJ SURES (C)2023");

  vdp_setCursor2(9, 6);
  vdp_print("LIVES:");
  vdp_setCursor2(9, 7);
  vdp_print("PLAYERS:");
  vdp_setCursor2(9, 8);
  vdp_print("PELLETS:");
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

  loadImage(61, "https://cloud.nabu.ca/resources/GameManYeah/RetroNETLogo1.SC2");
  playNoteDelay(0, 5, 80);        
  while (true) {

    if (isKeyPressed() && getChar() == ' ')
      break;
    
    if (_joyStatus[0] & Joy_Button)
      break;
  }

  vdp_clearVRAM();
  
  loadImage(60, "https://cloud.nabu.ca/resources/GameManYeah/GameManYeah1.SC2");
  nt_init(loop1_nt);

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

  vdp_initG2Mode(0, true, false, false);

  vdp_loadPatternTable(PATTERN, sizeof(PATTERN));
  vdp_loadColorTable(COLOR, sizeof(COLOR));
  vdp_loadSpritePatternNameTable(11, SPRITE_DATA);

  vdp_fillScreen(0x00);

  vdp_enableVDPReadyInt();

  vdp_waitVDPReadyInt();

  initRocks();

  initMenu();
  
  initPlayers();

  uint8_t menuPos = 0;

  while (true) {

    // LIVES
    // -------------------------------------------------
    sprintf(tb, "%u", PLAYER_LIVES);
    vdp_setCursor2(18, 6);
    vdp_print(tb);
    if (menuPos == 0)
      vdp_writeCharAtLocation(7, 6, '>');
    else
      vdp_writeCharAtLocation(7, 6, ' ');

    // PLAYERS
    // -------------------------------------------------
    sprintf(tb, "%u", PLAYER_CNT);
    vdp_setCursor2(18, 7);
    vdp_print(tb);
    if (menuPos == 1)
      vdp_writeCharAtLocation(7, 7, '>');
    else
      vdp_writeCharAtLocation(7, 7, ' ');

    // PELLETS
    // -------------------------------------------------
    sprintf(tb, "%03u", PELLET_CNT);
    vdp_setCursor2(18, 8);
    vdp_print(tb);
    if (menuPos == 2)
      vdp_writeCharAtLocation(7, 8, '>');
    else
      vdp_writeCharAtLocation(7, 8, ' ');

    // TIME
    // -------------------------------------------------
    sprintf(tb, "%03u", TIME);
    vdp_setCursor2(18, 9);
    vdp_print(tb);
    if (menuPos == 3)
      vdp_writeCharAtLocation(7, 9, '>');
    else
      vdp_writeCharAtLocation(7, 9, ' ');

    // PLAY
    // -------------------------------------------------
    if (menuPos == 4)
      vdp_writeCharAtLocation(7, 11, '>');
    else
      vdp_writeCharAtLocation(7, 11, ' ');

    // QUIT
    // -------------------------------------------------
    if (menuPos == 5)
      vdp_writeCharAtLocation(16, 11, '>');
    else
      vdp_writeCharAtLocation(16, 11, ' ');

    while (!(_joyStatus[0] & 0b00011111)) {

      if ((_loopCntr % 3 == 0) && vdpStatusRegVal & 0b00100000)
        doCollision();

      if (_loopCntr % 9 == 0)
        nt_handleNote();    

      if (_loopCntr % 60 == 0)
        doStars();

      for (uint8_t r = 0; r < ROCK_CNT; r++) 
        doRock(r);

      vdp_waitVDPReadyInt();
        
      renderRocks();

      _loopCntr++;
    }

    switch (_joyStatus[0] & 0b00011111) {

      case Joy_Up:
          if (menuPos == 0)
            menuPos = 5;
          else
            menuPos--;            
        break;
      case Joy_Right:
        switch (menuPos) {
          case 0:
            if (PLAYER_LIVES < 6)
              PLAYER_LIVES++;

            initPlayers();
            break;
          case 1:
            if (PLAYER_CNT < PLAYER_MAX)
              PLAYER_CNT++;

            initPlayers();
            break;
          case 2:
            if (PELLET_CNT < PELLET_MAX)
              PELLET_CNT++;
            break;
          case 3:
            if (TIME < 240)
              TIME+=10;
            break;
        }
        break;
      case Joy_Down:
          if (menuPos == 5)
            menuPos = 0;
          else
            menuPos++;            
        break;
      case Joy_Left:
        switch (menuPos) {
          case 0:
            if (PLAYER_LIVES > 1)
              PLAYER_LIVES--;

            initPlayers();
            break;
          case 1:
            if (PLAYER_CNT > 0)
              PLAYER_CNT--;

            initPlayers();
            break;
          case 2:
            if (PELLET_CNT > 1)
              PELLET_CNT--;
            break;
          case 3:
            if (TIME > 10)
              TIME-=10;
            break;
        }
        break;
      case Joy_Button:
        switch (menuPos) {
          case 4:      

            doGameLoop();

            initMenu();

            nt_init(loop1_nt);
            break;
          case 5:

            vdp_disableVDPReadyInt();
            nt_stopSounds();
            initNABULIBAudio();    
            playNoteDelay(0, 0, 80);        

            puts("");
            puts("----------------------------------------");
            puts("GameManYeah!            DJ Sures (c)2023");
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
