#include "nidavellir.h"
#include "resource.h"






//hash00二查树的第二级级表hash表没数据的补上x0000;
uint16_t hash930[1] = { 0x0000 };
uint16_t hash931[1] = { 0x0001 };
uint16_t hash932[1] = { 0x0002 };
uint16_t hash933[1] = { 0x0003 };
uint16_t hash934[1] = { 0x0004 };
uint16_t hash935[1] = { 0x0005 };
uint16_t hash936[1] = { 0x0006 };
uint16_t hash937[1] = { 0x0007 };
uint16_t hash938[1] = { 0x0008 };
uint16_t hash939[1] = { 0x0009 };
uint16_t hash93A[1] = { 0x000A };
uint16_t hash93B[1] = { 0x000B };
uint16_t hash93C[1] = { 0x000C };
uint16_t hash93D[1] = { 0x000D };
uint16_t hash93E[1] = { 0x000E };
uint16_t hash93F[1] = { 0x000F };
struct hash_index_t hash93[16] =
{
    hash930,hash931,hash932,hash933,hash934,hash935,hash936,hash937,hash938,hash939,hash93A,hash93B,hash93C,hash93D,hash93E,hash93F
};
//目前没有使用到的hash表先赋值为0x0000；
const struct hash_index_t hash00[] = { 0x0000 };
const struct hash_index_t hash01[] = { 0x0000 };
const struct hash_index_t hash02[] = { 0x0000 };
const struct hash_index_t hash03[] = { 0x0000 };
const struct hash_index_t hash04[] = { 0x0000 };
const struct hash_index_t hash05[] = { 0x0000 };
const struct hash_index_t hash06[] = { 0x0000 };
const struct hash_index_t hash07[] = { 0x0000 };
const struct hash_index_t hash08[] = { 0x0000 };
const struct hash_index_t hash09[] = { 0x0000 };
const struct hash_index_t hash0A[] = { 0x0000 };
const struct hash_index_t hash0B[] = { 0x0000 };
const struct hash_index_t hash0C[] = { 0x0000 };
const struct hash_index_t hash0D[] = { 0x0000 };
const struct hash_index_t hash0E[] = { 0x0000 };
const struct hash_index_t hash0F[] = { 0x0000 };
const struct hash_index_t hash10[] = { 0x0000 };
const struct hash_index_t hash11[] = { 0x0000 };
const struct hash_index_t hash12[] = { 0x0000 };
const struct hash_index_t hash13[] = { 0x0000 };
const struct hash_index_t hash14[] = { 0x0000 };
const struct hash_index_t hash15[] = { 0x0000 };
const struct hash_index_t hash16[] = { 0x0000 };
const struct hash_index_t hash17[] = { 0x0000 };
const struct hash_index_t hash18[] = { 0x0000 };
const struct hash_index_t hash19[] = { 0x0000 };
const struct hash_index_t hash1A[] = { 0x0000 };
const struct hash_index_t hash1B[] = { 0x0000 };
const struct hash_index_t hash1C[] = { 0x0000 };
const struct hash_index_t hash1D[] = { 0x0000 };
const struct hash_index_t hash1E[] = { 0x0000 };
const struct hash_index_t hash1F[] = { 0x0000 };
const struct hash_index_t hash20[] = { 0x0000 };
const struct hash_index_t hash21[] = { 0x0000 };
const struct hash_index_t hash22[] = { 0x0000 };
const struct hash_index_t hash23[] = { 0x0000 };
const struct hash_index_t hash24[] = { 0x0000 };
const struct hash_index_t hash25[] = { 0x0000 };
const struct hash_index_t hash26[] = { 0x0000 };
const struct hash_index_t hash27[] = { 0x0000 };
const struct hash_index_t hash28[] = { 0x0000 };
const struct hash_index_t hash29[] = { 0x0000 };
const struct hash_index_t hash2A[] = { 0x0000 };
const struct hash_index_t hash2B[] = { 0x0000 };
const struct hash_index_t hash2C[] = { 0x0000 };
const struct hash_index_t hash2D[] = { 0x0000 };
const struct hash_index_t hash2E[] = { 0x0000 };
const struct hash_index_t hash2F[] = { 0x0000 };
const struct hash_index_t hash30[] = { 0x0000 };
const struct hash_index_t hash31[] = { 0x0000 };
const struct hash_index_t hash32[] = { 0x0000 };
const struct hash_index_t hash33[] = { 0x0000 };
const struct hash_index_t hash34[] = { 0x0000 };
const struct hash_index_t hash35[] = { 0x0000 };
const struct hash_index_t hash36[] = { 0x0000 };
const struct hash_index_t hash37[] = { 0x0000 };
const struct hash_index_t hash38[] = { 0x0000 };
const struct hash_index_t hash39[] = { 0x0000 };
const struct hash_index_t hash3A[] = { 0x0000 };
const struct hash_index_t hash3B[] = { 0x0000 };
const struct hash_index_t hash3C[] = { 0x0000 };
const struct hash_index_t hash3D[] = { 0x0000 };
const struct hash_index_t hash3E[] = { 0x0000 };
const struct hash_index_t hash3F[] = { 0x0000 };
const struct hash_index_t hash40[] = { 0x0000 };
const struct hash_index_t hash41[] = { 0x0000 };
const struct hash_index_t hash42[] = { 0x0000 };
const struct hash_index_t hash43[] = { 0x0000 };
const struct hash_index_t hash44[] = { 0x0000 };
const struct hash_index_t hash45[] = { 0x0000 };
const struct hash_index_t hash46[] = { 0x0000 };
const struct hash_index_t hash47[] = { 0x0000 };
const struct hash_index_t hash48[] = { 0x0000 };
const struct hash_index_t hash49[] = { 0x0000 };
const struct hash_index_t hash4A[] = { 0x0000 };
const struct hash_index_t hash4B[] = { 0x0000 };
const struct hash_index_t hash4C[] = { 0x0000 };
const struct hash_index_t hash4D[] = { 0x0000 };
const struct hash_index_t hash4E[] = { 0x0000 };
const struct hash_index_t hash4F[] = { 0x0000 };
const struct hash_index_t hash50[] = { 0x0000 };
const struct hash_index_t hash51[] = { 0x0000 };
const struct hash_index_t hash52[] = { 0x0000 };
const struct hash_index_t hash53[] = { 0x0000 };
const struct hash_index_t hash54[] = { 0x0000 };
const struct hash_index_t hash55[] = { 0x0000 };
const struct hash_index_t hash56[] = { 0x0000 };
const struct hash_index_t hash57[] = { 0x0000 };
const struct hash_index_t hash58[] = { 0x0000 };
const struct hash_index_t hash59[] = { 0x0000 };
const struct hash_index_t hash5A[] = { 0x0000 };
const struct hash_index_t hash5B[] = { 0x0000 };
const struct hash_index_t hash5C[] = { 0x0000 };
const struct hash_index_t hash5D[] = { 0x0000 };
const struct hash_index_t hash5E[] = { 0x0000 };
const struct hash_index_t hash5F[] = { 0x0000 };
const struct hash_index_t hash60[] = { 0x0000 };
const struct hash_index_t hash61[] = { 0x0000 };
const struct hash_index_t hash62[] = { 0x0000 };
const struct hash_index_t hash63[] = { 0x0000 };
const struct hash_index_t hash64[] = { 0x0000 };
const struct hash_index_t hash65[] = { 0x0000 };
const struct hash_index_t hash66[] = { 0x0000 };
const struct hash_index_t hash67[] = { 0x0000 };
const struct hash_index_t hash68[] = { 0x0000 };
const struct hash_index_t hash69[] = { 0x0000 };
const struct hash_index_t hash6A[] = { 0x0000 };
const struct hash_index_t hash6B[] = { 0x0000 };
const struct hash_index_t hash6C[] = { 0x0000 };
const struct hash_index_t hash6D[] = { 0x0000 };
const struct hash_index_t hash6E[] = { 0x0000 };
const struct hash_index_t hash6F[] = { 0x0000 };
const struct hash_index_t hash70[] = { 0x0000 };
const struct hash_index_t hash71[] = { 0x0000 };
const struct hash_index_t hash72[] = { 0x0000 };
const struct hash_index_t hash73[] = { 0x0000 };
const struct hash_index_t hash74[] = { 0x0000 };
const struct hash_index_t hash75[] = { 0x0000 };
const struct hash_index_t hash76[] = { 0x0000 };
const struct hash_index_t hash77[] = { 0x0000 };
const struct hash_index_t hash78[] = { 0x0000 };
const struct hash_index_t hash79[] = { 0x0000 };
const struct hash_index_t hash7A[] = { 0x0000 };
const struct hash_index_t hash7B[] = { 0x0000 };
const struct hash_index_t hash7C[] = { 0x0000 };
const struct hash_index_t hash7D[] = { 0x0000 };
const struct hash_index_t hash7E[] = { 0x0000 };
const struct hash_index_t hash7F[] = { 0x0000 };
const struct hash_index_t hash80[] = { 0x0000 };
const struct hash_index_t hash81[] = { 0x0000 };
const struct hash_index_t hash82[] = { 0x0000 };
const struct hash_index_t hash83[] = { 0x0000 };
const struct hash_index_t hash84[] = { 0x0000 };
const struct hash_index_t hash85[] = { 0x0000 };
const struct hash_index_t hash86[] = { 0x0000 };
const struct hash_index_t hash87[] = { 0x0000 };
const struct hash_index_t hash88[] = { 0x0000 };
const struct hash_index_t hash89[] = { 0x0000 };
const struct hash_index_t hash8A[] = { 0x0000 };
const struct hash_index_t hash8B[] = { 0x0000 };
const struct hash_index_t hash8C[] = { 0x0000 };
const struct hash_index_t hash8D[] = { 0x0000 };
const struct hash_index_t hash8E[] = { 0x0000 };
const struct hash_index_t hash8F[] = { 0x0000 };
const struct hash_index_t hash90[] = { 0x0000 };
const struct hash_index_t hash91[] = { 0x0000 };
const struct hash_index_t hash92[] = { 0x0000 };
//const struct hash_index_t hash93[] = { 0x0000 };
const struct hash_index_t hash94[] = { 0x0000 };
const struct hash_index_t hash95[] = { 0x0000 };
const struct hash_index_t hash96[] = { 0x0000 };
const struct hash_index_t hash97[] = { 0x0000 };
const struct hash_index_t hash98[] = { 0x0000 };
const struct hash_index_t hash99[] = { 0x0000 };
const struct hash_index_t hash9A[] = { 0x0000 };
const struct hash_index_t hash9B[] = { 0x0000 };
const struct hash_index_t hash9C[] = { 0x0000 };
const struct hash_index_t hash9D[] = { 0x0000 };
const struct hash_index_t hash9E[] = { 0x0000 };
const struct hash_index_t hash9F[] = { 0x0000 };
const struct hash_index_t hashA0[] = { 0x0000 };
const struct hash_index_t hashA1[] = { 0x0000 };
const struct hash_index_t hashA2[] = { 0x0000 };
const struct hash_index_t hashA3[] = { 0x0000 };
const struct hash_index_t hashA4[] = { 0x0000 };
const struct hash_index_t hashA5[] = { 0x0000 };
const struct hash_index_t hashA6[] = { 0x0000 };
const struct hash_index_t hashA7[] = { 0x0000 };
const struct hash_index_t hashA8[] = { 0x0000 };
const struct hash_index_t hashA9[] = { 0x0000 };
const struct hash_index_t hashAA[] = { 0x0000 };
const struct hash_index_t hashAB[] = { 0x0000 };
const struct hash_index_t hashAC[] = { 0x0000 };
const struct hash_index_t hashAD[] = { 0x0000 };
const struct hash_index_t hashAE[] = { 0x0000 };
const struct hash_index_t hashAF[] = { 0x0000 };
const struct hash_index_t hashB0[] = { 0x0000 };
const struct hash_index_t hashB1[] = { 0x0000 };
const struct hash_index_t hashB2[] = { 0x0000 };
const struct hash_index_t hashB3[] = { 0x0000 };
const struct hash_index_t hashB4[] = { 0x0000 };
const struct hash_index_t hashB5[] = { 0x0000 };
const struct hash_index_t hashB6[] = { 0x0000 };
const struct hash_index_t hashB7[] = { 0x0000 };
const struct hash_index_t hashB8[] = { 0x0000 };
const struct hash_index_t hashB9[] = { 0x0000 };
const struct hash_index_t hashBA[] = { 0x0000 };
const struct hash_index_t hashBB[] = { 0x0000 };
const struct hash_index_t hashBC[] = { 0x0000 };
const struct hash_index_t hashBD[] = { 0x0000 };
const struct hash_index_t hashBE[] = { 0x0000 };
const struct hash_index_t hashBF[] = { 0x0000 };
const struct hash_index_t hashC0[] = { 0x0000 };
const struct hash_index_t hashC1[] = { 0x0000 };
const struct hash_index_t hashC2[] = { 0x0000 };
const struct hash_index_t hashC3[] = { 0x0000 };
const struct hash_index_t hashC4[] = { 0x0000 };
const struct hash_index_t hashC5[] = { 0x0000 };
const struct hash_index_t hashC6[] = { 0x0000 };
const struct hash_index_t hashC7[] = { 0x0000 };
const struct hash_index_t hashC8[] = { 0x0000 };
const struct hash_index_t hashC9[] = { 0x0000 };
const struct hash_index_t hashCA[] = { 0x0000 };
const struct hash_index_t hashCB[] = { 0x0000 };
const struct hash_index_t hashCC[] = { 0x0000 };
const struct hash_index_t hashCD[] = { 0x0000 };
const struct hash_index_t hashCE[] = { 0x0000 };
const struct hash_index_t hashCF[] = { 0x0000 };
const struct hash_index_t hashD0[] = { 0x0000 };
const struct hash_index_t hashD1[] = { 0x0000 };
const struct hash_index_t hashD2[] = { 0x0000 };
const struct hash_index_t hashD3[] = { 0x0000 };
const struct hash_index_t hashD4[] = { 0x0000 };
const struct hash_index_t hashD5[] = { 0x0000 };
const struct hash_index_t hashD6[] = { 0x0000 };
const struct hash_index_t hashD7[] = { 0x0000 };
const struct hash_index_t hashD8[] = { 0x0000 };
const struct hash_index_t hashD9[] = { 0x0000 };
const struct hash_index_t hashDA[] = { 0x0000 };
const struct hash_index_t hashDB[] = { 0x0000 };
const struct hash_index_t hashDC[] = { 0x0000 };
const struct hash_index_t hashDD[] = { 0x0000 };
const struct hash_index_t hashDE[] = { 0x0000 };
const struct hash_index_t hashDF[] = { 0x0000 };
const struct hash_index_t hashE0[] = { 0x0000 };
const struct hash_index_t hashE1[] = { 0x0000 };
const struct hash_index_t hashE2[] = { 0x0000 };
const struct hash_index_t hashE3[] = { 0x0000 };
const struct hash_index_t hashE4[] = { 0x0000 };
const struct hash_index_t hashE5[] = { 0x0000 };
const struct hash_index_t hashE6[] = { 0x0000 };
const struct hash_index_t hashE7[] = { 0x0000 };
const struct hash_index_t hashE8[] = { 0x0000 };
const struct hash_index_t hashE9[] = { 0x0000 };
const struct hash_index_t hashEA[] = { 0x0000 };
const struct hash_index_t hashEB[] = { 0x0000 };
const struct hash_index_t hashEC[] = { 0x0000 };
const struct hash_index_t hashED[] = { 0x0000 };
const struct hash_index_t hashEE[] = { 0x0000 };
const struct hash_index_t hashEF[] = { 0x0000 };
const struct hash_index_t hashF0[] = { 0x0000 };
const struct hash_index_t hashF1[] = { 0x0000 };
const struct hash_index_t hashF2[] = { 0x0000 };
const struct hash_index_t hashF3[] = { 0x0000 };
const struct hash_index_t hashF4[] = { 0x0000 };
const struct hash_index_t hashF5[] = { 0x0000 };
const struct hash_index_t hashF6[] = { 0x0000 };
const struct hash_index_t hashF7[] = { 0x0000 };
const struct hash_index_t hashF8[] = { 0x0000 };
const struct hash_index_t hashF9[] = { 0x0000 };
const struct hash_index_t hashFA[] = { 0x0000 };
const struct hash_index_t hashFB[] = { 0x0000 };
const struct hash_index_t hashFC[] = { 0x0000 };
const struct hash_index_t hashFD[] = { 0x0000 };
const struct hash_index_t hashFE[] = { 0x0000 };
const struct hash_index_t hashFF[] = { 0x0000 };
//hash二查树的第一级表；
struct hash_index_t hash_map[16][16]=
{
    { hash00,hash01,hash02,hash03,hash04,hash05,hash06,hash07,hash08,hash09,hash0A,hash0B,hash0C,hash0D,hash0E,hash0F },
    { hash10,hash11,hash12,hash13,hash14,hash15,hash16,hash17,hash18,hash19,hash1A,hash1B,hash1C,hash1D,hash1E,hash1F },
    { hash20,hash21,hash22,hash23,hash24,hash25,hash26,hash27,hash28,hash29,hash2A,hash2B,hash2C,hash2D,hash2E,hash2F },
    { hash30,hash31,hash32,hash33,hash34,hash35,hash36,hash37,hash38,hash39,hash3A,hash3B,hash3C,hash3D,hash3E,hash3F },
    { hash40,hash41,hash42,hash43,hash44,hash45,hash46,hash47,hash48,hash49,hash4A,hash4B,hash4C,hash4D,hash4E,hash4F },
    { hash50,hash51,hash52,hash53,hash54,hash55,hash56,hash57,hash58,hash59,hash5A,hash5B,hash5C,hash5D,hash5E,hash5F },
    { hash60,hash61,hash62,hash63,hash64,hash65,hash66,hash67,hash68,hash69,hash6A,hash6B,hash6C,hash6D,hash6E,hash6F },
    { hash70,hash71,hash72,hash73,hash74,hash75,hash76,hash77,hash78,hash79,hash7A,hash7B,hash7C,hash7D,hash7E,hash7F },
    { hash80,hash81,hash82,hash83,hash84,hash85,hash86,hash87,hash88,hash89,hash8A,hash8B,hash8C,hash8D,hash8E,hash8F },
    { hash90,hash91,hash92,hash93,hash94,hash95,hash96,hash97,hash98,hash99,hash9A,hash9B,hash9C,hash9D,hash9E,hash9F },
    { hashA0,hashA1,hashA2,hashA3,hashA4,hashA5,hashA6,hashA7,hashA8,hashA9,hashAA,hashAB,hashAC,hashAD,hashAE,hashAF },
    { hashB0,hashB1,hashB2,hashB3,hashB4,hashB5,hashB6,hashB7,hashB8,hashB9,hashBA,hashBB,hashBC,hashBD,hashBE,hashBF },
    { hashC0,hashC1,hashC2,hashC3,hashC4,hashC5,hashC6,hashC7,hashC8,hashC9,hashCA,hashCB,hashCC,hashCD,hashCE,hashCF },
    { hashD0,hashD1,hashD2,hashD3,hashD4,hashD5,hashD6,hashD7,hashD8,hashD9,hashDA,hashDB,hashDC,hashDD,hashDE,hashDF },
    { hashE0,hashE1,hashE2,hashE3,hashE4,hashE5,hashE6,hashE7,hashE8,hashE9,hashEA,hashEB,hashEC,hashED,hashEE,hashEF },
    { hashF0,hashF1,hashF2,hashF3,hashF4,hashF5,hashF6,hashF7,hashF8,hashF9,hashFA,hashFB,hashFC,hashFD,hashFE,hashFF },
};

