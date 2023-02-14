#pragma output nofileio
#pragma output noprotectmsdos
#pragma output noredir
#pragma output nogfxglobals

#define BIN_TYPE BIN_CPM

#define DISABLE_KEYBOARD_INT

#define DISABLE_VDP

#define RX_BUFFER_SIZE 300

// https://www.seasip.info/Cpm/bdos.html
// https://github.com/z88dk/z88dk/blob/master/include/cpm.h
#include <cpm.h>
#include "../NABULIB/NABU-LIB.h"
#include "../NABULIB/RetroNET-TCPClient.h"
#include <string.h>

bool    _isRunning = true;
uint8_t _handle    = 0xff;

#define IN_BUF_LEN 255
uint8_t inBuf[IN_BUF_LEN] = { 0 };

void doKeyInput() {

  uint8_t key = bios(3, 0, 0);

  switch (key) {
    case 0x1d: // ^]
    case 0xea: // tv<>nabu
    
      puts("");
      puts("Exiting...");
      puts("");

      _isRunning = false;
      break;
    case 127:
      
      inBuf[0] = 0x08;
      rn_TCPHandleWrite(_handle, 0, 1, inBuf);
      break;
    case 0xe1:

      // left
      inBuf[0] = 27;
      inBuf[1] = 'D';
      rn_TCPHandleWrite(_handle, 0, 2, inBuf);
      break;
    case 0xe0:

      // right
      inBuf[0] = 27;
      inBuf[1] = 'C';
      rn_TCPHandleWrite(_handle, 0, 2, inBuf);
      break;
    case 0xe2:
  
      // up
      inBuf[0] = 27;
      inBuf[1] = 'A';
      rn_TCPHandleWrite(_handle, 0, 2, inBuf);
      break;
    case 0xe3:
  
      // down
      inBuf[0] = 27;
      inBuf[1] = 'B';
      rn_TCPHandleWrite(_handle, 0, 2, inBuf);
      break;
    default:

      if (key < 0xe0) {

        inBuf[0] = key;
        rn_TCPHandleWrite(_handle, 0, 1, inBuf);
      }
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

  inBuf[0] = 0xff;
  inBuf[1] = optionCode;
  inBuf[2] = option;
  rn_TCPHandleWrite(_handle, 0, 3, inBuf);
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
#define MSG_32_LEN 15
const uint8_t msg_32[] = { 255, 250, 32, 0, '2', '4', '0', '0', ',', '2', '4', '0', '0', 255, 240 };
void telnet32() {

  sendOption(OPT_WILL, 32);

  rn_TCPHandleWrite(_handle, 0, MSG_32_LEN, msg_32);
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
#define MSG_31_LEN 9
const uint8_t msg_31[] = { 255, 250, 31, 0, 80, 0, 24, 255, 240 };
void telnet31() {

  sendOption(OPT_WILL, 31);

  rn_TCPHandleWrite(_handle, 0, MSG_31_LEN, msg_31);
}

// Terminal Type
#define MSG_24_LEN 10
const uint8_t msg_24[] = { 255, 250, 24, 0, 'V', 'T', '5', '2', 255, 240 };
void telnet24() {

  sendOption(OPT_WILL, 24);

  rn_TCPHandleWrite(_handle, 0, MSG_24_LEN, msg_24);

  // no extended ascii
  sendOption(OPT_DONT, 17);
}

void doHCCAInput() {

  int32_t read = rn_TCPHandleRead(_handle, inBuf, 0, IN_BUF_LEN);

  if (read == 0)
    return;

  if (read == -1) {

    _isRunning = false;

    puts("");
    puts("Disconnected");
    
    return;
  }
  
  uint8_t *end = inBuf + read;

  for (uint8_t *start = inBuf; start != end; start++) {
      
    if (*start == 0xff && _telnetEscapePos == 0) {

      // Telnet Escape started

      _telnetEscapeOptions[0] = *start;

      _telnetEscapePos = 1;

      continue;
    }

    if (_telnetEscapePos > 0) {

      // There's 3 telnet escape values. So let's add them to the option buffer

      _telnetEscapeOptions[_telnetEscapePos] = *start;

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

      continue;
    }

    bios(4, *start, 0);
  }
}

void main(int argc, char *argv[]) {

  if (argc <= 1) {

    puts("");
    puts("RetroNet Telnet Client CP/M (2.3b)");
    puts("by DJ Sures (c)2023");
    puts("");
    puts("Usage: telnet <hostname> <port optional>");
    puts("");
    puts("Examples:");
    puts("         telnet 10.0.0.17");
    puts("         telnet 10.0.0.17 23");
    puts("");

    return;
  }

  uint16_t port = 23;

  printf("\nConnecting to %s %u\n", argv[1], port);
  
  initNABULib();

  _handle = rn_TCPOpen(strlen(argv[1]), argv[1], port, 0xff);

  if (_handle == 0xff) {

    puts("Unable to establish a connection");
    puts("");

    return;
  }

  puts("Connected. Use CTRL+] to exit");
  puts("");

  while (_isRunning) {

    doHCCAInput();

    while (bios(2, 0, 0))
      doKeyInput();
  }

  rn_TCPHandleClose(_handle);
}
 
