#ifndef ND_AXDR_H
#define ND_AXDR_H

#include <stddef.h>
#include <stdint.h>

/* Six-byte OBIS logical name. Values live in mapper_idis/nd_idis_ids.h. */
typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t f;
} nd_obis_t;

typedef struct {
    int8_t scaler;
    uint8_t unit;
} nd_scaler_unit_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t day_of_week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t hundredths;
    int16_t deviation;
    uint8_t status;
} nd_dlms_datetime_t;

typedef enum {
    ND_AXDR_NULL = 0x00,
    ND_AXDR_ARRAY = 0x01,
    ND_AXDR_STRUCTURE = 0x02,
    ND_AXDR_BOOLEAN = 0x03,
    ND_AXDR_BIT_STRING = 0x04,
    ND_AXDR_DOUBLE_LONG = 0x05,
    ND_AXDR_DOUBLE_LONG_UNSIGNED = 0x06,
    ND_AXDR_OCTET_STRING = 0x09,
    ND_AXDR_VISIBLE_STRING = 0x0A,
    ND_AXDR_UTF8_STRING = 0x0C,
    ND_AXDR_BCD = 0x0D,
    ND_AXDR_INTEGER = 0x0F,
    ND_AXDR_LONG = 0x10,
    ND_AXDR_UNSIGNED = 0x11,
    ND_AXDR_LONG_UNSIGNED = 0x12,
    ND_AXDR_LONG64 = 0x14,
    ND_AXDR_LONG64_UNSIGNED = 0x15,
    ND_AXDR_ENUM = 0x16,
    ND_AXDR_FLOAT32 = 0x17,
    ND_AXDR_FLOAT64 = 0x18,
    ND_AXDR_DATE_TIME = 0x19,
    ND_AXDR_DATE = 0x1A,
    ND_AXDR_TIME = 0x1B,
    ND_AXDR_DELTA_INTEGER = 0x1C,
    ND_AXDR_DELTA_LONG = 0x1D,
    ND_AXDR_DELTA_DOUBLE_LONG = 0x1E,
    ND_AXDR_DELTA_UNSIGNED = 0x1F,
    ND_AXDR_DELTA_LONG_UNSIGNED = 0x20,
    ND_AXDR_DELTA_DOUBLE_LONG_UNSIGNED = 0x21,
    ND_AXDR_DONT_CARE = 0xFF
} nd_axdr_tag_t;

typedef struct {
    uint8_t *buf;
    size_t cap;
    size_t len;
    int err;
} nd_axdr_writer_t;

typedef struct {
    const uint8_t *buf;
    size_t len;
    size_t pos;
    int err;
} nd_axdr_reader_t;

void nd_axdr_writer_init(nd_axdr_writer_t *writer, uint8_t *buf, size_t cap);
void nd_axdr_put_tag(nd_axdr_writer_t *writer, uint8_t tag);
void nd_axdr_put_length(nd_axdr_writer_t *writer, uint16_t length);
void nd_axdr_put_array_header(nd_axdr_writer_t *writer, uint16_t count);
void nd_axdr_put_structure_header(nd_axdr_writer_t *writer, uint16_t count);
void nd_axdr_put_u8(nd_axdr_writer_t *writer, uint8_t value);
void nd_axdr_put_u16(nd_axdr_writer_t *writer, uint16_t value);
void nd_axdr_put_u32(nd_axdr_writer_t *writer, uint32_t value);
void nd_axdr_put_i16(nd_axdr_writer_t *writer, int16_t value);
void nd_axdr_put_octets(nd_axdr_writer_t *writer, const uint8_t *data,
                        size_t length);
int nd_axdr_writer_ok(const nd_axdr_writer_t *writer);

void nd_axdr_reader_init(nd_axdr_reader_t *reader, const uint8_t *buf,
                         size_t length);
uint8_t nd_axdr_get_tag(nd_axdr_reader_t *reader);
uint16_t nd_axdr_get_length(nd_axdr_reader_t *reader);
uint8_t nd_axdr_get_u8(nd_axdr_reader_t *reader);
uint16_t nd_axdr_get_u16(nd_axdr_reader_t *reader);
uint32_t nd_axdr_get_u32(nd_axdr_reader_t *reader);
int16_t nd_axdr_get_i16(nd_axdr_reader_t *reader);
void nd_axdr_get_octets(nd_axdr_reader_t *reader, uint8_t *out, size_t length);
/* Advances over exactly one A-XDR value; returns 0 on success. */
int nd_axdr_skip_value(nd_axdr_reader_t *reader);
size_t nd_axdr_remaining(const nd_axdr_reader_t *reader);
int nd_axdr_reader_ok(const nd_axdr_reader_t *reader);

#endif
