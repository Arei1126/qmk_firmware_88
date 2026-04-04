// Copyright 2026 Arei1126
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

void pc9801_uart_init(void);
bool pc9801_uart_available(void);
bool pc9801_uart_read(uint8_t *data);
