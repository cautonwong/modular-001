/**
 * @defgroup    instantaneous_cfg Instantaneous Cfg
 * @ingroup     cfg
 * @see         instantaneous
 * @brief       
 *
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "Instantaneous.h"
#include "fs.h"
#include "lib_calclib.h"
#include "snapshot.h"
#define PI                        3.141592654
#define DEGREES_PER_RADIAN        (360.0 / (2.0 * PI))
#ifndef WIN32
#include "RN8318.h"
#endif

#define  MAX_PHASE_NUM            4
#define  MAX_REAL_CLASS_NUM       8
#define  REAL_PF_DEFAULT          1000
#define  MEASURE_PHASE_BIT_MAP    0x0F

#define  ANGLE_II_THRESHOLD       10000 /*电流逆相序流流角最小间距值*/
#define  ANGLE_FULL_SCALE        360000 /*流流角满量角度值*/
#define  ANGLE_OFFSET            5000/*整体调整5度*/


/*over_flag*/
#define FLAG_OVER_60UN                       0x01
#define FLAG_OVER_120UN                      0x02
#define FLAG_POWER_ABERRANT                  0x04
#define FLAG_ALL_VOLTAGE_LOST                0x08
/*******************************************************************************
 * 合相分相公共负载状态定义
 ******************************************************************************/
#define STA_U_ALL_OVER_60UN                  0x0001
#define STA_I_ALL_OVER_5IB                   0x0002
#define STA_ACITVE_POWER_DIR_CHANGE          0x0004
#define STA_U_PHASE_SEQUENCE_REV             0x0008
#define STA_I_PHASE_SEQUENCE_REV             0x0010
#define STA_I_ALL_UNDER_5IB                  0x0020
#define STA_U_ALL_UNDER_68UN                 0x0040
#define STA_U_ALL_UNDER_60UN                 0x0080
#define STA_REACITVE_POWER_DIR_CHANGE        0x0100
#define STE_TEST                             0xFFFF

enum statistics_category_t
{
    STATISTICS_POW_ACT,         /**< 0 有功功率 */
    STATISTICS_POW_REACT,       /**< 1 无功功率 */
    STATISTICS_POW_APPAR,       /**< 2 视在功率 */
    STATISTICS_FREORVOL,        /**< 3 频率或电压 */
    STATISTICS_CURRENT,         /**< 4 电流 */
    STATISTICS_POWFACT,         /**< 5 功率因数 */
    STATISTICS_NUM
};



const struct real_ops_attr_t CODE max_real_ops_attr =
{
    MEASURE_PHASE_BIT_MAP,
    MAX_REAL_CLASS_NUM,
    STATISTICS_NUM,
    0,
};




const struct unit_decmail_t CODE decimal_list[MAX_REAL_CLASS_NUM] =
{
   {unit_W ,   -3 },
   {unit_var , -3 },
   {unit_VA ,  -3 },
    
   {unit_V ,         -2 },
   {unit_A ,         -4 },
   {unit_undefined ,         -3 },
   {unit_undefined ,         -3 },
   {unit_undefined ,         -3 },
};


static float     factor[5];
static uint16     factor_crc;
#ifdef WIN32
const float CODE factor_default[MAX_REAL_CLASS_NUM] =
{
    1,
    1,
    1,
    1,
    1,
};
#else
const float CODE factor_default[5] =
{
    //220 5(60)
    22.007,
    22.007,
    22.007,
    1,
    0.208333,
    //57.7 1.5(6)
/*     0.3029,
    0.3029,
    0.3029,
    1,
    0.027,*/
     //100 1.5(6)    
   /*  0.5327,    
     0.5327,    
     0.5327,    
     1,    
     0.027,*/
};
#endif


static struct phase_real_t instantaneous[MAX_PHASE_NUM];
int32               real[MAX_PHASE_NUM][MAX_REAL_CLASS_NUM];
//static struct phase_avg_t  avg[MAX_PHASE_NUM][STATISTICS_NUM];
//static uint32              maximum[MAX_PHASE_NUM][STATISTICS_NUM];
//static uint32              minimum[MAX_PHASE_NUM][STATISTICS_NUM];
//static uint32              val[MAX_PHASE_NUM][STATISTICS_NUM];
//static uint16              cnt[MAX_PHASE_NUM][STATISTICS_NUM];
#pragma object_attribute=__no_init
static struct current_acc_t current_acc[3];
#pragma object_attribute=__no_init
static uint16 current_acc_crc[3];

static int8 last_power_dir;
static int8 last_reactive_power_dir;
static uint8 no_current = 0;
static uint8 under_05ib = 0;
static uint8 under_68un = 0;
static uint8 under_60un = 0;
static uint8 over_flag = 0;
static uint8 power_statue = 0;
uint32 battery_under4v8 = 0;

struct real_t
{
    uint8 real_id[MAX_REAL_CLASS_NUM];
};

/** EMU 内部ID编码 */

const struct real_t CODE real_list[MAX_PHASE_NUM] = 
{
    { REAlID_P_A, REAlID_Q_A, REAlID_S_A, REAlID_U_A, REAlID_I_A, REAlID_PF_A,  REAlID_UI_ANGEL_A,  REAlID_ANGEL_A,},
    { REAlID_P_B, REAlID_Q_B, REAlID_S_B, REAlID_U_B, REAlID_I_B, REAlID_PF_B,  REAlID_UI_ANGEL_B,  REAlID_ANGEL_B,},
    { REAlID_P_C, REAlID_Q_C, REAlID_S_C, REAlID_U_C, REAlID_I_C, REAlID_PF_C,  REAlID_UI_ANGEL_C,  REAlID_ANGEL_C,},
    { REAlID_P_T, REAlID_Q_T, REAlID_S_T, REAlID_FRE, 0xFF,       REAlID_PF_T,  0xFF,               0xFF,},
};


