// ****************************************************************************************
// NABU-LIB C Library
// DJ Sures (c) 2023
// https://nabu.ca
// 
// Read the NABU-LIB.h file for details of each function in this file.
// 
// *Note: Some TMS9918 graphic functions were from: https://github.com/michalin/TMS9918_Arduino/tree/main/examples
//        Modified by DJ Sures 2022 for NABU PC
//
// **********************************************************************************************



uint8_t _rnFS_INT_BACKUP = 0;


// **************************************************************************
// INTERRUPTLESS HCCA COMMANDS
// ---------------------------
// These are HCCA commands that do not require the interrupt. The interrupt
// will be disabled while these commands are executed. This is because they
// will focus entirely on hcca data transfer for optimal speed. Interrupts
// use too many cycles for large amounts of data.
// **************************************************************************
void hcca_DiFocusInterrupts() {

  NABU_DisableInterrupts();

  _rnFS_INT_BACKUP = ayRead(IOPORTA);

  ayWrite(IOPORTA, INT_MASK_HCCARX);
}

void hcca_DiRestoreInterrupts() {

  ayWrite(IOPORTA, _rnFS_INT_BACKUP);

  NABU_EnableInterrupts();
}

void hcca_DiWriteByte(uint8_t c) {
  
  ayWrite(IOPORTA, INT_MASK_HCCATX);

  IO_AYLATCH = IOPORTB;
  while (IO_AYDATA & 0x04);

  IO_HCCA = c;

  ayWrite(IOPORTA, INT_MASK_HCCARX);
}

void hcca_DiWriteUInt32(uint32_t val) {

  hcca_DiWriteByte(val & 0xff);
  hcca_DiWriteByte((val >> 8) & 0xff);
  hcca_DiWriteByte((val >> 16) & 0xff);
  hcca_DiWriteByte((val >> 24) & 0xff);
}

void hcca_DiWriteInt32(int32_t val) {

  hcca_DiWriteByte(val & 0xff);
  hcca_DiWriteByte((val >> 8) & 0xff);
  hcca_DiWriteByte((val >> 16) & 0xff);
  hcca_DiWriteByte((val >> 24) & 0xff);
}

void hcca_DiWriteUInt16(uint16_t val) {

  hcca_DiWriteByte(val & 0xff);
  hcca_DiWriteByte((val >> 8) & 0xff);
}

void hcca_DiWriteInt16(int16_t val) {

  hcca_DiWriteByte(val & 0xff);
  hcca_DiWriteByte((val >> 8) & 0xff);
}

void hcca_DiWriteString(uint8_t *str) {
  
  for (uint8_t *start = str; *start != 0x00; start++)
    hcca_DiWriteByte(*start);
}

void hcca_DiWriteBytes(uint16_t offset, uint16_t length, uint8_t *bytes) {

  uint8_t *start = bytes + offset;
  uint8_t *end   = start + length;

  while (start != end) {

    hcca_DiWriteByte(*start);

    start++;
  }
}

// -----------------------------------------------------------------------------------

inline uint8_t hcca_DiReadByte() {

  IO_AYLATCH = IOPORTB;
  while (IO_AYDATA & 0x02);
  return IO_HCCA;
}

uint16_t hcca_DiReadUInt16() {

  return  (uint16_t)hcca_DiReadByte() |
         ((uint16_t)hcca_DiReadByte() << 8);
}

int16_t hcca_DiReadInt16() {

  return  (int16_t)hcca_DiReadByte() |
         ((int16_t)hcca_DiReadByte() << 8);
}

uint32_t hcca_DiReadUInt32() {

  uint8_t ret[4] = { hcca_DiReadByte(), hcca_DiReadByte(), hcca_DiReadByte(), hcca_DiReadByte() };

  return *((uint32_t *)ret);
}

