#include "nidavellir.h"
#include "fr_698.h"

const uint8_t messagebuf1[8] = {0xff,0xff,0xff,0xff,0xfc, 0x00, 0x00, 0x00};
const uint8_t messagebuf2[16] = {0xff,0xfe,0xc4,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
typedef struct 
{
    uint32_t    s_index;    //地址
    uint16_t    oad;    
    uint8_t     atr;        //属性
    uint8_t     tag;        //
    uint8_t     len;        //长度
}general_get4;

const general_get4  get4300_table[] = 
{
     //Device_descriptor,   0x0200,   0x02,     TYPE_visible_string,   33,//设备描述符
    
     Factory_No,          0x0301,   0x03,   TYPE_visible_string,    4, //厂家代码
     Version_SoftWare,    0x0302,   0x03,   TYPE_visible_string,    4, //软件版本
     Date_SoftWare,       0x0303,   0x03,   TYPE_visible_string,    6, //软件版本日期
     Version_HardWare,    0x0304,   0x03,   TYPE_visible_string,    4, //硬件版本
     Date_HardWare,       0x0305,   0x03,   TYPE_visible_string,    6, //硬件版本日期
     Extend_Info,         0x0306,   0x03,   TYPE_visible_string,    8, //扩展信息
    
     Produced_Date,       0x0400,   0x04,   TYPE_date_time_s,       7, //生产日期
//   Subset_List,         0x0500,   0x05,    2, //子设备列表(num+2*oi)
//   Protocol_List,       0x0600,   TYPE_visible_string,    20, //协议列表
//   Protocol_List,       0x0601,   TYPE_array,             20, //协议列表
     FollowReport_en,     0x0700,   0x07,   TYPE_bool,              1, //允许跟随上报
     ActiveReport_en,     0x0800,   0x08,   TYPE_bool,              1, //允许主动上报
     Comm_Master_en,      0x0900,   0x09,   TYPE_bool,              1, //允许与主站通讯
};


const general_get4  general_get4_talble[] =
{
     //Comm_Number,               0x4001,     TYPE_octet_string,      6,//表地址
     User_Number,               0x4003,     0x02,   TYPE_octet_string,      6,//客户编号
     SeasonTable_Changetime,    0x4008,     0x02,   TYPE_date_time_s,       7,//两套时区切换时间
     DayTable_Changetime,       0x4009,     0x02,   TYPE_date_time_s,       7,//两套时段切换时间
     //SecretKey_Num,             0x400F,     0x02,   TYPE_unsigned,          1,//秘钥总条数
     Meter_Num,                 0x4010,     0x02,   TYPE_unsigned,          1,//计量元件个数
     Weekend,                   0x4012,     0x02,   TYPE_bit_string,        1,//周休日特征字
     WeekendFee,                0x4013,     0x02,   TYPE_unsigned,          1,//周休日采用费率号
     Meter_CT,                  0x401C,     0x02,   TYPE_double_long_unsigned,   4,//CT
     Meter_PT,                  0x401D,     0x02,   TYPE_double_long_unsigned,   4,//PT
     Pulse_Width,               0x4102,     0x02,   TYPE_unsigned,          1,//脉冲宽度
     ASCII_AssetsMan,           0x4103,     0x02,   TYPE_visible_string,    32,//资产管理编码
     ASCII_Un,                  0x4104,     0x02,   TYPE_visible_string,    6,//额定电压
     ASCII_Ib,                  0x4105,     0x02,   TYPE_visible_string,    6,//额定电流
     ASCII_Imax,                0x4106,     0x02,   TYPE_visible_string,    6,//最大电流
     ASCII_Imin,                0x4106,     0x03,   TYPE_visible_string,    6,//最小电流
     ASCII_Itr,                 0x4106,     0x04,   TYPE_visible_string,    6,//转折电流
     ASCII_AccuracyClass1,      0x4107,     0x02,   TYPE_visible_string,    4,//有功准确度等级
     Meter_Const1,              0x4109,     0x02,   TYPE_double_long_unsigned,   4,//有功脉冲常数
     ASCII_MeterType,           0x410B,     0x02,   TYPE_visible_string,    32,//电能表型号
//     MeterRunWord,              0x4110,     TYPE_bit_string,        1,//电能表运行特征字
     SoftWareNo,                0x4111,     0x02,   TYPE_visible_string,    16,//软件备案号
//     Measure_Mode,              0x4112,     0x02,   TYPE_bit_string,        1,//有功组合特征字
     Meter_Password,            0x4401,     0x02,   TYPE_visible_string,    8,//应用连接密码
};
#define		get4num			sizeof(general_get4_talble)/sizeof(general_get4)

int get_support_protocol_list(struct instance_t* instance, uint32_t id, uint8_t* buf)
{
    uint8_t* p = buf;
    uint32_t addr = 0;
    uint8_t index = 0;
    uint8_t tempbuf[200] = {0};
    uint8_t *tmp1 = tempbuf,  *tmp1_cache = tempbuf;
    uint8_t *tmp2 = tempbuf + 100;
    uint8_t  i, count, len, maxlen;

    index = (uint8_t)(id & 0x000000FF);

    //tmp1 = res->tmp_buf;
    //tmp2 = tmp1 + 100;

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Protocol_List);
    nd_get_index(instance, addr, tmp2, 76); 


    maxlen = (76 - 1) / 3;
    count = *tmp2++;

    if ((index > count) || (count > 3))
    {
        return -DAR_OTHERS_ERROR;
    }

    if (index == 0)
    {
        *tmp1++ = TYPE_array;
        *tmp1++ = count;

        for (i = 0; i < count; i++)
        {
            *tmp1++ = TYPE_visible_string;
            len = tmp2[0] + 1;

            if (len > maxlen)
            {
                return -DAR_OTHERS_ERROR;
            }

            ndv_memcopy(tmp1, tmp2, len);
            tmp1 += len;
            tmp2 += maxlen;
        }
    }
    else
    {
        tmp2 += (index - 1) * maxlen;
        *tmp1++ = TYPE_visible_string;
        len = tmp2[0] + 1;

        if (len > maxlen)
        {
            return -DAR_OTHERS_ERROR;
        }

        ndv_memcopy(tmp1, tmp2, len);
        tmp1 += len;
    }

    len = tmp1 - tmp1_cache;



    ndv_memcopy(p, tmp1_cache, len);
    p += len;
    
    return (p-buf);
}


