/**
 * @defgroup    demand_cfg Demand Cfg
 * @ingroup     cfg
 * @see     demand
 * @brief       demand配置.
 *
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "hfp_handy.h"
#include "demand.h"
#include "lib_calclib.h"
#include "fs.h"

#define D_ATT(x)    ((uint8)(1<<x))

#define QUADRANT_JUDGE_START_TIME        5

#define QUADRANT_JUDGE_CONTINUE_TIME     10

struct quadrant_judge_t calc_quadrant_judge;

struct activepower_direction_judge_t activepower_direction_judge;

extern const struct demand_ops_t CODE demand_ops;
extern       struct demand_info_t     demand_info;

extern void demand_output_hook(void);

enum file_format_index
{   /* 注意 ：最好是 8 个一行， 中间不允许有间断 */
	format_00, format_01, format_02, format_03, format_04, format_05, format_06, format_07, /**< 00 -- 07 */
	format_08, format_09, format_10, format_11, format_12, format_13, format_14, format_15, /**< 08 -- 15 */
	format_16, format_17, format_18, format_19, format_20,
    F_U16_V_N1, F_I32_A_N3, F_U32_W_N1, F_U32_KW_N4, F_U32_KVAR_N4, F_U8_SEC_0, F_I16_PER_N1, F_I16_PER_N2, F_U16_PER_N2,
    F_ENUM, F_BOOL, F_OAD, F_I32_KWH_N2, F_I32_KVARH_N2, F_I32_KVAH_N2, F_I32_W_N1, F_I32_var_N1, F_I32_VA_N1,F_I16_COS_N3,
	F_U16_ANGLE_N1, F_U16_HZ_N2, F_U32_KWH_N2, F_U32_KVARH_N2, F_U32_KVAH_N2, 
	F_I32_KWH_N4, F_I32_KVARH_N4, F_I32_KVAH_N4, F_U32_KWH_N4, F_U32_KVARH_N4, F_U32_KVAH_N4, 
    F_U32_Ah_N2, F_ARRAY_NUDEF, F_OAD_NUDEF, F_DATETIME_S_NUDEF, F_I32_W_N2, F_I32_var_N2, F_I32_VA_N2, F_NULL
};

uint8 demand_quadrant_change_judge(uint8 reset)
{
    uint8 value;

    hfp_read(0x6F3F, (uint8 *)&value, 0x01, __NULL);

    if (reset)
    {
        calc_quadrant_judge.start_tim    = 0;
        return false;
    }
    
    if (calc_quadrant_judge.start_tim < QUADRANT_JUDGE_START_TIME)
    {
        calc_quadrant_judge.start_tim++;
    }
    else if (QUADRANT_JUDGE_START_TIME == calc_quadrant_judge.start_tim)
    {
        calc_quadrant_judge.quadrant     = value;
        calc_quadrant_judge.now_quadrant = value;
        calc_quadrant_judge.continue_tim = 0;
        calc_quadrant_judge.start_tim    = 0xFF;
    }
    else
    {
        if (calc_quadrant_judge.now_quadrant != value)
        {
            calc_quadrant_judge.continue_tim = 0;
            calc_quadrant_judge.now_quadrant = value;
        }
        if (calc_quadrant_judge.continue_tim < QUADRANT_JUDGE_CONTINUE_TIME)
        {
            calc_quadrant_judge.continue_tim++;
        }
        else
        {
            if (calc_quadrant_judge.quadrant != calc_quadrant_judge.now_quadrant)
            {
                calc_quadrant_judge.quadrant = calc_quadrant_judge.now_quadrant;
                return true;
            }
        }
    }

    return false;
}

uint8 active_power_direction_judge(void)
{
    uint8 value;

    hfp_read(0x6F32, (uint8 *)&value, 0x01, __NULL);
    
    if (activepower_direction_judge.start_tim < QUADRANT_JUDGE_START_TIME)
    {
        activepower_direction_judge.start_tim++;
    }
    else if (QUADRANT_JUDGE_START_TIME == activepower_direction_judge.start_tim)
    {
        activepower_direction_judge.direction  = value;
        activepower_direction_judge.now_direction = value;
        activepower_direction_judge.continue_tim = 0;
        activepower_direction_judge.start_tim    = 0xFF;
    }
    else
    {
        if (activepower_direction_judge.now_direction != value)
        {
            activepower_direction_judge.continue_tim = 0;
            activepower_direction_judge.now_direction = value;
        }
        if (activepower_direction_judge.continue_tim < QUADRANT_JUDGE_CONTINUE_TIME)
        {
            activepower_direction_judge.continue_tim++;
        }
        else
        {
            if (activepower_direction_judge.direction != activepower_direction_judge.now_direction)
            {
                activepower_direction_judge.direction = activepower_direction_judge.now_direction;
                return true;
            }
        }
    }

    return false;
}

