#include <kernel/asmutils.h>
#include <kernel/panic.h>
#include <kernel/tty.h>
#include <stdio.h>

void kerror(const char *format, ...) {
  clear_screen();
  va_list parameters;
  va_start(parameters, format);
  vprintf(format, parameters);
  va_end(parameters);
  cli();
  hlt();
}
