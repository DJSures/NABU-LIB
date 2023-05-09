#define FONT_LM80C

#define BIN_TYPE BIN_HOMEBREW

#include "../../NABULIB/NABU-LIB.h"
#include "../../NABULIB/patterns.h"
#include <arch/z80.h> // for z80_delay_ms()

void main() {
 
  // init the nabu lib which does interrupts and all that jazz
  initNABULib();

  // Set the text mode to foreground color white, bg black
  vdp_initTextMode(0xf, 0x0, true);
  vdp_loadASCIIFont(ASCII);

  // This is our intro song
  // col 0: note
  // col 1: length of note (ms)
  const uint8_t song[] = {
       26, 150,
       38, 50,
       36, 100,
       22, 50,
  };

  // Play the music from the little intro cause we be so funky
  for (uint8_t i = 0; i < sizeof(song); i += 2) {

    playNoteDelay(0, song[i], song[i + 1]);

    z80_delay_ms(song[i + 1]);
  }

  // Let's ask the user for their nam
  vdp_setCursor2(0, 0);

  while (true) {

    vdp_print("Hey, gimme your name: ");

    // We're going to get their name but with a max of 8 characters
    uint8_t name[8];

    uint8_t read = readLine(name, 8);

    vdp_newLine();

    vdp_print("Hello, there ");
    vdp_printPart(0, read, name);

    vdp_newLine();
    vdp_newLine();
  }
}
