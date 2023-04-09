// ************************************************************************
// Detect cloud cpm
// DJ Sures (c)2023
//
// Checks if you are running cloud cpm
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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../NABULIB/NABU-LIB.h"

void main() {

  puts("");

  if (isCloudCPM())
    puts("You are running Cloud CPM!");
  else
    puts("Cloud CPM is not detected");

  puts("");
}
