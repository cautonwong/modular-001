#include "nidavellir.h"
#include "fr_698.h"
#include "port.h"

extern unsigned char BYTE_BCD( unsigned char HEX );
extern void HEX_BCD( unsigned char* BCD, unsigned long HEX );

typedef struct 
{
    uint32_t    s_index;    //地址
    uint16_t    oad;        
    uint8_t     offset;     //偏移
    uint8_t     len;        //长度
}general_set4;

const general_set4  set4300_table[] = 
{
     Device_descriptor,   0x0200,   0x02,   32,//设备描述符
    
     Factory_No,          0x0301,   0x02,    4, //厂家代码
     Version_SoftWare,    0x0302,   0x02,    4, //软件版本
     Date_SoftWare,       0x0303,   0x02,    6, //软件版本日期
     Version_HardWare,    0x0304,   0x02,    4, //硬件版本
     Date_HardWare,       0x0305,   0x02,    6, //硬件版本日期
     Extend_Info,         0x0306,   0x02,    8, //扩展信息
    
     Produced_Date,       0x0400,   0x02,    7, //生产日期
//     Subset_List,       0x0500,  0x05,    2, //子设备列表(num+2*oi)
     Protocol_List,       0x0601,   0x02,   20, //协议列表
     FollowReport_en,     0x0700,   0x01,    1, //允许跟随上报
     ActiveReport_en,     0x0800,   0x01,    1, //允许主动上报
     Comm_Master_en,      0x0900,   0x01,    1, //允许与主站通讯
     Report_channel,      0x0A00,   0x03,    4, //上报通道
};

const general_set4  general_set4_talble[] =
{
     Comm_Number,               0x4001,     0x02,   6,//表地址
     Meter_Number,              0x4002,     0x02,   6,//表号
     User_Number,               0x4003,     0x02,   6,//客户编号
//     Meter_Location,            0x4004,     0x01,   27,//设备地理位置
     SeasonTable_Changetime,    0x4008,     0x01,   7,//两套时区切换时间
     DayTable_Changetime,       0x4009,     0x01,   7,//两套时段切换时间
     SecretKey_Num,             0x400F,     0x01,   1,//秘钥总条数
     Meter_Num,                 0x4010,     0x01,   1,//计量元件个数
     Weekend,                   0x4012,     0x01,   1,//周休日特征字
     WeekendFee,                0x4013,     0x01,   1,//周休日采用费率号
     Meter_CT,                  0x401C,     0x01,   4,//CT
     Meter_PT,                  0x401D,     0x01,   4,//PT
     Pulse_Width,               0x4102,     0x01,   1,//脉冲宽度
     ASCII_AssetsMan,           0x4103,     0x01,   32,//资产管理编码
     ASCII_Un,                  0x4104,     0x01,   6,//额定电压
     ASCII_Ib,                  0x4105,     0x01,   6,//额定电流
     ASCII_Imax,                0x4106,     0x01,   6,//最大电流
     ASCII_AccuracyClass1,      0x4107,     0x01,   4,//有功准确度等级
     Meter_Const1,              0x4109,     0x01,   4,//有功脉冲常数
     ASCII_MeterType,           0x410B,     0x01,   32,//电能表型号
     MeterRunWord,              0x4110,     0x01,   1,//电能表运行特征字
     SoftWareNo,                0x4111,     0x01,   16,//软件备案号
     Measure_Mode,              0x4112,     0x01,   1,//有功组合特征字
     Meter_Password,            0x4401,     0x01,   8,//应用连接密码
};
#define		set4num			sizeof(general_set4_talble)/sizeof(general_set4)

