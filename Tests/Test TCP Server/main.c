// ************************************************************************
// Skeleton Project
// DJ Sures (c)2023
//
// This is the progress of a game I'm making for the NABU computer.
// There are bugs, and I know about them. So you don't need to tell me :)
// I'm enjoying making it myself, so hope you understand keeping comments to yourself xD
// I shared the code so others can build their own games and use this as a starting point.
// If you make something, please let me know! We're a small NABU community and need to stick together.
//
// Have fun!
// - DJ
// ************************************************************************

#define BIN_TYPE BIN_CPM

#pragma output nofileio
#pragma output noprotectmsdos

#define DISABLE_VDP

#define DISABLE_CURSOR

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../NABULIB/NABU-LIB.h"
#include "../../NABULIB/RetroNET-FileStore.h"

#define IN_BUF_LEN 32
uint8_t _inBuf[IN_BUF_LEN] = { 0 };

#define OUT_BUF_LEN 1
uint8_t _outBuf[OUT_BUF_LEN] = { 0 };

void main() {

  initNABULib();  

  puts("");
  puts("This utility will test the RetroNET IA");
  puts("TCP Server. Ensure you have the server");
  puts("enabled in the IA settings menu.");
  puts("");
  puts("If enabled, you can telnet to the port");
  puts("and all incoming TCP data will be");
  puts("displayed here. All NABU keyboard data");
  puts("will be sent to all TCP clients");
  puts("");
  puts("- CTRL-C to exit");
  puts("- CTRL-D display client count");
  puts("");

  while (true) {

    if (isKeyPressed()) {

      uint8_t c = getChar();

      if (c == 0x03) {
      
        if (cpm_cursor.x != 0)
          puts("");

        puts("Exiting...");

        return;
      } else if (c == 0x04) {

        uint16_t num = rn_TCPServerClientCnt();

        if (cpm_cursor.x != 0)
          puts("");

        printf("%u clients connected\n", num);
      } else {

        _outBuf[0] = c;

        rn_TCPServerWrite(0, OUT_BUF_LEN, _outBuf);
      }
    }

    uint8_t read = rn_TCPServerRead(_inBuf, 0, IN_BUF_LEN);

    if (read > 0)
      for (uint8_t i = 0; i < read; i++)
        putchar(_inBuf[i]);
    
    for (uint16_t d = 0; d < 5000; d++)
    ;
  }
}
