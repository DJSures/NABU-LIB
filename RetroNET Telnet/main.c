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

void writeCharToScreen(uint8_t c) {

  if (c == 13) {

    if (vdp_cursor.y == 23) {

      vdp_scrollTextUp(0, 23);

      vdp_cursor.x = 0;
      vdp_cursor.y = 23;
    } else {

      vdp_newLine();
    }
  } else if (c >= 0x20 && c <= 0x7d) {

    if (vdp_cursor.x == 39 && vdp_cursor.y == 23) {

      vdp_scrollTextUp(0, 23);

      vdp_cursor.x = 0;
      vdp_cursor.y = 23;
    }

    vdp_write(c, true);
  }
}

void doKeyInput() {

  uint8_t key = isKeyPressed();

  if (key >= 0x20 && key <= 0x7d) {

    //    writeCharToScreen(key);

    hcca_writeByte(key);
  } else if (key == 13) {

    // enter
//    writeCharToScreen(key);

    hcca_writeByte(key);
  } else if (key == 127) {

    // backspace
    if (vdp_cursor.x == 0) {

      vdp_cursor.x = 39;
      vdp_cursor.y--;
    } else {

      vdp_cursor.x--;
    }

    vdp_writeCharAtLocation(vdp_cursor.x, vdp_cursor.y, ' ');

    hcca_writeByte(0x08);
  }
}

void doHCCAInput() {

  if (!hcca_isRxBufferAvailable())
    return;

  uint8_t i = hcca_readFromBuffer();

  if (i >= 0x20 && i <= 0x7d) {

    writeCharToScreen(i);
  } else if (i == 13) {

    // enter

    writeCharToScreen(i);

  } else if (i == 0x07) {

    playNoteDelay(0, 30, 5);
  }
}

void main2() {

  vdp_initTextMode(0xf, 0x0);

  vdp_setCursor2(0, 0);
  vdp_print("RetroNet Telnet Client (0.1b)");

  hcca_enableReceiveBufferInterrupt();

  initAudio();

  // col 0: note
  // col 1: length of note (ms)
  const uint8_t song[] = {
       36, 250,
       28, 50,
       36, 100,
       42, 100,
  };

  for (uint8_t i = 0; i < sizeof(song); i += 2) {

    playNoteDelay(0, song[i], song[i + 1]);

    z80_delay_ms(song[i + 1]);
  }

  vdp_clearRows(0, 1);

  vdp_setCursor2(0, 0);

  hcca_writeByte(0xa6);

  while (true) {

    doHCCAInput();

    doKeyInput();
  }
}
