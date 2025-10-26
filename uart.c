/** @file uart.c
 *
 * @brief UART driver implementation for STM32G0 with interrupt handling.
 *
 * Provides interrupt-driven UART TX/RX with state machine management
 * and hardware error detection/recovery.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#include "stm32g0xx.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Register bit position constants */
#define RCC_APBENR1_USART2_BIT     17u
#define RCC_IOPENR_GPIOA_BIT       0u
#define GPIO_MODER_AF_MODE         0x2u
#define GPIO_AFR_AF1               0x1u
#define USART_CR1_UE_BIT           0u
#define USART_CR1_TE_BIT           3u
#define USART_CR1_RE_BIT           2u
#define USART_CR1_TXEIE_BIT        7u
#define USART_CR1_RXNEIE_BIT       5u
#define USART_ISR_TXE_BIT          7u
#define USART_ISR_RXNE_BIT         5u
#define USART_ISR_ORE_BIT          3u
#define USART_ISR_FE_BIT           2u
#define USART_ISR_NF_BIT           1u
#define USART_ISR_PE_BIT           0u

/* Pin configuration constants */
#define PA2_PIN_NUM                2u
#define PA3_PIN_NUM                3u
#define BITS_PER_PIN               2u
#define AFR_BITS_PER_PIN           4u
#define PA2_AFR_SHIFT              8u

/* Baud rate calculation for 16MHz clock, 9600 baud */
#define BAUD_RATE_9600_AT_16MHZ    1667u

/* SysTick configuration for 1ms tick */
#define SYSTICK_CTRL_ENABLE_BIT    0u
#define SYSTICK_CTRL_CLKSRC_BIT    2u
#define SYSTICK_CTRL_COUNTFLAG_BIT 16u
#define SYSTICK_MS_DIVISOR         1000u

/* Global state variables for UART state machine */
volatile char const * g_p_tx_buffer;
char g_rx_buffer_storage[RX_BUFFER_SIZE_BYTES];
char * g_p_rx_buffer = g_rx_buffer_storage;
volatile uint32_t g_tx_length;
volatile uint32_t g_tx_index = 0u;
volatile uint32_t g_rx_index = 0u;
volatile uart_state_t g_tx_state = UART_STATE_IDLE;
volatile uart_state_t g_rx_state = UART_STATE_IDLE;
volatile uart_error_t g_error = UART_ERROR_NONE;


/*!
 * @brief Initialize UART2 peripheral with 9600 baud, 8N1 configuration.
 *
 * Configures GPIO pins PA2 (TX) and PA3 (RX) for UART alternate function,
 * enables peripheral clocks, and sets up UART2 for interrupt-driven operation.
 *
 * @return 0 on success, -1 if peripheral pointers are NULL.
 */
int32_t
uart_init (void)
{
    /* Verify peripheral base addresses are valid */
    if ((NULL == GPIOA) || (NULL == RCC) || (NULL == USART2))
    {
        return -1;
    }
    
    /* Enable peripheral clocks */
    RCC->APBENR1 |= (1u << RCC_APBENR1_USART2_BIT);
    RCC->IOPENR |= (1u << RCC_IOPENR_GPIOA_BIT);

    /* Configure PA2 (TX) and PA3 (RX) as alternate function mode */
    GPIOA->MODER &= ~(0x3u << (BITS_PER_PIN * PA2_PIN_NUM));
    GPIOA->MODER &= ~(0x3u << (BITS_PER_PIN * PA3_PIN_NUM));
    GPIOA->MODER |= (GPIO_MODER_AF_MODE << (BITS_PER_PIN * PA2_PIN_NUM));
    GPIOA->MODER |= (GPIO_MODER_AF_MODE << (BITS_PER_PIN * PA3_PIN_NUM));

    /* Set alternate function AF1 for USART2 on PA2 */
    GPIOA->AFR[0] &= ~(0xFu << PA2_AFR_SHIFT);
    GPIOA->AFR[0] |= (GPIO_AFR_AF1 << PA2_AFR_SHIFT);

    /* Configure baud rate for 9600 @ 16MHz system clock */
    USART2->BRR = BAUD_RATE_9600_AT_16MHZ;
    
    /* Enable USART, transmitter, and receiver */
    USART2->CR1 |= ((1u << USART_CR1_UE_BIT) | 
                    (1u << USART_CR1_TE_BIT) | 
                    (1u << USART_CR1_RE_BIT));
        
    return 0;
}


