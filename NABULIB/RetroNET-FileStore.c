// ****************************************************************************************
// NABU-LIB C Library
// DJ Sures (c) 2024
// https://nabu.ca
//
// Read the NABU-LIB.h file for details of each function in this file.
//
// **********************************************************************************************



uint8_t _rnFS_INT_BACKUP = 0;

// Forward declarations (the 16/32/64-bit helpers call these before they are defined)
void hcca_DiWriteBytes(uint16_t offset, uint16_t length, uint8_t *bytes);
void hcca_DiReadBytes(uint16_t offset, uint16_t bufferLen, uint8_t *buffer);

// The file details reply from the Internet Adapter is laid out exactly like the first 83
// bytes of FileDetailsStruct (SDCC does not pad structs), so it is read in one block.
// If this line fails to compile, the struct layout changed and _rn_readFileDetails()
// must go back to reading field by field.
typedef char _rn_check_file_details[(sizeof(FileDetailsStruct) == 85) ? 1 : -1];


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

void hcca_DiWriteByte(uint8_t c) __z88dk_fastcall {

  ayWrite(IOPORTA, INT_MASK_HCCATX);

  IO_AYLATCH = IOPORTB;
  while (IO_AYDATA & 0x04);

  IO_HCCA = c;

  ayWrite(IOPORTA, INT_MASK_HCCARX);
}

// The multi-byte writers send the variable straight from memory. The Z80 is
// little-endian, which matches the LSB-first wire format, so there is no
// shifting of 32-bit values.
void hcca_DiWriteUInt32(uint32_t val) __z88dk_fastcall {

  hcca_DiWriteBytes(0, 4, (uint8_t *)&val);
}

void hcca_DiWriteInt32(int32_t val) __z88dk_fastcall {

  hcca_DiWriteBytes(0, 4, (uint8_t *)&val);
}

void hcca_DiWriteUInt16(uint16_t val) __z88dk_fastcall {

  hcca_DiWriteByte((uint8_t)val);
  hcca_DiWriteByte((uint8_t)(val >> 8));
}

void hcca_DiWriteInt16(int16_t val) __z88dk_fastcall {

  hcca_DiWriteUInt16((uint16_t)val);
}

void hcca_DiWriteString(uint8_t *str) __z88dk_fastcall {

  for (; *str != 0x00; str++)
    hcca_DiWriteByte(*str);
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

  // Two separate statements so the low byte is guaranteed to be read first.
  // In "a() | (b() << 8)" C does not define which call happens first.
  uint16_t ret = hcca_DiReadByte();

  return ret | ((uint16_t)hcca_DiReadByte() << 8);
}

int16_t hcca_DiReadInt16() {

  return (int16_t)hcca_DiReadUInt16();
}

// The wider readers read straight into the variable (little-endian, like the wire).
// The old array initializers did not guarantee the order of the read calls.
uint32_t hcca_DiReadUInt32() {

  uint32_t ret;

  hcca_DiReadBytes(0, 4, (uint8_t *)&ret);

  return ret;
}

int32_t hcca_DiReadInt32() {

  return (int32_t)hcca_DiReadUInt32();
}

