
#ifndef UART_DRV_H_
#define UART_DRV_H_


enum uart_chl_t
{
    UART_RS4851_CHL = 0,
    UART_MODULE_CHL,
    UART_IR_CHL,
    CHL_NUM,
};


struct uart_chl_init_t
{
    struct fifo_t* tx_fifo;
    struct fifo_t* rx_fifo;
    const void* uart_cfg;

    uint32 reset_tmr;
    uint32 rx_time_out;
    uint32 tx_time_out;
    uint32 module_reset_tmr;

    uint8_t mcu_uart_index;
    uint8_t chl_type;
};


extern int uart_chl_initialize(struct uart_chl_init_t *uart_chl);





#endif