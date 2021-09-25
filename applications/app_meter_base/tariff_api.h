/**
 * @defgroup    
 * @ingroup   
 * @ingroup     
 * @brief       
 *
 * @{
 * @file
 *
 */
#ifndef TARIFF_API_H
#define TARIFF_API_H

#define RUN_CUN    0x5a
#define RUN_VICE   0xa5

#define real_check(x, y)   \
    { if (x) { return y; } \
    }

#define     TARIFF_INDEX_DEFAULT    1

#define     TIME_FRAME_TABLE_SWITCH_WAITING     0x01
#define     TIME_ZONE_TABLE_SWITCH_WAITING      0x02

struct tariff_state_t
{
    Word16 next_t_start_time;
    Word16 current_t_start_time;
    uint8  current_tariff;
    uint8  last_tariff;
    uint8  current_day_tab_id;
    uint8  change_out;
    uint8  table_seq;
    uint8  switch_flag;
};


struct holiday_store_t
{
    uint8  year;       /**<  1byte */
    uint8  month;      /**<  1byte */
    uint8  day;        /**<  1byte */
    uint8  week;       /**<  1byte */
    uint8  day_tab_id; /**<  1byte */
};

struct week_store_t
{
    uint8 week_mode;
    uint8 day_tab_id;
};



struct acctive_tariff_t
{
    uint16 td;
    uint16 back_td;
    uint16 len;
};


struct season_node
{
    uint8 month;
    uint8 day;
    uint8 day_tab_id;
};

struct tariff_node_t
{
    uint8 hour;
    uint8 minute;
    uint8 tariff_index;
};

enum check_out_type_t
{
    TYPE_TABLE_ID,
    TYPE_TABLE_SEQ,
    TYPE_TABLE_TARIFF,
};

struct tariff_para_t
{
    uint8  season_max_num;             
    uint8  daytab_max_num;             
    uint8  daytou_max_num;             
    uint8  tariff_max_num;             
    uint8  holiday_max_num;            
    uint16 season_num_fsid;            
    uint16 daytab_num_fsid;            
    uint16 daytou_num_fsid;            
    uint16 tariff_num_fsid;            
    uint16 holiday_num_fsid;           
    uint16 cur_season_data_fsid;       
    uint16 vice_season_data_fsid;      
    uint16 cur_daytab_data_fsid;       
    uint16 vice_daytab_data_fsid;      
    uint16 holiday_data_fsid;          
    uint16 weekday_config_fsid;        
    uint16 weedday_tab_fsid;           
};

struct vice_tariff_tim_t
{
    uint16 start_tim_fsid; 
    uint16 cur_data_fsid;
    uint16 vice_data_fsid;
    uint8  table_num;      
};

#endif
/**@}*/