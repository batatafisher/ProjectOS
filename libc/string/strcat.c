#include <string.h>

// this one start from the finish of dest and copy whatever in source there
char *strcat(char *dest, const char *source) {
  int i = strlen(dest);
  int j = 0;
  while (source[j] != '\0') {
    dest[i++] = source[j++];
  }
  dest[i] = '\0';
  return dest;
}
