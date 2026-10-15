#include <string.h>

int split_with(const char *str, const char *delim, char *out[],
               size_t max_tokens, size_t max_len) {
  if (!str || !delim || !out || max_tokens == 0)
    return 0;

  size_t delim_len = strlen(delim);
  size_t token_count = 0;
  const char *cursor = str;

  while (*cursor != '\0' && token_count < max_tokens) {
    const char *next_delim = strstr(cursor, delim);
    size_t token_len =
        next_delim ? (size_t)(next_delim - cursor) : strlen(cursor);

    if (token_len >= max_len) {
      token_len = max_len - 1;
    }
    if (!out[token_count]) {
      return -1;
    }
    // Copy directly into pre-allocated buffer pointer out[token_count]
    memcpy(out[token_count], cursor, token_len);
    out[token_count][token_len] = '\0';

    token_count++;

    if (!next_delim)
      break;
    cursor = next_delim + delim_len;
  }

  return token_count;
}
