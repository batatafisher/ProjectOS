#include <arch/i386/vga_text.h>
#include <string.h>

// === a struct of functions for forward compatibility ===
typedef struct {
  void (*putchar)(char c);
  void (*set_color)(uint8_t color);
  void (*clear)(void);
  void (*initialize)(void);
} tty_driver_t;

tty_driver_t vga_text_driver = {.putchar = terminal_putchar,
                                .set_color = terminal_setcolor,
                                .initialize = terminal_initialize,
                                .clear = clear_terminal};

tty_driver_t *active_tty = &vga_text_driver;

void terminal_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++)
    active_tty->putchar(data[i]);
}

void terminal_writestring(const char *data) {
  terminal_write(data, strlen(data));
}

void clear_screen() { active_tty->clear(); }

void init_tty() { active_tty->initialize(); }
