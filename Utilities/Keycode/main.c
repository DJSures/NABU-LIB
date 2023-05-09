#define INTERUPT_VECTOR_MAP_MSB 0xff
#define INTERUPT_VECTOR_MAP_ADDRESS 0xff00

#define INT_MASK_HCCARX   0x80
#define INT_MASK_HCCATX   0x40
#define INT_MASK_KEYBOARD 0x20
#define INT_MASK_VDP      0x10

#define IOPORTA  0x0e
#define IOPORTB  0x0f

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

volatile uint8_t _kbdBuffer[256];
volatile uint8_t _kbdBufferReadPos = 0;
volatile uint8_t _kbdBufferWritePos = 0;

__sfr __at 0x40 IO_AYDATA;
__sfr __at 0x41 IO_AYLATCH;

__sfr __at 0x90 IO_KEYBOARD;
__sfr __at 0x91 IO_KEYBOARD_STATUS;

void isrKeyboard() __naked {

  __asm
    push bc;
    push de;
    push hl;
    push af;
  __endasm;

  _kbdBuffer[_kbdBufferWritePos] = IO_KEYBOARD;

  _kbdBufferWritePos++;

  __asm
    pop af;
    pop hl;
    pop de;
    pop bc;
    ei;
    reti;
  __endasm;
}

uint8_t key = 0;

void main() {

  puts("\nSuper Fancy Scancode            DJ Sures");
  puts("CTRL-C exits\n");

  uint8_t lastStatus = IO_KEYBOARD_STATUS;

  __asm

    di;

    IM 2;

    ld a, INTERUPT_VECTOR_MAP_MSB;
    ld i, a;

    ld hl, _isrKeyboard;
    ld (INTERUPT_VECTOR_MAP_ADDRESS + 4), hl;

  __endasm;

  IO_AYLATCH = IOPORTA;
  IO_AYDATA = INT_MASK_KEYBOARD;

  __asm

    ei;

  __endasm;

  while (true) {

    while (_kbdBufferWritePos == _kbdBufferReadPos);

    key = _kbdBuffer[_kbdBufferReadPos];
    
    printk("Code: %u / 0x%x  Ascii: %c\n", key, key, key);

    if (key == 0x03)
      break;

    _kbdBufferReadPos++;    
  }

  puts("Exiting...");
}
