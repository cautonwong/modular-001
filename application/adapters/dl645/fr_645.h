#ifndef ND_APPLICATION_ADAPTERS_DL645_FR_645_H
#define ND_APPLICATION_ADAPTERS_DL645_FR_645_H

#include "fr_645_message.h"

struct instance_t;
struct pipeline_t;

int fr_decode_645_11(struct instance_t *instance, struct pipeline_t *pipeline,
                     nd_dl645_msg_t *mesg);
int fr_encode_645_11(struct instance_t *instance, struct pipeline_t *pipeline,
                     nd_dl645_msg_t *mesg);
int fr_decode_645_08(struct instance_t *instance, struct pipeline_t *pipeline,
                     nd_dl645_msg_t *mesg);
int fr_encode_645_86(struct instance_t *instance, struct pipeline_t *pipeline,
                     nd_dl645_msg_t *mesg);
int fr_decode_645_86(struct instance_t *instance, struct pipeline_t *pipeline,
                     nd_dl645_msg_t *mesg);
int fr_rx_645_end(struct instance_t *instance, struct pipeline_t *pipeline,
                  nd_dl645_msg_t *mesg);
int fr_tx_645_end(struct instance_t *instance, struct pipeline_t *pipeline,
                  nd_dl645_msg_t *mesg);
int fr_tx_645_entry(struct instance_t *instance, struct pipeline_t *pipeline,
                    nd_dl645_msg_t *mesg);

#endif
