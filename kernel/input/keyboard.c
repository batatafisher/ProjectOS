#include <input/keyboard.h>
#include <stdint.h>

#define MOD_SHIFT (1 << 0)
#define MOD_CTRL (1 << 1)
#define MOD_ALT (1 << 2)
#define MOD_META (1 << 3)
#define MOD_CAPS (1 << 4)

#define KBD_BUFFER_SIZE 256

static const key_t scancode_map[128] = {
    [0x01] = KEY_ESC,

    [0x02] = KEY_1,
    [0x03] = KEY_2,
    [0x04] = KEY_3,
    [0x05] = KEY_4,
    [0x06] = KEY_5,
    [0x07] = KEY_6,
    [0x08] = KEY_7,
    [0x09] = KEY_8,
    [0x0A] = KEY_9,
    [0x0B] = KEY_0,

    [0x0C] = KEY_MINUS,
    [0x0D] = KEY_EQUAL,
    [0x0E] = KEY_BACKSPACE,

    [0x0F] = KEY_TAB,

    [0x10] = KEY_Q,
    [0x11] = KEY_W,
    [0x12] = KEY_E,
    [0x13] = KEY_R,
    [0x14] = KEY_T,
    [0x15] = KEY_Y,
    [0x16] = KEY_U,
    [0x17] = KEY_I,
    [0x18] = KEY_O,
    [0x19] = KEY_P,

    [0x1C] = KEY_ENTER,

    [0x1D] = KEY_LEFT_CTRL,

    [0x1E] = KEY_A,
    [0x1F] = KEY_S,
    [0x20] = KEY_D,
    [0x21] = KEY_F,
    [0x22] = KEY_G,
    [0x23] = KEY_H,
    [0x24] = KEY_J,
    [0x25] = KEY_K,
    [0x26] = KEY_L,

    [0x2A] = KEY_LEFT_SHIFT,
    [0x2B] = KEY_BACKSLASH,

    [0x2C] = KEY_Z,
    [0x2D] = KEY_X,
    [0x2E] = KEY_C,
    [0x2F] = KEY_V,
    [0x30] = KEY_B,
    [0x31] = KEY_N,
    [0x32] = KEY_M,

    [0x36] = KEY_RIGHT_SHIFT,

    [0x38] = KEY_LEFT_ALT,
    [0x39] = KEY_SPACE,
    [0x3A] = KEY_CAPS_LOCK,

    [0x3B] = KEY_F1,
    [0x3C] = KEY_F2,
    [0x3D] = KEY_F3,
    [0x3E] = KEY_F4,
    [0x3F] = KEY_F5,
    [0x40] = KEY_F6,
    [0x41] = KEY_F7,
    [0x42] = KEY_F8,
    [0x43] = KEY_F9,
    [0x44] = KEY_F10,

    [0x45] = KEY_NUM_LOCK,
    [0x46] = KEY_SCROLL_LOCK,

    [0x47] = KEY_KP_7,
    [0x48] = KEY_KP_8,
    [0x49] = KEY_KP_9,
    [0x4A] = KEY_KP_MINUS,

    [0x4B] = KEY_KP_4,
    [0x4C] = KEY_KP_5,
    [0x4D] = KEY_KP_6,
    [0x4E] = KEY_KP_PLUS,

    [0x4F] = KEY_KP_1,
    [0x50] = KEY_KP_2,
    [0x51] = KEY_KP_3,

    [0x52] = KEY_KP_0,
    [0x53] = KEY_KP_DOT,

    [0x57] = KEY_F11,
    [0x58] = KEY_F12,
    [0x1A] = KEY_LEFT_BRACKET,
    [0x1B] = KEY_RIGHT_BRACKET,

    [0x27] = KEY_SEMICOLON,
    [0x28] = KEY_APOSTROPHE,
    [0x29] = KEY_GRAVE,

    [0x33] = KEY_COMMA,
    [0x34] = KEY_DOT,
    [0x35] = KEY_SLASH,
};

static const char key_layout[KEY_COUNT] = {
    [KEY_NONE] = '\0',

    [KEY_ESC] = '\0',

    [KEY_1] = '1',
    [KEY_2] = '2',
    [KEY_3] = '3',
    [KEY_4] = '4',
    [KEY_5] = '5',
    [KEY_6] = '6',
    [KEY_7] = '7',
    [KEY_8] = '8',
    [KEY_9] = '9',
    [KEY_0] = '0',

    [KEY_MINUS] = '-',
    [KEY_EQUAL] = '=',
    [KEY_BACKSPACE] = '\b',

    [KEY_TAB] = '\t',

    [KEY_Q] = 'q',
    [KEY_W] = 'w',
    [KEY_E] = 'e',
    [KEY_R] = 'r',
    [KEY_T] = 't',
    [KEY_Y] = 'y',
    [KEY_U] = 'u',
    [KEY_I] = 'i',
    [KEY_O] = 'o',
    [KEY_P] = 'p',

    [KEY_LEFT_BRACKET] = '[',
    [KEY_RIGHT_BRACKET] = ']',
    [KEY_ENTER] = '\n',

    [KEY_A] = 'a',
    [KEY_S] = 's',
    [KEY_D] = 'd',
    [KEY_F] = 'f',
    [KEY_G] = 'g',
    [KEY_H] = 'h',
    [KEY_J] = 'j',
    [KEY_K] = 'k',
    [KEY_L] = 'l',

    [KEY_SEMICOLON] = ';',
    [KEY_APOSTROPHE] = '\'',
    [KEY_GRAVE] = '`',

    [KEY_BACKSLASH] = '\\',

    [KEY_Z] = 'z',
    [KEY_X] = 'x',
    [KEY_C] = 'c',
    [KEY_V] = 'v',
    [KEY_B] = 'b',
    [KEY_N] = 'n',
    [KEY_M] = 'm',

    [KEY_COMMA] = ',',
    [KEY_DOT] = '.',
    [KEY_SLASH] = '/',

    [KEY_SPACE] = ' ',
};

static const char key_shift_layout[KEY_COUNT] = {
    [KEY_NONE] = '\0',

    [KEY_ESC] = '\0',

    [KEY_1] = '!',
    [KEY_2] = '@',
    [KEY_3] = '#',
    [KEY_4] = '$',
    [KEY_5] = '%',
    [KEY_6] = '^',
    [KEY_7] = '&',
    [KEY_8] = '*',
    [KEY_9] = '(',
    [KEY_0] = ')',

    [KEY_MINUS] = '_',
    [KEY_EQUAL] = '+',
    [KEY_BACKSPACE] = '\b',

    [KEY_TAB] = '\t',

    [KEY_Q] = 'Q',
    [KEY_W] = 'W',
    [KEY_E] = 'E',
    [KEY_R] = 'R',
    [KEY_T] = 'T',
    [KEY_Y] = 'Y',
    [KEY_U] = 'U',
    [KEY_I] = 'I',
    [KEY_O] = 'O',
    [KEY_P] = 'P',

    [KEY_LEFT_BRACKET] = '{',
    [KEY_RIGHT_BRACKET] = '}',
    [KEY_ENTER] = '\n',

    [KEY_A] = 'A',
    [KEY_S] = 'S',
    [KEY_D] = 'D',
    [KEY_F] = 'F',
    [KEY_G] = 'G',
    [KEY_H] = 'H',
    [KEY_J] = 'J',
    [KEY_K] = 'K',
    [KEY_L] = 'L',

    [KEY_SEMICOLON] = ':',
    [KEY_APOSTROPHE] = '"',
    [KEY_GRAVE] = '~',

    [KEY_BACKSLASH] = '|',

    [KEY_Z] = 'Z',
    [KEY_X] = 'X',
    [KEY_C] = 'C',
    [KEY_V] = 'V',
    [KEY_B] = 'B',
    [KEY_N] = 'N',
    [KEY_M] = 'M',

    [KEY_COMMA] = '<',
    [KEY_DOT] = '>',
    [KEY_SLASH] = '?',

    [KEY_SPACE] = ' ',
};

char kbd_buffer[KBD_BUFFER_SIZE];
volatile int kbd_head = 0; // Where the OS reads
volatile int kbd_tail = 0; // Where the keyboard writes

keyboard_key_t current_keyboard_key;
keyboard_event_t current_keyboard_event;

static uint8_t mod_state;

void keyboard_enqueue(char c) {
  int next_tail =
      (kbd_tail + 1) % KBD_BUFFER_SIZE; // this somehow is a circular variable

  if (next_tail != kbd_head) {
    kbd_buffer[kbd_tail] = c;
    kbd_tail = next_tail;
  }
}

char keyboard_dequeue() {
  if (kbd_head == kbd_tail) {
    return 0; // if keyboard buffer is empty
  }

  char c = kbd_buffer[kbd_head];
  kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
  return c;
}

void update_mod_state() {
  if (current_keyboard_key.key == KEY_LEFT_SHIFT ||
      current_keyboard_key.key == KEY_RIGHT_SHIFT)
    if (current_keyboard_key.state == pressed)
      mod_state |= MOD_SHIFT;
    else
      mod_state &= ~MOD_SHIFT;
  else if (current_keyboard_key.key == KEY_LEFT_CTRL ||
           current_keyboard_key.key == KEY_RIGHT_CTRL)
    if (current_keyboard_key.state == pressed)
      mod_state |= MOD_CTRL;
    else
      mod_state &= ~MOD_CTRL;

  else if (current_keyboard_key.key == KEY_LEFT_ALT ||
           current_keyboard_key.key == KEY_RIGHT_ALT)
    if (current_keyboard_key.state == pressed)
      mod_state |= MOD_ALT;
    else
      mod_state &= ~MOD_ALT;
  else if (current_keyboard_key.key == KEY_CAPS_LOCK)
    if (current_keyboard_key.state == pressed)
      mod_state ^= MOD_CAPS;
}

char keyboard_event_to_char(keyboard_event_t event) {
  if (event.modifiers & MOD_SHIFT)
    return key_shift_layout[event.key.key];

  return key_layout[event.key.key];
}

void keyboard_event(uint8_t scancode) {
  uint8_t code = scancode & 0x7F;

  current_keyboard_key.key = scancode_map[code];

  current_keyboard_key.state = (scancode & 0x80) ? released : pressed;

  update_mod_state();

  current_keyboard_event.key = current_keyboard_key;
  current_keyboard_event.modifiers = mod_state;

  if (current_keyboard_key.state == pressed) {
    char c = keyboard_event_to_char(current_keyboard_event);

    if (c != '\0')
      keyboard_enqueue(c);
  }
}
