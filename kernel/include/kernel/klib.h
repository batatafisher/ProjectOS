#ifndef _KERNEL_KLIB_H
#define _KERNEL_KLIB_H

#include <stdbool.h>
#include <stdint.h>

static inline int test_bit(int nr, const uint8_t *addr) {
  return (addr[nr >> 3] & (1 << (nr & 7))) != 0;
}

static inline void set_bit(int nr, uint8_t *addr) {
  addr[nr >> 3] |= (1 << (nr & 7));
}

static inline void clear_bit(int nr, uint8_t *addr) {
  addr[nr >> 3] &= ~(1 << (nr & 7));
}

static inline uint64_t round2(int nr,
                              uint64_t num) { // nr must be a multiple of two
  return (num + nr - 1) & ~(nr - 1);
}

static inline uint64_t floor2(int nr,
                              uint64_t num) { // nr must be a multiple of two
  return num & ~(nr - 1);
}

#endif
