#ifndef __FR_AXDR_H__
#define __FR_AXDR_H__



typedef enum 
{
    TYPE_null                   = 0,
    TYPE_array                  = 1,
    TYPE_structure              = 2,
    TYPE_bool                   = 3,
    TYPE_bit_string             = 4,
    TYPE_double_long            = 5,
    TYPE_double_long_unsigned   = 6,
    //7--8保留
    TYPE_octet_string           = 9,
    TYPE_visible_string         = 10,
    //11保留
    TYPE_UTF8_string            = 12,
    //13--14保留
    TYPE_integer                = 15,
    TYPE_long                   = 16,
    TYPE_unsigned               = 17,
    TYPE_long_unsigned          = 18,
    //19保留
    TYPE_long64                 = 20,
    TYPE_long64_unsigned        = 21,
    TYPE_enum                   = 22,
    TYPE_float32                = 23,
    TYPE_float64                = 24,
    TYPE_date_time              = 25,
    TYPE_date                   = 26,
    TYPE_time                   = 27,
    TYPE_date_time_s            = 28,
    //29--79保留

    TYPE_OI             = 80,
    TYPE_OAD            = 81,
    TYPE_ROAD           = 82,
    TYPE_OMD            = 83,
    TYPE_TI             = 84,
    TYPE_TSA            = 85,
    TYPE_MAC            = 86,
    TYPE_RN             = 87,
    TYPE_Region         = 88,
    TYPE_Scaler_Unit    = 89,
    TYPE_RSD            = 90,
    TYPE_CSD            = 91,
    TYPE_MS             = 92,
    TYPE_SID            = 93,
    TYPE_SID_MAC        = 94,
    TYPE_COMDCB         = 95,
    TYPE_RCSD           = 96,
    TYPE_VQDS           = 97,
    //98--255保留

    //以下类型协议中无定义，具体作用?
	TYPE_USER_HIGH_ENERGY_INTEGER  = 241,/* 用于高精度电能记录 */
	TYPE_USER_HIGH_ENERGY_UNSIGNED = 242,/* 用于高精度电能记录 */
	TYPE_USER_NOT_CARE			  = 243,
	TYPE_USER_SPECIAL_PROCESS	  = 244,
	TYPE_USER_PUSH_PROCESS         = 245,  /*事件上报标志 特殊标志*/
    TYPE_USER_ERROR                = 255,
}tlv_type_e;

uint8_t axdr_decode(uint8_t type, const uint8_t *buf, uint16_t *len);
uint8_t axdr_data_len_decode(uint8_t type, uint8_t* buf, uint16_t* len);

uint8_t axdr_len_encode(uint8_t* buf, uint16_t len);
uint16_t axdr_get_len(const uint8_t *data);

uint16_t get_octet_string(struct context_698_t* ctx, uint16_t *offset);





struct rsd_info_t
{
    uint8_t select;
    uint8_t mode;                             // 0- 全部访问   1-按序号搜索    2-按发生时间搜索
    uint8_t last_no;                          //方法9上n次
    uint32_t start_mark;                      //搜索时的起始标志
    uint32_t end_mark;                        //搜索时的结束标志
    uint32_t interval;                        //搜索间隔
    uint32_t interval_special;                //搜索间隔 特殊间隔 月
};



extern int rsd_decode(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad, struct rsd_info_t* rsd_info);
extern int rcsd_decode(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad,  uint32_t *rcsd_list);
extern int record_data_package(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info,  void *ctx);
#endif //__FR_AXDR_H__