int32_t hcca_DiReadInt32() {

  uint8_t ret[4] = { hcca_DiReadByte(), hcca_DiReadByte(), hcca_DiReadByte(), hcca_DiReadByte() };

  return *((int32_t *)ret);
}

void hcca_DiReadBytes(uint16_t offset, uint16_t bufferLen, uint8_t *buffer) {

  uint8_t *start = buffer + offset;
  uint8_t *end   = start + bufferLen;

  IO_AYLATCH = IOPORTB;
  while (start != end) {

    while (IO_AYDATA & 0x02);
    *start = IO_HCCA;

    start++;
  }
}

// **************************************************************************
// FILE & DIRECTORIES
// ------------------
//
// These functions are for reading and creating files and directories.
// **************************************************************************

uint8_t rn_fileOpen(uint8_t filenameLen, uint8_t* filename, uint16_t fileFlag, uint8_t fileHandle) {

  //0xa3

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xa3);

  hcca_DiWriteByte(filenameLen);

  hcca_DiWriteBytes(0, filenameLen, filename);

  hcca_DiWriteUInt16(fileFlag);

  hcca_DiWriteByte(fileHandle);

  uint8_t t = hcca_DiReadByte();

  hcca_DiRestoreInterrupts();

  return t;
}

void rn_fileHandleClose(uint8_t fileHandle) {

  //0xa7

  hcca_writeByte(0xa7);

  hcca_writeByte(fileHandle);
}

int32_t rn_fileSize(uint8_t filenameLen, uint8_t* filename) {

  //0xa8

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xa8);

  hcca_DiWriteByte(filenameLen);

  hcca_DiWriteBytes(0, filenameLen, filename);

  int32_t t = hcca_DiReadInt32();

  hcca_DiRestoreInterrupts();

  return t;
}

int32_t rn_fileHandleSize(uint8_t fileHandle) {

  //0xa4

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xa4);

  hcca_DiWriteByte(fileHandle);

  int32_t t = hcca_DiReadInt32();

  hcca_DiRestoreInterrupts();

  return t;
}

uint16_t rn_fileHandleRead(uint8_t fileHandle, uint8_t* buffer, uint16_t bufferOffset, uint32_t readOffset, uint16_t readLength) {

  //0xa5

  hcca_DiFocusInterrupts();

  uint8_t *start = buffer + bufferOffset;

  hcca_DiWriteByte(0xa5);

  hcca_DiWriteByte(fileHandle);

  hcca_DiWriteUInt32(readOffset);

  hcca_DiWriteUInt16(readLength);

  uint16_t toRead = hcca_DiReadUInt16();
  uint8_t *end    = start + toRead;
  
  while (start != end) {

    while (IO_AYDATA & 0x02);
    *start = IO_HCCA;

    start++;
  }

  hcca_DiRestoreInterrupts();

  return toRead;
}

void rn_fileHandleAppend(uint8_t fileHandle, uint16_t dataOffset, uint16_t dataLen, uint8_t* data) {

  //0xa9

  hcca_writeByte(0xa9);

  hcca_writeByte(fileHandle);

  hcca_writeUInt16(dataLen);

  hcca_writeBytes(dataOffset, dataLen, data);
}

void rn_fileHandleInsert(uint8_t fileHandle, uint32_t fileOffset, uint16_t dataOffset, uint16_t dataLen, uint8_t* data) {

  //0xaa

  hcca_writeByte(0xaa);

  hcca_writeByte(fileHandle);

  hcca_writeUInt32(fileOffset);

  hcca_writeUInt16(dataLen);

  hcca_writeBytes(dataOffset, dataLen, data);
}

void rn_fileHandleDeleteRange(uint8_t fileHandle, uint32_t fileOffset, uint16_t deleteLen) {

  // 0xab

  hcca_writeByte(0xab);

  hcca_writeByte(fileHandle);

  hcca_writeUInt32(fileOffset);

  hcca_writeUInt16(deleteLen);
}

