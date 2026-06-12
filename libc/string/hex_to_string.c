#include <string.h>

// Converts a 32-bit integer into a hex string.
// Ensure your buffer is at least 11 bytes long (e.g., "0xFFFFFFFF\0")
void hex_to_string(uint32_t value, char *buffer) {
  // 1. Add the standard hex prefix
  buffer[0] = '0';
  buffer[1] = 'x';

  // 2. Loop through all 8 hex characters (32 bits / 4 bits)
  for (int i = 0; i < 8; i++) {
    // Shift the specific nibble we want down to the bottom 4 bits,
    // then mask it with 0x0F (1111 in binary) to isolate it.
    uint8_t nibble = (value >> ((7 - i) * 4)) & 0x0F;

    // 3. Convert the mathematical value to an ASCII character
    if (nibble < 10) {
      // For numbers 0-9
      buffer[2 + i] = nibble + '0';
    } else {
      // For letters A-F
      buffer[2 + i] = (nibble - 10) + 'A';
    }
  }

  // 4. Cap the string with a null terminator!
  buffer[10] = '\0';
}
