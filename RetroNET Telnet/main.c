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
#include "vt100.c"

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

  if (key == 13) {


    hcca_writeByte(key);
  } else if (key >= 0x01 && key <= 0x7d) {

    hcca_writeByte(key);
  } else if (key == 127) {

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

// https://stackoverflow.com/questions/10413963/telnet-iac-command-answering
// http://www.faqs.org/rfcs/rfc854.html
// http://www.iana.org/assignments/telnet-options/telnet-options.xhtml
uint8_t _telnetEscapePos = 0;
uint8_t _telnetEscapeOptions[3];

#define OPT_WILL 251
#define OPT_WONT 252
#define OPT_DO 253
#define OPT_DONT 254

void sendOption(uint8_t optionCode, uint8_t option) {

  hcca_writeByte(0xff);
  hcca_writeByte(optionCode);
  hcca_writeByte(option);
}

// Echo
void telnet1() {

  sendOption(OPT_WONT, 1);

  sendOption(OPT_DO, 1);
}

// Supress Go Ahead
void telnet3() {

  sendOption(OPT_DO, 3);
}

// Terminal Speed
void telnet32() {

  sendOption(OPT_WILL, 32);

  const uint8_t tmpBuff1[] = { 255, 250, 32, 0, '2', '4', '0', '0', ',', '2', '4', '0', '0', 255, 240 };
  hcca_writeBytes(tmpBuff1, 15);
}

// X Display
void telnet35() {

  sendOption(OPT_WONT, 35);
}

// Environment
void telnet39() {

  sendOption(OPT_WONT, 39);
}

// NAWS (Negotiate about window size)
void telnet31() {

  sendOption(OPT_WILL, 31);

  const uint8_t tmpBuff1[] = { 255, 250, 31, 0, 40, 0, 24, 255, 240 };
  hcca_writeBytes(tmpBuff1, 9);
}

// Terminal Type
void telnet24() {

  sendOption(OPT_WILL, 24);

  const uint8_t tmpBuff1[] = { 255, 250, 24, 0, 'V', 'T', '1', '0', '0', 255, 240 };
  hcca_writeBytes(tmpBuff1, 11);

  // no extended ascii
  sendOption(OPT_DONT, 17);
}

void doHCCAInput() {

  if (!hcca_isRxBufferAvailable())
    return;

  uint8_t c = hcca_readFromBuffer();

  if (c == 0xff && _telnetEscapePos == 0) {

    // Telnet Escape started

    _telnetEscapeOptions[0] = c;

    _telnetEscapePos = 1;

    return;
  }

  if (_telnetEscapePos > 0) {

    // There's 3 telnet escape values. So let's add them to the option buffer

    _telnetEscapeOptions[_telnetEscapePos] = c;

    _telnetEscapePos++;

    // We have reached our 3 values
    if (_telnetEscapePos == 3) {

      if (_telnetEscapeOptions[2] == 1)
        telnet1();
      else if (_telnetEscapeOptions[2] == 3)
        telnet3();
      else if (_telnetEscapeOptions[2] == 24)
        telnet24();
      else if (_telnetEscapeOptions[2] == 31)
        telnet31();
      else if (_telnetEscapeOptions[2] == 32)
        telnet32();
      else if (_telnetEscapeOptions[2] == 35)
        telnet35();
      else if (_telnetEscapeOptions[2] == 39)
        telnet39();

      _telnetEscapePos = 0;
    }

    return;
  }

  vt100_putc(c);
}

void main2() {

  vdp_initTextMode(0xf, 0x0);

  vdp_setCursor2(0, 0);
  vdp_print("RetroNet Telnet Client (0.6b)");
  vdp_setCursor2(0, 1);
  vdp_print("by DJ Sures (c)2022");

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

  z80_delay_ms(2000);

  vdp_clearRows(0, 2);

  hcca_writeByte(0xa6);

  vt100_init(hcca_writeString);

  while (true) {

    doHCCAInput();

    doKeyInput();
  }
}
