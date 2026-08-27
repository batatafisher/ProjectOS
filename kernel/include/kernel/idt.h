#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H

#include <iso646.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t isr_low;   // The lower 16 bits of the ISR's address
  uint16_t kernel_cs; // The GDT segment selector that the CPU will load into CS
                      // before calling the ISR
  uint8_t reserved;   // Set to zero
  uint8_t attributes; // Type and attributes; see the IDT page
  uint16_t isr_high;  // The higher 16 bits of the ISR's address
} __attribute__((packed)) idt_entry_t;

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) idtr_t;

typedef struct {
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t esp;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;
  uint32_t interrupt_number;
  uint32_t error_code;
  uint32_t eip;
  uint32_t cs;
  uint32_t eflags;
} interrupt_frame_t;

__attribute__((noreturn)) void exception_handler(interrupt_frame_t *frame);

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);
void init_idt(void);

#endif
