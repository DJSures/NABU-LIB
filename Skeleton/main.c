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

// Disable stdio. Useful if not using DISABLE_VDP, and therefore you're using only the CPM built-in functions.
//#pragma output nostreams

// No FILEIO. This disables all ability to read/write CPM files
//#pragma output nofileio

// Strip the MS-DOS protection header (recommended because ain't nobody got time for that)
//#pragma output noprotectmsdos

// Do not insert the file redirection option while parsing the command line arguments (useless if “nostreams” is set)
//#pragma output noredir

// No global variables for graphics (recommended because you'll be using NABULIB for graphics)
//#pragma output nogfxglobals

// Disable the cursor for text input. If not using text mode, disable the cursor
//#define DISABLE_CURSOR

// Disable the NABULIB keyboard and use the CPM keyboard
// #define DISABLE_KEYBOARD_INT

// Disable the HCCA if you're not using filesystem or control ia
// #define DISABLE_HCCA_RX_INT

// Disable the VDP if you're making a CPM console app. Console apps can use vt_XXX commands for moving cursor in emulation mode
// #define DISABLE_VDP



#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../NABULIB/NABU-LIB.h"
//#include "../NABULIB/RetroNET-FileStore.h"
//#include "patterns.c"
//#include "../NABULIB/NabuTracker.h"

void main() {

  initNABULib();  
}
