#define BIN_TYPE BIN_CPM

#define DISABLE_KEYBOARD_INT

#define DISABLE_VDP

#define RX_BUFFER_SIZE 4096

#include <conio.h>
#include "../NABULIB/NABU-LIB.h"
#include <arch/z80.h>

bool _isRunning = true;

void doKeyInput() {

  uint8_t key = getch();
//  uint8_t key = LastKeyPressed;

  switch (key) {
    case 0xea:
    
      _isRunning = false;
      break;
    case 127:
      
      hcca_writeByte(0x08);
      break;
    case 0xe1:

      // left
      hcca_writeByte(27);
      hcca_writeByte('D');
      break;
    case 0xe0:

      // right
      hcca_writeByte(27);
      hcca_writeByte('C');
      break;
    case 0xe2:
  
      // up
      hcca_writeByte(27);
      hcca_writeByte('A');
      break;
    case 0xe3:
  
      // down
      hcca_writeByte(27);
      hcca_writeByte('B');
      break;
    default:
      if (key < 0xe0)    
        hcca_writeByte(key);
      break;       
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
  hcca_writeBytes(0, 15, tmpBuff1);
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

  const uint8_t tmpBuff1[] = { 255, 250, 31, 0, 80, 0, 24, 255, 240 };
  hcca_writeBytes(0, 9, tmpBuff1);
}

// Terminal Type
void telnet24() {

  sendOption(OPT_WILL, 24);

  const uint8_t tmpBuff1[] = { 255, 250, 24, 0, 'V', 'T', '5', '2', 255, 240 };
  hcca_writeBytes(0, 10, tmpBuff1);

  // no extended ascii
  sendOption(OPT_DONT, 17);
}

void doHCCAInput() {

  uint8_t c = hcca_readByte();

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

  putchar(c);
}

void main() {

  initNABULib();

  puts("");
  puts("RetroNet Telnet Client CPM (0.9b)");
  puts("by DJ Sures (c)2023");
  puts("");
  puts("");
  puts("*Hint: Press TV<->NABU key to exit"); 
  puts("");
  
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

  hcca_writeByte(0xa6);

  while (_isRunning) {

    while (hcca_isRxBufferAvailable())
      doHCCAInput();

    while (kbhit())
//    while (isKeyPressed())
      doKeyInput();
  }

  puts("");
  puts("");
  puts("Exiting telnet...");

  hcca_exitRetroNETBridgeMode();
  
  NABU_DisableInterrupts();  
}