void rn_fileHandleEmptyFile(uint8_t fileHandle) {

  // 0xb0

  hcca_writeByte(0xb0);

  hcca_writeByte(fileHandle);
}

void rn_fileHandleReplace(uint8_t fileHandle, uint32_t fileOffset, uint16_t dataOffset, uint16_t dataLen, uint8_t* data) {

  // 0xac

  hcca_writeByte(0xac);

  hcca_writeByte(fileHandle);

  hcca_writeUInt32(fileOffset);

  hcca_writeUInt16(dataLen);

  hcca_writeBytes(dataOffset, dataLen, data);
}

void rn_fileDelete(uint8_t filenameLen, uint8_t* filename) {

  // 0xad

  hcca_writeByte(0xad);

  hcca_writeByte(filenameLen);

  hcca_writeBytes(0, filenameLen, filename);
}

void rn_fileHandleCopy(uint8_t srcFilenameLen, uint8_t* srcFilename, uint8_t destFilenameLen, uint8_t* destFilename, uint8_t copyMoveFlag) {

  // 0xae

  hcca_writeByte(0xae);

  hcca_writeByte(srcFilenameLen);

  hcca_writeBytes(0, srcFilenameLen, srcFilename);

  hcca_writeByte(destFilenameLen);

  hcca_writeBytes(0, destFilenameLen, destFilename);

  hcca_writeByte(copyMoveFlag);
}

void rn_fileHandleMove(uint8_t srcFilenameLen, uint8_t* srcFilename, uint8_t destFilenameLen, uint8_t* destFilename, uint8_t copyMoveFlag) {

  // 0xaf

  hcca_writeByte(0xaf);

  hcca_writeByte(srcFilenameLen);

  hcca_writeBytes(0, srcFilenameLen, srcFilename);

  hcca_writeByte(destFilenameLen);

  hcca_writeBytes(0, destFilenameLen, destFilename);

  hcca_writeByte(copyMoveFlag);
}

uint16_t rn_fileList(uint8_t pathLen, uint8_t* path, uint8_t wildcardLen, uint8_t* wildcard, uint8_t fileListFlags) {

  // 0xb1

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xb1);

  hcca_DiWriteByte(pathLen);

  hcca_DiWriteBytes(0, pathLen, path);

  hcca_DiWriteByte(wildcardLen);

  hcca_DiWriteBytes(0, wildcardLen, wildcard);

  hcca_DiWriteByte(fileListFlags);

  uint16_t t = hcca_DiReadUInt16();

  hcca_DiRestoreInterrupts();

  return t;
}

void rn_fileListItem(uint16_t fileItemIndex, FileDetailsStruct* s) {

  // 0xb2
  // 
  // The response is 83 bytes and structured like so...
  // 
  // Bytes       Type      Description
  // ----------  --------  ------------------------------------
  // 0, 1, 2, 3  int32_t   Filesize (or -1 for a folder)
  // 4, 5        uint16_t  Created Year
  // 6           uint8_t   Created Month
  // 7           uint8_t   Created Day
  // 8           uint8_t   Created Hour (24 hour)
  // 9           uint8_t   Created Minute
  // 10          uint8_t   Created Second
  // 11, 12      uint16_t  Modified Year
  // 13          uint8_t   Modified Month
  // 14          uint8_t   Modified Day
  // 15          uint8_t   Modified Hour (24 hour)
  // 16          uint8_t   Modified Minute
  // 17          uint8_t   Modified Second
  // 18          uint8_t   Length of filename (max 64)
  // 19..82                The remaining bytes is the filename

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xb2);

  hcca_DiWriteUInt16(fileItemIndex);

  s->FileSize = hcca_DiReadInt32(); // 0, 1, 2, 3

  s->CreatedYear = hcca_DiReadUInt16(); // 4, 5
  s->CreatedMonth = hcca_DiReadByte(); // 6
  s->CreatedDay = hcca_DiReadByte(); // 7
  s->CreatedHour = hcca_DiReadByte(); // 8
  s->CreatedMinute = hcca_DiReadByte(); // 9
  s->CreatedSecond = hcca_DiReadByte(); // 10

  s->ModifiedYear = hcca_DiReadUInt16(); // 11, 12
  s->ModifiedMonth = hcca_DiReadByte(); // 13
  s->ModifiedDay = hcca_DiReadByte(); // 14
  s->ModifiedHour = hcca_DiReadByte(); // 15
  s->ModifiedMinute = hcca_DiReadByte(); // 16
  s->ModifiedSecond = hcca_DiReadByte(); // 17

  s->FilenameLen = hcca_DiReadByte(); // 18

  hcca_DiReadBytes(0, 64, (uint8_t*)s->Filename); // 19-64

  s->IsFile = (s->FileSize >= 0);
  s->Exists = (s->FileSize != -2);

  hcca_DiRestoreInterrupts();
}

