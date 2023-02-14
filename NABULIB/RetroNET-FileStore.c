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





// **************************************************************************
// RetroNET
// ------------
// **************************************************************************

void rn_focusInterrupts() {

  // temporarily disable all other interrupts while we perform an expensive hcca read
  // we let hcca_writeByte set the interrupt for us

  NABU_DisableInterrupts();

  _rn_TmpOriginalInterrupt = _ORIGINAL_INT_MASK;  
  _ORIGINAL_INT_MASK = INT_MASK_HCCARX;
  
  ayWrite(IOPORTA, INT_MASK_HCCARX);

  NABU_EnableInterrupts();
}

void rn_restoreInterrupts() {

  NABU_DisableInterrupts();

  _ORIGINAL_INT_MASK = _rn_TmpOriginalInterrupt;  
  ayWrite(IOPORTA, _ORIGINAL_INT_MASK);

  NABU_EnableInterrupts();
}

uint8_t rn_fileOpen(uint8_t filenameLen, uint8_t* filename, uint16_t fileFlag, uint8_t fileHandle) {

  //0xa3

  rn_focusInterrupts();

  hcca_writeByte(0xa3);

  hcca_writeByte(filenameLen);

  hcca_writeBytes(0, filenameLen, filename);

  hcca_writeUInt16(fileFlag);

  hcca_writeByte(fileHandle);

  uint8_t t = hcca_readByte();

  rn_restoreInterrupts();

  return t;
}

void rn_fileHandleClose(uint8_t fileHandle) {

  //0xa7

  hcca_writeByte(0xa7);

  hcca_writeByte(fileHandle);
}

int32_t rn_fileSize(uint8_t filenameLen, uint8_t* filename) {

  //0xa8

  rn_focusInterrupts();

  hcca_writeByte(0xa8);

  hcca_writeByte(filenameLen);

  hcca_writeBytes(0, filenameLen, filename);

  int32_t t = hcca_readInt32();

  rn_restoreInterrupts();

  return t;
}

int32_t rn_fileHandleSize(uint8_t fileHandle) {

  //0xa4

  rn_focusInterrupts();

  hcca_writeByte(0xa4);

  hcca_writeByte(fileHandle);

  int32_t t = hcca_readInt32();

  rn_restoreInterrupts();

  return t;
}

uint16_t rn_fileHandleRead(uint8_t fileHandle, uint8_t* buffer, uint16_t bufferOffset, uint32_t readOffset, uint16_t readLength) {

  //0xa5

  rn_focusInterrupts();

  hcca_writeByte(0xa5);

  hcca_writeByte(fileHandle);

  hcca_writeUInt32(readOffset);

  hcca_writeUInt16(readLength);

  uint16_t toRead = hcca_readUInt16();

  hcca_readBytes(bufferOffset, toRead, buffer);

  rn_restoreInterrupts();

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

  rn_focusInterrupts();

  hcca_writeByte(0xb1);

  hcca_writeByte(pathLen);

  hcca_writeBytes(0, pathLen, path);

  hcca_writeByte(wildcardLen);

  hcca_writeBytes(0, wildcardLen, wildcard);

  hcca_writeByte(fileListFlags);

  uint16_t t = hcca_readUInt16();

  rn_restoreInterrupts();

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

  rn_focusInterrupts();

  hcca_writeByte(0xb2);

  hcca_writeUInt16(fileItemIndex);

  s->FileSize = hcca_readInt32(); // 0, 1, 2, 3

  s->CreatedYear = hcca_readUInt16(); // 4, 5
  s->CreatedMonth = hcca_readByte(); // 6
  s->CreatedDay = hcca_readByte(); // 7
  s->CreatedHour = hcca_readByte(); // 8
  s->CreatedMinute = hcca_readByte(); // 9
  s->CreatedSecond = hcca_readByte(); // 10

  s->ModifiedYear = hcca_readUInt16(); // 11, 12
  s->ModifiedMonth = hcca_readByte(); // 13
  s->ModifiedDay = hcca_readByte(); // 14
  s->ModifiedHour = hcca_readByte(); // 15
  s->ModifiedMinute = hcca_readByte(); // 16
  s->ModifiedSecond = hcca_readByte(); // 17

  s->FilenameLen = hcca_readByte(); // 18

  hcca_readBytes(0, 64, (uint8_t*)s->Filename); // 19-64

  s->IsFile = (s->FileSize >= 0);
  s->Exists = (s->FileSize != -2);

  rn_restoreInterrupts();
}

