#ifndef ND_KERNEL_IPC_GLOBALS_H
#define ND_KERNEL_IPC_GLOBALS_H

#include <stdint.h>

struct fifo_t;
struct pipeline_t;
struct rtc_context_t;
struct context_dlms_t;

//to-be-deleted
extern struct fifo_t pipe_msta_rx_from_meter;//rx fifo
extern struct fifo_t pipe_msta_tx_to_meter;//tx fifo
extern struct pipeline_t pl_msta_rx_from_meter; 
extern struct fifo_t pipe_msta_tx_to_meter_req;
extern struct pipeline_t pl_msta_tx_to_meter; 
extern struct fifo_t pipe_msta_rx_from_bt;//rx fifo
extern struct fifo_t pipe_msta_tx_to_bt;//tx fifo
extern struct pipeline_t pl_msta_rx_from_bt; 
extern struct fifo_t pipe_msta_tx_to_bt_req;
extern struct pipeline_t pl_msta_tx_to_bt; 
extern struct fifo_t pipe_msta_rx_from_sta;//rx fifo
extern struct fifo_t pipe_msta_tx_to_sta;//tx fifo
extern struct pipeline_t pl_msta_rx_from_sta; 
extern struct fifo_t pipe_msta_tx_to_sta_req;
extern struct pipeline_t pl_msta_tx_to_sta; 
extern struct fifo_t pipe_msta_tx_to_bt_req;
extern struct pipeline_t pl_msta_tx_to_bt; 
extern struct fifo_t pipe_meter_rx_from_msta;//rx fifo
extern struct fifo_t pipe_meter_tx_to_msta;//tx fifo
extern struct context_dlms_t ctx_dlms;//rx ctx
extern struct pipeline_t pl_meter_rx_from_msta;
extern struct fifo_t pipe_meter_tx_to_msta_req;
extern struct pipeline_t pl_meter_tx_to_msta; 
extern struct fifo_t pipe_sta_rx_from_msta;//rx fifo
extern struct fifo_t pipe_sta_tx_to_msta;//tx fifo
extern struct pipeline_t pl_sta_rx_from_msta; 
extern struct fifo_t pipe_sta_tx_to_msta_req;
extern struct pipeline_t pl_sta_tx_to_msta; 
extern struct fifo_t pipe_bt_rx_from_msta;//rx fifo
extern struct fifo_t pipe_bt_tx_to_msta;//tx fifo
extern struct pipeline_t pl_bt_rx_from_msta; 
extern struct fifo_t pipe_bt_tx_to_msta_req;
extern struct pipeline_t pl_bt_tx_to_msta; 
extern struct fifo_t pipe_log;
extern struct pipeline_t pl_log;
extern struct rtc_context_t msta_context;
extern struct fifo_t pipe_msta;
extern struct pipeline_t pl_msta;
extern struct pipeline_t pl_kernel;
typedef int (*write__)(uint32_t id, uint8_t* buf, int len);
extern struct fifo_t stdin_698;
extern struct fifo_t stdout_698;
extern struct fifo_t pipe_tx_698_req;
extern struct pipeline_t pl_rx_698;
extern struct pipeline_t pl_tx_698;
extern struct pipeline_t pl_tx_to_serial;
extern struct pipeline_t pl_rx_from_serial;
extern struct pipeline_t pl_rx_dlms;
extern struct pipeline_t pl_tx_dlms;

#endif
