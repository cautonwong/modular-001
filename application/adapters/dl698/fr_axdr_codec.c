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

uint16_t axdr_get_len(const uint8_t *data)
{
    uint16_t type_len = 0;
    uint16_t data_len = 0,total_len=0;
    uint16_t len = 0;
    uint8_t num = 0;
    uint8_t type = data[0];
    uint8_t tmp = 0;
        
    switch (type)
    {
    case TYPE_null:
        return 1;
        break;
    case TYPE_array: 
    case TYPE_structure:
        data_len++;
        num = data[data_len];
        data_len++;
        data = data + data_len;
        data_len = 0;
        total_len = 0;
        while (num--)
        {
            data_len = axdr_get_len(data);
            total_len = total_len + data_len;
            data = data + data_len;
        }
        return (total_len + 2);
        break;
    case TYPE_bool:
    case TYPE_enum:
    case TYPE_integer:
    case TYPE_unsigned:
        return 1+1;
        break;
    case TYPE_long:
    case TYPE_long_unsigned:
    case TYPE_Scaler_Unit:
    case TYPE_OI:
        return 2+1;
        break;
    case TYPE_double_long:
    case TYPE_double_long_unsigned:
    case TYPE_OAD:
    case TYPE_OMD:
        return 4+1;
        break;
    case TYPE_long64:
    case TYPE_long64_unsigned:
        return 8+1;
        break;
    case TYPE_bit_string:
    case TYPE_octet_string:
    case TYPE_visible_string:
    case TYPE_UTF8_string:
    case TYPE_TSA:
    case TYPE_MAC:
    case TYPE_RN:        
        type_len = 1;
        type_len += axdr_decode(type, data+type_len, &data_len);
        if(TYPE_bit_string == type)
            type_len += (data_len + 7) / 8;
        else
            type_len += data_len;
        return (type_len);
        break;

    case TYPE_date_time:
        return 10+1;
        break;
    case TYPE_date:
    case TYPE_COMDCB:
        return 6;
        break;
    case TYPE_time:
    case TYPE_TI:
        return 3+1;
        break;
    case TYPE_date_time_s:
        return 7+1;
        break;

    case TYPE_Region:
        data++;
        len = 1;
        data_len = axdr_get_len(data);
        data += data_len;
        len += data_len;
        data_len = axdr_get_len(data);
        return (len + data_len);
        break;

    case TYPE_RSD:
        data++;
        type = data[0];
        len = 1;
        if (0 == type)
        {
           len = 1;
        }
        else if (1 == type)
        {
            len += 4;
            if ((TYPE_date_time_s != data[len]) && (TYPE_double_long_unsigned != data[len]  ))
            {
                //return 0;
            }

            data_len = axdr_get_len(&data[len]);
            len += data_len;
        }
        else if (2 == type)
        {
            len += 4;
            if ((TYPE_date_time_s != data[len]) && (TYPE_double_long_unsigned != data[len]))
            {
                //return 0;
            }

            //type_len = axdr_get_len(data[len], data+len, &data_len);
            data_len = axdr_get_len(&data[len]);
            len += data_len;

            if ((TYPE_date_time_s != data[len]) && (TYPE_double_long_unsigned != data[len] ))
            {
                //return 0;
            }

            data_len = axdr_get_len(&data[len]);
            len += data_len;
            
            if ((TYPE_TI != data[len]) && (TYPE_double_long_unsigned != data[len]) && (TYPE_null != data[len]))
            {
                //return 0;
            }
            
            data_len = axdr_get_len(&data[len]);
            len += data_len;
        }
        else if ( 9 == type)
        {
            len += 1;
        }
        else
        {
            return 0;
        }
        return len;
        break;

    case TYPE_CSD:
        data++;
        len = 1;
        len += (1+4);
        if (1 == data[0]){
            num  = data[5];
            len += 1;
            len += num * 4;
        }

        return len;
        break;

    case TYPE_SID:
        len = 4+1;
        type_len = axdr_decode(data[len], data+len, &data_len);
        return (len + type_len + data_len);
        break;

    case TYPE_SID_MAC:
        len = 1 + 4;
        type_len = axdr_decode(data[len], data+len, &data_len);
        len += type_len + data_len;
        type_len = axdr_decode(data[len], data+len, &data_len);
        return (len + type_len + data_len);
        break;

    case TYPE_ROAD:
        len = 4;
        num  = data[len];
        len += num * 4;
        return (len);
        break;

    case TYPE_RCSD:
        data++;
        len = 1;
        num = data[0];
        while (num--)
        {
            type = data[len];
            len += (1+4);
            if (1 == type){
                tmp  = data[len];
                len += tmp * 4;
            }
        }
        return len;
        break;
        
    default:
        break;
    }

    return 0;
}
