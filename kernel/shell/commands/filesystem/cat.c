#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <stdint.h>
#include <string.h>

const char *cat(char *args[], int argc) {
  if (argc == 1) {
    char *path = args[0];
    int err;
    static uint8_t buffer[1024];
    if (path[0] == '/') {
      int fd = open(path, 0);
      if (fd >= 0) {
        err = read(fd, buffer, 1023);
        close(fd);
        if (err != NO_ERROR) {
          return "ERROR: couldnt read file";
        }
        return (char *)buffer;
      } else {
        return "ERROR: invalid path";
      }
    } else {
      char full_path[128] = "";
      strcpy(full_path, shell_get_cwd());

      if (strcmp(shell_get_cwd(), "/") != 0)
        strcat(full_path, "/");

      strcat(full_path, path);
      int fd = open(full_path, 0);
      if (fd >= 0) {
        err = read(fd, buffer, 1023);
        close(fd);
        if (err != NO_ERROR) {
          return "ERROR: couldnt read file";
        }

        return (char *)buffer;
      } else {
        return "ERROR: invalid path";
      }
    }
  }
  return "USAGE: cat [path]";
}
