/** @file delay.c
 *
 * @brief Non-blocking delay implementation using SysTick.
 *
 * Provides both blocking (delay_ms) and non-blocking (delay_start/delay_elapsed)
 * timing functions for embedded applications.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#include <stdint.h>
#include "delay.h"

/* SysTick register addresses */
#define SYST_CSR  ((volatile uint32_t *)0xE000E010)
#define SYST_RVR  ((volatile uint32_t *)0xE000E014)
#define SYST_CVR  ((volatile uint32_t *)0xE000E018)

/* SysTick control bits */
#define SYST_CSR_ENABLE       0u
#define SYST_CSR_TICKINT      1u
#define SYST_CSR_CLKSRC       2u
#define SYST_CSR_COUNTFLAG    16u

/* System configuration */
#define SYSTEM_CORE_CLOCK     16000000UL
#define SYSTICK_MS_DIVISOR    1000u

/* Global tick counter (incremented every 1ms by SysTick ISR) */
static volatile uint32_t g_systick_ms = 0;


/*!
 * @brief Initialize SysTick for 1ms interrupts.
 *
 * Configures SysTick to generate interrupts every 1ms, providing
 * a time base for non-blocking delays.
 */
void delay_init(void)
{
    /* Configure SysTick for 1ms tick */
    *SYST_RVR = (SYSTEM_CORE_CLOCK / SYSTICK_MS_DIVISOR) - 1u;
    
    /* Clear current value */
    *SYST_CVR = 0;
    
    /* Enable SysTick with interrupt, using processor clock */
    *SYST_CSR = (1u << SYST_CSR_ENABLE) | 
                (1u << SYST_CSR_TICKINT) | 
                (1u << SYST_CSR_CLKSRC);
}


/*!
 * @brief Get current system tick count in milliseconds.
 *
 * @return Current tick count (wraps every ~49 days).
 */
uint32_t delay_get_tick(void)
{
    return g_systick_ms;
}


/*!
 * @brief Check if specified time has elapsed since start tick.
 *
 * Handles 32-bit timer wraparound correctly.
 *
 * @param[in] start_tick Tick count from delay_get_tick() or delay_start().
 * @param[in] delay_ms Delay duration in milliseconds.
 *
 * @return 1 if time has elapsed, 0 otherwise.
 */
uint8_t delay_elapsed(uint32_t start_tick, uint32_t delay_ms)
{
    uint32_t current_tick = g_systick_ms;
    uint32_t elapsed = current_tick - start_tick;
    
    return (elapsed >= delay_ms) ? 1u : 0u;
}


/*!
 * @brief Start a non-blocking delay timer.
 *
 * Usage:
 *   uint32_t timer = delay_start();
 *   // ... do other work ...
 *   if (delay_elapsed(timer, 500)) {
 *       // 500ms has passed
 *   }
 *
 * @return Current tick count (use with delay_elapsed()).
 */
uint32_t delay_start(void)
{
    return g_systick_ms;
}


/*!
 * @brief Blocking delay in milliseconds.
 *
 * @param[in] milliseconds Number of milliseconds to delay.
 *
 * @warning This is a blocking function. Use delay_elapsed() for non-blocking.
 */
void delay_ms(uint32_t milliseconds)
{
    uint32_t start = g_systick_ms;
    
    while ((g_systick_ms - start) < milliseconds) {
        /* Busy wait - could use __WFI() here for lower power */
    }
}


/*!
 * @brief SysTick interrupt handler.
 *
 * Increments global tick counter every 1ms. Must be defined in
 * startup.c vector table as SysTick_Handler.
 */
void SysTick_Handler(void)
{
    g_systick_ms++;
}

/*** end of file ***/