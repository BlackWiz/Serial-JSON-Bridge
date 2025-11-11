/** @file delay.h
 *
 * @brief Non-blocking delay interface for embedded applications.
 *
 * Provides timing functions that don't block program execution,
 * allowing concurrent operations while waiting.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

/*!
 * @brief Initialize delay subsystem.
 *
 * Call once at startup before using any delay functions.
 * Configures SysTick for 1ms interrupts.
 */
void delay_init(void);

/*!
 * @brief Get current system tick count.
 *
 * @return Milliseconds since delay_init() was called (wraps at 2^32).
 */
uint32_t delay_get_tick(void);

/*!
 * @brief Check if delay has elapsed (non-blocking).
 *
 * @param[in] start_tick Starting tick from delay_get_tick() or delay_start().
 * @param[in] delay_ms Delay duration in milliseconds.
 *
 * @return 1 if delay has elapsed, 0 otherwise.
 */
uint8_t delay_elapsed(uint32_t start_tick, uint32_t delay_ms);

/*!
 * @brief Start a non-blocking delay timer.
 *
 * @return Current tick count for use with delay_elapsed().
 */
uint32_t delay_start(void);

/*!
 * @brief Blocking delay (legacy compatibility).
 *
 * @param[in] milliseconds Number of milliseconds to block.
 *
 * @warning This blocks execution. Prefer delay_elapsed() for new code.
 */
void delay_ms(uint32_t milliseconds);

#endif /* DELAY_H */

/*** end of file ***/