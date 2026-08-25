#ifndef _KERNEL_PIC_H
#define _KERNEL_PIC_H

#include <stdint.h>

void init_PIC(int offset1, int offset2);
void PIC_disable(void);
void PIC_sendEOI(uint8_t irq);

void IRQ_mask(uint8_t IRQline);
void IRQ_unmask(uint8_t IRQline);

#endif
