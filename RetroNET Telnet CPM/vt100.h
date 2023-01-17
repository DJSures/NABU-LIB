#pragma once

#define VT100_WIDTH 39
#define VT100_HEIGHT 24

void vt100_init(void (*send_response)(char* str));
void vt100_putc(uint8_t ch);
void vt100_puts(const char* str);

