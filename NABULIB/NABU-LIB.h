// ****************************************************************************************
// NABU-LIB C Library
// DJ Sures (c) 2022
// https://nabu.ca
// 
// Last updated on December 26, 2022 (v2022.12.26.00)
// 
// This is a z88dk C library for the NABU Personal Computer. This is a large library
// with many functions for the VDP, Sound, HCCA, and Keyboard. The functions are split
// into sections based on the peripheral.  
// 
// Many of the functions have been optimized for performance with either __fastcall or 
// inlining.
// 
// Read the summaries below for details of each function in this file.
// 
// **********************************************************************************************


#ifndef NABU_H
#define NABU_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <im2.h>
#include <z80.h>
#include "patterns.h"


// **************************************************************************
// Special Function Registers
// --------------------------
// 
// These are special function register definitions where you can read and write
// values to the ports. using these compiles to one line of assembly, OUT or IN,
// which is very effecient. 
// 
// *Note: For example, to send a value to PORTA
// 
// IO_AYLATCH = IOPORTA;
// IO_AYDATA = 0x04;
// 
// *Note: For example to retrieve a value from PORTB
// 
// IO_LATCH = IOPORTB;
// return IO_AYDATA;
// 
// **************************************************************************
__sfr __at 0xA0 IO_VDPDATA;
__sfr __at 0xA1 IO_VDPLATCH;

__sfr __at 0x40 IO_AYDATA;
__sfr __at 0x41 IO_AYLATCH;

__sfr __at 0x80 IO_HCCA;

__sfr __at 0x90 IO_KEYBOARD;
__sfr __at 0x91 IO_KEYBOARD_STATUS;

#define IOPORTA  0x0e
#define IOPORTB  0x0f


/// <summary>
/// HCCA RX Input buffer (256 bytes)
/// </summary>
volatile uint8_t _rxBuffer[256];

/// <summary>
/// HCCA RX read position
/// </summary>
uint8_t _rxReadPos = 0;

/// <summary>
/// HCCA RX write position (used in interrupt)
/// </summary>
volatile uint8_t _rxWritePos = 0;


// **************************************************************************
// VDP Variables
// -------------
// 
// These are variables and definitions for the VDP functions, such as
// cursor location, screen width/height for text calculations, and 
// the text buffer.
// 
// **************************************************************************

#define VDP_MODE 11
#define VDP_CSW 10
#define VDP_CSR 9
#define VDP_RESET 8
#define VDP_R1_IE 0x20
#define VDP_R1_M1 0x10
#define VDP_R1_M2 0x08
#define VDP_R1_SIZE 0x02
#define VDP_R1_MAG 0x01

/// <summary>
/// Double buffer for text mode scrolling.
/// This is because reading and writing the VDP vram is very slow
/// and it is quicker to keep 960 bytes in RAM to double buffer
/// the text mode.
/// </summary>
uint8_t _vdp_textBuffer[24 * 40]; // row * col = 960 bytes

/// <summary>
/// The current position of the cursor used by textmode
/// functions. Such as vdp_write() and vdp_print() and
/// vdp_newLine().
/// </summary>
struct {
  uint8_t x;
  uint8_t y;
} vdp_cursor;

uint16_t       _vdp_sprite_attribute_table;
uint16_t       _vdp_sprite_pattern_table;
uint8_t        _vdp_sprite_size_sel;      // 0: 8x8 sprites 1: 16x16 sprites
uint16_t       _vdp_name_table;
uint16_t       _vdp_color_table_size = 0;
uint16_t       _vdp_color_table;
uint16_t       _vdp_pattern_table;
uint8_t        _vdp_crsr_max_x = 31;      // max number of colums (Overwritten in Text mode)
const uint8_t  _vdp_crsr_max_y = 23;      // max number of rows
uint8_t        _vdp_mode;
uint8_t        _vdp_fgcolor;
uint8_t        _vdp_bgcolor;


