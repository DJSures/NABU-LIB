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

uint16_t _inCntr = 0;

void main2() {

  vdp_initTextMode(0x0f, 0x00);

  hcca_enableReceiveBufferInterrupt();

  initAudio();

  while (true) {

    vdp_setCursor2(0, 2);
    vdp_writeUInt8ToBinary(ayRead(IOPORTB));

    playNoteDelay(0, 55, 300);

    vdp_setCursor2(0, 0);
    vdp_writeUInt16(_rxWritePos);

    vdp_setCursor2(0, 1);
    vdp_writeUInt16(_inCntr);

    _inCntr++;

    for (int p = 0; p < 5; p++)
      hcca_writeByte(0xa1);

    vdp_clearRows(5, 6);

    vdp_setCursor2(0, 5);
    while (hcca_isRxBufferAvailable())
      vdp_write(hcca_readFromBuffer(), true);

    z80_delay_ms(1000);
  }
}