/*!
 * @brief Transmit a null-terminated string via UART2.
 *
 * Initiates interrupt-driven transmission. The ISR handles byte-by-byte
 * transmission until the entire string is sent.
 *
 * @param[in] p_str Pointer to null-terminated string to transmit.
 *
 * @return 0 on success, -1 if p_str is NULL, -2 if transmitter busy.
 */
int32_t
uart_transmit_buffer (char const * const p_str)
{
    if (NULL == p_str)
    {
        return -1;
    }

    /* Critical section: check and update TX state atomically */
    __disable_irq();
        
    if (UART_STATE_IDLE != g_tx_state)
    {
        __enable_irq();
        return -2;
    }

    g_tx_state = UART_STATE_TX_BUSY;
    __enable_irq();

    /* Initialize transmission parameters */
    g_p_tx_buffer = p_str;
    g_tx_length = (uint32_t)strlen(p_str);
    g_tx_index = 0u;
    
    /* Enable TXE interrupt to start transmission */
    USART2->CR1 |= (1u << USART_CR1_TXEIE_BIT);
    
    return 0;
}


/*!
 * @brief Enable interrupt-driven UART reception.
 *
 * Prepares the receiver state machine and enables RXNE interrupt.
 * Reception continues until newline or buffer overflow.
 *
 * @return 0 on success, -1 if receiver already busy.
 */
int32_t
uart_receive_buffer (void)
{
    /* Critical section: check and update RX state atomically */
    __disable_irq();
        
    if (UART_STATE_IDLE != g_rx_state)
    {
        __enable_irq();
        return -1;
    }
    
    g_rx_state = UART_STATE_RX_BUSY;
    __enable_irq();

    /* Enable RXNE interrupt to start reception */
    USART2->CR1 |= (1u << USART_CR1_RXNEIE_BIT);
    
    return 0;
}


/*!
 * @brief USART2 interrupt service routine.
 *
 * Handles both TX and RX interrupts with error detection.
 * TX: Sends next byte or disables interrupt when complete.
 * RX: Receives bytes until newline or error, with overflow protection.
 */
