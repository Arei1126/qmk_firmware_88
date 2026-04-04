#pragma once

/* RP2040 UART line from the PC-9801 keyboard MCU (19200, 8O1). */
#define PC9801_UART_DRIVER SIOD0
#define PC9801_UART_TX_PIN GP0
#define PC9801_UART_RX_PIN GP1
#define PC9801_UART_BAUD 19200
#define PC9801_UART_ENABLE_TX 0

/*
 * GPIO control lines:
 * - I_RST: active-low reset, keep HIGH for normal operation
 * - I_RDY: active-low ready line, keep LOW in this trial build
 */
#define PC9801_I_RST_PIN GP0
#define PC9801_I_RDY_PIN GP4

/*
 * Protocol defaults:
 * - bit7 marks key release
 * - lower 7 bits are linear key index (0..126) mapped as row/col
 */
#define PC9801_KEYCODE_RELEASE_MASK 0x80
#define PC9801_KEYCODE_INDEX_MASK 0x7F
#define PC9801_ALL_KEYS_UP_CODE 0x7F

/* UART smoke-test mode: any received byte creates a short key press. */
#define PC9801_UART_DEBUG_TEST_MODE 0
#define PC9801_UART_DEBUG_ROW 0
#define PC9801_UART_DEBUG_COL 0
#define PC9801_UART_DEBUG_TAP_MS 30
