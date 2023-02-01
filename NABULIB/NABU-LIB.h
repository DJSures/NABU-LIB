// ****************************************************************************************
// NABU-LIB C Library
// DJ Sures (c) 2023
// https://nabu.ca
// 
// Last updated on Feb 1, 2023 (v2023.02.01.00)
// 
// Get latest copy and examples from: https://github.com/DJSures/NABU-LIB
// 
// This is a z88dk C library for the NABU Personal Computer. This is a large library
// with many functions for the VDP, Sound, HCCA, and Keyboard. The functions are split
// into sections based on the peripheral.  
// 
// Read the summaries below for details of each function in this file.
// 
// **********************************************************************************************

// No touch
// -----------------------
#ifndef NABU_H
#define NABU_H
#define BIN_HOMEBREW 100
#define BIN_CPM 200


/// **************************************************************************
/// 
/// CONFIGURE FONT
/// --------------
/// 
/// If you will be using the VDP, choose the default font.
/// Add one of these #define's into your main.c program before the #include "nabu-lib.h"
/// If you with to use your own font, specify the font as:
///
///    const uint8_t[768] ASCII = {} 
///
/// before the #include "nabu-lib.h" in your code. You can look at the included
/// patterns.h for the array format. The default popular font with case sensitive 
/// characters is FONT_LM80C
///
/// *Note: You do not need to include a font if DISABLE_VDP is set and you're
///        building a text-only cp/m program.
/// **************************************************************************
/// #define FONT_AMIGA
/// #define FONT_SET1
/// #define FONT_STANDARD
/// #define FONT_LM80C


/// **************************************************************************
/// 
/// BINARY TYPE
/// -----------
/// 
/// It is important to define what kind of binary we are going to be creating.
/// Add one of these two options to the top of your main.c before the #include
/// statements.
///
/// HOMEBREW
/// This binary is executable as a standalone application from the NABU
/// Internet Adapter or as a NABU Channel.
///
/// Cloud CP/M
/// A binary that produces a .COM executable which will be run on the
/// Cloud CP/M Operating System.
///
/// **************************************************************************
// #define BIN_TYPE BIN_HOMEBREW
// #define BIN_TYPE BIN_CPM


/// **************************************************************************
/// 
/// KEYBOARD INPUT TYPE
/// -------------------
/// 
/// If you are using CPM stdio, such as gets, you will need to disable the keyboard
/// interupt in NABULIB. Add this #define above your #include in the main.c
/// If you disable the keyboard interrupt, you cannot use any NABULIB keyboard
/// input commands and have to use the cpm ones.
///
/// For safety and memory, this will hide all NABULIB keyboard input functions
/// **************************************************************************
// #define DISABLE_KEYBOARD_INT


/// **************************************************************************
/// 
/// HCCA (SERIAL)
/// -------------
/// 
/// If your program is not using the file store or HCCA for retronet, you can
/// disable the RX interrupt to save filesize.
///
/// Note that there are a few VDP commands in the HCCA functions, so you cannot
/// disable VDP and enable HCCA RX. You can disable HCCA RX and keep VDP, but 
/// you cannot disable VDP and keep HCCA.
///
/// If you are keeping the HCCA, you can customize the buffer size further down
/// in this file by adding the #define RX_BUFFER_SIZE XXX to your main.c above
/// the #includes. That allows you to lower the reserved buffer if needed.
///
/// Add this #define above your #include in the main.c to disable the HCCA RX INT
/// **************************************************************************
// #define DISABLE_HCCA_RX_INT


/// **************************************************************************
/// 
/// VDP GRAPHICS
/// ------------
/// 
/// You can disable the vdp functions if you're just using the cpm built-in
/// console stuff, like puts() or printf(), etc.
///
/// Note that there are a few VDP commands in the HCCA functions, so you cannot
/// disable VDP and enable HCCA RX. You can disable HCCA RX and keep VDP, but 
/// you cannot disable VDP and keep HCCA.
///
/// Add this #define above your #include in the main.c to disable the VDP commands
/// **************************************************************************
// #define DISABLE_VDP


