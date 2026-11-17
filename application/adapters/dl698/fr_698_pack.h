#ifndef ND_APPLICATION_ADAPTERS_DL698_PACK_H
#define ND_APPLICATION_ADAPTERS_DL698_PACK_H
#include "fr_698_message.h"
int fr_pack_698_head(void * instance, void * pipeline,
                     nd_dl698_msg_t* mesg);
int fr_pack_698_tx_head(void * instance,
                        void * pipeline, nd_dl698_msg_t* mesg);
int fr_pack_698_apdu(void * instance, void * pipeline,
                     nd_dl698_msg_t* mesg);
int fr_pack_698_tail(void * instance, void * pipeline,
                     nd_dl698_msg_t* mesg);
#endif
