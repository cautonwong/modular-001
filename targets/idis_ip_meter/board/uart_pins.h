#ifndef ND_UART_PINS_H
#define ND_UART_PINS_H

void nd_uart_rs485_receiver_enable(void);
void nd_uart_rs485_receiver_disable(void);
void nd_uart_rs485_rx_input(void);
void nd_uart_rs485_tx_output(void);
void nd_uart_rs485_tx_set(int high);

#endif
