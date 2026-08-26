#include "commands.h"

const char *echo(char *args[], int argc) {
  if (argc == 0 || argc > 1) {
    return "USAGE: echo [message]";
  }
  return args[0];
}
