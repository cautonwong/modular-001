#ifndef __INCLUDE_QUEUE_T__
#define __INCLUDE_QUEUE_T__
struct instance_t;
struct queue_t
{
    uint8_t *buf;
    uint32_t size;
    uint32_t item_size;
    uint32_t r;
    uint32_t w;
};
int init_queue(struct queue_t *queue,size_t queue_length, size_t item_size);

//struct queue_t *queue_create(struct instance_t *instance,size_t queue_length, size_t item_size);
void queue_delete(struct instance_t *instance,struct queue_t *queuep);
bool queue_send(struct instance_t *instance,struct queue_t *queuep, const void *itemp, uint32_t timeout_ms);
bool queue_send_fromISR(struct instance_t *instance,struct queue_t *queuep, const void *itemp, bool *wokenp);
bool queue_receive(struct instance_t *instance,struct queue_t *queuep, void *itemp, uint32_t timeout_ms);
#endif