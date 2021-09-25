/**
 * @defgroup    energy_cfg  Energy Cfg
 * @ingroup     cfg
 * @see         energy
 * @brief       电能计量配置
 *
 *              包括计量方式...
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "energy.h"
#include "fs.h"
#include "lib_calclib.h"

#define  MAX_PHASE_NUM                  4
#define  MAX_BASIC_ENERGY_CLASS_NUM     6
#define  MAX_BASIC_ENERGY_TARIFF_NUM    5
#define  MAX_PLUSE_ACC_PHASE_NUM        4



//#define  INC_DIFF                       600
//#define  INC_DIFF                       100

#pragma object_attribute=__no_init
uint32 energy_kwh[MAX_PHASE_NUM][MAX_BASIC_ENERGY_CLASS_NUM][MAX_BASIC_ENERGY_TARIFF_NUM];
#pragma object_attribute=__no_init
uint16 energy_pluse[MAX_PHASE_NUM][MAX_BASIC_ENERGY_CLASS_NUM][MAX_BASIC_ENERGY_TARIFF_NUM];
#pragma object_attribute=__no_init
struct energy_root_t     energy_root[MAX_PHASE_NUM][MAX_BASIC_ENERGY_CLASS_NUM];
#pragma object_attribute=__no_init
struct energy_variable_t energy_var[MAX_PHASE_NUM][MAX_BASIC_ENERGY_CLASS_NUM];
#pragma object_attribute=__no_init
struct energy_object_t   energy[MAX_PHASE_NUM][MAX_BASIC_ENERGY_CLASS_NUM];


/**月度电能*/
#pragma object_attribute=__no_init
uint32 month_energy_kwh[MAX_BASIC_ENERGY_TARIFF_NUM];
#pragma object_attribute=__no_init
uint16 month_energy_pluse[MAX_BASIC_ENERGY_TARIFF_NUM];
#pragma object_attribute=__no_init
struct energy_root_t     month_energy_root;
#pragma object_attribute=__no_init
struct energy_variable_t month_energy_var;
#pragma object_attribute=__no_init
struct energy_object_t   month_energy;


/**年度电能*/
#pragma object_attribute=__no_init
uint32 year_energy_kwh[MAX_BASIC_ENERGY_TARIFF_NUM];
#pragma object_attribute=__no_init
uint16 year_energy_pluse[MAX_BASIC_ENERGY_TARIFF_NUM];
#pragma object_attribute=__no_init
struct energy_root_t year_energy_root;
#pragma object_attribute=__no_init
struct energy_variable_t year_energy_var;
#pragma object_attribute=__no_init
struct energy_object_t year_energy;

struct pluse_inc_t pluse_inc;

uint8  combination_mode[7];
uint16 pluse_inc_crc;
uint16 combination_crc[7];

uint16 pulse_allsave_state = 0x3B69;

const struct pluse_inc_t CODE pluse_inc_default =
{
    4, 600,
};
//static const uint32 CODE pluse_const_default         = 4;
static const uint8 CODE  combination_mode_default[7] = 
{
    0x05,0x01,0x04,
    0x41, 0x14,0x55,0x55,
};


const struct energy_fsid_t CODE energy_id_list[MAX_PHASE_NUM][MAX_BASIC_ENERGY_CLASS_NUM] =
{
    { { 0x8000, 0x8500, 0x8C80 }, { 0x8010, 0x8510, 0x8C90 }, { 0x8020, 0x8520, 0x8CA0 }, { 0x8030, 0x8530, 0x8CB0 }, { 0x8040, 0x8540, 0x8CC0 }, { 0x8050, 0x8550, 0x8CD0 }, /*{0x8060,0x8CE0},{0x8070,0x8CF0},*/ },
    { { 0x8100, 0x8600, 0x8D80 }, { 0x8110, 0x8610, 0x8D90 }, { 0x8120, 0x8620, 0x8DA0 }, { 0x8130, 0x8630, 0x8DB0 }, { 0x8140, 0x8640, 0x8DC0 }, { 0x8150, 0x8650, 0x8DD0 }, /*{0x8160,0x8DE0},{0x8170,0x8DF0},*/ },
    { { 0x8200, 0x8700, 0x8E80 }, { 0x8210, 0x8710, 0x8E90 }, { 0x8220, 0x8720, 0x8EA0 }, { 0x8230, 0x8730, 0x8EB0 }, { 0x8240, 0x8740, 0x8EC0 }, { 0x8250, 0x8750, 0x8ED0 }, /*{0x8260,0x8EE0},{0x8270,0x8EF0},*/ },
    { { 0x8300, 0x8800, 0x8F80 }, { 0x8310, 0x8810, 0x8F90 }, { 0x8320, 0x8820, 0x8FA0 }, { 0x8330, 0x8830, 0x8FB0 }, { 0x8340, 0x8840, 0x8FC0 }, { 0x8350, 0x8850, 0x8FD0 }, /*{0x8360,0x8FE0},{0x8370,0x8FF0},*/ },
};
const struct energy_fsid_t CODE month_energy_id_list =
{
    0x8400, 0x8900, 0x8B80,
};
const struct energy_fsid_t CODE year_energy_id_list =
{
    0x8410, 0x8910, 0x8B90,
};

