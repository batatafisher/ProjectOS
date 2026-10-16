#include "commands/commands.h"
#include <input/keyboard.h>
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static char buffer[128];
static size_t buffer_len;
static char response[256];

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
    {.name = "put", .description = "write to a file", .command = put}};

int num_commands = sizeof(command_registry) / sizeof(shell_command_t);

const char *shell_get_cwd(void) { return current_directory; }

int shell_set_cwd(const char *path) {
  size_t len = strlen(path);

  if (len >= CD_PATH_MAX)
    return -1;

  memcpy(current_directory, path, len + 1);
  return 0;
}

void clear_shell_buffer() {
  buffer[0] = '\0';
  buffer_len = 0;
  shell_command_ready = 0;
}

void init_shell() { clear_shell_buffer(); }

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

char *shell(char *command) {
  char *argv[16];
  int argc = split_shell_command(command, argv, 16);
  if (argc == 0)
    strcpy(response, "ERROR: no command was provided");
  else {
    int i = 0;
    while (strcmp(argv[0], command_registry[i].name) != 0) {
      i++;
      if (i >= num_commands) {
        strcpy(response, argv[0]);
        strcat(response, ": command not found\n");
        clear_shell_buffer();
        return response;
      }
    }
    strcpy(response, command_registry[i].command(argv + 1, argc - 1));
  }
  clear_shell_buffer();
  return strcat(response, "\n");
}

void shell_buffer(char c) {
  if (c == '\b' && buffer_len != 0)
    buffer[--buffer_len] = '\0';

  else if (c == '\n') {
    shell_command_ready = 1;
  }

  else if (buffer_len < sizeof(buffer) - 1) {
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
    printf("%s", shell(buffer));
    printf("ProjectOS> ");
  }
}
