// ****************************************************************************************
// NABU-LIB IA CONTROL
//
// DJ Sures (c) 2024
// https://nabu.ca
// 
// Last updated on v2024.05.18.00
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
uint16_t ia_getChildCount2(uint8_t parentId) {

  // 0xbc
  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x18);

  hcca_writeByte(parentId);

  uint16_t t = hcca_readUInt16();

  ia_restoreInterrupts();

  return t;
}

// -----------------------------------------------------------
// Get the name of the child Id under the parent Id
//
// *Note: the titleBuf must be 64 bytes long
// -----------------------------------------------------------
void ia_getChildName2(uint8_t parentId, uint16_t childId, uint8_t *titleBuf) {

  // 0xbd
  ia_focusInterrupts();

  for (uint8_t i = 0; i < 64; i++)
    titleBuf[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x19);

  hcca_writeByte(parentId);

  hcca_writeUInt16(childId);

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, titleBuf);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Set the selection in the NABU Channels
// -----------------------------------------------------------
void ia_setSelection2(uint8_t parentId, uint16_t childId) {
  
  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x1a);

  hcca_writeByte(parentId);

  hcca_writeUInt16(childId);
}

// -----------------------------------------------------------
// Get the description of the child Id under the parent Id
//
// *Note: the descBuff must be 256 bytes long
// -----------------------------------------------------------
void ia_getChildDescription2(uint8_t parentId, uint16_t childId, uint8_t *descBuff) {

  ia_focusInterrupts();

  for (unsigned int i = 0; i < 255; i++)
    descBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x1b); // ia_extended_getChildDescription

  hcca_writeByte(parentId);

  hcca_writeUInt16(childId);

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, descBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the author of the child Id under the parent Id
//
// *Note: the authBuff must be 16 bytes long
// -----------------------------------------------------------
void ia_getChildAuthor2(uint8_t parentId, uint16_t childId, uint8_t *authBuff) {

  ia_focusInterrupts();

  for (uint8_t i = 0; i < 16; i++)
    authBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x1c); // ia_extended_getChildAuthor

  hcca_writeByte(parentId);

  hcca_writeUInt16(childId);

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
void ia_getChildIconTileColor2(uint8_t parentId, uint16_t childId, uint8_t *colorBuff) {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x1d); // ia_extended_iconTileColor

  hcca_writeByte(parentId);

  hcca_writeUInt16(childId);

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
void ia_getChildIconTilePattern2(uint8_t parentId, uint16_t childId, uint8_t *patternBuff) {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x1e); // ia_extended_iconTilePattern

  hcca_writeByte(parentId);

  hcca_writeUInt16(childId);

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

// -----------------------------------------------------------
// Get the number of news articles
// -----------------------------------------------------------
uint8_t ia_extended_getNewsCount() {

  // 0x0e
  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0e); // ia_extended_getNewsCount

  uint8_t t = hcca_readByte();

  ia_restoreInterrupts();

  return t;
}

