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
    value *= -1;
  }

  while (value > 0) {
    temp[i++] = '0' + (value % 10);
    value /= 10;
  }

  if (negative)
    str[j++] = '-';

  while (i > 0) {
    str[j++] = temp[--i];
  }

  str[j] = '\0';
}
