#ifndef _FIFO_BUFFER_H_
#define _FIFO_BUFFER_H_
//-1 迭代出错
//0 正常继续
//1 正常结束
typedef int (*fifo_callback_t)(void* ctx, uint8_t *b);

struct fifo_t
{
    uint8_t *buf_ptr;
    volatile uint16_t rd;
    volatile uint16_t wr;
    uint16_t buf_size;
    //uint8_t  trigger_level_bytes;//最小长度
};

struct cache_t
{
    uint32_t capacity;
    uint32_t used;
    //uint8_t buf[0];
    uint8_t* buf;
};

// *********************

uint16_t fifoBuf_getSize(struct fifo_t *buf);

uint16_t fifoBuf_getUsed(struct fifo_t *buf);
uint16_t fifoBuf_getFree(struct fifo_t *buf);
uint16_t fifoBuf_end(struct fifo_t* buf, uint16_t len);

void ringbuffer_clear(struct fifo_t *buf);
void fifoBuf_removeData(struct fifo_t *buf, uint16_t len);

int16_t fifoBuf_getBytePeek(struct fifo_t *buf);
uint8_t fifoBuf_getByte(struct fifo_t *buf);
int16_t fifoBuf_peekByteB(struct fifo_t *buf, uint16_t offset);
int16_t fifoBuf_patchByteB(struct fifo_t *buf,uint8_t b, uint16_t offset);

uint16_t fifoBuf_getDataPeek(struct fifo_t *buf, void *data, uint16_t len);
uint16_t fifoBuf_getData(struct fifo_t *buf, void *data, uint16_t len);
int16_t fifoBuf_getByteCount(struct fifo_t* buf, uint16_t* count);

uint16_t fifoBuf_putByte(struct fifo_t *buf, const uint8_t b);

uint16_t fifoBuf_putData(struct fifo_t *buf, const void *data, uint16_t len);

void fifoBuf_init(struct fifo_t *buf, const void *buffer, const uint16_t buffer_size);

int ringbuffer_filter(struct fifo_t* buf, int (*callback)(void* ctx, uint8_t *b), void* ctx);
uint16_t append_data(struct fifo_t* buf, const void* data, uint16_t len, int (*callback)(void* ctx, uint8_t *b), void* ctx);


uint16_t append_byte(struct fifo_t* buf, uint8_t b, int (*callback)(void* ctx, uint8_t *b), void* ctx);


int append_data_to_cache(struct cache_t* cache, uint8_t* data, size_t len, int (*callback)(void* ctx, uint8_t *b), void* ctx);
int append_byte_to_cache(struct cache_t* cache, uint8_t b, int (*callback)(void* ctx, uint8_t *b), void* ctx);
void init_cache(struct cache_t* cache, const void* buffer, uint16_t capacity);

uint16_t fifo_popData(struct fifo_t* buf,uint16_t len, int (*callback)(void* ctx, uint8_t *b), void* ctx);
int16_t fifo_pop(struct fifo_t* buf, int (*callback)(void* ctx, uint8_t *b), void* ctx);

int ringbuffer_putU16(struct fifo_t* buf, uint16_t data);
int ringbuffer_putU32(struct fifo_t* buf, uint32_t data);
int ringbuffer_getU16(struct fifo_t* buf, uint16_t* data);
int ringbuffer_getU32(struct fifo_t* buf, uint32_t* data);
int ringbuffer_popU8(struct fifo_t* buf, uint8_t* data);
int ringbuffer_pushU8(struct fifo_t* buf, uint8_t data);

#endif /* _FIFO_BUFFER_H_ */

/**
 * @}
 */