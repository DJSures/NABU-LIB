#include <ctype.h>
#include <math.h>
#include "vt100.h"
#include "../NABULIB/NABU-LIB.h"

#define KEY_ESC 0x1b
#define KEY_DEL 0x7f
#define KEY_BELL 0x07

#define STATE(NAME, TERM, EV, ARG) void NAME(struct vt100 *TERM, uint8_t EV, uint16_t ARG)

// states 
enum {
  STATE_IDLE,
  STATE_ESCAPE,
  STATE_COMMAND
};

// events that are passed into states
enum {
  EV_CHAR = 1,
};

#define MAX_COMMAND_ARGS 4
static struct vt100 {

  union flags {

    uint8_t val;

    struct {

      // 0 = cursor remains on last column when it gets there
      // 1 = lines wrap after last column to next line
      bool cursor_wrap : true;
      bool scroll_mode : true;
      bool origin_mode : true;
    };
  } flags;

  // used for cursor save restore
  int16_t saved_cursor_x, saved_cursor_y;

  int16_t scroll_start_row, scroll_end_row;

  // colors used for rendering current characters
  uint8_t back_color, front_color;

  // command arguments that get parsed as they appear in the terminal
  uint8_t narg; uint16_t args[MAX_COMMAND_ARGS];

  // current arg pointer (we use it for parsing) 
  uint8_t carg;

  void (*state)(struct vt100* term, uint8_t ev, uint16_t arg);
  void (*send_response)(char* str);
  void (*ret_state)(struct vt100* term, uint8_t ev, uint16_t arg);
} term;

STATE(_st_idle, term, ev, arg);
STATE(_st_esc_sq_bracket, term, ev, arg);
STATE(_st_esc_question, term, ev, arg);
STATE(_st_esc_hash, term, ev, arg);

void _vt100_reset(void) {

  term.back_color = 0;
  term.front_color = 15;
  term.narg = 0;
  term.state = _st_idle;
  term.ret_state = 0;
  term.scroll_start_row = 0;
  term.scroll_end_row = VT100_HEIGHT;
  term.flags.cursor_wrap = true;
  term.flags.origin_mode = false;

  vdp_setCursor2(0, 0);
}

void _vt100_clearLines(struct vt100* t, uint8_t start_line, uint8_t end_line) {

  vdp_clearRows(start_line, end_line);
}


/// <summary>
/// scrolls the scroll region up (lines > 0) or down (lines < 0)
/// </summary>
void _vt100_scroll(struct vt100* t, int16_t lines) {

  if (!lines)
    return;

  if (lines > 0) {

    for (uint8_t i = 0; i < lines; i++)
      vdp_scrollTextUp(0, 23);

  } else if (lines < 0) {

  }
}

/// <summary>
/// moves the cursor relative to current cursor position and scrolls the screen 
/// </summary>
void _vt100_move(struct vt100* term, int16_t right_left, int16_t bottom_top) {

  // calculate how many lines we need to move down or up if x movement goes outside screen
  int16_t new_x = right_left + vdp_cursor.x;

  if (new_x > VT100_WIDTH) {

    if (term->flags.cursor_wrap) {

      bottom_top += new_x / VT100_WIDTH;
      vdp_cursor.x = new_x % VT100_WIDTH - 1;
    } else {

      vdp_cursor.x = VT100_WIDTH;
    }
  } else if (new_x < 0) {

    bottom_top += new_x / VT100_WIDTH - 1;
    vdp_cursor.x = VT100_WIDTH - (abs(new_x) % VT100_WIDTH) + 1;
  } else {

    vdp_cursor.x = new_x;
  }

  if (bottom_top) {

    int16_t new_y = vdp_cursor.y + bottom_top;
    int16_t to_scroll = 0;

    // bottom margin 23 marks last line as static on 40 line display
    // therefore, we would scroll when new cursor has moved to line 39
    // (or we could use new_y > VT100_HEIGHT here
    // NOTE: new_y >= term->scroll_end_row ## to_scroll = (new_y - term->scroll_end_row) +1
    if (new_y >= term->scroll_end_row) {

      to_scroll = (new_y - term->scroll_end_row) + 1;

      // place cursor back within the scroll region
      vdp_cursor.y = term->scroll_end_row - 1; //new_y - to_scroll; 
    } else if (new_y < term->scroll_start_row) {

      to_scroll = (new_y - term->scroll_start_row);
      vdp_cursor.y = term->scroll_start_row; //new_y - to_scroll; 
    } else {
      // otherwise we move as normal inside the screen
      vdp_cursor.y = new_y;
    }

    _vt100_scroll(term, to_scroll);
  }
}

