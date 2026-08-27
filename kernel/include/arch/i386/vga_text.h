#ifndef _ARCH_I386_VGA_TEXT_H
#define _ARCH_I386_VGA_TEXT_H

#include <stddef.h>
#include <stdint.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_setcolor(uint8_t color);
void clear_terminal();

#endif
