#ifndef ND_KERNEL_MESSAGE_H
#define ND_KERNEL_MESSAGE_H
/* Legacy message mechanism. New components use nd_bus and nd_channel. */

#include <stdint.h>

#include "types.h"

struct instance_t;
struct pipeline_t;

enum recv_mesg_v2_e 
{
    MESG_V2_ERR_LEN             = -3,
    MESG_V2_ERR_MAGIC           = -2,
    MESG_V2_ERR_CHECK           = -1,
    
    MESG_V2_OK                  = 0,
    MESG_V2_HEAD                = 1,
    MESG_V2_HEAD_PAYLOAD        = 2,
    MESG_V2_HEAD_PAYLOAD_EXT    = 3,
 
};


//
#define MSG_MAGIC  0xe8f9ab39

//to-be-deleted
struct message_hdr_t
{
    uint8_t head;
    uint8_t type;
    uint16_t len;
};

#pragma pack(1)
struct message_t
{
    uint32_t magic;
    pid_t from;
    pid_t to;
    uint8_t ctrl;
    uint8_t qos;
    uint32_t id;
    uint16_t len;
    uint8_t oad_n;
    uint8_t x;
#ifdef WIN32
    int32_t err_no;
#endif
    uint8_t *payload;
    uint16_t check;
};
#pragma pack()

int send_pipe(struct fifo_t *fifo, uint8_t ctrl, uint32_t id, uint8_t len,
              uint8_t *payload);
int send_mesg(struct instance_t *instance, struct pipeline_t *pipeline,
              struct message_t *mesg);
int recv_mesg_v2(struct instance_t *instance, struct pipeline_t *pipeline,
                 struct message_t *mesg, uint8_t *payload, uint16_t max_len);

#endif
