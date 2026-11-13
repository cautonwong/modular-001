#include "application/adapters/dl698/report_encoder.h"

#define ND_REPORT_ENCODER_ARRAY 0x01u
#define ND_REPORT_ENCODER_BIT_STRING 0x04u

static int put_byte(nd_report_encoder_t *encoder, uint8_t value)
{
    if (encoder->offset >= encoder->capacity) {
        return -1;
    }
    encoder->buffer[encoder->offset++] = value;
    return 0;
}

static int put_u32_be(nd_report_encoder_t *encoder, uint32_t value)
{
    if (encoder->capacity - encoder->offset < 4u) {
        return -1;
    }
    encoder->buffer[encoder->offset++] = (uint8_t)(value >> 24);
    encoder->buffer[encoder->offset++] = (uint8_t)(value >> 16);
    encoder->buffer[encoder->offset++] = (uint8_t)(value >> 8);
    encoder->buffer[encoder->offset++] = (uint8_t)value;
    return 0;
}

int nd_report_encoder_init(nd_report_encoder_t *encoder, uint8_t *buffer,
                           size_t capacity)
{
    if ((encoder == NULL) || (buffer == NULL)) {
        return -1;
    }
    encoder->buffer = buffer;
    encoder->capacity = capacity;
    encoder->offset = 0;
    encoder->apdu_start = 0;
    encoder->length_offset = 0;
    return 0;
}

int nd_report_encoder_append(nd_report_encoder_t *encoder, const void *data,
                             size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;

    if ((encoder == NULL) || (bytes == NULL)
        || (encoder->capacity - encoder->offset < length)) {
        return -1;
    }
    while (length-- > 0u) {
        encoder->buffer[encoder->offset++] = *bytes++;
    }
    return 0;
}

int nd_report_encoder_begin_3011(nd_report_encoder_t *encoder, uint8_t piid,
                                 int with_length_prefix)
{
    if (encoder == NULL) {
        return -1;
    }
    if (with_length_prefix) {
        if (encoder->capacity < 3u) {
            return -1;
        }
        encoder->buffer[0] = 0;
        encoder->buffer[1] = 0;
        encoder->buffer[2] = 0;
        encoder->offset = 3;
        encoder->length_offset = 0;
    } else {
        encoder->offset = 0;
        encoder->length_offset = (size_t)-1;
    }
    encoder->apdu_start = encoder->offset;

    if ((put_byte(encoder, 0x88) != 0) || (put_byte(encoder, 0x02) != 0)
        || (put_byte(encoder, (uint8_t)(piid & 0x3Fu)) != 0)
        || (put_byte(encoder, 0x01) != 0)
        || (put_u32_be(encoder, 0x30110201u) != 0)) {
        return -1;
    }
    return 0;
}

int nd_report_encoder_3011_rcsd(nd_report_encoder_t *encoder,
                                const uint32_t *oads, uint8_t count)
{
    uint8_t i;

    if ((encoder == NULL) || (oads == NULL) || (count > 30u)) {
        return -1;
    }
    if (put_byte(encoder, count) != 0) {
        return -1;
    }
    for (i = 0; i < count; i++) {
        if ((put_byte(encoder, 0) != 0)
            || (put_u32_be(encoder, oads[i]) != 0)) {
            return -1;
        }
    }
    return 0;
}

int nd_report_encoder_finish_3011(nd_report_encoder_t *encoder)
{
    size_t length;

    if ((encoder == NULL) || (encoder->length_offset == (size_t)-1)) {
        return 0;
    }
    length = encoder->offset - encoder->apdu_start;
    encoder->buffer[0] = 0x10;
    encoder->buffer[1] = 0x00;
    encoder->buffer[2] = (uint8_t)length;
    return 0;
}

int nd_report_encoder_begin_normal(nd_report_encoder_t *encoder, uint8_t piid,
                                   uint8_t count)
{
    if (encoder == NULL) {
        return -1;
    }
    encoder->offset = 0;
    encoder->length_offset = 2;
    if ((put_byte(encoder, 0x10) != 0) || (put_byte(encoder, 0x00) != 0)
        || (put_byte(encoder, 0x00) != 0)) {
        return -1;
    }
    encoder->apdu_start = encoder->offset;
    if ((put_byte(encoder, 0x88) != 0) || (put_byte(encoder, 0x01) != 0)
        || (put_byte(encoder, (uint8_t)(piid & 0x3Fu)) != 0)
        || (put_byte(encoder, count) != 0)) {
        return -1;
    }
    return 0;
}

int nd_report_encoder_3320(nd_report_encoder_t *encoder, const uint8_t *items,
                           uint8_t count)
{
    if ((encoder == NULL) || (items == NULL)) {
        return -1;
    }
    if ((put_byte(encoder, 0x33) != 0) || (put_byte(encoder, 0x20) != 0)
        || (put_byte(encoder, 0x02) != 0) || (put_byte(encoder, 0x00) != 0)
        || (put_byte(encoder, 0x01) != 0)
        || (put_byte(encoder, ND_REPORT_ENCODER_ARRAY) != 0)
        || (put_byte(encoder, count) != 0)) {
        return -1;
    }
    return nd_report_encoder_append(encoder, items, 5u * count);
}

int nd_report_encoder_2015(nd_report_encoder_t *encoder, uint32_t value)
{
    if (encoder == NULL) {
        return -1;
    }
    if ((put_byte(encoder, 0x20) != 0) || (put_byte(encoder, 0x15) != 0)
        || (put_byte(encoder, 0x02) != 0) || (put_byte(encoder, 0x00) != 0)
        || (put_byte(encoder, 0x01) != 0)
        || (put_byte(encoder, ND_REPORT_ENCODER_BIT_STRING) != 0)
        || (put_byte(encoder, 0x20) != 0)) {
        return -1;
    }
    return put_u32_be(encoder, value);
}

int nd_report_encoder_end_normal(nd_report_encoder_t *encoder)
{
    size_t length;

    if (encoder == NULL) {
        return -1;
    }
    if ((put_byte(encoder, 0x00) != 0) || (put_byte(encoder, 0x00) != 0)) {
        return -1;
    }
    length = encoder->offset - encoder->apdu_start;
    encoder->buffer[encoder->length_offset] = (uint8_t)length;
    return 0;
}

int nd_report_encoder_rn_mac_tail(nd_report_encoder_t *encoder,
                                  const uint8_t *random12,
                                  const uint8_t *esam_tail,
                                  int esam_abnormal)
{
    uint8_t mac[4] = {0xFFu, 0xFFu, 0xFFu, 0xFFu};
    uint8_t marker = 0x04;

    if ((encoder == NULL) || (random12 == NULL)) {
        return -1;
    }
    if (!esam_abnormal) {
        if (esam_tail == NULL) {
            return -1;
        }
        mac[0] = esam_tail[13];
        mac[1] = esam_tail[14];
        mac[2] = esam_tail[15];
        mac[3] = esam_tail[16];
    }

    if ((put_byte(encoder, 0x02) != 0) || (put_byte(encoder, 0x0C) != 0)
        || (nd_report_encoder_append(encoder, random12, 12) != 0)
        || (put_byte(encoder, marker) != 0)
        || (nd_report_encoder_append(encoder, mac, sizeof(mac)) != 0)) {
        return -1;
    }
    return 0;
}
