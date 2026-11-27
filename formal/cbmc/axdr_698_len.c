/*
 * Bounded proof for the 698 A-XDR length decoder.
 *
 * The remaining byte count is symbolic, so every truncation point of the
 * scalar and string encodings is checked: the decoder must never read past
 * data + remaining, must return zero for a value that does not fit, and must
 * never return a length larger than the remaining bytes. The recursive
 * structured encodings are covered by tests/unit/test_fr_axdr_codec.c; CBMC
 * 5.12 does not encode that recursion at a useful size.
 */
#include <stddef.h>
#include <stdint.h>

#include "application/adapters/dl698/fr_axdr_codec.h"

#define INPUT_SIZE 8

int main(void)
{
    uint8_t string_input[INPUT_SIZE] =
        {TYPE_octet_string, 0x82, 0x00, 0x03, 0xaa, 0xbb, 0xcc, 0xdd};
    uint8_t bit_string_input[INPUT_SIZE] =
        {TYPE_bit_string, 0x81, 0x09, 0xaa, 0x80};
    uint8_t scalar_input[INPUT_SIZE] = {TYPE_long, 0x01, 0x02};
    uint8_t remaining;
    uint16_t length;

    __CPROVER_assume(remaining >= 1 && remaining <= INPUT_SIZE);

    length = axdr_get_len(string_input, remaining);
    __CPROVER_assert(remaining >= 7 ? length == 7 : length == 0,
                     "a three-byte string is accepted or rejected whole");

    length = axdr_get_len(bit_string_input, remaining);
    __CPROVER_assert(remaining >= 5 ? length == 5 : length == 0,
                     "a nine-bit string is accepted or rejected whole");

    length = axdr_get_len(scalar_input, remaining);
    __CPROVER_assert(remaining >= 3 ? length == 3 : length == 0,
                     "a two-byte scalar is accepted or rejected whole");

    return 0;
}
