#ifndef UART_H
#define UART_H

#define RX_BUFF_SIZE 100
#define UART_NORMAL 0
#define UART_ECHO_ENABLE 1 // Enable or disable echo by default

#define UART_CONFIG UART_NORMAL

// Enum for UART State Machine
typedef enum
{
UART_STATE_IDLE,
UART_STATE_TXBUSY,
UART_STATE_RXBUSY,
UART_STATE_ERROR
}uart_state;

// Enum for UART Error Codes --> These are Hardware reported errors
typedef enum
{
UART_ERROR_NONE,
UART_ERROR_OVERRUN,
UART_ERROR_FRAMING,
UART_ERROR_PARITY,
UART_ERROR_NOISE
}uart_error;

// Declaring Variables for handling and executing UART state machine
extern volatile const char *txbuffer;
extern char rxbuffer_storage[RX_BUFF_SIZE];
extern char *rxbuffer;
extern volatile int txlength;
extern volatile int txindex;
extern volatile int rxindex;
extern volatile int txstate;
extern volatile int rxstate;
extern volatile int error;

// Necessary Functions for UART functionality
int uart_init(void);
int uart_transmit_buffer(const char * str);
int uart_receive_buffer();
void error_reset();
void delay(int n);

#endif  /* UART_H */