/// **************************************************************************
/// The cursor that will be displayed with getChar() and readLine()
/// You can override this value by defining it before the #include in your main.c
/// If you do not want a cursor, set it to 0x00
/// Note: This is only valid when using the nabulib vdp and keyboard interupts
/// **************************************************************************
#ifndef CURSOR_CHAR
#define CURSOR_CHAR '_'
#endif

/// **************************************************************************
/// You can disable the cursor by putting this line above the #include in your main.c
/// If you disable the VDP but still want to use the keyboard interrupt, then
/// this will also work. 
/// **************************************************************************
// #define DISABLE_CURSOR

/// **************************************************************************
/// HCCA RX Input buffer.
/// You can override the RX BUFFER SIZE by defining it before the #include in your main.c
/// 
/// ****IMPORTANT: It is important that any buffer/packet size you read using RetroNET
///                remote filesystem cannot be larger than this buffer size!
///
/// **************************************************************************
#ifndef DISABLE_HCCA_RX_INT
  #ifndef RX_BUFFER_SIZE
    #define RX_BUFFER_SIZE 1024
    #warning
    #warning Using default 1024 HCCA RX Buffer
    #warning
  #endif
#endif

#ifndef BIN_TYPE
  #error A BIN_TYPE has not been specified. Look at the NABU-LIB.h to configure your application.
#endif

#include <stdbool.h>
#include <stdlib.h>
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

__sfr __at 0x00 IO_CONTROL;

#define CONTROL_ROMSEL     0x01
#define CONTROL_VDOBUF     0x02
#define CONTROL_STROBE     0x04
#define CONTROL_LED_CHECK  0x08
#define CONTROL_LED_ALERT  0x10
#define CONTROL_LED_PAUSE  0x20

#define INTERUPT_VECTOR_MAP_MSB 0xff
#define INTERUPT_VECTOR_MAP_ADDRESS 0xff00

#define INT_MASK_HCCARX   0x80
#define INT_MASK_HCCATX   0x40
#define INT_MASK_KEYBOARD 0x20
#define INT_MASK_VDP      0x10

/// <summary>
/// The interupt vector address should not be changed becuase it is shared with CPM.
/// This is because you can disable either the HCCA RX or Keyboard interrupt and let
/// CPM subsystem (BDOS) continue using it. So this address must always stay the same
/// as CPM is using.
/// </summary>
#define INTERUPT_VECTOR_MAP_MSB     0xff
#define INTERUPT_VECTOR_MAP_ADDRESS 0xff00

#define IOPORTA  0x0e
#define IOPORTB  0x0f

// Used for getting the r register from the z80 to seed srand()
volatile uint8_t _randomSeed = 0;

#ifndef DISABLE_HCCA_RX_INT
  uint8_t _rxBuffer[RX_BUFFER_SIZE];
  uint16_t _rxBufferReadPos = 0;
  uint16_t _rxBufferWritePos = 0;
  #warning
  #warning HCCA Interupt: Enabled
  #warning
#else
  #warning
  #warning HCCA Interrupt: Disabled
  #warning
#endif

#ifndef DISABLE_KEYBOARD_INT

  volatile uint8_t _kbdBuffer[256];
  volatile uint8_t _kbdBufferReadPos = 0;
  volatile uint8_t _kbdBufferWritePos = 0;
  volatile uint8_t _lastKeyboardIntVal = 0;
  volatile uint8_t _joyStatus[4] = {0};

  typedef enum JOYSTICKENUM {
    Joy_Left = 0b00000001,
    Joy_Down = 0b00000010,
    Joy_Right = 0b00000100,
    Joy_Up = 0b00001000,
    Joy_Button = 0b00010000,
  };

  #warning
  #warning Keyboard Interupt Enabled. Use NABU-LIB keyboard functions only (no CPM STDIN)
  #warning
#else
  #warning
  #warning Keyboard Interupt Disabled. If building for CPM, uses C STDIN stdio/conio functions for keyboard.
  #warning
#endif

/// <summary>
/// The original Interrupt Mask when initNABULib() was called. 
/// This is used when a function needs to briefly pause or change interrupts, it can put the interrupt back.
/// For example, this is used with writeByte() and all of it's parents (i.e. writeByteUInt16, etc..)
/// The writebyte() will disable interrupts and modify the interrupt mask so it can monitor the TX flag.
/// Once the byte has been sent, it will restore the interrupts as they were.
/// </summary>
uint8_t _ORIGINAL_INT_MASK = 0;
 
