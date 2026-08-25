#include <stddef.h>
#include <string.h>

char *strcpy(char *dest, const char *source) {
  char *ret = dest;

  while ((*dest++ = *source++) != '\0')
    ;

  return ret;
}
