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

void main2() {

  // Put the graphics into text mode with the text color 0x01 and background color 0x03
  vdp_initTextMode(VDP_WHITE, VDP_BLACK);

  // Enable the receive buffer interrupt
  hcca_enableReceiveBufferInterrupt();

  // Instruct the IA to get a file and put it in request store #0
  bool ret = rn_requestStoreHttpGet(0, "https://cloud.nabu.ca/httpGetQueryResponse.txt");

  // Display the status if the file was received
  vdp_setCursor2(0, 0);
  vdp_writeUInt8(ret);

  // Get the size of the file that was received in request store #0
  uint16_t size = rn_requestStoreGetSize(0);

  // Display the size of the file
  vdp_setCursor2(0, 1);
  vdp_writeUInt16(size);


  // These next few lines will use a 4 byte buffer to read from
  // the IA each part of the file that is stored in request store #0.
  // We're using a 4 byte buffer just to demonstrate how to obtain
  // parts of the file.
  //
  // As the file is requested by the NABU, the text is displayed 
  // on the screen.

  vdp_setCursor2(0, 2);
#define bufferSize 4
  uint8_t buffer[bufferSize];
  uint16_t read = 0;

  while (read < size) {

    uint8_t toRead = Min(bufferSize, size - read);

    read += rn_requestStoreGetData(0, buffer, 0, read, toRead);

    vdp_printPart(buffer, 0, toRead);
  }

  // We're done displaying the data, so let's display DONE and hang out
  // until someone preses physical reset on the NABU.
  vdp_newLine();
  vdp_newLine();

  vdp_print("Done");

  while (true) {

  }
}