uint16_t hash_find(uint16_t key)
{
    struct hash_index_t t_map,s_map;
    uint8_t k[] = { 0x00, 0x00, 0x00, 0x00 };          //存放输入的K值
    uint8_t i = 0;

    for (i = 0; i < 4; i++)
    {
        k[i] = (uint8_t)(key >> ((3 - i) * 4));       //获取每层节点的键值；
    }

    t_map.map = hash_map[k[0]][k[1]].map;

    if (t_map.map != 0x0000)
    {
        s_map.map = t_map.map[k[2]];
        if (s_map.map != 0x0000)
        {
            return s_map.map[k[3]] ;
        }
    }

    return 0x0000;
}
//**********************************************************************以下是数据表格********************************************************//

enum data_format_t
{
    f_hex,
    f_bcd,
    f_ascii,
    f_float
};
enum data_verific_t
{
    ver_crc,
    ver_odd,
    ver_even
};
enum memory_layout_t
{
    m_isr,
    m_queue,
    m_root,
    m_workers_root,
    m_workers_data,
    m_power_safe,
    m_dev_fifo,
    m_cache,
    m_stack
};
enum decimals_t
{
    unit_0 = 5,     //0位小数，倍率=1
    unit_1 = 6,     //1位小数，倍率=1
    unit_2 = 7,     //2位小数，倍率=1
    unit_3 = 8,     //3位小数，倍率=1
    unit_4 = 9,     //4位小数，倍率=1
    unit_0_K = 2,   //0位小数，倍率=1000
    unit_1_K = 3,   //1位小数，倍率=1000
    unit_2_K = 4,   //2位小数，倍率=1000
    unit_3_K = 5,   //3位小数，倍率=1000
    unit_4_K = 6,   //4位小数，倍率=1000
    unit_1_M = 0,   //1位小数，倍率=1000000
    unit_2_M = 1,   //2位小数，倍率=1000000
    unit_3_M = 2    //3位小数，倍率=1000000
};