/// <summary>
/// Colors for the VDP fgColor or bgColor settings
/// </summary>
enum VDP_COLORS {
  VDP_TRANSPARENT = 0,
  VDP_BLACK = 1,
  VDP_MED_GREEN = 2,
  VDP_LIGHT_GREEN = 3,
  VDP_DARK_BLUE = 4,
  VDP_LIGHT_BLUE = 5,
  VDP_DARK_RED = 6,
  VDP_CYAN = 7,
  VDP_MED_RED = 8,
  VDP_LIGHT_RED = 9,
  VDP_DARK_YELLOW = 10,
  VDP_LIGHT_YELLOW = 11,
  VDP_DARK_GREEN = 12,
  VDP_MAGENTA = 13,
  VDP_GRAY = 14,
  VDP_WHITE = 15
};


/// <summary>
/// Available graphic/text modes for the VDP
/// </summary>
enum VDP_MODES {
  VDP_MODE_G1 = 0,
  VDP_MODE_G2 = 1,
  VDP_MODE_MULTICOLOR = 2,
  VDP_MODE_TEXT = 3,
};


/// <summary>
/// The direction to move the cursor that some VDP functions will accept.
/// </summary>
enum VDP_CURSOR_DIR {
  VDP_CURSOR_UP = 0,
  VDP_CURSOR_DOWN = 1,
  VDP_CURSOR_LEFT = 2,
  VDP_CURSOR_RIGHT = 3
};


// ---------------------------------------------
// Coincidence flag, set when sprites overlap
#define VDP_FLAG_COIN 0x20 


// 5th sprite flag, set when more than 4 sprite per line 
#define VDP_FLAG_S5 0x40  


/// <summary>
/// 4-Byte record defining sprite attributes
/// </summary>
typedef struct {
  uint8_t x; //Sprite X position
  uint8_t y; //Sprite Y position
  uint8_t name_ptr; //Sprite name in pattern table
  uint8_t ecclr; //Bit 7: Early clock bit, bit 3:0 color
} Sprite_attributes;


// VDP Status
#define VDP_OK 0
#define VDP_ERROR 1




// **************************************************************************
// Audio Notes
// -----------
// 
// For performance, the notes are split into two unsigned 8-bit arrays rather 
// than having to bitshift a 16-bit value for each note.
// When using the playNoteDelay(), pass the index value which is the first number
// in the comment below. The equilevant midi note is also displayed, but that's
// not the value you will pass to playNoteDelay(). 
// 
// 8, //   8, midi note: 44 (�G#2/Ab2�)
//         ^ 
//         this value you pass to playNoteDelay()
// 
// 
// **************************************************************************

