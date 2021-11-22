#include "nidavellir.h"
#include "fr_698.h"


uint8_t axdr_decode(uint8_t type, const uint8_t *buf, uint16_t *len)
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

uint8_t axdr_encode(uint8_t type, const uint8_t* buf, uint16_t* len)
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


uint16_t axdr_get_len(const uint8_t *data)
{
    uint16_t type_len = 0;
    uint16_t data_len = 0;
    uint16_t len = 0;
    uint8_t num = 0;
    uint8_t type = data[0];

    switch (type)
    {
    case TYPE_null:
        return 0;
        break;
    case TYPE_array: 
    case TYPE_structure:
        num = *data++;
        while (num--)
        {
            data_len += axdr_get_len(data);
            data = data + data_len;
        }
        return data_len;
        break;
    case TYPE_bool:
    case TYPE_enum:
    case TYPE_integer:
    case TYPE_unsigned:
        return 1;
        break;
    case TYPE_long:
    case TYPE_long_unsigned:
    case TYPE_Scaler_Unit:
    case TYPE_OI:
        return 2;
        break;
    case TYPE_double_long:
    case TYPE_double_long_unsigned:
    case TYPE_OAD:
    case TYPE_OMD:
        return 4;
        break;
    case TYPE_long64:
    case TYPE_long64_unsigned:
        return 8;
        break;
    case TYPE_bit_string:
    case TYPE_octet_string:
    case TYPE_visible_string:
    case TYPE_UTF8_string:
    case TYPE_TSA:     /**< 2..17 */
    case TYPE_MAC:
    case TYPE_RN:
        type_len = axdr_decode(type, data, &data_len);
        if(TYPE_bit_string == type)
            type_len += (data_len + 7) / 8;
        else
            type_len += data_len;
        return (type_len);
        break;

    case TYPE_date_time:
        return 10;
        break;
    case TYPE_date:
    case TYPE_COMDCB:
        return 5;
        break;
    case TYPE_time:
    case TYPE_TI:
        return 3;
        break;
    case TYPE_date_time_s:
        return 7;
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
#if 0
    case TYPE_RSD:
        i = *p++;

        if (i == 0)
        {
            break;
        }
        else if (i == 1)
        {
            p += 4;
            if (*p != TYPE_date_time_s && *p != TYPE_double_long_unsigned)
            {
            //    return 0;   /*!!!*/
            }
            p += axdr_get_data_len(p);
        }
        else if (i == 2)
        {
            p += 4;
            if (*p != TYPE_date_time_s && *p != TYPE_double_long_unsigned)
            {
            //    return 0;   /*!!!*/
            }
            p += axdr_get_data_len(p);
            if (*p != TYPE_date_time_s && *p != TYPE_double_long_unsigned)
            {
            //    return 0;   /*!!!*/
            }
            p += axdr_get_data_len(p);
            if (*p != TYPE_TI && *p != TYPE_double_long_unsigned && *p != TYPE_null)
            {
            //    return 0;   /*!!!*/
            }
            p += axdr_get_data_len(p);
        }
        else if (i == 9)
        {
            p += 1;
        }
        else
        {
            return 0;
        }

        break;
#endif 
    case TYPE_CSD:
        len = 1+4;
        if (1 == data[0]){
            num  = data[len];
            len += num * 4;
        }
        return len;
        break;

    case TYPE_SID:
        len = 4;
        if(TYPE_octet_string == data[len])
            type_len = axdr_decode(data[len], data+len, &data_len);
        return (len + type_len + data_len);
        break;
#if 0
    case TYPE_SID_MAC:
        len = 4;
        p  += 4;
        p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += len;
        p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += len;
        break;
#endif
    case TYPE_ROAD:
        len = 4;
        num  = data[len];
        len += num * 4;
        return (len);
        break;


#if 0
    case TYPE_RCSD:
        n = *p++;

        while (n--)
        {
            i = *p++;

            if (i == 0)
            {
                p += 4;
            }
            else
            {
                p += 4;
                i  = *p++;
                p += (uint16_t)(i) * 4;
            }
        }

        break;


    case DTC_HIGH_ENERGY_INTEGER:
    case DTC_HIGH_ENERGY_UNSIGNED:
        p += 5;
        break;
#endif
    default:
        break;
    }

    return 0;
}


