#include <stddef.h>
#include "application/adapters/dl698/fr_axdr_codec.h"

uint8_t axdr_decode(uint8_t, const uint8_t *buf, uint16_t *len)
{
    if (buf[0] < 128)
    {
        if (len != NULL)
        {
            *len = buf[0];
        }
        return 1;
    }
    else if (buf[0] == 0x81)
    {
        if (len != NULL)
        {
            *len = buf[1];
        }
        return 2;
    }
    else if (buf[0] == 0x82)
    {
        if (len != NULL)
        {
            *len = ((uint16_t)buf[1] << 8) + buf[2];
        }
        return 3;
    }
    else
    {
        if (len != NULL)
        {
            *len = 0;
        }
        return 0;
    }
}

uint8_t axdr_len_encode(uint8_t* buf, uint16_t len)
{
    if (len < 128)
    {
        if (buf != NULL)
        {
            buf[0] = (uint8_t)len;
        }

        return 1;
    }
    else if (len < 256)
    {
        if (buf != NULL)
        {
            buf[0] = 0x81;
            buf[1] = (uint8_t)len;
        }

        return 2;
    }
    else
    {
        if (buf != NULL)
        {
            buf[0] = 0x82;
            buf[1] = (uint8_t)(len >> 8);
            buf[2] = (uint8_t)len;
        }

        return 3;
    }
}

/* Decode the A-XDR length form at buf, bounded by remaining. Returns the
 * number of form bytes, or 0 for an invalid or truncated form. */
static uint8_t axdr_len_form_decode(const uint8_t *buf, uint16_t remaining,
                                    uint16_t *len)
{
    if (remaining < 1)
    {
        return 0;
    }
    if (buf[0] < 128)
    {
        *len = buf[0];
        return 1;
    }
    if (buf[0] == 0x81)
    {
        if (remaining < 2)
        {
            return 0;
        }
        *len = buf[1];
        return 2;
    }
    if (buf[0] == 0x82)
    {
        if (remaining < 3)
        {
            return 0;
        }
        *len = ((uint16_t)buf[1] << 8) + buf[2];
        return 3;
    }
    *len = 0;
    return 0;
}

/* Total byte length of one A-XDR value (tag included), or 0 when the value
 * does not fit in remaining bytes. Never reads past data + remaining. */
