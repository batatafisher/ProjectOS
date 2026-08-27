#ifndef _DRIVER_PIT_H
#define _DRIVER_PIT_H

#include <stdint.h>

void pit_handler();
void init_pit(uint16_t hz);
void sleep(uint32_t ms);

#endif
