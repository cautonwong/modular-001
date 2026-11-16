/*
 * Bounded proof for the A-XDR length decoder.
 *
 * The buffer is four symbolic bytes. The decoder must never move past the
 * buffer and must raise the error flag whenever a long form runs out of
 * bytes or encodes a value above UINT16_MAX.
 */
#include <stddef.h>
#include <stdint.h>

#include "components/protocol/dlms/include/nd_axdr.h"

#define INPUT_SIZE 4

int main(void)
{
    uint8_t input[INPUT_SIZE];
    nd_axdr_reader_t reader;

    nd_axdr_reader_init(&reader, input, INPUT_SIZE);
    uint16_t length = nd_axdr_get_length(&reader);

    __CPROVER_assert(reader.pos <= reader.len,
                     "decoder position stays within the buffer");

    if (reader.err == 0 && input[0] < 0x80) {
        __CPROVER_assert(length == input[0], "short form returns the byte");
        __CPROVER_assert(reader.pos == 1, "short form consumes one byte");
    }
    if (reader.err == 0 && input[0] == 0x81) {
        __CPROVER_assert(length == input[1], "0x81 returns the next byte");
        __CPROVER_assert(reader.pos == 2, "0x81 consumes two bytes");
    }
    if (reader.err == 0 && input[0] == 0x82) {
        __CPROVER_assert(length == (uint16_t)((input[1] << 8) | input[2]),
                         "0x82 returns the next two bytes");
        __CPROVER_assert(reader.pos == 3, "0x82 consumes three bytes");
    }
    if (reader.err == 0 && input[0] == 0x83) {
        __CPROVER_assert(reader.pos == 4, "0x83 consumes four bytes");
    }
    if (input[0] >= 0x84) {
        __CPROVER_assert(reader.err != 0, "reserved length form is rejected");
    }

    return 0;
}
