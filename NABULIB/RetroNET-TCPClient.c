// ****************************************************************************************
// NABU-LIB C Library
// DJ Sures (c) 2023
// https://nabu.ca
// 
// Read the RetroNET-TCPClient.h file for details of each function in this file.
// 
// *Note: Some TMS9918 graphic functions were from: https://github.com/michalin/TMS9918_Arduino/tree/main/examples
//        Modified by DJ Sures 2022 for NABU PC
//
// **********************************************************************************************





// **************************************************************************
// RetroNET
// ------------
// **************************************************************************

void rn_tcpFocusInterrupts() {

  // temporarily disable all other interrupts while we perform an expensive hcca read
  // we let hcca_writeByte set the interrupt for us

  NABU_DisableInterrupts();

  _rn_TCPTmpOriginalInterrupt = _ORIGINAL_INT_MASK;  
  _ORIGINAL_INT_MASK = INT_MASK_HCCARX;

  NABU_EnableInterrupts();
}

void rn_tcpRestoreInterrupts() {

  NABU_DisableInterrupts();

  _ORIGINAL_INT_MASK = _rn_TCPTmpOriginalInterrupt;  
  ayWrite(IOPORTA, _ORIGINAL_INT_MASK);

  NABU_EnableInterrupts();
}

uint8_t rn_TCPOpen(uint8_t hostnameLen, uint8_t* hostname, uint16_t port, uint8_t fileHandle) {

  // 0xd0

  rn_tcpFocusInterrupts();

  hcca_writeByte(0xd0);

  hcca_writeByte(hostnameLen);

  hcca_writeBytes(0, hostnameLen, hostname);

  hcca_writeUInt16(port);

  hcca_writeByte(fileHandle);

  uint8_t t = hcca_readByte();

  rn_tcpRestoreInterrupts();

  return t;
}

void rn_TCPHandleClose(uint8_t fileHandle) {

  // 0xd1

  hcca_writeByte(0xd1);

  hcca_writeByte(fileHandle);
}

int32_t rn_TCPHandleSize(uint8_t fileHandle) {

  // 0xd2

  rn_tcpFocusInterrupts();

  hcca_writeByte(0xd2);

  hcca_writeByte(fileHandle);

  int32_t t = hcca_readInt32();

  rn_tcpRestoreInterrupts();

  return t;
}

int32_t rn_TCPHandleRead(uint8_t tcpHandle, uint8_t* buffer, uint16_t bufferOffset, uint16_t readLength) {
  // 0xd3

  rn_tcpFocusInterrupts();

  hcca_writeByte(0xd3);

  hcca_writeByte(fileHandle);

  hcca_writeUInt16(readLength);

  int32_t toRead = hcca_readInt32();

  hcca_readBytes(bufferOffset, toRead, buffer);

  rn_tcpRestoreInterrupts();

  return toRead;
}

int32_t rn_TCPHandleWrite(uint8_t fileHandle, uint16_t dataOffset, uint16_t dataLen, uint8_t* data) {

  // 0xd4
  rn_tcpFocusInterrupts();

  hcca_writeByte(0xd4);

  hcca_writeByte(fileHandle);

  hcca_writeUInt16(dataLen);

  hcca_writeBytes(dataOffset, dataLen, data);

  int32_t wrote = hcca_readInt32();

  rn_tcpRestoreInterrupts();

  return wrote;
}
