#ifndef _ARCH_I386_FAULTS_HANDLER_H
#define _ARCH_I386_FAULTS_HANDLER_H

#include <kernel/idt.h>

void page_fault(interrupt_frame_t *frame);
void general_fault_handler(interrupt_frame_t *frame);

#endif
