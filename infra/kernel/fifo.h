#ifndef ND_INFRA_KERNEL_FIFO_H
#define ND_INFRA_KERNEL_FIFO_H

#include <stddef.h>
#include <stdint.h>
#include "application/ports/fifo.h"
//-1 迭代出错
//0 正常继续
//1 正常结束
struct cache_t
{
    uint32_t capacity;
    uint32_t used;
    //uint8_t buf[0];
    uint8_t* buf;
};

// *********************
// The shared FIFO API is declared in application/ports/fifo.h. This header
// adds the legacy helpers that only the kernel and protocol code use.

uint16_t fifoBuf_update_wr(struct fifo_t* buf, uint16_t len);
int fifoBuf_pushU8(struct fifo_t* buf, uint8_t b);


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

uint16_t apdu_cache_getFree(struct fifo_t *buf);
#endif
