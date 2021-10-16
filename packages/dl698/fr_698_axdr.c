///

enum data_type_choice_e
{
    DTC_NULL                 = 0,
    DTC_ARRAY                = 1,
    DTC_STRUCTURE            = 2,
    DTC_BOOLEAN              = 3,
    DTC_BIT_STRING           = 4,
    DTC_DOUBLE_LONG          = 5,   /**< int32 */
    DTC_DOUBLE_LONG_UNSIGNED = 6,   /**< uint32 */
    DTC_FLOATING_POINT       = 7,
    DTC_OCTET_STRING         = 9,
    DTC_VISIBLE_STRING       = 10,
    DTC_UTF8_STRING          = 12,
    DTC_BCD                  = 13,
    DTC_INTEGER              = 15,   /**< int8 */
    DTC_LONG                 = 16,   /**< int16 */
    DTC_INTEGER_UNSIGNED     = 17,   /**< uint8 */
    DTC_LONG_UNSIGNED        = 18,   /**< uint16 */
    DTC_COMPACT_ARRAY        = 19,//空缺
    DTC_LONG64               = 20,   /**< int64 */
    DTC_LONG64_UNSIGNED      = 21,   /**< uint64 */
    DTC_ENUM                 = 22,
    DTC_FLOAT32              = 23,
    DTC_FLOAT64              = 24,
    DTC_DATE_TIME            = 25,
    DTC_DATE                 = 26,
    DTC_TIME                 = 27,
    DTC_DATETIME_S           = 28,

    DTC_OI                   = 80,
    DTC_OAD                  = 81,
    DTC_ROAD                 = 82,
    DTC_OMD                  = 83,
    DTC_TI                   = 84,
    DTC_TSA                  = 85,
    DTC_MAC                  = 86,
    DTC_RN                   = 87,
    DTC_REGION               = 88,
    DTC_SCALER_UNIT          = 89,
    DTC_RSD                  = 90,
    DTC_CSD                  = 91,
    DTC_MS                   = 92,
    DTC_SID                  = 93,
    DTC_SID_MAC              = 94,
    DTC_COMDCB               = 95,
    DTC_RCSD                 = 96,
    DTC_VQDS                 = 97,

    //以下类型协议中无定义，具体作用?
	DTC_USER_HIGH_ENERGY_INTEGER  = 241,/* 用于高精度电能记录 */
	DTC_USER_HIGH_ENERGY_UNSIGNED = 242,/* 用于高精度电能记录 */
	DTC_USER_NOT_CARE			  = 243,
	DTC_USER_SPECIAL_PROCESS	  = 244,
	DTC_USER_PUSH_PROCESS         = 245,  /*事件上报标志 特殊标志*/
    DTC_USER_ERROR                = 255,
};
///
enum data_type_choice_e
{
    TYPE_NULL = 0,  
    TYPE_array = 1,
    TYPE_structure = 2,
    TYPE_bool = 3,
    TYPE_bit_string = 4,  
    TYPE_double_long = 5,  
    TYPE_double_long_unsigned = 6, 

    TYPE_octet_string = 9, 
    TYPE_visible_string = 10,

    TYPE_UTF8_string = 12,

    TYPE_integer = 15,
    TYPE_long = 16,
    TYPE_unsigned = 17,
    TYPE_long_unsigned = 18,

    TYPE_long64 = 20,
    TYPE_long64_unsigned = 21,
    TYPE_enum = 22,
    TYPE_float32 = 23,
    TYPE_float64 =24,
    TYPE_date_time = 25,
    TYPE_date = 26,
    TYPE_time = 27,
    TYPE_date_time_s = 28,

    TYPE_OI = 80,
    TYPE_OAD = 81,
    TYPE_ROAD = 82,
    TYPE_OMD = 83,
    TYPE_TI = 84,
    TYPE_TSA = 85,
    TYPE_MAC = 86,
    TYPE_RN = 87,
    TYPE_Region = 88,
    TYPE_Scaler_Unit = 89,
    TYPE_RSD = 90,
    TYPE_CSD = 91,
    TYPE_MS = 92,
    TYPE_SID = 93, 
    TYPE_SID_MAC = 94,
    TYPE_COMDCB = 95,
    TYPE_RCSD = 96,
    TYPE_VQDS = 97, 
};



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
        p  += axdr_data_len_decode((uint8_t *)p, &len);
        p  += (len + 7) / 8;
        break;

    case DTC_OCTET_STRING:
    case DTC_VISIBLE_STRING:
    case DTC_UTF8_STRING:
    case DTC_TSA:     /**< 2..17 */
    case DTC_MAC:
    case DTC_RN:
        len = 0;
        p  += axdr_data_len_decode((uint8_t *)p, &len);
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

    return (uint16_t)(p - data);
}