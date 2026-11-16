/*
 * Bounded proof for the fixed-size A-XDR values.
 *
 * The tag is symbolic but constrained to the tags whose payload size is
 * fixed by the encoding. The buffer length is symbolic, so both the
 * complete-value and the truncated-value paths are checked.
 */
#include <stddef.h>
#include <stdint.h>

#include "components/protocol/dlms/include/nd_axdr.h"

#define INPUT_MAX 16

static size_t payload_size(uint8_t tag)
{
    switch (tag) {
    case ND_AXDR_NULL:
        return 0;
    case ND_AXDR_BOOLEAN:
    case ND_AXDR_ENUM:
    case ND_AXDR_UNSIGNED:
    case ND_AXDR_INTEGER:
    case ND_AXDR_DELTA_INTEGER:
    case ND_AXDR_DELTA_UNSIGNED:
        return 1;
    case ND_AXDR_LONG:
    case ND_AXDR_LONG_UNSIGNED:
    case ND_AXDR_DELTA_LONG:
    case ND_AXDR_DELTA_LONG_UNSIGNED:
        return 2;
    case ND_AXDR_DOUBLE_LONG:
    case ND_AXDR_DOUBLE_LONG_UNSIGNED:
    case ND_AXDR_DELTA_DOUBLE_LONG:
    case ND_AXDR_DELTA_DOUBLE_LONG_UNSIGNED:
    case ND_AXDR_FLOAT32:
    case ND_AXDR_TIME:
        return 4;
    case ND_AXDR_LONG64:
    case ND_AXDR_LONG64_UNSIGNED:
    case ND_AXDR_FLOAT64:
        return 8;
    case ND_AXDR_DATE:
        return 5;
    case ND_AXDR_DATE_TIME:
        return 12;
    default:
        return (size_t)-1;
    }
}

int main(void)
{
    uint8_t input[INPUT_MAX];
    size_t length;

    __CPROVER_assume(length >= 1 && length <= INPUT_MAX);
    __CPROVER_assume(input[0] == ND_AXDR_NULL || input[0] == ND_AXDR_BOOLEAN
                     || input[0] == ND_AXDR_ENUM || input[0] == ND_AXDR_UNSIGNED
                     || input[0] == ND_AXDR_INTEGER || input[0] == ND_AXDR_DELTA_INTEGER
                     || input[0] == ND_AXDR_DELTA_UNSIGNED || input[0] == ND_AXDR_LONG
                     || input[0] == ND_AXDR_LONG_UNSIGNED || input[0] == ND_AXDR_DELTA_LONG
                     || input[0] == ND_AXDR_DELTA_LONG_UNSIGNED
                     || input[0] == ND_AXDR_DOUBLE_LONG
                     || input[0] == ND_AXDR_DOUBLE_LONG_UNSIGNED
                     || input[0] == ND_AXDR_DELTA_DOUBLE_LONG
                     || input[0] == ND_AXDR_DELTA_DOUBLE_LONG_UNSIGNED
                     || input[0] == ND_AXDR_FLOAT32 || input[0] == ND_AXDR_TIME
                     || input[0] == ND_AXDR_LONG64 || input[0] == ND_AXDR_LONG64_UNSIGNED
                     || input[0] == ND_AXDR_FLOAT64 || input[0] == ND_AXDR_DATE
                     || input[0] == ND_AXDR_DATE_TIME);

    nd_axdr_reader_t reader;
    nd_axdr_reader_init(&reader, input, length);
    int result = nd_axdr_skip_value(&reader);
    size_t expected = payload_size(input[0]) + 1;

    __CPROVER_assert(reader.pos <= reader.len,
                     "reader position stays within the buffer");
    if (length >= expected) {
        __CPROVER_assert(result == 0, "complete value is accepted");
        __CPROVER_assert(reader.pos == expected, "complete value consumes its size");
    } else {
        __CPROVER_assert(result != 0, "truncated value is rejected");
        __CPROVER_assert(reader.err != 0, "truncated value sets the error flag");
    }

    return 0;
}