// -----------------------------------------------------------
// Get the current news content by index
//
// *Note: the newsBuff must be 512 bytes long
// -----------------------------------------------------------
void ia_extended_getNewsContentById(uint8_t id, uint8_t *newsBuff) {

  ia_focusInterrupts();

  for (uint16_t i = 0; i < 512; i++)
    newsBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0f); // ia_extended_getNewsContentById

  hcca_writeByte(id); 

  uint16_t readCnt = hcca_readUInt16();

  hcca_readBytes(0, readCnt, newsBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the current news title by index
//
// *Note: the titleBuff must be 64 bytes long
// -----------------------------------------------------------
void ia_extended_getNewsTitleById(uint8_t id, uint8_t *titleBuff) {

  ia_focusInterrupts();

  for (uint8_t i = 0; i < 64; i++)
    titleBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x10); // ia_extended_getNewsTitleById

  hcca_writeByte(id);

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, titleBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the current news date by id
//
// *Note: the dateBuff must be 20 bytes long
// -----------------------------------------------------------
void ia_extended_getNewsDateById(uint8_t id, uint8_t *dateBuff) {

  ia_focusInterrupts();

  for (uint8_t i = 0; i < 20; i++)
    dateBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x11); // ia_extended_getNewsDateById

  hcca_writeByte(id);

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, dateBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the tile color for the news item by id
// There's 8 bytes per tile.
// 0 2
// 1 3
// *Note: the colorBuff must be 32 bytes long
// -----------------------------------------------------------
void ia_extended_getNewsIconTileColor(uint8_t id, uint8_t *colorBuff) {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x12); // ia_extended_getNewsIconTileColor

  hcca_writeByte(id);

  hcca_readBytes(0, 32, colorBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the tile pattern for the news item by id
// There's 8 bytes per tile.
// 0 2
// 1 3
// *Note: the colorBuff must be 32 bytes long
// -----------------------------------------------------------
void ia_extended_getNewsIconTilePattern(uint8_t id, uint8_t *patternBuff) {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x13); // ia_extended_getNewsIconTilePattern

  hcca_writeByte(id);

  hcca_readBytes(0, 32, patternBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the operating system of the internet adapter
// 0 = Windows
// 1 = MacOS
// 2 = Linux
// 99 = Unknown
// -----------------------------------------------------------
uint8_t ia_extended_getOperatingSystem() {

  // 0x14
  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x14); // ia_extended_getOperatingSystem

  uint8_t t = hcca_readByte();

  ia_restoreInterrupts();

  return t;
}

// -----------------------------------------------------------
// Get the current date/time as a string in the specified format
// Send blank dateformat with 0 bytes for a default string.
//
// Examples: 
//   "dddd, MMMM dd" outputs "Sunday, April 14"
//   "yyyy-MM-dd HH:mm:ss" outputs "2024-04-14 19:04:01"
//   "HH:mm:ss" outputs 19:04:01
//   "MM/dd/yyyy HH:mm" outputs 04/14/2024 19:04
//
// yyyy: Four-digit year.
// yy: Two-digit year.
// MMMM: Full month name.
// MMM: Abbreviated month name.
// MM: Two-digit month (with leading zero).
// M: One or two-digit month.
// dddd: Full name of the day of the week.
// ddd: Abbreviated name of the day.
// dd: Two-digit day of the month (with leading zero).
// d: One or two-digit day of the month.
// HH: Two digits of hour (24-hour clock) with leading zeros.
// H: One or two digits of hour (24-hour clock).
// hh: Two digits of hour (12-hour clock) with leading zeros.
// h: One or two digits of hour (12-hour clock).
// mm: Two digits of minute with leading zeros.
// m: One or two digits of minute.
// ss: Two digits of second with leading zeros.
// s: One or two digits of second.
// fff: Three digits of milliseconds.
// ff: Two digits of milliseconds.
// f: One digit of milliseconds.
// tt: AM or PM designator.
// zzz: Time zone offset in hours and minutes.
// zz: Time zone offset in hours.
// z: Hours of time zone offset.
// 
// *Note: the dateBuff must be 64 bytes long
// -----------------------------------------------------------
void ia_getCurrentDateTimeStr(uint8_t *dateFormatStr, uint8_t dateFormatStrLen, uint8_t *dateBuff) {

  ia_focusInterrupts();

  for (uint8_t i = 0; i < 64; i++)
    dateBuff[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x15); // ia_getCurrentDateTimeStr

  hcca_writeByte(dateFormatStrLen);

  hcca_writeBytes(0, dateFormatStrLen, dateFormatStr);

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, dateBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Returns a string with the internet adapter version
// versionStr must be 14 bytes long
// ie 2024.04.19.00
// -----------------------------------------------------------
void ia_getAdapterVersion(uint8_t *versionStr) {

  ia_focusInterrupts();

  for (uint8_t i = 0; i < 14; i++)
    versionStr[i] = 0x00;

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x16); // ia_getAdapterVersion

  uint8_t readCnt = hcca_readByte();

  hcca_readBytes(0, readCnt, versionStr);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Returns true or false if a new version is available
// -----------------------------------------------------------
bool ia_getNewVersionAvailable() {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x17); // ia_getNewVersionAvailable

  uint8_t val = hcca_readByte();

  ia_restoreInterrupts();

  return (val > 0);
}

#endif
