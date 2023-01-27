#define BIN_TYPE BIN_CPM

// use cpm bdos
#define DISABLE_KEYBOARD_INT

// use cpm bdos (i.e. printf, etc)
#define DISABLE_VDP

#include "../NABULIB/RetroNET-FileStore.h"
#include "../NABULIB/NABU-LIB.h"
#include <arch/z80.h> // for z80_delay_ms()

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

  puts("Altering the local file...");
  puts("");
  puts("");

  // For fun, let's insert some text at the beginning of the file that we just creaed
  // ------------------------------------------------------------------------------------------------
  rn_fileHandleInsert(writeFileHandle, 0, 0, 18, "This is new text. ");

  // Oh, now let's replace some text in the file
  // ------------------------------------------------------------------------------------------------
  rn_fileHandleReplace(writeFileHandle, 20, 0, 12, "I MADE THIS!");

  rn_fileHandleClose(writeFileHandle);
}

void putsPart(uint8_t offset, uint8_t len, uint8_t *buf) {

  for (uint8_t i = 0 ; i < len; i++) 
    printf("%c", buf[offset + i]);
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
  printf("read file handle: %u", readFileHandle);
  puts("");

  puts("Displaying local file non-seq read...");
  puts("");
  puts("");

  // Get the size of the file that we created and wrote to
  uint32_t size = rn_fileHandleSize(readFileHandle);

  // Display the size of the file
  printf("our file length: %u", size);
  puts("");
 
  puts("Skipping first 3 characters with seek");
  puts("");
  uint32_t newPos = rn_fileHandleSeek(readFileHandle, 3, RN_SEEK_SET);

  printf("read position is: %u", newPos);
  puts("");

  while (true) {

    // read data from the http file that we downloaded in BUFFERSIZE (4 byte) parts
    uint16_t read = rn_fileHandleReadSeq(readFileHandle, _buffer, 0, BUFFERSIZE);

    // reached end of file so break out of loop
    if (read == 0)
      break;

    // display the data that we read onto the screen
    putsPart(0, read, _buffer);

    z80_delay_ms(100);
  }

  rn_fileHandleClose(readFileHandle);

  puts("");
  puts("");
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
  printf("http file Handle: %u", httpFileHandle);
  puts("");

  // Get the size of the file that was received
  uint32_t size = rn_fileHandleSize(httpFileHandle);

  // Display the size of the file
  printf("read response length: %u", size);
  puts("");

  uint8_t writeFileHandle = rn_fileOpen(LOCAL_TMP_FILE_LEN, LOCAL_TMP_FILE, OPEN_FILE_FLAG_READWRITE, 0xff);

  // Display the handle of the file we're going to write to
  printf("write file handle: %u", writeFileHandle);
  puts("");

  // If the file that we're going to write to has data in it, let's clear the data.
  // This is because we don't want data from the last time we ran it. We want a new
  // fresh empty file to populate.
  uint32_t testSize = rn_fileHandleSize(writeFileHandle);

  if (testSize > 0) {

    printf("File already exists and is %u", testSize);
    puts(" bytes");
    puts("");

    puts("Emptying the file!");
    puts("");

    rn_fileHandleEmptyFile(writeFileHandle);
  }

  puts("");
  puts("");

  // Display the file contents of the http file with a non-sequential read
  // 
  // These next few lines will use the buffer to read from the IA each part of the file
  // 
  // We will also write the contents to a new file 
  //
  // As the file is requested by the NABU, the text is displayed 
  // on the screen. 
  // ------------------------------------------------------------------------------------------------

  puts("Displaying from the http file...");
  puts("");
  puts("");

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
    putsPart(0, read, _buffer);

    z80_delay_ms(100);
  }

  rn_fileHandleClose(httpFileHandle);
  rn_fileHandleClose(writeFileHandle);

  puts("");
  puts("");
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

  puts("Filename: ");
  putsPart(0, fs.FilenameLen, fs.Filename);
  puts("");

  printf("File Size: %u" , fs.FileSize);
  puts("");

  printf("Created: %u-%u-%u %u:%u:%u", fs.CreatedYear, fs.CreatedMonth, fs.CreatedDay, fs.CreatedHour, fs.CreatedMinute, fs.CreatedSecond);
  puts("");

  printf("Modified: %u-%u-%u %u:%u:%u", fs.ModifiedYear, fs.ModifiedMonth, fs.ModifiedDay, fs.ModifiedHour, fs.ModifiedMinute, fs.ModifiedSecond);
  puts("");

  puts("");
}


/// <summary>
/// Tests 
/// - rn_fileList() include files & directories
/// - rn_fileListItem()
/// </summary>
void doDirectoryListing() {

  uint16_t fileCnt = rn_fileList(17, "z:\\test\\directory", 1, "*", FILE_LIST_FLAG_INCLUDE_FILES | FILE_LIST_FLAG_INCLUDE_DIRECTORIES);

  printf("Files in z:\\test\\directory: %u", fileCnt);
  puts("");

  for (uint16_t i = 0; i < fileCnt; i++) {

    puts("");
    puts("");

    rn_fileListItem(i, &fs);

    if (fs.IsFile) {

      puts("Filename: ");
      putsPart(0, fs.FilenameLen, fs.Filename);
      puts("");

      printf("File Size: %u", fs.FileSize);
      puts("");
    } else {

      puts("Directory Name: ");
      putsPart(0, fs.FilenameLen, fs.Filename);
      puts("");
    }

    printf("Created: %u-%u-%u %u:%u:%u", fs.CreatedYear, fs.CreatedMonth, fs.CreatedDay, fs.CreatedHour, fs.CreatedMinute, fs.CreatedSecond);
    puts("");

    printf("Modified: %u-%u-%u %u:%u:%u", fs.ModifiedYear, fs.ModifiedMonth, fs.ModifiedDay, fs.ModifiedHour, fs.ModifiedMinute, fs.ModifiedSecond);
    puts("");
  }

  puts("");
}

void prompt() {

  vt_setCursor(8, 23);
  printf("Press SPACE for next test");

  while (getchar() != ' ');
}

void main() {

  initNABULib();

  puts("Hi, the goal of NABU is to be a PC with no local storage, relying on the network (or internet) for everything. This is a demo of reading/writing files from the cloud and over the Internet Adapter.");
  puts("");
  puts("I have slowed down the read/write to make it easier to follow...");
  puts("");

  prompt();
  vt_clearScreen();

  doReadHTTPWriteLocal();

  prompt();
  vt_clearScreen();

  doAlterLocalFile();

  prompt();
  vt_clearScreen();

  doReadSequential();

  prompt();
  vt_clearScreen();

  doShowFileDetails();

  prompt();
  vt_clearScreen();

  doDirectoryListing();

  puts("Done");
}