uint16_t axdr_get_len(const uint8_t *data, uint16_t remaining)
{
    uint16_t total_len = 0;
    uint16_t data_len = 0;
    uint16_t form_len = 0;
    uint8_t num = 0;
    uint8_t type = 0;
    uint8_t tmp = 0;

    if (data == NULL || remaining == 0)
    {
        return 0;
    }
    type = data[0];

    switch (type)
    {
    case TYPE_null:
        return 1;

    case TYPE_array:
    case TYPE_structure:
        if (remaining < 2)
        {
            return 0;
        }
        num = data[1];
        total_len = 2;
        while (num-- > 0)
        {
            if (total_len >= remaining)
            {
                return 0;
            }
            data_len = axdr_get_len(data + total_len,
                                    (uint16_t)(remaining - total_len));
            if (data_len == 0)
            {
                return 0;
            }
            total_len = (uint16_t)(total_len + data_len);
        }
        return total_len;

    case TYPE_bool:
    case TYPE_enum:
    case TYPE_integer:
    case TYPE_unsigned:
        return remaining >= 2 ? 2 : 0;

    case TYPE_long:
    case TYPE_long_unsigned:
    case TYPE_Scaler_Unit:
    case TYPE_OI:
        return remaining >= 3 ? 3 : 0;

    case TYPE_double_long:
    case TYPE_double_long_unsigned:
    case TYPE_OAD:
    case TYPE_OMD:
        return remaining >= 5 ? 5 : 0;

    case TYPE_long64:
    case TYPE_long64_unsigned:
        return remaining >= 9 ? 9 : 0;

    case TYPE_bit_string:
    case TYPE_octet_string:
    case TYPE_visible_string:
    case TYPE_UTF8_string:
    case TYPE_TSA:
    case TYPE_MAC:
    case TYPE_RN:
        form_len = axdr_len_form_decode(data + 1,
                                        (uint16_t)(remaining - 1), &data_len);
        if (form_len == 0)
        {
            return 0;
        }
        total_len = (uint16_t)(1 + form_len);
        if (TYPE_bit_string == type)
        {
            data_len = (uint16_t)((data_len + 7) / 8);
        }
        if (data_len > remaining - total_len)
        {
            return 0;
        }
        return (uint16_t)(total_len + data_len);

    case TYPE_date_time:
        return remaining >= 11 ? 11 : 0;

    case TYPE_date:
    case TYPE_COMDCB:
        return remaining >= 6 ? 6 : 0;

    case TYPE_time:
    case TYPE_TI:
        return remaining >= 4 ? 4 : 0;

    case TYPE_date_time_s:
        return remaining >= 8 ? 8 : 0;

    case TYPE_Region:
        if (remaining < 2)
        {
            return 0;
        }
        data_len = axdr_get_len(data + 1, (uint16_t)(remaining - 1));
        if (data_len == 0)
        {
            return 0;
        }
        total_len = (uint16_t)(1 + data_len);
        if (total_len >= remaining)
        {
            return 0;
        }
        data_len = axdr_get_len(data + total_len,
                                (uint16_t)(remaining - total_len));
        if (data_len == 0)
        {
            return 0;
        }
        return (uint16_t)(total_len + data_len);

    case TYPE_RSD:
    {
        const uint8_t *rsd = data + 1;

        total_len = 1;
        if (remaining < 2)
        {
            return 0;
        }
        type = rsd[0];
        if (0 == type)
        {
            total_len = 1;
        }
        else if (1 == type)
        {
            total_len = (uint16_t)(total_len + 4);
            if (remaining < (uint16_t)(total_len + 2))
            {
                return 0;
            }
            data_len = axdr_get_len(&rsd[total_len],
                                    (uint16_t)(remaining - total_len - 1));
            if (data_len == 0)
            {
                return 0;
            }
            total_len = (uint16_t)(total_len + data_len);
        }
        else if (2 == type)
        {
            total_len = (uint16_t)(total_len + 4);
            for (uint8_t i = 0; i < 3; i++)
            {
                if (remaining < (uint16_t)(total_len + 2))
                {
                    return 0;
                }
                data_len = axdr_get_len(&rsd[total_len],
                                        (uint16_t)(remaining - total_len - 1));
                if (data_len == 0)
                {
                    return 0;
                }
                total_len = (uint16_t)(total_len + data_len);
            }
        }
        else if (9 == type)
        {
            total_len = (uint16_t)(total_len + 1);
        }
        else
        {
            return 0;
        }
        return total_len;
    }

    case TYPE_CSD:
        if (remaining < 6)
        {
            return 0;
        }
        total_len = 6;
        if (1 == data[1])
        {
            if (remaining < 7)
            {
                return 0;
            }
            total_len = (uint16_t)(7 + data[6] * 4);
            if (total_len > remaining)
            {
                return 0;
            }
        }
        return total_len;

    case TYPE_SID:
        if (remaining < 6)
        {
            return 0;
        }
        form_len = axdr_len_form_decode(data + 5,
                                        (uint16_t)(remaining - 5), &data_len);
        if (form_len == 0)
        {
            return 0;
        }
        total_len = (uint16_t)(5 + form_len);
        if (data_len > remaining - total_len)
        {
            return 0;
        }
        return (uint16_t)(total_len + data_len);

    case TYPE_SID_MAC:
        if (remaining < 6)
        {
            return 0;
        }
        form_len = axdr_len_form_decode(data + 5,
                                        (uint16_t)(remaining - 5), &data_len);
        if (form_len == 0)
        {
            return 0;
        }
        total_len = (uint16_t)(5 + form_len);
        if (data_len > remaining - total_len)
        {
            return 0;
        }
        total_len = (uint16_t)(total_len + data_len);
        if (total_len >= remaining)
        {
            return 0;
        }
        form_len = axdr_len_form_decode(data + total_len,
                                        (uint16_t)(remaining - total_len),
                                        &data_len);
        if (form_len == 0 || data_len > remaining - total_len - form_len)
        {
            return 0;
        }
        return (uint16_t)(total_len + form_len + data_len);

    case TYPE_ROAD:
        if (remaining < 5)
        {
            return 0;
        }
        num = data[4];
        if ((uint16_t)(5 + num * 4) > remaining)
        {
            return 0;
        }
        return (uint16_t)(4 + num * 4);

    case TYPE_RCSD:
    {
        uint16_t len = 1;
        uint8_t count;

        if (remaining < 2)
        {
            return 0;
        }
        count = data[1];
        while (count-- > 0)
        {
            if (remaining < (uint16_t)(len + 2))
            {
                return 0;
            }
            type = data[1 + len];
            len = (uint16_t)(len + 5);
            if (1 == type)
            {
                if (remaining < (uint16_t)(len + 2))
                {
                    return 0;
                }
                tmp = data[1 + len];
                len = (uint16_t)(len + tmp * 4);
            }
            if (len > remaining)
            {
                return 0;
            }
        }
        return len;
    }

    default:
        return 0;
    }
}
