#include <drivers/pit.h>
#include <kernel/asmutils.h>
#include <stdint.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43
#define PIT_FREQ 1193182

// CHANNELS
#define CHANNEL0 0
#define CHANNEL1 1 << 6 // 01000000
#define CHANNEL2 2 << 6 // 10000000

// access mode
#define LATCH_COUNT 0
#define LSB_ONLY 1 << 4     // 00010000
#define MSB_ONLY 2 << 4     // 00100000
#define LSB_THEN_MSB 3 << 4 // 00110000

// operating mode
#define INT_ON_TERM_COUNT 0
#define HARDWARE_RETRIGGERABLE_SOMTHING 1 << 1 // 00000010
#define RATE_GENERATOR 2 << 1                  // 00000100
#define SQUARE_WAVE_GENERATOR 3 << 1           // 00000110
#define SOFTWARE_TRIGGERED_STROBE 4 << 1       // 00001000
#define HARDWARE_TRIGGERED_STROBE 5 << 1       // 00001010

static uint16_t pit_hz;

void init_pit(uint16_t hz) {
  pit_hz = hz;
  uint16_t devisor = PIT_FREQ / hz;
  uint8_t control_word = CHANNEL0 | LSB_THEN_MSB | SQUARE_WAVE_GENERATOR;
  outb(PIT_COMMAND, control_word);
  outb(PIT_CHANNEL0, devisor & 0xFF);
  outb(PIT_CHANNEL0, (devisor >> 8) & 0xFF);
}

static uint64_t tick = 0;

void pit_handler() { tick++; }

void sleep(uint32_t ms) {
  uint64_t target = tick + (ms * pit_hz) / 1000;

  while (tick < target) {
    __asm__ volatile("hlt");
  }
}
