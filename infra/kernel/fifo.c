#include "infra/kernel/fifo.h"

#include <string.h>


// *****************************************************************************
// circular buffer functions

uint16_t fifoBuf_getSize(struct fifo_t *buf)
{       // return the usable size of the buffer

    uint16_t buf_size = buf->buf_size;

    if (buf_size > 0)
      return (buf_size - 1);
    else
      return 0;
}

uint16_t fifoBuf_update_wr(struct fifo_t* buf, uint16_t len)
{  
    uint16_t wr = buf->wr;
    uint16_t buf_size = buf->buf_size;

    wr += len;
    if (wr >= buf_size)
        wr -= buf_size;
    buf->wr = wr;
    return wr;
}

uint16_t fifoBuf_update_rd(struct fifo_t* buf, uint16_t len, uint8_t type)
{
    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;
    
    if(type){
        rd += len;
        if (rd >= buf_size)
            rd -= buf_size;
    }else{
        if(rd >= len)
            rd -= len;
        else
            rd = buf_size - len - rd;
    }

    buf->rd = rd;
    return rd;
}

uint16_t fifoBuf_getUsed(struct fifo_t *buf)
{       // return the number of bytes available in the rx buffer

    uint16_t rd = buf->rd;
    uint16_t wr = buf->wr;
    uint16_t buf_size = buf->buf_size;

    uint16_t num_bytes = wr - rd;
    if (wr < rd)
        num_bytes = (buf_size - rd) + wr;

    return num_bytes;
}
uint16_t fifoBuf_getFree(struct fifo_t *buf)
{       // return the free space size in the buffer

    uint16_t buf_size = buf->buf_size;

    uint16_t num_bytes = fifoBuf_getUsed(buf);

    return ((buf_size - num_bytes) - 1);
}

void ringbuffer_clear(struct fifo_t *buf)
{       // remove all data from the buffer
    buf->wr = 0;
	buf->rd = buf->wr;
}

void fifoBuf_removeData(struct fifo_t *buf, uint16_t len)
{       // remove a number of bytes from the buffer

    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);

    if (num_bytes > len)
        num_bytes = len;

    if (num_bytes < 1)
        return;                         // nothing to remove

    rd += num_bytes;
    if (rd >= buf_size)
        rd -= buf_size;

    buf->rd = rd;
}

int16_t fifoBuf_patchByteB(struct fifo_t *buf,uint8_t b, uint16_t offset)
{       // remove a number of bytes from the buffer

    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);
    if (num_bytes < 1)
        return -1;                         // nothing to remove

    //n_assert(num_bytes >= offset);
    if (num_bytes > offset)
        num_bytes = offset;

    rd += num_bytes;
    if (rd >= buf_size)
        rd -= buf_size;

    buf->buf_ptr[rd] = b;
    return 0;            // return the byte
}

uint16_t fifo_popData(struct fifo_t* buf,uint16_t len, int (*callback)(void* ctx, uint8_t *b), void* ctx)
{      
    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);

    if (num_bytes >= len)
        num_bytes = len;
    else
        return 0;//发送数据不够

    if (num_bytes < 1)
        return 0;               // return number of bytes copied

   // uint8_t* p = (uint8_t*)data;
    uint16_t i = 0;

    while (num_bytes > 0)
    {
        if (callback != NULL)
        {
            callback(ctx, buff + rd);
        }
        //*p++ = buff[rd];
        num_bytes--;
        if (++rd >= buf_size)
            rd = 0;
    }

    buf->rd = rd;

    return i;                   // return number of bytes copied
}

int16_t fifo_pop(struct fifo_t* buf, int (*callback)(void* ctx, uint8_t *b), void* ctx)
{   
    uint8_t b;
    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    uint16_t num_bytes = fifoBuf_getUsed(buf);
    //n_assert(num_bytes >= 1);
    if (num_bytes < 1)
        return -1;                      // no byte returned

    if (callback != NULL)
    {
        callback(ctx, buff + rd);
    }
    
    b = buff[rd];
    if (++rd >= buf_size)
        rd = 0;

    buf->rd = rd;

    return b;                           // return the byte
}

uint16_t append_byte(struct fifo_t* buf, uint8_t b, int (*callback)(void* ctx, uint8_t *b), void* ctx)
{
    uint16_t wr = buf->wr;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    uint16_t num_bytes = fifoBuf_getFree(buf);
    //n_assert(num_bytes >= 1);//overflow
    if (num_bytes < 1)
        return 0;

    buff[wr] = b;
    if (callback != NULL)
    {
        callback(ctx, buff+wr);
    }

    if (++wr >= buf_size)
        wr = 0;


    buf->wr = wr;

    return 1;                   // return number of bytes copied

}

