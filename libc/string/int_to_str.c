#include <string.h>

void int_to_str(int value, char *str) {
  char temp[12];
  int i = 0;
  int j = 0;

  if (value == 0) {
    str[0] = '0';
    str[1] = '\0';
    return;
  }

  while (value > 0) {
    temp[i++] = '0' + (value % 10);
    value /= 10;
  }

  while (i > 0) {
    str[j++] = temp[--i];
  }

  str[j] = '\0';
}
