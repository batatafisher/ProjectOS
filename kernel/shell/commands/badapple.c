#include "bad_apple_frames.h" // Your massive 13MB header file!
#include "commands.h"
#include <stdint.h>

const char *badapple(char **argv, int argc) {
  uint16_t *vga_memory = (uint16_t *)0xC00B8000;
  uint8_t color = 0x0F; // White text on black background

  for (int frame = 0; frame < bad_apple_total_frames; frame++) {

    // Render the frame directly to VGA memory to prevent flickering
    for (int i = 0; i < 2000; i++) {
      vga_memory[i] = (color << 8) | bad_apple_frames[frame][i];
    }

    // TODO: We need a sleep() function here!
    // Right now, the CPU will render all 6500 frames in 0.2 seconds.
    for (volatile int wait = 0; wait < 10000000; wait++) {
      // Terrible temporary hack to slow down the CPU
    }
  }
  return "";
}
