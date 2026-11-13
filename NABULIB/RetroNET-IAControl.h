// ****************************************************************************************
// NABU-LIB IA CONTROL
//
// DJ Sures (c) 2024
// https://nabu.ca
// 
// Last updated on v2026.11.12.00 (weather)
// 
// Get latest copy and examples from: https://github.com/DJSures/NABU-LIB
//
// Interface for controlling the Internet Adapter settings from the NABU
// 
// Read the summaries below for details of each function in this file.
//
// **********************************************************************************************

// **********************************************************************************************
// FUNCTION INDEX
// --------------
// A quick reference for every function in this file. Scroll down to a function for its
// full description.
//
// Buffer sizes are the sizes the function writes. Give it at least that many bytes. Text
// replies are padded with zeros to the end of the buffer.
//
// NABU CHANNELS (the categories and programs the adapter offers)
//   uint8_t  ia_getParentCount()
//              Number of top-level categories (parents).
//
//   void     ia_getParentName(uint8_t parentId, uint8_t *titleBuf)
//              titleBuf (64 bytes): name of the parent.
//
//   void     ia_getParentDescription(uint8_t parentId, uint8_t *titleBuf)
//              titleBuf (255 bytes): description of the parent.
//
//   uint16_t ia_getChildCount2(uint8_t parentId)
//              Number of programs (children) under a parent.
//
//   void     ia_getChildName2(uint8_t parentId, uint16_t childId, uint8_t *titleBuf)
//              titleBuf (64 bytes): name of the child.
//
//   void     ia_getChildDescription2(uint8_t parentId, uint16_t childId, uint8_t *descBuff)
//              descBuff (255 bytes): description of the child. Use 256 bytes and set
//              descBuff[255] = 0 if you need it NUL terminated.
//
//   void     ia_getChildAuthor2(uint8_t parentId, uint16_t childId, uint8_t *authBuff)
//              authBuff (16 bytes): author of the child.
//
//   void     ia_getChildIconTileColor2(uint8_t parentId, uint16_t childId, uint8_t *colorBuff)
//              colorBuff (32 bytes): icon colours, 4 tiles x 8 bytes, tile order 0 2 / 1 3.
//
//   void     ia_getChildIconTilePattern2(uint8_t parentId, uint16_t childId, uint8_t *patternBuff)
//              patternBuff (32 bytes): icon patterns, same layout as the colours.
//
//   void     ia_setSelection2(uint8_t parentId, uint16_t childId)
//              Sets the selection in the NABU Channels.
//
// NEWS
//   void     ia_getNewsContent(uint8_t *newsBuff)
//              newsBuff (512 bytes): body of the current news item.
//
//   void     ia_getNewsTitle(uint8_t *titleBuff)
//              titleBuff (64 bytes): title of the current news item.
//
//   void     ia_getNewsDate(uint8_t *dateBuff)
//              dateBuff (20 bytes): date of the current news item.
//
//   uint8_t  ia_extended_getNewsCount()
//              Number of news articles, for the ...ById functions below.
//
//   void     ia_extended_getNewsContentById(uint8_t id, uint8_t *newsBuff)
//              newsBuff (512 bytes): body of news article id.
//
//   void     ia_extended_getNewsTitleById(uint8_t id, uint8_t *titleBuff)
//              titleBuff (64 bytes): title of news article id.
//
//   void     ia_extended_getNewsDateById(uint8_t id, uint8_t *dateBuff)
//              dateBuff (20 bytes): date of news article id.
//
//   void     ia_extended_getNewsIconTileColor(uint8_t id, uint8_t *colorBuff)
//              colorBuff (32 bytes): icon colours of news article id (tile order 0 2 / 1 3).
//
//   void     ia_extended_getNewsIconTilePattern(uint8_t id, uint8_t *patternBuff)
//              patternBuff (32 bytes): icon patterns of news article id.
//
// ADAPTER
//   void     ia_getLog(uint8_t *logBuff)
//              logBuff (512 bytes): the last 512 bytes of the adapter's log.
//
//   uint8_t  ia_extended_getOperatingSystem()
//              OS the adapter runs on: 0 Windows, 1 MacOS, 2 Linux, 99 unknown.
//
//   void     ia_getCurrentDateTimeStr(uint8_t *dateFormatStr, uint8_t dateFormatStrLen,
//                                     uint8_t *dateBuff)
//              dateBuff (64 bytes): current date / time in a format such as
//              "yyyy-MM-dd HH:mm:ss". Pass length 0 for the default format. The format
//              codes are listed at the function.
//
//   void     ia_getAdapterVersion(uint8_t *versionStr)
//              versionStr (14 bytes): adapter version, e.g. "2024.04.19.00".
//
//   uint32_t ia_getAdapterVersionNumber()
//              Adapter version as a number, e.g. 2026111200 for "2026.11.12.00", so you
//              can write if (ia_getAdapterVersionNumber() >= 2026111200UL). Works with
//              every adapter (it reads the version string, it doesn't need 0x23).
//
//   bool     ia_getNewVersionAvailable()
//              True when a newer adapter version is available.
//
// WEATHER (needs adapter IA_WEATHER_MIN_VERSION or newer - check ia_isWeatherSupported() first)
//   bool     ia_isWeatherSupported()
//              True when the adapter understands the weather functions. An older adapter
//              never answers them and the NABU would hang, so call this FIRST.
//
//   uint8_t  ia_getWeather(uint8_t daysFromNow, uint8_t partOfDay, uint8_t units,
//                          WeatherInfo *info)
//              Fills info (WEATHER_INFO_SIZE = 177 bytes) with the forecast for
//              daysFromNow (0 = today ... 6) and partOfDay (WEATHER_PART_*): temperatures,
//              condition text + icon id, precipitation, wind, UV, sunrise/sunset and a
//              ready-to-print summary. Returns WEATHER_STATUS_OK or WEATHER_STATUS_*.
//              The adapter caches the forecast for 30 minutes, so after the first call
//              (which can take a few seconds) every call is answered instantly.
//
//   void     ia_getWeatherIcon(uint8_t conditionId, uint8_t bgColor,
//                              uint8_t *patternBuff, uint8_t *colorBuff)
//              patternBuff (32 bytes), colorBuff (32 bytes): 16x16 icon for a
//              WEATHER_COND_* id (info.conditionId), tile order 0 2 / 1 3, same layout
//              as the channel icons. bgColor is the TMS9918 background colour (0-15).
//
//   uint8_t  ia_setWeatherLocation(uint8_t *location, uint8_t locationLen,
//                                  bool saveToSettings, uint8_t *resolvedName)
//              Changes the weather location: "Toronto", "Portland, ME", a postal code
//              or "43.65,-79.38". saveToSettings false = this session only. Length 0
//              goes back to the adapter's setting. resolvedName (32 bytes, may be NULL):
//              the place found, e.g. "Toronto, Ontario". Returns WEATHER_STATUS_*.
//
// LOW LEVEL (the functions above already do this; you rarely need them)
//   void     ia_focusInterrupts()
//   void     ia_restoreInterrupts()
//              Mask every interrupt except HCCA receive around an adapter exchange, then
//              put the previous mask back. _ia_readReply() is internal.
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
// Reads a reply of readCnt bytes into buf, which is bufLen bytes long.
// - Bytes beyond bufLen are read and discarded, so a long reply can never
//   overrun the caller's buffer (the old code wrote readCnt bytes regardless).
// - The unused tail of buf is zeroed. This replaces the byte-by-byte clear loops
//   each function used to run before sending its command.
// - A readCnt of 0 is safe (hcca_readBytes used to read 65536 bytes and hang).
// -----------------------------------------------------------
void _ia_readReply(uint16_t readCnt, uint8_t *buf, uint16_t bufLen) {

  uint16_t keep = (readCnt > bufLen) ? bufLen : readCnt;

  hcca_readBytes(0, keep, buf);

  memset(buf + keep, 0x00, bufLen - keep);

  for (readCnt -= keep; readCnt != 0; readCnt--)
    hcca_readByte();
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


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x01);

  hcca_writeByte(parentId);

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, titleBuf, 64);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the name of the parent by it's ID
//
// *Note: the titleBuf must be 64 bytes long
// (correction: this is the description, so titleBuf must be at least 255 bytes long)
// -----------------------------------------------------------
void ia_getParentDescription(uint8_t parentId, uint8_t *titleBuf) {

  // 0xbb
  ia_focusInterrupts();


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0d);

  hcca_writeByte(parentId);

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, titleBuf, 255);

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


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x19);

  hcca_writeByte(parentId);

  hcca_writeUInt16(childId);

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, titleBuf, 64);

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
// *Note: the descBuff must be at least 255 bytes long. The function
// writes exactly 255 bytes: the description, padded with zeros. A
// description of the full 255 characters leaves no room for a NUL,
// so if you need a terminated string use a 256 byte buffer and set
// descBuff[255] = 0 yourself.
// -----------------------------------------------------------
void ia_getChildDescription2(uint8_t parentId, uint16_t childId, uint8_t *descBuff) {

  ia_focusInterrupts();


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x1b); // ia_extended_getChildDescription

  hcca_writeByte(parentId);

  hcca_writeUInt16(childId);

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, descBuff, 255);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the author of the child Id under the parent Id
//
// *Note: the authBuff must be 16 bytes long
// -----------------------------------------------------------
void ia_getChildAuthor2(uint8_t parentId, uint16_t childId, uint8_t *authBuff) {

  ia_focusInterrupts();


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x1c); // ia_extended_getChildAuthor

  hcca_writeByte(parentId);

  hcca_writeUInt16(childId);

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, authBuff, 16);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the current news content
//
// *Note: the newsBuff must be 512 bytes long
// -----------------------------------------------------------
void ia_getNewsContent(uint8_t *newsBuff) {

  ia_focusInterrupts();


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x07); // ia_extended_getNewsContent

  uint16_t readCnt = hcca_readUInt16();

  _ia_readReply(readCnt, newsBuff, 512);

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


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0a); // ia_extended_getNews

  uint16_t readCnt = hcca_readUInt16();

  _ia_readReply(readCnt, logBuff, 512);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the current news title
