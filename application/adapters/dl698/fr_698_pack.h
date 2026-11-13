#ifndef ND_APPLICATION_ADAPTERS_DL698_PACK_H
#define ND_APPLICATION_ADAPTERS_DL698_PACK_H
#include "fr_698_message.h"
int fr_pack_698_head(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl698_msg_t* mesg);
int fr_pack_698_tx_head(struct instance_t* instance,
                        struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
int fr_pack_698_apdu(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl698_msg_t* mesg);
int fr_pack_698_tail(struct instance_t* instance, struct pipeline_t* pipeline,
                     nd_dl698_msg_t* mesg);
#endif
