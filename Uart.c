#include "stm32g0xx.h"
#include "stdio.h"
#include "stddef.h"
#include <stdlib.h>
#include <string.h>
#include "Uart.h"


volatile const char *buffer;
char rxbuffer_storage[RX_BUFF_SIZE];
char *rxbuffer = rxbuffer_storage;
volatile int len;
volatile int txindex;
volatile int rxindex;
volatile int txbusy = 0;
volatile int rxbusy = 0;


void uart_init()
{
    RCC->APBENR1 |= (1 << 17); // Enable clock for USART2
    RCC->IOPENR |= (1 << 0);   // Enable clock for GPIOA

    // Configure PA2 (TX) and PA3 (RX) as AF1
	
    GPIOA->MODER &= ~(0x3 << (2 * 2));
    GPIOA->MODER &= ~(0x3 << (2 * 3));
    GPIOA->MODER |= (0x2 << (2 * 2));
    GPIOA->MODER |= (0x2 << (2 * 3));

    GPIOA->AFR[0] &= ~(0xF << 8);    // Clear AF for PA2
    GPIOA->AFR[0] |=  (0x1 << 8);    // Set AF1 for USART2 TX and RX (PA2 and PA3)

    USART2->BRR = 1667;              // Baud rate 9600 @16MHz clock
    
    USART2->CR1 |= (1 << 0) | (1 << 3) | (1 << 2);  // Enable USART, TX and RX, without TXEIE or RXNEIE initially
}

void tx_string(const char * str)
{
    buffer = str;
    len = strlen(str);
    txindex = 0;
    USART2->CR1 |= (1 << 7);  // Enable TXE interrupt
    txbusy = 1;
}

void USART2_IRQHandler()
{
	    // Transmit interrupt handler - TXE flag
    if (USART2->ISR & (1 << 7))
    {
        if (buffer && txindex < len)
        {
            USART2->TDR = buffer[txindex++];
        }
        else
        {
            USART2->CR1 &= ~(1 << 7);  // Disable TXE interrupt
            buffer = NULL;
            txbusy = 0;
        }
    }

    // Receive interrupt handler - RXNE flag
    if (USART2->ISR & (1 << 5))
    {
        if (rxindex < RX_BUFF_SIZE - 1)
        {
            rxbuffer[rxindex++] = USART2->RDR;

            if (rxindex > 0 && (rxbuffer[rxindex - 1] == '\n' || rxbuffer[rxindex - 1] == '\r'))
            {
                rxbuffer[rxindex] = '\0';  // Null terminate received string
                rxbusy = 0;                // Mark reception done
                USART2->CR1 &= ~(1 << 5);  // Disable RXNE interrupt until restarted
            }
        }
        else
        {
            // Buffer overflow
            USART2->CR1 &= ~(1 << 5);      // Disable RXNE interrupt
            rxbusy = 0;
        }
    }

}

void delay(int n)
{
	SysTick->CTRL |= (1 << 0) | (1 << 2);   // SysTick Enable Timer

	SysTick->LOAD = (SystemCoreClock/1000) -1;

	for (volatile int j = 1; j <= n; j++)
    {
		while(!(SysTick->CTRL & (1 << 16)));
    }

	SysTick->CTRL &= ~(1 << 0);
}

