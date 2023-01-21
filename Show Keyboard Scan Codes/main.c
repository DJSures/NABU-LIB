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

#define FONT_LM80C

#define BIN_TYPE BIN_HOMEBREW

#define DISABLE_KEYBOARD_INT

#define DISABLE_HCCA_RX_INT

#include "../NABULIB/NABU-LIB.h"
#include <z80.h>

void main2() {

  vdp_initTextMode(0x1, 0x3, false);

  vdp_setCursor2(0, 0);
  vdp_print("Super Fancy Scancode Displayer!");

  vdp_setCursor2(0, 2);
  vdp_print("By DJ Sures (c)2022");

  // Enable only the Tone Generator on Channel A
  ayWrite(0x7, 0b00111110);

  // Set the amplitude (volume) to maximum on Channel A
  ayWrite(0x8, 0b00001111);

  const uint8_t song[] = { 200, 100, 50, 100, 150, 200, 100, 50, 230, 90 };

  for (uint8_t i = 0; i < sizeof(song); i++) {

    ayWrite(0, song[i]);
    ayWrite(1, 1);

    z80_delay_ms(50);
  }

  ayWrite(0, 0);
  ayWrite(1, 0);

  z80_delay_ms(500);

  for (uint8_t i = 0; i < 31; i++) {

    vdp_setCursor2(i, 0);
    ayWrite(0, i * 10);
    vdp_write(' ');
    z80_delay_ms(5);
  }
  for (uint8_t i = 31; i > 0; i--) {

    vdp_setCursor2(i - 1, 2);
    ayWrite(0, i * 10);
    vdp_write(' ');
    z80_delay_ms(20);
  }

  ayWrite(0, 0);
  ayWrite(1, 0);

  uint8_t listLine = 5;

  uint8_t lastStatus = IO_KEYBOARD_STATUS;

  while (true) {

    vdp_setCursor2(0, 0);
    uint8_t key = IO_KEYBOARD;
    vdp_print("Key/joy (90h): ");
    vdp_writeUInt8(key);
    vdp_print("   ");

    // ---------------------------------------------------------

    vdp_setCursor2(0, 1);
    uint8_t status = IO_KEYBOARD_STATUS;
    vdp_print("Status (91h): ");
    vdp_writeUInt8(status);
    vdp_write(' ');
    vdp_writeUInt8ToBinary(status);
    vdp_print("   ");

    if (lastStatus != status) {

      vdp_setCursor2(0, listLine);

      vdp_print("(90h): ");
      vdp_writeUInt8(key);
      vdp_print("(91h): ");
      vdp_writeUInt8(status);
      vdp_write(' ');
      vdp_writeUInt8ToBinary(status);
      vdp_newLine();

      if (listLine == 22) {

        vdp_scrollTextUp(5, 22);
      } else {

        listLine++;
      }

      lastStatus = status;
    }
  }
}
