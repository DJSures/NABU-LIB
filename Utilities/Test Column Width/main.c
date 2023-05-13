// ************************************************************************
// Skeleton Project
// DJ Sures (c)2023
//
// This is the progress of a game I'm making for the NABU computer.
// There are bugs, and I know about them. So you don't need to tell me :)
// I'm enjoying making it myself, so hope you understand keeping comments to yourself xD
// I shared the code so others can build their own games and use this as a starting point.
// If you make something, please let me know! We're a small NABU community and need to stick together.
//
// Have fun!
// - DJ
// ************************************************************************

#define BIN_TYPE BIN_CPM

// Disable the NABULIB keyboard and use the CPM keyboard
 #define DISABLE_KEYBOARD_INT

// Disable the HCCA if you're not using filesystem or control ia
 #define DISABLE_HCCA_RX_INT

// Disable the VDP if you're making a CPM console app. Console apps can use vt_XXX commands for moving cursor in emulation mode
 #define DISABLE_VDP

// If your program is using graphic mode and using vdp_waitVDPReadyInt(), this can be used during debugging cycle time
// Read the NABULIB.H file for more info on how to use this.
// #define DEBUG_VDP_INT


#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../NABULIB/NABU-LIB.h"

void main() {

  puts("");
  puts("CPM Column Width Display");
  puts("------------------------");
  puts("");
  printf("CPM column width: %u\n\n", _CPM_COLUMN_WIDTH);

  for (uint8_t i = 0; i < _CPM_COLUMN_WIDTH; i++)
    printf("%u", i % 10);

  puts("");
  puts("");
}