const uint8_t _NOTES_COURSE[] = {
13, //  0, midi note: 36 (C2)
12, //  1, midi note: 37 (�C#2/Db2�)
11, //  2, midi note: 38 (D2)
11, //  3, midi note: 39 (�D#2/Eb2�)
10, //  4, midi note: 40 (E2)
10, //  5, midi note: 41 (F2)
9, //  6, midi note: 42 (�F#2/Gb2�)
8, //  7, midi note: 43 (G2)
8, //  8, midi note: 44 (�G#2/Ab2�)
7, //  9, midi note: 45 (A2)
7, //  10, midi note: 46 (�A#2/Bb2�)
7, //  11, midi note: 47 (B2)
6, //  12, midi note: 48 (C3)
6, //  13, midi note: 49 (�C#3/Db3�)
5, //  14, midi note: 50 (D3)
5, //  15, midi note: 51 (�D#3/Eb3�)
5, //  16, midi note: 52 (E3)
5, //  17, midi note: 53 (F3)
4, //  18, midi note: 54 (�F#3/Gb3�)
4, //  19, midi note: 55 (G3)
4, //  20, midi note: 56 (�G#3/Ab3�)
3, //  21, midi note: 57 (A3)
3, //  22, midi note: 58 (�A#3/Bb3�)
3, //  23, midi note: 59 (B3)
3, //  24, midi note: 60 (C4)
3, //  25, midi note: 61 (�C#4/Db4�)
2, //  26, midi note: 62 (D4)
2, //  27, midi note: 63 (�D#4/Eb4�)
2, //  28, midi note: 64 (E4)
2, //  29, midi note: 65 (F4)
2, //  30, midi note: 66 (�F#4/Gb4�)
2, //  31, midi note: 67 (G4)
2, //  32, midi note: 68 (�G#4/Ab4�)
1, //  33, midi note: 69 (A4)
1, //  34, midi note: 70 (�A#4/Bb4�)
1, //  35, midi note: 71 (B4)
1, //  36, midi note: 72 (C5)
1, //  37, midi note: 73 (�C#5/Db5�)
1, //  38, midi note: 74 (D5)
1, //  39, midi note: 75 (�D#5/Eb5�)
1, //  40, midi note: 76 (E5)
1, //  41, midi note: 77 (F5)
1, //  42, midi note: 78 (�F#5/Gb5�)
1, //  43, midi note: 79 (G5)
1, //  44, midi note: 80 (�G#5/Ab5�)
0, //  45, midi note: 81 (A5)
0, //  46, midi note: 82 (�A#5/Bb5�)
0, //  47, midi note: 83 (B5)
0, //  48, midi note: 84 (C6)
0, //  49, midi note: 85 (�C#6/Db6�)
0, //  50, midi note: 86 (D6)
0, //  51, midi note: 87 (�D#6/Eb6�)
0, //  52, midi note: 88 (E6)
0, //  53, midi note: 89 (F6)
0, //  54, midi note: 90 (�F#6/Gb6�)
0, //  55, midi note: 91 (G6)
0, //  56, midi note: 92 (�G#6/Ab6�)
0, //  57, midi note: 93 (A6)
0, //  58, midi note: 94 (�A#6/Bb6�)
0, //  59, midi note: 95 (B6)
0, //  60, midi note: 96 (C7)
0, //  61, midi note: 97 (�C#7/Db7�)
0, //  62, midi note: 98 (D7)
0, //  63, midi note: 99 (�D#7/Eb7�)
0, //  64, midi note: 100 (E7)
0, //  65, midi note: 101 (F7)
0, //  66, midi note: 102 (�F#7/Gb7�)
0, //  67, midi note: 103 (G7)
0, //  68, midi note: 104 (�G#7/Ab7�)
0, //  69, midi note: 105 (A7)
0, //  70, midi note: 106 (�A#7/Bb7�)
0, //  71, midi note: 107 (B7)
};

