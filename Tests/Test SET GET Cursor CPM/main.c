// Best? -> https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797  

// http://www.braun-home.net/michael/info/misc/VT100_commands.htm#:~:text=On%20color%20systems%20underlined%20characters,)%20*%20ESC%20%5B%200%20K%20erase
// https://www2.ccs.neu.edu/research/gpc/VonaUtils/vona/terminal/vtansi.htm

#define FONT_LM80C

#define BIN_TYPE BIN_CPM

#define DISABLE_VDP 

#define DISABLE_KEYBOARD_INT

#define DISABLE_VDP

#include <stdio.h>
#include <conio.h>
#include "../../NABULIB/NABU-LIB.h"

void prompt() {

  puts("");
  puts("");
  printf("Press SPACE for next test"); 
  while (getch() != ' '); 
}

void main() {

  initNABULib();

  puts("");
  puts("CPM Cursor by DJ Sures");
  puts("---------------------------");
  puts("CPM does not provide access to the");
  puts("cursor position. The BIOS has a RAM");
  puts("location for the cursor that any");
  puts("application can use to GET and SET.");
  puts("This demo will show how we can use");
  puts("the CPM cursor position.");
  puts("");

  prompt();

  puts("");
  puts("");
  puts("");

  // Tell us where the cursor is
  // -------------------------------------------------------
  printf("CPM Cursor is %ux%u\n", cpm_cursor.x, cpm_cursor.y);

  prompt();

  // square in center of the screen
  // -------------------------------------------------------
  for (uint8_t i = 10; i < 30; i++) {

    cpm_cursor.x = i;
    cpm_cursor.y = 5;
    printf("*");

    cpm_cursor.x = i;
    cpm_cursor.y = 19;
    printf("*");
  }

  for (uint8_t i = 5; i < 19; i++) {

    cpm_cursor.x = 10;
    cpm_cursor.y = i;
    printf("*");

    cpm_cursor.x = 30;
    cpm_cursor.y = i;
    printf("*");
  }

  while (getch() != ' '); 

  // -------------------------------------------------------

  printf("Press SPACE to exit"); 
  while (getch() != ' ');

  puts("Bye bye!");
}
