//#define FONT_AMIGA
//#define FONT_SET1
//#define FONT_STANDARD
#define FONT_LM80C

// This is a homebrew app
#define BIN_TYPE BIN_HOMEBREW

// Not using any file system or hcca calls
#define DISABLE_HCCA_RX_INT

#include "../../NABULIB/NABU-LIB.h"
#include "../../NABULIB/patterns.h"
#include <arch/z80.h>

void main() {

  // Put the graphics into text mode with the text color 0x01 and background color 0x03
  vdp_initTextMode(0x01, 0x03, false);
  vdp_loadASCIIFont(ASCII);

  initNABULib();

  const uint8_t song[] = { 20, 10, 50, 10, 15, 20, 10, 30, 23, 9, 2, 8, 20, 2, 20, 8, 30, 10, 20, 8, 20, 8 };

  struct {
    uint8_t x;
    uint8_t y;
  } newPosition = { 10, 10 };

  struct {
    uint8_t x;
    uint8_t y;
  } lastPosition = { 10, 10 };

  bool    xDir = true;
  bool    yDir = true;
  uint8_t songPos = 0;

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

    playNoteDelay(0, song[songPos++ % sizeof(song)], 10);

    z80_delay_ms(100);

    // for fun display the keys pressed as trail
    if (isKeyPressed()) 
      vdp_write(getChar());    
  }
}