int get4300_para(struct instance_t* instance, uint32_t id, uint8_t* buf)
{
    uint16_t idl=0, tmpid1;
    uint8_t j, i, tmpbuf[13] = {0}, num, Tnum;
    uint8_t* p = buf;
    int16_t ret;
    uint32_t addr = 0;
    
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
    
  
    if(idl == 0x0200)//设备描述符
    {
        *p++ = TYPE_visible_string;
        *p++ = 32;
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Device_descriptor);
        nd_get_index(instance, addr, p, 32); 
        //num = p[0];
        p = p + 32;
        return (p-buf);
    }
    else if(idl == 0x0300)//版本信息
    {
        *p++ = TYPE_structure;
        *p++ = 6;
        Tnum = 6; // 6个成员
        tmpid1 = idl;
        for (j = 1; j < (Tnum + 1) ; j++)
        {
            for(i=0; i<sizeof(get4300_table)/sizeof(general_get4); i++)
            {
                if((tmpid1 + j) == get4300_table[i].oad)   
                {
                    *p++ = TYPE_visible_string;
                    *p++ = get4300_table[i].len;
                    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + get4300_table[i].s_index);
                    nd_get_index(instance, addr, p, get4300_table[i].len); 
                    p = p + get4300_table[i].len;
                }
            }
        }
        return (p-buf);
    }
    else if(idl == 0x0500)//子设备列表
    {
        *p++ = TYPE_array;
        *p++ = 0;
        return (p-buf);
    }
    else if((idl&0xFF00) == 0x0600)//协议列表
    {
        return  get_support_protocol_list(instance, id, buf);
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
    
    //事件用
    switch(id)
    {
        case 0x40132200: //编程前周休日时段表号
        {
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_WeekTab);
            nd_get_index(instance, addr, p, 2);
            p += 2;
            return (p-buf);
        }
            break;
        case 0x41122200: //编程前组合有功特征字
        {
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_ActCom);
            nd_get_index(instance, addr, p, 3);
            p += 3;
            return (p-buf);
        }
            break;
        default:
            break;
    }


    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8);
    
    for(i=0; i<get4num; i++)
    {
        if((oad == general_get4_talble[i].oad) && (attribute == general_get4_talble[i].atr))
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
    }
    return -1;
}

