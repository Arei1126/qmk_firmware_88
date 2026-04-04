// Copyright 2026 Arei1126
// SPDX-License-Identifier: GPL-2.0-or-later

#include "matrix.h"
#include "pc9801_uart.h"
#include "pc98_codes.h"
#include "timer.h"
#include "gpio.h"
#include "wait.h"

#ifndef PC9801_KEYCODE_RELEASE_MASK
#    define PC9801_KEYCODE_RELEASE_MASK 0x80
#endif

#ifndef PC9801_KEYCODE_INDEX_MASK
#    define PC9801_KEYCODE_INDEX_MASK 0x7F
#endif

#ifndef PC9801_ALL_KEYS_UP_CODE
#    define PC9801_ALL_KEYS_UP_CODE 0x7F
#endif

#ifndef PC9801_CMD_FA
#    define PC9801_CMD_FA 0xFA
#endif

#ifndef PC9801_CMD_FB
#    define PC9801_CMD_FB 0xFB
#endif

#ifndef PC9801_CMD_FC
#    define PC9801_CMD_FC 0xFC
#endif

#ifndef PC9801_RELEASE_DELAY_MS
#    define PC9801_RELEASE_DELAY_MS 4U
#endif

#ifndef PC9801_LOCK_TAP_MS
#    define PC9801_LOCK_TAP_MS 8U
#endif

#ifndef PC9801_UART_DEBUG_TEST_MODE
#    define PC9801_UART_DEBUG_TEST_MODE 0
#endif

#ifndef PC9801_UART_DEBUG_ROW
#    define PC9801_UART_DEBUG_ROW 0
#endif

#ifndef PC9801_UART_DEBUG_COL
#    define PC9801_UART_DEBUG_COL 0
#endif

#ifndef PC9801_UART_DEBUG_TAP_MS
#    define PC9801_UART_DEBUG_TAP_MS 30
#endif

#ifndef PC9801_I_RDY_PIN
#    define PC9801_I_RDY_PIN PC9801_UART_TX_PIN
#endif

#ifndef PC9801_I_RST_PIN
#    define PC9801_I_RST_PIN PC9801_UART_TX_PIN
#endif

#ifndef PC9801_I_RDY_ACK_US
#    define PC9801_I_RDY_ACK_US 40U
#endif

static uint32_t pending_release_ms[MATRIX_ROWS][MATRIX_COLS];
static uint32_t pending_lock_tap_release_ms[MATRIX_ROWS][MATRIX_COLS];

static inline bool pc9801_coord_valid(matrix_coord_t c) {
    return c.row < MATRIX_ROWS && c.col < MATRIX_COLS;
}

static inline bool pc9801_is_lock_key(matrix_coord_t c) {
    return (c.row == 3 && c.col == 1) || (c.row == 5 && c.col == 0);
}

static bool pc9801_set_key(matrix_row_t current_matrix[], uint8_t row, uint8_t col, bool on) {
    matrix_row_t before = current_matrix[row];
    if (on) {
        current_matrix[row] |= ((matrix_row_t)1 << col);
    } else {
        current_matrix[row] &= ~((matrix_row_t)1 << col);
    }
    return before != current_matrix[row];
}

static void pc9801_schedule_release(uint8_t row, uint8_t col, uint32_t now_ms) {
    pending_release_ms[row][col] = now_ms;
}

static void pc9801_cancel_release(uint8_t row, uint8_t col) {
    pending_release_ms[row][col] = 0;
}

static void pc9801_schedule_lock_tap_release(uint8_t row, uint8_t col, uint32_t now_ms) {
    pending_lock_tap_release_ms[row][col] = now_ms;
}

static bool pc9801_apply_pending_releases(matrix_row_t current_matrix[]) {
    bool     changed = false;
    uint32_t now_ms  = timer_read32();

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (pending_release_ms[row][col] != 0 && timer_elapsed32(pending_release_ms[row][col]) >= PC9801_RELEASE_DELAY_MS) {
                changed |= pc9801_set_key(current_matrix, row, col, false);
                pending_release_ms[row][col] = 0;
            }

            if (pending_lock_tap_release_ms[row][col] != 0 && timer_elapsed32(pending_lock_tap_release_ms[row][col]) >= PC9801_LOCK_TAP_MS) {
                changed |= pc9801_set_key(current_matrix, row, col, false);
                pending_lock_tap_release_ms[row][col] = 0;
            }
        }
    }

    (void)now_ms;
    return changed;
}

