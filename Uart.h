#ifndef UART_H
#define UART_H

#define RX_BUFF_SIZE 100
#define UART_NORMAL 0
#define UART_ECHO_ENABLE 1 // Enable or disable echo by default

#define UART_CONFIG UART_NORMAL

void uart_init(void);
void tx_string(const char * str);
void delay(int n);

#endif  /* UART_H */