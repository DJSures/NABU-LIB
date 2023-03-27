#define BIN_TYPE BIN_CPM

#define FONT_LM80C
#define DISABLE_HCCA_RX_INT
#define DISABLE_CURSOR

#define DEBUG_VDP_INT

#include <stdint.h>
#include "../NABULIB/NABU-LIB.h"
#include "../NABULIB/patterns.h"

void init() {

  initNABULib();
  vdp_clearVRAM();
  vdp_initG2Mode(5, false, false, false, false);
  vdp_enableVDPReadyInt();

  vdp_loadASCIIFont(ASCII);

  for (uint8_t c = ' '; c < 'z'; c++)
    vdp_colorizePattern(c, VDP_WHITE, VDP_BLACK);
}

void main(void) {

  init();

  uint8_t c = 'A';

  while(true) {

    if (isKeyPressed()) 
        break;

    // this is slow and takes longer than a vdp interrupt refresh cycle
    for (uint8_t y = 0; y < 24; y++)
      for (uint8_t x = 0; x < 32; x++)
        vdp_setCharAtLocationBuf(x, y, c);

    c++;
    
    if (c == 'Z')
      c = 'A';

    // so refresh the vdpIsReady flag so we force the vdp interrupt to wait another cycle    
    vdpIsReady = false;
    vdp_waitVDPReadyInt();
    vdp_refreshViewPort();
    playNoteDelay(0, 1, 1);    
  }
}