const uint8_t _NOTES_FINE[] = {
92, //  0, midi note: 36 (C2)
156, //  1, midi note: 37 (�C#2/Db2�)
231, //  2, midi note: 38 (D2)
60, //  3, midi note: 39 (�D#2/Eb2�)
155, //  4, midi note: 40 (E2)
2, //  5, midi note: 41 (F2)
114, //  6, midi note: 42 (�F#2/Gb2�)
235, //  7, midi note: 43 (G2)
106, //  8, midi note: 44 (�G#2/Ab2�)
242, //  9, midi note: 45 (A2)
127, //  10, midi note: 46 (�A#2/Bb2�)
20, //  11, midi note: 47 (B2)
174, //  12, midi note: 48 (C3)
78, //  13, midi note: 49 (�C#3/Db3�)
243, //  14, midi note: 50 (D3)
158, //  15, midi note: 51 (�D#3/Eb3�)
77, //  16, midi note: 52 (E3)
1, //  17, midi note: 53 (F3)
185, //  18, midi note: 54 (�F#3/Gb3�)
117, //  19, midi note: 55 (G3)
53, //  20, midi note: 56 (�G#3/Ab3�)
249, //  21, midi note: 57 (A3)
191, //  22, midi note: 58 (�A#3/Bb3�)
138, //  23, midi note: 59 (B3)
87, //  24, midi note: 60 (C4)
39, //  25, midi note: 61 (�C#4/Db4�)
249, //  26, midi note: 62 (D4)
207, //  27, midi note: 63 (�D#4/Eb4�)
166, //  28, midi note: 64 (E4)
128, //  29, midi note: 65 (F4)
92, //  30, midi note: 66 (�F#4/Gb4�)
58, //  31, midi note: 67 (G4)
26, //  32, midi note: 68 (�G#4/Ab4�)
252, //  33, midi note: 69 (A4)
223, //  34, midi note: 70 (�A#4/Bb4�)
197, //  35, midi note: 71 (B4)
171, //  36, midi note: 72 (C5)
147, //  37, midi note: 73 (�C#5/Db5�)
124, //  38, midi note: 74 (D5)
103, //  39, midi note: 75 (�D#5/Eb5�)
83, //  40, midi note: 76 (E5)
64, //  41, midi note: 77 (F5)
46, //  42, midi note: 78 (�F#5/Gb5�)
29, //  43, midi note: 79 (G5)
13, //  44, midi note: 80 (�G#5/Ab5�)
254, //  45, midi note: 81 (A5)
239, //  46, midi note: 82 (�A#5/Bb5�)
226, //  47, midi note: 83 (B5)
213, //  48, midi note: 84 (C6)
201, //  49, midi note: 85 (�C#6/Db6�)
190, //  50, midi note: 86 (D6)
179, //  51, midi note: 87 (�D#6/Eb6�)
169, //  52, midi note: 88 (E6)
160, //  53, midi note: 89 (F6)
151, //  54, midi note: 90 (�F#6/Gb6�)
142, //  55, midi note: 91 (G6)
134, //  56, midi note: 92 (�G#6/Ab6�)
127, //  57, midi note: 93 (A6)
119, //  58, midi note: 94 (�A#6/Bb6�)
113, //  59, midi note: 95 (B6)
106, //  60, midi note: 96 (C7)
100, //  61, midi note: 97 (�C#7/Db7�)
95, //  62, midi note: 98 (D7)
89, //  63, midi note: 99 (�D#7/Eb7�)
84, //  64, midi note: 100 (E7)
80, //  65, midi note: 101 (F7)
75, //  66, midi note: 102 (�F#7/Gb7�)
71, //  67, midi note: 103 (G7)
67, //  68, midi note: 104 (�G#7/Ab7�)
63, //  69, midi note: 105 (A7)
59, //  70, midi note: 106 (�A#7/Bb7�)
56, //  71, midi note: 107 (B7)
};





// **************************************************************************
// System
// ------
// 
// NABU helper functions.
// **************************************************************************

/// <summary>
/// Disable interrupts on the nabu
/// </summary>
void NABU_DisableInterrupts();

/// <summary>
/// Enable interrupts on the nabu
/// </summary>
void NABU_EnableInterrupts();

/// <summary>
/// Perform one NOP
/// </summary>
inline void nop();





// **************************************************************************
// Sound
// -----
// 
// Use the AY sound IC for generating audio. This defaults to initializing the 
// AY in 3 channel tone mode, with no noise channel. The mixer is configured
// for an fading envelope for convenience.
// 
// To use the generic simple audio functions provided, use initAudio()
// at the beginning of your program.
// 
// *Note: You can use the AY ports (IO_AYLATCH and IO_AYDATA) to configure
// the AY chip yourself. Otherwise, you can use these simple helper functions.
// 
// **************************************************************************

/// <summary>
/// Initializes the audio for 3 channels of tones
/// </summary>
void initAudio();

/// <summary>
/// Play a note with delay envelope
/// 
/// - Channel: 0, 1, 2
/// - Note: 0-71 (See note array above and use an index from it)
/// - DelayLength: 0-4096
/// </summary>
void playNoteDelay(uint8_t channel, uint8_t note, uint16_t delayLength);

/// <summary>
/// Write VAL in REG to the AY IC
/// </summary>
void ayWrite(uint8_t reg, uint8_t val);

/// <summary>
/// Read from the REG of AY IC
/// </summary>
inline uint8_t ayRead(uint8_t reg);





// **************************************************************************
// Keyboard
// --------
// 
// Get keys from the keyboard, which is also the joysticks.
// 
// **************************************************************************

/// <summary>
/// The last key that was pressed when isKeyPressed is called
/// </summary>
uint8_t LastKeyPressed = 0x00;

