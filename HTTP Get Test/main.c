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

//#define FONT_AMIGA
//#define FONT_SET1
//#define FONT_STANDARD
#define LM80C

#include "../NABULIB/RetroNET-FileStore.h"
#include "../NABULIB/NABU-LIB.h"
#include <z80.h> // for z80_delay_ms()


#define LOCAL_TMP_FILE "z:\\test\\directory\\testing.txt"
#define LOCAL_TMP_FILE_LEN 29

  // We use this for displaying file information and directory information later on
FileDetailsStruct fs;

#define BUFFERSIZE 4
uint8_t _buffer[BUFFERSIZE];


/// <summary>
/// Tests
/// - rn_fileOpen() local file readwrite
/// - rn_fileHandleInsert()
/// - rn_fileHandleReplace()
/// - rn_fileHandleClose()
/// </summary>
void doAlterLocalFile() {

  uint8_t writeFileHandle = rn_fileOpen(LOCAL_TMP_FILE_LEN, LOCAL_TMP_FILE, OPEN_FILE_FLAG_READWRITE, 0xff);

  vdp_print("Altering the local file...");
  vdp_newLine();
  vdp_newLine();

  // For fun, let's insert some text at the beginning of the file that we just creaed
  // ------------------------------------------------------------------------------------------------
  rn_fileHandleInsert(writeFileHandle, 0, 0, 18, "This is new text. ");

  // Oh, now let's replace some text in the file
  // ------------------------------------------------------------------------------------------------
  rn_fileHandleReplace(writeFileHandle, 20, 0, 12, "I MADE THIS!");

  rn_fileHandleClose(writeFileHandle);
}


/// <summary>
/// Tests
/// - rn_fileOpen() readonly local file
/// - rn_fileHandleSize()
/// - rn_fileHandleSeek()
/// - rn_fileHandleReadSeq()
/// - rn_fileHandleClose
/// </summary>
void doReadSequential() {

  uint8_t readFileHandle = rn_fileOpen(LOCAL_TMP_FILE_LEN, LOCAL_TMP_FILE, OPEN_FILE_FLAG_READONLY, 0xff);

  // Display the handle of the file we're going to write to
  vdp_print("read file handle: ");
  vdp_writeUInt8(readFileHandle);
  vdp_newLine();

  vdp_print("Displaying local file non-seq read...");
  vdp_newLine();
  vdp_newLine();

  // Get the size of the file that we created and wrote to
  uint32_t size = rn_fileHandleSize(readFileHandle);

  // Display the size of the file
  vdp_print("our file length: ");
  vdp_writeInt32(size);
  vdp_newLine();
 
  vdp_print("Skipping first 3 characters with seek");
  vdp_newLine();
  uint32_t newPos = rn_fileHandleSeek(readFileHandle, 3, SEEK_SET);

  vdp_print("read position is: ");
  vdp_writeInt32(newPos);
  vdp_newLine();

  while (true) {

    // read data from the http file that we downloaded in BUFFERSIZE (4 byte) parts
    uint16_t read = rn_fileHandleReadSeq(readFileHandle, _buffer, 0, BUFFERSIZE);

    // reached end of file so break out of loop
    if (read == 0)
      break;

    // display the data that we read onto the screen
    vdp_printPart(0, read, _buffer);

    z80_delay_ms(100);
  }

  rn_fileHandleClose(readFileHandle);

  vdp_newLine();
  vdp_newLine();
}


