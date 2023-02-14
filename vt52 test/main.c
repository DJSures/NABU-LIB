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
#include "../NABULIB/NABU-LIB.h"

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

  vt_clearScreen();

  puts("");
  puts("VT52 Test by DJ Sures");
  puts("---------------------------");
  puts("This will display a number of ANSI and");
  puts("VT52 tests that are built into the");
  puts("Cloud CP/M BIOS. Normally, a CPM");
  puts("computer would not need a terminal");
  puts("emulator because CPM computers use");
  puts("serial terminals. The NABU uses the");
  puts("built-in VDP, so to run CPM programs");
  puts("correctly, we need a terminal emulator");
  puts("inside the BIOS.");
  puts("");

  prompt();

  // square border
  // -------------------------------------------------------
  vt_clearScreen();
  vt_wrapOff();
  for (uint8_t i = 0; i < 80; i++) {

    vt_setCursor(i, 0);
    printf("*");

    vt_setCursor(i, 23);
    printf("*");
  }

  for (uint8_t i = 0; i < 24; i++) {

    vt_setCursor(0, i);
    printf("*");

    vt_setCursor(79, i);
    printf("*");
  }

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
  printf("^");
  prompt();

  vt_setCursor(10, 5);
  vt_moveCursorDown(2);
  printf("v");
  prompt();

  vt_setCursor(10, 5);
  vt_moveCursorRight(2);
  printf(">");
  prompt();

  vt_setCursor(10, 5);
  vt_moveCursorLeft(2);
  printf("<");
  prompt();

  vt_setCursor(5, 12);
  printf("hidden and shown");
  prompt();

  vt_setCursor(11, 12);
  vt_clearToStartOfLine();
  prompt();
    
  // save & restore cursor position
  // -------------------------------------------------------
  vt_setCursor(18, 2);
  printf("1");
  vt_saveCursorPosition();
  prompt();

  vt_setCursor(20, 2);
  printf("2");
  prompt();

  vt_restoreCursorPosition();
  printf("3");
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

  puts("Bye bye!");
  vt_clearScreen();
}