/// <summary>
/// Returns false if no key has been pressed, otherwise it returns the key value
/// </summary>
uint8_t isKeyPressed();

/// <summary>
/// Pauses and waits for a key, returns that key value
/// </summary>
uint8_t getChar();





// **************************************************************************
// HCCA Receive
// ------------
// 
// Receive bytes from the HCCA port. This uses an interrupt, so you will need
// to initialize the interrupt before the method can be used. That means you
// will need to call hcca_EnableReceiveBufferInterrupt() at the beginning of
// your program. From that point on, use the Buffer specific functions for
// receiving, such as hcca_readFromBuffer()
// 
// Also, make sure you call hcca_IsRxBufferAvailable() before hcca_readFromBuffer()
// to ensure there is actually data available.
// 
// **************************************************************************

/// <summary>
/// Enable the HCCA rx buffer (256 bytes).
/// </summary>
void hcca_enableReceiveBufferInterrupt();

/// <summary>
/// Returns TRUE if there is data to be read from the hcca rx buffer (256 bytes)
/// </summary>
bool hcca_isRxBufferAvailable();

/// <summary>
/// Read a character from the buffer (256 bytes). 
/// Check the hcca_ISRxBufferAvailable() first!
/// </summary>
uint8_t hcca_readFromBuffer();

/// <summary>
/// Initializes the HCCA interrupt to know when there is data to read
/// 
/// *Note: Do not call this if using the buffer interrupt mode
/// </summary>
void hcca_receiveModeStart();

/// <summary>
/// Always check if there is data before calling hcca_readbyte() otherwise you get garbage
/// </summary>
bool hcca_isDataAvailable();

/// <summary>
/// Read a byte from the HCCA
/// *Note: You must first check if a byte is available with hcca_IsDataAvailble() first or you get garbage.
/// </summary>
uint8_t hcca_readByte();




// **************************************************************************
// HCCA Transmit
// -------------
// 
// These are transmit methods for the HCCA port
// 
// **************************************************************************

/// <summary>
/// Write to the HCCA
/// </summary>
void hcca_writeByte(uint8_t c);

/// <summary>
/// Write null terminated string to the HCCA
/// </summary>
void hcca_writeString(uint8_t* str);

/// <summary>
/// Write to the HCCA
/// </summary>
void hcca_writeBytes(uint8_t* str, uint8_t len);




// **************************************************************************
// VDP
// ---
// 
// Start by calling one of the graphic modes that you wish to initialize.
// Such as vdp_initTextMode().
// 
// *Note: Read about the function in this file to see if it's compatible 
// with your graphic mode. Some text functions, such as scroll up, are only
// available in text mode.
// **************************************************************************

/// <summary>
/// initialize the VDP
/// Not all parameters are useful for all modes.Refer to documentation
///
/// - mode VDP_MODE_G1 | VDP_MODE_G2 | VDP_MODE_MULTICOLOR | VDP_MODE_TEXT
/// - color
/// - big_sprites true: Use 16x16 sprites false : use 8x8 sprites
/// - magnify true: Scale sprites up by 2
/// </summary>
int vdp_init(uint8_t mode, uint8_t color, bool big_sprites, bool magnify);


/// <summary>
/// Initializes the VDP in text mode
/// 
/// - fgcolor Text color default: default black
/// - bgcolor Background color : default white
/// 
/// returns VDP_ERROR | VDP_SUCCESS
/// </summary>
int vdp_initTextMode(uint8_t fgcolor, uint8_t);

/// <summary>
/// Initializes the VDP in Graphic Mode 1. Not really useful if more than 4k Video ram is available
///
/// - fgcolor Text color default: default black
/// - bgcolor Background color : default white
/// 
/// returns VDP_ERROR | VDP_SUCCESS
/// </summary>
int vdp_initG1Mode(uint8_t fgcolor, uint8_t bgcolor);

/// <summary>
/// Initializes the VDP in Graphic Mode 2
/// 
/// - big_sprites true: use 16x16 sprites false : use 8x8 sprites
/// - scale_sprites Scale sprites up by 2
/// 
/// returns VDP_ERROR | VDP_SUCCESS
/// </summary>
int vdp_initG2Mode(bool big_sprites, bool scale_sprites);

