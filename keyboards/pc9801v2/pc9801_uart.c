// Copyright 2026 Arei1126
// SPDX-License-Identifier: GPL-2.0-or-later

#include "pc9801_uart.h"

#include QMK_KEYBOARD_H
#include <hal.h>

#ifndef PC9801_UART_DRIVER
#    define PC9801_UART_DRIVER SIOD0
#endif

#ifndef PC9801_UART_TX_PIN
#    define PC9801_UART_TX_PIN GP0
#endif

#ifndef PC9801_UART_RX_PIN
#    define PC9801_UART_RX_PIN GP1
#endif

#ifndef PC9801_UART_BAUD
#    define PC9801_UART_BAUD 19200
#endif

#ifndef PC9801_UART_ENABLE_TX
#    define PC9801_UART_ENABLE_TX 1
#endif

#ifndef PC9801_UART_TX_PAL_MODE
#    define PC9801_UART_TX_PAL_MODE PAL_MODE_ALTERNATE_UART
#endif

#ifndef PC9801_UART_RX_PAL_MODE
#    define PC9801_UART_RX_PAL_MODE PAL_MODE_ALTERNATE_UART
#endif

/* 8O1 on RP2040 UART: WLEN=8, PEN=1, EPS=0 (odd), STP2=0 (1 stop). */
static SIOConfig sio_config = {
    .baud      = PC9801_UART_BAUD,
    .UARTLCR_H = (UART_UARTLCR_H_WLEN_8BITS | UART_UARTLCR_H_PEN | UART_UARTLCR_H_FEN),
    .UARTCR    = 0U,
    .UARTIFLS  = (UART_UARTIFLS_RXIFLSEL_1_8F | UART_UARTIFLS_TXIFLSEL_1_8E),
    .UARTDMACR = 0U,
};

void pc9801_uart_init(void) {
    static bool initialized = false;

    if (initialized) {
        return;
    }
    initialized = true;

#if PC9801_UART_ENABLE_TX
    palSetLineMode(PC9801_UART_TX_PIN, PC9801_UART_TX_PAL_MODE);
#endif
    palSetLineMode(PC9801_UART_RX_PIN, PC9801_UART_RX_PAL_MODE);

    sioStart(&PC9801_UART_DRIVER, &sio_config);
}

bool pc9801_uart_available(void) {
    return !sioIsRXEmptyX(&PC9801_UART_DRIVER);
}

bool pc9801_uart_read(uint8_t *data) {
    if (!data || !pc9801_uart_available()) {
        return false;
    }

    msg_t result = chnGetTimeout(&PC9801_UART_DRIVER, TIME_IMMEDIATE);
    if (result < MSG_OK) {
        return false;
    }

    if (sioHasRXErrorsX(&PC9801_UART_DRIVER)) {
        sioGetAndClearErrors(&PC9801_UART_DRIVER);
    }

    *data = (uint8_t)result;
    return true;
}
