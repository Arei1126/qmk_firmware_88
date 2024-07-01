// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "config.h"
#define LOCKING_SUPPORT_ENABLE

enum layer_number {
	_DEFAULT = 0,
	_FUNCTION
};

#define LT_PAUSE LT(_FUNCTION, KC_PAUSE)

enum custom_keycodes{
	KC_CAPS_MCLOCK = SAFE_RANGE,
	KC_KANA_MCLOCK,
};


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌───┬───┬───┬───┐
     * │ 7 │ 8 │ 9 │ / │
     * ├───┼───┼───┼───┤
     * │ 4 │ 5 │ 6 │ * │
     * ├───┼───┼───┼───┤
     * │ 1 │ 2 │ 3 │ - │
     * ├───┼───┼───┼───┤
     * │ 0 │ . │Ent│ + │
     * └───┴───┴───┴───┘
     */
    [_DEFAULT] = LAYOUT(
	LT_PAUSE, KC_PSCR, 	KC_F1, 	KC_F2,	 KC_F3,	 KC_F4,	 KC_F5,		 KC_PGUP, KC_PGDN,
	KC_ESC,	 KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINS, KC_EQL, KC_INT3, KC_BSPC,			KC_NUM, KC_SCRL, KC_PMNS, KC_PSLS, 
	KC_TAB, 	KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRC, KC_RBRC, KC_ENT,			KC_P7,	KC_P8,	 KC_P9,	  KC_PAST,
	KC_LEFT_CTRL, 	KC_CAPS_MCLOCK, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT, KC_NUHS,		KC_P4,	KC_P5,	 KC_P6,   KC_PPLS,
	KC_LSFT,	 KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_INT1, KC_UP,	 		KC_P1,  KC_P2,   KC_P3,   KC_PEQL,
	KC_KANA_MCLOCK, KC_LALT, KC_SPC, 							KC_LEFT, KC_RIGHT, KC_DOWN,	 	KC_P0, 	KC_COMM, KC_PDOT
    ),
[_FUNCTION] = LAYOUT(
	LT_PAUSE, KC_PSCR, 	KC_F6, 	KC_F7,	 KC_F8,	 KC_F9,	 KC_F10,		 KC_F11, KC_F12,
	KC_ESC,	 KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINS, KC_EQL, KC_INT3, KC_BSPC,			KC_NUM, KC_SCRL, KC_PMNS, KC_PSLS, 
	KC_TAB, 	KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRC, KC_RBRC, KC_ENT,			KC_P7,	KC_P8,	 KC_P9,	  KC_PAST,
	KC_RIGHT_CTRL, 	KC_CAPS_MCLOCK, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT, KC_NUHS,		KC_P4,	KC_P5,	 KC_P6,   KC_PPLS,
	KC_RSFT,	 KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_INT1, KC_UP,	 		KC_P1,  KC_P2,   KC_P3,   KC_PEQL,
	KC_KANA_MCLOCK, KC_RALT, KC_SPC, 							KC_LEFT, KC_RIGHT, KC_DOWN,	 	KC_P0, 	KC_COMM, KC_PDOT
    )
};

// result of process_record_user
#define PROCESS_OVERRIDE_BEHAVIOR   (false)
#define PROCESS_USUAL_BEHAVIOR      (true)

bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
	switch (keycode) {
		case KC_CAPS_MCLOCK: {
				      tap_code(KC_CAPS);
				      break;
			      }

		case KC_KANA_MCLOCK: {
				     tap_code(KC_GRV);
				     break;
			     }

		default: {
				 break;
			 }
	}

	return PROCESS_USUAL_BEHAVIOR;
}
