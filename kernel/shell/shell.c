#include "commands/commands.h"
#include <input/keyboard.h>
#include <kernel/asmutils.h>
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define INIT_BUFFER_SIZE 128
#define BUFFER_ADDITION_COUNT 16

static char *buffer;
static size_t buffer_len;
static size_t buffer_size;

static char current_directory[CD_PATH_MAX] = "/";

volatile int shell_command_ready = 0;

// The array of all your shell commands
shell_command_t command_registry[] = {
    {.name = "echo",
     .description = "Prints text back to the terminal",
     .command = echo},
    //{.name = "badapple",
    //.description =
    //    "plays bad apple frames in ascii format directly in the terminal",
    //.command = badapple},
    {.name = "clear",
     .description = "clears the terminal screen",
     .command = clear},
    {.name = "ls",
     .description = "prints the working directores entries to the terminal",
     .command = ls},
    {.name = "cd",
     .description = "changing the working directory",
     .command = cd},
    {.name = "touch", .description = "create a new file", .command = touch},
    {.name = "pwd",
     .description = "get current working directory",
     .command = pwd},
    {.name = "mkdir",
     .description = "creates new directories",
     .command = mk_dir},
    {.name = "rm", .description = "remove files of directories", .command = rm},
    {.name = "cat", .description = "read file contents", .command = cat},
    {.name = "put", .description = "write to a file", .command = put},
    {.name = "heap",
     .description = "somthing related to the heap",
     .command = heap}};

int num_commands = sizeof(command_registry) / sizeof(shell_command_t);

const char *shell_get_cwd(void) { return current_directory; }

int shell_set_cwd(const char *path) {
  size_t len = strlen(path);

  if (len >= CD_PATH_MAX)
    return -1;

  memcpy(current_directory, path, len + 1);
  // TODO: the path needs to be normalized (/etc/.. should be /)
  return 0;
}

void clear_shell_buffer() {
  kfree(buffer);
  buffer = kmalloc(sizeof(char) * INIT_BUFFER_SIZE);
  buffer_size = INIT_BUFFER_SIZE;
  buffer[0] = '\0';
  buffer_len = 0;
  shell_command_ready = 0;
}

char *get_full_path(const char *path) {
  if (path[0] == '/') {
    if (strlen(path) >= CD_PATH_MAX)
      return "err";
    return path;
  } else {
    if (strlen(path) + strlen(shell_get_cwd()) >= CD_PATH_MAX)
      return "err";
    static char full_path[CD_PATH_MAX] = "";
    strcpy(full_path, shell_get_cwd());

    strcat(full_path, path);
    return full_path;
  }
}

void init_shell() {
  buffer = NULL;
  clear_shell_buffer();
}

int split_shell_command(char *str, char *args[], int max_args) {
  int argc = 0;
  char *p = str;

  while (*p != '\0' && argc < max_args) {
    while (*p == ' ')
      p++;

    if (*p == '\0')
      break;

    args[argc++] = p;

    while (*p != '\0' && *p != ' ')
      p++;

    if (*p == ' ') {
      *p = '\0';
      p++;
    }
  }
  return argc;
}

void shell(char *command) {
  char *argv[16];
  int argc = split_shell_command(command, argv, 16);
  if (argc == 0)
    printf("ERROR: no command was provided\n");
  else {
    int i = 0;
    while (strcmp(argv[0], command_registry[i].name) != 0) {
      i++;
      if (i >= num_commands) {
        printf("%s : command not found\n", argv[0]);
        clear_shell_buffer();
        return;
      }
    }
    command_registry[i].command(argv + 1, argc - 1);
  }
  clear_shell_buffer();

  return;
}

void shell_buffer(char c) {
  if (c == '\b' && buffer_len == 0) {
    return;
  }

  if (c == '\b' && buffer_len != 0)
    buffer[--buffer_len] = '\0';

  else if (c == '\n') {
    shell_command_ready = 1;
  }

  else {
    if (buffer_len + 1 >= buffer_size) {
      buffer_size += BUFFER_ADDITION_COUNT;
      buffer = krealloc(buffer, buffer_size);
    }
    buffer[buffer_len++] = c;
    buffer[buffer_len] = '\0';
  }

  putchar(c);
}

int once = 1;
void run_shell() {
  if (once) {
    printf("ProjectOS> ");
    once = 0;
  }
  char c = keyboard_dequeue();
  while (c != 0) {

    shell_buffer(c);

    c = keyboard_dequeue();
  }
  if (shell_command_ready) {
    shell(buffer);
    printf("\nProjectOS> ");
  }
}