void rn_fileDetails(uint8_t filenameLen, uint8_t* filename, FileDetailsStruct* s) {

  // 0xb3

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xb3);

  hcca_DiWriteByte(filenameLen);

  hcca_DiWriteBytes(0, filenameLen, filename);

  s->FileSize = hcca_DiReadInt32(); // 0, 1, 2, 3

  s->CreatedYear = hcca_DiReadUInt16(); // 4, 5
  s->CreatedMonth = hcca_DiReadByte(); // 6
  s->CreatedDay = hcca_DiReadByte(); // 7
  s->CreatedHour = hcca_DiReadByte(); // 8
  s->CreatedMinute = hcca_DiReadByte(); // 9
  s->CreatedSecond = hcca_DiReadByte(); // 10

  s->ModifiedYear = hcca_DiReadUInt16(); // 11, 12
  s->ModifiedMonth = hcca_DiReadByte(); // 13
  s->ModifiedDay = hcca_DiReadByte(); // 14
  s->ModifiedHour = hcca_DiReadByte(); // 15
  s->ModifiedMinute = hcca_DiReadByte(); // 16
  s->ModifiedSecond = hcca_DiReadByte(); // 17

  s->FilenameLen = hcca_DiReadByte(); // 18

  hcca_DiReadBytes(0, 64, (uint8_t*)s->Filename); // 19-64

  s->IsFile = (s->FileSize >= 0);
  s->Exists = (s->FileSize != -2);

  hcca_DiRestoreInterrupts();
}

void rn_fileHandleDetails(int8_t fileHandle, FileDetailsStruct* s) {

  // 0xb4

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xb4);

  hcca_DiWriteByte(fileHandle);

  s->FileSize = hcca_DiReadInt32(); // 0, 1, 2, 3

  s->CreatedYear = hcca_DiReadUInt16(); // 4, 5
  s->CreatedMonth = hcca_DiReadByte(); // 6
  s->CreatedDay = hcca_DiReadByte(); // 7
  s->CreatedHour = hcca_DiReadByte(); // 8
  s->CreatedMinute = hcca_DiReadByte(); // 9
  s->CreatedSecond = hcca_DiReadByte(); // 10

  s->ModifiedYear = hcca_DiReadUInt16(); // 11, 12
  s->ModifiedMonth = hcca_DiReadByte(); // 13
  s->ModifiedDay = hcca_DiReadByte(); // 14
  s->ModifiedHour = hcca_DiReadByte(); // 15
  s->ModifiedMinute = hcca_DiReadByte(); // 16
  s->ModifiedSecond = hcca_DiReadByte(); // 17

  s->FilenameLen = hcca_DiReadByte(); // 18

  hcca_DiReadBytes(0, 64, (uint8_t*)s->Filename); // 19-64

  s->IsFile = (s->FileSize >= 0);
  s->Exists = (s->FileSize != -2);

  hcca_DiRestoreInterrupts();
}

