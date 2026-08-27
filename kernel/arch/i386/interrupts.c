#include <drivers/keyboard.h>
#include <drivers/pit.h>
#include <kernel/pic.h>

void hardware_interrupt_handler(int irq_number) {
  uint8_t IRQline = irq_number - 32;
  if (IRQline == 0)
    pit_handler();
  else if (IRQline == 1)
    keyboard_handler();
  PIC_sendEOI(IRQline);
}