/// <summary>
/// sends the character to the display and updates cursor position 
/// </summary>
void _vt100_putc(struct vt100* t, uint8_t ch) {

  if (ch < 0x20 || ch > 0x7e) {

    //static const char hex[] = "0123456789abcdef";

    //_vt100_putc(t, '0');
    //_vt100_putc(t, 'x');
    //_vt100_putc(t, hex[((ch & 0xf0) >> 4)]);
    //_vt100_putc(t, hex[(ch & 0x0f)]);

    return;
  }

  vdp_writeCharAtLocation(vdp_cursor.x, vdp_cursor.y, ch);

  // move cursor right
  _vt100_move(t, 1, 0);
}

/// <summary>
/// Put null terminated string on the screen
/// </summary>
void vt100_puts(const char* str) {

  while (*str)
    vt100_putc(*str++);
}

STATE(_st_command_arg, term, ev, arg) {

  switch (ev) {

  case EV_CHAR:

    if (isdigit(arg)) { // a digit argument

      term->args[term->narg] = term->args[term->narg] * 10 + (arg - '0');
    } else if (arg == ';') { // separator

      term->narg++;
    } else { // no more arguments

      // go back to command state 
      term->narg++;

      if (term->ret_state) {

        term->state = term->ret_state;
      } else {

        term->state = _st_idle;
      }

      // execute next state as well because we have already consumed a char!
      term->state(term, ev, arg);
    }

    break;
  }
}

