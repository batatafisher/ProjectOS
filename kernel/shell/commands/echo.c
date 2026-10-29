#include "commands.h"
#include <stdio.h>

void echo(char *args[], int argc) {
  if (argc == 0 || argc > 1) {
    printf("USAGE: echo [message]\n");
    return;
  }
  printf("%s\n", args[0]);
  return;
}
