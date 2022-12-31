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

#define BUFFERSIZE 4
  uint8_t buffer[BUFFERSIZE];
  uint32_t read;

  // Put the graphics into text mode with the text color 0x01 and background color 0x03
  vdp_initTextMode(VDP_WHITE, VDP_BLACK, true);

  // Enable the receive buffer interrupt
  hcca_enableReceiveBufferInterrupt();

  // Instruct the IA to get a file and let the server return a file handle id
  uint8_t fileHandle = rn_fileOpen(46, "https://cloud.nabu.ca/httpGetQueryResponse.txt", OPEN_FILE_FLAG_READONLY, 0xff);

  // Display the status if the file was received
  vdp_print("read file Handle: ");
  vdp_writeUInt8(fileHandle);
  vdp_newLine();

  // Get the size of the file that was received in request store #0
  uint32_t size = rn_fileHandleSize(fileHandle);

  // Display the size of the file
  vdp_print("read response length: ");
  vdp_writeInt32(size);
  vdp_newLine();

  uint8_t writeFileHandle = rn_fileOpen(29, "z:\\test\\directory\\testing.txt", OPEN_FILE_FLAG_READWRITE, 0xff);

  // Display the handle of the file we're going to write to
  vdp_print("write file handle: ");
  vdp_writeUInt8(writeFileHandle);
  vdp_newLine();

  // If the file that we're going to write to has data in it, let's clear the data.
  // This is because we don't want data from the last time we ran it. We want a new
  // fresh empty file to populate.
  uint32_t testSize = rn_fileHandleSize(writeFileHandle);
  if (testSize > 0) {

    vdp_print("Our test file size is: ");
    vdp_writeInt32(testSize);
    vdp_newLine();

    vdp_print("Emptying the file");
    vdp_newLine();

    rn_fileHandleEmptyFile(writeFileHandle);
  }

  vdp_newLine();
  vdp_newLine();

  // Display the file contents of the http file
  // 
  // These next few lines will use a 4 byte buffer to read from
  // the IA each part of the file that is stored in request store #0.
  // We're using a 4 byte buffer just to demonstrate how to obtain
  // parts of the file.
  // 
  // We will also write the contents to a new file that we will
  // then later read from.
  //
  // As the file is requested by the NABU, the text is displayed 
  // on the screen. 
  // ------------------------------------------------------------------------------------------------

  vdp_print("Displaying from the http file...");
  vdp_newLine();

  read = 0;
  while (read < size) {

    // Make sure we read either the maximum size of the buffer, or what's left to read
    uint8_t toRead = Min(BUFFERSIZE, size - read);

    // read data from the http file that we downloaded in BUFFERSIZE (4 byte) parts
    read += rn_fileHandleRead(fileHandle, buffer, 0, read, toRead);

    // write the data we read out to the file
    rn_fileHandleAppend(writeFileHandle, 0, toRead, buffer);

    // display the data that we read onto the screen
    vdp_printPart(buffer, 0, toRead);
  }

  vdp_newLine();
  vdp_newLine();


  // For fun, let's insert some text at the beginning of the file that we just creaed
  // ------------------------------------------------------------------------------------------------
  rn_fileHandleInsert(writeFileHandle, 0, 0, 18, "This is new text. ");

  // Oh, now let's replace some text in the file
  // ------------------------------------------------------------------------------------------------
  rn_fileHandleReplace(writeFileHandle, 20, 0, 12, "I MADE THIS!");


  // Now let's read from our file that we just created and display that on the screen
  // ------------------------------------------------------------------------------------------------

  vdp_print("Displaying from the local file...");
  vdp_newLine();

  // Get the size of the file that we created and wrote to
  size = rn_fileHandleSize(writeFileHandle);

  // Display the size of the file
  vdp_print("our file length: ");
  vdp_writeInt32(size);
  vdp_newLine();

  read = 0;
  while (read < size) {

    // Make sure we read either the maximum size of the buffer, or what's left to read
    uint8_t toRead = Min(BUFFERSIZE, size - read);

    // read data from the http file that we downloaded in BUFFERSIZE (4 byte) parts
    read += rn_fileHandleRead(writeFileHandle, buffer, 0, read, toRead);

    // display the data that we read onto the screen
    vdp_printPart(buffer, 0, toRead);
  }

  vdp_newLine();
  vdp_newLine();

  // We're done withour test, so let's display DONE and hang out
  // until someone preses physical reset on the NABU.

  vdp_print("Done");

  while (true) {

  }
}