STATE(_st_esc_sq_bracket, term, ev, arg) {

  switch (ev) {

  case EV_CHAR:

    if (isdigit(arg)) {

      // start of an argument

      term->ret_state = _st_esc_sq_bracket;
      _st_command_arg(term, ev, arg);
      term->state = _st_command_arg;
    } else if (arg == ';') {

      // arg separator. 

      // skip. And also stay in the command state
    } else {
      // otherwise we execute the command and go back to idle

      switch (arg) {

      case 'A':
      {
        // move cursor up (cursor stops at top margin)

        int n = (term->narg > 0) ? term->args[0] : 1;

        vdp_cursor.y -= n;

        term->state = _st_idle;
        break;
      }
      case 'B':
      {
        // cursor down (cursor stops at bottom margin)

        int n = (term->narg > 0) ? term->args[0] : 1;

        vdp_cursor.y += n;

        if (vdp_cursor.y > VT100_HEIGHT)
          vdp_cursor.y = VT100_HEIGHT;

        term->state = _st_idle;

        break;
      }
      case 'C':
      {
        // cursor right (cursor stops at right margin)

        int n = (term->narg > 0) ? term->args[0] : 1;

        vdp_cursor.x += n;

        if (vdp_cursor.x > VT100_WIDTH)
          vdp_cursor.x = VT100_WIDTH;

        term->state = _st_idle;
        break;
      }
      case 'D':
      {
        // cursor left

        int n = (term->narg > 0) ? term->args[0] : 1;

        vdp_cursor.x -= n;

        term->state = _st_idle;

        break;
      }
      case 'f':
      case 'H':
      {
        // move cursor to position (default 0;0)
        // cursor stops at respective margins
        vdp_cursor.x = (term->narg >= 1) ? (term->args[1] - 1) : 0;
        vdp_cursor.y = (term->narg == 2) ? (term->args[0] - 1) : 0;

        if (term->flags.origin_mode) {

          vdp_cursor.y += term->scroll_start_row;

          if (vdp_cursor.y >= term->scroll_end_row)
            vdp_cursor.y = term->scroll_end_row - 1;
        }

        if (vdp_cursor.x > VT100_WIDTH)
          vdp_cursor.x = VT100_WIDTH;

        if (vdp_cursor.y > VT100_HEIGHT)
          vdp_cursor.y = VT100_HEIGHT;

        term->state = _st_idle;
        break;
      }
      case 'J':
      {
        // clear screen from cursor up or down

        if (term->narg == 0 || (term->narg == 1 && term->args[0] == 0)) {

          // clear down to the bottom of screen (including cursor)
          _vt100_clearLines(term, vdp_cursor.y, VT100_HEIGHT);
        } else if (term->narg == 1 && term->args[0] == 1) {

          // clear top of screen to current line (including cursor)
          _vt100_clearLines(term, 0, vdp_cursor.y);
        } else if (term->narg == 1 && term->args[0] == 2) {

          // clear whole screen
          _vt100_clearLines(term, 0, VT100_HEIGHT);
        }

        term->state = _st_idle;

        break;
      }
      case 'K':
      {
        // clear line from cursor right/left

        if (term->narg == 0 || (term->narg == 1 && term->args[0] == 0)) {

          // clear to end of line (to \n or to edge?)
          // including cursor
          // ili9340_fillRect(x, y, VT100_SCREEN_WIDTH - x, VT100_CHAR_HEIGHT, term->back_color);
        } else if (term->narg == 1 && term->args[0] == 1) {

          // clear from left to current cursor position
          // ili9340_fillRect(0, y, x + VT100_CHAR_WIDTH, VT100_CHAR_HEIGHT, term->back_color);
        } else if (term->narg == 1 && term->args[0] == 2) {

          // clear whole current line
          // ili9340_fillRect(0, y, VT100_SCREEN_WIDTH, VT100_CHAR_HEIGHT, term->back_color);
        }

        term->state = _st_idle;

        break;
      }

      case 'L': // insert lines (args[0] = number of lines)
      case 'M': // delete lines (args[0] = number of lines)

        if (term->args[0] > 0)
          vdp_clearRows(vdp_cursor.y, vdp_cursor.y + term->args[0]);

        term->state = _st_idle;
        break;
      case 'P':
      {
        // delete characters args[0] or 1 in front of cursor

        int n = ((term->narg > 0) ? term->args[0] : 1);

        _vt100_move(term, -n, 0);

        for (int c = 0; c < n; c++)
          _vt100_putc(term, ' ');

        term->state = _st_idle;

        break;
      }
      case 'c':
      {
        // query device code

        term->send_response("\e[?1;0c");
        term->state = _st_idle;

        break;
      }
      case 'x':
      {

        term->state = _st_idle;

        break;
      }
      case 's':
      {
        // save cursor pos

        term->saved_cursor_x = vdp_cursor.x;
        term->saved_cursor_y = vdp_cursor.y;
        term->state = _st_idle;

        break;
      }
      case 'u':
      {
        // restore cursor pos

        vdp_cursor.x = term->saved_cursor_x;
        vdp_cursor.y = term->saved_cursor_y;

        //_vt100_moveCursor(term, term->saved_cursor_x, term->saved_cursor_y);
        term->state = _st_idle;

        break;
      }
      case 'h':
      case 'l':
      {

        term->state = _st_idle;

        break;
      }

      case 'g':
      {

        term->state = _st_idle;

        break;
      }
      case 'm':
      {
        // sets colors. Accepts up to 3 args
        // [m means reset the colors to default

        if (!term->narg) {

          term->front_color = 15;
          term->back_color = 0;
        }

        while (term->narg) {

          term->narg--;
          int n = term->args[term->narg];

          static const uint8_t colors[] = {
            0, // black
            9, // red
            3, // green
            11, // yellow
            5, // blue
            13, // magenta
            7, // cyan
            15 // white
          };

          if (n == 0) {

            // all attributes off
            term->front_color = 15;
            term->back_color = 0;
          }

          if (n >= 30 && n < 38) {

            // fg colors
            term->front_color = colors[n - 30];

            //        ili9340_setFrontColor(term->front_color);
          } else if (n >= 40 && n < 48) {

            term->back_color = colors[n - 40];

            //      ili9340_setBackColor(term->back_color);
          }
        }
        term->state = _st_idle;

        break;
      }

      case '@':

        // Insert Characters          
        term->state = _st_idle;

        break;
      case 'r':

        // Set scroll region (top and bottom margins)

        // the top value is first row of scroll region
        // the bottom value is the first row of static region after scroll
        if (term->narg == 2 && term->args[0] < term->args[1]) {

          // [1;40r means scroll region between 8 and 312
          // bottom margin is 320 - (40 - 1) * 8 = 8 pix
          term->scroll_start_row = term->args[0] - 1;
          term->scroll_end_row = term->args[1] - 1;
          uint8_t top_margin = term->scroll_start_row;
          uint8_t bottom_margin = VT100_SCREEN_HEIGHT - term->scroll_end_row;

          //  ili9340_setScrollMargins(top_margin, bottom_margin);
            //ili9340_setScrollStart(0); // reset scroll 
        }

        term->state = _st_idle;

        break;
      case 'i':
        // Printing  
      case 'y':
        // self test modes..
      case '=':
      {
        // argument follows... 
        // term->state = _st_screen_mode;

        term->state = _st_idle;

        break;
      }
      case '?':
        // '[?' escape mode

        term->state = _st_esc_question;

        break;
      default:
      {
        // unknown sequence

        term->state = _st_idle;
        break;
      }
      }
    }
    break;
  default:
    // switch (ev)
    // for all other events restore normal mode
    term->state = _st_idle;
  }
}