void
USART2_IRQHandler (void)
{
    bool b_has_error = false;

    /* Handle transmit interrupt - TXE flag set */
    if (((USART2->ISR & (1u << USART_ISR_TXE_BIT)) != 0u) && 
        (UART_STATE_TX_BUSY == g_tx_state))
    {
        if ((NULL != g_p_tx_buffer) && (g_tx_index < g_tx_length))
        {
            USART2->TDR = (uint32_t)g_p_tx_buffer[g_tx_index];
            g_tx_index++;
        }
        else
        {
            /* Transmission complete */
            USART2->CR1 &= ~(1u << USART_CR1_TXEIE_BIT);
            g_p_tx_buffer = NULL;
            g_tx_state = UART_STATE_IDLE;
        }
    }

    /* Handle receive interrupt - RXNE flag set */
    if (((USART2->ISR & (1u << USART_ISR_RXNE_BIT)) != 0u) && 
        (UART_STATE_RX_BUSY == g_rx_state))
    {
        /* Check for hardware errors */
        b_has_error = (((USART2->ISR & (1u << USART_ISR_ORE_BIT)) != 0u) ||
                       ((USART2->ISR & (1u << USART_ISR_FE_BIT)) != 0u) ||
                       ((USART2->ISR & (1u << USART_ISR_NF_BIT)) != 0u) ||
                       ((USART2->ISR & (1u << USART_ISR_PE_BIT)) != 0u));

        if (!b_has_error)
        {
            g_error = UART_ERROR_NONE;
            
            /* Buffer has space for new data plus null terminator */
            if (g_rx_index < (RX_BUFFER_SIZE_BYTES - 1u))
            {
                g_p_rx_buffer[g_rx_index] = (char)USART2->RDR;
                g_rx_index++;

                /* Check for line ending characters */
                if ((g_rx_index > 0u) && 
                    (('\n' == g_p_rx_buffer[g_rx_index - 1u]) || 
                     ('\r' == g_p_rx_buffer[g_rx_index - 1u])))
                {
                    g_p_rx_buffer[g_rx_index] = '\0';
                    g_rx_state = UART_STATE_IDLE;
                    USART2->CR1 &= ~(1u << USART_CR1_RXNEIE_BIT);
                }
            }
            else
            {
                /* Buffer overflow - disable interrupt */
                USART2->CR1 &= ~(1u << USART_CR1_RXNEIE_BIT);
                g_rx_state = UART_STATE_IDLE;
            }
        }
        else
        {
            /* Hardware error detected */
            USART2->CR1 &= ~(1u << USART_CR1_RXNEIE_BIT);
            g_rx_state = UART_STATE_ERROR;
            
            /* Identify and clear specific error */
            if ((USART2->ISR & (1u << USART_ISR_ORE_BIT)) != 0u)
            {
                USART2->ICR |= (1u << USART_ISR_ORE_BIT);
                g_error = UART_ERROR_OVERRUN;
            }
            else if ((USART2->ISR & (1u << USART_ISR_FE_BIT)) != 0u)
            {
                USART2->ICR |= (1u << USART_ISR_FE_BIT);
                g_error = UART_ERROR_FRAMING;
            }
            else if ((USART2->ISR & (1u << USART_ISR_PE_BIT)) != 0u)
            {
                USART2->ICR |= (1u << USART_ISR_PE_BIT);
                g_error = UART_ERROR_PARITY;
            }
            else if ((USART2->ISR & (1u << USART_ISR_NF_BIT)) != 0u)
            {
                USART2->ICR |= (1u << USART_ISR_NF_BIT);
                g_error = UART_ERROR_NOISE;
            }
            else
            {
                /* Should never reach here */
            }
        }
    }
}


/*!
 * @brief Reset UART receiver after error condition.
 *
 * Clears error state and re-enables reception. Call this after
 * handling the error reported in g_error.
 */
void
uart_error_reset (void)
{
    if (UART_STATE_ERROR == g_rx_state)
    {
        g_rx_state = UART_STATE_RX_BUSY;
        g_error = UART_ERROR_NONE;
        g_rx_index = 0u;
        USART2->CR1 |= (1u << USART_CR1_RXNEIE_BIT);
    }
}


/*!
 * @brief Blocking delay using SysTick timer.
 *
 * @param[in] milliseconds Number of milliseconds to delay.
 *
 * @par
 * NOTE: This is a blocking delay and should not be called from ISRs.
 */
void
delay_ms (uint32_t milliseconds)
{
    /* Enable SysTick with processor clock */
    SysTick->CTRL |= ((1u << SYSTICK_CTRL_ENABLE_BIT) | 
                      (1u << SYSTICK_CTRL_CLKSRC_BIT));

    SysTick->LOAD = (SystemCoreClock / SYSTICK_MS_DIVISOR) - 1u;

    for (uint32_t i = 0u; i < milliseconds; i++)
    {
        /* Wait for COUNTFLAG to be set */
        while (0u == (SysTick->CTRL & (1u << SYSTICK_CTRL_COUNTFLAG_BIT)))
        {
            /* Busy wait */
        }
    }

    /* Disable SysTick */
    SysTick->CTRL &= ~(1u << SYSTICK_CTRL_ENABLE_BIT);
}

#ifdef __cplusplus
}
#endif

/*** end of file ***/
