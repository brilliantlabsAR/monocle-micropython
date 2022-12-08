/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */

#ifndef MONOCLE_BOARD_H
#define MONOCLE_BOARD_H

/**
 * Built out of all other drivers, providing a general interface.
 * @defgroup board
 */

#include <stdbool.h>
#include <stdint.h>

#define BOARD_ASSERT(e) do if (!(e)) board_assert_func(__FILE__, __LINE__, __func__, #e); while (0)

void board_assert_func(char const *file, int line, char const *func, char const *expr);
void board_init(void);
void board_deinit(void);

#endif
