#include "components/protocol/dlms/include/nd_axdr.h"

void nd_axdr_writer_init(nd_axdr_writer_t *writer, uint8_t *buf, size_t cap)
{
    writer->buf = buf;
    writer->cap = cap;
    writer->len = 0;
    writer->err = 0;
}

static int reserve(nd_axdr_writer_t *writer, size_t length)
{
    if (writer->err != 0 || length > writer->cap - writer->len) {
        writer->err = 1;
        return 0;
    }
    return 1;
}

void nd_axdr_put_tag(nd_axdr_writer_t *writer, uint8_t tag)
{
    nd_axdr_put_u8(writer, tag);
}

void nd_axdr_put_length(nd_axdr_writer_t *writer, uint16_t length)
{
    if (length < 0x80) {
        nd_axdr_put_u8(writer, (uint8_t)length);
    } else if (length <= 0xFF) {
        nd_axdr_put_u8(writer, 0x81);
        nd_axdr_put_u8(writer, (uint8_t)length);
    } else {
        nd_axdr_put_u8(writer, 0x82);
        nd_axdr_put_u16(writer, length);
    }
}

void nd_axdr_put_array_header(nd_axdr_writer_t *writer, uint16_t count)
{
    nd_axdr_put_tag(writer, ND_AXDR_ARRAY);
    nd_axdr_put_length(writer, count);
}

void nd_axdr_put_structure_header(nd_axdr_writer_t *writer, uint16_t count)
{
    nd_axdr_put_tag(writer, ND_AXDR_STRUCTURE);
    nd_axdr_put_length(writer, count);
}

void nd_axdr_put_u8(nd_axdr_writer_t *writer, uint8_t value)
{
    if (reserve(writer, 1)) {
        writer->buf[writer->len++] = value;
    }
}

void nd_axdr_put_u16(nd_axdr_writer_t *writer, uint16_t value)
{
    if (reserve(writer, 2)) {
        writer->buf[writer->len++] = (uint8_t)(value >> 8);
        writer->buf[writer->len++] = (uint8_t)value;
    }
}

void nd_axdr_put_u32(nd_axdr_writer_t *writer, uint32_t value)
{
    if (reserve(writer, 4)) {
        writer->buf[writer->len++] = (uint8_t)(value >> 24);
        writer->buf[writer->len++] = (uint8_t)(value >> 16);
        writer->buf[writer->len++] = (uint8_t)(value >> 8);
        writer->buf[writer->len++] = (uint8_t)value;
    }
}

void nd_axdr_put_i16(nd_axdr_writer_t *writer, int16_t value)
{
    nd_axdr_put_u16(writer, (uint16_t)value);
}

void nd_axdr_put_octets(nd_axdr_writer_t *writer, const uint8_t *data,
                        size_t length)
{
    if (!reserve(writer, length)) {
        return;
    }
    for (size_t i = 0; i < length; i++) {
        writer->buf[writer->len++] = data[i];
    }
}

int nd_axdr_writer_ok(const nd_axdr_writer_t *writer)
{
    return writer->err == 0;
}

void nd_axdr_reader_init(nd_axdr_reader_t *reader, const uint8_t *buf,
                         size_t length)
{
    reader->buf = buf;
    reader->len = length;
    reader->pos = 0;
    reader->err = 0;
}

static int take(nd_axdr_reader_t *reader, size_t length)
{
    if (reader->err != 0 || length > reader->len - reader->pos) {
        reader->err = 1;
        return 0;
    }
    return 1;
}

uint8_t nd_axdr_get_tag(nd_axdr_reader_t *reader)
{
    return nd_axdr_get_u8(reader);
}

uint16_t nd_axdr_get_length(nd_axdr_reader_t *reader)
{
    uint8_t first = nd_axdr_get_u8(reader);
    if (first < 0x80) {
        return first;
    }
    if (first == 0x81) {
        return nd_axdr_get_u8(reader);
    }
    if (first == 0x82) {
        return nd_axdr_get_u16(reader);
    }
    if (first == 0x83) {
        uint32_t value = (uint32_t)nd_axdr_get_u8(reader) << 16;
        value |= (uint32_t)nd_axdr_get_u8(reader) << 8;
        return (uint16_t)(value | nd_axdr_get_u8(reader));
    }
    reader->err = 1;
    return 0;
}

uint8_t nd_axdr_get_u8(nd_axdr_reader_t *reader)
{
    if (!take(reader, 1)) {
        return 0;
    }
    return reader->buf[reader->pos++];
}

