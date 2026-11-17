#ifndef ND_APPLICATION_ADAPTERS_DL645_FR_645_H
#define ND_APPLICATION_ADAPTERS_DL645_FR_645_H

#include "fr_645_message.h"
int fr_decode_645_11(void *instance, void *pipeline,
                     nd_dl645_msg_t *mesg);
int fr_encode_645_11(void *instance, void *pipeline,
                     nd_dl645_msg_t *mesg);
int fr_decode_645_08(void *instance, void *pipeline,
                     nd_dl645_msg_t *mesg);
int fr_encode_645_86(void *instance, void *pipeline,
                     nd_dl645_msg_t *mesg);
int fr_decode_645_86(void *instance, void *pipeline,
                     nd_dl645_msg_t *mesg);
int fr_rx_645_end(void *instance, void *pipeline,
                  nd_dl645_msg_t *mesg);
int fr_tx_645_end(void *instance, void *pipeline,
                  nd_dl645_msg_t *mesg);
int fr_tx_645_entry(void *instance, void *pipeline,
                    nd_dl645_msg_t *mesg);

#endif
