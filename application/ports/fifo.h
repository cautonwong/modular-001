#ifndef ND_APPLICATION_PORTS_FIFO_H
#define ND_APPLICATION_PORTS_FIFO_H

#include <stdint.h>

typedef int (*fifo_callback_t)(void *ctx, uint8_t *b);

struct fifo_t {
    uint8_t *buf_ptr;
    volatile uint16_t rd;
    volatile uint16_t wr;
    uint16_t buf_size;
};

uint16_t fifoBuf_getSize(struct fifo_t *buf);
uint16_t fifoBuf_getUsed(struct fifo_t *buf);
uint16_t fifoBuf_getFree(struct fifo_t *buf);
uint16_t fifoBuf_end(struct fifo_t *buf, uint16_t len);
void ringbuffer_clear(struct fifo_t *buf);
void fifoBuf_removeData(struct fifo_t *buf, uint16_t len);
int16_t fifoBuf_getBytePeek(struct fifo_t *buf);
uint8_t fifoBuf_getByte(struct fifo_t *buf);
int16_t fifoBuf_peekByteB(struct fifo_t *buf, uint16_t offset);
int16_t fifoBuf_peekByteBuf(struct fifo_t *buf, uint16_t offset, uint8_t *data,
                            uint16_t len);
int16_t fifoBuf_patchByteB(struct fifo_t *buf, uint8_t b, uint16_t offset);
uint16_t fifoBuf_getDataPeek(struct fifo_t *buf, void *data, uint16_t len);
uint16_t fifoBuf_getData(struct fifo_t *buf, void *data, uint16_t len);
int16_t fifoBuf_getByteCount(struct fifo_t *buf, uint16_t *count);
uint16_t fifoBuf_putByte(struct fifo_t *buf, const uint8_t b);
uint16_t fifoBuf_putData(struct fifo_t *buf, const void *data, uint16_t len);
uint16_t fifoBuf_update_rd(struct fifo_t *buf, uint16_t len, uint8_t type);
void fifoBuf_init(struct fifo_t *buf, const void *buffer,
                  const uint16_t buffer_size);
int ringbuffer_filter(struct fifo_t *buf,
                      int (*callback)(void *ctx, uint8_t *b), void *ctx);
uint16_t append_data(struct fifo_t *buf, const void *data, uint16_t len,
                     int (*callback)(void *ctx, uint8_t *b), void *ctx);
uint16_t append_byte(struct fifo_t *buf, uint8_t b,
                     int (*callback)(void *ctx, uint8_t *b), void *ctx);
uint16_t fifo_popData(struct fifo_t *buf, uint16_t len,
                      int (*callback)(void *ctx, uint8_t *b), void *ctx);
uint16_t apdu_cache_getFree(struct fifo_t *buf);

#endif