uint64_t hcca_DiReadUInt64() {

  uint64_t ret;

  hcca_DiReadBytes(0, 8, (uint8_t *)&ret);

  return ret;
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

// Reads the 83 byte file details reply straight into the struct, then fills in the
// two flags that are not part of the reply.
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
static void _rn_readFileDetails(FileDetailsStruct *s) {

  hcca_DiReadBytes(0, 83, (uint8_t *)s);

  s->IsFile = (s->FileSize >= 0);
  s->Exists = (s->FileSize != -2);
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

uint16_t rn_FileRead(uint8_t filenameLen, uint8_t* filename, uint8_t* buffer, uint16_t bufferOffset, uint32_t readOffset, uint16_t readLength) {

  //0xe7

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xe7);

  hcca_DiWriteByte(filenameLen);

  hcca_DiWriteBytes(0, filenameLen, filename);

  hcca_DiWriteUInt32(readOffset);

  hcca_DiWriteUInt16(readLength);

  uint16_t toRead = hcca_DiReadUInt16();

  hcca_DiReadBytes(bufferOffset, toRead, buffer);

  hcca_DiRestoreInterrupts();

  return toRead;
}

void rn_FileReplace(uint8_t filenameLen, uint8_t* filename, uint32_t fileOffset, uint16_t dataOffset, uint16_t dataLen, int8_t* data) {

  // 0xe8

  hcca_writeByte(0xe8);

  // These two used the interruptless (Di) writers without hcca_DiFocusInterrupts().
  // hcca_DiWriteByte() leaves the interrupt mask set to HCCA RX only, so the keyboard
  // (and VDP) interrupts were switched off for good after calling this function.
  hcca_writeByte(filenameLen);

  hcca_writeBytes(0, filenameLen, filename);

  hcca_writeUInt32(fileOffset);

  hcca_writeUInt16(dataLen);

  hcca_writeBytes(dataOffset, dataLen, (uint8_t *)data);
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

  hcca_DiWriteByte(0xa5);

  hcca_DiWriteByte(fileHandle);

  hcca_DiWriteUInt32(readOffset);

  hcca_DiWriteUInt16(readLength);

  uint16_t toRead = hcca_DiReadUInt16();

  hcca_DiReadBytes(bufferOffset, toRead, buffer);

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

// Shared body of rn_fileHandleCopy() and rn_fileHandleMove(), which only differ by command byte
static void _rn_copyMove(uint8_t cmd, uint8_t srcFilenameLen, uint8_t* srcFilename, uint8_t destFilenameLen, uint8_t* destFilename, uint8_t copyMoveFlag) {

  hcca_writeByte(cmd);

  hcca_writeByte(srcFilenameLen);

  hcca_writeBytes(0, srcFilenameLen, srcFilename);

  hcca_writeByte(destFilenameLen);

  hcca_writeBytes(0, destFilenameLen, destFilename);

  hcca_writeByte(copyMoveFlag);
}

void rn_fileHandleCopy(uint8_t srcFilenameLen, uint8_t* srcFilename, uint8_t destFilenameLen, uint8_t* destFilename, uint8_t copyMoveFlag) {

  // 0xae

  _rn_copyMove(0xae, srcFilenameLen, srcFilename, destFilenameLen, destFilename, copyMoveFlag);
}

void rn_fileHandleMove(uint8_t srcFilenameLen, uint8_t* srcFilename, uint8_t destFilenameLen, uint8_t* destFilename, uint8_t copyMoveFlag) {

  // 0xaf

  _rn_copyMove(0xaf, srcFilenameLen, srcFilename, destFilenameLen, destFilename, copyMoveFlag);
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
  // The response is 83 bytes, see _rn_readFileDetails() for the layout

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xb2);

  hcca_DiWriteUInt16(fileItemIndex);

  _rn_readFileDetails(s);

  hcca_DiRestoreInterrupts();
}

void rn_fileDetails(uint8_t filenameLen, uint8_t* filename, FileDetailsStruct* s) {

  // 0xb3

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xb3);

  hcca_DiWriteByte(filenameLen);

  hcca_DiWriteBytes(0, filenameLen, filename);

  _rn_readFileDetails(s);

  hcca_DiRestoreInterrupts();
}

void rn_fileHandleDetails(int8_t fileHandle, FileDetailsStruct* s) {

  // 0xb4

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xb4);

  hcca_DiWriteByte(fileHandle);

  _rn_readFileDetails(s);

  hcca_DiRestoreInterrupts();
}

uint16_t rn_fileHandleReadSeq(uint8_t fileHandle, uint8_t* buffer, uint16_t bufferOffset, uint16_t readLength) {

  // 0xb5

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xb5);

  hcca_DiWriteByte(fileHandle);

  hcca_DiWriteUInt16(readLength);

  uint16_t toRead = hcca_DiReadUInt16();

  hcca_DiReadBytes(bufferOffset, toRead, buffer);

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

uint16_t rn_fileHandleLineCount(uint8_t fileHandle) {

  // 0xdc

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xdc);

  hcca_DiWriteByte(fileHandle);

  uint16_t t = hcca_DiReadUInt16();

  hcca_DiRestoreInterrupts();

  return t;

}

