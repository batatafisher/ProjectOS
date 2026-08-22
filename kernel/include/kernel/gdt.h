#ifndef _KERNEL_GDT_H
#define _KERNEL_GDT_H

#include <stdint.h>

struct GDT {
  uint32_t base;       // Base address of the segment
  uint32_t limit;      // Limit of the segment
  uint8_t access_byte; // Access byte (type, privilege level, present)
  uint8_t flags;       // Flags (granularity, size, etc.)
};

void init_gdt();

#endif // !_KERNEL_GDT_H
