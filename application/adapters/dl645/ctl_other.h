#ifndef ND_APPLICATION_ADAPTERS_DL645_CTL_OTHER_H
#define ND_APPLICATION_ADAPTERS_DL645_CTL_OTHER_H

#include <stdint.h>

struct instance_t;
struct context_645_t;

signed char Comm_Other(void *instance, uint8_t *p_rx,
                       uint8_t *p_tx, uint8_t *len);
signed char ft_adj_set_meter(void *instance, uint8_t *p_rx,
                             uint8_t *p_tx, uint8_t *len);
signed char Broadcast_timeadj(void *instance, uint8_t chl,
                              uint8_t *p_rx);
signed char MulPort_set(void *instance, uint8_t chl, uint8_t *p_rx);
signed char meteraddr_write(void *instance, uint8_t *p_rx);
signed char dl645_event_clear(void *instance, uint8_t *p_rx,
                              uint8_t *p_tx, uint8_t *tx_len,
                              struct context_645_t *ctx);
signed char dl645_meter_clear(void *instance, uint8_t *p_rx,
                              uint8_t *p_tx, uint8_t *tx_len,
                              struct context_645_t *ctx);
signed char Comm_Read(void *instance, uint8_t *p_rx,
                      uint8_t *p_tx, uint8_t *len, struct context_645_t *ctx);
signed char Comm_Write(struct instance_t *instance, uint8_t *p_rx,
                       uint8_t *p_tx, uint8_t *len, struct context_645_t *ctx);
int8_t safe_Authen03_645(struct instance_t *instance, uint8_t *p_rx,
                         uint8_t *p_tx, uint8_t *tx_len,
                         struct context_645_t *ctx);
int8_t dlt645_image_transfer(void *instance, uint8_t *p_rx,
                             uint8_t *p_tx, uint8_t *tx_len,
                             struct context_645_t *ctx);
int8_t dlt645_image_read(struct instance_t *instance, uint8_t *p_rx,
                         uint8_t *p_tx, uint8_t *tx_len,
                         struct context_645_t *ctx);

#endif
