#define DISABLE_HCCA_RX_INT

#define BIN_TYPE BIN_CPM

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "patterns.h"
#include "../NABULIB/NABU-LIB.h"

unsigned char SPRITE_UFO[]={
0x00,0x00,0x00,0x03,0x04,0x08,0x28,0x68,
0x7F,0x3F,0x0F,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x80,0x40,0x20,0x28,0x2C,
0xFC,0xF8,0xE0,0x00,0x00,0x00,0x00,0x00,
};

unsigned char SPRITE_ROCK1[]={
0x12,0x1B,0x1F,0x7F,0xFF,0xFF,0x7F,0x3F,
0x3F,0xFF,0xFF,0xDF,0x1F,0x3F,0x7C,0x60,
0x38,0x38,0xF8,0xFC,0xFF,0xFF,0xFF,0xFD,
0xFE,0xFE,0xFC,0xFC,0xFC,0xBE,0x18,0x18,
};

unsigned char SPRITE_ROCK2[]={
0x38,0x3F,0x3F,0x3F,0x3F,0x7F,0xFF,0xFF,
0x3F,0x3F,0x3F,0x7F,0x7F,0x1F,0x3F,0x32,
0x07,0xCE,0xFC,0xFF,0xFE,0xFE,0xFF,0xFF,
0xFE,0xFE,0xFE,0xFC,0xFC,0xDE,0x1E,0x0E,
};

unsigned char SPRITE_ROCK3[]={
0x30,0x77,0x3F,0x3F,0x1F,0x3F,0xFF,0xFF,
0x7F,0x3F,0x3F,0x1F,0x7F,0x7F,0xF3,0xE1,
0x70,0xFC,0xFE,0xFE,0xFF,0xFF,0xFE,0xFC,
0xFC,0xF8,0xFC,0xFE,0xFF,0xFE,0xB6,0x14,
};

unsigned char SPRITE_PELLET[]={
0x00,0x00,0x07,0x0F,0x1F,0x3F,0x7F,0x7F,
0x7F,0x7F,0x7F,0x3F,0x1B,0x0C,0x07,0x00,
0x00,0x00,0xC0,0xE0,0xB0,0xD8,0xDC,0xEC,
0xEC,0xFC,0xFC,0xF8,0xF0,0xE0,0xC0,0x00};

unsigned char SPRITE_DEAD[]={
0x06,0x02,0x07,0x0D,0x0F,0x06,0x03,0x01,
0x07,0x09,0x09,0x01,0x02,0x04,0x04,0x0C,
0xC0,0x80,0xC0,0x60,0xE0,0xC0,0x80,0x00,
0xC0,0x20,0x20,0x00,0x80,0x40,0x40,0x60};

#define SCREEN_MAX_X 250
#define SCREEN_MIN_X 50

#define SCREEN_MAX_Y 150
#define SCREEN_MIN_Y 10

uint8_t PLAYER_LIVES = 3;
uint8_t PLAYER_CNT = 1;
uint8_t PELLET_CNT = 5;
uint8_t TIME = 30;

// Used for getting the r register from the z80 to seed srand()
volatile uint8_t r = 0;

// Main game loop counter used for player & rock movement inertia
uint16_t _loopCntr = 0;

// Set in the VDP ISR if there's sprite collision
volatile bool _isCollision = false;

#define SPEED_CURVES 6
uint8_t _speedCurve[SPEED_CURVES] = {
  0,
  15,
  10,
  5,
  2,
  1
};

#define PLAYER_MAX 4
struct {
  volatile uint8_t  x;
  volatile uint8_t  y;
  bool     xDir;
  bool     yDir;
  uint8_t  xSpeed;
  uint8_t  ySpeed;
  uint8_t  score;
  uint8_t  lives;
  volatile uint16_t sprite;
  volatile bool     vdpReady;
} _player[PLAYER_MAX] = 
  {
    { 0 },
    { 0 },
    { 0 },
    { 0 },
  };