/// <summary>
/// This stream of bytes are sent by calling the function hcca_exitRetroNETBridgeMode();
/// Sending this stream of bytes when in a bridge mode (i.e. TCP connection terminal connection) will exit bridge mode
/// and return you back to the main nabu talker loop.
/// 
/// Yes this is a hack, because I should have implemented a file stream just like I did with rn_fileHandle stuff (TODO I guess)
/// </summary>
#define RETRONET_BRIDGE_EXIT_CODE_LEN 21
uint8_t RETRONET_BRIDGE_EXIT_CODE[RETRONET_BRIDGE_EXIT_CODE_LEN] = { 0x0f, 0xb7, 0xb8, 0xb9, 0x0f, 0xb9, 0xb8, 0xb7, 0xb8, 0xb9, 0xb7, 0xb7, 0xb8, 0xb9, 0xb9, 0xb8, 0xb7, 0xb8, 0x0f, 0xb9, 0xb7 };


// **************************************************************************
// VDP Variables
// -------------
// 
// These are variables and definitions for the VDP functions, such as
// cursor location, screen width/height for text calculations, and 
// the text buffer.
// 
// **************************************************************************
#ifndef DISABLE_VDP

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
  } vdp_cursor = { 0, 0 };


  /// <summary>
  // For the customizable vdp scanline interrupt
  // See the void vdp_addISR(void (*vdpISR)()) function for this
  /// </summary>
  void (*_vdp_ISR)();

  
  /// <summary>
  // The original value of the VDP Register 1 (i.e. graphic mode, memory, and interrupt status).
  // This is used for enabling/disabling interrupts progrmatically because we can't re-read
  // resgister 1 because they are write-only.
  /// </summary>
  volatile uint8_t _vdpReg1Val = 0;

  uint16_t       _vdpSpriteAttributeTableAddr;
  uint16_t       _vdpSpritePatternTableAddr;
  uint8_t        _vdpSpriteSizeSelected;
  uint16_t       _vdpNameTableAddr;
  uint16_t       _vdpColorTableSize = 0;
  uint16_t       _vdpColorTableAddr;
  uint16_t       _vdpPatternTableAddr;
  uint8_t        _vdpCursorMaxX;
  const uint8_t  _vdpCursorMaxY = 23;
  uint8_t        _vdpMode;
  bool           _autoScroll;


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


  #warning
  #warning VDP has been enabled. If using CPM, you can mix STDOUT and NABU-LIB vdp functions.
  #warning

#else

  #warning
  #warning VDP has been disabled. If using CPM, you must use STDOUT functions (i.e. printf, fputs, etc). NABU-LIB vdp functions are not available
  #warning

#endif



// **************************************************************************
// Generics
// --------
//
// Here are some helper functions that are useful when 
//
// **************************************************************************

#define Max(x,y) (((x) >= (y)) ? (x) : (y))

#define Min(x,y) (((x) <= (y)) ? (x) : (y))

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
/// Initialize the NABU-LIB. This should be the very first thing in your program.
/// Based on your configuration, this will initialize what you have not "#define DISABLE_X"
/// This will enable the audio, HCCA RX & keyboard Interupts, seed random generator, and disable the ROM
/// </summary>
void initNABULib();

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
void nop();



// **************************************************************************
// Sound
// -----
// 
// Use the AY sound IC for generating audio. This defaults to initializing the 
// AY in 3 channel tone mode, with no noise channel. The mixer is configured
// for an fading envelope for convenience.
// 
// To use the generic simple audio functions provided, use initNABULib()
// at the beginning of your program.
// 
// *Note: You can use the AY ports (IO_AYLATCH and IO_AYDATA) to configure
// the AY chip yourself. Otherwise, you can use these simple helper functions.
// 
// **************************************************************************

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
uint8_t ayRead(uint8_t reg);