int ringbuffer_filter(struct fifo_t *buf, int (*callback)(void* ctx, uint8_t *b),void *ctx)
{       
    uint16_t i = 0;
    uint16_t rd = buf->rd;
    uint16_t wr = buf->wr;
    uint16_t r = 0;
    int ret = 0;
    uint16_t buf_size = buf->buf_size;
    uint16_t num_bytes = fifoBuf_getUsed(buf);
    if (num_bytes < 1)
        return -1;                        

    while(i < num_bytes)
    {
        r = rd + i;
        if (r >= buf_size)
            r -= buf_size;
        ret =   callback(ctx, buf->buf_ptr+r);
        if(ret < 0)
        {
            if (++rd >= buf_size)//删掉一字节
                rd = 0;
            
            num_bytes = wr - rd;
            if (wr < rd)
            {
                num_bytes = (buf_size - rd) + wr;
            }            
            buf->rd = rd;
            i = 0;
            if (num_bytes < 1)
            {
                return ret;//-1迭代结束  没找到
            }
            continue;
        }
        else if(ret == 1)
        {
            return ++i;//解析成功
        }
        else if (ret  == 2)
        {
            return 0;//字节不够  下次再来
        }
        else//  ret == 0  keep going
        {
            ++i;//已符合迭代器要求的字节数
        }
    }
    buf->rd = buf->wr;
    return -1;            
}

int16_t fifoBuf_peekByteB(struct fifo_t *buf, uint16_t offset)
{       // remove a number of bytes from the buffer

    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);
    if (num_bytes < 1)
        return -1;                         // nothing to remove

    //n_assert(num_bytes >= offset);
    if (num_bytes > offset)
        num_bytes = offset;

    rd += num_bytes;
    if (rd >= buf_size)
        rd -= buf_size;

    return buf->buf_ptr[rd];            // return the byte
}

int16_t fifoBuf_peekByteBuf(struct fifo_t* p_fifo, uint16_t offset, uint8_t* data, uint16_t len)
{
    uint16_t i = 0;
    int16_t val = 0;
    for (i = 0; i < len; i++) {
        val = fifoBuf_peekByteB(p_fifo, offset+i);
        if (val < 0) {
            return -1;
        }
        data[i] = val;
    }
    return i;
}

int16_t fifoBuf_getBytePeek(struct fifo_t *buf)
{	// get a data byte from the buffer without removing it

    uint16_t rd = buf->rd;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);
    //n_assert(num_bytes >= 1);
    if (num_bytes < 1)
        return -1;                      // no byte retuened

    return buf->buf_ptr[rd];            // return the byte
}

uint8_t fifoBuf_getByte(struct fifo_t *buf)
{       // get a data byte from the buffer

    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;
    uint8_t *buff = buf->buf_ptr;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);
    //n_assert(num_bytes >= 1);
    if (num_bytes < 1)
        return 0;                      // no byte returned

    uint8_t b = buff[rd];
    if (++rd >= buf_size)
        rd = 0;

    buf->rd = rd;

    return b;                           // return the byte
}

int16_t fifoBuf_getByteCount(struct fifo_t* buf,uint16_t *count)
{       // get a data byte from the buffer
    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);
    //n_assert(num_bytes >= 1);
    if (num_bytes < 1)
        return -1;                      // no byte returned

    uint8_t b = buff[rd];
    if (++rd >= buf_size)
        rd = 0;

    *count++;
    buf->rd = rd;

    return b;                           // return the byte
}

uint16_t fifoBuf_getDataPeek(struct fifo_t *buf, void *data, uint16_t len)
{       // get data from the buffer without removing it

    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;
    uint8_t *buff = buf->buf_ptr;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);

    if (num_bytes > len)
        num_bytes = len;

    if (num_bytes < 1)
        return 0;		// return number of bytes copied

    uint8_t *p = (uint8_t *)data;
    uint16_t i = 0;

    while (num_bytes > 0)
    {
        uint16_t j = buf_size - rd;
        if (j > num_bytes)
            j = num_bytes;
        memcpy(p + i, buff + rd, j);
        i += j;
        num_bytes -= j;
        rd += j;
        if (rd >= buf_size)
            rd = 0;
    }

    return i;                   // return number of bytes copied
}

