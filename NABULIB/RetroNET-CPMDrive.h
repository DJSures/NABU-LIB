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

#ifndef RETRONET_CPM_FILE_STORE_H
#define RETRONET_CPM_FILE_STORE_H

#include <stdbool.h>
#include <stdlib.h>
#include "NABU-LIB.h"

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
  hcca_writeByte(0xde);

  hcca_writeByte(driveLetter);

  uint8_t readCnt = hcca_readByte();
  hcca_readBytes(0, readCnt, responseMsg);
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
  hcca_writeByte(0xdf);

  hcca_writeByte(driveLetter);

  uint8_t readCnt = hcca_readByte();
  hcca_readBytes(0, readCnt, responseMsg);
}

#endif
