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

#define DISABLE_HCCA_RX_INT
#define DISABLE_VDP
#define DISABLE_KEYBOARD_INT
#define DISABLE_CURSOR

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../NABULIB/NABU-LIB.h"

__at (0x0003) uint8_t IO_BYTE;

uint8_t _ioByte;

void doConsole() {

  puts("Console Output (bits 0, 1): ");

  // -------------------------------------

  printf(" a) ");
  if ((_ioByte & 0b11) == 0b00) {

    vt_reverseVideo();
    printf("*");
  }

  puts("TTY (8251 Hack)");

  vt_normalVideo();

  // -------------------------------------

  printf(" b) ");
  if ((_ioByte & 0b11) == 0b01) {
    vt_reverseVideo();
    printf("*");
  }

  puts("CRT (NABU Video Output)");

  vt_normalVideo();

  // -------------------------------------

  printf(" c) ");
  if ((_ioByte & 0b11) == 0b10) {
    vt_reverseVideo();
    printf("*");
  }

  puts("BAT (Unused)");

  vt_normalVideo();

  // -------------------------------------

  printf(" d) ");
  if ((_ioByte & 0b11) == 0b11) {
    vt_reverseVideo();
    printf("*");
  }

  puts("UC1 (Telnet Server)");

  vt_normalVideo();
}

void doReader() {

  puts("Reader Input (bits 2, 3): ");

  // -------------------------------------

  printf(" e) ");
  if ((_ioByte & 0b1100) == 0b0000) {
    vt_reverseVideo();
    printf("*");
  }

  puts("TTY (NABU Keyboard)");

  vt_normalVideo();

  // -------------------------------------

  printf(" f) ");
  if ((_ioByte & 0b1100) == 0b0100) {
    vt_reverseVideo();
    printf("*");
  }

  puts("PTR (Unused)");

  vt_normalVideo();

  // -------------------------------------

  printf(" g) ");
  if ((_ioByte & 0b1100) == 0b1000) {
    vt_reverseVideo();
    printf("*");
  }

  puts("UR1 (Telnet Server)");

  vt_normalVideo();

  // -------------------------------------

  printf(" h) ");
  if ((_ioByte & 0b1100) == 0b1100) {
    vt_reverseVideo();
    printf("*");
  }

  puts("UR2 (Unused)");

  vt_normalVideo();
}

void main() {

  _ioByte = IO_BYTE;

  while (true) {

    vt_clearScreen();

    vt_reverseVideo();
    puts("\nIO Byte Redirector              DJ Sures\n");
    vt_normalVideo();

    doConsole();
    puts("");
    doReader();
    puts("");
    puts("Options:");
    puts(" s) Save & Quit");
    puts(" q) Quit without save");

    uint8_t c = getchar();

    switch (c) {

      case 's':
        IO_BYTE = _ioByte;
        return;
        break;

      case 'q':
        return;
        break;

      // CONSOLE
      case 'a':
        _ioByte &= 0b11111100;
        _ioByte |= 0b00000000;
        break;
      case 'b':
        _ioByte &= 0b11111100;
        _ioByte |= 0b00000001;
        break;
      case 'c':
        _ioByte &= 0b11111100;
        _ioByte |= 0b00000010;
        break;
      case 'd':
        _ioByte &= 0b11111100;
        _ioByte |= 0b00000011;
        break;

      // READER
      case 'e':
        _ioByte &= 0b11110011;
        _ioByte |= 0b00000000;
        break;
      case 'f':
        _ioByte &= 0b11110011;
        _ioByte |= 0b00000100;
        break;
      case 'g':
        _ioByte &= 0b11110011;
        _ioByte |= 0b00001000;
        break;
      case 'h':
        _ioByte &= 0b11110011;
        _ioByte |= 0b00001100;
        break;
    }
  }
}