struct specification_t            specification_parameter;
static uint16                     specification_parameter_crc;

const struct specification_t CODE specification_parament_default =
{
   //220 5(60)
    22000,
    26400,
    50000,
    800,
    13200,
    14960,
    150,
    2500,
    600,

    //57.7 1.5(6)
/*    5770,    
    6924,    
    15000,    
    2400,    
    3462,    
    3923,    
    00,    
    750,    
    60,*/

   //100 1.5(6)
 /*   10000,    
    12000,    
    15000,    
    2400,    
    6000,    
    6800,    
    00,    
    750,    
    60,*/
};

const struct parameter_attr_t CODE instantaneous_parameter[] =
{
    {
        0x2007,
        sizeof(specification_parameter),
        &specification_parameter,
        &specification_parameter_crc,
        hfp_crc16_calculate,
        &specification_parament_default,
        0,
    },
    {
        0x2004,
        sizeof(factor),
        factor,
        &factor_crc,
        hfp_crc16_calculate,
        &factor_default,
        0,
    },
};
const uint8 instantaneous_parameter_num = sizeof(instantaneous_parameter) / sizeof(instantaneous_parameter[0]);

#if 0
#define STA_ACITVE_POWER_DIR_CHANGE          0x04
#define STA_U_PHASE_SEQUENCE_REV             0x08
#define STA_I_PHASE_SEQUENCE_REV             0x10
#define STA_I_ALL_UNDER_0_05IB               0x20
#define STA_U_ALL_UNDER_0_68UN               0x40
#define STA_U_ALL_UNDER_0_60UN               0x80
#endif
/*


if (which_instantaneous->average)
{
    hfp_buffer_clear((uint8 *)which_instantaneous->average, max_real_ops_attr.statistics << 2);
}

if(which_instantaneous->maximum)
{
    hfp_buffer_clear((uint8 *)which_instantaneous->maximum, max_real_ops_attr.statistics << 2);
}

if (which_instantaneous->minimum)
{
    hfp_memset((uint8 *)which_instantaneous->minimum, 0xFF, max_real_ops_attr.statistics << 2);
}

void instantaneous_statistics_calculate(struct phase_real_t *instantaneous, uint8 phase)
{
    uint8  j;
    int32  value;
    uint32 real_value;

    for (j = 0; j < max_real_ops_attr.statistics; j++)
    {
        value      = instantaneous->real[j];
        real_value = hfp_abs(value);

        if (instantaneous->average)
        {
            if (instantaneous->average->cnt[j] < 0xFFFF)
            {
                instantaneous->average->val[j] += real_value;
                instantaneous->average->cnt[j]++;
            }
            else
            {
                instantaneous->average->cnt[j] = 1;
                instantaneous->average->val[j] = real_value;
            }
        }

        if (instantaneous->maximum)
        {
            if (value > instantaneous->maximum[j])
            {
                instantaneous->maximum[j] = real_value;
            }
        }

        if (instantaneous->minimum)
        {
            if (value < instantaneous->minimum[j])
            {
                instantaneous->minimum[j] = real_value;
            }
        }
    }
}
*/            
            

