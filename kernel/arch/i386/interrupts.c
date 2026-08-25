#include <drivers/keyboard.h>
#include <kernel/pic.h>
#include <stdio.h>

void hardware_interrupt_handler(int irq_number) {
  uint8_t IRQline = irq_number - 32;

  if (IRQline == 1)
    keyboard_handler();
  PIC_sendEOI(IRQline);
}
