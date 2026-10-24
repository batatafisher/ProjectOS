#ifndef _KERNEL_ASMUTILS_H
#define _KERNEL_ASMUTILS_H

#include <stdbool.h>
#include <stdint.h>

static inline void __native_flush_tlb_single(unsigned long addr) {
  asm volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

// ======== I/O access ========

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
  /* There's an outb %al, $imm8 encoding, for compile-time constant port numbers
   * that fit in 8b. (N constraint). Wider immediate constants would be
   * truncated at assemble-time (e.g. "i" constraint). The  outb  %al, %dx
   * encoding is the only option for all other cases. %1 expands to %dx because
   * port  is a uint16_t.  %w1 could be used if we had the port number a wider C
   * type */
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

static inline void insw(uint16_t port, void *addr, uint32_t word_count) {
  __asm__ volatile("rep insw"
                   : "+D"(addr), "+c"(word_count)
                   : "d"(port)
                   : "memory");
}

static inline void outsw(uint16_t port, void *addr, uint32_t word_count) {
  __asm__ volatile("rep outsw"
                   : "+S"(addr), "+c"(word_count)
                   : "d"(port)
                   : "memory");
}

static inline uint16_t inw(uint16_t port) {
  uint16_t value;

  __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));

  return value;
}

static inline void outw(uint16_t port, uint16_t value) {
  __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline void io_wait(void) { outb(0x80, 0); }

// ======== Interrupts ========

static inline bool are_interrupts_enabled() {
  unsigned long flags;
  asm volatile("pushf\n\t"
               "pop %0"
               : "=g"(flags));
  return flags & (1 << 9);
}

static inline void sti() {
  __asm__ volatile("sti"); // set the interrupt flag
}

static inline void cli() { __asm__ volatile("cli"); }

#endif
