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

#include "../NABULIB/NABU-LIB.h"

void main2() {

  vdp_initTextMode(0xf, 0x0, false);

  z80_delay_ms(1000);

  for (int x = 0; x < 10; x++) {

    vdp_setCursor2(0, 0);

    for (uint16_t i = 0; i < 960; i++)
      vdp_write((i % 30) + 65, true);

    for (int y = 0; y < 24; y++)
      vdp_scrollTextUp(0, 23);

    z80_delay_ms(100);
  }

  while (true);
}