uint16_t rn_fileHandleGetLine(uint8_t fileHandle, uint16_t lineNumber, uint8_t *buffer) {

  // 0xdd

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xdd);

  hcca_DiWriteByte(fileHandle);

  hcca_DiWriteUInt16(lineNumber);

  uint16_t toRead = hcca_DiReadUInt16();

  hcca_DiReadBytes(0, toRead, buffer);

  hcca_DiRestoreInterrupts();

  return toRead;
}

uint16_t rn_fileHandleTruncate(uint8_t fileHandle, uint16_t truncateLength, uint8_t truncateFlag) {

  // 0xdf

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xdf);

  hcca_DiWriteByte(fileHandle);

  hcca_DiWriteUInt16(truncateLength);

  hcca_DiWriteByte(truncateFlag);

  // Server returns new file length modulo 64K (cast from int64).
  // Caller should follow up with rn_fileHandleSize() for true size if file may exceed 64KB.
  uint16_t t = hcca_DiReadUInt16();

  hcca_DiRestoreInterrupts();

  return t;
}

// Shared body of the three directory commands: send cmd + name, read a one byte result
static uint8_t _rn_directoryCmd(uint8_t cmd, uint8_t directoryNameLen, uint8_t* directoryName) {

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(cmd);

  hcca_DiWriteByte(directoryNameLen);

  hcca_DiWriteBytes(0, directoryNameLen, directoryName);

  uint8_t t = hcca_DiReadByte();

  hcca_DiRestoreInterrupts();

  return t;
}

uint8_t rn_DirectoryCreate(uint8_t directoryNameLen, uint8_t* directoryName) {

  return _rn_directoryCmd(0xea, directoryNameLen, directoryName);
}

uint8_t rn_DirectoryDelete(uint8_t directoryNameLen, uint8_t* directoryName) {

  return _rn_directoryCmd(0xeb, directoryNameLen, directoryName);
}

uint8_t rn_directoryExists(uint8_t directoryNameLen, uint8_t* directoryName) {

  return _rn_directoryCmd(0xec, directoryNameLen, directoryName);
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

  hcca_DiWriteByte(0xd3);

  hcca_DiWriteByte(tcpHandle);

  hcca_DiWriteUInt16(readLength);

  int32_t toRead = hcca_DiReadInt32();

  // The data is read directly from the HCCA port. This loop also used to advance
  // _rxBufferReadPos once per byte, which corrupted the interrupt receive ring buffer
  // (hcca_readByte() would then return up to 255 bytes of garbage).
  if (toRead > 0)
    hcca_DiReadBytes(bufferOffset, (uint16_t)toRead, buffer);

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
// HOST SHELL
// ----------
// Dispatch byte 0xee, sub-commands 0x01..0x05. See RetroNETShellClient on
// the IA side for the matching protocol. The shell feature is gated by the
// "Enable RetroNET host shell" setting in the IA — when disabled,
// rn_ShellOpen returns 0xff with no shell launched.
// **************************************************************************

uint8_t rn_ShellOpen() {

  // 0xee 0x01

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xee);

  hcca_DiWriteByte(0x01);

  uint8_t handle = hcca_DiReadByte();

  hcca_DiRestoreInterrupts();

  return handle;
}

void rn_ShellClose(uint8_t shellHandle) {

  // 0xee 0x02 — fire-and-forget, mirrors rn_TCPHandleClose

  hcca_writeByte(0xee);

  hcca_writeByte(0x02);

  hcca_writeByte(shellHandle);
}

int32_t rn_ShellAvailable(uint8_t shellHandle) {

  // 0xee 0x03

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xee);

  hcca_DiWriteByte(0x03);

  hcca_DiWriteByte(shellHandle);

  int32_t avail = hcca_DiReadInt32();

  hcca_DiRestoreInterrupts();

  return avail;
}

int32_t rn_ShellRead(uint8_t shellHandle, uint8_t* buffer, uint16_t bufferOffset, uint16_t readLength) {

  // 0xee 0x04

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xee);

  hcca_DiWriteByte(0x04);

  hcca_DiWriteByte(shellHandle);

  hcca_DiWriteUInt16(readLength);

  int32_t toRead = hcca_DiReadInt32();

  // Read directly from the HCCA port. (No longer advances _rxBufferReadPos, which
  // corrupted the interrupt receive ring buffer. See rn_TCPHandleRead.)
  if (toRead > 0)
    hcca_DiReadBytes(bufferOffset, (uint16_t)toRead, buffer);

  hcca_DiRestoreInterrupts();

  return toRead;
}

