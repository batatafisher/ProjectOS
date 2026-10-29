#include "commands.h"
#include <kernel/tty.h>
#include <stdio.h>
#include <string.h>

void clear(char *argv[], int argc) {
  if (argc > 0) {
    printf("ERROR expected 0 arguments found %d\n", argc);
  } else
    clear_screen();
  return;
}
