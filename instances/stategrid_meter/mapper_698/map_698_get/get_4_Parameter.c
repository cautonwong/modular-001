#include "nidavellir.h"
#include "fr_698.h"

const uint8_t messagebuf1[8] = {0xff,0xff,0xff,0xff,0xfc, 0x00, 0x00, 0x00};
const uint8_t messagebuf2[16] = {0xff,0xfe,0xc4,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
typedef struct 
{
    uint32_t    s_index;    //地址
    uint16_t    oad;        
    uint8_t     tag;        //
    uint8_t     len;        //长度
}general_get4;

const general_get4  get4300_table[] = 
{
     Device_descriptor,   0x0200,   TYPE_visible_string,   32,//设备描述符
    
     Factory_No,          0x0301,   TYPE_visible_string,    4, //厂家代码
     Version_SoftWare,    0x0302,   TYPE_visible_string,    4, //软件版本
     Date_SoftWare,       0x0303,   TYPE_visible_string,    6, //软件版本日期
     Version_HardWare,    0x0304,   TYPE_visible_string,    4, //硬件版本
     Date_HardWare,       0x0305,   TYPE_visible_string,    6, //硬件版本日期
     Extend_Info,         0x0306,   TYPE_visible_string,    8, //扩展信息
    
     Produced_Date,       0x0400,   TYPE_date_time_s,       7, //生产日期
//     Subset_List,       0x0500,  0x05,    2, //子设备列表(num+2*oi)
     Protocol_List,       0x0600,   TYPE_visible_string,    20, //协议列表
     Protocol_List,       0x0601,   TYPE_array,             20, //协议列表
     FollowReport_en,     0x0700,   TYPE_bool,              1, //允许跟随上报
     ActiveReport_en,     0x0800,   TYPE_bool,              1, //允许主动上报
     Comm_Master_en,      0x0900,   TYPE_bool,              1, //允许与主站通讯
};


const general_get4  general_get4_talble[] =
{
     //Comm_Number,               0x4001,     TYPE_octet_string,      6,//表地址
     User_Number,               0x4003,     TYPE_octet_string,      6,//客户编号
     SeasonTable_Changetime,    0x4008,     TYPE_date_time_s,       7,//两套时区切换时间
     DayTable_Changetime,       0x4009,     TYPE_date_time_s,       7,//两套时段切换时间
     SecretKey_Num,             0x400F,     TYPE_unsigned,          1,//秘钥总条数
     Meter_Num,                 0x4010,     TYPE_unsigned,          1,//计量元件个数
     Weekend,                   0x4012,     TYPE_bit_string,        1,//周休日特征字
     WeekendFee,                0x4013,     TYPE_unsigned,          1,//周休日采用费率号
     Meter_CT,                  0x401C,     TYPE_double_long_unsigned,   4,//CT
     Meter_PT,                  0x401D,     TYPE_double_long_unsigned,   4,//PT
     Pulse_Width,               0x4102,     TYPE_unsigned,          1,//脉冲宽度
     ASCII_AssetsMan,           0x4103,     TYPE_visible_string,    32,//资产管理编码
     ASCII_Un,                  0x4104,     TYPE_visible_string,    6,//额定电压
     ASCII_Ib,                  0x4105,     TYPE_visible_string,    6,//额定电流
     ASCII_Imax,                0x4106,     TYPE_visible_string,    6,//最大电流
     ASCII_AccuracyClass1,      0x4107,     TYPE_visible_string,    4,//有功准确度等级
     Meter_Const1,              0x4109,     TYPE_double_long_unsigned,   4,//有功脉冲常数
     ASCII_MeterType,           0x410B,     TYPE_visible_string,    32,//电能表型号
     MeterRunWord,              0x4110,     TYPE_bit_string,        1,//电能表运行特征字
     SoftWareNo,                0x4111,     TYPE_visible_string,    16,//软件备案号
     Measure_Mode,              0x4112,     TYPE_bit_string,        1,//有功组合特征字
     Meter_Password,            0x4401,     TYPE_visible_string,    8,//应用连接密码
};
#define		get4num			sizeof(general_get4_talble)/sizeof(general_get4)


int get4300_para(struct instance_t* instance, uint32_t id, uint8_t* buf)
{
    uint16_t idl=0;
    uint8_t i, tmpbuf[13] = {0}, num;
    uint8_t* p = buf;
    uint32_t ret, addr = 0;
    
    idl = (uint16_t)(id & 0x0000FFFF);
    
    for(i=0; i< sizeof(get4300_table)/sizeof(general_get4); i++)
    {
        if(idl == get4300_table[i].oad)
        {
            if((idl == 0x0600) || (idl == 0x0601))
            {
                *p++ = TYPE_array;
                *p++ = 1;
            }
            *p++ = get4300_table[i].tag;
            if(get4300_table[i].tag == TYPE_visible_string)
            {
                *p++ = get4300_table[i].len;
            }
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + get4300_table[i].s_index);
            nd_get_index(instance, addr, p, get4300_table[i].len); 
            p = p + get4300_table[i].len;
            return (p-buf);
        }
    }
    if(idl == 0x0300)//版本信息
    {
        *p++ = TYPE_structure;
        *p++ = 6;
        for(i=0; i<6; i++)
        {
            *p++ = TYPE_visible_string;
            *p++ = get4300_table[1+i].len;
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + get4300_table[1+i].s_index);
            nd_get_index(instance, addr, p, get4300_table[1+i].len); 
            p = p + get4300_table[1+i].len;
        }
        return (p-buf);
    }
    else if(idl == 0x0500)//子设备列表
    {
        *p++ = TYPE_array;
        *p++ = 0;
        return (p-buf);
    }
    else if(idl == 0x0A00)//上报通道
    {
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Report_channel);
        ret = nd_get_index(instance, addr, tmpbuf, 13); 
        num = tmpbuf[0];
        if(ret < 0)
        {
            num = 0;
        }
        
        *p++ = TYPE_array;
        *p++ = num;
        
        for (i = 0; i < num; i++)
        {
            *p++ = TYPE_OAD;
            mem_swap_cpy(p, &tmpbuf[4*i+1], 4);
            p += 4;
        }
        return (p-buf);
    }
    
    return -1;
}


