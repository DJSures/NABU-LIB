#pragma once

#define VT100_SCREEN_WIDTH 40
#define VT100_SCREEN_HEIGHT 23
#define VT100_HEIGHT VT100_SCREEN_HEIGHT
#define VT100_WIDTH VT100_SCREEN_WIDTH

void vt100_init(void (*send_response)(char* str));
void vt100_putc(uint8_t ch);
void vt100_puts(const char* str);