uint16_t fifoBuf_getData(struct fifo_t *buf, void *data, uint16_t len)
{       // get data from our rx buffer

    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;
    uint8_t *buff = buf->buf_ptr;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);

    if (num_bytes > len)
        num_bytes = len;

    if (num_bytes < 1)
        return 0;               // return number of bytes copied

    uint8_t *p = (uint8_t *)data;
    uint16_t i = 0;

    while (num_bytes > 0)
    {
        uint16_t j = buf_size - rd;
        if (j > num_bytes)
            j = num_bytes;
        memcpy(p + i, buff + rd, j);
        i += j;
        num_bytes -= j;
        rd += j;
        if (rd >= buf_size)
            rd = 0;
    }

    buf->rd = rd;

    return i;                   // return number of bytes copied
}

int ringbuffer_getU32(struct fifo_t* buf, uint32_t* data)
{       // get data from our rx buffer

    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);

    if (num_bytes > 4)
        num_bytes = 4;

    if (num_bytes < 1)
        return -1;               // return number of bytes copied

    *data = 0;

    while (num_bytes > 0)
    {
        *data |= (buff[rd] << (num_bytes - 1) * 8);
        num_bytes--;
        if (++rd >= buf_size)
            rd = 0;
    }

    buf->rd = rd;

    return 4;                   // return number of bytes copied
}

int ringbuffer_putU32(struct fifo_t* buf, uint32_t data)
{       // add data to the buffer

    uint16_t wr = buf->wr;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    uint16_t num_bytes = fifoBuf_getFree(buf);
    if (num_bytes > 4)
        num_bytes = 4;
    //n_assert(num_bytes >= len);//overflow
    if (num_bytes < 4)
    {
        wr = buf->wr;
    }

    if (num_bytes < 1)
        return -1;               // return number of bytes copied

    while (num_bytes > 0)
    {
        buff[wr] = (data >> (num_bytes - 1) * 8);
        num_bytes--;
        if (++wr >= buf_size)
            wr = 0;
    }

    buf->wr = wr;

    return 4;                   
}

int ringbuffer_getU16(struct fifo_t* buf, uint16_t* data)
{       // get data from our rx buffer

    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    // get number of bytes available
    uint16_t num_bytes = fifoBuf_getUsed(buf);

    if (num_bytes > 2)
        num_bytes = 2;

    if (num_bytes < 1)
        return -1;               // return number of bytes copied

    *data = 0;

    while (num_bytes > 0)
    {
        *data |= (buff[rd] << (num_bytes - 1) * 8);
        num_bytes--;
        if (++rd >= buf_size)
            rd = 0;
    }

    buf->rd = rd;

    return 4;                   // return number of bytes copied
}
int ringbuffer_putU16(struct fifo_t* buf, uint16_t data)
{       // add data to the buffer

    uint16_t wr = buf->wr;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    uint16_t num_bytes = fifoBuf_getFree(buf);
    if (num_bytes > 2)
        num_bytes = 2;
    //n_assert(num_bytes >= len);//overflow
    if (num_bytes < 2)
    {
        wr = buf->wr;
    }

    if (num_bytes < 1)
        return -1;               // return number of bytes copied

    while (num_bytes > 0)
    {
        buff[wr] = (data >> (num_bytes - 1) * 8);
        num_bytes--;
        if (++wr >= buf_size)
            wr = 0;
    }

    buf->wr = wr;

    return 4;
}

int ringbuffer_popU8(struct fifo_t* buf, uint8_t* data)
{       // get data from our rx buffer

    uint16_t rd = buf->rd;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    uint16_t num_bytes = fifoBuf_getUsed(buf);
    if (num_bytes < 1)
        return -1;               // return number of bytes copied

    *data = buff[rd];
    if (++rd >= buf_size)
        rd = 0;

    buf->rd = rd;

    return 1;                   // return number of bytes copied
}

int fifoBuf_pushU8(struct fifo_t* buf, uint8_t b)
{       
    uint16_t wr = buf->wr;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    uint16_t num_bytes = fifoBuf_getFree(buf);
    //n_assert(num_bytes >= 1);//overflow
    if (num_bytes < 1)
        return -1;

    buff[wr] = b;
    if (++wr >= buf_size)
        wr = 0;

    buf->wr = wr;

    return 1;                   
}

