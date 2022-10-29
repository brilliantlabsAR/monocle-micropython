/*
 * Copyright (c) 2022 Brilliant Labs Limited
 * Licensed under the MIT License
 */
#ifndef MONOCLE_BOARD_H
#define MONOCLE_BOARD_H
/**
 * Built out of all other drivers, providing a general interface.
 *
 * @defgroup Board
 * @{
 */

void board_power_rails_on(void);
void board_power_rails_off(void);
void board_init(void);
void board_deinit(void);

/* @} */
#endif