#define ROCK_CNT 6
struct {
  volatile uint8_t  x;
  volatile uint8_t  y;
  bool     xDir;
  bool     yDir;    
  uint8_t  xSpeed;
  uint8_t  ySpeed;
  volatile uint16_t sprite;
  volatile bool     vdpReady;
} _rock[ROCK_CNT] = 
  {
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
  };


// ********************************************************************************************
// INITIALIZE THE GAME STUFFSSSS
// Before the game starts, we need to initialize variables and randomize the playfield
// ********************************************************************************************
void initGame() {

  for (uint8_t p = 0; p < PLAYER_CNT; p++) {

    _player[p].x = SCREEN_MIN_X;
    _player[p].y = rand() % SCREEN_MAX_Y;
    _player[p].xSpeed = 0;
    _player[p].ySpeed = 0;
    _player[p].xDir = true;
    _player[p].yDir = true;
    _player[p].lives = PLAYER_LIVES;
    _player[p].score = 0;
    _player[p].vdpReady = true;
  }

  for (uint8_t p = 0; p  < ROCK_CNT; p++) {

    _rock[p].x = (rand() % (SCREEN_MAX_X - SCREEN_MIN_X)) + SCREEN_MIN_X;
    _rock[p].y = (rand() % (SCREEN_MAX_Y - SCREEN_MIN_Y)) + SCREEN_MIN_Y;
    _rock[p].xSpeed = (rand() % (30)) + 1;
    _rock[p].ySpeed = (rand() % (30)) + 1;    
    _rock[p].xDir = rand() % 2;
    _rock[p].yDir = rand() % 2;
    _rock[p].vdpReady = true;
  }
}

void initSprites() {

  // ***************************************************
  // Init player UFOs
  // ***************************************************
  vdp_setSpritePattern(0, SPRITE_UFO);
  _player[0].sprite = vdp_spriteInit(0, 0, 3);

  vdp_setSpritePattern(1, SPRITE_UFO);
  _player[1].sprite = vdp_spriteInit(1, 1, 3);

  vdp_setSpritePattern(2, SPRITE_UFO);
  _player[2].sprite = vdp_spriteInit(2, 2, 3);

  vdp_setSpritePattern(3, SPRITE_UFO);
  _player[3].sprite = vdp_spriteInit(3, 3, 3);

  // ***************************************************
  // Init asteroids
  // ***************************************************
  vdp_setSpritePattern(4, SPRITE_ROCK1);
  _rock[0].sprite = vdp_spriteInit(4, 4, VDP_DARK_BLUE);
  
  vdp_setSpritePattern(5, SPRITE_ROCK2);
  _rock[1].sprite = vdp_spriteInit(5, 5, VDP_LIGHT_BLUE);
  
  vdp_setSpritePattern(6, SPRITE_ROCK3);
  _rock[2].sprite = vdp_spriteInit(6, 6, VDP_DARK_RED);
  
  vdp_setSpritePattern(7, SPRITE_ROCK1);
  _rock[3].sprite = vdp_spriteInit(7, 7, VDP_CYAN);
  
  vdp_setSpritePattern(8, SPRITE_ROCK2);
  _rock[4].sprite = vdp_spriteInit(8, 8, VDP_GRAY);
  
  vdp_setSpritePattern(9, SPRITE_ROCK3);
  _rock[5].sprite = vdp_spriteInit(9, 9, VDP_LIGHT_YELLOW);
}

// ********************************************************************************************
// UFO
// Handle the UFO movement and inertia
// ********************************************************************************************