enum device_ids_t
{
    device_fram,
    device_eeprom,
    device_flash
};

const struct explorer_t explorer_list[] =
{
    { 0x9300, 0x10000000, 0x0004, ver_crc, f_hex, 0x1000, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom  }, //当前基础总有功电能+
    { 0x9310, 0x10000016, 0x0004, ver_crc, f_hex, 0x1016, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前基础总有功电能-
    { 0x9320, 0x1000002C, 0x0004, ver_crc, f_hex, 0x102C, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前基础总无功电能1
    { 0x9330, 0x10000042, 0x0004, ver_crc, f_hex, 0x1042, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前基础总无功电能2
    { 0x9340, 0x10000058, 0x0004, ver_crc, f_hex, 0x1058, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前基础总无功电能3
    { 0x9350, 0x1000006E, 0x0004, ver_crc, f_hex, 0x106E, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前基础总无功电能4
    { 0x9360, 0x10000084, 0x0004, ver_crc, f_hex, 0x1084, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前基础总视在+
    { 0x9370, 0x1000009A, 0x0004, ver_crc, f_hex, 0x109A, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前基础总视在-
    { 0x9380, 0x100000B0, 0x0004, ver_crc, f_hex, 0x10B0, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前有功总组合+
    { 0x9390, 0x100000C6, 0x0004, ver_crc, f_hex, 0x10C6, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前有功总组合-
    { 0x93A0, 0x100000DC, 0x0004, ver_crc, f_hex, 0x10DC, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前有功总组合总
    { 0x93B0, 0x100000F2, 0x0004, ver_crc, f_hex, 0x10F2, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前无功总组合+
    { 0x93C0, 0x10000108, 0x0004, ver_crc, f_hex, 0x1108, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前无功总组合-
    { 0x93D0, 0x1000011E, 0x0004, ver_crc, f_hex, 0x111E, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前无功总组合总
    { 0x93E0, 0x10000134, 0x0004, ver_crc, f_hex, 0x1134, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前无功总组合
    { 0x93F0, 0x1000014A, 0x0004, ver_crc, f_hex, 0x114A, 0x0004,0x05,unit_1,m_power_safe,0x01,0x00,device_eeprom, }, //当前视在总
};
const uint16_t explorer_list_size = sizeof(explorer_list) / sizeof(explorer_list[0]);


//计算累计和的函数
uint16_t do_sum(uint8_t* buff, uint16_t length)
{
    uint16_t result = 0;                      //赋初值

    while (length--)                          //逐个字节进行累加
    {
        result += *buff++;
    }

    return result;                           //返回结果
}

uint8_t verify_data(uint8_t *buf, uint16_t len)   //数据校验，保证外部读取的数据是正确的
{
    uint16_t checksum = 0xA5A5, rchecksum = 0;

    checksum += do_sum(buf, len);                //先计算校验码
    memcpy((uint8_t*)&rchecksum, buf+len, 0x02);   //获取原有的校验码

    if (checksum != rchecksum)                   
    {
        return false;                            //校验码不相等，返回失败；
    }

    return true;                                 //校验码正确，返回成功；
}

void update_verify(uint8_t* buf, uint16_t len)   //数据校验，保证外部读取的数据是正确的
{
    uint16_t checksum = 0xA5A5;

    checksum += do_sum(buf, len);                  //先计算校验码
    memcpy(buf + len, (uint8_t*)&checksum, 0x02);   //更新校验码
}

//**********************************************************************以下是基本数据操作操作********************************************************//
const struct explorer_t *look_up(struct instance_t* instance, uint16_t id)
{
    uint16_t s_index;

    s_index = hash_find(id);

    if ((0x0000 != s_index) && (s_index <= explorer_list_size))
    {
        return &explorer_list[s_index];
    }

    return NULL;
}

const struct hw_def_t* match_device(const struct recipe_t* recipe, uint16_t device_id)
{
    uint8_t i;
    uint16_t hw_def_n;
    const struct hw_def_t* hw_def;

    if (NULL != recipe)
    {
        hw_def_n = recipe->hw_def_n;
        hw_def = recipe->hw_def;

        if (NULL != hw_def)
        {
            for (i = 0; i < hw_def_n; i++)
            {
                if (device_id == hw_def[i].id)
                {
                    return &hw_def[i];
                }
            }
        }
    }
    return NULL;
}


uint8_t simple_proces_get(struct instance_t* instance,const struct explorer_t* exp, struct driver_t* ops,uint8_t *buf)
{
    uint8_t ret = false;

    if(0 != exp->r_addr)                                  //获取ID的信息如果有RAM区的，先读RAM区的数据；
    {      
        if(true == verify_data(exp->r_addr, exp->r_len))  //校验数据；
        {
            memcpy(buf, exp->r_addr, exp->r_len);         //直接读取RAM数据；
            return true;                                  //数据读取成功；
        }
    }

    if ((NULL != ops->read_dev)&&(0x00 != exp->d_addr))   
    {
        if (true == ops->read_dev(instance, exp->d_addr, buf, exp->d_len))  //获取外部存储器的数据
        {
            ret = verify_data(exp->d_addr, exp->d_len);       //校验数据是否正确；

            if((true == ret)&&(0 != exp->r_addr))
            {
                memcpy(exp->r_addr, buf, exp->r_len);       //直接更新RAM数据；
            }
        }
    }
        
    return ret;
}

uint8_t simple_proces_set(struct instance_t* instance,const struct explorer_t* exp, struct driver_t* ops, uint8_t* buf)
{
    uint8_t ret = true;

    update_verify(buf, exp->d_len);                        //先计算校验码
    if (0 != exp->r_addr)                                  //有RAM区，需要先更新ram的数据；
    {
        memcpy(exp->r_addr, buf, exp->d_len);              //直接读取RAM数据；
    }

    if ((NULL != ops->write_dev) && (0x00 != exp->d_addr))     //ram更新完成后，更新外部存储区；
    {
        ret =  ops->write_dev(instance, exp->d_addr, buf, exp->d_len);
    }

    return ret;
}
//**********************************************************************以下是数据块操作********************************************************//
//批量存储的数据的查找，返回符合条件的条数；
uint8_t ldw_search_box(struct instance_t* instance, uint16_t id, struct search_info_t* info)
{
    const struct explorer_t* this = NULL;
    struct batch_info_t* batch_info = NULL;
    uint32_t s_time = 0x00000000, e_time = 0x00000000;
    uint32_t current_time = 0x00000000;
    uint8_t ret = false;

    memset((uint8_t*)&info->result, 0x00, sizeof(info->result));
    this = look_up(instance, id);      //先查找ID是否存在；
    if (this != NULL)
    {
        batch_info = this->r_addr;
        if (batch_info != NULL)
        {
            //s_time = (info->start_rtc > batch_info->start_time) ? info->start_rtc : batch_info->start_time;
            //e_time = (info->end_rtc > current_time) ? current_time : info->end_rtc;

            //把不规则的时间去掉
            if (batch_info->record_len != 0x0000)
            {
                s_time = (s_time / batch_info->record_len) * batch_info->record_len;
                e_time = (e_time / batch_info->record_len) * batch_info->record_len;

                if ((e_time >= s_time) && (batch_info->record_period != 0x0000))
                {
                    info->result.pass_num = (e_time - s_time) / batch_info->record_period + 1;
                    info->result.start_flag = (s_time - batch_info->start_time) / batch_info->record_period + 1;
                    ret = true;
                }
            }
        }
    }

    return ret;
}

uint8_t batch_proces_get(struct instance_t* instance, const struct explorer_t* exp, struct driver_t* ops, uint8_t* buf, struct search_info_t* info)
{
    struct batch_info_t* batch_info = NULL;
    uint32_t this_addr = 0x00000000;
    uint8_t ret = false;

    batch_info = exp->d_addr;     //获取内存地址；
    ret = verify_data(batch_info, sizeof(struct batch_info_t) - 0x02);
    if ((info->result.pass_num != 0x0000) && (false != ret))
    {
        this_addr = batch_info->start_addr + (batch_info->record_len * info->result.start_flag) + (batch_info->record_len * info->result.read_ptr);
        if (NULL != ops->read_dev)
        {
            ret = ops->read_dev(this_addr, buf, batch_info->record_len, NULL);
            info->result.read_ptr++;
        }
    }

    return ret;
}

uint8_t batch_proces_set(struct instance_t* instance, const struct explorer_t* exp, struct driver_t* ops, uint8_t* buf, struct set_info_t *info)
{
    struct batch_info_t* batch_info = NULL;
    uint32_t this_addr = 0x00000000;
    uint32_t current_time = 0x00000000;  //需要通过函数来计算当前的分钟数；
    uint8_t ret = false,flag = 0x00;
    uint32_t s_max_num = 0x00000000;

    if (NULL != ops->write_dev)  
    {
        batch_info = exp->d_addr;     //获取内存地址；
        if (0x0000 != batch_info)
        {
            ret = verify_data(batch_info, sizeof(struct batch_info_t) - 0x02);

            if (ret != true)   //内存数据无效；
            {
                //外部存储器获取，这里还未完全想明白，可以能要用到模拟铁电 CHENGBO;
                //如果外部存储器获取也失败，说明当前为第一次存储数据；
                memset((uint8_t*)batch_info, 0x00, sizeof(struct batch_info_t));
                if ((info->record_len == 0x0000) || (info->record_period == 0x0000))
                {
                    return false;   //说明数据是无效的；
                }

               // batch_info->start_time = info->record_time / info->record_period * info->record_period;
                batch_info->max_addr = exp->d_addr + exp->d_len * 1024;
                batch_info->start_addr = exp->d_addr;
                batch_info->record_len = info->record_len;
                batch_info->record_period = info->record_period;
                update_verify((uint8_t*)batch_info, sizeof(struct batch_info_t) - 0x02);
                //更新外部存储器的内容      CHENGBO；
            }

            if ((batch_info->record_len == info->record_len) && (batch_info->record_period == info->record_period)
                && (0x0000 != batch_info->record_len) && (0x0000 != batch_info->record_period))  //说明基本信息是正确的
            {
                this_addr = exp->d_addr + ((current_time - batch_info->start_time) / batch_info->record_period) * batch_info->record_len;
                s_max_num = ((exp->d_len - 0x01) * 1024) / batch_info->record_len;

                while (this_addr > batch_info->max_addr)  //处理翻转的情况；
                {
                    this_addr = this_addr - batch_info->max_addr;
                    flag = 0x01;
                }

                if (flag == 0x01)
                {
                    flag = 0x00;
                    this_addr = this_addr + exp->d_addr;   //更新当前要操作的地址；
                    batch_info->start_addr = this_addr + 1024 + batch_info->record_len;//更新要读的地址；
                }

                ret = ops->write_dev(this_addr, buf, batch_info->record_len, NULL);
                batch_info->record_num++;

                if (batch_info->record_num > s_max_num)
                {
                    batch_info->record_num = s_max_num;
                }
                update_verify((uint8_t*)batch_info, sizeof(struct batch_info_t) - 0x02);
                //更新外部存储器的内容      CHENGBO；
            }
        }
    }
}

uint8_t ldw_get(struct instance_t* instance, uint16_t id, uint8_t* buf, struct search_info_t* info)   //对外接口之一数据获取；
{
    const struct explorer_t* this = NULL;
    struct hw_def_t* def = NULL;
    struct driver_t* ops = NULL;
    uint8_t ret = false;

    this = look_up(instance, id);      //先查找ID是否存在；
    if (this != NULL)
    {
        def = match_device(instance->recipe, this->device_id);      //获取设备的ID表格

        if (NULL != def)
        {
            ops = def->driver;
            if (ops != NULL)
            {
                if (this->proces_mode == 0x01)//获取操作模式；
                {
                    ret = simple_proces_get(instance, this, ops, buf);
                }

                if (this->proces_mode == 0x02)//获取操作模式；
                {
                    ret = batch_proces_get(instance, this, ops, buf, info);
                }
            }
        }
    }
    return ret;
}

uint8_t ldw_set(struct instance_t* instance, uint16_t id, uint8_t* buf, struct set_info_t* info)   //对外接口之一数据获取；
{
    const struct explorer_t* this = NULL;
    struct hw_def_t* def = NULL;
    struct driver_t* ops = NULL;
    uint8_t ret = false;

    this = look_up(instance, id);      //先查找ID是否存在；
    if (this != NULL)
    {
        def = match_device(instance->recipe, this->device_id);      //获取设备的ID表格
        if (def != NULL)
        {
            ops = def->driver;
            if (ops != NULL)
            {
                if (this->proces_mode == 0x01)//获取操作模式；
                {
                    ret = simple_proces_set(instance, this, ops, buf);
                }

                if (this->proces_mode == 0x02)//获取操作模式；
                {
                    ret = batch_proces_set(instance, this, ops, buf, info);
                }
            }
        }
    }
    return ret;
}