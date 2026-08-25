#include <drivers/keyboard.h>
#include <kernel/klib.h>
#include <kernel/shell.h>
#include <stdint.h>
#include <stdio.h>

#define ACK 0xFA
#define RESEND 0xFE

const char kbdus[128] = {
    0,    27,  '1', '2', '3',  '4', '5', '6', '7',  '8', /* 0 - 9 */
    '9',  '0', '-', '=', '\b',                           /* Backspace */
    '\t',                                                /* Tab */
    'q',  'w', 'e', 'r',                                 /* 16 - 19 */
    't',  'y', 'u', 'i', 'o',  'p', '[', ']', '\n',      /* Enter key */
    0,                                                   /* 29   - Control */
    'a',  's', 'd', 'f', 'g',  'h', 'j', 'k', 'l',  ';', /* 30 - 39 */
    '\'', '`', 0,                                        /* Left shift */
    '\\', 'z', 'x', 'c', 'v',  'b', 'n',                 /* 43 - 49 */
    'm',  ',', '.', '/', 0,                              /* Right shift */
    '*',  0,                                             /* Alt */
    ' ',                                                 /* Space bar */
    0,                                                   /* Caps lock */
    0,    0,   0,   0,   0,    0,   0,   0,   0,    0,   /* F1 - F10 */
    0,                                                   /* 69 - Num lock*/
    0,                                                   /* Scroll Lock */
    0,                                                   /* Home key */
    0,                                                   /* Up Arrow */
    0,                                                   /* Page Up */
    '-',  0,                                             /* Left Arrow */
    0,    0,                                             /* Right Arrow */
    '+',  0,                                             /* 79 - End key*/
    0,                                                   /* Down Arrow */
    0,                                                   /* Page Down */
    0,                                                   /* Insert Key */
    0,                                                   /* Delete Key */
    0,    0,   0,   0,                                   /* F11 Key */
    0,                                                   /* F12 Key */
    0, /* All other keys are undefined */
};

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
  if (scan_code <= 0x80) {
    char c = kbdus[scan_code];
    if (c != 0) {
      printf("%c", c);
      shell_buffer(c);
    }
  }
}
