#include "domain/events/record_message.h"
#include "nidavellir.h"

void nd_record_message_send(struct instance_t *instance,
                            struct pipeline_t *pipeline, uint16_t record_id,
                            uint8_t *payload)
{
    struct message_t mesg;

    (void)pipeline;
    mesg.from = kernel;
    mesg.to = pid_resource_manager;
    mesg.ctrl = 0;
    mesg.qos = 0;
    mesg.id = record_id;
    mesg.oad_n = 1;
    mesg.payload = payload;
    mesg.len = 0;
    send_mesg(instance, NULL, &mesg);
}
