#ifndef __FR_AXDR_H__
#define __FR_AXDR_H__
#if 1
enum data_type_choice_e
{
    TYPE_null = 0,
    TYPE_array = 1,
    TYPE_structure = 2,
    TYPE_bool = 3,
    TYPE_bit_string = 4,
    TYPE_double_long = 5,
    TYPE_double_long_unsigned = 6,

    ////////////////////////////
    DTC_FLOATING_POINT       = 7,
    DTC_OCTET_STRING         = 9,
    DTC_VISIBLE_STRING       = 10,
    DTC_UTF8_STRING          = 12,
    DTC_BCD                  = 13,

    TYPE_integer = 15,
    TYPE_long = 16,
    TYPE_unsigned = 17,
    TYPE_long_unsigned = 18,

    DTC_COMPACT_ARRAY        = 19,//空缺

    TYPE_long64 = 20,
    TYPE_long64_unsigned = 21,

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
#endif
///
enum data_type_222choice_e
{


    TYPE_octet_string = 9, 
    TYPE_visible_string = 10,

    TYPE_UTF8_string = 12,




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

uint8_t axdr_data_len_decode(uint8_t type, uint8_t* buf, uint16_t* len);

#endif //__FR_AXDR_H__