void doPlayer(uint8_t p) {

  switch (_joyStatus[p] & 0b00011111) {

    case Joy_Up:

      if (_player[p].ySpeed == 0) {

        _player[p].yDir = false;
        _player[p].ySpeed = 1;
      } else {

        if (_player[p].yDir) {

          _player[p].ySpeed--;
        } else if (_player[p].ySpeed < SPEED_CURVES){

          _player[p].ySpeed++;
        }
      }

      break;
    case Joy_Right:

      if (_player[p].xSpeed == 0) {    

        _player[p].xDir = true;

        _player[p].xSpeed = 1;
      } else {

        if (!_player[p].xDir) {

          _player[p].xSpeed--;
        } else if (_player[p].xSpeed < SPEED_CURVES){

          _player[p].xSpeed++;
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
        } else if (_player[p].ySpeed < SPEED_CURVES){

          _player[p].ySpeed++;
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
        } else if (_player[p].xSpeed < SPEED_CURVES){

          _player[p].xSpeed++;
        }
      }
      break;
  }

  _joyStatus[p] = 0;

  bool needsRedraw = false;

  if (_player[p].xSpeed != 0 && !(_loopCntr % _speedCurve[_player[p].xSpeed])) {

    if (_player[p].xDir) {

      _player[p].x++;

      if (_player[p].x > SCREEN_MAX_X) {

        playNoteDelay(0, 50, 5);
        _player[p].xDir = false;
      }
    } else {

      _player[p].x--;

      if (_player[p].x < SCREEN_MIN_X) {

        playNoteDelay(0, 50, 5);
        _player[p].xDir = true;
      }
    }

    needsRedraw = true;
  }

  if (_player[p].ySpeed != 0 && !(_loopCntr % _speedCurve[_player[p].ySpeed])) {

    if (_player[p].yDir) {

      _player[p].y++;

      if (_player[p].y > SCREEN_MAX_Y) {

        playNoteDelay(0, 50, 5);
        _player[p].yDir = false;
      }
    } else {

      _player[p].y--;

      if (_player[p].y < SCREEN_MIN_Y) {

        playNoteDelay(0, 50, 5);
        _player[p].yDir = true;
      }
    }

    needsRedraw = true;
  }

  if (needsRedraw) 
    _player[p].vdpReady = true;      
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

      if (_rock[r].x > SCREEN_MAX_X) {

        playNoteDelay(0, 0, 10);
        _rock[r].xDir = false;
      }
    } else {

      _rock[r].x--;

      if (_rock[r].x < SCREEN_MIN_X) {

        playNoteDelay(0, 0, 10);
        _rock[r].xDir = true;
      }
    }

    needsRedraw = true;
  }

  if (!(_loopCntr % _rock[r].ySpeed)) {

    if (_rock[r].yDir) {

      _rock[r].y++;

      if (_rock[r].y > SCREEN_MAX_Y) {

        playNoteDelay(0, 0, 10);
        _rock[r].yDir = false;
      }
    } else {

      _rock[r].y--;

      if (_rock[r].y < SCREEN_MIN_Y) {

        playNoteDelay(0, 0, 10);
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

    uint8_t xd = _player[p].x + 16;
    uint8_t yd = _player[p].y + 16;

    uint8_t xc = _player[p].x + 8;
    uint8_t yc = _player[p].y + 8;

    for (uint8_t i = 0; i < ROCK_CNT; i++) {

      if ( (_player[p].x < _rock[i].x + 16) && (xd > _rock[i].x) && 
           (_player[p].y < _rock[i].y + 16) && (yd > _rock[i].y) ) {

        playNoteDelay(1, 38, 5);

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

        _player[p].xSpeed = SPEED_CURVES - 1;
        _player[p].ySpeed = SPEED_CURVES - 1;    

        _rock[i].xSpeed = (rand() % (10)) + 1;
        _rock[i].ySpeed = (rand() % (10)) + 1;    

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

        playNoteDelay(0, 12, 10);

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

        _rock[r].xSpeed = (rand() % (20)) + 1;
        _rock[r].ySpeed = (rand() % (20)) + 1;    

        break;
      }
    }
  }
}


// ********************************************************************************************
// VDP ISR
// Loops are too slow so each rock and player is inline
// ********************************************************************************************
void myVdpISR() __naked {

  __asm
    push	hl;      
    push  bc;      
    push  de;      
    push  af;      
    push  iy;
    push  ix;
  __endasm;

  uint8_t vdpStatus = IO_VDPLATCH;

  _isCollision = vdpStatus & 0b00100000;
  
  // UFOs
  // ------------------------------------------------------------------
  if (PLAYER_CNT > 0 && _player[0].vdpReady) {

    vdp_setSpritePosition(_player[0].sprite, _player[0].x, _player[0].y);

    _player[0].vdpReady = false;
  }

  if (PLAYER_CNT > 1 && _player[1].vdpReady) {

    vdp_setSpritePosition(_player[1].sprite, _player[1].x, _player[1].y);

    _player[1].vdpReady = false;
  }

  if (PLAYER_CNT >2 &&_player[2].vdpReady) {

    vdp_setSpritePosition(_player[2].sprite, _player[2].x, _player[2].y);

    _player[2].vdpReady = false;
  }

  if (PLAYER_CNT > 3 && _player[3].vdpReady) {

    vdp_setSpritePosition(_player[3].sprite, _player[3].x, _player[3].y);

    _player[3].vdpReady = false;
  }

  // ROCKS
  // ------------------------------------------------------------------
  if (_rock[0].vdpReady) {

    vdp_setSpritePosition(_rock[0].sprite, _rock[0].x, _rock[0].y);  

    _rock[0].vdpReady = false;
  }

  if (_rock[1].vdpReady) {

    vdp_setSpritePosition(_rock[1].sprite, _rock[1].x, _rock[1].y);  

    _rock[1].vdpReady = false;
  }

  if (_rock[2].vdpReady) {

    vdp_setSpritePosition(_rock[2].sprite, _rock[2].x, _rock[2].y);  

    _rock[2].vdpReady = false;
  }

  if (_rock[3].vdpReady) {

    vdp_setSpritePosition(_rock[3].sprite, _rock[3].x, _rock[3].y);  

    _rock[3].vdpReady = false;
  }

  if (_rock[4].vdpReady) {

    vdp_setSpritePosition(_rock[4].sprite, _rock[4].x, _rock[4].y);  

    _rock[4].vdpReady = false;
  }

  if (_rock[5].vdpReady) {

    vdp_setSpritePosition(_rock[5].sprite, _rock[5].x, _rock[5].y);  

    _rock[5].vdpReady = false;
  }

  __asm
    pop ix;
    pop iy;
    pop af;      
    pop de;      
    pop bc;      
    pop hl;      
    ei;
    reti;
  __endasm;
}


// ********************************************************************************************
// PLAYING THE GAME YO!
// Sets up the graphic mode, vdp interrupt, initializes the game, plays it in a loop
// When q is pressed, we exit the loop and that returns to the calling function (ie main menu)
// ********************************************************************************************
void doGameLoop() {

  vdp_initG2Mode(true, false);

  vdp_setBackDropColor(0);

  initGame();

  initSprites();

  vdp_addISR(myVdpISR);

  while (true) {

    if (isKeyPressed() && getChar() == 'q')
      break;

   for (uint8_t p = 0; p < PLAYER_CNT; p++)
      doPlayer(p);

    for (uint8_t r = 0; r < ROCK_CNT; r++) 
      doRock(r);
    
    if (_isCollision)
      doCollision();

    _loopCntr++;
  }

  vdp_removeISR();
}

// ********************************************************************************************
// INIT MENU
// Initializes the main menu and sets the text mode
// ********************************************************************************************
void printTitle() {

  vdp_initTextMode(VDP_WHITE, VDP_BLACK, false);
  vdp_clearRows(0, 24);
  vdp_setCursor2(0, 0);
  vdp_print("jklmjklmjklmjklmjklmjklmjklmjklmjklmjklm");
  vdp_print("GAME MAN YEAH!          DJ Sures (C)2023");
  vdp_print("jklmjklmjklmjklmjklmjklmjklmjklmjklmjklm");
  vdp_setCursor2(35, 23);
  vdp_print("v0.2");

  vdp_setCursor2(8, 20);
  vdp_print("(PRESS Q IN GAME TO EXIT)");
}


// ********************************************************************************************
// I AM THE MAIN and MENU
// ********************************************************************************************
void main() {

  initNABULib();

  __asm
    ld a, r;
    ld (_r), a;
  __endasm;

  srand(r);

  printTitle();

  uint8_t bf[64];
  uint8_t menuPos = 0;
  uint16_t srandCnt = 0;

  while (true) {

    // LIVES
    // -------------------------------------------------
    sprintf(bf, "LIVES: %u", PLAYER_LIVES);
    vdp_setCursor2(13, 6);
    vdp_print(bf);
    if (menuPos == 0)
      vdp_writeCharAtLocation(11, 6, 'b');
    else
      vdp_writeCharAtLocation(11, 6, ' ');

    // PLAYERS
    // -------------------------------------------------
    sprintf(bf, "PLAYERS: %u", PLAYER_CNT);
    vdp_setCursor2(13, 7);
    vdp_print(bf);
    if (menuPos == 1)
      vdp_writeCharAtLocation(11, 7, 'b');
    else
      vdp_writeCharAtLocation(11, 7, ' ');

    // PELLETS
    // -------------------------------------------------
    sprintf(bf, "PELLETS: %03u", PELLET_CNT);
    vdp_setCursor2(13, 8);
    vdp_print(bf);
    if (menuPos == 2)
      vdp_writeCharAtLocation(11, 8, 'b');
    else
      vdp_writeCharAtLocation(11, 8, ' ');

    // TIME
    // -------------------------------------------------
    sprintf(bf, "TIME: %03u", TIME);
    vdp_setCursor2(13, 9);
    vdp_print(bf);
    if (menuPos == 3)
      vdp_writeCharAtLocation(11, 9, 'b');
    else
      vdp_writeCharAtLocation(11, 9, ' ');

    // PLAY
    // -------------------------------------------------
    vdp_setCursor2(13, 11);
    vdp_print("PLAY!");
    if (menuPos == 4)
      vdp_writeCharAtLocation(11, 11, 'b');
    else
      vdp_writeCharAtLocation(11, 11, ' ');

    // QUIT
    // -------------------------------------------------
    vdp_setCursor2(22, 11);
    vdp_print("QUIT!");
    if (menuPos == 5)
      vdp_writeCharAtLocation(20, 11, 'b');
    else
      vdp_writeCharAtLocation(20, 11, ' ');

    while (!(_joyStatus[0] & 0b00011111));

    switch (_joyStatus[0] & 0b00011111) {

      case Joy_Up:
          playNoteDelay(0, 10, 10);
          if (menuPos == 0)
            menuPos = 5;
          else
            menuPos--;            
        break;
      case Joy_Right:
          playNoteDelay(0, 20, 10);
        switch (menuPos) {
          case 0:
            if (PLAYER_LIVES < 8)
              PLAYER_LIVES++;
            break;
          case 1:
            if (PLAYER_CNT < PLAYER_MAX)
              PLAYER_CNT++;
            break;
          case 2:
            if (PELLET_CNT < 10)
              PELLET_CNT++;
            break;
          case 3:
            if (TIME < 255)
              TIME++;
            break;
        }
        break;
      case Joy_Down:
          playNoteDelay(0, 5, 10);
          if (menuPos == 5)
            menuPos = 0;
          else
            menuPos++;            
        break;
      case Joy_Left:
          playNoteDelay(0, 15, 10);
        switch (menuPos) {
          case 0:
            if (PLAYER_LIVES > 1)
              PLAYER_LIVES--;
            break;
          case 1:
            if (PLAYER_CNT > 0)
              PLAYER_CNT--;
            break;
          case 2:
            if (PELLET_CNT > 1)
              PELLET_CNT--;
            break;
          case 3:
            if (TIME > 10)
              TIME--;
            break;
        }
        break;
      case Joy_Button:
        switch (menuPos) {
          case 4:      

            doGameLoop();

            printTitle();
            break;
          case 5:
            playNoteDelay(0, 0, 30);
            return;
        }
      break;
    }

    _joyStatus[0] = 0;
  }
}
