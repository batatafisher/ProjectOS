#include "commands/commands.h"
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static char buffer[128];
static size_t buffer_len;
static char response[256];

volatile int shell_command_ready = 0;

// The array of all your shell commands
shell_command_t command_registry[] = {
    {.name = "echo",
     .description = "Prints text back to the terminal",
     .command = echo}};

int num_commands = sizeof(command_registry) / sizeof(shell_command_t);

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

char *shell() {
  char *argv[16];
  int argc = split_shell_command(buffer, argv, 16);
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