// **************************************************************************
// Keyboard
// --------
// 
// Get keys from the keyboard, which is also the joysticks.
// 
// **************************************************************************
#ifndef DISABLE_KEYBOARD_INT

  /// <summary>
  /// Returns false if no key has been pressed, otherwise it returns the key value
  /// </summary>
  uint8_t isKeyPressed();

  /// <summary>
  /// Blocks and waits for a key, returns that key value
  /// </summary>
  uint8_t getChar();

  /// <summary>
  /// Read a line of text inputed by the keyboard. This does not null terminate the
  /// user input. If you need the input null terminated, make the maxInputLen one byte
  /// less than the buffer size, and manually set the 0x00 yourself after the return.
  /// 
  /// - buffer is the pointer to store the text input
  /// - maxInputLen is the max length of data you want to retrieve (no larger than the buffer!)
  /// 
  /// Returns the length of data entered by the user. 
  /// </summary>
  uint8_t readLine(uint8_t* buffer, uint8_t maxInputLen);

  /// <summary>
  /// Returns the status of the joystick at the specified index (0 - 3)
  /// The status will return the following byte structure...
  ///
  /// MSB Byte 5: Button status
  ///     Byte 4: Up
  ///     Byte 3: Right
  ///     Byte 2: Down
  /// LSB Byte 1: Left
  ///
  /// You can check for a direction by comparing the value against JOYSTICKENUM values. 
  /// For example you can check if a button is pressed with this example. Same applies to directions...
  ///
  ///   if (getJoyStatus(0) & Joy_Button)
  ///  
  /// or check a direction...
  ///  
  ///   if (getJoyStatus(0) & Joy_Up)
  ///  
  /// </summary>
  uint8_t getJoyStatus(uint8_t joyNum);
#endif

#if BIN_TYPE == BIN_CPM

  // **************************************************************************
  // VT52
  // ----------
  // 
  // When using CPM STDOUT (i.e. printf, etc.) you can use the VT52 emulation built into the BIOS
  // You can also SET and GET the cursor in CPM by using cpm_cursor.x and cpm_cursor.y
  // **************************************************************************

  /// <summary>
  /// This is the current position of the cursor in CPM in the BIOS.
  /// This is here because CPM does not provide cursor information. So by using this with STDOUT, you can SET and GET the CPM cursor
  /// position rather than using the VT52 commands.
  /// </summary>
  struct {
    uint8_t x;
    uint8_t y;
  } __at (0xff10) cpm_cursor;

  void vt_clearToEndOfScreen();

  void vt_clearToEndOfLine();

  void vt_clearScreen();

  void vt_clearLine();

  void vt_clearToStartOfLine();

  void vt_clearToStartOfScreen();

  void vt_moveCursorDown(uint8_t count);

  void vt_cursorHome();

  void vt_moveCursorLeft(uint8_t count);

  void vt_moveCursorRight(uint8_t count);

  void vt_moveCursorUp(uint8_t count);

  void vt_deleteLine();

  void vt_setCursor(uint8_t x, uint8_t y);

  void vt_foregroundColor(uint8_t color);

  void vt_insertLine();

  void vt_restoreCursorPosition();

  void vt_backgroundColor(uint8_t color);

  void vt_saveCursorPosition();

  void vt_cursorUpAndInsert();

  void vt_wrapOff();

  void vt_wrapOn();

