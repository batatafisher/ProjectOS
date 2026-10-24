#include <drivers/keyboard.h>
#include <input/keyboard.h>
#include <kernel/asmutils.h>
#include <kernel/shell.h>
#include <stdint.h>

#define ACK 0xFA
#define RESEND 0xFE

void init_keyboard() {
  uint8_t data = inb(0x60);
  outb(0x64, 0x20); // request to send the configuration byte
  data = inb(0x60); // the configuration byte should be here
  data |= 1; // set bit 0 to 1 to tell the unit to start writing keystrocks to
             // the according port
  outb(0x64,
       0x60); // to tell the controller we are about to send a comand toport x60
  outb(0x60, data);
}

void keyboard_handler() {
  int scan_code = inb(0x60);
  keyboard_event(scan_code);
}
