#include "commands.h"
#include <stdio.h>

void echo(char *args[], int argc) {
  if (argc == 0 || argc > 1) {
    printf("USAGE: echo [message]");
    return;
  }
  printf("%s", args[0]);
  return;
}