#endif 


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
#ifndef DISABLE_HCCA_RX_INT

  /// <summary>
  /// Returns TRUE if there is data to be read from the hcca rx buffer (256 bytes)
  /// </summary>
  bool hcca_isRxBufferAvailable();

  /// <summary>
  /// Returns how much data is currently in the RX buffer
  /// </summary>
  uint8_t hcca_getSizeOfDataInBuffer();

  /// <summary>
  /// Read a byte from the buffer.
  /// Check the hcca_ISRxBufferAvailable() first or this blocks.
  /// </summary>
  uint8_t hcca_readByte();

  /// <summary>
  /// Read an unsigned 16-bit integer from the HCCA
  /// *Note: You must first check if a byte is available with hcca_IsDataAvailble() first or you get garbage.
  /// </summary>
  uint16_t hcca_readUInt16();

  /// <summary>
  /// Read an signed 16-bit integer from the HCCA
  /// *Note: You must first check if a byte is available with hcca_IsDataAvailble() first or you get garbage.
  /// </summary>
  int16_t hcca_readInt16();

  /// <summary>
  /// Read an unsigned 32-bit integer from the HCCA
  /// *Note: You must first check if a byte is available with hcca_IsDataAvailble() first or you get garbage.
  /// </summary>
  uint32_t hcca_readUInt32();

  /// <summary>
  /// Read an signed 32-bit integer from the HCCA
  /// *Note: You must first check if a byte is available with hcca_IsDataAvailble() first or you get garbage.
  /// </summary>
  int32_t hcca_readInt32();

  /// <summary>
  /// Read bufferLen into buffer, starting at the offset
  /// </summary>
  void hcca_readBytes(uint8_t offset, uint8_t bufferLen, uint8_t* buffer);




  // **************************************************************************
  // HCCA Transmit
  // -------------
  // 
  // These are transmit methods for the HCCA port
  // 
  // **************************************************************************

  /// <summary>
  /// Write a byte to the HCCA
  /// </summary>
  void hcca_writeByte(uint8_t c);

  /// <summary>
  /// Write the unsigned 32-bit integer to the HCCA.
  /// This is LSB First
  /// </summary>
  void hcca_writeUInt32(uint32_t val);

  /// <summary>
  /// Write the signed 32-bit integer to the HCCA.
  /// This is LSB First
  /// </summary>
  void hcca_writeInt32(int32_t val);

  /// <summary>
  /// Write the unsigned 16-bit integer to the HCCA.
  /// This is LSB First
  /// </summary>
  void hcca_writeUInt16(uint16_t val);

  /// <summary>
  /// Write the signed 16-bit integer to the HCCA.
  /// This is LSB First
  /// </summary>
  void hcca_writeInt16(int16_t val);

  /// <summary>
  /// Write null terminated string to the HCCA
  /// </summary>
  void hcca_writeString(uint8_t* str);

  /// <summary>
  /// Write to the HCCA
  /// </summary>
  void hcca_writeBytes(uint16_t offset, uint16_t length, uint8_t* bytes);

  /// <summary>
  /// Exit the TCP bridge mode for Telnet and RetroNET Chat
  /// This is a hack because I should have implemented a file stream just like i did with rn_fileHandle stuff (TODO I guess)
  /// </summary>
  void hcca_exitRetroNETBridgeMode();

