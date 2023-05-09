#define BIN_TYPE BIN_CPM

#define DISABLE_VDP 

#define DISABLE_KEYBOARD_INT

#include <stdio.h>
#include "../../NABULIB/NABU-LIB.h"

void displayMode() {

  puts("");
  printf("Terminal Emulation Mode: ");

  if (_EMULATION_MODE == 0)
    puts("ADM3a");
  else
    puts("VT52");

  printf("LST Printer Device: ");

  if (_LIST_DEVICE == 0)
    puts("IA");
  else
    puts("LPT");
}

void displayHelp() {

  puts("");
  puts("Change terminal modes:");
  puts(" - ADM3a type: mode 0");
  puts(" - VT52  type: mode 1");
  puts("");
  puts("Change printer output:");
  puts(" - Internet Adapter type: mode 10");
  puts(" - Parallel Port    type: mode 11");
}

uint16_t strToInt(char *str) {

  uint16_t result = 0;

  while ((*str >= '0') && (*str <= '9')) {

      result = (result * 10) + ((*str) - '0');

      str++;
  }

  return result;
}

void main(int argc, char *argv[]) {

  if (argc <= 1) {

    displayMode();

    displayHelp();

    return;
  }

  uint8_t mode = strToInt(argv[1]);

  if (mode == 0) {

    _EMULATION_MODE = 0;

    displayMode();
  } else if (mode == 1) {

    _EMULATION_MODE = 1;

    displayMode();
  } else if (mode == 10) {

    _LIST_DEVICE = 0;

    displayMode();
  } else if (mode == 11) {

    _LIST_DEVICE = 1;

    displayMode();
  } else {

    displayMode();

    displayHelp();
  }
}
