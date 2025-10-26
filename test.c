/** @file test.c
 *
 * @brief UART driver testing application with echo and normal modes.
 *
 * Provides two test modes:
 * - UART_MODE_NORMAL: Periodic transmission of test string
 * - UART_MODE_ECHO: Echo received data back to sender
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#include "stm32g0xx.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart.h"
#include "types.h"  /* For bool_t type */

/* Delay between normal mode transmissions in milliseconds */
#define NORMAL_MODE_TX_DELAY_MS    5000u

/* External UART state variables */
extern volatile uart_state_t g_tx_state;
extern volatile uart_state_t g_rx_state;
extern char * g_p_rx_buffer;


/*!
 * @brief Main test application entry point.
 *
 * Initializes UART and runs continuous test loop based on UART_CONFIG setting.
 * - NORMAL mode: Transmits test string every 5 seconds
 * - ECHO mode: Receives data and echoes it back
 *
 * @return Never returns (embedded system main loop).
 */
int32_t
main (void)
{
    (void)uart_init();
    
    NVIC_EnableIRQ(USART2_IRQn);
    __enable_irq();
    
    (void)uart_receive_buffer();
    
    for (;;)
    {
#if (UART_CONFIG == UART_MODE_NORMAL)
        {
            /* Normal mode: periodic transmission */
            if (UART_STATE_IDLE == g_tx_state)
            {
                (void)uart_transmit_buffer("Hello I am Iron Man\r\n");
                delay_ms(NORMAL_MODE_TX_DELAY_MS);
            }

            if (UART_STATE_IDLE == g_rx_state)
            {
                /* Process received data here as needed */
                (void)uart_receive_buffer();
            }
        }
#else
        {
            /* Echo mode: receive and transmit back */
            if ((UART_STATE_IDLE == g_rx_state) && 
                (UART_STATE_IDLE == g_tx_state))
            {
                (void)uart_transmit_buffer(g_p_rx_buffer);
                (void)uart_receive_buffer();
            }
        }
#endif
    }

    /* Never reached */
    return 0;
}

/*** end of file ***/