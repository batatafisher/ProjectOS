#include "commands.h"
#include <kernel/tty.h>
#include <string.h>

char response[28] = {""};

const char *clear(char *argv[], int argc) {
  if (argc > 0) {
    strcat(response, "ERROR expected 0 arguments found ");
    char str[5];
    int_to_str(argc, str);
    strcat(response, str);
  } else
    clear_screen();
  return response;
}