/// <summary>
/// Initializes the VDP in 64x48 Multicolor Mode. Not really useful if more than 4k Video ram is available
///
/// returns VDP_ERROR | VDP_SUCCESS
/// </summary>
int vdp_initMultiColorMode();

/// <summary>
/// Set foreground and background color of the pattern at the current cursor position
/// 
/// Only available in Graphic mode 2
/// - fgcolor Foreground color
/// - bgcolor Background color
/// </summary>
void vdp_colorize(uint8_t fgcolor, uint8_t bgcolor);

/// <summary>
/// Plot a point at position (x,y), where x <= 255. The full resolution of 256 by 192 is available.
/// Only two different colors are possible whithin 8 neighboring pixels
/// VDP_MODE G2 only
///
/// - x
/// - y
/// - color1 Color of pixel at (x,y). If NULL, plot a pixel with color2
/// - color2 Color of the pixels not set or color of pixel at (x,y) when color1 == NULL
/// </summary>
void vdp_plotHires(uint8_t x, uint8_t y, uint8_t color1, uint8_t color2);

/// <summary>
/// Plot a point at position (x,y), where x <= 64. In Graphics mode2, the resolution is 64 by 192 pixels, neighboring pixels can have different colors.
/// In Multicolor  mode, the resolution is 64 by 48 pixels
///
/// - x
/// - y
/// - color
/// </summary>
void vdp_plotColor(uint8_t x, uint8_t y, uint8_t color);

/// <summary>
///  Print string at current cursor position. These Escape sequences are supported:
/// 
/// Graphic Mode 2 only: \\033[<fg>;[<bg>]m sets the colors and optionally the background of the subsequent characters 
/// 
/// Example: vdp_print("\033[4m Dark blue on transparent background\n\r\033[4;14m dark blue on gray background");
/// 
/// - text Text to print
/// </summary>
void vdp_print(uint8_t* text);

///////
///  Set backdrop color
///
/// - color
////
void vdp_setBackDropColor(uint8_t);

/// <summary>
///  Set the color of patterns at the cursor position
///
/// - index VDP_MODE_G2: Number of pattern to set the color, VDP_MODE_G1: one of 32 groups of 8 subsequent patterns
/// - fg Pattern foreground color
/// - bg Pattern background color
/// </summary>
void vdp_setPatternColor(uint16_t index, uint8_t fgcolor, uint8_t bgcolor);

/// <summary>
///  Position the cursor at the specified position
///
/// - col column
/// - row row
/// </summary>
void vdp_setCursor2(uint8_t col, uint8_t row);

/// <summary>
///  Move the cursor along the specified direction
///
/// - direction {VDP_CURSOR_UP|VDP_CURSOR_DOWN|VDP_CURSOR_LEFT|VDP_CURSOR_RIGHT}
/// </summary
void vdp_setCursor(uint8_t direction) __z88dk_fastcall;

/// <summary>
///  set foreground and background color of the characters printed after this function has been called.
/// In Text Mode and Graphics Mode 1, all characters are changed. In Graphics Mode 2, the escape sequence \\033[<fg>;<bg>m can be used instead.
/// See vdp_print()
///
/// - fg Foreground color
/// - bg Background color
/// </summary>
void vdp_setTextColor(uint8_t fgcolor, uint8_t bgcolor);

/// <summary>
///  Write ASCII character at current cursor position
///
/// - chr Pattern at the respective location of the  pattern memory. Graphic Mode 1 and Text Mode: Ascii code of character
/// </summary>
void vdp_write(uint8_t chr, bool advanceNextChar);

/// <summary>
///  Write ASCII character at current cursor position
///
/// - chr Pattern at the respective location of the  pattern memory. Graphic Mode 1 and Text Mode: Ascii code of character
/// </summary>
void vdp_writeG2(uint8_t chr, bool advanceNextChar);