//读取应用语境 44000300
int apply_context_element_get(struct instance_t* instance, uint8_t index, uint8_t* buf, struct connect_request_t *connect)
{
    uint8_t *p = buf;
	uint32_t data;

	switch (index)
	{
	case 0:
        *p++ = TYPE_long_unsigned;
        mem_swap_cpy(p, (uint8_t*)&connect->pro_ver, 2);//版本号
        p += 2;
		break;
	case 1:
    case 2:
    case 3:
        *p++ = TYPE_long_unsigned;   //最大 接收 apdu 尺寸
        mem_swap_cpy(p, (uint8_t*)&connect->client_apdu_size, 2);    
        p += 2;
		break;

	case 4:
        *p++ = TYPE_bit_string;
        *p++ = 0x40;
        //memcpy(p, messagebuf1, 8);
        ndv_memcopy(p, (uint8_t*)&connect->protocal_conformance, 8);//期望协议一致性块
        p = p + 8;
		break;
	case 5:
        *p++ = TYPE_bit_string;
        *p++ = 0x81;
        *p++ = 0x80;
        //memcpy(p, messagebuf2, 16);
        ndv_memcopy(p, (uint8_t*)&connect->function_conformance, 16);  //期望的功能一致性块 
        p = p + 16;
		break;
	case 6:
        *p++ = TYPE_double_long_unsigned;
        mem_swap_cpy(p, (uint8_t*)&connect->connect_time, 4);    //期望的应用连接超时时间 
        p += 4;
		break;
	default:
		break;
	}

	return (uint16_t)(p - buf);

}