int general_get_4_para(struct instance_t* instance, uint32_t id, uint8_t* buf)
{
//    struct mbase_control_block_t* mbase_cb;
    uint16_t oad,i;
    uint8_t attribute;
    uint8_t* p = buf;
    uint32_t addr=0;
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    attribute = (unsigned char)((id & 0x00001F00) >> 8);
    
    for(i=0; i<get4num; i++)
    {
        if(oad == general_get4_talble[i].oad) 
        {
            if(attribute == 0x02)
            {
                if(0x4401 == oad)   return -DAR_RW_REJECT;
                *p++ = general_get4_talble[i].tag;
                if((general_get4_talble[i].tag == TYPE_octet_string) || (general_get4_talble[i].tag == TYPE_visible_string))
                {
                    *p++ = general_get4_talble[i].len;
                }
                else if(general_get4_talble[i].tag == TYPE_bit_string)
                {
                    *p++ = general_get4_talble[i].len * 8;
                }
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + general_get4_talble[i].s_index);
                nd_get_index(instance, addr, p, general_get4_talble[i].len); 
                p = p + general_get4_talble[i].len;
                return (p-buf);
            }
            else
            {
                return -1;
            }
        }
    }
    return -1;
}

int GET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;
    uint32_t addr = 0,s_index=0;
    uint16_t oad;
    uint8_t index,attribute,i,j,temp[42]={0},num=0,num_hourtab = 0;
    uint8_t* p = buf;
    int len = 0;
    int ret;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    len = general_get_4_para(instance, id, buf);
    if( len != -1)
    {
        return len;
    }
    

    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8); // 待调整 1F
    
    switch (oad)
    {
        case 0x4000://时间
        {
            switch (attribute)
            {
                case 0x02:
                {
                *p++ = TYPE_date_time_s;
                rtc_get_from_tm(instance,temp);
                *p++ = temp[6];
                *p++ = temp[5];
                *p++ = temp[4];
                *p++ = temp[3];
                *p++ = temp[2];
                *p++ = temp[1];
                *p++ = temp[0];
                
                return (p - buf);
                }
                break;
                case 0x22:
                case 0x42:
                {
                    *p++ = TYPE_date_time_s;
                    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
                    nd_get_index(instance, addr, p, 7);
                    p += 7;
                    return (p - buf);
                }
                break;
                case 0x62:
                case 0x82:
                {
                    *p++ = TYPE_date_time_s;
                    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
                    nd_get_index(instance, addr, p, 7);
                    p += 7;
                    return (p - buf);
                }
                break;

                default:
                    return -1;
                    break;
            }
            return -1;
        }
            break;
        case 0x4001://地址
            if(attribute == 0x02)
            {
                *p++ = TYPE_octet_string;
                *p++ = 6;
                memcpy(p, mbase_cb->tsa, 6);
                p = p + 6;
                return (p - buf);
            }
            else
            {
                return -1;
            }
            break;
        case 0x4002://表号
            if(attribute == 0x02)
            {
                *p++ = TYPE_octet_string;
                *p++ = 6;
                read_esam_no(instance, temp);
                memcpy(p, temp+7, 6);
                p = p + 6;
                return (p - buf);
            }
            else
            {
                return -1;
            }
            break;
        case 0x4004://设备地理位置
            if(index == 0x00)
            {
                *p++ = TYPE_structure;
                *p++ = 0x03;
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location1);
                nd_get_index(instance, addr, p, 10); 
                p = p + 10;
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location2);
                nd_get_index(instance, addr, p, 10); 
                p = p + 10;
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Location3);
                nd_get_index(instance, addr, p, 5); 
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
                return -1;
            
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + s_index);
            nd_get_index(instance, addr, p, len); 
            p = p + len;
            return (p-buf);
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
                uint8_t tag[] = {TYPE_unsigned, TYPE_long_unsigned, TYPE_long_unsigned, TYPE_long_unsigned, TYPE_unsigned, TYPE_unsigned, TYPE_unsigned};
                if(index == 0)
                {
                    *p++ = TYPE_structure;
                    *p++ = 7;
                    for(i=0; i<7; i++)
                    {
                        *p++ = tag[i];
                        mem_swap_cpy(p, (uint8_t*)idbuf[i], lens[i]);
                        p = p + lens[i];
                    }
                    return (p - buf);
                }
                else if(index < 8)
                {
                    *p++ = tag[index-1];
                    mem_swap_cpy(p, (uint8_t*)idbuf[index-1], lens[index-1]);
                    p = p + lens[index-1];
                    return (p - buf);
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
        case 0x400C://时区时段表数
            if(index > 5)   return -1;
            if(index == 0)
            {
                *p++ = TYPE_structure;
                *p++ = 5;
                for(i=0;i<5;i++)
                {
                    *p++ = TYPE_unsigned;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + i);
                    nd_get_index(instance, s_index, p, 1); 
                    p++;
                }
                return (p-buf);
            }
            else
            {
                *p++ = TYPE_unsigned;
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + (index - 1));
                nd_get_index(instance, s_index, p, 1); 
                p++;
                return (p-buf);
            }
            break;
            break;
        case 0x4011://公共假日表
            if(index > 10)    return -1;
            if(index == 0)
            {
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 4);
                nd_get_index(instance, s_index, &num, 1);
                if(num>10) num = 10;
                *p++ = TYPE_array;
                *p++ = num;
                for(i=0;i<num;i++)
                {
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + HoliDay_Table + i);
                    nd_get_index(instance, s_index, temp, 6); 
                    
                    *p++ = TYPE_structure;
                    *p++ = 2;
                    *p++ = TYPE_date;
                    memcpy(p, temp, 5);
                    p = p + 5;
                    *p++ = TYPE_unsigned;
                    *p++ = temp[5];
                }
                return (p-buf);
            }
            else 
            {
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + HoliDay_Table + (index-1));
                nd_get_index(instance, s_index, temp, 6); 
                
                *p++ = TYPE_structure;
                *p++ = 2;
                *p++ = TYPE_date;
                memcpy(p, temp, 5);
                p = p + 5;
                *p++ = TYPE_unsigned;
                *p++ = temp[5];
                return (p-buf);
            }
            break;
        case 0x4014://当前套时区表
        case 0x4015://备用套时区表
            if (id == 0x40152200) // 编程事件用
            {
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_SeasonT);
                ret = nd_get_index(instance, s_index, p, 114);
                if ((ret < 0) || (p[1] > 14) || (p[0] != TYPE_array))// 错误
                {
                    *p++ = 0;
                }
                else
                {
                    p += (p[1] * 8 + 2);
                }
                return (p - buf);
            }

            if(index > 14)    return -1;
            if(oad == 0x4014)   
                addr = Season_Table0;
            else 
                addr = Season_Table1;
            if(index == 0)
            {
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para);
                nd_get_index(instance, s_index, &num, 1);
                if(num>14) num = 14;
                *p++ = TYPE_array;
                *p++ = num;
                for(i=0; i<num; i++)
                {
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + i);
                    nd_get_index(instance, s_index, temp, 3);
                    *p++ = TYPE_structure;
                    *p++ = 3;
                    *p++ = TYPE_unsigned;
                    *p++ = temp[0];
                    *p++ = TYPE_unsigned;
                    *p++ = temp[1];
                    *p++ = TYPE_unsigned;
                    *p++ = temp[2];
                }
                return (p-buf);
            }
            else
            {
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + (index-1));
                nd_get_index(instance, s_index, temp, 3);
                *p++ = TYPE_structure;
                *p++ = 3;
                *p++ = TYPE_unsigned;
                *p++ = temp[0];
                *p++ = TYPE_unsigned;
                *p++ = temp[1];
                *p++ = TYPE_unsigned;
                *p++ = temp[2];
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
                *p++ = TYPE_array;
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 1);//日时段表数
                nd_get_index(instance, s_index, p, 1);
                num = *p++;
                for(j=0;j<num;j++)//日时段表数
                {
                    *p++ = TYPE_array;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 2);//日时段数，每日切换数
                    nd_get_index(instance, s_index, p, 1);
                    num_hourtab = *p++; 
                    
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + j);
                    nd_get_index(instance, s_index, temp, 3*num_hourtab);
                    for(i=0; i<num_hourtab; i++)
                    {
                        *p++ = TYPE_structure;
                        *p++ = 3;
                        *p++ = TYPE_unsigned;
                        *p++ = temp[0+i*3];
                        *p++ = TYPE_unsigned;
                        *p++ = temp[1+i*3];
                        *p++ = TYPE_unsigned;
                        *p++= temp[2+i*3];
                    }
                }
                return (p-buf);
            }
            else
            {
                *p++ = TYPE_array;
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 2);//日时段数，每日切换数
                nd_get_index(instance, s_index, p, 1);
                num_hourtab = *p++; 
                
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + (index-1));
                nd_get_index(instance, s_index, temp, 3*num_hourtab);
                for(i=0; i<num_hourtab; i++)
                {
                    *p++ = TYPE_structure;
                    *p++ = 3;
                    *p++ = TYPE_unsigned;
                    *p++ = temp[0+i*3];
                    *p++ = TYPE_unsigned;
                    *p++ = temp[1+i*3];
                    *p++ = TYPE_unsigned;
                    *p++= temp[2+i*3];
                }
                return (p-buf);
            }
            break;
        case 0x4116://结算日
            if(index == 0)
            {
                *p++ = TYPE_array;
                *p++ = 3;
                for(i=0;i<3;i++)
                {
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
                    nd_get_index(instance, s_index, temp, 2); 
                    
                    *p++ = TYPE_structure;
                    *p++ = 2;
                    *p++ = TYPE_unsigned;
                    *p++ = temp[0];
                    *p++ = TYPE_unsigned;
                    *p++ = temp[1];
                }
                return (p-buf);
            }
            else if(index < 4)
            {
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + (index-1));
                nd_get_index(instance, s_index, temp, 2); 
                
                *p++ = TYPE_structure;
                *p++ = 2;
                *p++ = TYPE_unsigned;
                *p++ = temp[0];
                *p++ = TYPE_unsigned;
                *p++ = temp[1];
                return (p-buf);
            }
            else
                return -1;
            break;
        case 0x4300:
            return get4300_para(instance,  id,  buf);
            break;
        case 0x4400:
            switch(attribute)
            {
                case 0x03:
                    *p++ = TYPE_structure;
                    *p++ = 7;
                    *p++ = TYPE_long_unsigned;
                    *p++ = 0x00;
                    *p++ = 0x16;
                    *p++ = TYPE_long_unsigned;
                    *p++ = 0x02;
                    *p++ = 0x00;
                    *p++ = TYPE_long_unsigned;
                    *p++ = 0x02;
                    *p++ = 0x00;
                    *p++ = TYPE_long_unsigned;
                    *p++ = 0x07;
                    *p++ = 0xd0;
                    *p++ = TYPE_bit_string;
                    *p++ = 0x40;
                    memcpy(p, messagebuf1, 8);
                    p = p + 8;
                    *p++ = TYPE_bit_string;
                    *p++ = 0x81;
                    *p++ = 0x80;
                    memcpy(p, messagebuf2, 16);
                    p = p + 16;
                    *p++ = TYPE_double_long_unsigned;
                    *p++ = 0x00;
                    *p++ = 0x00;
                    *p++ = 0x07;
                    *p++ = 0x08;
                    return (p-buf);
                case 0x04://当前连接的客户机地址
                    *p++ = TYPE_unsigned;
                    *p++ = mbase_cb->clientaddr;
                    return (p-buf);
                    break;
                case 0x05:
                    *p++ = TYPE_enum;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AuthMeth);
                    nd_get_index(instance, s_index, temp, 1); 
                    *p++ = temp[0];
                    return (p-buf);
                    break;
            }
            break;
        default:
            break;
    }
    return -DAR_OBJECT_NOT_EXIST;
}
