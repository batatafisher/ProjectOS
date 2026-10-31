#include <string.h>

void int_to_str(int value, char *str) {
  char temp[12];
  int negative = 0;
  int i = 0;
  int j = 0;

  if (value == 0) {
    str[0] = '0';
    str[1] = '\0';
    return;
  }

  if (value < 0) {
    negative = 1;
  }

  unsigned int u = negative ? -(unsigned)value : value;

  while (u > 0) {
    temp[i++] = '0' + (u % 10);
    u /= 10;
  }

  if (negative)
    str[j++] = '-';

  while (i > 0) {
    str[j++] = temp[--i];
  }

  str[j] = '\0';
}
