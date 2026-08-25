#include <kernel/pic.h>
#include <stdio.h>

void hardware_interrupt_handler(int irq_number) {
  uint8_t IRQline = irq_number - 32;
  printf("irq_number %d fired\n", IRQline);
  PIC_sendEOI(IRQline);
}
