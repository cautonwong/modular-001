#ifndef ND_APPLICATION_ADAPTERS_DL645_PACK_H
#define ND_APPLICATION_ADAPTERS_DL645_PACK_H
#include "fr_645_message.h"
int fr_pack_645_apdu(void * instance, void * pipeline,
                     nd_dl645_msg_t* mesg);
int fr_pack_645_head(void * instance, void * pipeline,
                     nd_dl645_msg_t* mesg);
int fr_pack_645_data(void * instance, void * pipeline,
                     nd_dl645_msg_t* mesg);
int fr_pack_645_tail(void * instance, void * pipeline,
                     nd_dl645_msg_t* mesg);
int __update_cs(void* pctx, uint8_t* byte);
int __update_scrambler_and_cs(void* pctx, uint8_t* byte);
#endif
