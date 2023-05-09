#define BIN_TYPE BIN_CPM

#define DISABLE_VDP 

#define DISABLE_KEYBOARD_INT

#include <stdio.h>
#include "../NABULIB/RetroNET-FileStore.h"
#include "../NABULIB/NABU-LIB.h"
#include <string.h>

#define BUFFERSIZE 128
uint8_t _buffer[BUFFERSIZE];

#define URL "http://cloud.nabu.ca/news.txt"

uint8_t wc = 0;

void putsPart(uint8_t offset, uint8_t len, uint8_t *buf) {

  for (uint8_t i = 0 ; i < len; i++) {

    uint8_t c = buf[offset + i];

    if (wc == 0 && c == ' ')
    ; // LOL!
    else
      putchar(c);

    if (c == '\n') {
      
      wc = 0;
    } else {

      wc++;

      if (wc == 39) {

        putchar('\n');
        wc = 0;
      }

      if (wc > 32 && c == ' ') {

        putchar('\n');
        wc = 0;
      }    
    }
  }
}

void main() {
  
  vt_clearScreen();
  
  vt_reverseVideo();
  printf("NABU RETRONET NEWS                      ");
  vt_normalVideo();
  puts("");

  uint8_t readFileHandle = rn_fileOpen(strlen(URL), URL, OPEN_FILE_FLAG_READONLY, 0xff);

  while (true) {

    uint16_t read = rn_fileHandleReadSeq(readFileHandle, _buffer, 0, BUFFERSIZE);

    if (read == 0)
      break;

    putsPart(0, read, _buffer);
  }

  rn_fileHandleClose(readFileHandle);
}
