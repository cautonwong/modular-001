#ifndef ND_APPLICATION_DL698_ESAM_FLOW_H
#define ND_APPLICATION_DL698_ESAM_FLOW_H

#include "fr_698_message.h"

struct instance_t;
struct pipeline_t;

int fr_698_esam_verify(struct instance_t* instance, struct pipeline_t* pipeline,
                       nd_dl698_msg_t* mesg);

#endif
