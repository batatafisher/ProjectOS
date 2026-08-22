#include <kernel/gdt.h>

// This MUST be static/global so it doesn't get destroyed when init_gdt returns!
static uint8_t raw_gdt[5 * 8];

void encodeGdtEntry(uint8_t *target, struct GDT source) {
  // Check the limit to make sure that it can be encoded
  if (source.limit > 0xFFFFF) {
    // kerror("GDT cannot encode limits larger than 0xFFFFF");
  }

  // Encode the limit
  target[0] = source.limit & 0xFF;
  target[1] = (source.limit >> 8) & 0xFF;
  target[6] = (source.limit >> 16) & 0x0F;

  // Encode the base
  target[2] = source.base & 0xFF;
  target[3] = (source.base >> 8) & 0xFF;
  target[4] = (source.base >> 16) & 0xFF;
  target[7] = (source.base >> 24) & 0xFF;

  // Encode the access byte
  target[5] = source.access_byte;

  // Encode the flags
  target[6] |= (source.flags << 4);
}

void init_gdt() {
  // Allocate 5 slots, not 4
  struct GDT gdt_entries[5];

  gdt_entries[0] = (struct GDT){0, 0, 0, 0};            // Null
  gdt_entries[1] = (struct GDT){0, 0xfffff, 0x9a, 0xc}; // Kernel Code
  gdt_entries[2] = (struct GDT){0, 0xfffff, 0x92, 0xc}; // Kernel Data
  gdt_entries[3] = (struct GDT){0, 0xfffff, 0xfa, 0xc}; // User Code
  gdt_entries[4] = (struct GDT){0, 0xfffff, 0xf2, 0xc}; // User Data

  // Loop through and encode the structs into the raw CPU byte array
  for (int i = 0; i < 5; i++) {
    encodeGdtEntry(&raw_gdt[i * 8], gdt_entries[i]);
  }

  extern void setGdt(uint32_t limit,
                     uint32_t base); // Declare the external function

  // Calculate the size of the raw array minus 1
  uint32_t limit = (5 * 8) - 1;
  // Get the memory address of the raw array
  uint32_t base = (uint32_t)&raw_gdt;

  // Fire the assembly instruction!
  setGdt(limit, base);
}