const unsigned int  DayTab[13] = { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

/***********************************************************************/
/*  函数名称：BCD_BYTE                                                 */
/*  函数功能：将单字节BCD码转换为单字节HEX码                           */
/*  输入参数：BCD：单字节BCD码数据                                     */
/*  输出参数：单字节HEX码数据                                          */
/***********************************************************************/
unsigned char BCD_BYTE( unsigned char BCD )
{
     return( ((BCD>>4) & 0x0F)*10 + (BCD & 0x0F) );
}

/***********************************************************************/
/*  函数名称：GetTotalDay                                              */
/*  函数功能：根据年月日计算总天数                                     */
/*  输入参数：Addr：年月日数据地址                                     */
/*  输出参数：总天数                                                   */
/***********************************************************************/
unsigned int GetTotalDay( unsigned char* Addr )
{
  	unsigned int Day,Mon,Year;

	Year= BCD_BYTE( *(Addr+2) );
	Mon = BCD_BYTE( *(Addr+1) );
	Day = BCD_BYTE( *Addr );
	Day = Day + DayTab[Mon] + Year*365 + (Year>>2) + 1;
	if( ((Year&0x03) == 0) && (Mon <= 2) )
		Day--;
   	return Day;
}

/***********************************************************************/
/*  函数名称：GetWeek                                                  */
/*  函数功能：根据年月日计算周                                         */
/*  输入参数：Addr：年月日数据地址                                     */
/*  输出参数：周                                                       */
/***********************************************************************/
unsigned char GetWeek( unsigned char* Addr )
{
	unsigned char Week;

	Week = (unsigned char)(( GetTotalDay( Addr ) + 5 ) % 7);
	return Week;
}

int general_set_4_para(struct instance_t* instance, uint32_t id, uint8_t* buf)
{
//    struct mbase_control_block_t* mbase_cb;
    uint16_t oad,i;
    uint8_t attribute;
    uint8_t* p = buf;
    uint32_t addr=0;
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8);
    
    for(i=0; i<set4num; i++)
    {
        if(oad == general_set4_talble[i].oad) 
        {
            if(attribute == 0x02)
            {
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + general_set4_talble[i].s_index);
                nd_set_index(instance, addr, (p + general_set4_talble[i].offset), general_set4_talble[i].len); 
                return (general_set4_talble[i].offset + general_set4_talble[i].len);
            }
            else
            {
                return -1;
            }
        }
    }
    return -1;
}

int set4300(struct instance_t* instance, uint32_t id, uint8_t* buf)
{
    struct mbase_control_block_t* mbase_cb;
    uint8_t i;
    uint8_t* p = buf;
    uint32_t addr=0;
    uint16_t idl = 0;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    idl = (uint16_t)(id & 0x0000FFFF);
    
    for(i=0; i<sizeof(set4300_table)/sizeof(general_set4); i++)
    {
        if(idl == set4300_table[i].oad)
        {
            p = p + set4300_table[i].offset;
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + set4300_table[i].s_index);
            nd_set_index(instance, addr, p, set4300_table[i].len); 
            p = p + set4300_table[i].len;
            return (p-buf);
        }
    }
    return -1;
}