static void pc9801_clear_matrix(matrix_row_t current_matrix[]) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        current_matrix[row] = 0;
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            pending_release_ms[row][col]          = 0;
            pending_lock_tap_release_ms[row][col] = 0;
        }
    }
}

void matrix_init_custom(void) {
    gpio_set_pin_output(PC9801_I_RST_PIN);
    gpio_write_pin_high(PC9801_I_RST_PIN);

    gpio_set_pin_output(PC9801_I_RDY_PIN);
    gpio_write_pin_low(PC9801_I_RDY_PIN);

    pc9801_uart_init();
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool    changed = false;
    uint8_t raw     = 0;

    gpio_write_pin_high(PC9801_I_RST_PIN);
    gpio_write_pin_low(PC9801_I_RDY_PIN);

#if PC9801_UART_DEBUG_TEST_MODE
    static bool     debug_key_is_on = false;
    static uint16_t debug_key_timer = 0;

    while (pc9801_uart_read(&raw)) {
        (void)raw;

        if (PC9801_UART_DEBUG_ROW < MATRIX_ROWS && PC9801_UART_DEBUG_COL < MATRIX_COLS) {
            matrix_row_t row_before = current_matrix[PC9801_UART_DEBUG_ROW];
            current_matrix[PC9801_UART_DEBUG_ROW] |= ((matrix_row_t)1 << PC9801_UART_DEBUG_COL);
            changed |= row_before != current_matrix[PC9801_UART_DEBUG_ROW];
            debug_key_is_on = true;
            debug_key_timer = timer_read();
        }
    }

    if (debug_key_is_on && timer_elapsed(debug_key_timer) >= PC9801_UART_DEBUG_TAP_MS) {
        if (PC9801_UART_DEBUG_ROW < MATRIX_ROWS && PC9801_UART_DEBUG_COL < MATRIX_COLS) {
            matrix_row_t row_before = current_matrix[PC9801_UART_DEBUG_ROW];
            current_matrix[PC9801_UART_DEBUG_ROW] &= ~((matrix_row_t)1 << PC9801_UART_DEBUG_COL);
            changed |= row_before != current_matrix[PC9801_UART_DEBUG_ROW];
        }
        debug_key_is_on = false;
    }

    return changed;
#endif

    while (pc9801_uart_read(&raw)) {
        // ACK pulse: HIGH for 40us immediately after receiving one byte.
        gpio_write_pin_high(PC9801_I_RDY_PIN);
        wait_us(PC9801_I_RDY_ACK_US);
        gpio_write_pin_low(PC9801_I_RDY_PIN);
        wait_us(1);

        if (raw == PC9801_CMD_FA || raw == PC9801_CMD_FB || raw == PC9801_CMD_FC) {
            continue;
        }

        if (raw == PC9801_ALL_KEYS_UP_CODE) {
            pc9801_clear_matrix(current_matrix);
            changed = true;
            continue;
        }

        int8_t  signed_data = (int8_t)raw;
        bool    released    = signed_data < 0;
        uint8_t scan_code   = released ? (uint8_t)(signed_data + 128) : (uint8_t)signed_data;

        matrix_coord_t coord = pc98_to_matrix[scan_code];
        if (!pc9801_coord_valid(coord)) {
            continue;
        }

        uint32_t now_ms = timer_read32();

        if (pc9801_is_lock_key(coord)) {
            // Mechanical lock keys (Caps/Kana): emit a tap on both make and break.
            changed |= pc9801_set_key(current_matrix, coord.row, coord.col, true);
            pc9801_schedule_lock_tap_release(coord.row, coord.col, now_ms);
            continue;
        }

        if (released) {
            // Delay break to smooth keyboard-side auto-repeat interaction.
            pc9801_schedule_release(coord.row, coord.col, now_ms);
        } else {
            changed |= pc9801_set_key(current_matrix, coord.row, coord.col, true);
            pc9801_cancel_release(coord.row, coord.col);
        }
    }

    changed |= pc9801_apply_pending_releases(current_matrix);

    gpio_write_pin_low(PC9801_I_RDY_PIN);

    return changed;
}
