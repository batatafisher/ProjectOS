#include <string.h>

// Converts a 32-bit? integer into a hex string.
void hex_to_string(uint32_t value, char *buffer) {

  for (int i = 0; i < 8; i++) {
    uint8_t nibble = (value >> ((7 - i) * 4)) & 0x0F;

    if (nibble < 10) {
      buffer[i] = nibble + '0';
    } else {
      buffer[i] = (nibble - 10) + 'A';
    }
  }

  buffer[8] = '\0';
}
