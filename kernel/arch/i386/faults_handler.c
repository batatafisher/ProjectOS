#include <kernel/idt.h>
#include <kernel/tty.h>
#include <stdint.h>
#include <stdio.h>

void page_fault(interrupt_frame_t *frame) {
  uint32_t faulting_address;
  __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_address));

  clear_screen();

  printf("KERNEL PANIC: Committed a crime against the memory unit (Page "
         "Fault)\n\n");

  printf("EIP (Instruction): 0x%x\n", frame->eip);
  printf("CR2 (Target Addr): 0x%x\n", faulting_address);
  printf("Error Code:        0x%x\n\n", frame->error_code);

  printf("System halted.\n");

  __asm__ volatile("cli; hlt");
}

void general_fault_handler(interrupt_frame_t *frame) {
  uint32_t faulting_address;
  __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_address));

  clear_screen();

  printf("KERNEL PANIC: I dont fucking know what happened but here are some "
         "helpful numbers\nGood luck bro.\n\n");

  printf("EIP (Instruction): 0x%x\n", frame->eip);
  printf("CR2 (Target Addr): 0x%x\n", faulting_address);
  printf("Error Code:        0x%x\n\n", frame->error_code);

  printf("System halted.\n");

  __asm__ volatile("cli; hlt");
}
