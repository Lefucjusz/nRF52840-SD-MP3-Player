#pragma once

#define KEYBOARD_DEBOUNCE_TIME_MS 120

typedef enum 
{
	KEYBOARD_UP,
	KEYBOARD_DOWN,
	KEYBOARD_LEFT,
	KEYBOARD_RIGHT,
	KEYBOARD_ENTER,
	KEYBOARD_BUTTONS_COUNT
} keyboard_button_t;

typedef void (*keyboard_callback_t)(void);

int keyboard_init(void);
void keyboard_attach_callback(keyboard_button_t button, keyboard_callback_t callback);