#endif



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
#ifndef DISABLE_VDP

  /// <summary>
  /// Add a function to the VDP frame sync interrupt. The function you add should be _naked with a ei and reti. 
  /// After calling initNABULib(), this can be called if you need the vdp interrupt (i.e. for G2 graphics).
  /// This function requires that initNABULib() be called first because it will setup the interrupts, and
  /// specifically, the _ORIGINAL_INT_MASK, which this function will modify.
  ///
  /// It is important that the STATUS register (0x01) be read. It is provided in the template below.
  /// 
  /// When you are done with VDP interrupts (ie presenting a menu or something) call vdp_removeISR() to stop the interrupt
  ///
  /// See this example to setup a custom vdp interrupt...
  //
  // void myVdpISR() __naked {
  //
  //   __asm
  //     push	hl;      
  //     push  bc;      
  //     push  de;      
  //     push  af;      
  //     push  iy;
  //     push  ix;
  //  __endasm;
  //
  //   uint8_t vdpStatus = IO_VDPLATCH;
  //
  //   PUT YOUR CODE HERE
  //
  //   __asm
  //     pop ix;
  //     pop iy;
  //     pop af;      
  //     pop de;      
  //     pop bc;      
  //     pop hl;      
  //     ei;
  //     reti;
  //   __endasm;
  // }
  //
  // void main() { 
  //
  //   initNABULib();
  //   vdp_addISR(myVdpISR);
  // }
  //
  /// </summary>
  void vdp_addISR(void (*isr)());


  /// <summary>
  /// Removes and stops the VDP interrupt after calling vdp_addISR();
  /// When you are switching to a menu in text mode or need to stop the game, call this.
  /// </summary>
  void vdp_removeISR();

  /// <summary>
  /// initialize the VDP with the default font.
  /// Add one of these #define's into your main.c program before the #include "nabu-lib.h"
  /// If you with to use your own font, specify the font as a const uint8_t[768] ASCII = {} before the #include "nabu-lib.h" in your code
  /// --------------------------------------------------------------------------------------------------------------------------
  /// #define FONT_AMIGA
  /// #define FONT_SET1
  /// #define FONT_STANDARD
  /// #define FONT_LM80C
  /// 
  /// mode:         One of the graphic modes (VDP_MODE_G1 | VDP_MODE_G2 | VDP_MODE_MULTICOLOR | VDP_MODE_TEXT)
  /// color:        Color of the background
  /// spriteSize:   true for 16x16 sprites, or false for 8x8 sprites
  /// scaleSprites: use software to scale the sprites by 2.
  ///               You will still provide the sprite size specified from 'spriteSize' but
  ///               they will be double the size when put on the screen 
  ///  autoScroll:  Scrolls textmode vertically when your text is at the bottom of the screen 
  /// </summary>
  void vdp_init(uint8_t mode, uint8_t color, bool spriteSize, bool scaleSprites, bool autoScroll);

  /// <summary>
  /// This will update the vram to the specified font.
  /// The vdp_init will initialize the default font. This is if you want to change the font.
  /// </summary>
  void vdp_initTextModeFont(uint8_t* font);

  /// <summary>
  /// Initializes the VDP in text mode
  /// 
  /// - fgcolor Text color 
  /// - bgcolor Background 
  /// 
  /// returns VDP_ERROR | VDP_SUCCESS
  /// </summary>
  void vdp_initTextMode(uint8_t fgcolor, uint8_t bgcolor, bool autoScroll);

  /// <summary>
  /// Initializes the VDP in Graphic Mode 1. Not really useful if more than 4k Video ram is available
  ///
  /// - fgcolor Text color default: default black
  /// - bgcolor Background color : default white
  /// 
  /// returns VDP_ERROR | VDP_SUCCESS
  /// </summary>
  void vdp_initG1Mode(uint8_t fgcolor, uint8_t bgcolor);

  /// <summary>
  /// Initializes the VDP in Graphic Mode 2
  /// 
  /// spriteSize:   true for 16x16 sprites, or false for 8x8 sprites
  /// scaleSprites: use software to scale the sprites by 2.
  ///               You will still provide the sprite size specified from 'spriteSize' but
  ///               they will be double the size when put on the screen 
  /// 
  /// returns VDP_ERROR | VDP_SUCCESS
  /// </summary>
  void vdp_initG2Mode(bool spriteSize, bool scaleSprites);

  /// <summary>
  /// Initializes the VDP in 64x48 Multicolor Mode. Not really useful if more than 4k Video ram is available
  ///
  /// returns VDP_ERROR | VDP_SUCCESS
  /// </summary>
  void vdp_initMultiColorMode();

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
  /// Print null terminated string at current cursor position. These Escape sequences are supported:
  /// 
  /// Graphic Mode 2 only: \\033[<fg>;[<bg>]m sets the colors and optionally the background of the subsequent characters 
  /// 
  /// Example: vdp_print("\033[4m Dark blue on transparent background\n\r\033[4;14m dark blue on gray background");
  /// 
  /// - text Text to print
  /// </summary>
  void vdp_print(uint8_t* text);

  /// <summary>
  /// Print the specified portion of the string. No escape sequences are supported.
  /// </summary>
  void vdp_printPart(uint16_t offset, uint16_t textLength, uint8_t* text);
    
  /// <summary>
  /// Set backdrop border color
  /// </summary>
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
  ///  Write ASCII character at current cursor position and advances forward
  ///
  /// - chr Pattern at the respective location of the  pattern memory. Graphic Mode 1 and Text Mode: Ascii code of character
  /// </summary>
  void vdp_write(uint8_t chr);

  /// <summary>
  ///  Write ASCII character at current cursor position and advances forward
  ///
  /// - chr Pattern at the respective location of the  pattern memory. Graphic Mode 1 and Text Mode: Ascii code of character
  /// </summary>
  void vdp_writeG2(uint8_t chr);

  /// <summary>
  ///  Disable a sprite
  /// </summary>
  void vdp_disableSprite(uint8_t id);

  /// <summary>
  ///  Write a sprite into the sprite pattern table
  ///
  ///  1) the first sprite in the sprite table that you make must be completely empty
  ///     this is because sprites have a default id of 0 when not initialized. So if you
  ///     your first entry is not empty, you will always have a collision. Unused
  ///     sprites will get a pattern name of 0.
  ///
  ///  2) call vdp_loadSpritePatternNameTable() to load all of the sprite patterns into vdp ram
  ///
  ///  3) vdp_spriteInit() to assign a pattern name to a sprite id
  ///  
  ///  4) Now you can call vdp_setSpritePosition() to move the spriteId around in a vdp interrupt
  ///
  ///  5) You can disable sprites that you don't shown with vdp_disableSprite()
  ///
  /// </summary>
  void vdp_loadSpritePatternNameTable(uint16_t numSprites, const uint8_t* sprite);

  /// <summary>
  ///  Initialize a sprite by the ID
  ///
  ///  Parameters:
  ///
  ///                   id: The unique ID that you assign to this sprite between 0-31. 
  ///                       Higher number layers the sprites on top of lower numbers
  ///
  ///  spritePatternNameId: The ID of the pattern that you assign to this sprite. You get this from
  ///                       the pattern table that you created and loaded.
  ///
  ///                    x: The X cordinate to place the sprite on the screen.
  ///
  ///                    y: The y cordinate to place the sprite on the screen.
  ///
  ///                color: The color of the sprite
  ///
  ///  1) the first sprite in the sprite table that you make must be completely empty
  ///     this is because sprites have a default id of 0 when not initialized. So if you
  ///     your first entry is not empty, you will always have a collision. Unused
  ///     sprites will get a pattern name of 0.
  ///
  ///  2) call vdp_loadSpritePatternNameTable() to load all of the sprite patterns into vdp ram
  ///
  ///  3) vdp_spriteInit() to assign a pattern name to a sprite id
  ///  
  ///  4) Now you can call vdp_setSpritePosition() to move the spriteId around in a vdp interrupt
  ///
  ///  5) You can disable sprites that you don't shown with vdp_disableSprite()
  ///
  ///  Returns the sprite ID (the id that was passed as a parameter)
  /// </summary>
  uint8_t vdp_spriteInit(uint8_t id, uint8_t spritePatternNameId, uint8_t x, uint8_t y, uint8_t color);

  /// <summary>
  ///  Set the sprite color by id
  ///
  /// - id of sprite
  /// - color
  /// </summary>
  void vdp_setSpriteColor(uint8_t id, uint8_t color);

  /// <summary>
  ///  Set position of a sprite
  ///
  /// - id of sprite
  /// - x
  /// - y
  /// </summary>
  void vdp_setSpritePosition(uint8_t id, uint8_t x, uint8_t y);

  /// <summary>
  ///  Set position and color of a sprite
  ///
  /// - id of sprite
  /// - x
  /// - y
  /// - color
  /// </summary>
  void vdp_setSpritePositionAndColor(uint8_t id, uint8_t x, uint8_t y, uint8_t color);

  /// <summary>
  ///  Get the current position of a sprite
  ///
  /// - id of sprite to get position of
  /// - xpos Reference to x-position
  /// - ypos Reference to y-position
  /// </summary>
  void vdp_getSpritePosition(uint8_t id, uint8_t * xpos, uint8_t * ypos);

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
  /// Display the numeric value of the variable
  /// </summary>
  void vdp_writeUInt8(uint8_t v);

  /// <summary>
  /// Display the numeric value of the variable
  /// </summary>
  void vdp_writeInt8(int8_t v);

  /// <summary>
  /// Display the numer value of the variable
  /// </summary>
  void vdp_writeUInt16(uint16_t v);

  /// <summary>
  /// Display the numer value of the variable
  /// </summary
  void vdp_writeInt16(int16_t v);

  /// <summary>
  /// Display the numer value of the variable
  /// </summary>
  void vdp_writeUInt32(uint32_t v);

  /// <summary>
  /// Display the numer value of the variable
  /// </summary>
  void vdp_writeInt32(int32_t v);

  /// <summary>
  /// Display the binary value of the variable
  /// </summary>
  void vdp_writeUInt8ToBinary(uint8_t v);

  /// <summary>
  /// Display the binary value of the variable
  /// </summary>
  void vdp_writeUInt16ToBinary(uint16_t v);

#endif


#include "NABU-LIB.c"

#endif
