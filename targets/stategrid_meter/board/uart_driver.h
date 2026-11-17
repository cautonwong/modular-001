
#ifndef UART_DRV_H_
#define UART_DRV_H_

#include <stddef.h>
#include <stdint.h>

#define ND_UART_RS4851 0
#define ND_UART_MODULE 4
#define ND_UART_IR 5


enum uart_chl_t
{
    UART_RS4851_CHL = 0,
    UART_MODULE_CHL,
    UART_IR_CHL,
    CHL_NUM,
};


struct fifo_t;

struct uart_chl_init_t
{
    struct fifo_t* tx_fifo;
    struct fifo_t* rx_fifo;
    const void* uart_cfg;

    uint32_t reset_tmr;
    uint32_t rx_time_out;
    uint32_t tx_time_out;
    uint32_t module_reset_tmr;

    uint8_t mcu_uart_index;
    uint8_t chl_type;
    void (*CallBackReceiveTimeOut)();
    void (*CallBackSendTimeOut)(void *CallBackSendPara);
    void *CallBackSendPara;
};


extern int uart_chl_initialize(struct uart_chl_init_t *uart_chl);
extern int uart_chl_open(uint8_t chl_type);

void nd_board_uart_init(size_t index, const void *config);
void nd_board_uart_tx_enable(size_t index, uint8_t enable);
void nd_board_uart_rx_enable(size_t index, uint8_t enable);
int nd_board_uart_write(size_t index, uint8_t byte);
int nd_board_uart_tx_done(size_t index);
int nd_board_uart_read(size_t index, uint8_t *byte);
int nd_board_uart_rx_done(size_t index);




#endif
