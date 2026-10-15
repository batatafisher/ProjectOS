#ifndef COMMANDS_H
#define COMMANDS_H

typedef struct {
  const char *(*command)(char *args[], int argc);
  char *description;
  char *name;
} shell_command_t;

const char *echo(char *args[], int argc);
// const char *badapple(char *args[], int argc);
const char *clear(char *args[], int argc);
const char *ls(char *args[], int argc);
const char *cd(char *args[], int argc);
const char *touch(char *args[], int argc);
const char *pwd(char *args[], int argc);
const char *mk_dir(char *args[], int argc);

#endif
