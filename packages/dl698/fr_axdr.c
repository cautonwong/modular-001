#include "nidavellir.h"
#include "fr_698.h"


uint8_t axdr_data_len_decode(uint8_t type, uint8_t *buf, uint16_t *len)
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

uint16_t axdr_get_data_len(const uint8_t *data)
{

    const uint8_t *p = data;
#if 0
    uint8_t       i, n;
    uint16_t      len;

    switch (*p++)
    {
    case TYPE_null:
        break;

    case TYPE_bool:
    case TYPE_enum:
    case TYPE_integer:
    case TYPE_unsigned:
        p += 1;
        break;

    case TYPE_long:
    case TYPE_long_unsigned:
    case TYPE_Scaler_Unit:
        p += 2;
        break;

    case TYPE_double_long:
    case TYPE_double_long_unsigned:
        p += 4;
        break;

    case TYPE_long64:
    case TYPE_long64_unsigned:
        p += 8;
        break;

    case TYPE_bit_string:
        len = 0;
        //p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += (len + 7) / 8;
        break;

    case TYPE_octet_string:
    case TYPE_visible_string:
    case TYPE_UTF8_string:
    case TYPE_TSA:     /**< 2..17 */
    case TYPE_MAC:
    case TYPE_RN:
        len = 0;
        //p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += len;
        break;

    case TYPE_date_time:
        p += 10;
        break;

    case TYPE_date:
        p += 5;
        break;

    case TYPE_time:
        p += 3;
        break;

    case TYPE_date_time_s:
        p += 7;
        break;

    case TYPE_array:
        i = *p++;

        while (i--)
        {
            p += axdr_get_data_len(p);
        }

        break;

    case TYPE_structure:
        i = *p++;

        while (i--)
        {
            p += axdr_get_data_len(p);
        }

        break;

    case TYPE_OI:
        p += 2;
        break;

    case TYPE_OAD:
    case TYPE_OMD:
        p += 4;
        break;

    case TYPE_TI:
        p += 3;
        break;

    case TYPE_Region:
        p++;
        p += axdr_get_data_len(p);
        p += axdr_get_data_len(p);
        break;

    case TYPE_SID:
        len = 0;
        p  += 4;
        p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += len;
        break;

    case TYPE_SID_MAC:
        len = 0;
        p  += 4;
        p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += len;
        p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += len;
        break;

    case TYPE_COMDCB:
        p += 5;
        break;


    case TYPE_ROAD:
        p += 4;
        i  = *p++;
        p += (uint16_t)i * 4;
        break;

    case TYPE_CSD:
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

        break;

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
    case DTC_HIGH_ENERGY_INTEGER:
    case DTC_HIGH_ENERGY_UNSIGNED:
        p += 5;
        break;
    default:
        break;
    }
#endif
    return (uint16_t)(p - data);
}


uint8_t axdr_encode(uint8_t type, uint8_t* buf, uint16_t* len)
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