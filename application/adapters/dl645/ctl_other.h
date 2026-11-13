#ifndef ND_APPLICATION_ADAPTERS_DL645_CTL_OTHER_H
#define ND_APPLICATION_ADAPTERS_DL645_CTL_OTHER_H

#include <stdint.h>

struct instance_t;

signed char Comm_Other(struct instance_t *instance, uint8_t *p_rx,
                       uint8_t *p_tx, uint8_t *len);

#endif
