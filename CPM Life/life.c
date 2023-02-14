/*==========================================================================

  life.c

  Simple implementation of Conway's "Game of Life" for CP/M on the
  Z80 Playground. This program uses VT100 or VT52 character codes, 
  on an 80x24 terminal.

  This file is intended to be compiled using the z88dk compiled "zcc".

  Written by Kevin Boone, May 2021. No rights reserved -- do with 
  this as you wish. 

==========================================================================*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Size of the grid. We can't go much bigger the 16x32 on the Z80-PG. */
#define ROWS 16 
#define COLS 32 

/* Where to position the grid on the display. */
#define ROW_OFFSET 2 
#define COL_OFFSET 24 

/* The character to draw for a cell. */
#define CELL_CHAR 'o'

#define TRUE 1
#define FALSE 0
typedef char BOOL;

#define TERM_ANSI 0
#define TERM_VT52 1

//Change this to get VT52 codes.
int term_type = TERM_VT52;

/* Maintain two copies of the grid -- lastgrid is used to calculate which
   character cells need updating; this is to minimize the amount of
   data that has to be sent to the terminal. */
char grid [ROWS][COLS];
char lastgrid [ROWS][COLS];

/* This is a lookup table for numbers up to 80 -- this is the largest
   number the program will output. Using a table takes up a certain
   amount of memory, but it avoids doing number-to-ASCII conversion,
   which is slow. */

static char *nums[81] = 
  {
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", 
  "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", 
  "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", 
  "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", 
  "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", 
  "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", 
  "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", 
  "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", 
  "80"
  };

/** */
void term_putchar (char c)
  {
  bdos (0x02, c);
  }

/** Output a number <= 80 from the lookup table. This is complicated,
    but it saves a heap of arithmetic, and it has to be done for
    every cell displayed. */
void putnum (int c)
  {
  if (c < 10) 
    {
    term_putchar ('0' + c);
    }
  else
    {
    const char *s = nums[c];
    term_putchar (s[0]);
    term_putchar (s[1]);
    }
  }

/** Show cursor */ 
void term_show_cursor (void)
  {
  if (term_type == TERM_VT52)
    ; // Can't do it 
  else
    printf ("\x1B[?25h");
  }

/** Hide cursor */ 
void term_hide_cursor (void)
  {
  if (term_type == TERM_VT52)
    ; // Can't do it 
  else
    printf ("\x1B[?25l");
  }

/** Set cursor position starting at 0,0 */
void term_set_cursor (int row, int col) 
  {
  if (term_type == TERM_VT52)
    printf ("\x1BY%c%c", ' ' + row, ' ' + col);
  else
    {
    term_putchar (27);
    term_putchar ('[');
    putnum (row + 1);
    term_putchar (';');
    putnum (col + 1);
    term_putchar ('H');
    }
  }

/** Clear screen and home */
void term_clear ()
  {
  term_set_cursor (0, 0);
  if (term_type == TERM_VT52)
    printf ("\x1BJ");
  else
    printf ("\x1B[J");
  }

/** Return the last character typed, or zero if there wasn't one. */
int term_peekchar (void)
  {
  int a = bdos (0x06, 0xFF);
  return a;
  }

/** Draw the grid, taking into account which cells are different from
    the last run. */
void life_draw_grid ()
  {
  char *_grid = (char *)grid;
  char *_lastgrid = (char *)lastgrid;
  for (int i = 0; i < ROWS; i++)
    {
    int row_offset = i * COLS;
    for (int j = 0; j < COLS; j++)
      {
      int offset = row_offset + j;
      if (_grid[offset] != _lastgrid[offset])
        {
	term_set_cursor (i + ROW_OFFSET, j + COL_OFFSET);
	if (_grid[offset])
          term_putchar (CELL_CHAR);
        else
          term_putchar (' ');
	}
      }
    }
  memcpy (lastgrid, grid, sizeof (grid));
  }

