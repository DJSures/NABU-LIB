// ****************************************************************************************
// NABU-LIB IA CONTROL
//
// DJ Sures (c) 2023
// https://nabu.ca
// 
// Last updated on March 22, 2023 (v2023.03.22.00)
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

  // 0xba
  ia_focusInterrupts();

  hcca_writeByte(0xba);

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

  hcca_writeByte(0xbb);

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

  hcca_writeByte(0xbc);

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

  hcca_writeByte(0xbd);

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
  
  // 0xbe
  hcca_writeByte(0xbe);

  hcca_writeByte(parentId);

  hcca_writeByte(childId);
}

#endif
