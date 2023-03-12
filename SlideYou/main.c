#define BIN_TYPE BIN_CPM

#define DISABLE_CURSOR

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <strings.h>
#include <stdio.h>
#include "../NABULIB/NABU-LIB.h"
#include "../NABULIB/RetroNET-FileStore.h"
#include <arch/z80.h>

FileDetailsStruct _fs = { 0 };

uint16_t _fileCnt = 0;

#define PIC_BUF 128
uint8_t buf[PIC_BUF] = { 0 };

#define TMP_FILENAME_LEN 255
uint8_t FILENAME[TMP_FILENAME_LEN] = { 0 };

void loadImage(uint8_t *filename) {

  sprintf(FILENAME, "\\SLIDESHOW\\%s", filename);

  uint8_t fileId = rn_fileOpen(strlen(FILENAME), FILENAME, OPEN_FILE_FLAG_READONLY, 0xff);

  rn_fileHandleSeek(fileId, 7, RN_SEEK_SET);

  vdp_setWriteAddress(0x0);

  while (true) {

    uint8_t read = rn_fileHandleReadSeq(fileId, buf, 0, PIC_BUF);

    if (read == 0)
      break;

    for (uint8_t i = 0; i < read; i++)
      IO_VDPDATA = buf[i];
  }

  rn_fileHandleClose(fileId);
}

void slideShow() {

  vdp_initMSXMode(0);

  while (true) {

    uint16_t file = rand() % _fileCnt;
    
    rn_fileListItem(file, &_fs);

    loadImage(_fs.Filename);

    for (uint8_t i = 0; i < 250; i++) {

      z80_delay_ms(50);

      if (isKeyPressed()) {
        
        uint8_t p = getChar();

        if (p == ' ')
          return;
        else if (p == 'n')
          break;        
      }
    }
  }
}

void main() {

  initNABULib();

  puts("");
  vt_reverseVideo();
  puts("Slide You v0.1         (c)2023 DJ Sures"); 
  vt_normalVideo();
  puts(" - View SC2 files in IA SLIDESHOW folder"); 
  puts(" - Images display for 30 seconds"); 
  puts(" - SPACEBAR exits during slide show");
  puts(" - N skips to next image\n");  
  puts("Initializing...\n");

  _fileCnt =  rn_fileList(10, "\\slideshow", 5, "*.sc2", FILE_LIST_FLAG_INCLUDE_FILES);

  if (_fileCnt > 0) {

    printf("Found %u images... starting\n", _fileCnt);

    z80_delay_ms(5000);

    slideShow();
  } else {

    puts("No images were found!\n");
    puts("View DJ's 68 NABU video for instructions");
    puts("The video can be found on my YouTube or");
    puts("the SOFTWARE section of https://nabu.ca");
    puts("");
  }  
}
