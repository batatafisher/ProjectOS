#include <stdbool.h>
#include <string.h>

#include <arch/i386/vga_text.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t *const VGA_MEMORY = (uint16_t *)0xC00B8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t *terminal_buffer;

void terminal_initialize(void) {
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_LIGHT_BLUE);
  terminal_buffer = VGA_MEMORY;
  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
  }
}

void terminal_setcolor(uint8_t color) { terminal_color = color; }

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
  const size_t index = y * VGA_WIDTH + x;
  terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
  unsigned char uc = c;
  if (c == '\n') {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT)
      terminal_row = 0;
    return;
  }
  if (c == '\b') {
    if (terminal_column != 0) {
      terminal_column--;
      terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    }
    return;
  }
  terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
  if (++terminal_column == VGA_WIDTH) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT)
      terminal_row = 0;
  }
}

void clear_terminal() {
  // Point directly to the VGA text buffer
  uint16_t *vga_memory = (uint16_t *)0xC00B8000;

  // Pre-calculate the blank character with the current terminal color
  uint16_t blank = (terminal_color << 8) | ' ';

  // Blast the entire screen with blanks instantly
  for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
    vga_memory[i] = blank;
  }

  // Reset the internal cursor coordinates
  terminal_column = 0;
  terminal_row = 0;
}