STATE(_st_esc_question, term, ev, arg) {

  // DEC mode commands
  switch (ev) {
  case EV_CHAR:
  {

    if (isdigit(arg)) {

      // start of an argument
      term->ret_state = _st_esc_question;
      _st_command_arg(term, ev, arg);
      term->state = _st_command_arg;
    } else if (arg == ';') {

      // arg separator. 
      // skip. And also stay in the command state
    } else {

      switch (arg) {

      case 'l':
        // dec mode: OFF (arg[0] = function)
      case 'h':
      {
        // dec mode: ON (arg[0] = function)
        switch (term->args[0]) {

        case 1:
        {
          // cursor keys mode
          // h = esc 0 A for cursor up
          // l = cursor keys send ansi commands
          break;
        }
        case 2:
        {
          // ansi / vt52
          // h = ansi mode
          // l = vt52 mode
          break;
        }
        case 3:
        {
          // h = 132 chars per line
          // l = 80 chars per line
          break;
        }
        case 4:
        {
          // h = smooth scroll
          // l = jump scroll
          break;
        }
        case 5:
        {
          // h = black on white bg
          // l = white on black bg
          break;
        }
        case 6:
        {
          // h = cursor relative to scroll region
          // l = cursor independent of scroll region
          term->flags.origin_mode = (arg == 'h') ? 1 : 0;
          break;
        }
        case 7:
        {
          // h = new line after last column
          // l = cursor stays at the end of line
          term->flags.cursor_wrap = (arg == 'h') ? 1 : 0;
          break;
        }
        case 8:
        {
          // h = keys will auto repeat
          // l = keys do not auto repeat when held down
          break;
        }
        case 9:
        {
          // h = display interlaced
          // l = display not interlaced
          break;
        }
        // 10-38 - all quite DEC speciffic commands so omitted here
        }
        term->state = _st_idle;
        break;
      }
      case 'i':
        // Printing
      case 'n':
        // Request printer status 
      default:
        term->state = _st_idle;
        break;
      }
      term->state = _st_idle;
    }
  }
  }
}