uint16_t nd_axdr_get_u16(nd_axdr_reader_t *reader)
{
    if (!take(reader, 2)) {
        return 0;
    }
    uint16_t value = (uint16_t)((uint16_t)reader->buf[reader->pos] << 8);
    value |= reader->buf[reader->pos + 1];
    reader->pos += 2;
    return value;
}

uint32_t nd_axdr_get_u32(nd_axdr_reader_t *reader)
{
    if (!take(reader, 4)) {
        return 0;
    }
    uint32_t value = (uint32_t)reader->buf[reader->pos] << 24;
    value |= (uint32_t)reader->buf[reader->pos + 1] << 16;
    value |= (uint32_t)reader->buf[reader->pos + 2] << 8;
    value |= reader->buf[reader->pos + 3];
    reader->pos += 4;
    return value;
}

int16_t nd_axdr_get_i16(nd_axdr_reader_t *reader)
{
    return (int16_t)nd_axdr_get_u16(reader);
}

void nd_axdr_get_octets(nd_axdr_reader_t *reader, uint8_t *out, size_t length)
{
    if (!take(reader, length)) {
        return;
    }
    for (size_t i = 0; i < length; i++) {
        out[i] = reader->buf[reader->pos++];
    }
}

static int skip_bytes(nd_axdr_reader_t *reader, size_t length)
{
    if (!take(reader, length)) {
        return 0;
    }
    reader->pos += length;
    return 1;
}

int nd_axdr_skip_value(nd_axdr_reader_t *reader)
{
    if (reader == NULL) {
        return -1;
    }
    uint8_t tag = nd_axdr_get_tag(reader);
    switch (tag) {
    case ND_AXDR_NULL:
        break;
    case ND_AXDR_ARRAY:
    case ND_AXDR_STRUCTURE: {
        uint16_t count = nd_axdr_get_length(reader);
        for (uint16_t index = 0; index < count; index++) {
            if (nd_axdr_skip_value(reader) != 0) {
                return -1;
            }
        }
        break;
    }
    case ND_AXDR_BOOLEAN:
    case ND_AXDR_ENUM:
    case ND_AXDR_UNSIGNED:
    case ND_AXDR_INTEGER:
    case ND_AXDR_DELTA_INTEGER:
    case ND_AXDR_DELTA_UNSIGNED:
        if (!skip_bytes(reader, 1)) {
            return -1;
        }
        break;
    case ND_AXDR_LONG:
    case ND_AXDR_LONG_UNSIGNED:
    case ND_AXDR_DELTA_LONG:
    case ND_AXDR_DELTA_LONG_UNSIGNED:
        if (!skip_bytes(reader, 2)) {
            return -1;
        }
        break;
    case ND_AXDR_DOUBLE_LONG:
    case ND_AXDR_DOUBLE_LONG_UNSIGNED:
    case ND_AXDR_DELTA_DOUBLE_LONG:
    case ND_AXDR_DELTA_DOUBLE_LONG_UNSIGNED:
    case ND_AXDR_FLOAT32:
    case ND_AXDR_TIME:
        if (!skip_bytes(reader, 4)) {
            return -1;
        }
        break;
    case ND_AXDR_LONG64:
    case ND_AXDR_LONG64_UNSIGNED:
    case ND_AXDR_FLOAT64:
        if (!skip_bytes(reader, 8)) {
            return -1;
        }
        break;
    case ND_AXDR_DATE:
        if (!skip_bytes(reader, 5)) {
            return -1;
        }
        break;
    case ND_AXDR_DATE_TIME:
        if (!skip_bytes(reader, 12)) {
            return -1;
        }
        break;
    case ND_AXDR_BIT_STRING: {
        uint16_t bits = nd_axdr_get_length(reader);
        if (!skip_bytes(reader, (size_t)(bits + 7u) / 8u)) {
            return -1;
        }
        break;
    }
    case ND_AXDR_OCTET_STRING:
    case ND_AXDR_VISIBLE_STRING:
    case ND_AXDR_UTF8_STRING:
    case ND_AXDR_BCD: {
        uint16_t length = nd_axdr_get_length(reader);
        if (!skip_bytes(reader, length)) {
            return -1;
        }
        break;
    }
    default:
        reader->err = 1;
        return -1;
    }
    return nd_axdr_reader_ok(reader) ? 0 : -1;
}

size_t nd_axdr_remaining(const nd_axdr_reader_t *reader)
{
    return reader->err == 0 ? reader->len - reader->pos : 0;
}

int nd_axdr_reader_ok(const nd_axdr_reader_t *reader)
{
    return reader->err == 0;
}