const struct parameter_attr_t CODE energy_parameter[] =
{
    {
        0x2006,
        4,
        &pluse_inc,
        &pluse_inc_crc,
        hfp_crc16_calculate,
        &pluse_inc_default,
        0,
    },

    {
        0x2165,
        1,
        &combination_mode[0],
        &combination_crc[0],
        hfp_crc16_calculate,
        &combination_mode_default[0],
        1,
    },

    {
        0x2166,
        1,
        &combination_mode[3],
        &combination_crc[3],
        hfp_crc16_calculate,
        &combination_mode_default[3],
        1,
    },

    {
        0x2167,
        1,
        &combination_mode[4],
        &combination_crc[4],
        hfp_crc16_calculate,
        &combination_mode_default[4],
        1,
    },
};

const uint8 CODE energy_parameter_num = sizeof(energy_parameter) / sizeof(energy_parameter[0]);


#ifdef WIN32
static uint32 pluse_acc[MAX_PLUSE_ACC_PHASE_NUM][MAX_BASIC_ENERGY_CLASS_NUM];
#endif


/*
const uint16 acc_energy_fsid[MAX_PLUSE_ACC_PHASE_NUM][M9AX_BASIC_ENERGY_CLASS_NUM] =
{
    { 0xC100, 0xC101, 0xC102, 0xC103, 0xC104, 0xC105 },
    { 0xC100, 0xC101, 0xC102, 0xC103, 0xC104, 0xC105 },
    { 0xC100, 0xC101, 0xC102, 0xC103, 0xC104, 0xC105 },
};
*/
/**
 * @fn  void energy_add_hook(struct energy_object_t *energy, struct energy_ops_attr_t *energy_ops_attr)
 *
 * @brief   Energy add hook.
 *
 * @param [in,out]  energy          If non-null, the energy.
 * @param [in,out]  energy_ops_attr If non-null, the energy ops attribute.
 */




struct energy_object_t *get_energy_root(uint8 phase, uint8 energy_class , uint8 hook);

const struct energy_callback_t CODE energy_callback =
{
    get_energy_root,
    hfp_crc16_calculate,
};


extern const struct energy_interface_t CODE plugin_energy;


const struct energy_user_cfg_t CODE energy_cfg =
{
    &plugin_energy,

    &energy_callback,

    //pluse_inc.inc_diff,
    MAX_PHASE_NUM,
    MAX_BASIC_ENERGY_CLASS_NUM,
    MAX_BASIC_ENERGY_TARIFF_NUM,
    0,
    3,
};

struct energy_object_t *get_energy_root(uint8 phase, uint8 energy_class , uint8 hook)
{
    switch(hook)
    {
        case 0:
            if ((phase < MAX_PHASE_NUM) && (energy_class < MAX_BASIC_ENERGY_CLASS_NUM))
            {
                energy_root[phase][energy_class].ec_kwh           = &energy_kwh[phase][energy_class][0];
                energy_root[phase][energy_class].ec_pulse         = &energy_pluse[phase][energy_class][0];
                #ifdef WIN32
                energy_root[phase][energy_class].pluse_acc = &pluse_acc[phase][energy_class];
                #endif
                energy[phase][energy_class].root = &energy_root[phase][energy_class];
                energy[phase][energy_class].var  = &energy_var[phase][energy_class];
                energy[phase][energy_class].energy_fsid = &energy_id_list[phase][energy_class];
                return &energy[phase][energy_class];
            }
            break;
    case 1:
            if ((phase == 3) && (energy_class == 0))
            {
                month_energy_root.ec_kwh           = &month_energy_kwh[0];
                month_energy_root.ec_pulse         = &month_energy_pluse[0];
                month_energy_root.pluse_acc        = __NULL;
                month_energy.root = &month_energy_root;
                month_energy.var  = &month_energy_var;  
                month_energy.energy_fsid = &month_energy_id_list;
                return &month_energy;
            }
            break;
    case 2:       
            if ((phase == 3) && (energy_class == 0))
            {
                year_energy_root.ec_kwh           = &year_energy_kwh[0];
                year_energy_root.ec_pulse         = &year_energy_pluse[0];
                year_energy_root.pluse_acc        = __NULL;
                year_energy.root = &year_energy_root;
                year_energy.var  = &year_energy_var;  
                year_energy.energy_fsid = &year_energy_id_list;
                return &year_energy;
            }
            break;
     default:
            break;
    }
    return NULL;
}