void cal_instantaneous_hook(struct phase_real_t *which_instantaneous, uint8 phase)
{
    
    uint32 user1 = 0;
    uint32 abs_current = 0;
    uint8 mask, i;
    uint8 not_mask;
    uint8 mask_type = 0x07;
    float s;
    
    which_instantaneous->state &= STA_ACITVE_POWER_DIR_CHANGE;
    which_instantaneous->state &= STA_REACITVE_POWER_DIR_CHANGE;
    over_flag = 0;
    if (specification_parameter.un == 10000)    
    {
        mask_type = 0x05;
    }
#ifdef WIN32

	specification_parameter.u_start = 0;
	specification_parameter.i_start = 0;
#endif

    if (phase == 3)
    {
        if (no_current == (MEASURE_PHASE_BIT_MAP & mask_type))
        {
            which_instantaneous->real[POW_ACT]   = 0;
            which_instantaneous->real[POW_REACT] = 0;
            which_instantaneous->real[POW_APPAR] = 0;
            which_instantaneous->real[POWFACT]   = REAL_PF_DEFAULT;
        }
        
        if(under_05ib == 0)
        {
            which_instantaneous->state |= STA_I_ALL_OVER_5IB;
        }
        else if(under_05ib == (MEASURE_PHASE_BIT_MAP & mask_type))
        {
            which_instantaneous->state |= STA_I_ALL_UNDER_5IB;
        }
        
        if(under_60un == 0)
        {
            which_instantaneous->state |= STA_U_ALL_OVER_60UN;
        }
        else if(under_60un == (MEASURE_PHASE_BIT_MAP & mask_type))
        {
            which_instantaneous->state |= STA_U_ALL_UNDER_60UN;
        }        
        if(under_68un == (MEASURE_PHASE_BIT_MAP & mask_type))
        {
            which_instantaneous->state |= STA_U_ALL_UNDER_68UN;
        }        
        if (true == emu_driver.read(USER1, (uint8 *)&user1, 4, &device_emu))
        {
            if(user1 & 0x00000008)
            {
                which_instantaneous->state |= STA_U_PHASE_SEQUENCE_REV;
            }
            if(user1 & 0x00000010)
            {
                which_instantaneous->state |= STA_I_PHASE_SEQUENCE_REV;
            }
        }
        
        if((which_instantaneous->real[POW_ACT] < 0)&&(last_power_dir > 0))
        {
            which_instantaneous->state |= STA_ACITVE_POWER_DIR_CHANGE;
            #if 0
            last_power_dir = which_instantaneous->real[POW_ACT] >> 24;
            #endif
            last_power_dir = 0xff;
        }
        
        if((which_instantaneous->real[POW_ACT] > 0)&&(last_power_dir < 0))
        {
            which_instantaneous->state |= STA_ACITVE_POWER_DIR_CHANGE;
            #if 0
            last_power_dir = which_instantaneous->real[POW_ACT] >> 24;
            #endif
            last_power_dir = 0x01;
        }

        if((which_instantaneous->real[POW_REACT] < 0)&&(last_reactive_power_dir > 0))
        {
            which_instantaneous->state |= STA_REACITVE_POWER_DIR_CHANGE;
            #if 0
            last_reactive_power_dir = which_instantaneous->real[POW_REACT] >> 24;
            #endif
            last_reactive_power_dir = 0xff;
        }
        
        if((which_instantaneous->real[POW_REACT] > 0)&&(last_reactive_power_dir < 0))
        {
            which_instantaneous->state |= STA_REACITVE_POWER_DIR_CHANGE;
            #if 0
            last_reactive_power_dir = which_instantaneous->real[POW_REACT] >> 24;
            #endif
            last_reactive_power_dir = 0x01;
        }
          
        if(true == plugin_machine.adc_start_one(2))
        {
            plugin_machine.read_adc_measure_value(2 , (uint32*)&(which_instantaneous->real[4]));
        }
        if(true == plugin_machine.adc_start_one(4))
        {
            plugin_machine.read_adc_measure_value(4 , (uint32*)&(which_instantaneous->real[6]));

            if (which_instantaneous->real[6] < 4800)
            {
                battery_under4v8 = 0x5aa5;
            }
            else
            {
                battery_under4v8 = 0;
            }
        }
#ifndef WIN32
        which_instantaneous->real[7] = RTC->TEMP;//暂时放一下
        which_instantaneous->real[7] &= 0x3FF;
        if(which_instantaneous->real[7] & 0x200)
        {
            which_instantaneous->real[7] = 0x3FF - which_instantaneous->real[7];
            which_instantaneous->real[7] = 0 - which_instantaneous->real[7];
        }
        which_instantaneous->real[7] = which_instantaneous->real[7] * 250;
#endif
    }
    else
    {
        if(which_instantaneous->real[FREORVOL] < specification_parameter.u_start)
        {
            which_instantaneous->real[FREORVOL]  = 0;
            which_instantaneous->real[POW_ACT]   = 0;
            which_instantaneous->real[POW_REACT] = 0;
            which_instantaneous->real[POW_APPAR] = 0;
            which_instantaneous->real[POWFACT]   = REAL_PF_DEFAULT;
        }
        
        abs_current = hfp_abs(which_instantaneous->real[CURRENT]);
        
        not_mask = 0x01 << phase;
        mask = ~not_mask;
        
        if (abs_current < specification_parameter.ib_005)
        {
            under_05ib |= not_mask;
            if(which_instantaneous->real[FREORVOL] > 0)
            {
                s = (float)which_instantaneous->real[POW_APPAR]*1000/which_instantaneous->real[FREORVOL];
                which_instantaneous->real[CURRENT] = (int32)s;
            }
        }
        else
        {
            under_05ib &= mask;
        }   
        
        abs_current = hfp_abs(which_instantaneous->real[CURRENT]);
        
        if (abs_current < specification_parameter.i_start)
        {
            which_instantaneous->real[CURRENT]   = 0;
            which_instantaneous->real[POW_ACT]   = 0;
            which_instantaneous->real[POW_REACT] = 0;
            which_instantaneous->real[POW_APPAR] = 0;
            which_instantaneous->real[POWFACT]   = REAL_PF_DEFAULT;
            which_instantaneous->real[UI_ANGEL] = 0;
            no_current |= not_mask;
        }
        else
        {
            no_current &= mask;

            which_instantaneous->current_acc->milli_ampere_second += abs_current;
            if (which_instantaneous->current_acc->milli_ampere_second > 360000)//0.01AH
            {
                which_instantaneous->current_acc->ampere_hour += which_instantaneous->current_acc->milli_ampere_second / 360000;
                which_instantaneous->current_acc->milli_ampere_second %= 360000;
            }
            current_acc_crc[phase] = hfp_crc16_calculate((uint8*)&current_acc[phase] , sizeof(current_acc[0]));
        }
        
        if (which_instantaneous->real[FREORVOL] < specification_parameter.u_60)
        {
            under_60un |= not_mask;
        }
        else
        {
            over_flag |= FLAG_OVER_60UN;
            under_60un &= mask;
        }
        if (which_instantaneous->real[FREORVOL] < specification_parameter.u_68)
        {
            under_68un |= not_mask;
        }
        else
        {
            under_68un &= mask;
        }
        
        if(which_instantaneous->real[FREORVOL] > specification_parameter.un_1_2)
        {
            over_flag |= FLAG_OVER_120UN;
        }
        
        if(which_instantaneous->real[POW_ACT] < 0)
        {
            if(which_instantaneous->real[CURRENT] > 0)
            {
                which_instantaneous->real[CURRENT] = 0 - which_instantaneous->real[CURRENT];
            }
        }

        if ((specification_parameter.un == 10000) && (phase == 1))   /*三相三 b相不参与*/
        {
            under_05ib &= mask;
            no_current &= mask;
            under_60un &= mask;
            under_68un &= mask;
            for (i = 0; i < MAX_REALCLASSNUM; i++)
            {
                which_instantaneous->real[i] = 0;
            }
        }
        
    }

    if (which_instantaneous->real[POW_ACT] < 0)
    {
        if(which_instantaneous->real[POW_APPAR] > 0)
        {
            which_instantaneous->real[POW_APPAR] = 0 - which_instantaneous->real[POW_APPAR];
        }
    }
    
    
}