uint16_t rn_fileHandleReadSeq(uint8_t fileHandle, uint8_t* buffer, uint16_t bufferOffset, uint16_t readLength) {

  // 0xb5

  hcca_DiFocusInterrupts();

  uint8_t *start = buffer + bufferOffset;

  hcca_DiWriteByte(0xb5);

  hcca_DiWriteByte(fileHandle);

  hcca_DiWriteUInt16(readLength);

  uint16_t toRead = hcca_DiReadUInt16();
  uint8_t *end    = start + toRead;
  
  while (start != end) {

    while (IO_AYDATA & 0x02);
    *start = IO_HCCA;

    start++;
  }

  hcca_DiRestoreInterrupts();

  return toRead;
}

int32_t rn_fileHandleSeek(uint8_t fileHandle, int32_t offset, uint8_t seekOption) {

  // 0xb6

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xb6);

  hcca_DiWriteByte(fileHandle);

  hcca_DiWriteInt32(offset);

  hcca_DiWriteByte(seekOption);

  int32_t t = hcca_DiReadInt32();

  hcca_DiRestoreInterrupts();

  return t;
}

// **************************************************************************
// TCP/IP CLIENT
// -------------
//
// These functions are for communicating with TCP server ports
// **************************************************************************

uint8_t rn_TCPOpen(uint8_t hostnameLen, uint8_t* hostname, uint16_t port, uint8_t fileHandle) {

  // 0xd0

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xd0);

  hcca_DiWriteByte(hostnameLen);

  hcca_DiWriteBytes(0, hostnameLen, hostname);

  hcca_DiWriteUInt16(port);

  hcca_DiWriteByte(fileHandle);

  uint8_t t = hcca_DiReadByte();

  hcca_DiRestoreInterrupts();

  return t;
}

void rn_TCPHandleClose(uint8_t fileHandle) {

  // 0xd1

  hcca_writeByte(0xd1);

  hcca_writeByte(fileHandle);
}

int32_t rn_TCPHandleSize(uint8_t fileHandle) {

  // 0xd2

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xd2);

  hcca_DiWriteByte(fileHandle);

  int32_t t = hcca_DiReadInt32();

  hcca_DiRestoreInterrupts();

  return t;
}

int32_t rn_TCPHandleRead(uint8_t tcpHandle, uint8_t* buffer, uint16_t bufferOffset, uint16_t readLength) {
  // 0xd3

  hcca_DiFocusInterrupts();

  uint8_t *start = buffer + bufferOffset;

  hcca_DiWriteByte(0xd3);

  hcca_DiWriteByte(tcpHandle);

  hcca_DiWriteUInt16(readLength);

  int32_t toRead = hcca_DiReadInt32();

  if (toRead > 0) {

    uint8_t *end = start + toRead;
    
    do {

      while (IO_AYDATA & 0x02);
      *start = IO_HCCA;

      start++;

      _rxBufferReadPos++;
    } while (start != end);
  }

  hcca_DiRestoreInterrupts();

  return toRead;
}

int32_t rn_TCPHandleWrite(uint8_t tcpHandle, uint16_t dataOffset, uint16_t dataLen, uint8_t* data) {

  // 0xd4
  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xd4);

  hcca_DiWriteByte(tcpHandle);

  hcca_DiWriteUInt16(dataLen);

  hcca_DiWriteBytes(dataOffset, dataLen, data);

  int32_t wrote = hcca_DiReadInt32();

  hcca_DiRestoreInterrupts();

  return wrote;
}

// **************************************************************************
// PRINTER
// -------
//
// These functions are for printing to a file on the host PC
// **************************************************************************
void rn_Printer(uint8_t c) {

  hcca_writeByte(0xda);
  hcca_writeByte(c);
}

void rn_PunchOut(uint8_t c) {

  hcca_writeByte(0xdb);
  hcca_writeByte(c);
}