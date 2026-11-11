// ****************************************************************************************
// NABU-LIB CPM Drive Manager
//
// DJ Sures (c) 2024
// https://nabu.ca
// 
// Last updated on 2024.03.31.00
// 
// Get latest copy and examples from: https://github.com/DJSures/NABU-LIB
//
// Control the CPM Drive manager features of the Internet Adapter
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
//   void     cpm_buildDrive(uint8_t driveLetter, uint8_t *responseMsg)
//              Asks the adapter to build CP/M drive driveLetter (0 = A ... 15 = P).
//              responseMsg (128 bytes): the adapter's reply text.
//
//   void     cpm_extractDrive(uint8_t driveLetter, uint8_t *responseMsg)
//              Asks the adapter to extract CP/M drive driveLetter (0 = A ... 15 = P).
//              responseMsg (128 bytes): the adapter's reply text.
//
//   _cpm_driveCmd() is the internal helper both of them use.
// **********************************************************************************************

#ifndef RETRONET_CPM_FILE_STORE_H
#define RETRONET_CPM_FILE_STORE_H

#include <stdbool.h>
#include <stdlib.h>
#include "NABU-LIB.h"

// -----------------------------------------------------------
// Shared by cpm_buildDrive() and cpm_extractDrive(): sends the
// command and drive, then reads the length-prefixed response.
// The response is clipped to the 128 byte responseMsg buffer
// (extra bytes are read and discarded) so a long message can
// never overrun it. An empty response is safe too.
// -----------------------------------------------------------
void _cpm_driveCmd(uint8_t cmd, uint8_t driveLetter, uint8_t *responseMsg) {

  hcca_writeByte(cmd);

  hcca_writeByte(driveLetter);

  uint8_t readCnt = hcca_readByte();
  uint8_t keep    = (readCnt > 128) ? 128 : readCnt;

  hcca_readBytes(0, keep, responseMsg);

  for (readCnt -= keep; readCnt != 0; readCnt--)
    hcca_readByte();
}

// -----------------------------------------------------------
// Build the specified drive and receive the response message
// Yeah, i'm just receiving a response message becuase this way
// I can release a new IA with new features and not have to update
// this file or existing utilities
//
// driveLetter - (0=A...15=P)
// responseMsg - must be 128 bytes
// -----------------------------------------------------------
void cpm_buildDrive(uint8_t driveLetter, uint8_t *responseMsg) {

  // 0xde
  _cpm_driveCmd(0xde, driveLetter, responseMsg);
}

// -----------------------------------------------------------
// Extract the specified drive and receive the response message
// Yeah, i'm just receiving a response message becuase this way
// I can release a new IA with new features and not have to update
// this file or existing utilities
//
// driveLetter - (0=A...15=P)
// responseMsg - must be 128 bytes
// -----------------------------------------------------------
void cpm_extractDrive(uint8_t driveLetter, uint8_t *responseMsg) {

  // 0xdf
  _cpm_driveCmd(0xdf, driveLetter, responseMsg);
}

#endif
