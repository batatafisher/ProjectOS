#include "bad_apple_frames.h" // Your massive 13MB header file!
#include "commands.h"
#include <drivers/pit.h>
#include <kernel/shell.h>
#include <stdint.h>

const char *badapple(char **argv, int argc) {
  uint16_t *vga_memory = (uint16_t *)0xC00B8000;
  uint8_t color = 0x0F; // White text on black background

  for (int frame = 0; frame < bad_apple_total_frames; frame++) {

    // Render the frame directly to VGA memory to prevent flickering
    for (int i = 0; i < 2000; i++) {
      vga_memory[i] = (color << 8) | bad_apple_frames[frame][i];
    }
    sleep(33);
  }
  shell("clear");
  return "";
}
