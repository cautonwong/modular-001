#include "nidavellir.h"

int init_queue(struct queue_t *queue,size_t queue_length, size_t item_size)
{
    uint8_t *buf = malloc(item_size*queue_length);
    queue->buf = buf;
    queue->r = 0;
    queue->w = 0;
    queue->size = item_size *queue_length;
    queue->item_size = item_size;
    return 0;
}
void queue_delete(struct instance_t *instance,struct queue_t *queuep)
{}
bool queue_send(struct instance_t *instance,struct queue_t *queuep, const void *itemp, uint32_t timeout_ms)
{
    struct work_t *work = (struct work_t *)itemp;
    struct work_t w;
    w.work_type =  work->work_type;
    w.worker_mask = work->worker_mask;
    //struct timeval tv;
    //gettimeofday(&tv,NULL);
    assert(instance != NULL);
    assert(queuep != NULL);
    //printf("%ld-----instance %d send work%d to worker %x\n",tv.tv_sec,instance->recipe->name,w.work_type,w.worker_mask);
    printf("---q --- r:%d w:%d size:%d\n",queuep->r,queuep->w,queuep->size);
    if(queuep->w + queuep->item_size >= queuep->size)
    {
        memcpy(queuep->buf,&w,queuep->item_size);
        struct work_t p ;
        memcpy(&p,queuep->buf,queuep->item_size);
        printf("got work%d for worker %x\n",p.work_type,p.worker_mask);
        queuep->w = queuep->item_size;
    }
    else
    {
        memcpy(queuep->buf + queuep->w,&w,queuep->item_size);
        struct work_t p;
        memcpy(&p,queuep->buf+queuep->w,queuep->item_size);
        printf("got work%d for worker %x\n",p.work_type,p.worker_mask);
        queuep->w += queuep->item_size;
    }
    printf("q --- r:%d w:%d size:%d\n",queuep->r,queuep->w,queuep->size);
}
bool queue_send_fromISR(struct instance_t *instance,struct queue_t *queuep, const void *itemp, bool *wokenp)
{}
bool queue_receive(struct instance_t *instance,struct queue_t *queuep, void *itemp, uint32_t timeout_ms)
{
   // struct work_t *work = (struct work_t *)itemp;
   // struct work_t w;
    uint32_t w = queuep->w;//todo  查看这行代码的汇编代码
    uint32_t r = queuep->r;
    if(w > r)
    {
        printf("-----instance got %d work\n",(w-r)/queuep->item_size);
        return true;
    }
    return false;
    /*
    w.work_type =  work->work_type;
    w.worker_mask = work->worker_mask;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    printf("%ld-----instance %d send work%d to worker %x\n",tv.tv_sec,instance->recipe->name,w.work_type,w.worker_mask);
    printf("---q --- r:%d w:%d size:%d\n",queuep->r,queuep->w,queuep->size);
    if(queuep->w + queuep->item_size >= queuep->size)
    {
        memcpy(queuep->buf,&w,queuep->item_size);
        struct work_t p ;
        memcpy(&p,queuep->buf,queuep->item_size);
        printf("got work%d for worker %x\n",p.work_type,p.worker_mask);
        queuep->w = queuep->item_size;
    }
    else
    {
        memcpy(queuep->buf + queuep->w,&w,queuep->item_size);
        struct work_t p;
        memcpy(&p,queuep->buf+queuep->w,queuep->item_size);
        printf("got work%d for worker %x\n",p.work_type,p.worker_mask);
        queuep->w += queuep->item_size;
    }
    printf("q --- r:%d w:%d size:%d\n",queuep->r,queuep->w,queuep->size);   */ 
}
