#ifndef __BOOT_H__
#define __BOOT_H__

#include "types.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>


//测试缓冲区某位是否为1
#define	is_bitmap(a,x)	((a)[(x)/8] & (1<<((x)%8)))

//设置缓冲区某位为1
#define	set_bitmap(a,x)	((a)[(x)/8] |= (1<<((x)%8)))



//F0010300 文件传输状态  get
#define  get_status_ota_oad  0xF0010300
struct status_ota_t
{
    uint16_t statu_1;
    uint16_t statu_2;
};

//F0010400 读取位图信息  get
#define  get_map_ota_oad  0xF0010400
struct map_ota_t
{
    uint32_t bit_num;
    uint8_t bitmap[512];
};


//F0010700 启动传输      action
#define start_ota_oad  0xF0010700
struct start_ota_t
{
    uint32_t bin_ver; //版本信息    
    uint32_t bin_size;//文件大小 double-long-unsigned，
    uint32_t block_size;//128,256,512,1024,2048
    uint32_t type;//0x38de82ac-全包,0x9267becc-定点,0xa8e62b0c-差分
    uint8_t  check_VAL[16];//校验值
};


//F0010800 传输中        action
#define download_ota_oad 0xF0010800
struct download_ota_t
{
    uint32_t block_no;      //块序号
    uint32_t block_len;     //块长度
    uint8_t* block_data;    //块数据
};


#endif