uint8 positive_or_negative_active_demand_calc_policy(uint16 fd)
{
    uint8   i;
    struct  judge_t
    {
        uint16 fd;
        uint8  value;
    };
    struct  judge_t  fd_table[] = {{0xA010,0},{0xA020,1}};
        
    for(i = 0; i < array_count(fd_table); i++)
    {
        if (fd == fd_table[i].fd)
        {
            if (activepower_direction_judge.direction == fd_table[i].value)
            {
                return true;
            }
            return false;
        }
    }
    
    return false;
}

uint8 quadrant_demand_calc_policy(uint16 fd)
{
    uint8   i;
    uint16  fd_table[] = {0xA050,0xA060,0xA070,0xA080};
#if 0
    hfp_read(0x6F3F, (uint8 *)&value, 0x01, __NULL);
#endif
    
    for (i = 0; i < array_count(fd_table); i++)
    {
        if ((fd_table[i] == fd)&&((i + 1) == calc_quadrant_judge.quadrant))
        {
            return true;
        }
    }
    return false;
}

const struct demand_item_t CODE demand_item[] =
{
    { 0xA010, 0xC130, 0x003C, F_I32_W_N2, D_ATT(SAVE_EEPROM)|D_ATT(ESTABLISH_MEMORY),0,positive_or_negative_active_demand_calc_policy }, /**< 当前正向有功总最大需量 */
    { 0xA020, 0xC131, 0x003C, F_I32_var_N2,  D_ATT(SAVE_EEPROM),                        0,positive_or_negative_active_demand_calc_policy }, /**< 当前反向有功总最大需量*/
    #if 0
    { 0xA030, 0xC13B, 0x003C, F_I32_var_N2, D_ATT(SAVE_EEPROM)|D_ATT(ESTABLISH_MEMORY),1,__NULL }, /**< 当前组合无功1总最大需量*/
    { 0xA040, 0xC13C, 0x003C, F_I32_var_N2, D_ATT(SAVE_EEPROM)|D_ATT(ESTABLISH_MEMORY),2,__NULL }, /**< 当前组合无功2总最大需量*/
    #endif
    { 0xA050, 0xC132, 0x003C, F_I32_var_N2, D_ATT(QUADRANT_REACTIVE_DEMAND) | D_ATT(SAVE_EEPROM)|D_ATT(ESTABLISH_MEMORY),1,quadrant_demand_calc_policy }, /**< 当前总瞬时无功象限1 */
    { 0xA060, 0xC133, 0x003C, F_I32_var_N2, D_ATT(QUADRANT_REACTIVE_DEMAND) | D_ATT(SAVE_EEPROM),                        1,quadrant_demand_calc_policy }, /**< 当前总瞬时无功象限2 */
    { 0xA070, 0xC134, 0x003C, F_I32_var_N2, D_ATT(QUADRANT_REACTIVE_DEMAND) | D_ATT(SAVE_EEPROM),                        1,quadrant_demand_calc_policy }, /**< 当前总瞬时无功象限3 */
    { 0xA080, 0xC135, 0x003C, F_I32_var_N2, D_ATT(QUADRANT_REACTIVE_DEMAND) | D_ATT(SAVE_EEPROM),                        1,quadrant_demand_calc_policy }, /**< 当前总瞬时无功象限4 */
    #if 0                                  
    { 0xA090, 0xC136, 0x003C, F_I32_VA_N2, D_ATT(SAVE_EEPROM)|D_ATT(SAVE_EEPROM),     4,__NULL }, /**< 当前总瞬时正向视在需量 */
    { 0xA0A0, 0xC137, 0x003C, F_I32_VA_N2, D_ATT(SAVE_EEPROM)|D_ATT(SAVE_EEPROM),     4,__NULL }, /**< 当前总瞬时反向视在需量 */
    #endif

    /* 以下仅计算当前需量，不计算最大需量，且过结算日不参与转存 */
    { 0xA0B0, 0xC030, 0x003C, F_I32_W_N2,   D_ATT(0x00)|D_ATT(ESTABLISH_MEMORY),       2,__NULL }, /**< 当前总瞬时有功功率 */
    { 0xA0C0, 0xC031, 0x003C, F_I32_var_N2, D_ATT(QUADRANT_REACTIVE_DEMAND)|D_ATT(ESTABLISH_MEMORY),       3,__NULL }, /**< 当前总瞬时无功功率 */
    #if 0
    { 0xA0D0, 0xC032, 0x003C, D_ATT(0x00)|D_ATT(ESTABLISH_MEMORY),       6,__NULL }, /**< 当前总瞬时视在功率 */
    #endif
};

