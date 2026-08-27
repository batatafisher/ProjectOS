#include <drivers/init_drivers.h>
#include <drivers/keyboard.h>
#include <drivers/pit.h>
#include <stdint.h>

void init_drivers(uint16_t pit_hz) {
  init_keyboard();
  init_pit(pit_hz);
}