//
// *Note: the titleBuff must be 64 bytes long
// -----------------------------------------------------------
void ia_getNewsTitle(uint8_t *titleBuff) {

  ia_focusInterrupts();


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0b); // ia_extended_getNewsContent

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, titleBuff, 64);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the current news date
//
// *Note: the dateBuff must be 20 bytes long
// -----------------------------------------------------------
void ia_getNewsDate(uint8_t *dateBuff) {

  ia_focusInterrupts();


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0c); // ia_extended_getNewsDate

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, dateBuff, 20);

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


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x0f); // ia_extended_getNewsContentById

  hcca_writeByte(id); 

  uint16_t readCnt = hcca_readUInt16();

  _ia_readReply(readCnt, newsBuff, 512);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the current news title by index
//
// *Note: the titleBuff must be 64 bytes long
// -----------------------------------------------------------
void ia_extended_getNewsTitleById(uint8_t id, uint8_t *titleBuff) {

  ia_focusInterrupts();


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x10); // ia_extended_getNewsTitleById

  hcca_writeByte(id);

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, titleBuff, 64);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Get the current news date by id
//
// *Note: the dateBuff must be 20 bytes long
// -----------------------------------------------------------
void ia_extended_getNewsDateById(uint8_t id, uint8_t *dateBuff) {

  ia_focusInterrupts();


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x11); // ia_extended_getNewsDateById

  hcca_writeByte(id);

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, dateBuff, 20);

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


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x15); // ia_getCurrentDateTimeStr

  hcca_writeByte(dateFormatStrLen);

  hcca_writeBytes(0, dateFormatStrLen, dateFormatStr);

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, dateBuff, 64);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Returns a string with the internet adapter version
// versionStr must be 14 bytes long
// ie 2024.04.19.00
// -----------------------------------------------------------
void ia_getAdapterVersion(uint8_t *versionStr) {

  ia_focusInterrupts();


  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x16); // ia_getAdapterVersion

  uint8_t readCnt = hcca_readByte();

  _ia_readReply(readCnt, versionStr, 14);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Returns the internet adapter version as a number so versions