int GET_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb  = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct context_698_t* p_ctx = (struct context_698_t*)ctx;
    uint32_t addr = 0,s_index=0;
    uint16_t oad;
    uint8_t index,attribute,i,j,temp[42]={0},num=0,num_hourtab = 0, cilent_index = 0;
    uint8_t* p = buf;
    int len = 0;
    struct connect_request_t connect = {0};
    int ret;
    uint8_t channel = 0;
    
    if((p_ctx)&&(p_ctx->channel < chl_NUM))
    {
        channel = p_ctx->channel;
    }


    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }

    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    index = (unsigned char)(id & 0x000000FF);
    attribute = (unsigned char)((id & 0x0000FF00) >> 8); 

    
    len = general_get_4_para(instance, id, buf);
    if( len != -1)
    {
        return len;
    }

    if (CA_TYPE_TER(p_ctx->CA)) 
    {
        cilent_index = 1;
    }
    else
    {
        cilent_index = 0;
    }
    

    switch (oad)
    {
        case 0x4000://时间
        {
            switch (attribute)
            {
                case 1:
                {
                    p += fr_698_get_logic_name(p, oad);
                    return (p-buf);
                }
                break;
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
                case 0x05:
                {
                    *p++ = TYPE_structure;
                    *p++ = 2;
                    *p++ = TYPE_long_unsigned;
                    mem_swap_cpy(p, (uint8_t*)&mbase_cb->Limit_T_Min, 2);
                    p = p + 2;
                    *p++ = TYPE_long_unsigned;
                    mem_swap_cpy(p, (uint8_t*)&mbase_cb->Limit_T_Max, 2);
                    p = p + 2;
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
                    return -DAR_OBJECT_NOT_EXIST;
                    break;
            }
        }
            break;
        case 0x4001://地址
            if(attribute == 1)
            {
                p += fr_698_get_logic_name(p, oad);
                return (p-buf);
            }
            if(attribute == 0x02)
            {
                *p++ = TYPE_octet_string;
                *p++ = 6;
                mem_swap_cpy(p, mbase_cb->tsa, 6);
                p = p + 6;
                return (p - buf);
            }
            else
            {
                return -DAR_OBJECT_NOT_EXIST;
            }
            break;
        case 0x4002://表号
            if(attribute == 1)
            {
                p += fr_698_get_logic_name(p, oad);
                return (p-buf);
            }
            if(attribute == 0x02)
            {
                *p++ = TYPE_octet_string;
                *p++ = 6;
                //read_esam_no(instance, temp);
                memcpy(p, mbase_cb->meterNo, 6);
                p = p + 6;
                return (p - buf);
            }
            else
            {
                return -DAR_OBJECT_NOT_EXIST;
            }
            break;
        case 0x4004://设备地理位置
            if(attribute == 1)
            {
                p += fr_698_get_logic_name(p, oad);
                return (p-buf);
            }
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
                return -DAR_OBJECT_NOT_EXIST;
            
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + s_index);
            nd_get_index(instance, addr, p, len); 
            p = p + len;
            return (p-buf);
            break;
        case 0x4007://LCD参数
            if(attribute == 1)
            {
                p += fr_698_get_logic_name(p, oad);
                return (p-buf);
            }
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
                    return -DAR_OBJECT_NOT_EXIST;
                }
            }
            else
            {
                return -DAR_OBJECT_NOT_EXIST;
            }
            break;
        case 0x400C://时区时段表数
            if(attribute == 1)
            {
                p += fr_698_get_logic_name(p, oad);
                return (p-buf);
            }
            
            if(index > 5)   return -DAR_OBJECT_NOT_EXIST;
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
            if(attribute == 1)
            {
                p += fr_698_get_logic_name(p, oad);
                return (p-buf);
            }
            if(index > MaxHolidayNum)    return -DAR_OBJECT_NOT_EXIST;
            if(index == 0)
            {
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 4);
                nd_get_index(instance, s_index, &num, 1);
                if(num > MaxHolidayNum ) num = MaxHolidayNum;
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
            if(attribute == 1)
            {
                p += fr_698_get_logic_name(p, oad);
                return (p-buf);
            }
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
                return 114;//(p - buf);
            }

            if(index > 14)    return -DAR_OBJECT_NOT_EXIST;
            if(oad == 0x4014)   
                addr = Season_Table0;
            else 
                addr = Season_Table1;
            if(index == 0)
            {
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para);
                nd_get_index(instance, s_index, &num, 1);
                if(num > MaxSeasonNum) num = MaxSeasonNum;
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
            if(attribute == 1)
            {
                p += fr_698_get_logic_name(p, oad);
                return (p-buf);
            }        
            if(index > 0x08)    return -DAR_OBJECT_NOT_EXIST;
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
                if(num > MaxTouTabNum)     num = MaxTouTabNum;
                for(j=0;j<num;j++)//日时段表数
                {
                    *p++ = TYPE_array;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 2);//日时段数，每日切换数
                    nd_get_index(instance, s_index, p, 1);
                    num_hourtab = *p++; 
                    
                    if(num_hourtab > MaxTouNum)     num_hourtab = MaxTouNum;
                    
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
                
                if(num_hourtab > MaxTouNum)     num_hourtab = MaxTouNum;
                
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
        case 0x4112:    //组合特征字
            if((attribute == 0x02) && (index == 0))
            {
                *p++ = 0x04;
                *p++ = 0x08;
                *p++ = mbase_cb->active_energy_cal_word;
                return (p-buf);
            }
            else
                return -DAR_OBJECT_NOT_EXIST;
            break;
        case 0x4116://结算日
            if(attribute == 1)
            {
                p += fr_698_get_logic_name(p, oad);
                return (p-buf);
            }
            if (id == 0x41162200) // 编程事件用
            {
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSet_Balance);
                nd_get_index(instance, s_index, p, 20);
                p += 20;
                return (p-buf);
            }

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
                return -DAR_OBJECT_NOT_EXIST;
            break;
        case 0x4300:
            if(attribute == 1)
            {
                p += fr_698_get_logic_name(p, oad);
                return (p-buf);
            }
            return get4300_para(instance,  id,  buf);
            break;
        case 0x4400:
            switch(attribute)
            {
                case 1:
                {
                    p += fr_698_get_logic_name(p, oad);
                    return (p-buf);
                }
                break;
                case 0x03:
                    if(0 != mbase_cb->connect_timeout[channel][cilent_index])
                    {
                        ndv_memcopy(&connect, &cfg_connect, sizeof(cfg_connect));
                    }
                    else
                    {
                        connect = get_default_connect_para(channel);
                    }
                    
                    switch(index)
                    {
                        case 0:
                            *p++ = TYPE_structure;
                            *p++ = 7;
                            for (i = 0; i < 7; i++)
                            {
                                len = apply_context_element_get(instance, i, p, &connect);
                                p += len;
                            }
                            break;

                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 6:
                        case 7:   
                            len = apply_context_element_get(instance, index - 1, p, &connect);
                            p += len;
                            break;

                        default:
                            return -DAR_OBJECT_UNDEF;

                    }

                    return (p-buf);
                case 0x04://当前连接的客户机地址
                    *p++ = TYPE_unsigned;
                    if(0 != mbase_cb->connect_timeout[channel][cilent_index])
                    {
                        *p++ = mbase_cb->clientaddr[channel];
                    }
                    else
                    {
                        *p++ = 0;
                    }

                    return (p-buf);
                    break;
                case 0x05:
                    *p++ = TYPE_enum;
                    if(0 != mbase_cb->connect_timeout[channel][cilent_index])
                    {
                        ndv_memcopy(&connect, &cfg_connect, sizeof(cfg_connect));
                    }
                    else
                    {
                        connect = get_default_connect_para(channel);
                    }

                    if(connect.connect_mode > 3)  connect.connect_mode = 0;
                    *p++ = connect.connect_mode;
                    return (p-buf);
                    break;
            }
            break;
        default:
            return -DAR_OBJECT_NOT_EXIST;
            break;
    }
    return -DAR_OBJECT_NOT_EXIST;
}
