/** @file uart.h
 *
 * @brief UART driver interface for STM32G0 interrupt-driven communication.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "types.h"  /* For bool_t type (Keil compatible) */

/* Buffer size for UART reception */
#define RX_BUFFER_SIZE_BYTES   100u

/* UART configuration modes */
#define UART_MODE_NORMAL       0u
#define UART_MODE_ECHO         1u

/* Select active UART mode */
#define UART_CONFIG            UART_MODE_NORMAL

/* UART state machine states */
typedef enum
{
    UART_STATE_IDLE   = 0,
    UART_STATE_TX_BUSY,
    UART_STATE_RX_BUSY,
    UART_STATE_ERROR
} uart_state_t;

/* UART hardware error codes */
typedef enum
{
    UART_ERROR_NONE    = 0,
    UART_ERROR_OVERRUN,
    UART_ERROR_FRAMING,
    UART_ERROR_PARITY,
    UART_ERROR_NOISE
} uart_error_t;

/* Public API functions */
int32_t uart_init(void);
int32_t uart_transmit_buffer(char const * const p_str);
int32_t uart_receive_buffer(void);
void uart_error_reset(void);
void delay_ms(uint32_t milliseconds);

#endif /* UART_H */

/*** end of file ***/
