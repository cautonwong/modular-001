/**
 * @defgroup    tariff_cfg Tariff Cfg
 * @ingroup     cfg
 * @see     tou
 * @brief       tariff配置.
 *
 *
 * @{
 * @file
 */
#include "hfp_system.h"
#include "fs.h"
#include "tou.h"
#include "tariff_api.h"
#include "fs_public.h"
#include "lib_calclib.h"


/** 费率时段外部参数 */
const struct tariff_para_t CODE tariff_para[] =
{
    0x0E,                /**< 最大年时区数 */
    0x08,                /**< 最大日时段表数 */
    0x0E,                /**< 最大日时段数 */
    0x04,                /**< 最大费率数 */
    0xFE,                /**< 最大公共假日数 */
    0x3200,              /**< 年时区数 */
    0x3201,              /**< 日时段表数 */
    0x3202,              /**< 日时段数 */
    0x3203,              /**< 费率数 */
    0x3204,              /**< 公共假日数 */
    0x3220,              /**< 当前套时区表 */
    0x3230,              /**< 备用套时区表 */
    0x3240,              /**< 当前套日时段表 */
    0x3250,              /**< 备用套日时段表 */
    0x3300,              /**< 公共假日表 */
    0x2176,              /**< 周休日特征字 */
    0x2177,              /**< 周休日日时段表编号 */
};

const struct vice_tariff_tim_t CODE vice_start_para[] =
{
    { 0x3210, 0x3220, 0x3230, 0x01 }, /**< 两套时区表切换时间 */
    { 0x3211, 0x3240, 0x3250, 0x08 }, /**< 两套时段表切换时间 */
};

uint8 vice_tariff_para_num = sizeof(vice_start_para) / sizeof(vice_start_para[0]);

const uint16 CODE tariff_notify_fsid[] = {0xC500,0xC503,0x2176,0x2177,0x3200,0x3201,0x3202,0x3203,0x3204,0x3210,0x3211,0};


struct time_zone_id
{
	uint16 start_id;
	uint16 end_id;
	uint16 time_zone_fsid;
};

const struct time_zone_id CODE find_time_zone_id[] =
{
		{ 0x7680,0x768D,0x3220 },
		{ 0x768E,0x769B,0x3230 },
		{ 0x7600,0x760D,0x3240 },
		{ 0x760E,0x761B,0x3241 },
		{ 0x761C,0x7629,0x3242 },
	    { 0x762A,0x7637,0x3243 },
		{ 0x7638,0x7645,0x3250 },
		{ 0x7646,0x7653,0x3251 },
	    { 0x7654,0x7661,0x3252 },
		{ 0x7662,0x766F,0x3253 },
};
#define array_count(arr)           (sizeof arr / sizeof arr[0])
uint8 time_zone_table_read(uint32 id, uint8 *dest, uint16 len, const struct hfp_device_t CODE *device)
{
    uint8 i;
	uint8 hex_buf[42];
	uint8 bcb_buf[42];
	uint16 offset = 0;
	hfp_memset(hex_buf,0x00,42);	
	hfp_memset(bcb_buf,0x00,42);
	for (i = 0; i < array_count(find_time_zone_id); i++)
	{
			if (((uint16)id >= find_time_zone_id[i].start_id) && ((uint16)id <= find_time_zone_id[i].end_id) )
			{
					offset=(uint16)id - find_time_zone_id[i].start_id;
				    if (false == hfp_read(find_time_zone_id[i].time_zone_fsid, hex_buf, 42, NULL))
				    {
						return false;
					}
					break;
			}
	}
   
	hfp_hex_to_bcd_array(hex_buf, bcb_buf,42 );
	hfp_memswapcpy(dest, (uint8 *)&bcb_buf[offset*3], len);
	return true;
}

/**@}*/