int32 get_real(struct phase_real_t *which_instantaneous, uint8 instantaneous_class, uint8 mode)
{
    if (instantaneous_class < MAX_REAL_CLASS_NUM)
    {
        return(which_instantaneous->real[instantaneous_class]);
    }
    return 0;
}


struct phase_power_t
{
    uint8 basicPowerID;
	uint8 judge_data;
	uint8 judge_para;
	uint8 judge_mode;
};

/** 0:无判断 ， 1:小于  ，2表示大于，3表示等于 */
const struct phase_power_t CODE  phase_power_list[13]=
{
	{ACTIVE_POWER_POS , POW_ACT ,  0 , 2,},
	{ACTIVE_POWER_NEG , POW_ACT ,  0 , 1,},
    {REACTIVE_POWER_Q1 ,POW_REACT ,1 , 3,},
    {REACTIVE_POWER_Q2 ,POW_REACT, 2 , 3,},
    {REACTIVE_POWER_Q3 ,POW_REACT, 3 , 3,},
    {REACTIVE_POWER_Q4 ,POW_REACT, 4 , 3,},
	
	{APPARENT_POWER_POS, POW_APPAR,0 , 2,},
	{APPARENT_POWER_NEG, POW_APPAR,0 , 1,},
	{REACTIVE_POWER_POS, POW_REACT,0 , 2,},
	{REACTIVE_POWER_NEG, POW_REACT,0 , 1,},

	{ACTIVE_POWER_ABS,   POW_ACT , 0, 0,},
    {REACTIVE_POWER_ABS, POW_REACT , 0, 0,},
    {APPARENT_POWER_ABS, POW_APPAR , 0, 0,},
};



uint32 get_basic_power(void *value, uint8 basicPowerID, uint8 mode)
{
	uint8 i;
    struct phase_real_t *which_instantaneous = (struct phase_real_t*)value;
    
	for(i=0;i<13;i++)
	{
		if(basicPowerID == phase_power_list[i].basicPowerID)
		{
			switch(phase_power_list[i].judge_mode)
			{
				case 0:
					    return hfp_abs(which_instantaneous->real[phase_power_list[i].judge_data]);
				case 1:
						if(which_instantaneous->real[phase_power_list[i].judge_data] < phase_power_list[i].judge_para)
						{
							return hfp_abs(which_instantaneous->real[phase_power_list[i].judge_data]);
						}
                        return 0;
						break;
				case 2:
						if(which_instantaneous->real[phase_power_list[i].judge_data] > phase_power_list[i].judge_para)
						{
							 return hfp_abs(which_instantaneous->real[phase_power_list[i].judge_data]);
						}
                        return 0;
						break;
				case 3:
						 if(which_instantaneous->quadrant == phase_power_list[i].judge_para)
						 {
							 return hfp_abs(which_instantaneous->real[phase_power_list[i].judge_data]);
						 }
                         return 0;
						 break;	
				default:
					return 0 ;
			}
		}
	}
	return 0;
}

extern uint8 get_index_combination_mode(uint8 index);


int32 combination_power(struct phase_real_t *which_instantaneous , uint8 get_category  ,  uint8 mode)
{
    uint8 i,j,k,bit_ctrl;
    int32 ec;
    int32 com_value = 0;
    uint8 com_bin_ctrl = 0;
    
    if (get_category < 8)
    {
        return 0;
    }

    if (get_category < 11)
    {
        j = 2;
        k = 0;
    }
    else
    {
        j = 6;
        k = 2;
    }
    get_category -= 8;

    com_bin_ctrl = get_index_combination_mode(get_category);
    bit_ctrl = 0;
    for (i = k; i < j; i++)
    {
        ec = get_basic_power(which_instantaneous , i , mode);
        
        if (com_bin_ctrl & (0x01 << (bit_ctrl << 1)))       /**< 加组合判断 */
        {
            com_value += ec;                       /**< 瓦时电能加 */
        }
        if (com_bin_ctrl & (0x01 << ((bit_ctrl << 1) + 1))) /**< 减组合判断 */
        {
            com_value -= ec;                       /**< 瓦时电能减 */
        }
        bit_ctrl++;
    }
    
    return com_value;
}


int32 get_power(struct phase_real_t *which_instantaneous, uint8 instantaneous_class, uint8 mode)
{
    if(instantaneous_class > 7)
    {
        return combination_power (which_instantaneous , instantaneous_class ,  mode);
    }
    return get_basic_power(which_instantaneous , instantaneous_class, mode);
}

