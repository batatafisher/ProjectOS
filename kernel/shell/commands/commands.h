#ifndef COMMANDS_H
#define COMMANDS_H

typedef struct {
  void (*command)(char *args[], int argc);
  char *description;
  char *name;
} shell_command_t;

void echo(char *args[], int argc);
// const char *badapple(char *args[], int argc);
void clear(char *args[], int argc);
void ls(char *args[], int argc);
void cd(char *args[], int argc);
void touch(char *args[], int argc);
void pwd(char *args[], int argc);
void mk_dir(char *args[], int argc);
void cat(char *args[], int argc);
void put(char *args[], int argc);
void rm(char *args[], int argc);

void heap(char *args[], int argc);

#endif
