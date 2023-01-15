static void orgit() __naked {
  __asm
  org     0x140D
    nop
    nop
    nop
    __endasm;
}

void main2();

void main() {

  main2();
}

//#define FONT_AMIGA
//#define FONT_SET1
//#define FONT_STANDARD
#define LM80C

#include "../NABULIB/NABU-LIB.h"
#include <z80.h> // for z80_delay_ms()

void main2() {
  
  // Put the graphics into text mode with the text color 0x01 and background color 0x03
  vdp_initTextMode(0x01, 0x03, false);

  // Enable only the Tone Generator on Channel A
  ayWrite(0x7, 0b00111110);

  // Set the amplitude (volume) to maximum on Channel A
  ayWrite(0x8, 0b00001111);

  const uint8_t song[] = { 200, 100, 50, 100, 150, 200, 100, 50, 230, 90, 20, 80, 20, 20, 20, 80, 50, 50, 20, 80, 20, 80 };

  struct {
    uint8_t x;
    uint8_t y;
  } newPosition;

  struct {
    uint8_t x;
    uint8_t y;
  } lastPosition;

  bool    xDir = true;
  bool    yDir = true;
  uint8_t songPos = 0;

  lastPosition.x = 0;
  lastPosition.y = 0;

  while (true) {

    // Have we hit the X axis limits? If so, reverse directions
    if (lastPosition.x >= 29)
      xDir = false;
    else if (lastPosition.x == 0)
      xDir = true;

    // What direction are we moving
    if (xDir)
      newPosition.x = lastPosition.x + 1;
    else
      newPosition.x = lastPosition.x - 1;

    // Have we hit the Y axis limits? If so, reverse directions
    if (lastPosition.y >= 23)
      yDir = false;
    else if (lastPosition.y == 0)
      yDir = true;

    // What direction are we moving
    if (yDir)
      newPosition.y = lastPosition.y + 1;
    else
      newPosition.y = lastPosition.y - 1;

    // Move the cursor to the last position that we were and erase what was there
    vdp_setCursor2(lastPosition.x, lastPosition.y);
    vdp_print("          ");

    // Write the text in the new position
    vdp_setCursor2(newPosition.x, newPosition.y);
    vdp_print("Hello NABU");

    // Save the new position into the last position
    lastPosition.x = newPosition.x;
    lastPosition.y = newPosition.y;

    ayWrite(0, song[songPos++ % sizeof(song)]);
    ayWrite(1, 1);

    z80_delay_ms(100);
  }
}
