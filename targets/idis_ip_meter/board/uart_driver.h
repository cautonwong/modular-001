
#ifndef UART_DRV_H_
#define UART_DRV_H_

#include <stddef.h>
#include <stdint.h>

#define MCU_UART_NUM 7

struct nd_idis_board_state;
typedef struct nd_idis_board_state nd_idis_board_state_t;

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
    void (*CallBackReceiveTimeOut)(void);
    void (*CallBackSendTimeOut)(void *CallBackSendPara);
    void *CallBackSendPara;
};


extern int uart_chl_initialize(nd_idis_board_state_t *state,
                               struct uart_chl_init_t *uart_chl);
extern int uart_chl_open(nd_idis_board_state_t *state, uint8_t chl_type);
uint8_t uart_time_process(void *tickpara);
void UartReceiveHandle(void *para);
void UartTransmissionHandle(void *para);

void nd_board_uart_init(nd_idis_board_state_t *state, size_t index,
                        const void *config);
void nd_board_uart_tx_enable(nd_idis_board_state_t *state, size_t index,
                             uint8_t enable);
void nd_board_uart_rx_enable(nd_idis_board_state_t *state, size_t index,
                             uint8_t enable);
int nd_board_uart_write(nd_idis_board_state_t *state, size_t index, uint8_t byte);
int nd_board_uart_tx_done(nd_idis_board_state_t *state, size_t index);
int nd_board_uart_read(nd_idis_board_state_t *state, size_t index, uint8_t *byte);
int nd_board_uart_rx_done(nd_idis_board_state_t *state, size_t index);




#endif
