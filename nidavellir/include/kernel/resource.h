#ifndef _RESOURCE_H_
#define _RESOURCE_H_

struct hash_index_t
{
    const uint16_t* map;
};

struct explorer_t
{
    uint16_t               d_id;           // 数据的ID号  
    uint32_t               d_addr;         // 数据地址 
    uint16_t               d_len;          // 数据长度 
    enum data_verific_t    d_ver;          // 数据属性  
    enum data_format_t     d_form;         // 数据库的存储格式  
    uint16_t               r_addr;         // ram 地址
    uint16_t               r_len;          // ram 地址
    uint8_t                same;           //同类项个数；
    enum decimals_t        decimals;       //小数点位数；
    enum memory_layout_t   m_type;         //内存分区
    unsigned               data_bak : 1;   // data_back  + process_mode + device_id
    unsigned               proces_mode : 1;
    unsigned               device_id : 6;
};

struct batch_info_t          //这个信息放到instance里面；
{
    uint32_t history_time;   //单位是分钟；
    uint32_t start_time;     //记录的开始时间，单位是分钟
    uint32_t max_addr;       //记录的最大地址；
    uint32_t start_addr;     //记录的开始地址；
    uint16_t record_num;     //记录的总条数；
    uint16_t record_len;     //记录的单条长度；
    uint16_t record_period;  //记录的周期；
    uint8_t time_change_flag;  //修改时间标志，需要实时更新；
    uint16_t info_crc;         //校验码；
};
struct rtc_t
{
    uint8_t second;       // 秒
    uint8_t minute;       // 分
    uint8_t hour;         // 时
    uint8_t day;          // 日
    uint8_t month;        // 月
    uint8_t year;         // 年
    uint8_t week;         // 周
};

struct result_t
{
    uint16_t pass_num;
    uint16_t read_ptr;
    uint16_t start_flag;
};

struct search_info_t
{
    struct rtc_t start_rtc;
    struct rtc_t end_rtc;
    struct result_t result;
};

struct set_info_t
{
    struct rtc_t record_time;
    uint16_t record_len;
    uint16_t record_period;
};

#endif