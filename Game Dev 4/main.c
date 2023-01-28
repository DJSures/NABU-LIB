#define FONT_LM80C

#define BIN_TYPE BIN_CPM

#include "../NABULIB/NABU-LIB.h"
#include <stdlib.h>
#include <stdbool.h>
#include <arch/z80.h>

unsigned char SPRITE_DATA[]={
0x90,0x90,0xD0,0xD0,0xB2,0xB7,0x95,0x05,
0x07,0x05,0x05,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x50,
0x50,0x60,0x55,0x55,0x65,0x05,0x05,0x07};

const int xmax = 270;
const int xmin = 16;
const int ymin = 5;
const int ymax = 170;

uint16_t yPos = 50;
uint16_t xPos = 50;

bool xDir = true;
bool yDir = true;

void main() {

  initNABULib();
  
  vdp_initG2Mode(true, false);

  vdp_setSpritePattern(0, SPRITE_DATA);

  uint16_t sprite_name = vdp_spriteInit(0, 0, 0);

  vdp_setSpriteColor(sprite_name, 3);

  while (true) {

    vdp_setSpritePosition(sprite_name, xPos, yPos);

    if (yDir)
      yPos++;
    else 
      yPos--;

    if (yPos > ymax)
      yDir = false;
    else if (yPos < ymin)
      yDir = true;

    if (xDir)
      xPos++;
    else 
      xPos--;

    if (xPos > xmax)
      xDir = false;
    else if (xPos < xmin)
      xDir = true;

    if (isKeyPressed())
      return;

    z80_delay_ms(25);
  }  
}