int32_t rn_ShellWrite(uint8_t shellHandle, uint16_t dataOffset, uint16_t dataLen, uint8_t* data) {

  // 0xee 0x05

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xee);

  hcca_DiWriteByte(0x05);

  hcca_DiWriteByte(shellHandle);

  hcca_DiWriteUInt16(dataLen);

  hcca_DiWriteBytes(dataOffset, dataLen, data);

  int32_t wrote = hcca_DiReadInt32();

  hcca_DiRestoreInterrupts();

  return wrote;
}


// **************************************************************************
// UTilities
// -------------
// **************************************************************************

int32_t rn_PingIP(uint8_t ipLen, uint8_t* ipStr) {

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xed);

  hcca_DiWriteByte(0x00);

  hcca_DiWriteByte(ipLen);

  hcca_DiWriteBytes(0, ipLen, ipStr);

  int32_t wrote = hcca_DiReadInt32();

  hcca_DiRestoreInterrupts();

  return wrote;
}

uint8_t rn_DnsLookup(uint8_t hostLen, uint8_t* host, uint8_t *outBuffer) {

  // 0xed sub-command 0x01: hostname -> IPv4 string (or error description).
  // Wire response: 1 byte length, followed by 'length' ASCII bytes.
  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xed);

  hcca_DiWriteByte(0x01);

  hcca_DiWriteByte(hostLen);

  hcca_DiWriteBytes(0, hostLen, host);

  uint8_t respLen = hcca_DiReadByte();

  // (hcca_DiReadBytes() reads nothing for a length of 0)
  hcca_DiReadBytes(0, respLen, outBuffer);

  // Always NUL-terminate so callers can treat outBuffer as a C string.
  outBuffer[respLen] = 0;

  hcca_DiRestoreInterrupts();

  return respLen;
}

// Shared body of rn_GetFreeDiskSpace() and rn_GetUsedDiskSpace()
static int64_t _rn_diskSpace(uint8_t subCmd) {

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xed);

  hcca_DiWriteByte(subCmd);

  // Wire payload is 8 bytes; reinterpret as signed so callers can detect -1 errors
  int64_t wrote = (int64_t)hcca_DiReadUInt64();

  hcca_DiRestoreInterrupts();

  return wrote;
}

int64_t rn_GetFreeDiskSpace() {

  return _rn_diskSpace(0x02);
}

int64_t rn_GetUsedDiskSpace() {

  return _rn_diskSpace(0x03);
}

// **************************************************************************
// TCP/IP SERVER
// -------------
//
// These functions are for communicating with clients connected to the IA TCP Server
// The IA TCP Server can be configured in the Internet Adapter settings
// **************************************************************************

// Shared body of the two TCP server commands that only send a command byte and
// read back a single byte
static uint8_t _rn_cmdReadByte(uint8_t cmd) __z88dk_fastcall {

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(cmd);

  uint8_t t = hcca_DiReadByte();

  hcca_DiRestoreInterrupts();

  return t;
}

uint8_t rn_TCPServerClientCnt() {

  // 0xd5

  return _rn_cmdReadByte(0xd5);
}

uint8_t rn_TCPServerAvailable() {

  // 0xd6

  return _rn_cmdReadByte(0xd6);
}

uint8_t rn_TCPServerRead(uint8_t* buffer, uint16_t bufferOffset, uint8_t readLength) {

  // 0xd7

  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xd7);

  hcca_DiWriteByte(readLength);

  uint8_t toRead = hcca_DiReadByte();

  // Read directly from the HCCA port. (No longer advances _rxBufferReadPos, which
  // corrupted the interrupt receive ring buffer. See rn_TCPHandleRead.)
  hcca_DiReadBytes(bufferOffset, toRead, buffer);

  hcca_DiRestoreInterrupts();

  return toRead;
}

void rn_TCPServerWrite(uint16_t dataOffset, uint8_t dataLen, uint8_t* data) {

  // 0xd8
  hcca_DiFocusInterrupts();

  hcca_DiWriteByte(0xd8);

  hcca_DiWriteByte(dataLen);

  hcca_DiWriteBytes(dataOffset, dataLen, data);

  hcca_DiRestoreInterrupts();
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
