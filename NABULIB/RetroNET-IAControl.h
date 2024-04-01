// ****************************************************************************************
// NABU-LIB IA CONTROL
//
// DJ Sures (c) 2024
// https://nabu.ca
// 
// Last updated on v2024.04.01.00
// 
// Get latest copy and examples from: https://github.com/DJSures/NABU-LIB
//
// Interface for controlling the Internet Adapter settings from the NABU
// 
// Read the summaries below for details of each function in this file.
// 
// **********************************************************************************************

#ifndef RETRONET_IA_FILE_STORE_H
#define RETRONET_IA_FILE_STORE_H

#include <stdbool.h>
#include <stdlib.h>
#include "NABU-LIB.h"

uint8_t _ia_TmpOriginalInterrupt = 0;

void ia_focusInterrupts() {

  // temporarily disable all other interrupts while we perform an expensive hcca read
  // we let hcca_writeByte set the interrupt for us

  NABU_DisableInterrupts();

  _ia_TmpOriginalInterrupt = ayRead(IOPORTA);

  ayWrite(IOPORTA, INT_MASK_HCCARX);
}

void ia_restoreInterrupts() {

  ayWrite(IOPORTA, _ia_TmpOriginalInterrupt);

  NABU_EnableInterrupts();
}

// -----------------------------------------------------------
// Get the number of items at the parent level of nabu channels
// -----------------------------------------------------------
uint8_t ia_getParentCount() {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x00);

  uint8_t t = hcca_readByte();

  ia_restoreInterrupts();

  return t;
}

// -----------------------------------------------------------
// Get the name of the parent by it's ID
//
// *Note: the titleBuf must be 64 bytes long
// -----------------------------------------------------------
void ia_getParentName(uint8_t parentId, uint8_t *titleBuf) {

  // 0xbb
  ia_focusInterrupts();

  for (uint8_t i = 0; i < 64; i++)
    titleBuf[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x01);

  hcca_writeByte(parentId);

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, titleBuf);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the name of the parent by it's ID
//
// *Note: the titleBuf must be 64 bytes long
// -----------------------------------------------------------
void ia_getParentDescription(uint8_t parentId, uint8_t *titleBuf) {

  // 0xbb
  ia_focusInterrupts();

  for (uint16_t i = 0; i < 255; i++)
    titleBuf[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0d);

  hcca_writeByte(parentId);

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, titleBuf);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the number of child items under the parent Id
// -----------------------------------------------------------
uint8_t ia_getChildCount(uint8_t parentId) {

  // 0xbc
  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x02);

  hcca_writeByte(parentId);

  uint8_t t = hcca_readByte();

  ia_restoreInterrupts();

  return t;
}

// -----------------------------------------------------------
// Get the name of the child Id under the parent Id
//
// *Note: the titleBuf must be 64 bytes long
// -----------------------------------------------------------
void ia_getChildName(uint8_t parentId, uint8_t childId, uint8_t *titleBuf) {

  // 0xbd
  ia_focusInterrupts();

  for (uint8_t i = 0; i < 64; i++)
    titleBuf[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x03);

  hcca_writeByte(parentId);

  hcca_writeByte(childId);

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, titleBuf);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Set the selection in the NABU Channels
// -----------------------------------------------------------
void ia_setSelection(uint8_t parentId, uint8_t childId) {
  
  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x04);

  hcca_writeByte(parentId);

  hcca_writeByte(childId);
}

// -----------------------------------------------------------
// Get the description of the child Id under the parent Id
//
// *Note: the descBuff must be 256 bytes long
// -----------------------------------------------------------
void ia_getChildDescription(uint8_t parentId, uint8_t childId, uint8_t *descBuff) {

  ia_focusInterrupts();

  for (unsigned int i = 0; i < 255; i++)
    descBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x05); // ia_extended_getChildDescription

  hcca_writeByte(parentId);

  hcca_writeByte(childId);

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, descBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the author of the child Id under the parent Id
//
// *Note: the authBuff must be 16 bytes long
// -----------------------------------------------------------
void ia_getChildAuthor(uint8_t parentId, uint8_t childId, uint8_t *authBuff) {

  ia_focusInterrupts();

  for (uint8_t i = 0; i < 16; i++)
    authBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x06); // ia_extended_getChildAuthor

  hcca_writeByte(parentId);

  hcca_writeByte(childId);

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, authBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the current news content
//
// *Note: the newsBuff must be 512 bytes long
// -----------------------------------------------------------
void ia_getNewsContent(uint8_t *newsBuff) {

  ia_focusInterrupts();

  for (uint16_t i = 0; i < 512; i++)
    newsBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x07); // ia_extended_getNewsContent

  uint16_t readCnt = hcca_readUInt16();

  hcca_readBytes(0, readCnt, newsBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the tile color for the child. There are 4 tiles that form
// a square. There's 8 bytes per tile.
// 0 2
// 1 3
// *Note: the colorBuff must be 32 bytes long
// -----------------------------------------------------------
void ia_getChildIconTileColor(uint8_t parentId, uint8_t childId, uint8_t *colorBuff) {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x08); // ia_extended_iconTileColor

  hcca_writeByte(parentId);

  hcca_writeByte(childId);

  hcca_readBytes(0, 32, colorBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the tile pattern for the child. There are 4 tiles that form
// a square. There are 8 bytes per tile.
// 0 2
// 1 3
// *Note: the patternBuff must be 32 bytes long
// -----------------------------------------------------------
void ia_getChildIconTilePattern(uint8_t parentId, uint8_t childId, uint8_t *patternBuff) {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x09); // ia_extended_iconTilePattern

  hcca_writeByte(parentId);

  hcca_writeByte(childId);

  hcca_readBytes(0, 32, patternBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the last 512 bytes of log
//
// *Note: the logBuff must be 512 bytes long
// -----------------------------------------------------------
void ia_getLog(uint8_t *logBuff) {

  ia_focusInterrupts();

  for (uint16_t i = 0; i < 512; i++)
    logBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0a); // ia_extended_getNews

  uint16_t readCnt = hcca_readUInt16();

  hcca_readBytes(0, readCnt, logBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the current news title
//
// *Note: the titleBuff must be 64 bytes long
// -----------------------------------------------------------
void ia_getNewsTitle(uint8_t *titleBuff) {

  ia_focusInterrupts();

  for (uint8_t i = 0; i < 64; i++)
    titleBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0b); // ia_extended_getNewsContent

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, titleBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the current news date
//
// *Note: the dateBuff must be 20 bytes long
// -----------------------------------------------------------
void ia_getNewsDate(uint8_t *dateBuff) {

  ia_focusInterrupts();

  for (uint8_t i = 0; i < 20; i++)
    dateBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0c); // ia_extended_getNewsDate

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, dateBuff);

  ia_restoreInterrupts();
}

#endif
