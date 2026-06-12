#include <kernel/asmutils.h>

static inline void __native_flush_tlb_single(unsigned long addr) {
  asm volatile("invlpg (%0)" ::"r"(addr) : "memory");
}