STATE(_st_esc_left_br, term, ev, arg) {

  switch (ev) {

  case EV_CHAR:

    switch (arg) {

    case 'A':
    case 'B':
      // translation map command?
    case '0':
    case 'O':
      // another translation map command?
      term->state = _st_idle;
      break;
    default:
      term->state = _st_idle;
    }
  }
}

STATE(_st_esc_right_br, term, ev, arg) {

  switch (ev) {

  case EV_CHAR:

    switch (arg) {

    case 'A':
    case 'B':
      // translation map command?
    case '0':
    case 'O':
      // another translation map command?
      term->state = _st_idle;
      break;
    default:
      term->state = _st_idle;
    }
  }
}

STATE(_st_esc_hash, term, ev, arg) {

  switch (ev) {

  case EV_CHAR:

    switch (arg) {

    case '8':
      // self test: fill the screen with 'E'

      term->state = _st_idle;
      break;
    default:
      term->state = _st_idle;
    }
  }
}

STATE(_st_escape, term, ev, arg) {

  switch (ev) {

  case EV_CHAR:

#define CLEAR_ARGS \
				{ term->narg = 0;\
				for(int c = 0; c < MAX_COMMAND_ARGS; c++)\
					term->args[c] = 0; }\

    switch (arg) {

    case '[':
      // command
      // prepare command state and switch to it
      CLEAR_ARGS;
      term->state = _st_esc_sq_bracket;
      break;
    case '(':
      /* ESC ( */
      CLEAR_ARGS;
      term->state = _st_esc_left_br;
      break;
    case ')':
      /* ESC ) */
      CLEAR_ARGS;
      term->state = _st_esc_right_br;
      break;
    case '#':
      // ESC # 
      CLEAR_ARGS;
      term->state = _st_esc_hash;
      break;
    case 'P':
      //ESC P (DCS, Device Control String)
      term->state = _st_idle;
      break;
    case 'D':
      // moves cursor down one line and scrolls if necessary
      // move cursor down one line and scroll window if at bottom line
      _vt100_move(term, 0, 1);
      term->state = _st_idle;
      break;
    case 'M':
      // Cursor up
      // move cursor up one line and scroll window if at top line
      _vt100_move(term, 0, -1);
      term->state = _st_idle;
      break;
    case 'E':
      // next line
      // same as '\r\n'
      _vt100_move(term, 0, 1);
      vdp_cursor.x = 0;
      term->state = _st_idle;
      break;
    case '7':
      // Save attributes and cursor position  
    case 's':
      term->saved_cursor_x = vdp_cursor.x;
      term->saved_cursor_y = vdp_cursor.y;
      term->state = _st_idle;
      break;
    case '8':
      // Restore them  
    case 'u':
      vdp_cursor.x = term->saved_cursor_x;
      vdp_cursor.y = term->saved_cursor_y;
      term->state = _st_idle;
      break;
    case '=':
      // Keypad into applications mode 
      term->state = _st_idle;
      break;
    case '>':
      // Keypad into numeric mode   
      term->state = _st_idle;
      break;
    case 'Z':
      // Report terminal type 
      // vt 100 response
      term->send_response("\033[?1;0c");

      // unknown terminal     
      //out("\033[?c");
      term->state = _st_idle;
      break;
    case 'c':
      // Reset terminal to initial state 
      _vt100_reset();
      term->state = _st_idle;
      break;
    case 'H':
      // Set tab in current position 
    case 'N':
      // G2 character set for next character only  
    case 'O':
      // G3 "               "     
    case '<':
      // Exit vt52 mode
      // ignore
      term->state = _st_idle;
      break;
    case KEY_ESC:
      // marks start of next escape sequence
      // stay in escape state
      break;
    default:
      // unknown sequence - return to normal mode
      term->state = _st_idle;
      break;

    }
#undef CLEAR_ARGS
    break;

  default:

    // for all other events restore normal mode
    term->state = _st_idle;
  }
}

