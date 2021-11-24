#ifndef __BOOT_H__
#define __BOOT_H__

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef unsigned int       size_t;

//测试缓冲区某位是否为1
#define	is_bitmap(a,x)	((a)[(x)/8] & (1<<((x)%8)))

//设置缓冲区某位为1
#define	set_bitmap(a,x)	((a)[(x)/8] |= (1<<((x)%8)))



//F0010300 文件传输状态  get
#define  status_ota_oad F0010300
struct status_ota_t
{
    uint16_t statu_1;
    uint16_t statu_2;
};

//F0010400 读取位图信息  get
#define  map_ota_oad  F0010400
struct map_ota_t
{
    uint32_t bit_num;
    uint8_t bitmap[512];
};


//F0010700 启动传输      action
#define start_ota_oad
struct start_ota_t
{
    uint32_t bin_ver; //版本信息    
    uint32_t bin_size;//文件大小 double-long-unsigned，
    uint32_t block_size;//128,256,512,1024,2048
    uint32_t type;//0-全包,1-定点,2-差分
    uint8_t  check_VAL[16];//校验值
};


//F0010800 传输中        action
#define download_ota_oad F0010800
struct download_ota_t
{
    uint32_t block_no;      //块序号
    uint32_t block_len;     //块长度
    uint8_t* block_data;    //块数据
};


#endif
