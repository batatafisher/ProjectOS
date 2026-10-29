#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool print(const char *data, size_t length) {
  const unsigned char *bytes = (const unsigned char *)data;
  for (size_t i = 0; i < length; i++)
    if (putchar(bytes[i]) == EOF)
      return false;
  return true;
}

int vprintf(const char *restrict format, va_list parameters) {
  int written = 0;

  while (*format != '\0') {
    size_t maxrem = INT_MAX - written;
    if (format[0] != '%' || format[1] == '%') {
      if (format[0] == '%')
        format++;
      size_t amount = 1;
      while (format[amount] && format[amount] != '%')
        amount++;
      if (maxrem < amount) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!print(format, amount))
        return -1;
      format += amount;
      written += amount;
      continue;
    }

    const char *format_begun_at = format++;

    if (*format == 'c') {
      format++;
      char c = (char)va_arg(parameters, int /* char promotes to int */);
      if (!maxrem) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!print(&c, sizeof(c)))
        return -1;
      written++;
    } else if (*format == 's') {
      format++;
      const char *str = va_arg(parameters, const char *);
      if (str == NULL)
        str = "(nill)";
      size_t len = strlen(str);
      if (maxrem < len) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!print(str, len))
        return -1;
      written += len;
    } else if (*format == 'x') {
      format++;
      uint32_t value = va_arg(parameters, uint32_t);
      char buffer[9];
      hex_to_string(value, buffer);
      size_t len = strlen(buffer);
      if (maxrem < len) {
        // TODO Set errno to EOVERFLOW
        return -1;
      }
      if (!print(buffer, len))
        return -1;
      written += len;
    } else if (*format == 'd') {
      format++;
      uint32_t value = va_arg(parameters, int);
      char buffer[12];
      int_to_str(value, buffer);
      size_t len = strlen(buffer);
      if (maxrem < len) {
        // TODO: set errno to EOVERFLOW
        return -1;
      }
      if (!print(buffer, len))
        return -1;
      written += len;
    } else {
      format = format_begun_at;
      size_t len = strlen(format);
      if (maxrem < len) {
        // TODO: Set errno to EOVERFLOW.
        return -1;
      }
      if (!print(format, len))
        return -1;
      written += len;
      format += len;
    }
  }

  return written;
}