void rn_fileDetails(uint8_t filenameLen, uint8_t* filename, FileDetailsStruct* s) {

  // 0xb3

  rn_focusInterrupts();

  hcca_writeByte(0xb3);

  hcca_writeByte(filenameLen);

  hcca_writeBytes(0, filenameLen, filename);

  s->FileSize = hcca_readInt32(); // 0, 1, 2, 3

  s->CreatedYear = hcca_readUInt16(); // 4, 5
  s->CreatedMonth = hcca_readByte(); // 6
  s->CreatedDay = hcca_readByte(); // 7
  s->CreatedHour = hcca_readByte(); // 8
  s->CreatedMinute = hcca_readByte(); // 9
  s->CreatedSecond = hcca_readByte(); // 10

  s->ModifiedYear = hcca_readUInt16(); // 11, 12
  s->ModifiedMonth = hcca_readByte(); // 13
  s->ModifiedDay = hcca_readByte(); // 14
  s->ModifiedHour = hcca_readByte(); // 15
  s->ModifiedMinute = hcca_readByte(); // 16
  s->ModifiedSecond = hcca_readByte(); // 17

  s->FilenameLen = hcca_readByte(); // 18

  hcca_readBytes(0, 64, (uint8_t*)s->Filename); // 19-64

  s->IsFile = (s->FileSize >= 0);
  s->Exists = (s->FileSize != -2);

  rn_restoreInterrupts();
}

void rn_fileHandleDetails(int8_t fileHandle, FileDetailsStruct* s) {

  // 0xb4

  rn_focusInterrupts();

  hcca_writeByte(0xb4);

  hcca_writeByte(fileHandle);

  s->FileSize = hcca_readInt32(); // 0, 1, 2, 3

  s->CreatedYear = hcca_readUInt16(); // 4, 5
  s->CreatedMonth = hcca_readByte(); // 6
  s->CreatedDay = hcca_readByte(); // 7
  s->CreatedHour = hcca_readByte(); // 8
  s->CreatedMinute = hcca_readByte(); // 9
  s->CreatedSecond = hcca_readByte(); // 10

  s->ModifiedYear = hcca_readUInt16(); // 11, 12
  s->ModifiedMonth = hcca_readByte(); // 13
  s->ModifiedDay = hcca_readByte(); // 14
  s->ModifiedHour = hcca_readByte(); // 15
  s->ModifiedMinute = hcca_readByte(); // 16
  s->ModifiedSecond = hcca_readByte(); // 17

  s->FilenameLen = hcca_readByte(); // 18

  hcca_readBytes(0, 64, (uint8_t*)s->Filename); // 19-64

  s->IsFile = (s->FileSize >= 0);
  s->Exists = (s->FileSize != -2);

  rn_restoreInterrupts();
}

uint16_t rn_fileHandleReadSeq(uint8_t fileHandle, uint8_t* buffer, uint16_t bufferOffset, uint16_t readLength) {

  // 0xb5

  rn_focusInterrupts();

  hcca_writeByte(0xb5);

  hcca_writeByte(fileHandle);

  hcca_writeUInt16(readLength);

  uint16_t toRead = hcca_readUInt16();

  hcca_readBytes(bufferOffset, toRead, buffer);

  rn_restoreInterrupts();

  return toRead;
}

uint32_t rn_fileHandleSeek(uint8_t fileHandle, int32_t offset, uint8_t seekOption) {

  // 0xb6

  rn_focusInterrupts();

  hcca_writeByte(0xb6);

  hcca_writeByte(fileHandle);

  hcca_writeInt32(offset);

  hcca_writeByte(seekOption);

  uint32_t t = hcca_readUInt32();

  rn_restoreInterrupts();

  return t;
}

