#define BIN_TYPE BIN_CPM

#define DISABLE_VDP

#define DISABLE_HCCA_RX_INT

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../NABULIB/NABU-LIB.h"

#include "../NABULIB/NabuTracker.h"
//#include "./midis/test1-nt.h"
#include "./midis/loop1-nt.h"

void main() {

  initNABULib();
  
  uint16_t _cntr = 0;

  nt_init(loop1_nt);

  while (true) {

    if (_cntr % 420 == 0) {

      nt_handleNote();    
    }

    if (isKeyPressed() && getChar() == 'q')
      break;

      _cntr++;
  }

  nt_stopSounds();
}