/// <summary>
/// Tests
/// - rn_fileOpen() with http file
/// - rn_fileHandleSize()
/// - rn_fileHandleEmptyFile()
/// - rn_fileHandleRead()
/// - rn_fileHandleAppend()
/// - rn_fileHandleClose()
/// </summary>
void doReadHTTPWriteLocal() {

  // Instruct the IA to get a file and let the server return a file handle id
  uint8_t httpFileHandle = rn_fileOpen(46, "https://cloud.nabu.ca/httpGetQueryResponse.txt", OPEN_FILE_FLAG_READONLY, 0xff);

  // Display the status if the file was received
  vdp_print("http file Handle: ");
  vdp_writeUInt8(httpFileHandle);
  vdp_newLine();

  // Get the size of the file that was received
  uint32_t size = rn_fileHandleSize(httpFileHandle);

  // Display the size of the file
  vdp_print("read response length: ");
  vdp_writeInt32(size);
  vdp_newLine();

  uint8_t writeFileHandle = rn_fileOpen(LOCAL_TMP_FILE_LEN, LOCAL_TMP_FILE, OPEN_FILE_FLAG_READWRITE, 0xff);

  // Display the handle of the file we're going to write to
  vdp_print("write file handle: ");
  vdp_writeUInt8(writeFileHandle);
  vdp_newLine();

  // If the file that we're going to write to has data in it, let's clear the data.
  // This is because we don't want data from the last time we ran it. We want a new
  // fresh empty file to populate.
  uint32_t testSize = rn_fileHandleSize(writeFileHandle);

  if (testSize > 0) {

    vdp_print("File already exists and is  ");
    vdp_writeInt32(testSize);
    vdp_print(" bytes");
    vdp_newLine();

    vdp_print("Emptying the file!");
    vdp_newLine();

    rn_fileHandleEmptyFile(writeFileHandle);
  }

  vdp_newLine();
  vdp_newLine();

  // Display the file contents of the http file with a non-sequential read
  // 
  // These next few lines will use the buffer to read from the IA each part of the file
  // 
  // We will also write the contents to a new file 
  //
  // As the file is requested by the NABU, the text is displayed 
  // on the screen. 
  // ------------------------------------------------------------------------------------------------

  vdp_print("Displaying from the http file...");
  vdp_newLine();
  vdp_newLine();

  uint32_t readPos = 0;
  while (true) {

    // read data from the http file that we downloaded in BUFFERSIZE (4 byte) parts
    uint16_t read = rn_fileHandleRead(httpFileHandle, _buffer, 0, readPos, BUFFERSIZE);

    // reached end of file so break out of loop
    if (read == 0)
      break;

    readPos += read;

    // write the data we read out to the file
    rn_fileHandleAppend(writeFileHandle, 0, read, _buffer);

    // display the data that we read onto the screen
    vdp_printPart(0, read, _buffer);

    z80_delay_ms(100);
  }

  rn_fileHandleClose(httpFileHandle);
  rn_fileHandleClose(writeFileHandle);

  vdp_newLine();
  vdp_newLine();
}


/// <summary>
/// Tests
/// - rn_fileOpen() local file read only
/// - rn_fileHandleDetails()
/// - rn_fileHandleClose()
/// </summary>
void doShowFileDetails() {

  // Display addition details about the file we just wrote to
  // ------------------------------------------------------------------------------------------------

  uint8_t readFileHandle = rn_fileOpen(LOCAL_TMP_FILE_LEN, LOCAL_TMP_FILE, OPEN_FILE_FLAG_READONLY, 0xff);

  rn_fileHandleDetails(readFileHandle, &fs);

  rn_fileHandleClose(readFileHandle);

  vdp_print("Filename: ");
  vdp_printPart(0, fs.FilenameLen, fs.Filename);
  vdp_newLine();

  vdp_print("File Size: ");
  vdp_writeInt32(fs.FileSize);
  vdp_newLine();

  vdp_print("Created: ");
  vdp_writeUInt16(fs.CreatedYear);
  vdp_write('-', true);
  vdp_writeUInt8(fs.CreatedMonth);
  vdp_write('-', true);
  vdp_writeUInt8(fs.CreatedDay);
  vdp_write(' ', true);
  vdp_writeUInt8(fs.CreatedHour);
  vdp_write(':', true);
  vdp_writeUInt8(fs.CreatedMinute);
  vdp_write(':', true);
  vdp_writeUInt8(fs.CreatedSecond);
  vdp_newLine();

  vdp_print("Modified: ");
  vdp_writeUInt16(fs.ModifiedYear);
  vdp_write('-', true);
  vdp_writeUInt8(fs.ModifiedMonth);
  vdp_write('-', true);
  vdp_writeUInt8(fs.ModifiedDay);
  vdp_write(' ', true);
  vdp_writeUInt8(fs.ModifiedHour);
  vdp_write(':', true);
  vdp_writeUInt8(fs.ModifiedMinute);
  vdp_write(':', true);
  vdp_writeUInt8(fs.ModifiedSecond);

  vdp_newLine();
  vdp_newLine();
}