const struct demand_adapter_t CODE demand_adapter =
{
    demand_item,
    array_count(demand_item),
    &demand_ops,
};

uint8 demand_dlt645type_transform(int32 dat, uint8 *dest)
{
    uint8  negative   = 0;
    uint32 abs_demand = 0;
    
    if( dat < 0 )
    {
        abs_demand = -dat;
        negative = 1;
    }
    else
    {
        abs_demand =  dat;
    }
    
    abs_demand = (uint32)hfp_longhex_to_bcd(abs_demand);
    
    hfp_memcopy(dest, (uint8 *)&abs_demand, 3);
    
    if( negative )
    {
        *(dest + 2) |= 0x80;
    }

    return true;
}

const uint16 CODE relation_data_id[] = {0xA050,0xA060,0xA070,0xA080};
const uint16 CODE relation_time_id[] = {0xB050,0xB060,0xB070,0xB080};

struct combination_t
{
    uint16 fsid;
    uint16 mode_id;
    uint8  init_mode;
    uint8  attr;
    const  uint16 *ptr;
};

const struct combination_t CODE reactive_max_demand[] = 
{
    {0x6700,0x2166,0x41,0x00,relation_data_id},
    {0x6710,0x2167,0x14,0x00,relation_data_id},
    {0x6720,0x2166,0x41,0x01,relation_data_id},
    {0x6730,0x2167,0x14,0x01,relation_data_id},
    {0x6740,0x2166,0x41,0x00,relation_data_id},
    {0x6750,0x2167,0x14,0x00,relation_data_id},

};
uint8 combination_reactive_max_demand_get(uint32 id, uint8 *dest, uint16 len, const struct hfp_device_t CODE *device)
{
    uint8                   i,j,seq;
    uint16                  fd,mode_byte;
    int32                   demand;
    uint32                  value,value1;
    const uint16            *p;
    struct file_stat_t      file;

    for(i = 0; i < array_count(reactive_max_demand); i++)
    {
        hfp_stat(reactive_max_demand[i].fsid, &file);
        if(((uint16)id >= reactive_max_demand[i].fsid) && (uint16)id <= reactive_max_demand[i].fsid + file.same)
        {
            if(hfp_read(reactive_max_demand[i].mode_id, (uint8 *)&mode_byte, 0x02, __NULL))
            {
                mode_byte >>= 8;
            }
            else
            {
                mode_byte = reactive_max_demand[i].init_mode;
            }
            
            p = reactive_max_demand[i].ptr;

            value  = 0;
            value1 = 0;
            for (j = 0; j < 4; j++)
            {
                if (mode_byte & 0x03)
                {
                    fd = *(p + j) + id - reactive_max_demand[i].fsid;
                    if (true != hfp_read(fd, (uint8 *)&value, 0x04, __NULL))
                    {
                        value = 0;
                    }
                    if (value >= value1)
                    {
                        seq    = j;
                        value1 = value; 
                    }
                }
                mode_byte >>= 2;
            }
            demand = value1;

            if (seq > 1)
            {
                if ( value1 > 0)
                {
                    if ((id == 0x6740)||(id == 0x6750))
                    {
                        hfp_swap_buffer((uint8*)value1, 4);
                        value1 = hfp_longhex_to_bcd(value1);
                        demand = value1 | 0x80000000; 
                    }
                    else
                    {
                        demand = 0 - value1;
                    }
                }

            }
            

            if (!reactive_max_demand[i].attr)
            {
                hfp_memcopy(dest, (uint8 *)&demand, 0x04);
            }
            else
            {
                fd = *(reactive_max_demand[i].ptr + seq) + 0x1000 + id - reactive_max_demand[i].fsid;
                if (true != hfp_read(fd, dest, 0x05, __NULL))
                {
                    hfp_memset(dest, 0x00, 0x05);
                }
            }
            return true;
        }
    }
    return false;
}

void demand_output_hook(void)
{
    if (DEMANDOUT == multifunctional_io_out_way)
    {
        hfp_ctrl(0x1050 , CTRL_RTC_MULT_PLUSE , __NULL);
    }
}

void demand_power_on_hook(void)
{
    hfp_buffer_clear((uint8 *)&calc_quadrant_judge, sizeof(struct quadrant_judge_t));
    hfp_buffer_clear((uint8 *)&activepower_direction_judge, sizeof(struct activepower_direction_judge_t));
}
/**@}*/
