#ifndef ND_APPLICATION_ADAPTERS_DL645_PACK_H
#define ND_APPLICATION_ADAPTERS_DL645_PACK_H
#include "fr_645_message.h"
int fr_pack_645_apdu(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl645_msg_t* mesg);
int fr_pack_645_head(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl645_msg_t* mesg);
int fr_pack_645_data(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl645_msg_t* mesg);
int fr_pack_645_tail(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl645_msg_t* mesg);
#endif