/// <summary>
///  Write a sprite into the sprite pattern table
///
/// - name Reference of sprite 0-255 for 8x8 sprites, 0-63 for 16x16 sprites
/// - sprite Array with sprite data. Type uint8_t[8] for 8x8 sprites, uint8_t[32] for 16x16 sprites
/// </summary>
void vdp_setSpritePattern(uint8_t name, const uint8_t* sprite);

/// <summary>
///  Set the sprite color
///
/// - handle Sprite Handle returned by vdp_sprite_init()
/// - color
/// </summary>
void vdp_setSpriteColor(uint16_t handle, uint8_t color);

/// <summary>
///  Get the sprite attributes
///
/// - handle Sprite Handle returned by vdp_sprite_init()
/// 
/// Returns Sprite_attributes
/// </summary>
Sprite_attributes vdp_sprite_get_attributes(uint16_t handle);

/// <summary>
/// Add a new line (move down and to line start)
/// </summary>
void vdp_newLine();

/// <summary>
/// Get the character in text mode at the specified location directly from VRAM, which is slow
/// </summary>
uint8_t vdp_getCharAtLocationVRAM(uint8_t x, uint8_t y);

/// <summary>
/// In text mode, there is a buffer copy of the screen
/// </summary>
uint8_t vdp_getCharAtLocationBuf(uint8_t x, uint8_t y);

/// <summary>
/// Set the character in memory buffer at location. This does not update the screen!
/// It is used by the text scroll methods
/// </summary
void vdp_setCharAtLocationBuf(uint8_t x, uint8_t y, uint8_t c);

/// <summary>
/// Scroll all lines up between topRow and bottomRow
/// </summary>
void vdp_scrollTextUp(uint8_t topRow, uint8_t bottomRow);

/// <summary>
/// Clear the rows with 0x20 between topRow and bottomRow
/// </summary>
void vdp_clearRows(uint8_t topRow, uint8_t bottomRow);

/// <summary>
/// Write a character at the specified location
/// </summary>
void vdp_writeCharAtLocation(uint8_t x, uint8_t y, uint8_t c);

/// <summary>
/// Dump the current font/name table to the screen for preview
/// </summary>
void vdp_dumpFontToScreen();

/// <summary>
/// Display the numeric value of the variable
/// </summary>
void vdp_writeUInt8(uint8_t v) __z88dk_fastcall;

/// <summary>
/// Display the numer value of the variable
/// </summary>/// <param name="v"></param>
void vdp_writeUInt16(uint16_t v) __z88dk_fastcall;

/// <summary>
/// Display the numer value of the variable
/// </summary>/// <param name="v"></param>
void vdp_writeInt16(int16_t v) __z88dk_fastcall;

/// <summary>
/// Display the numeric value of the variable
/// </summary>
void vdp_writeInt8(int8_t v) __z88dk_fastcall;

/// <summary>
/// Display the binary value of the variable
/// </summary>
void vdp_writeUInt8ToBinary(uint8_t v) __z88dk_fastcall;

/// <summary>
/// Display the binary value of the variable
/// </summary>
void vdp_writeUInt16ToBinary(uint16_t v) __z88dk_fastcall;

/// <summary>
///  Get the current position of a sprite
///
/// - handle Sprite Handle returned by vdp_sprite_init()
/// - xpos Reference to x-position
/// - ypos Reference to y-position
/// </summary>
void vdp_getSpritePosition(uint16_t handle, uint16_t xpos, uint8_t ypos);

/// <summary>
///  Activate a sprite
///
/// - name Number of the sprite as defined in vdp_set_sprite()
/// - priority 0: Highest priority; 31: Lowest priority
/// - color
/// Returnss     Sprite Handle
/// </summary>
uint16_t vdp_spriteInit(uint8_t name, uint8_t priority, uint8_t color);

/// <summary>
///  Set position of a sprite
///
/// - handle  Sprite Handle returned by vdp_sprite_init()
/// - x
/// - y
/// Returnss     true: In case of a collision with other sprites
/// </summary>
uint8_t vdp_setSpritePosition(uint16_t handle, uint16_t x, uint8_t y);



#include "NABU-LIB.c"

#endif
