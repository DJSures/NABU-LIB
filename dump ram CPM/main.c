void main();

#define BIN_TYPE BIN_CPM
#define DISABLE_KEYBOARD_INT
#define DISABLE_VDP

#include <stdio.h>
#include "../NABULIB/RetroNET-FileStore.h"
#include "../NABULIB/NABU-LIB.h"

#define REMOTE_FILENAME_LEN 11
#define REMOTE_FILENAME "DUMPRAM.BIN"

void main() {

  initNABULib();

  puts("");
  puts("DumpRam by DJ Sures");
  puts("-------------------");
  puts("");
  puts("This will copy all NABU RAM to the");
  puts("Internet Adapter Store folder.");
  puts("The process takes appro 1 minute.");
  puts("");
  puts("Would you like to continue? (y/n)");

  while (true) {

    uint8_t c = getchar();

    if (c == 'y' || c == 'Y') {

      break;
    } else if (c == 'n' || c == 'N') {
   
      puts("Returning to CPM...");

      return;
    }
  }

  printf("- Opening remote file: %s\n", REMOTE_FILENAME);

  uint8_t writeFileHandle = rn_fileOpen(REMOTE_FILENAME_LEN, REMOTE_FILENAME, OPEN_FILE_FLAG_READWRITE, 0xff);

  printf("- File handle: %u\n", writeFileHandle);

  puts("- Emptying file");

  rn_fileHandleEmptyFile(writeFileHandle);

  uint16_t i;
  #define PSIZE 2048
  for (i = 0; i < 0xffff - PSIZE; i+=PSIZE) {

      printf("- Writing %u to %u\n", i, i + PSIZE);

      rn_fileHandleAppend(writeFileHandle, 0, PSIZE, (void *)i);
  }

  if (0xffff - i > 0) {

    printf("- Writing %u to %u\n", i, 0xffff);
    rn_fileHandleAppend(writeFileHandle, 0, 0xffff - i, (void *)i);
  }

  rn_fileHandleClose(writeFileHandle);

  puts("Done\n");
}
