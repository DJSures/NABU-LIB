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
              
void clearScreen() {

  printf("%c[2J", 27);
}

void clearRow() {

  printf("%c[2K", 27);
}

void resetScrollRegion() {

  printf("%c[0;23r", 27);
}

void setScrollRegion(uint8_t t, uint8_t b) {

  printf("%c[%u;%ur", 27, t, b);
}

void setCursor(uint8_t x, uint8_t y) {

  printf("%c[%u;%uH", 27, y, x);
}

void moveCursorUp(uint8_t i) {

  printf("%c[%uA", 27, i);  
}

void moveCursorDown(uint8_t i) {

  printf("%c[%uB", 27, i);  
}

void moveCursorRight(uint8_t i) {

  printf("%c[%uC", 27, i);  
}

void moveCursorLeft(uint8_t i) {

  printf("%c[%uD", 27, i);  
}

void startOfLineDown(uint8_t i) {

  printf("%c[%uE", 27, i);  
}

void startOfLineUp(uint8_t i) {

  printf("%c[%uF", 27, i);  
}

void moveToColumn(uint8_t i) {

  printf("%c[%uG", 27, i);  
}

void saveCursorPosition() {

  printf("%c7", 27);  
}

void restoreCursorPosition() {

  printf("%c8", 27);  
}

void prompt() {

  setCursor(8, 23); 
  printf("Press SPACE for next test"); 
  while (getch() != ' '); 

  setCursor(1, 23);
  clearRow();  
}

void main() {

  initNABULib();

  clearScreen();

setScrollRegion(8, 12);
setCursor(0, 10);

for (uint8_t i = 0; i < 15; i++) 
  printf("%u\r\n", i);

prompt();

return;

  puts("");
  puts("VT100/ANSI Test by DJ Sures");
  puts("---------------------------");
  puts("This will display a number of ANSI and");
  puts("VT100 tests that are built into the");
  puts("Cloud CP/M BIOS. Normally, a CPM");
  puts("computer would not need a terminal");
  puts("emulator because CPM computers use");
  puts("serial terminals. The NABU uses the");
  puts("built-in VDP, so to run CPM programs");
  puts("correctly, we need a terminal emulator");
  puts("inside the BIOS.");
  puts("");

  prompt();

  clearScreen();

  // move to a specific position
  // -------------------------------------------------------
  setCursor(2, 1);
  printf("I am at 2x1");
  prompt();

  // test cursor movements to make a compass
  // -------------------------------------------------------
  setCursor(10, 5);
  moveCursorUp(2);
  printf("^");
  prompt();

  setCursor(10, 5);
  moveCursorDown(2);
  printf("v");
  prompt();

  setCursor(10, 5);
  moveCursorRight(2);
  printf(">");
  prompt();

  setCursor(10, 5);
  moveCursorLeft(2);
  printf("<");
  prompt();

  // test lines down goes to start of line
  // -------------------------------------------------------
  setCursor(5, 12);
  printf("here");
  startOfLineDown(2);
  printf("below @ start");
  prompt();
    
  // test lines down goes to start of line
  // -------------------------------------------------------
  setCursor(10, 12);
  printf("here");
  startOfLineUp(2);
  printf("above @ start");
  prompt();

  // move to column
  // -------------------------------------------------------
  setCursor(14, 5);
  moveToColumn(15);
  printf("column 15");
  prompt();

  // save & restore cursor position
  // -------------------------------------------------------
  setCursor(18, 2);
  printf("1");
  saveCursorPosition();
  prompt();

  setCursor(20, 2);
  printf("2");
  prompt();

  restoreCursorPosition();
  printf("3");

  setCursor(10, 23);
  printf("Press SPACE to exit"); 
  while (getch() != ' ');

  setCursor(1, 23);
  clearRow();  
}
