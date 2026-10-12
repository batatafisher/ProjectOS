#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void *, const void *, size_t);
void *memcpy(void *__restrict, const void *__restrict, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
size_t strlen(const char *);
void hex_to_string(uint32_t value, char *buffer);
void int_to_str(int value, char *str);
char *strcpy(char *dest, const char *source);
int strcmp(const char *s1, const char *s2);
char *strcat(char *dest, const char *source);
int strncmp(const char *s1, const char *s2, size_t n);
char *strstr(const char *haystack, const char *needle);
int split_with(const char *str, const char *delim, char *out[],
               size_t max_tokens, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif
