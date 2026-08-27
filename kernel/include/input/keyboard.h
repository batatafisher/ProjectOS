#ifndef _INPUT_KEYBOARD_H
#define _INPUT_KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  KEY_NONE = 0,

  /* ===== Alphanumeric ===== */

  KEY_ESC,

  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_0,

  KEY_MINUS,
  KEY_EQUAL,
  KEY_BACKSPACE,

  KEY_TAB,

  KEY_Q,
  KEY_W,
  KEY_E,
  KEY_R,
  KEY_T,
  KEY_Y,
  KEY_U,
  KEY_I,
  KEY_O,
  KEY_P,

  KEY_LEFT_BRACKET,
  KEY_RIGHT_BRACKET,
  KEY_ENTER,

  KEY_LEFT_CTRL,

  KEY_A,
  KEY_S,
  KEY_D,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_J,
  KEY_K,
  KEY_L,

  KEY_SEMICOLON,
  KEY_APOSTROPHE,
  KEY_GRAVE,

  KEY_LEFT_SHIFT,

  KEY_BACKSLASH,

  KEY_Z,
  KEY_X,
  KEY_C,
  KEY_V,
  KEY_B,
  KEY_N,
  KEY_M,

  KEY_COMMA,
  KEY_DOT,
  KEY_SLASH,

  KEY_RIGHT_SHIFT,

  KEY_KP_MULTIPLY,

  KEY_LEFT_ALT,
  KEY_SPACE,
  KEY_CAPS_LOCK,

  /* ===== Function keys ===== */

  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,

  /* ===== Lock / navigation ===== */

  KEY_NUM_LOCK,
  KEY_SCROLL_LOCK,

  KEY_KP_7,
  KEY_KP_8,
  KEY_KP_9,
  KEY_KP_MINUS,

  KEY_KP_4,
  KEY_KP_5,
  KEY_KP_6,
  KEY_KP_PLUS,

  KEY_KP_1,
  KEY_KP_2,
  KEY_KP_3,

  KEY_KP_0,
  KEY_KP_DOT,

  /* ===== Extended keys (0xE0) ===== */

  KEY_RIGHT_CTRL,
  KEY_KP_DIVIDE,

  KEY_RIGHT_ALT,

  KEY_HOME,
  KEY_UP,
  KEY_PAGE_UP,

  KEY_LEFT,
  KEY_RIGHT,

  KEY_END,
  KEY_DOWN,
  KEY_PAGE_DOWN,

  KEY_INSERT,
  KEY_DELETE,

  /* ===== Laptop / miscellaneous ===== */

  KEY_PRINT_SCREEN,
  KEY_PAUSE,

  KEY_MENU,

  KEY_COUNT
} key_t;

typedef enum { pressed, released } key_state_t;

typedef struct {
  key_t key;
  key_state_t state;
} keyboard_key_t;

typedef struct {
  keyboard_key_t key;
  uint8_t modifiers;
} keyboard_event_t;

char keyboard_dequeue();
void keyboard_event(uint8_t scancode);

#endif
