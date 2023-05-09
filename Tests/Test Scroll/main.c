#define FONT_STANDARD

#define BIN_TYPE BIN_HOMEBREW

#define DISABLE_CURSOR

#define DISABLE_HCCA_RX_INT

#include "../../NABULIB/NABU-LIB.h"
#include "../../NABULIB/patterns.h"

// This just tests the performance of scroll. Automatic text scroll like a console can 
// be enabled in vdp_initTextMode()
void main() {

  initNABULib();

  vdp_initTextMode(0xf, 0x0, false);
  vdp_loadASCIIFont(ASCII);

  while (true) {

    vdp_fillScreen('A');
    vdp_clearScreen();  

    vdp_fillScreen('B'); 
    vdp_clearRows(10, 15);
    
    // scroll text up
    // -------------------------------
    vdp_setCursor2(0, 0);

    for (uint16_t i = 0; i < 960; i++)
      vdp_write((i % 30) + 65);

    playNoteDelay(0, 10, 100);

    for (int y = 0; y < 24; y++) {
  
//      getChar();

      vdp_scrollTextUp(0, 23);
    }

    // scroll text down
    // -------------------------------

    for (uint16_t i = 0; i < 960; i++)
      vdp_write((i % 30) + 65);

    playNoteDelay(0, 10, 100);

    for (int y = 0; y < 24; y++) {

//      getChar();
      
      vdp_scrollTextDown(0, 23);
    }
  }
}