STATE(_st_idle, term, ev, arg) {

  switch (ev) {

  case EV_CHAR:

    switch (arg) {

    case 5:
      // AnswerBack for vt100's  
      term->send_response("X"); // should send SCCS_ID?
      break;
    case '\n':
      // new line
      _vt100_move(term, 0, 1);
      vdp_cursor.x = 0;
      //_vt100_moveCursor(term, 0, vdp_cursor.y + 1);
      // do scrolling here! 
      break;
    case '\r':
      // carrage return (0x0d)
      vdp_cursor.x = 0;
      //_vt100_move(term, 0, 1);
      //_vt100_moveCursor(term, 0, vdp_cursor.y); 
      break;
    case '\b':
      // backspace 0x08
      //_vt100_move(term, -1, 0);
      // Backspace is handled in main.c
      break;
    case KEY_DEL:
      // del - delete character under cursor
      // Problem: with current implementation, we can't move the rest of line
      // to the left as is the proper behavior of the delete character
      _vt100_putc(term, ' ');
      _vt100_move(term, -1, 0);
      break;
    case '\t':
    {
      // tab
      // tab fills characters on the line until we reach a multiple of tab_stop
      int to_put = 4 - (vdp_cursor.x % 4);
      while (to_put--) _vt100_putc(term, ' ');
      break;
    }
    case KEY_BELL:
      playNoteDelay(0, 40, 5);

      break;
    case KEY_ESC:
      // escape
      term->state = _st_escape;
      break;
    default:
      _vt100_putc(term, arg);
      break;
    }
    break;
  default:

    break;
  }
}

/// <summary>
/// Initialize the vt100 render engine.
/// 
/// - parameter is a function that will be used for state machine to respond
///   to the host.
/// </summary>
void vt100_init(void (*send_response)(char* str)) {

  term.send_response = send_response;

  _vt100_reset();
}

/// <summary>
/// Put a character into the vt100 render engine, it will be processed
/// and displayed on the vdp.
/// </summary>
void vt100_putc(uint8_t c) {

  /*
  char *buffer = 0;

  switch(c){
    case KEY_UP:         buffer="\e[A";    break;
    case KEY_DOWN:       buffer="\e[B";    break;
    case KEY_RIGHT:      buffer="\e[C";    break;
    case KEY_LEFT:       buffer="\e[D";    break;
    case KEY_BACKSPACE:  buffer="\b";      break;
    case KEY_IC:         buffer="\e[2~";   break;
    case KEY_DC:         buffer="\e[3~";   break;
    case KEY_HOME:       buffer="\e[7~";   break;
    case KEY_END:        buffer="\e[8~";   break;
    case KEY_PPAGE:      buffer="\e[5~";   break;
    case KEY_NPAGE:      buffer="\e[6~";   break;
    case KEY_SUSPEND:    buffer="\x1A";    break;      // ctrl-z
    case KEY_F(1):       buffer="\e[[A";   break;
    case KEY_F(2):       buffer="\e[[B";   break;
    case KEY_F(3):       buffer="\e[[C";   break;
    case KEY_F(4):       buffer="\e[[D";   break;
    case KEY_F(5):       buffer="\e[[E";   break;
    case KEY_F(6):       buffer="\e[17~";  break;
    case KEY_F(7):       buffer="\e[18~";  break;
    case KEY_F(8):       buffer="\e[19~";  break;
    case KEY_F(9):       buffer="\e[20~";  break;
    case KEY_F(10):      buffer="\e[21~";  break;
  }

  if(buffer){

    while(*buffer)
      term.state(&term, EV_CHAR, *buffer++);
  } else {

    term.state(&term, EV_CHAR, 0x0000 | c);
  }
  */

  term.state(&term, EV_CHAR, 0x0000 | c);
}