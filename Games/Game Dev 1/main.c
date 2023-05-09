#define BIN_TYPE BIN_CPM

#define DISABLE_CURSOR

#include "patterns.h"
#include "../../NABULIB/NABU-LIB.h"
#include <stdio.h>
#include <arch/z80.h>

#define PLAYER_CNT 4

#define SHIP_UP 'a'
#define SHIP_RIGHT 'b'
#define SHIP_DOWN 'c'
#define SHIP_LEFT 'd'
#define SHIP_DEAD 'f'
#define CHAR_BOMB 'i'
#define CHAR_HOME 'h'
#define CHAR_PELLET 'g'

uint8_t PLAYER_LIVES = 3;
uint8_t BOMB_CNT = 100;
uint8_t PELLET_CNT = 100;
uint8_t TIME = 30;

struct {
  uint8_t x;
  uint8_t y;
  uint8_t score;
  uint8_t lives;
  uint8_t prevSpaceChar;
} volatile _player[PLAYER_CNT] = 
  {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
  };

volatile uint8_t  _time = 0;
volatile uint16_t _timeCntr = 0;

void updateScore(uint8_t p) {
  
  uint8_t bf[7];

  sprintf(bf, "P%u:%03u", p + 1, _player[p].score);
  
  vdp_setCursor2(8 * p, 0);
  vdp_print(bf);
}

void updateLives(uint8_t p) {

  vdp_setCursor2(8 * p, 1);
  for (uint8_t i = 0; i < 8; i++)
    vdp_write(' ');

  vdp_setCursor2(8 * p, 1);
  for (uint8_t i = 0; i < _player[p].lives; i++)
    vdp_write(SHIP_UP);
}

void updateTime() {

  if ((_timeCntr % 1000) == 0) {

    uint8_t bf[6];

    sprintf(bf, "T:% 3u", _time);
    vdp_setCursor2(34, 0);
    vdp_print(bf);

    _time--;
  }

  _timeCntr++;
}

void placePlayer(uint8_t p) {

  while (true) {

    uint8_t y = (rand() % 22) + 2;

    if (vdp_getCharAtLocationBuf(0, y) != ' ')
      continue;

    _player[p].x = 0;
    _player[p].y = y;

    vdp_writeCharAtLocation(_player[p].x, _player[p].y, SHIP_RIGHT);

    break;
  }
}

void placeHome() {

  while (true) {

    uint8_t x = (rand() % 40);
    uint8_t y = (rand() % 22) + 2;

    if (vdp_getCharAtLocationBuf(x, y) != ' ')
      continue;

    vdp_writeCharAtLocation(x, y, CHAR_HOME);

    playNoteDelay(1, x * 2, 10);

    break;
  }
}

void initGame() {

  vdp_clearRows(0, 24);

  _time = TIME;
  _timeCntr = 0;

  // place players
  for (uint8_t p = 0; p < PLAYER_CNT; p++) {

    _player[p].score = 0;
    _player[p].lives = PLAYER_LIVES;
    _player[p].prevSpaceChar = ' ';

    placePlayer(p); 
    updateScore(p);
    updateLives(p);
  }

  // place bombs
  for (uint8_t i = 0; i < BOMB_CNT; i++) {

    uint8_t x = (rand() % 40);
    uint8_t y = (rand() % 22) + 2;

    if (vdp_getCharAtLocationBuf(x, y) != ' ')
      continue;

    vdp_writeCharAtLocation(x, y, CHAR_BOMB);
  }

  // place pellets
  for (uint8_t i = 0; i < PELLET_CNT; i++) {

    uint8_t x = (rand() % 40);
    uint8_t y = (rand() % 22) + 2;

    if (vdp_getCharAtLocationBuf(x, y) != ' ')
      continue;

    vdp_writeCharAtLocation(x, y, CHAR_PELLET);
  }

  placeHome();
}