uint16_t fifoBuf_putByte(struct fifo_t *buf, const uint8_t b)
{       // add a data byte to the buffer
    uint16_t wr = buf->wr;
    uint16_t buf_size = buf->buf_size;
    uint8_t *buff = buf->buf_ptr;

    uint16_t num_bytes = fifoBuf_getFree(buf);
    //n_assert(num_bytes >= 1);//overflow
    //if (num_bytes < 1)
    //    return 0;

    buff[wr] = b;
    if (++wr >= buf_size)
        wr = 0;

    buf->wr = wr;

    return 1;                   // return number of bytes copied
}

uint16_t fifoBuf_putData(struct fifo_t *buf, const void *data, uint16_t len)
{       // add data to the buffer

    uint16_t wr = buf->wr;
    uint16_t buf_size = buf->buf_size;
    uint8_t *buff = buf->buf_ptr;

    uint16_t num_bytes = fifoBuf_getFree(buf);
    if (num_bytes > len)
        num_bytes = len;
    //n_assert(num_bytes >= len);//overflow
    if (num_bytes < len)
    {
        wr = buf->wr;
    }

    if ((num_bytes < 1) || (buf_size == 0))
        return 0;               // return number of bytes copied

    uint8_t *p = (uint8_t *)data;
    uint16_t i = 0;

    while (num_bytes > 0)
    {
        uint16_t j = buf_size - wr;
        if (j > num_bytes)
            j = num_bytes;
        memcpy(buff + wr, p + i, j);
        i += j;
        num_bytes -= j;
        wr += j;
        if (wr >= buf_size)
            wr = 0;
    }

    buf->wr = wr;

    return i;                   // return number of bytes copied
}

uint16_t append_data(struct fifo_t* buf, const void* data, uint16_t len, int (*callback)(void* ctx, uint8_t *b), void* ctx)
{       // add data to the buffer

    uint16_t wr = buf->wr;
    uint16_t buf_size = buf->buf_size;
    uint8_t* buff = buf->buf_ptr;

    uint16_t num_bytes = fifoBuf_getFree(buf);
    if (num_bytes > len)
        num_bytes = len;
    //n_assert(num_bytes >= len);//overflow
    if (num_bytes < len)
    {
        wr = buf->wr;
    }

    if (num_bytes < 1)
        return 0;               // return number of bytes copied

    uint8_t* p = (uint8_t*)data;
    uint16_t i = 0;

    while (num_bytes > 0)
    {
        buff[wr] = *p++;
        if (callback != NULL)
        {
            callback(ctx, buff+wr);
        }
        if (++wr >= buf_size)
            wr = 0;
        num_bytes--;
    }

    buf->wr = wr;

    return i;                   // return number of bytes copied
}

void fifoBuf_init(struct fifo_t *buf, const void *buffer, const uint16_t buffer_size)
{
    buf->buf_ptr = (uint8_t *)buffer;
    buf->rd = 0;
    buf->wr = 0;
    buf->buf_size = buffer_size;
}

void init_cache(struct cache_t* cache, const void* buffer, uint16_t capacity)
{
    cache->buf = (uint8_t*)buffer;
    cache->used = 0;
    cache->capacity = capacity;
}

int append_byte_to_cache(struct cache_t* cache, uint8_t b, int (*callback)(void* ctx, uint8_t *b), void* ctx)
{
    uint32_t used = cache->used;
    if (used < cache->capacity)
    {
        cache->buf[used] = b;
        if (callback != NULL)
        {
            callback(ctx, cache->buf+used);
        }
        cache->used++;
        return 1;
    }

    //n_assert(0);
    return 0;
}

int append_data_to_cache(struct cache_t* cache, uint8_t* data, size_t len, fifo_callback_t callback, void* ctx)
{
    uint32_t used = cache->used;
    if (used + len > cache->capacity)
    {
        return 0;
    }

    for (size_t i = 0; i < len; i++)
    {
        cache->buf[used] = data[i];
        if (callback != NULL)
        {
            callback(ctx, cache->buf + used);
        }
        used++;
    }

    cache->used = used;
    return len;
}



//用于协议apdu 2k缓存计算  剩余大小
uint16_t apdu_cache_getFree(struct fifo_t *buf)
{       // return the free space size in the buffer

    uint16_t buf_size = buf->buf_size;
    uint16_t rd = buf->rd;
    uint16_t wr = buf->wr;
    uint16_t num_bytes = wr - rd;

    if(num_bytes >= (buf_size - 1))   
    {
        num_bytes = 0;
    }
    else
    {
        num_bytes = buf_size - num_bytes - 1;
    }

    return num_bytes;
}

