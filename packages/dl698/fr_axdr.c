#include "kernel\nidavellir.h"
#include "fr_axdr.h"
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
    uint8_t       i, n;
    uint16_t      len;
#if 0
    switch (*p++)
    {
    case DTC_NULL:
        break;

    case DTC_BOOLEAN:
    case DTC_ENUM:
    case DTC_INTEGER:
    case DTC_INTEGER_UNSIGNED:
        p += 1;
        break;

    case DTC_LONG:
    case DTC_LONG_UNSIGNED:
    case DTC_SCALER_UNIT:
        p += 2;
        break;

    case DTC_DOUBLE_LONG:
    case DTC_DOUBLE_LONG_UNSIGNED:
        p += 4;
        break;

    case DTC_LONG64:
    case DTC_LONG64_UNSIGNED:
        p += 8;
        break;

    case DTC_BIT_STRING:
        len = 0;
        //p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += (len + 7) / 8;
        break;

    case DTC_OCTET_STRING:
    case DTC_VISIBLE_STRING:
    case DTC_UTF8_STRING:
    case DTC_TSA:     /**< 2..17 */
    case DTC_MAC:
    case DTC_RN:
        len = 0;
        //p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += len;
        break;

    case DTC_DATE_TIME:
        p += 10;
        break;

    case DTC_DATE:
        p += 5;
        break;

    case DTC_TIME:
        p += 3;
        break;

    case DTC_DATETIME_S:
        p += 7;
        break;

    case DTC_ARRAY:
        i = *p++;

        while (i--)
        {
            p += axdr_get_data_len(p);
        }

        break;

    case DTC_STRUCTURE:
        i = *p++;

        while (i--)
        {
            p += axdr_get_data_len(p);
        }

        break;

    case DTC_OI:
        p += 2;
        break;

    case DTC_OAD:
    case DTC_OMD:
        p += 4;
        break;

    case DTC_TI:
        p += 3;
        break;

    case DTC_REGION:
        p++;
        p += axdr_get_data_len(p);
        p += axdr_get_data_len(p);
        break;

    case DTC_SID:
        len = 0;
        p  += 4;
        p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += len;
        break;

    case DTC_SID_MAC:
        len = 0;
        p  += 4;
        p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += len;
        p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += len;
        break;

    case DTC_COMDCB:
        p += 5;
        break;


    case DTC_ROAD:
        p += 4;
        i  = *p++;
        p += (uint16_t)i * 4;
        break;

    case DTC_CSD:
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

    case DTC_RCSD:
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

    case DTC_RSD:
        i = *p++;

        if (i == 0)
        {
            break;
        }
        else if (i == 1)
        {
            p += 4;
            if (*p != DTC_DATETIME_S && *p != DTC_DOUBLE_LONG_UNSIGNED)
            {
            //    return 0;   /*!!!*/
            }
            p += axdr_get_data_len(p);
        }
        else if (i == 2)
        {
            p += 4;
            if (*p != DTC_DATETIME_S && *p != DTC_DOUBLE_LONG_UNSIGNED)
            {
            //    return 0;   /*!!!*/
            }
            p += axdr_get_data_len(p);
            if (*p != DTC_DATETIME_S && *p != DTC_DOUBLE_LONG_UNSIGNED)
            {
            //    return 0;   /*!!!*/
            }
            p += axdr_get_data_len(p);
            if (*p != DTC_TI && *p != DTC_DOUBLE_LONG_UNSIGNED && *p != DTC_NULL)
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