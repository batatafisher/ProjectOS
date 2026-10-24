#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char *cat(char *args[], int argc) {
  if (argc == 1) {
    char *path = args[0];
    int err;
    static uint8_t buffer[1024];
    char *full_path = get_full_path(path);
    if (strcmp(full_path, "err") == 0)
      return "ERROR: path name too long";
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
  return "USAGE: cat [path]";
}