// can be compared with a plain >=
// ie "2026.11.12.00" returns 2026111200
//
// Adapters since 2026.11.12.00 also answer 0xBA 0x23 with this
// number directly, but an older adapter would never answer it
// and the NABU would hang. So this builds the number from the
// version string (0x16), which every adapter since 2024 answers.
// Returns 0 if the version can't be read.
// -----------------------------------------------------------
uint32_t ia_getAdapterVersionNumber() {

  uint8_t version[14];

  ia_getAdapterVersion(version);

  uint32_t result = 0;
  uint16_t part   = 0;
  uint8_t  parts  = 0;
  uint8_t  *p     = version;

  // "yyyy.mm.dd.bb" -> yyyy, then * 100 + mm, * 100 + dd, * 100 + bb
  while (parts < 4) {

    if (*p >= '0' && *p <= '9') {

      part = (part * 10) + (*p - '0');

      p++;

      continue;
    }

    // '.' or end of string closes the part
    result = (parts == 0) ? part : (result * 100) + part;

    part = 0;

    parts++;

    if (*p == 0x00)
      break;

    p++;
  }

  // Missing parts (e.g. "2026.11") count as 00
  for (; parts < 4; parts++)
    result *= 100;

  return result;
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

// **********************************************************************************************
// WEATHER
//
// Forecasts come from Open-Meteo through the Internet Adapter. The location and default
// units are set in the adapter's Settings -> Weather tab (or with ia_setWeatherLocation).
//
// Example
// -------
//   WeatherInfo wx;
//   uint8_t     pattern[32];
//   uint8_t     color[32];
//
//   if (!ia_isWeatherSupported())
//     -> tell the user to update their Internet Adapter
//
//   if (ia_getWeather(0, WEATHER_PART_NOW, WEATHER_UNITS_DEFAULT, &wx) == WEATHER_STATUS_OK) {
//
//     // wx.locationStr  "Toronto, Ontario"
//     // wx.periodStr    "Now"
//     // wx.summaryStr   "Cloudy, 3C. Feels like -1C. Wind 15 km/h NW."
//     // wx.temperature  3   (wx.units: 0 = C, 1 = F)
//
//     ia_getWeatherIcon(wx.conditionId, VDP_BLACK, pattern, color);
//     // load pattern / color into 4 tiles, drawn as   0 2
//     //                                                1 3
//   }
//
//   // Tomorrow afternoon, always in Fahrenheit
//   ia_getWeather(1, WEATHER_PART_AFTERNOON, WEATHER_UNITS_IMPERIAL, &wx);
// **********************************************************************************************

// The minimum Internet Adapter version that understands the weather functions, as
// returned by ia_getAdapterVersionNumber(). Define your own value before including this
// file to override it.
#ifndef IA_WEATHER_MIN_VERSION
#define IA_WEATHER_MIN_VERSION 2026111200UL
#endif

// Number of bytes the adapter sends for a WeatherInfo
#define WEATHER_INFO_SIZE 177

// Result / WeatherInfo.status
#define WEATHER_STATUS_OK                 0 // all good
#define WEATHER_STATUS_NOT_CONFIGURED     1 // no location set in the adapter
#define WEATHER_STATUS_LOCATION_NOT_FOUND 2 // the location could not be found
#define WEATHER_STATUS_UNAVAILABLE        3 // weather service unreachable (no internet?)
#define WEATHER_STATUS_BAD_REQUEST        4 // daysFromNow or partOfDay out of range

// partOfDay
#define WEATHER_PART_FULL_DAY  0 // the whole day (midnight to midnight)
#define WEATHER_PART_MORNING   1 // 6am - noon
#define WEATHER_PART_AFTERNOON 2 // noon - 6pm
#define WEATHER_PART_EVENING   3 // 6pm - midnight
#define WEATHER_PART_OVERNIGHT 4 // midnight - 6am of the following day ("tonight")
#define WEATHER_PART_NOW       5 // current conditions, daysFromNow is ignored

// daysFromNow can be 0 (today) to WEATHER_MAX_DAYS
#define WEATHER_MAX_DAYS 6

// units
#define WEATHER_UNITS_DEFAULT  0 // whatever the adapter is set to
#define WEATHER_UNITS_METRIC   1 // C, km/h, tenths of mm
#define WEATHER_UNITS_IMPERIAL 2 // F, mph, hundredths of an inch

// WeatherInfo.conditionId - also the icon id for ia_getWeatherIcon()
#define WEATHER_COND_UNKNOWN             0
#define WEATHER_COND_CLEAR_DAY           1
#define WEATHER_COND_CLEAR_NIGHT         2
#define WEATHER_COND_PARTLY_CLOUDY_DAY   3
#define WEATHER_COND_PARTLY_CLOUDY_NIGHT 4
#define WEATHER_COND_CLOUDY              5
#define WEATHER_COND_FOG                 6
#define WEATHER_COND_DRIZZLE             7
#define WEATHER_COND_RAIN                8
#define WEATHER_COND_HEAVY_RAIN          9
#define WEATHER_COND_FREEZING_RAIN       10
#define WEATHER_COND_SNOW                11
#define WEATHER_COND_HEAVY_SNOW          12
#define WEATHER_COND_THUNDERSTORM        13
#define WEATHER_COND_WINDY               14
#define WEATHER_COND_COUNT               15

// The weather for one day / part of day. Exactly WEATHER_INFO_SIZE (177) bytes, in the
// order the adapter sends them (z88dk does not pad structs). Strings are NUL terminated.
//
// For WEATHER_PART_NOW: temperature / feelsLike / humidity / wind are the current
// readings and tempHigh / tempLow are today's. For a part of the day they are the
// average (temperature, feelsLike, humidity), highest (tempHigh, precipChance, windSpeed,
// uvIndex), lowest (tempLow) and total (precipAmount) over those hours.
typedef struct {
  uint8_t  status;           // 0   WEATHER_STATUS_*. When not OK everything else is 0
  uint8_t  conditionId;      // 1   WEATHER_COND_*, pass to ia_getWeatherIcon()
  uint8_t  wmoCode;          // 2   raw WMO weather code, for programs that want more detail
  uint8_t  units;            // 3   0 = metric (C, km/h, mm), 1 = imperial (F, mph, in)
  int16_t  temperature;      // 4   degrees
  int16_t  tempHigh;         // 6   degrees
  int16_t  tempLow;          // 8   degrees
  int16_t  feelsLike;        // 10  degrees (wind chill / humidex)
  uint8_t  humidity;         // 12  percent
  uint8_t  precipChance;     // 13  percent
  uint16_t precipAmount;     // 14  metric: tenths of mm (25 = 2.5mm), imperial: hundredths of an inch (25 = 0.25in)
  uint16_t windSpeed;        // 16  km/h or mph
  uint16_t windDirection;    // 18  degrees the wind comes from, 0 = north, 90 = east
  uint8_t  uvIndex;          // 20  UV index (0 at night)
  uint8_t  isDay;            // 21  1 = the icon is a daytime icon (sun), 0 = night (moon)
  uint8_t  sunriseHour;      // 22  local time at the location, 24 hour clock
  uint8_t  sunriseMinute;    // 23
  uint8_t  sunsetHour;       // 24
  uint8_t  sunsetMinute;     // 25
  uint8_t  month;            // 26  1 - 12, date of the forecast day
  uint8_t  day;              // 27  1 - 31
  uint8_t  dayOfWeek;        // 28  0 = Sunday ... 6 = Saturday
  uint8_t  windDirStr[4];    // 29  compass point, e.g. "NNE"
  uint8_t  conditionStr[24]; // 33  e.g. "Partly cloudy", "Light snow"
  uint8_t  periodStr[24];    // 57  e.g. "Now", "This afternoon", "Tomorrow", "Friday overnight"
  uint8_t  locationStr[32];  // 81  e.g. "Toronto, Ontario"
  uint8_t  summaryStr[64];   // 113 e.g. "Partly cloudy. High 12C, low 4C. Precip 30%. Wind 15 km/h NW."
} WeatherInfo;

// -----------------------------------------------------------
// Returns true when the Internet Adapter supports the weather
// functions. Call it before any other weather function: an older
// adapter does not answer them and the NABU would hang waiting.
// -----------------------------------------------------------
bool ia_isWeatherSupported() {

  return ia_getAdapterVersionNumber() >= IA_WEATHER_MIN_VERSION;
}

// -----------------------------------------------------------
// Get the weather for a day and part of the day.
//
// daysFromNow: 0 = today, 1 = tomorrow ... WEATHER_MAX_DAYS
// partOfDay:   WEATHER_PART_*
// units:       WEATHER_UNITS_*
// info:        filled with WEATHER_INFO_SIZE bytes
//
// Returns info->status (WEATHER_STATUS_OK on success).
//
// The first call can take a few seconds while the adapter
// downloads the forecast. It is then cached by the adapter for
// 30 minutes, so you can call this as often as you like.
// -----------------------------------------------------------
uint8_t ia_getWeather(uint8_t daysFromNow, uint8_t partOfDay, uint8_t units, WeatherInfo *info) {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x20); // ia_getWeather

  hcca_writeByte(daysFromNow);

  hcca_writeByte(partOfDay);

  hcca_writeByte(units);

  hcca_readBytes(0, WEATHER_INFO_SIZE, (uint8_t *)info);

  ia_restoreInterrupts();

  return info->status;
}

