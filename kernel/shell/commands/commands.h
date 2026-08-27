#ifndef COMMANDS_H
#define COMMANDS_H

typedef struct {
  const char *(*command)(char *args[], int argc);
  char *description;
  char *name;
} shell_command_t;

const char *echo(char *args[], int argc);
const char *badapple(char *args[], int argc);
const char *clear(char *args[], int argc);

#endif