int SET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint16_t oad;
    uint8_t index,attribute,i,len=0,num,buffer[42]={0},num_hourtab=0,j;
    uint8_t* p = buf;
    uint32_t s_index = 0x00,addr=0;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    if(general_set_4_para(instance, id, buf) != -1)
    {
        return 1;
    }
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8);
    
    switch (oad)
    {
        case 0x4000://时间
            if(attribute == 0x02)
            {
                uint8_t settimebuf[8]={0};
                p++;
                instance->time.tm_year = 0;
                mem_swap_cpy((uint8_t*)&instance->time.tm_year, p, 2);
                p = p + 2;
                instance->time.tm_mon = *p++;//
                instance->time.tm_mday = *p++;//
                instance->time.tm_hour = *p++;
                instance->time.tm_min = *p++;
                instance->time.tm_sec = *p++;
                //秒、分、时、日、月、年、周
                settimebuf[0] = BYTE_BCD((uint8_t)instance->time.tm_sec);
                settimebuf[1] = BYTE_BCD((uint8_t)instance->time.tm_min);
                settimebuf[2] = BYTE_BCD((uint8_t)instance->time.tm_hour);
                settimebuf[3] = BYTE_BCD((uint8_t)instance->time.tm_mday);
                settimebuf[4] = BYTE_BCD((uint8_t)instance->time.tm_mon);
                HEX_BCD( &settimebuf[5], instance->time.tm_year );//年
                settimebuf[6] = GetWeek(&settimebuf[3]);
                write_mcu_rtc(0, settimebuf, 7);
                return (p-buf);
            }
            break;
        case 0x4004://地理位置
            if(attribute != 0x02)   return -1;
            if(index == 0)
            {
                p = p + 2;
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location1);
                nd_set_index(instance, addr, p, 10); 
                p = p + 10;
                
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location2);
                nd_set_index(instance, addr, p, 10); 
                p = p + 10;
                
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location3);
                nd_set_index(instance, addr, p, 5); 
                p = p + 5;
                
                return (p-buf);
            }
            else if(index == 0x01)
            {
                s_index = Meter_Location1;
                len = 10;
            }
            else if(index == 0x02)
            {
                s_index = Meter_Location2;
                len = 10;
            }
            else if(index == 0x03)
            {
                s_index = Meter_Location3;
                len = 5;
            }
            else 
            {
                return -1;
            }
            break;
        case 0x4007://LCD参数
            if(attribute == 0x02)
            {
                uint32_t idbuf[] = {
                (uint32_t)&mbase_cb->lcd_poweron_all_dis_time,            //上电全显时间
                (uint32_t)&mbase_cb->backlight_key_time,                  //背光点亮时间
                (uint32_t)&mbase_cb->lcd_checkdis_keep_sec,               //显示查看背光点亮时间
                (uint32_t)&mbase_cb->lcd_nopower_key_keep_sec,            //无电按键驻留时间
                (uint32_t)&mbase_cb->energy_point,                        //电能小数显示位
                (uint32_t)&mbase_cb->power_point,                         //功率小数显示位
                (uint32_t)&mbase_cb->lcd_12_mean,                         //液晶12字样意义
                };
                uint8_t lens[] = {1, 2, 2, 2, 1, 1, 1};
                uint8_t offset[] = {0, 1, 3, 5, 7, 8, 9};
                if(index == 0)
                {
                    p = p + 2;
                    for(i=0; i<7; i++)
                    {
                        p++;
                        mem_swap_cpy((uint8_t*)idbuf[i], p, lens[i]);
                        p = p + lens[i];
                    }
                    
                    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                    offsetof(struct mbase_control_block_t, backlight_key_time));
                    uint16_t s_len = offsetof(struct mbase_control_block_t, lcd_checkdis_all_keep_sec) - offsetof(struct mbase_control_block_t, backlight_key_time);
                    nd_set_offset(instance, s_index, &mbase_cb->backlight_key_time, s_len);
                    
                    return (p-buf);
                }
                else if(index < 8)
                {
                    p++;
                    mem_swap_cpy((uint8_t*)idbuf[index-1], p, lens[index-1]);
                    
                    s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                    offsetof(struct mbase_control_block_t, lcd_poweron_all_dis_time) + offset[index-1]);
                    nd_set_offset(instance, s_index, p, lens[index-1]);
                    
                    p = p + lens[index-1];
                    return (p-buf);
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
            break;
        case 0x400C://时区时段数
            if(index > 5)   return -1;
            if(index == 0)
            {
                p = p + 2;
                for(i=0;i<5;i++)
                {
                    p++;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + i);
                    nd_set_index(instance, s_index, p, 1); 
                    p++;
                }
                return (p-buf);
            }
            else
            {
                p++;
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + (index - 1));
                nd_set_index(instance, s_index, p, 1); 
                p++;
                return (p-buf);
            }
            break;
        case 0x4011://公共假日表
            if((index == 0) || (index > 10))    return -1;
            p = p + 3;
            memcpy(buffer, p, 5);
            p = p + 6;
            buffer[5] = *p++;
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + HoliDay_Table + (index-1));
            nd_set_index(instance, s_index, buffer, 6); 
            return (p-buf);
            break;
        case 0x4014://当前套时区表
        case 0x4015://备用套时区表
            if(index > 14)    return -1;
            if(oad == 0x4014)   
                addr = Season_Table0;
            else 
                addr = Season_Table1;
            if(index == 0)
            {
                num = *p;
                if(num>14) num = 14;
                p = p + 2;
                for(i=0; i<num; i++)
                {
                    p = p + 3;
                    buffer[0] = *p++;
                    p++;
                    buffer[1] = *p++;
                    p++;
                    buffer[2] = *p++;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + (index-1));
                    nd_set_index(instance, s_index, buffer, 3);
                }
                return (p-buf);
            }
            else
            {
                p = p + 3;
                buffer[0] = *p++;
                p++;
                buffer[1] = *p++;
                p++;
                buffer[2] = *p++;
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + (index-1));
                nd_set_index(instance, s_index, buffer, 3);
                return (p-buf);
            }
            break;
        case 0x4016://当前套日时段表
        case 0x4017://备用套日时段表
            if(index > 0x08)    return -1;
            if(oad == 0x4016)
                addr = Day_Table00;
            else
                addr = Day_Table10;
            if(index == 0)
            {
                p++;
                num = *p++;//
                for(j=0;j<num;j++)
                {
                    p++;
                    num_hourtab = *p++; //日时段数，每日切换数
                    for(i=0; i<num_hourtab; i++)
                    {
                        p = p + 3;
                        buffer[0+i*3] = *p++;
                        p++;
                        buffer[1+i*3] = *p++;
                        p++;
                        buffer[2+i*3] = *p++;
                    }
                    
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + j);
                    nd_set_index(instance, s_index, buffer, 3*num_hourtab);
                }
                return (p-buf);
            }
            else
            {
                p++;
                num_hourtab = *p++; //日时段数，每日切换数
                for(i=0; i<num_hourtab; i++)
                {
                    p = p + 3;
                    buffer[0+i*3] = *p++;
                    p++;
                    buffer[1+i*3] = *p++;
                    p++;
                    buffer[2+i*3] = *p++;
                }
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + (index - 1));
                nd_set_index(instance, s_index, buffer, 3*num_hourtab);
                return (p-buf);
            }
            break;
        
        case 0x4116://结算日
            if(index == 0)
            {
                if(*(p+1) > 0x03)    return -1;
                num = *(p+1);
                p = p + 2;
                for(i=0;i<num;i++)
                {
                    p = p + 2;
                    p++;//tag
                    buffer[0] = *p++;
                    p++;//tag
                    buffer[1] = *p++;
                    
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
                    nd_set_index(instance, s_index, buffer, 2); 
                    
                    memset(buffer, 0xaa, 2);
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
                    nd_get_index(instance, s_index, buffer, 2);
                }
                return (p-buf);
            }
            else if(index < 4)
            {
                p = p + 2;
                p++;//tag
                buffer[0] = *p++;
                p++;//tag
                buffer[1] = *p++;
                
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + (index - 1));
                nd_set_index(instance, s_index, buffer, 2);
                return (p-buf);
            }
            else
                return -1;
            break;
        case 0x4300://设备信息
            return set4300(instance, id, buf);
        default:
            break;
    }
    if(len != 0)
    {
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + s_index);
        nd_set_index(instance, addr, p, len); 
        p = p + len;
    }
    return (p-buf);
}

/*
struct inis_para_t 
{
    uint8_t Device_descriptor[8];           979 //设备描述符
    uint8_t Factory_No[8];                  980 //厂家代码
    uint8_t Version_SoftWare[2];            981 //软件版本
      uint32_t xx[];
}


const struct inis_para_t inis_para =
{
  {0,0,0,},  //设备描述符
  {},  //厂家代码
  {},  //软件版本
}
*/