void doPlayerMovement(uint8_t p) {

  if (_player[p].lives == 0)
    return;

  uint8_t prevChar = 0x00;

  switch (_joyStatus[p] & 0b00011111) {

    case Joy_Up:

      if (_player[p].y == 2)
        break;

      vdp_writeCharAtLocation(_player[p].x, _player[p].y, _player[p].prevSpaceChar);

      _player[p].y--;

      prevChar = vdp_getCharAtLocationBuf(_player[p].x, _player[p].y);

      vdp_writeCharAtLocation(_player[p].x, _player[p].y, SHIP_UP);

      break;
    case Joy_Right:

      if (_player[p].x == 39)
        break;

      vdp_writeCharAtLocation(_player[p].x, _player[p].y, _player[p].prevSpaceChar);

      _player[p].x++;

      prevChar = vdp_getCharAtLocationBuf(_player[p].x, _player[p].y);

      vdp_writeCharAtLocation(_player[p].x, _player[p].y, SHIP_RIGHT);
      break;
    case Joy_Down:

      if (_player[p].y == 23)
        break;

      vdp_writeCharAtLocation(_player[p].x, _player[p].y, _player[p].prevSpaceChar);

      _player[p].y++;

      prevChar = vdp_getCharAtLocationBuf(_player[p].x, _player[p].y);

      vdp_writeCharAtLocation(_player[p].x, _player[p].y, SHIP_DOWN);
      break;
    case Joy_Left:

      if (_player[p].x == 0)
        break;

      vdp_writeCharAtLocation(_player[p].x, _player[p].y, _player[p].prevSpaceChar);

      _player[p].x--;

      prevChar = vdp_getCharAtLocationBuf(_player[p].x, _player[p].y);

      vdp_writeCharAtLocation(_player[p].x, _player[p].y, SHIP_LEFT);

      break;
  }

  _joyStatus[p] = 0x00;

  switch (prevChar) {

    case CHAR_BOMB:

      playNoteDelay(0, 2, 30);

      _player[p].lives--;

      vdp_writeCharAtLocation(_player[p].x, _player[p].y, SHIP_DEAD);

      _player[p].prevSpaceChar = ' ';

      updateLives(p);

      placePlayer(p);

      break;
    case CHAR_HOME:

      playNoteDelay(0, 48, 30);

      _player[p].score+=5;

      placeHome();

      break;
    case CHAR_PELLET:

      playNoteDelay(0, 19, 3);

      _player[p].score++;

      _player[p].prevSpaceChar = CHAR_BOMB;

      updateScore(p);
      break;
    case SHIP_DEAD:

      playNoteDelay(0, 38, 10);

      _player[p].lives++;

      if (_player[p].lives > 8)
        _player[p].lives = 8;

      _player[p].prevSpaceChar = CHAR_BOMB;

      updateLives(p);
      break;
    case ' ':
      _player[p].prevSpaceChar = ' ';
      break;
  }
}

void gameLoop() {

  initGame();

  bool _isRunning = true;

  while (_isRunning && _time > 0) {

    if (isKeyPressed()) {

      uint8_t c = getChar();

      switch (c) {
        case 27:
        case 'q':
        case 'Q':
          _isRunning = false;
          break;
      }
    }

    for (uint8_t p = 0; p < PLAYER_CNT; p++)
      doPlayerMovement(p);    

    updateTime();
  }
}

void printTitle() {

  vdp_clearRows(2, 24);
  vdp_setCursor2(0, 1);
  vdp_print("jklmjklmjklmjklmjklmjklmjklmjklmjklmjklm");
  vdp_print("GAME MAN YEAH!         DJ Sures (C)2023");
  vdp_print("jklmjklmjklmjklmjklmjklmjklmjklmjklmjklm");
  vdp_setCursor2(35, 23);
  vdp_print("v0.1");

  vdp_setCursor2(8, 20);
  vdp_print("(PRESS Q IN GAME TO EXIT)");
}

void main() {

  initNABULib();
  
  vdp_initTextMode(VDP_WHITE, VDP_BLACK, false);
  vdp_loadASCIIFont(ASCII);

  printTitle();

  uint8_t bf[64];
  uint8_t menuPos = 0;
  uint16_t srandCnt = 0;

  while (true) {

    sprintf(bf, "LIVES: %u", PLAYER_LIVES);
    vdp_setCursor2(13, 6);
    vdp_print(bf);
    if (menuPos == 0)
      vdp_writeCharAtLocation(12, 6, SHIP_RIGHT);
    else
      vdp_writeCharAtLocation(12, 6, ' ');

    sprintf(bf, "BOMBS: % 3u", BOMB_CNT);
    vdp_setCursor2(13, 7);
    vdp_print(bf);
    if (menuPos == 1)
      vdp_writeCharAtLocation(12, 7, SHIP_RIGHT);
    else
      vdp_writeCharAtLocation(12, 7, ' ');

    sprintf(bf, "PELLETS: %03u", PELLET_CNT);
    vdp_setCursor2(13, 8);
    vdp_print(bf);
    if (menuPos == 2)
      vdp_writeCharAtLocation(12, 8, SHIP_RIGHT);
    else
      vdp_writeCharAtLocation(12, 8, ' ');

    sprintf(bf, "TIME: %03u", TIME);
    vdp_setCursor2(13, 9);
    vdp_print(bf);
    if (menuPos == 3)
      vdp_writeCharAtLocation(12, 9, SHIP_RIGHT);
    else
      vdp_writeCharAtLocation(12, 9, ' ');

    vdp_setCursor2(13, 11);
    vdp_print("PLAY!");
    if (menuPos == 4)
      vdp_writeCharAtLocation(12, 11, SHIP_RIGHT);
    else
      vdp_writeCharAtLocation(12, 11, ' ');

    vdp_setCursor2(22, 11);
    vdp_print("QUIT!");
    if (menuPos == 5)
      vdp_writeCharAtLocation(21, 11, SHIP_RIGHT);
    else
      vdp_writeCharAtLocation(21, 11, ' ');

    while (!(_joyStatus[0] & 0b00011111))
      srandCnt++;

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
            if (BOMB_CNT < 255)
              BOMB_CNT++;
            break;
          case 2:
            if (PELLET_CNT < 255)
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
            if (BOMB_CNT > 1)
              BOMB_CNT--;
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
            srand(srandCnt);    
            gameLoop();

            playNoteDelay(1, 0, 100);

            for (uint8_t i = 0; i < 23; i++) {

              vdp_scrollTextUp(1, 23);
              z80_delay_ms(2);
            }

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