uint32 hfp_get_combine_acc_energy(uint8 phase , uint8 basicPowerID)
{
	struct phase_real_t     *which_instantaneous;

	which_instantaneous = instantaneous_callback.get_instantaneous_memory(phase);

	return get_basic_power(which_instantaneous,  basicPowerID, 0 );
}
/*
int32 get_max(struct phase_real_t *which_instantaneous, uint8 instantaneous_class, uint8 mode)
{
    uint32 value;

    if (instantaneous_class < STATISTICS_NUM)
    {
        value = which_instantaneous->maximum[instantaneous_class];
        which_instantaneous->maximum[instantaneous_class] = 0;
        return value;
    }

    return 0;
}


int32 get_min(struct phase_real_t *which_instantaneous, uint8 instantaneous_class, uint8 mode)
{
    uint32 value;

    if (instantaneous_class < STATISTICS_NUM)
    {
        value = which_instantaneous->minimum[instantaneous_class];
        which_instantaneous->minimum[instantaneous_class] = 0xffffffffU;

        if (value != 0xffffffffU)
        {
            return value;
        }
    }

    return 0;
}


int32 get_avg(struct phase_real_t *which_instantaneous, uint8 instantaneous_class, uint8 mode)
{
    uint32 value;

    if (instantaneous_class < STATISTICS_NUM)
    {
        if (which_instantaneous->average->cnt[instantaneous_class])
        {
            value = which_instantaneous->average->val[instantaneous_class] / which_instantaneous->average->cnt[instantaneous_class];
            which_instantaneous->average->val[instantaneous_class] = 0;
            which_instantaneous->average->cnt[instantaneous_class] = 0;
            return value;
        }

        return 0;
    }

    return 0;
}
*/

int32 get_ampere_hour(struct phase_real_t *which_instantaneous, uint8 instantaneous_class, uint8 mode)
{
    switch(mode)
    {
        case 0:
            return (current_acc[0].ampere_hour + current_acc[1].ampere_hour + current_acc[2].ampere_hour);
        case 1:
            return current_acc[0].ampere_hour;
        case 2:
            return current_acc[1].ampere_hour;
        case 3:
            return current_acc[2].ampere_hour;
    }
    return 0;
}

uint8 get_local_instantaneous_decimal(uint8 instantaneous_class , uint8 phase , struct unit_decmail_t *local_decimal)
{ 
    local_decimal->unit = decimal_list[instantaneous_class].unit;
    local_decimal->decmail = decimal_list[instantaneous_class].decmail;
    return true;
}
uint8 get_local_power_decimal(uint8 instantaneous_class , uint8 phase , struct unit_decmail_t *local_decimal)
{ 
    local_decimal->unit = unit_W;
    local_decimal->decmail = -3;
    return true;
}

uint8 get_local_ampere_hour_decimal(uint8 instantaneous_class , uint8 phase , struct unit_decmail_t *local_decimal)
{
    local_decimal->unit = unit_Ah;
    local_decimal->decmail = -2;

    return true;
}

const struct read_ops_t CODE read_ops_list[] =
{
    { get_real,                 get_local_instantaneous_decimal , 0x00, },
    { get_power ,               get_local_power_decimal ,         0x00, },
    { get_ampere_hour ,         get_local_ampere_hour_decimal ,   0x00, },
    { NULL ,                    NULL ,                            0x00, },
};


uint8 voltage_reverse_phase_sequence(struct phase_real_t *which_instantaneous)
{
    if(which_instantaneous->state & STA_U_PHASE_SEQUENCE_REV)
    {
        return 1;
    }
    return 0;
}

uint8 current_reverse_calc(void)
{
    uint8 i;
    int32   angle_ui[MAX_PHASE_NUM];
    uint32  angle_uu[MAX_PHASE_NUM];
    uint32  angle_ii[MAX_PHASE_NUM];
        
    struct phase_real_t *this_instantaneous;

    for (i = 0; i < MAX_PHASE_NUM; i++)
    {
        this_instantaneous = instantaneous_callback.get_instantaneous_memory(i);

        angle_ui[i] = this_instantaneous->real[UI_ANGEL];

        angle_uu[i] = this_instantaneous->real[U_ANGEL];

        angle_ii[i] = angle_ui[i] + angle_uu[i] /* + ANGLE_OFFSET */;

        if (angle_ii[i] > ANGLE_FULL_SCALE)
        {
            angle_ii[i] -= ANGLE_FULL_SCALE;
        }


        if (i != 0)
        {
            if (angle_ii[i] < angle_ii[0])
            {
                angle_ii[i] += ANGLE_FULL_SCALE;
            }
            angle_ii[i] -= angle_ii[0];
        }
    }
    angle_ii[0] = 0;

    if ((angle_ii[1] > angle_ii[2]) && (angle_ii[2] != angle_ii[0]))
    {
        return true;
    }

    return false;
}

uint8 current_reverse_phase_sequence(struct phase_real_t *which_instantaneous)
{
    current_reverse_calc();
    
    if (which_instantaneous->state & STA_I_PHASE_SEQUENCE_REV)
    {
        if (specification_parameter.un != 10000) /*三相四线*/
        {
            return current_reverse_calc();
        }
        else
        {
            return 1;
        }
    }
    return 0;
}

uint8 quadrant_judge(struct phase_real_t *which_instantaneous)
{
    return which_instantaneous->quadrant;
}

