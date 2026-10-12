#include <string.h>

char *strstr(const char *haystack, const char *needle) {
  if (!*needle) {
    return (char *)haystack;
  }

  for (; *haystack != '\0'; haystack++) {
    if (*haystack == *needle) {
      const char *h = haystack;
      const char *n = needle;

      while (*h && *n && *h == *n) {
        h++;
        n++;
      }

      if (!*n) {
        return (char *)haystack;
      }
    }
  }

  return NULL;
}
