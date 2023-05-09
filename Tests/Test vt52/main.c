// Best? -> https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797  

// http://www.braun-home.net/michael/info/misc/VT100_commands.htm#:~:text=On%20color%20systems%20underlined%20characters,)%20*%20ESC%20%5B%200%20K%20erase
// https://www2.ccs.neu.edu/research/gpc/VonaUtils/vona/terminal/vtansi.htm

#define FONT_LM80C

#define BIN_TYPE BIN_CPM

#define DISABLE_VDP 

#define DISABLE_CURSOR

#define DISABLE_KEYBOARD_INT

#include <stdio.h>
#include <conio.h>
#include "../../NABULIB/NABU-LIB.h"

void prompt() {

  vt_setCursor(8, 23); 
  vt_clearLine();
  vt_setCursor(8, 23); 
  printf("Press SPACE for next test"); 
  while (getch() != ' '); 

  vt_setCursor(1, 23);
  vt_clearLine();
}

void main() {

  initNABULib();

  int8_t t = -1;
  do {

    vt_clearScreen();

    puts("");
    puts("Terminal Test by DJ Sures");
    puts("-------------------------");
    puts("This will display a number of terminal");
    puts("tests that are built into the Cloud");
    puts("CP/M BIOS. Normally, a CPM computer");
    puts("would not need a terminal emulator");
    puts("because CPM computers use serial ");
    puts("dumb terminals. The NABU uses the");
    puts("built-in VDP, so to run CPM programs");
    puts("correctly, we need a terminal emulator");
    puts("inside the BIOS.");

    puts("");
    puts("");

    printf("Current BIOS Mode: ");

    if (_EMULATION_MODE == 0)
      puts("ADM3a");
    else
      puts("VT52");

    puts("");

    puts("Select emulation mode to test:");
    puts(" 1) ADM3a");
    puts(" 2) VT52");
    puts(" 3) Exit");

    t = getchar() - '1';

    if (t == 2) {

      puts("Exiting...");

      return;
    }

  } while (t < 0 || t > 1);

  _EMULATION_MODE = t;

  // square border
  // -------------------------------------------------------
  vt_clearScreen();
  vt_wrapOff();
  for (uint8_t i = 0; i < 80; i++) {

    vt_setCursor(i, 0);
    putchar('*');

    vt_setCursor(i, 23);
    putchar('*');
  }

  for (uint8_t i = 0; i < 24; i++) {

    vt_setCursor(0, i);
    putchar('*');

    vt_setCursor(79, i);
    putchar('*');
  }

  vt_setCursor(2, 8);
  printf("Emulation mode: ");
  if (_EMULATION_MODE == 0)
    puts("ADM3a");
  else
    puts("VT52");

  vt_setCursor(2, 10);
  puts("There is a border around the entire 80 column screen.");
  vt_setCursor(2, 11);
  puts("Use <<< (page up) and >>> (page down) keys to scroll the virtual screen");
  
  vt_setCursor(2, 12);
  vt_reverseVideo();
  puts("This is inverted text!    and blank spaces");
  vt_normalVideo();

  vt_wrapOn();
  vt_setCursor(8, 14);  
  printf("Press SPACE for next test"); 
  while (getch() != ' '); 
  vt_clearScreen();

  // move to a specific position
  // -------------------------------------------------------
  vt_setCursor(2, 1);
  printf("I am at 2x1");
  prompt();

  // test cursor movements to make a compass
  // -------------------------------------------------------
  vt_setCursor(10, 5);
  vt_moveCursorUp(2);
  putchar('^');
  prompt();

  vt_setCursor(10, 5);
  vt_moveCursorDown(2);
  putchar('v');
  prompt();

  vt_setCursor(10, 5);
  vt_moveCursorRight(2);
  putchar('>');
  prompt();

  vt_setCursor(10, 5);
  vt_moveCursorLeft(2);
  putchar('<');
  prompt();

  vt_setCursor(5, 12);
  printf("hidden and shown");
  prompt();

  vt_setCursor(11, 12);
  vt_clearToStartOfLine();
  prompt();
    
  // save & restore cursor position
  // -------------------------------------------------------
  vt_setCursor(15, 2);
  puts("Testing Save Restore cursor");

  vt_setCursor(15, 3);
  putchar('1');
  vt_saveCursorPosition();
  prompt();

  vt_setCursor(17, 3);
  putchar('2');
  prompt();

  vt_restoreCursorPosition();
  putchar('3');
  prompt();

  // clear end of screen
  // -------------------------------------------------------
  vt_clearScreen();
  for (uint8_t i = 0; i < 23; i++)
    puts("clear the end of screen from the center");
  prompt();
  
  vt_setCursor(0, 12);
  vt_clearToEndOfScreen();
  prompt();

  // clean line from cursor
  // -------------------------------------------------------
  vt_clearScreen();
  vt_setCursor(0, 10);
  printf("everything after X will disappear");
  prompt();
  vt_setCursor(18, 10);
  vt_clearToEndOfLine();
  prompt();

  // clear rows above cursor
  // -------------------------------------------------------
  vt_clearScreen();
  for (uint8_t i = 0; i < 23; i++)
    puts("clear the top of screen from the center");
  prompt();
  
  vt_setCursor(0, 12);
  vt_clearToStartOfScreen();
  prompt();

  // insert line
  // -------------------------------------------------------
  vt_clearScreen();
  for (uint8_t i = 0; i < 23; i++)
    printf("%u insert line at center shift down\n", i);
  prompt();
  
  vt_setCursor(0, 12);
  vt_insertLine();
  prompt();

  // -------------------------------------------------------

  vt_setCursor(10, 23);
  printf("Press SPACE to exit");
  while (getch() != ' ');

  puts("\n\n");

  puts("Bye bye!");

  if (_EMULATION_MODE != 0) {

    puts("Returning BIOS emulation mode to ADM3a");
    _EMULATION_MODE = 0;
  }
}