uint8 active_power_forward_direction_judge(struct phase_real_t *which_instantaneous)
{
    if (which_instantaneous->real[POW_ACT] >= 0)
    {
        return 1;
    }
    return 0;
}

uint8 active_power_reverse_direction_judge(struct phase_real_t *which_instantaneous)
{
    if (which_instantaneous->real[POW_ACT] < 0)
    {
        return 1;
    }
    return 0;
}

uint8 active_power_direction_change(struct phase_real_t *which_instantaneous)
{
    uint8 tmp = 0;
    if(which_instantaneous->state & STA_ACITVE_POWER_DIR_CHANGE)
    {
        tmp = 1;
        which_instantaneous->state &= ~STA_ACITVE_POWER_DIR_CHANGE;
    }
    return tmp;
}

uint8 reactive_power_direction_change(struct phase_real_t *which_instantaneous)
{
    uint8 tmp = 0;
    if(which_instantaneous->state & STA_REACITVE_POWER_DIR_CHANGE)
    {
        tmp = 1;
        which_instantaneous->state &= ~STA_REACITVE_POWER_DIR_CHANGE;
    }
    return tmp;
}

uint8 current_all_under_05ib_judge(struct phase_real_t *which_instantaneous)
{
    if (which_instantaneous->state & STA_I_ALL_UNDER_5IB)
    {
        return 1;
    }
    return 0;
}
uint8 voltage_all_under_60un_judge(struct phase_real_t *which_instantaneous)
{
    if (which_instantaneous->state & STA_U_ALL_UNDER_60UN)
    {
        return 1;
    }
    return 0;
}
uint8 voltage_all_under_68un_judge(struct phase_real_t *which_instantaneous)
{
    if (which_instantaneous->state & STA_U_ALL_UNDER_68UN)
    {
        return 1;
    }
    return 0;
}

uint8 voltage_over_120un_judge(struct phase_real_t *which_instantaneous)
{
    if (over_flag & FLAG_OVER_120UN)
    {
        return 1;
    }
    return 0;
}

uint8 rtc_batter_under_voltage(struct phase_real_t *which_instantaneous)
{
    return (which_instantaneous->real[4] < 28000);
}

uint8 read_meter_batter_under_voltage(struct phase_real_t *which_instantaneous)
{
    return (which_instantaneous->real[6] < 4800);
}

uint8 reactive_power_reverse_direction_judge(struct phase_real_t *which_instantaneous)
{
    return (which_instantaneous->real[POW_REACT] < 0);
}

uint8 current_all_over_05ib_judge(struct phase_real_t *which_instantaneous)
{
    if (which_instantaneous->state & STA_I_ALL_OVER_5IB)
    {
        return 1;
    }
    return 0;
}

uint8 voltage_all_over_60un_judge(struct phase_real_t *which_instantaneous)
{
    if (which_instantaneous->state & STA_U_ALL_OVER_60UN)
    {
        return 1;
    }
    return 0;
}

uint8 power_aberrant_judge(struct phase_real_t *which_instantaneous)
{
    if(over_flag & FLAG_POWER_ABERRANT)
    {
        over_flag &= ~FLAG_POWER_ABERRANT;
        return true;
    }
    return false;
}

uint8 all_voltage_lost(struct phase_real_t *which_instantaneous)
{
    if(over_flag & FLAG_ALL_VOLTAGE_LOST)
    {
        //over_flag &= ~FLAG_ALL_VOLTAGE_LOST;
        return true;
    }
    return false;    
}
const struct read_instantaneous_status_t read_instantaneous_status_list[]=
{
  {0x6F30,     reactive_power_reverse_direction_judge,},
  {0x6F31,     power_aberrant_judge,},
  {0x6F32,     active_power_reverse_direction_judge,},
  {0x6F33,     all_voltage_lost,},
  {0x6F34,     voltage_reverse_phase_sequence,},
  {0x6F35,     current_reverse_phase_sequence,},
  {0x6F36,     active_power_direction_change,},
  {0x6F2F,     reactive_power_direction_change,},
  {0x6F37,     current_all_under_05ib_judge,},
  {0x6F38,     voltage_all_under_60un_judge,},
  {0x6F39,     voltage_all_under_68un_judge,},
  {0x6F3A,     current_all_over_05ib_judge,},
  {0x6F3B,     voltage_over_120un_judge,},
  {0x6F3C,     voltage_all_over_60un_judge,},
  {0x6F3D,     read_meter_batter_under_voltage,},
  {0x6F3E,     rtc_batter_under_voltage,},
  {0x6F3F,     quadrant_judge,    },
  
  {0x6F00,      reactive_power_reverse_direction_judge,},
  {0x6F02,      active_power_reverse_direction_judge,},
  {0x6F03,      active_power_forward_direction_judge,},
  {0x6F10,      reactive_power_reverse_direction_judge,},
  {0x6F12,      active_power_reverse_direction_judge,},
  {0x6F13,      active_power_forward_direction_judge,},
  {0x6F20,      reactive_power_reverse_direction_judge,},
  {0x6F22,      active_power_reverse_direction_judge,},
  {0x6F23,      active_power_forward_direction_judge,},
  
  {0x0000,     0x00, },
};