// -----------------------------------------------------------
// Get the 16x16 icon for a weather condition (WEATHER_COND_*,
// usually WeatherInfo.conditionId). There are 4 tiles that form
// a square, 8 bytes per tile:
// 0 2
// 1 3
// bgColor is the TMS9918 colour behind the icon (0 = transparent,
// 1 = black ... 15 = white).
//
// *Note: patternBuff and colorBuff must each be 32 bytes long
// -----------------------------------------------------------
void ia_getWeatherIcon(uint8_t conditionId, uint8_t bgColor, uint8_t *patternBuff, uint8_t *colorBuff) {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x21); // ia_getWeatherIcon

  hcca_writeByte(conditionId);

  hcca_writeByte(bgColor);

  hcca_readBytes(0, 32, patternBuff);

  hcca_readBytes(0, 32, colorBuff);

  ia_restoreInterrupts();
}

// -----------------------------------------------------------
// Change the weather location from the NABU.
//
// location:       "Toronto", "Portland, ME", "Paris, France",
//                 a postal code, or "latitude,longitude"
// locationLen:    length of location. 0 = forget the session
//                 location and use the adapter's setting again
// saveToSettings: true saves it in the adapter's settings,
//                 false keeps it until the adapter restarts
// resolvedName:   32 bytes (may be NULL). The place that was
//                 found, e.g. "Portland, Maine"
//
// Returns WEATHER_STATUS_OK, or WEATHER_STATUS_LOCATION_NOT_FOUND
// / WEATHER_STATUS_UNAVAILABLE (the location is left unchanged).
// -----------------------------------------------------------
uint8_t ia_setWeatherLocation(uint8_t *location, uint8_t locationLen, bool saveToSettings, uint8_t *resolvedName) {

  ia_focusInterrupts();

  hcca_writeByte(0xba); // ia_control

  hcca_writeByte(0x22); // ia_setWeatherLocation

  hcca_writeByte(saveToSettings ? 0x01 : 0x00);

  hcca_writeByte(locationLen);

  hcca_writeBytes(0, locationLen, location);

  uint8_t status = hcca_readByte();

  uint8_t readCnt = hcca_readByte();

  if (resolvedName) {

    _ia_readReply(readCnt, resolvedName, 32);
  } else {

    for (; readCnt != 0; readCnt--)
      hcca_readByte();
  }

  ia_restoreInterrupts();

  return status;
}

#endif
