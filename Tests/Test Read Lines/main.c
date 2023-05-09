// ************************************************************************
// Test line read
// DJ Sures (c)2023
//
// Have fun!
// - DJ
// ************************************************************************

#define BIN_TYPE BIN_CPM

#define DISABLE_KEYBOARD_INT
#define DISABLE_VDP

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../NABULIB/NABU-LIB.h"
#include "../../NABULIB/RetroNET-FileStore.h"

#define BUFFER_LEN 1024
uint8_t _buffer[BUFFER_LEN] = { 0 };

void main() {

  initNABULib();  

  uint8_t fileHandle = rn_fileOpen(7, "LST.TXT", OPEN_FILE_FLAG_READONLY, 0xff);

  printf("File handle: %u\n", fileHandle);

  uint16_t lineCnt = rn_fileHandleLineCount(fileHandle);

  printf("Line Count: %u\n", lineCnt);

  uint16_t bufLen = rn_fileHandleGetLine(fileHandle, 150, _buffer);

  printf("Buffer length: %u\n", bufLen);

  printf("Line: %s\n", _buffer);

  rn_fileHandleClose(fileHandle);
}