struct phase_real_t *get_instantaneous_root(uint8 phase)
{
    if (phase < MAX_PHASE_NUM)
    {
        instantaneous[phase].real              = &real[phase][0];
        instantaneous[phase].maximum           = __NULL;
        instantaneous[phase].minimum           = __NULL;
        instantaneous[phase].average           = __NULL;
        //instantaneous[phase].average->cnt      = __NULL;
        //instantaneous[phase].average->val      = __NULL;
        instantaneous[phase].real_id = &real_list[phase].real_id[0];
        instantaneous[phase].real_fact         = factor;
        instantaneous[phase].read_ops_list     = read_ops_list;
        instantaneous[phase].read_status_list  = read_instantaneous_status_list;
        instantaneous[phase].current_acc       = &current_acc[phase];
        return &instantaneous[phase];
    }

    return 0;
}
void instantaneous_poweron_hook(void)
{
    uint8 i;
  
    last_power_dir = 1;
    last_reactive_power_dir = 1;
    no_current = 0;
    under_05ib = 0;
    under_68un = 0;
    under_60un = 0;
    over_flag = 0;
    
    for(i=0;i<3;i++)
    {
        if(current_acc_crc[i] != hfp_crc16_calculate((uint8*)&current_acc[i] , sizeof(current_acc[0])))
        {
            if(true == hfp_read(0x2803+i, (uint8*)&current_acc[i], sizeof(current_acc[0]), __NULL))
            {
                current_acc_crc[i] = hfp_crc16_calculate((uint8*)&current_acc[i] , sizeof(current_acc[0]));
            }
        }
    }
    
    
}

uint8 instantaneous_poweroff_hook(uint8 *buf)
{
	struct file_stat_t stat;
	uint8 *ptr;
    uint8 len = 0;
    uint8 i;
    uint16 check_sum;

    power_statue = 0x5A;
    
    if (true == hfp_stat(0x2803, &stat))
    {
        ptr = buf + (stat.info.addr - stat.info.first_addr);
        for(i=0;i<3;i++)
        {
            hfp_memcopy(ptr, (uint8*)&current_acc[i], sizeof(current_acc[0]));
            check_sum = 0xa5a5 + hfp_do_sum(ptr, sizeof(current_acc[0]));
            ptr += sizeof(current_acc[0]);
            *(ptr++) = (uint8)(check_sum>>8);
            *(ptr++) = (uint8)check_sum;
            len += (sizeof(current_acc[0]) + 2);
        }
        return len;
    }
    return 0;
}

uint8 init_emu_adj_parameter(void)
{
    uint8              i;
    uint8              temp_buffer[32];

    for (i = 0; i < 3; i++)
    {
        if (true == hfp_read(0x2001 + i, (uint8 *)temp_buffer, 22, __NULL))
        {
             if(false == emu_driver.ioctrl(CTRL_EMU_ADJ_A_SET + i, &device_emu, temp_buffer))
             {
                 return false;
             }
        }
        else
        {
            return false;
        }
    }
    return true;
}



void emu_calibration_oneself(void)
{
    uint8 i;
	int32 value;
    struct adj_std_source_t *stdSource;
#if 0
    float                a;
#endif
    struct phase_real_t  *instantaneous;
    struct calibration_t calibration;
    struct error_t  err;
    struct calibration_parameter_t calibration_parameter;


    struct emu_value
    {
        int32 u_m;
        int32 i_m;
        int32 p_m;
        int32 q_m;
    };
    struct emu_value emuValue[3];
    uint8            buffer[52];

	
    if (true == hfp_read(0x2FF1, (uint8 *)buffer, 52, __NULL))
    {
        stdSource = (struct adj_std_source_t *)buffer;
    }
    else
    {
        return;
    }
    
    
    for (i = 0; i < 3; i++)
    {
        hfp_read(0x2001 + i, (uint8 *)(&calibration_parameter), sizeof(calibration_parameter), __NULL);
        instantaneous = instantaneous_callback.get_instantaneous_memory(i);
        
        if (instantaneous)
        {
            emuValue[i].u_m = instantaneous->real[FREORVOL];		/*xxx.xxV*/
            emuValue[i].i_m = instantaneous->real[CURRENT];			/*x.xxxxA*/
            emuValue[i].p_m = instantaneous->real[POW_APPAR] / 10;	/*xxx.xxVa*/

            if (stdSource->volatge == 100) /*电压1V的时候初始化配置寄存器*/
            {
                emu_driver.ioctrl(CTRL_EMU_CONFIG_DEFAULT , &device_emu, __NULL);
                //emu_driver.ioctrl(CTRL_EMU_ADJ_INIT , &device_emu, &calibration_parameter);
                return;
            }
            
            if ((stdSource->volatge == 0) || (stdSource->current == 0) || (stdSource->actPower == 0))  
            {
                stdSource++;
                continue;
            }
#if 0            
            a     = (float)stdSource->volatge * stdSource->current;
#endif
            
            err.calibration_error[0] = stdSource->volatge * 32768 / emuValue[i].u_m;
            err.calibration_error[1] = stdSource->current * 32768 / emuValue[i].i_m;
            err.calibration_error[2] = stdSource->actPower * 32768 / emuValue[i].p_m;
            
            calibration.error = &err;
            calibration.calibration_parameter_back = &calibration_parameter;
            calibration.calibration_bit_map = 0x07;
            emu_driver.ioctrl(CTRL_EMU_ADJ_A_SELF + i , &device_emu, &calibration);

            hfp_write(0x2001 + i, (uint8 *)(&calibration_parameter), sizeof(calibration_parameter), __NULL);
        }

        stdSource++;
    }
    
#if 1
    /*校准相位 需在功率增益校准后适当延时*/
	stdSource = (struct adj_std_source_t *)buffer;
    for(i = 200; i > 0; i--)
    {
    	plugin_machine.mcu_delay_us(5000);
        plugin_machine.clear_watchdog();
    }

	for (i = 0; i < 3; i++)
	{
	    hfp_read(0x2001 + i, (uint8 *)(&calibration_parameter), sizeof(calibration_parameter), __NULL);
        instantaneous = instantaneous_callback.get_instantaneous_memory(i);
		if (instantaneous)
		{
			if (true == emu_driver.read(real_list[i].real_id[POW_ACT], (uint8 *)&value, 4, &device_emu))
			{
				value = (int32)((factor[POW_ACT]*value) + 0.5);
			}
			emuValue[i].q_m = value / 10;	/*xxx.xxW*/

			if ((stdSource->volatge == 0) || (stdSource->current == 0) || (stdSource->actPower == 0) )
			{
			    stdSource++;
				continue;
			}

			err.calibration_error[4] = (float)  emuValue[i].q_m / stdSource->actPower ;
			
            calibration.error = &err;
            calibration.calibration_parameter_back = &calibration_parameter;
			calibration.calibration_bit_map = 0x10;
			emu_driver.ioctrl(CTRL_EMU_ADJ_A_SELF + i , &device_emu, &calibration);

			hfp_write(0x2001 + i, (uint8 *)(&calibration_parameter), sizeof(calibration_parameter), __NULL);
		}

		stdSource++;
	}

#endif


}

