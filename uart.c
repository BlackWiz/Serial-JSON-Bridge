#include "stm32g0xx.h"
#include "stdio.h"
#include "stddef.h"
#include <stdlib.h>
#include <string.h>
#include "uart.h"

#ifdef __cplusplus
extern "C" {
#endif

// Required Variables for handling and executing UART state machine
volatile const char *txbuffer;
char rxbuffer_storage[RX_BUFF_SIZE];
char *rxbuffer = rxbuffer_storage;
volatile int txlength;
volatile int txindex = 0;
volatile int rxindex = 0;
volatile int txstate = UART_STATE_IDLE;
volatile int rxstate = UART_STATE_IDLE;
volatile int error = UART_ERROR_NONE;


/* Initialisation function for UART. This mainly handles setup of all the registers and sets the baud rate
required for UART functioning */
int uart_init()
{
    if((GPIOA == NULL) || (RCC == NULL) || (USART2 == NULL))
	return -1;  // Error Code : Regsiters Not initialised properly
	
	RCC->APBENR1 |= (1 << 17); // Enable clock for USART2
    RCC->IOPENR |= (1 << 0);   // Enable clock for GPIOA

    // Configure PA2 (TX) and PA3 (RX) as AF1
	
    GPIOA->MODER &= ~(0x3 << (2 * 2));
    GPIOA->MODER &= ~(0x3 << (2 * 3));
    GPIOA->MODER |= (0x2 << (2 * 2));
    GPIOA->MODER |= (0x2 << (2 * 3));

    GPIOA->AFR[0] &= ~(0xF << 8);    // Clear AF for PA2
    GPIOA->AFR[0] |=  (0x1 << 8);    // Set AF1 for USART2 TX and RX (PA2 and PA3)

    USART2->BRR = 1667;              // Baud rate 9600 @16MHz clock. Baud rate formula can be referred from respective data sheets
    
    USART2->CR1 |= (1 << 0) | (1 << 3) | (1 << 2);  // Enable USART, TX and RX, without TXEIE or RXNEIE initially
		
	return 0;	// Init is success
}

/* Transmit function for buffer */
int uart_transmit_buffer(const char * str)
{
	if(str == NULL)
	return -1; // Error Code : Indicates the given input string is empty

	__disable_irq(); // Disable all interrupts so as to perform an atomic operation such that the transmissions happen properly
		
	if(txstate != UART_STATE_IDLE)
	{
		__enable_irq(); // Re-enables the Interrrupts so as to continue with the current transmission
		return -2; // Error Code : Indicates the Transmitter is already busy and cannot transmit the new data
	}

	txstate = UART_STATE_TXBUSY; // Set UART state to BUSY to begin the transmission
	__enable_irq(); // Enabling of interrupts to continue the transmission / start the transmission

	
    txbuffer = str;
    txlength = strlen(str);
    txindex = 0;
    
	USART2->CR1 |= (1 << 7);  // Enable TXE interrupt
	
	return 0; // Indicates a succesful transmission
}

/* Receive function for UART buffers */
int uart_receive_buffer()
{
	__disable_irq(); // Disable all interrupts so as to perform an atomic operation such that the receptions happen properly
		
	if(rxstate != UART_STATE_IDLE)
	{
		__enable_irq(); // Re-enables the Interrrupts so as to continue with the current reception
		return -1; // Error Code : Indicates the receiver is already busy and cannot receive the new data
	}
	
	rxstate = UART_STATE_RXBUSY; // Set UART state to BUSY to begin the reception	
	__enable_irq(); // Re-enables the Interrrupts so as to continue with the current reception

	
	USART2->CR1 |= (1 << 5);  // Enable RXNE interrupt
	
	return 0; // Indicates a succesful reception
	
}

/* ISR Handler for UART2  */
void USART2_IRQHandler()
{
	    // Transmit interrupt handler - TXE flag
    if ((USART2->ISR & (1 << 7)) && (txstate == UART_STATE_TXBUSY))
    {
        if (txbuffer && txindex < txlength)
        {
            USART2->TDR = txbuffer[txindex++];
        }
        else
        {
            USART2->CR1 &= ~(1 << 7);  // Disable TXE interrupt
            txbuffer = NULL;
            txstate = UART_STATE_IDLE;
        }
    }

    // Receive interrupt handler - RXNE flag
    if ((USART2->ISR & (1 << 5)) 
		&& (rxstate == UART_STATE_RXBUSY)) 

    {
        if((!(USART2->ISR & (1 << 3))) 
		   || (!(USART2->ISR & (1 << 2))) 
			 || (!(USART2->ISR & (1 << 1)))
				|| (!(USART2->ISR & (1 << 0))))
				{
					error = UART_ERROR_NONE;
					if (rxindex < RX_BUFF_SIZE - 1)
					{
						rxbuffer[rxindex++] = USART2->RDR;

						if (rxindex > 0 && (rxbuffer[rxindex - 1] == '\n' || rxbuffer[rxindex - 1] == '\r'))
						{
							rxbuffer[rxindex] = '\0';  // Null terminate received string
							rxstate = UART_STATE_IDLE;                // Mark reception done
							USART2->CR1 &= ~(1 << 5);  // Disable RXNE interrupt until restarted
						}
					}
        
					else
					{
						// Buffer overflow
						USART2->CR1 &= ~(1 << 5);      // Disable RXNE interrupt
						rxstate = UART_STATE_IDLE;			
					}
				}
		else
		{
			USART2->CR1 &= ~(1 << 5);      // Disable RXNE interrupt
			rxstate = UART_STATE_ERROR;    // Set UART rx state to Error
			if((USART2->ISR) & (1 << 3))
			{
				USART2->ICR |= (1 << 3);
				error = UART_ERROR_OVERRUN;
			}
			
			else if((USART2->ISR) & (1 << 2))
			{
				USART2->ICR |= (1 << 2);
				error = UART_ERROR_FRAMING;
			}
			
			else if((USART2->ISR) & (1 << 0))
			{
				USART2->ICR |= (1 << 0);
				error = UART_ERROR_PARITY;
			}
			
			else if((USART2->ISR) & (1 << 1))
			{
				USART2->ICR |= (1 << 1);
				error = UART_ERROR_NOISE;
			}			
			
		}
	}

}

/* Error reset function to reset the driver again to a known state  */
void error_reset()
{
	if(rxstate == UART_STATE_ERROR)
	{
		rxstate = UART_STATE_RXBUSY;
		error = UART_ERROR_NONE;
		rxindex = 0;
		USART2->CR1 |= (1 << 5);  // Enable RXNE interrupt
		
	}
	
}

/* Custom Delay function in ms */
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

#ifdef __cplusplus
}
#endif