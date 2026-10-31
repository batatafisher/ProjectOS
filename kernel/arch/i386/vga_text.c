#include <kernel/asmutils.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <arch/i386/vga_text.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static volatile uint16_t *const VGA_MEMORY = (volatile uint16_t *)0xC00B8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static volatile uint16_t *terminal_buffer;

static int current_text_count = 0;

void terminal_initialize(void) {
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = vga_entry_color(
      VGA_COLOR_LIGHT_GREY,
      VGA_COLOR_BLUE); // <--- BECAUSE OF THIS LINE 2 HOUR OF MY LIFE WAS WASTED
  terminal_buffer = VGA_MEMORY;
  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
  }
}

void vga_cursor_set(uint16_t pos) {
  outb(0x3D4, 0x0E);
  outb(0x3D5, (pos >> 8) & 0xFF);

  outb(0x3D4, 0x0F);
  outb(0x3D5, pos & 0xFF);
}

void terminal_setcolor(uint8_t color) { terminal_color = color; }

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
  const size_t index = y * VGA_WIDTH + x;
  terminal_buffer[index] = vga_entry(c, color);
}

void clear_terminal() {

  uint16_t blank = (terminal_color << 8) | ' ';

  for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
    terminal_buffer[i] = blank;
  }
  current_text_count = 0;
  terminal_column = 0;
  terminal_row = 0;
  vga_cursor_set(terminal_row * VGA_WIDTH + terminal_column);
}

void terminal_scroll() {
  for (size_t y = 1; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++)
      terminal_buffer[(y - 1) * VGA_WIDTH + x] =
          terminal_buffer[y * VGA_WIDTH + x];
  }
  for (size_t i = 0; i < VGA_WIDTH; i++)
    terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + i] =
        vga_entry(' ', terminal_color);
  terminal_row = VGA_HEIGHT - 1;
  terminal_column = 0;
}

void terminal_putchar(char c) {
  unsigned char uc = c;
  if (c == '\n') {
    current_text_count = 0;
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT)
      terminal_scroll();
    vga_cursor_set(terminal_row * VGA_WIDTH + terminal_column);
    return;
  }
  if (c == '\b') {
    if (terminal_column != 0) {
      current_text_count--;
      terminal_column--;
      terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    } else if (terminal_column == 0 && terminal_row != 0 &&
               current_text_count > 0) {
      current_text_count--;
      terminal_column = VGA_WIDTH - 1;
      terminal_row--;
      terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    }
    vga_cursor_set(terminal_row * VGA_WIDTH + terminal_column);
    return;
  }
  current_text_count++;
  terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
  if (++terminal_column == VGA_WIDTH) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT)
      terminal_scroll();
  }
  vga_cursor_set(terminal_row * VGA_WIDTH + terminal_column);
}