extern void lowpower_event(uint8 check_name);

void instantaneous_awaken(void)
{
    uint8 i;
    int32 value;
    uint32 th;
    
    if(power_statue != 0x5A)
    {
        return;
    }

    if(hfp_system_state.power_off_sencond > 59)
    {
        return;
    }
    
    switch(hfp_system_state.power_off_sencond)
    {
        case 1:
        case 58:
            emu_driver.init(&device_emu, POWER_ON_MODE);
            emu_driver.ioctrl(CTRL_DEVICE_OPEN, &device_emu, __NULL);
            if(false == init_emu_adj_parameter())
            {
                emu_driver.ioctrl(CTRL_DEVICE_CLOSE, &device_emu, __NULL);
            }
            hfp_system_state.next_wakeup_osc_mode = WAKEUP_HIGH_OSC;
            break;
        case 57:
            hfp_system_state.next_wakeup_osc_mode = WAKEUP_HIGH_OSC;
            break;
        case 2:
            th = (uint32)specification_parameter.un*51;
            for(i=0;i<3;i++)
            {
                if (true == emu_driver.read(real_list[i].real_id[FREORVOL], (uint8 *)&value, 4, &device_emu))
                {
                    
                    //大于75%UN
                    if(value > (th>>6))
                    {
                        over_flag |= FLAG_POWER_ABERRANT;
                        hfp_system_state.next_wakeup_osc_mode = WAKEUP_HIGH_OSC;
                        lowpower_event(LE_DCDETECTED);
                        break;
                    }
                }
            }
            emu_driver.ioctrl(CTRL_DEVICE_CLOSE, &device_emu, __NULL);
            break;
        case 59:
            for(i=0;i<3;i++)
            {
                if (true == emu_driver.read(real_list[i].real_id[CURRENT], (uint8 *)&value, 4, &device_emu))
                {
                    value = (int32)(factor[CURRENT]*value);
                    
                    if(value > specification_parameter.ib_005)
                    {
                        real[i][4] = value;
                        over_flag |= FLAG_ALL_VOLTAGE_LOST;
                        hfp_system_state.next_wakeup_osc_mode = WAKEUP_HIGH_OSC;
                    }
                }
            }
            emu_driver.ioctrl(CTRL_DEVICE_CLOSE, &device_emu, __NULL);
            lowpower_event(LE_LPFALLPHASES + LE_VOLTAGELOSTALL + LE_ASSISTPOWERLOST); /*!!!! 辅助电源测试*/
            break;
            
        default:
            
            break;
    }
}
 
const struct instantaneous_callback_t CODE instantaneous_callback =
{
    get_instantaneous_root,
    instantaneous_poweron_hook,
    cal_instantaneous_hook,
    instantaneous_poweroff_hook,
    instantaneous_awaken,
};

#ifndef WIN32
extern const struct gpio_pin_t CODE pin_clk_emu;
extern const struct gpio_pin_t CODE pin_miso_emu;
extern const struct gpio_pin_t CODE pin_mosi_emu;
extern const struct gpio_pin_t CODE pin_cs_emu;
extern const struct gpio_pin_t CODE pin_power_ctrl_emu;
extern const struct gpio_pin_t CODE pin_sleep_ctrl_emu;
extern const struct gpio_pin_t CODE pin_reset_emu;

const struct spi_bus_resource_t CODE emu_spi_resource = 
{
  &pin_clk_emu,
  &pin_miso_emu,
  &pin_mosi_emu,
};
const struct emu_hal_cfg_t CODE emu_hal_cfg = 
{
    &pin_power_ctrl_emu,
    &pin_cs_emu,
    &pin_sleep_ctrl_emu,
    &pin_reset_emu,
};
extern const struct spi_bus_ops_t CODE spi_bus_driver;


const struct hfp_device_t CODE device_emu = 
{
    DEVICE_EMU,
    BUS_SPI,
    &spi_bus_driver,
    &emu_spi_resource,
    &emu_hal_cfg,
    &emu_driver,
};
#endif
