#ifndef ND_APPLICATION_DL698_ESAM_FLOW_H
#define ND_APPLICATION_DL698_ESAM_FLOW_H

#include "fr_698_message.h"
int fr_698_esam_verify(void * instance, void * pipeline,
                       nd_dl698_msg_t* mesg);

#endif