/// <summary>
/// Tests 
/// - rn_fileList() include files & directories
/// - rn_fileListItem()
/// </summary>
void doDirectoryListing() {

  uint16_t fileCnt = rn_fileList(17, "z:\\test\\directory", 1, "*", FILE_LIST_FLAG_INCLUDE_FILES | FILE_LIST_FLAG_INCLUDE_DIRECTORIES);

  vdp_print("Files in z:\\test\\directory: ");
  vdp_writeInt32(fileCnt);
  vdp_newLine();

  for (uint16_t i = 0; i < fileCnt; i++) {

    vdp_newLine();
    vdp_newLine();

    rn_fileListItem(i, &fs);

    if (fs.IsFile) {

      vdp_print("Filename: ");
      vdp_printPart(0, fs.FilenameLen, fs.Filename);
      vdp_newLine();

      vdp_print("File Size: ");
      vdp_writeInt32(fs.FileSize);
      vdp_newLine();
    } else {

      vdp_print("Directory Name: ");
      vdp_printPart(0, fs.FilenameLen, fs.Filename);
      vdp_newLine();
    }

    vdp_print("Created: ");
    vdp_writeUInt16(fs.CreatedYear);
    vdp_write('-', true);
    vdp_writeUInt8(fs.CreatedMonth);
    vdp_write('-', true);
    vdp_writeUInt8(fs.CreatedDay);
    vdp_write(' ', true);
    vdp_writeUInt8(fs.CreatedHour);
    vdp_write(':', true);
    vdp_writeUInt8(fs.CreatedMinute);
    vdp_write(':', true);
    vdp_writeUInt8(fs.CreatedSecond);
    vdp_newLine();

    vdp_print("Modified: ");
    vdp_writeUInt16(fs.ModifiedYear);
    vdp_write('-', true);
    vdp_writeUInt8(fs.ModifiedMonth);
    vdp_write('-', true);
    vdp_writeUInt8(fs.ModifiedDay);
    vdp_write(' ', true);
    vdp_writeUInt8(fs.ModifiedHour);
    vdp_write(':', true);
    vdp_writeUInt8(fs.ModifiedMinute);
    vdp_write(':', true);
    vdp_writeUInt8(fs.ModifiedSecond);
    vdp_newLine();
  }

  vdp_newLine();
}

void main2() {

  initNABULib();

  // Put the graphics into text mode with the text color 0x01 and background color 0x03
  vdp_initTextMode(VDP_WHITE, VDP_BLACK, true);

  vdp_print("Hi, the goal of NABU is to be a PC with no local storage, relying on the network (or internet) for everything. This is a demo of reading/writing files from the cloud and over the Internet Adapter.");
  vdp_newLine();
  vdp_newLine();
  vdp_print("I have slowed down the read/write to make it easier to follow...");
  vdp_newLine();
  vdp_newLine();
  vdp_print("Press any key to begin");

  getChar();

  doReadHTTPWriteLocal();

  doAlterLocalFile();

  doReadSequential();

  doShowFileDetails();

  doDirectoryListing();

  vdp_print("Done");

  while (true);
}
