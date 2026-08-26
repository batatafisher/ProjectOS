#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>

void terminal_write(const char *data, size_t size);
void terminal_writestring(const char *data);
void init_tty();

#endif