/** life_clear_display. Clear the display and draw the menu line. */
void life_clear_display (void)
  {
  term_clear();
  term_set_cursor (ROWS + 1 + ROW_OFFSET, 0);
  printf ("                                [R]estart [Q]uit");
  fflush (stdout);
  }

/** life_evolve.
    Calculate which cells live and which die. */
int life_evolve ()
  {
  char *_grid = (char *)grid;
  char *_lastgrid = (char *)lastgrid;
  /* Note that I'm referencing the grid as a one-dimentional array 
     here, so I can precompute the offsets into the array while doing
     the big loop. Every evaluation of the form "grid[row][col]" 
     requires a multiplication and an addition; most of these are 
     unnecessary, since we're working through the grid sequentially. */
  int diffs = 0;
  for (unsigned int i = 0; i < ROWS; i++)
    {
    int row_offset = i * COLS;
    for (unsigned int j = 0; j < COLS; j++)
      {
      int offset = row_offset + j;
      int c = 0;
      if (i > 0)
        {
	int row_above = i - 1;
	if (j > 0        && grid[row_above][j - 1]) c++;
	if (                grid[row_above][j]) c++;
	if (j < COLS - 1 && grid[row_above][j + 1]) c++;
	}
      if (j > 0          && _grid[offset - 1]) c++;
      if (j < COLS - 1   && _grid[offset + 1]) c++;
      if (i < ROWS - 1)
        {
	int row_below = i + 1;
	if (j > 0        && grid[row_below][j - 1]) c++;
	if (                grid[row_below][j]) c++;
	if (j < COLS - 1 && grid[row_below][j + 1]) c++;
	}

      /* This is where the actual simulation is done -- just the next
         dozen lines. Work out whether a cell lives or dies, according
	 to its current status and the number of neighbours it has. */

      if (_grid[offset])
        {
	if (c == 2 || c == 3) 
	  {
	  }
	else
	  _grid[offset] = 0;
	}
      else
        {
	if (c == 3) _grid[offset] = 1;
	}
      if (_grid[offset] != _lastgrid[offset]) diffs++;
      }
    }
  return diffs;
  }

/** life_init_grid */
int life_init_grid ()
  {
  memset (grid, 0, sizeof (grid)); 
  memset (lastgrid, 0, sizeof (lastgrid)); 
  for (int i = 0; i < 60; i++)
    {
    /* The below is mathematically wrong, unless ROWS and COLS are
       powers of 2 (which they might be). */
    int r = rand() % ROWS;
    int c = rand() % COLS;
    grid[r][c] = 1;
    }
  memcpy (lastgrid, grid, sizeof (grid));
  return 0;
  }

/** Get a "random" number from the R register */
static int getrnd (void)
  {
  __asm
  ld a,r
  ld l,a
  ld h,0
  /* Note that zcc expects return values in the HL register. */
  __endasm
  }

/** main */
int main (int argc, char **argv)
  {
  BOOL stop = FALSE;
  srand (getrnd());
  life_init_grid();
  life_clear_display();
  /* Use a running count of program loops to seed the random
     number generator when "R" is pressed. This gives a 16-bit
     seed, which is better than the 8-bit seed we can get from 
     the R register. The first time the simulation runs, we have
     to use the R register. */
  unsigned int cycles = 0;
  int delay = 1;
  /* It's best to hide the cursor -- otherwise we see it scanning
     down the display */
  term_hide_cursor();   while (!stop)
    {
    life_draw_grid ();
    term_set_cursor (ROWS + 1, 0);
    int diffs = life_evolve ();
    if (diffs == 0)
      {
      srand (cycles);
      life_init_grid();
      life_clear_display();
      }
    int c = term_peekchar();
    if (c == 'q') stop = TRUE;
    if (c == 'r') 
      {
      srand (cycles);
      life_init_grid ();
      life_clear_display();
      }
    cycles++;
    }

  term_clear();
  term_show_cursor();
  fflush (stdout);
  return 0;
  }

