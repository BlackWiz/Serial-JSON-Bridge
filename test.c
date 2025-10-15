#include "stm32g0xx.h"
#include "stdio.h"
#include "stddef.h"
#include <stdlib.h>
#include <string.h>
#include "Uart.h"



int main()
{
	uart_init();
	
	NVIC_EnableIRQ(USART2_IRQn); // Enable USART2 interrupts in NVIC

    __enable_irq();  // Enable global interrupts
	
	uart_receive_buffer(); // Enable reception;
	
	while (1)
    {
#if (UART_CONFIG == UART_NORMAL)
        {

            if (!txstate)
            {
                uart_transmit_buffer("Hello I am Iron Man\r\n");
                delay(5000);
            }

            if (!rxtxstate)
            {
                // Process rxbuffer here as needed
				uart_receive_buffer();
				
            }
        }
#else
        {   // RX_B2B - echo back received string

            if (!rxstate && !txstate)
            {
                uart_transmit_buffer(rxbuffer);    // Transmit received string (echo)
                uart_receive_buffer();
            }
        }
#endif
    }
}
 // Also error can be checked and error_reset function can be called

}