#include "nidavellir.h"
#include "645id_table.h"
#include "gs_record.h"
#include "fr_axdr.h"
#include "fr_698.h"


#define RECORED_FILL_NO_OBJ_645  0x00
#define RECORED_FILL_NO_DATA_645  0x00

extern uint8_t check_capture_exist(uint32_t rcsd_req, uint32_t rcsd_src);
extern uint32_t get_interval_basic_value(uint8_t *src, struct rsd_info_t *rsd_info);
extern uint8_t select_record_time_check(uint8_t *src, uint8_t non_period, struct rsd_info_t *rsd_info);
extern int record_data_package645(struct instance_t* instance, uint32_t ID, uint8_t* buf,  struct rsd_info_t *rsd_info, uint32_t *rcsd_info, uint16_t *capture_645len,  uint8_t  package_type,   void *ctx);
extern int record_data_package645_packageAA(struct instance_t* instance, uint8_t* dest, uint8_t* src, uint16_t datalen, struct record_storage_645_id_info_t  *info);

struct load_rsd_info_t
{
    struct rsd_info_t rsd_info;
    uint8_t read_num;
};

//645 部分

//645特殊数据标记
//EEEEXXNN:  数据用 XX填充 NN个字节
const uint32_t  DayFreeze_645oad[]         = { 
    13, 
    (0x20210200), (0x00100200), (0x00200200), (0xEEEEAA01), (0xEEEEAA01),
    (0xEEEEAA01), (0xEEEEAA01), (0xEEEEAA01), (0xEEEEAA01),  (0xEEEEAA01),
    (0xEEEEAA01), 
    (0x20040200), (0x20050200),   //复合 共用一个id
};

const uint16_t DayFreeze_645oad_len[] = 
{
    269, //总长度
    5, 
    
    0x8000|52,     /*冻结模式字bit0*/
    0x8000|52, 
    0x8000|52,          
    0x8000|52, 
    0x8000|52, 0x8000|52, 0x8000|52, 0x8000|52,
    0x8000|104, 
    0x8000|104,          
    12, 12, /*bit7*//*最后两个为同一个子ID*/
};

//BIT15: 长度变长      BIT14:单相不支持
const uint16_t DayFreeze_645oad_package_len[] = 
{
    5, 
    0x8000|52,     /*冻结模式字bit0*/
    0x8000|52, 
    
    0xC000|52,          
    0xC000|52, 
    0xC000|52, 0xC000|52, 0xC000|52, 0xC000|52,
    0xC000|104, 
    0xC000|104,          
    24,   /*最后两个为同一个子ID*/
};


const uint32_t  HourFreeze_645oad[]         = { 
    3, 
    (0x20210200), (0x00100201), (0x00200201), 
};


const uint16_t HourFreeze_645oad_len[] = 
{
    13, //总长度
    5,4,4, 
    
};

//BIT15: 长度变长      BIT14:单相不支持
const uint16_t HourFreeze_645oad_package_len[] = 
{
    5,4,4, 
};



//过流   19
const uint32_t  OverCurrent_645oad[]         = { 
    49, 
    (0x201E0200),  //发生时刻
    (0x00102201), (0x00202201), (0xEEEE0004), (0xEEEE0004),//
    (0x00102201), (0x00202201), (0xEEEE0004), (0xEEEE0004),//A相
    (0x20002201), (0x20012201), (0x20042202), (0x20052202), (0x200A2202),//A相    

    (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004),//B相
    (0x20002202), (0x20012202), (0x20042203), (0x20052203), (0x200A2203),//B相    

    (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004),//C相
    (0x20002203), (0x20012203), (0x20042204), (0x20052204), (0x200A2204),//C相    

    (0x20200200), //结束时刻
    (0x00108201), (0x00208201), (0xEEEE0004), (0xEEEE0004),//
    (0x00108201), (0x00208201), (0xEEEE0004), (0xEEEE0004),//A相
    (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004),//B相
    (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004),//C相  
};


const uint16_t OverCurrent_645oad_len[1+49] = 
{
    179,            //数据块总长度
    6, 4, 4, 4, 4,  //发生时刻
    4, 4, 4, 4, 2, 3, 3, 3, 2, 
    4, 4, 4, 4, 2, 3, 3, 3, 2, 
    4, 4, 4, 4, 2, 3, 3, 3, 2, 
    
    6, 4, 4, 4, 4, 4, 4, 4, 4,  4, 4, 4, 4, 4, 4, 4, 4,   //结束时刻
};

//开盖   03300D01   不支持块内单个数据项抄读
const uint32_t  CoverOpen_645oad[]         = { 
    14, 
    (0x201E0200),   //发生时刻
    (0x20200200),   //结束时刻
    (0x00102201), (0x00202201), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), //
    (0x00108201), (0x00208201), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), 
};


const uint16_t CoverOpen_645oad_len[1+14] = 
{
    60,
    6,6,4,4,4,4,4,4,4,4,4,4,4,4,
};
//掉电   03110001   不支持块内单个数据项抄读
const uint32_t  PowerOff_645oad[]         = { 
    2, 
    (0x201E0200),   //发生时刻
    (0x20200200),   //结束时刻
};


const uint16_t PowerOff_645oad_len[1+2] = 
{
    12,
    6,6
};

//跳闸   
const uint32_t  SwitchOff_645oad[]         = { 
    8, 
    (0x201E0200),   //发生时刻
    (0xEEEE0004),
    (0x00102201), (0x00202201), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), //
};


const uint16_t SwitchOff_645oad_len[1+8] = 
{
    34,
    6,4,4,4,4,4,4,4,
};

//合闸   
const uint32_t  SwitchOn_645oad[]         = { 
    8, 
    (0x201E0200),   //发生时刻
    (0xEEEE0004),
    (0x00102201), (0x00202201), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), //
};


const uint16_t SwitchOn_645oad_len[1+8] = 
{
    34,
    6,4,4,4,4,4,4,4,
};

//结算日编程
const uint32_t  BillDayWr_645oad[]  = { 
    3, 
    (0x201E0200),   //发生时刻
    (0xEEEE0004),
    (0x41162200), //复合3个
};

const uint16_t BillDayWr_645oad_len[1+3] = 
{
    16,
    6,4,6,
};
//有功组合方式编程
const uint32_t  ActCombWr_645oad[]  = { 
    3, 
    (0x201E0200),   //发生时刻
    (0xEEEE0004),
    (0x41122200), 
};

const uint16_t ActCombWr_645oad_len[1+3] = 
{
    11,
    6,4,1,
};

//周休日编程
const uint32_t  WeekTabWr_645oad[]  = { 
    3, 
    (0x201E0200),   //发生时刻
    (0xEEEE0004),
    (0x40132200), 
};

const uint16_t WeekTabWr_645oad_len[1+3] = 
{
    11,
    6,4,1,
};

//校时
const uint32_t  SetTime_645oad[]  = { 
    3, 
    (0xEEEE0004),
    (0x201E0200),   //发生时刻
    (0x20200200),   //结束时刻
};

const uint16_t SetTime_645oad_len[1+3] = 
{
    16,
    4,6,6,
};

//事件清零
const uint32_t  EventClr_645oad[]  = { 
    3, 

    (0x201E0200),   //发生时刻
    (0xEEEE0004),
    (0x330C0206),   //事件清零数据标识
};

const uint16_t EventClr_645oad_len[1+3] = 
{
    14,
    6,4,4,
};
//电表清零
const uint32_t  MeterClr_645oad[]  = { 
    26, 
    (0x201E0200),   //发生时刻
    (0xEEEE0004),   

    (0x00102201), (0x00202201), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004),//
    (0x00102201), (0x00202201), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004),//A相        

    (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004),//B相
    (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004), (0xEEEE0004),//B相  
};

const uint16_t MeterClr_645oad_len[1+26] = 
{
    106,
    6,4,
    4,4,4,4,4,4,
    4,4,4,4,4,4,
    4,4,4,4,4,4,
    4,4,4,4,4,4,
};
//负荷开关误动作
const uint32_t  SwitchErr_645oad[]  = { 
    7, 
    (0x201E0200),   //发生时刻
    (0x20200200),   //结束时刻 
    (0xF2052201),   //负荷开关误动作后状态
    (0x00102201), (0x00202201), 
    (0x00108201), (0x00208201),         
};

const uint16_t SwitchErr_645oad_len[1+7] = 
{
    29,
    6, 6, 1, 
    4, 4, 4, 4,
};
//电源异常
const uint32_t  PowerAbnormal_645oad[]  = { 
    4, 
    (0x201E0200),   //发生时刻
    (0x20200200),   //结束时刻 
    (0x00102201), (0x00202201), 
        
};

const uint16_t PowerAbnormal_645oad_len[1+4] = 
{
    20,
    6,6,4,4,
};

//645编程记录
const uint32_t MeterWr645_645oad[]  = { 
    3, 
    (0x201E0200),   //发生时刻
    (0xEEEE0004), 
    (0xFFFE0000),   //645编程列表      
};

const uint16_t MeterWr645_645oad_len[1+4] = 
{
    50,
    6,4,40
};



const uint32_t  Load_di2_01_645oad[]         = { 
    3, 
    (0x20000201), (0x20000202), (0x20000203), 
};


const uint16_t Load_di2_01_645oad_len[] = 
{
    6, //总长度
    2,2,2, 
};

const uint32_t  Load_di2_02_645oad[]         = { 
    3, 
    (0x20010201), (0x20010202), (0x20010203), 
};


const uint16_t Load_di2_02_645oad_len[] = 
{
    9, //总长度
    3,3,3, 
};

const uint32_t  Load_di2_03_645oad[]         = { 
    4, 
    (0x20040201), (0x20040202), (0x20040203), (0x20040204), 
};


const uint16_t Load_di2_03_645oad_len[] = 
{
    12, //总长度
    4,4,4,4 
};

const uint32_t  Load_di2_05_645oad[]         = { 
    4, 
    (0x200A0201), (0x200A0202), (0x200A0203), (0x200A0204), 
};


const uint16_t Load_di2_05_645oad_len[] = 
{
    8, //总长度
    2,2,2,2 
};


const uint32_t  Load_di2_06_645oad[]         = { 
    4, 
    (0x00100201), (0x00200201), ((0xEEEE0004)), ((0xEEEE0004)), 
};


const uint16_t Load_di2_06_645oad_len[] = 
{
    12, //总长度
    4,4,4,4 
};


// 获取冻结中实际费率的需量或电量长度
uint16_t get_645record_rate_obj_len(struct instance_t* instance,uint16_t obj_len)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base); 

    uint8_t single_len = 0;
    uint32_t addr;

    uint8_t tariff_num = 4;


    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 3); //读取费率数
    nd_get_index(instance, addr, &tariff_num, 1); 
    
    tariff_num += 1;   // 总加 费率个数
    if(tariff_num > 13)  tariff_num = 13;
    if (obj_len & 0x8000)
    {
        obj_len &=0x3FFF;
        if (obj_len == 52) // 电能    
        {
            single_len = 4;
        }
        else//需量   三相
        {
            single_len = 8;
        }

        
        obj_len = single_len*(tariff_num);
    }

    if (obj_len > (8*13)) //异常
    {
        return 0;
    }
    
    return obj_len;

}





// 通过oad 获取 事件相关的存储地址信息
int get_record_645_id_info(struct instance_t* instance, uint32_t ID, struct record_storage_645_id_info_t* info)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base); 
    union dl645_DI_t DI;
    uint32_t oad = 0;
    uint16_t SaveId = 0, SeqId = 0, CaptureId = 0, PeriodId = 0, DepthId = 0, ReportFlagId = 0, ReportModeId = 0;
    uint8_t SaveDevice, SaveBlock;
    uint8_t temp = 0;
    uint32_t addr;
    int ret = 0;


    if (mbase_cb == NULL)
    {
        return -1;
    }

    DI.di = ID;
    
    memset((uint8_t*)info, 0, sizeof(struct record_storage_645_id_info_t));
    info->package_num  = 1;
    info->mark_para = 0xFF;

    switch (ID&0xFF000000)//XX000000
    {
        case 0x19000000: //过流
        {
            
            if ((DI.elem.di2 == 0)|| (DI.elem.di2 > 1))  // 单相A 1    B 2   C 3
            {
                return -1;
            }
       
            if ((DI.elem.di1 == 0) && ((DI.elem.di0 > 2) || (DI.elem.di0 == 0)))  //  总次数 和累计时间
            {
                return -1;
            }

            info->oad698 = 0x30050700 + ((DI.elem.di2 - 1)<<8);
            info->rd_645obj = OverCurrent_645oad;
            info->rd_645obj_len = OverCurrent_645oad_len;
        }
        break;
        case 0x1E000000: //合闸
        case 0x1D000000: //跳闸
        {
            if ((DI.elem.di2 != 0))  
            {
                return -1;
            }
            if ((DI.elem.di1 == 0) && (DI.elem.di0 != 1))  //  总次
            {
                return -1;
            }

            if(DI.elem.di3 == 0x1E)
            {
                info->oad698 = 0x30200200;
                info->rd_645obj = SwitchOn_645oad;
                info->rd_645obj_len = SwitchOn_645oad_len;
            }
            else
            {
                info->oad698 = 0x301F0200;
                info->rd_645obj = SwitchOff_645oad;
                info->rd_645obj_len = SwitchOff_645oad_len;
            }
        }
        break;

        case 0x03000000:  // 事件
        {
            switch((ID&0xFFFF0000)) 
            {
                case 0x03300000:// 不支持块内单个数据项
                {
                    
                    info->package_num  = 0;// 不支持块内单个数据项----------------
                    
                    if(DI.elem.di0 > 10)  return -1;

                    switch((ID&0xFFFFFF00))
                    {
                        case 0x03300D00: //开盖
                        {
                            info->oad698 = 0x301B0200;
                            info->rd_645obj = CoverOpen_645oad;
                            info->rd_645obj_len = CoverOpen_645oad_len;
                        }
                        break;

                        //case 0x03301200://秘钥更新
                        //{
                        //    info->oad698 = 0x30260200;
                        //    info->rd_645obj = SecretKeyWr_645oad;
                        //    info->rd_645obj_len = SecretKeyWr_645oad_len;
                        //}
                        //break;

                        case 0x03300C00://结算日编程
                        {
                            info->oad698 = 0x301A0200;
                            info->rd_645obj = BillDayWr_645oad;
                            info->rd_645obj_len = BillDayWr_645oad_len;
                        }
                        break;
                        case 0x03300900://有功组合方式编程
                        {
                            info->oad698 = 0x30220200;
                            info->rd_645obj = ActCombWr_645oad;
                            info->rd_645obj_len = ActCombWr_645oad_len;
                        }
                        break;
                        //case 0x03300800://节假日编程
                        //{
                        //    info->oad698 = 0x30210200;
                        //    info->rd_645obj = HolidayWr_645oad;
                        //    info->rd_645obj_len = HolidayWr_645oad_len;
                        //}
                        //break;
                        case 0x03300700://周休日编程
                        {
                            info->oad698 = 0x30190200;
                            info->rd_645obj = WeekTabWr_645oad;
                            info->rd_645obj_len = WeekTabWr_645oad_len;
                        }
                        break;
                        //case 0x03300600://时区表编程
                        //{
                        //    info->oad698 = 0x30180200;
                        //    info->rd_645obj = SeasonTabWr_645oad;
                        //    info->rd_645obj_len = SeasonTabWr_645oad_len;
                        //}
                        //break;
                        //case 0x03300500://时段表编程
                        //{
                        //    info->oad698 = 0x30170200;
                        //    info->rd_645obj = DayTabWr_645oad;
                        //    info->rd_645obj_len = DayTabWr_645oad_len;
                        //}
                        //break;
                        case 0x03300400://校时
                        {
                            info->oad698 = 0x30160200;
                            info->rd_645obj = SetTime_645oad;
                            info->rd_645obj_len = SetTime_645oad_len;
                        }
                        break;
                        case 0x03300300://事件清零
                        {
                            info->oad698 = 0x30150200;
                            info->rd_645obj = EventClr_645oad;
                            info->rd_645obj_len = EventClr_645oad_len;
                        }
                        break;
                        case 0x03300100://电表清零
                        {
                            info->oad698 = 0x30130200;
                            info->rd_645obj = MeterClr_645oad;
                            info->rd_645obj_len = MeterClr_645oad_len;
                        }
                        break;
                        case 0x03300000://645编程记录 
                        {

                            info->oad698 = 0xFF310000;
                            info->rd_645obj = MeterWr645_645oad;
                            info->rd_645obj_len = MeterWr645_645oad_len;
                        }
                        break;
                        //case 0x03300A00://无功组合1编程
                        //case 0x03300B00://无功组合2编程
                        //case 0x03301300://异常插卡
                        //case 0x03301400://非法插卡
                        //case 0x03301000://阶梯表编程
                        //case 0x03300F00://费率表编程
                        //case 0x03300E00://开端盖

                        default:
                            return -1;
                        break;
                    }
                }
                break;
                case 0x03110000://掉电
                {
                    info->package_num  = 0;// 不支持块内单个数据项
                    if(DI.elem.di1 != 0)  return -1;
                    info->oad698 = 0x30110200;
                    info->rd_645obj = PowerOff_645oad;
                    info->rd_645obj_len = PowerOff_645oad_len;
                }
                break;
                case 0x03360000://负荷开关误动作
                {
                    info->package_num  = 0;// 不支持块内单个数据项
                    if(DI.elem.di1 != 0)  return -1;
                    info->oad698 = 0x302B0200;
                    info->rd_645obj = SwitchErr_645oad;
                    info->rd_645obj_len = SwitchErr_645oad_len;
                }
                break;
                case 0x03370000://电源异常
                {
                    info->package_num  = 0;// 不支持块内单个数据项
                    if(DI.elem.di1 != 0)  return -1;
                    info->oad698 = 0x302C0200;
                    info->rd_645obj = PowerAbnormal_645oad;
                    info->rd_645obj_len = PowerAbnormal_645oad_len;
                }
                break;
                
                //case 0x03350000://恒定磁场
                default:
                    return -1;
                break;
            }
   
        }
        break;

        
        case 0x00000000: //结算类  特殊处理  645关联对象外部构造
        {
            info->package_type = PACKAGE645_NORMAL;
            info->package_num  = 0;
            info->oad698 = 0x50050200;
            
            temp = 12;
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 3); //读取费率数
            nd_get_index(instance, addr, &temp, 1); 
            if(temp > 12)
            {
                temp = 4;
            }
            
            if(((DI.elem.di0 == 0) || ((DI.elem.di0 >= 12)&& (DI.elem.di0 != 0xFF)))
            ||((DI.elem.di1 != 0xFF) && (DI.elem.di1 > temp))  //代表索引
            ||((DI.elem.di2 >= 3))) //单相
            {
                return -1;
            }
#if 0            
            oad = 00000200;
            oad +=   ((DI.elem.di2 << 4) << 24); // 00XX000200
            
            info->rd_645obj[0] = 1;
            if(DI.elem.di1 == 0xFF) //单个电能 块
            {
               info->rd_645obj_len[0] = temp * 4;
               info->rd_645obj_len[1] = temp * 4;
            }
            else
            {
                oad += ((DI.elem.di1) + 1); // 00XX0002YY 
                info->rd_645obj_len[0] =  4;
                info->rd_645obj_len[1] =  4;
            }
            info->rd_645obj[1] = oad;
            
#endif
        }
        break;
        case 0x05000000: //冻结类
        {
            if(DI.elem.di1 == 0xFF)
            {
                info->package_type = PACKAGE645_FREEZE;
            }

            switch (ID&0xFFFF0000)// OI
            {
                case 0x05040000: //整点冻结  
                {
                    if((DI.elem.di1 > 0x02)&&(DI.elem.di1 != 0xFF)) //单相 
                    {
                        return -1;
                    }
                    info->oad698 = 0x50030200;
                    info->rd_645obj = HourFreeze_645oad;
                    info->rd_645obj_len = HourFreeze_645oad_len;
                    info->rd_645obj_package_len = HourFreeze_645oad_package_len;
                    info->package_num  = 3;   
                    
                }
                break;
                //case 0x05000000: //定时冻结
                case 0x05010000: //瞬时
                case 0x05020000: //两套时区切换
                case 0x05030000: //两套时段切换                  
                case 0x05060000: //日冻结
                {
                    if((DI.elem.di1 >= 0x03)&&(DI.elem.di1 != 0x10)&&(DI.elem.di1 != 0xFF)) //单相 
                    {
                        return -1;
                    }

                    info->oad698 = 0x50040200;

                    if(DI.elem.di2 == 0x06) info->oad698 = 0x50040200;
                    else if(DI.elem.di2 == 0x01) info->oad698 = 0x50000200;
                    else if(DI.elem.di2 == 0x02) info->oad698 = 0x50080200;
                    else if(DI.elem.di2 == 0x03) info->oad698 = 0x50090200;

                    info->rd_645obj = DayFreeze_645oad;
                    info->rd_645obj_len = DayFreeze_645oad_len;
                    info->rd_645obj_package_len = DayFreeze_645oad_package_len;
                    info->package_num  = 12;   
                }
                break;
                case 0x05050000: //两套费率电价切换
                case 0x05070000: //两套阶梯电价切换
                {
                    if(!(mbase_cb->metertype & BIT3))
                    {
                        return -1;
                    }
                    if((DI.elem.di1 >= 0x03)&&(DI.elem.di1 <= 0x0A))
                    {
                        return -1;
                    }
                    
                    info->oad698 = 0x50040200;
                    if(DI.elem.di2 == 0x05) info->oad698 = 0x500A0200;
                    if(DI.elem.di2 == 0x07) info->oad698 = 0x500B0200;
                    
                    info->rd_645obj = DayFreeze_645oad;
                    info->rd_645obj_len = DayFreeze_645oad_len;
                    info->rd_645obj_package_len = DayFreeze_645oad_package_len;
                    info->package_num  = 12;   //初始化

                }
                break;
                
                //case 0x05080000: //剩余金额   
 
             
                default:
                    return -1;
                    break;
            }
            
        }
        break;

        case 0x06000000: //负荷曲线 06xxxxxx
        {

            info->package_type = PACKAGE645_LOAD;
            info->package_num  = 0xFF;
            info->oad698 = 0x50020200;

            switch (ID&0xFFFF0000)// OI
            {
                case 0x06100000: //新负荷
                {
                    if(DI.elem.di0 == 0xFF) //单相 
                    {
                        return -1;
                    }
                    
                    switch (DI.elem.di1)// 0610XX00
                    {
                        case 0x01: //电压
                        {
                            if((DI.elem.di0 == 0) || ((DI.elem.di0 > 1) && (DI.elem.di0 != 0xFF))) //三相 DI.elem.di0 > 3
                            {
                                return -1;
                            }
                            info->mark_bit = 0;
                            info->rd_645obj = Load_di2_01_645oad;
                            info->rd_645obj_len = Load_di2_01_645oad_len;
                            info->package_num  = 1; //id0 从1开始
                        }
                        break;
                        case 0x02: //电流
                        {
                            if((DI.elem.di0 == 0) || ((DI.elem.di0 > 1) && (DI.elem.di0 != 0xFF))) //三相DI.elem.di0  > 3
                            {
                                return -1;
                            }
                            info->mark_bit = 0;
                            info->rd_645obj = Load_di2_02_645oad;
                            info->rd_645obj_len = Load_di2_02_645oad_len;
                            info->package_num  = 1; //id0 从1开始
                        }
                        break;
                        case 0x03: //有功功率
                        {
                            if(((DI.elem.di0 > 1) && (DI.elem.di0 != 0xFF))) //三相 DI.elem.di0 > 3
                            {
                                return -1;
                            }
                            info->mark_bit = 1;
                            info->rd_645obj = Load_di2_03_645oad;
                            info->rd_645obj_len = Load_di2_03_645oad_len;
                        }
                        break;
                        case 0x04: //无功功率
                        {
                            info->mark_bit = 1;
                            return -1;
                        }
                        break;
                        case 0x05: //功率因数
                        {
                            if(((DI.elem.di0 > 1) && (DI.elem.di0 != 0xFF))) //三相 DI.elem.di0 > 3
                            {
                                return -1;
                            }    
                            info->mark_bit = 2;
                            info->rd_645obj = Load_di2_05_645oad;
                            info->rd_645obj_len = Load_di2_05_645oad_len;
                        }
                        break;
                        case 0x06: //有功无功总电能
                        {
                            if((DI.elem.di0 == 0) || ((DI.elem.di0 > 2) && (DI.elem.di0 != 0xFF))) //三相 DI.elem.di0 > 4
                            {
                                return -1;
                            }
                            info->mark_bit = 3;
                            info->rd_645obj = Load_di2_06_645oad;
                            info->rd_645obj_len = Load_di2_06_645oad_len;
                            info->package_num  = 1; //id0 从1开始
                        }
                        break;
                        case 0x07: //四象限无功 
                        {
                            info->mark_bit = 4;
                            return -1;
                        }
                        break;
                        case 0x08: //有功无功需量
                        {
                            info->mark_bit = 5;
                            return -1;
                        }
                        break;                        
                        default:
                            return -1;
                        break;
                    }
                }
                break;
                default:
                    return -1;
                    break;
            }            

        }
        break;




        default:
            return -1;
            break;
    }


    if((info->package_type == PACKAGE645_NORMAL)  && (info->package_num == 1))  //事件
    {
        if (((DI.elem.di1 > info->rd_645obj[0]) && (DI.elem.di1 != 0xFF))            //  子项
        ||((DI.elem.di0 > 10) && (DI.elem.di0 != 0xFF)))  //最多支持10条
        {
            return -1;
        }   
    }

    ret = get_record_id_info(instance, info->oad698, &info->id_info);
    if(ret < 0)
    {
        return -1;
    }

    return 1;
}

int response_x_byte_err_645(uint8_t *p_tx, uint16_t err, uint8_t errLen, uint8_t *tx_len)
{
    if (errLen > 2) 
    {
        *tx_len = 0;
        return -1;
    }

    p_tx[0] = (uint8_t)err;
    if (errLen == 2)
    {
        p_tx[0] = (uint8_t)err;
        p_tx[1] = (uint8_t)(err>>8);
    }


    *tx_len = errLen;
    return (int)errLen;
}


// data_time_s 转6字节 
int fc_fun_01_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint32_t temp;
    uint8_t *p = des;
    uint8_t tmpbuf[8] = {0};
    uint16_t year, data_len = 6;

    //tag 处理
    src++;
    if (mode == RFC_WRITE)
    {


    }
    else if (mode == RFC_READ)
    {
        temp = buf_to_uint32_msb(src);
        second_to_dt_buf(temp, tmpbuf); 
        year = buf_to_uint16_msb(&tmpbuf[0]);
        
        p[0] = BYTE_BCD((uint8_t)(year-2000));
        p[1] = BYTE_BCD(tmpbuf[2]);
        p[2] = BYTE_BCD(tmpbuf[3]);
        p[3] = BYTE_BCD(tmpbuf[4]);
        p[4] = BYTE_BCD(tmpbuf[5]);
        if(fc)
        {
            data_len = fc->original_len;
        }
        
        if (data_len == 6)
        {
            p[5] = BYTE_BCD(tmpbuf[6]);
        }

        swap_buffer(p, data_len);
        p += data_len;
    }
    return (p - des);
}

// //事件清零数据标识
int fc_fun_02_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint16_t decode_len = 0;
    uint8_t *p = des;
    
    if (mode == RFC_WRITE)
    {

    }
    else if (mode == RFC_READ)
    {
        src += 3;
        
        mem_swap_cpy(p, src, 4);
        p += 4;
    }
    return (p - des);
}


int fc_fun_03_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint16_t decode_len = 0;
    uint8_t *p = des;
    //uint8_t num = 0;


    if (mode == RFC_WRITE)
    {

    }
    else if (mode == RFC_READ)
    {
        src += 12;
        
        ndv_memcopy(p, src, 1);
        p += 1;
    }
    return (p - des);

}

//电压块 支持单个电压读取
int fc_fun_04_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint16_t decode_len = 0, len = 0, tmp_U16 = 0;
    uint8_t *p = des;
    //uint8_t num = 0;
    uint8_t index;
    
    index = get_OAD_Index(oad_req);

    if (mode == RFC_GET_ORIGINAL_LEN) 
    {
        if (index == 0)
        {
            len = 6;
        }
        else
        {
            len = 2;
        }
        return len;
    }
    /*    
        else if (mode == RFC_GET_STORE_LEN) //单相
        {
            
        }
    */

    else if (mode == RFC_WRITE)
    {
    


    }
    else if (mode == RFC_READ)
    {
        if (index == 0)
        {
            tmp_U16 = shorthex_to_bcd(((uint16_t)(src[0]<<8) + (src[1])));
            uint16_to_buf_lsb(tmp_U16, p);
            p += 2;
            src += 2;
            
            memset(p, RECORED_FILL_NO_OBJ_645, 2*2); 
            p += 2*2;

        }
        else if (index == 1)
        {
            tmp_U16 = shorthex_to_bcd(((uint16_t)(src[0]<<8) + (src[1])));
            uint16_to_buf_lsb(tmp_U16, p);
            p += 2;
            src += 2;

            //swap_buffer(p, 2); 

        }
        else
        {
        
        }
    }
    return (p - des);
}

//电流块 支持单个读取
int fc_fun_05_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint16_t decode_len = 0, len;
    uint8_t *p = des;
    //uint8_t num = 0;
    uint8_t index;
    int32_t tmp_32 = 0;
    
    index = get_OAD_Index(oad_req);

    
    if (mode == RFC_GET_ORIGINAL_LEN) 
    {
        if (index == 0)
        {
            len = 3*3;
        }
        else
        {
            len = 3;
        }
        return len;
    }
    /*    
        else if (mode == RFC_GET_STORE_LEN) //单相
        {
            
        }
    */

    if (mode == RFC_WRITE)
    {

    }
    else if (mode == RFC_READ)
    {
        if (index == 0)
        {
            
            tmp_32 = ((int32_t)src[0]<<24) + (src[1]<<16) + (src[2]<<8) + (src[3]);


            //转bcd
            HEX_BCD_Signed((int32_t)tmp_32 , p , 3);

            p += 3;
            src += 4;

            memset(p, RECORED_FILL_NO_OBJ_645, 3*2); 
            p += 3*2;
        }
        else if (index == 1)
        {
            tmp_32 = ((int32_t)src[0]<<24) + (src[1]<<16) + (src[2]<<8) + (src[3]);


            //转bcd
            HEX_BCD_Signed((int32_t)tmp_32 , p , 3);

            p += 3;
            src += 4;

        }
        else//三相 
        {
        
        }
    }
    return (p - des);
}

//功率块 支持单个读取
int fc_fun_06_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint16_t decode_len = 0, len;
    uint8_t *p = des;
    //uint8_t num = 0;
    uint8_t index;
    int32_t tmp_32 = 0;
    
    index = get_OAD_Index(oad_req);


    if (mode == RFC_GET_ORIGINAL_LEN) 
    {
        if (index == 0)
        {
            len = 12;
        }
        else 
        {
            len = 3;
        }

        return len;
    }
    /*    
        else if (mode == RFC_GET_STORE_LEN) //单相
        {
            
        }
    */

    if (mode == RFC_WRITE)
    {

    }
    else if (mode == RFC_READ)
    {
        if (index == 0)
        {
            tmp_32 = ((int32_t)src[0]<<24) + (src[1]<<16) + (src[2]<<8) + (src[3]);

            //转bcd
            HEX_BCD_Signed((int32_t)tmp_32 , p , 3);
            p += 3;
            //转bcd
            HEX_BCD_Signed((int32_t)tmp_32 , p , 3);
            p += 3;
            src += 4;

            memset(p, 0xFF, 3*2); 
            p += 3*2;

        }
        else if ((index == 1)|| (index == 2))
        {
            tmp_32 = ((int32_t)src[0]<<24) + (src[1]<<16) + (src[2]<<8) + (src[3]);

            //转bcd
            HEX_BCD_Signed((int32_t)tmp_32 , p , 3);
            p += 3;

            src += 4;
        }
        else//三相 
        {
        
        }
    }
    return (p - des);
}
//功率因数块 支持单个读取
int fc_fun_07_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint16_t decode_len = 0, len;
    uint8_t *p = des;
    //uint8_t num = 0;
    uint8_t index;
    int16_t tmp_16;
    
    index = get_OAD_Index(oad_req);
    
    if (mode == RFC_GET_ORIGINAL_LEN) 
    {
        if (index == 0)
        {
            len = 8;
        }
        else
        {
            len = 2;
        }
        return len;
    }
    /*    
        else if (mode == RFC_GET_STORE_LEN) //单相
        {
            
        }
    */

    if (mode == RFC_WRITE)
    {

    }
    else if (mode == RFC_READ)
    {
        if (index == 0)
        {

            tmp_16 = (int16_t)(src[2]<<8) + (src[3]);

            //转bcd
            HEX_BCD_Signed((int32_t)tmp_16 , p , 2);
            p += 2;
            //转bcd
            HEX_BCD_Signed((int32_t)tmp_16 , p , 2);
            p += 2;
            src += 2;

            memset(p, RECORED_FILL_NO_OBJ_645, 2*2); 
            p += 2*2;

        }
        else if ((index == 1)|| (index == 2))
        {
            //转bcd
            HEX_BCD_Signed((int32_t)tmp_16 , p , 2);
            p += 2;
            src += 2;
        }
        else//三相 
        {
        
        }
    }
    return (p - des);
}

// 结算日
int fc_fun_08_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint8_t *p = des;
    uint8_t i;


    if (mode == RFC_WRITE)
    {

    }
    else if (mode == RFC_READ)
    {
        src += 2;
        for(i = 0; i < 3; i++)
        {
            src += 2;
            src += 1;
            p[1] = BYTE_BCD(src[0]);
            src += 1;
            src += 1;//tag
            p[0] = BYTE_BCD(src[0]);
            src += 1;
            p += 2;
        }
    }
    return (p - des);

}
// 有功组合方式特征字
int fc_fun_09_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint8_t *p = des;
    uint8_t i;


    if (mode == RFC_WRITE)
    {

    }
    else if (mode == RFC_READ)
    {
        src += 2;
        p[0] = swap_byte_bit(src[0]);
        p++;
        src++;
    }
    return (p - des);

}
// 周休日采用时段表号    1字节tag + 1字节数据   hex 转BCD
int fc_fun_10_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint8_t *p = des;
    uint8_t i;


    if (mode == RFC_WRITE)
    {

    }
    else if (mode == RFC_READ)
    {
        src += 1;
        p[0] = BYTE_BCD(src[0]);
        p++;
        src++;

    }
    return (p - des);

}
// 645编程列表
int fc_fun_11_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src, uint8_t *des, uint8_t* src, enum format_conversion_mode mode, struct format_conversion_t *fc)
{
    uint8_t *p = des;
    uint8_t i,num = 0;

    if (mode == RFC_WRITE)
    {

    }
    else if (mode == RFC_READ)
    {
        src += 1;
        num = *src++;
        if((num > 10) ||(num == 0))
        {
            return 0; //异常
        }
        
        for(i = 0; i < 10; i++)
        {
            src++;
            if (i < num)
            {
                mem_swap_cpy(p, src, 4);
            }
            else
            {
                memset(p, 0xFF, 4);
            }
            p += 4;
            src += 4;
        }
    }
    return (p - des);

}




// 电能数据 存储和协议转换  
int fc_fun_Energy_645(struct instance_t* instance, uint32_t oad_req, uint32_t oad_src,uint8_t *des, uint8_t* src, enum format_conversion_mode mode)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    uint8_t i, /*type,*/ attr, index, index_src,  /*len, */block_num = (4+1);
    uint16_t oi;
    uint8_t *p = des;
    uint8_t energy_tag = 0, sign_flag=0;
    uint32_t addr;
    int64_t tmp_64 = 0;
    
    //type = get_OAD_Class(oad_req);
    attr = get_OAD_Attribute(oad_req);
    index = get_OAD_Index(oad_req);
    oi = get_OAD_OI(oad_req);


    index_src = get_OAD_Index(oad_src);

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 3); //读取费率数
    nd_get_index(instance, addr, &block_num, 1); 
    
    block_num += 1;   // 总加 费率个数
    if(block_num > 13)  block_num = 13;

    if (mode == RFC_WRITE)// 645用不到写
    {   
 

    }
    else if (mode == RFC_READ)
    {
        if (((oi&0x0FF0) == 0x0000) || ((oi&0x0FF0) == 0x0030) || ((oi&0x0FF0) == 0x0040))
        {
            sign_flag = 1;
            energy_tag = TYPE_double_long;
            if(attr == 4) //
            {
               energy_tag = TYPE_long64;
            }
        }
        else
        {
            sign_flag = 0;
            energy_tag = TYPE_double_long_unsigned;
            if(attr == 4) //
            {
               energy_tag = TYPE_long64_unsigned;
            }
        }
  
        if(attr == 2)//普通
        {
            if(index)
            {
                if (index_src == 0) //取索引偏移
                {
                    src += (index-1)*5;
                }
                

                tmp_64 = (int64_t)((int64_t)src[0]<<32)+ ((int64_t)src[1]<<24) + (src[2]<<16) + (src[3]<<8) + (src[4]);
                if ((sign_flag)&&(src[0] & 0x80))
                {
                    tmp_64 += 0xFFFFFF0000000000;
                }
                tmp_64 /= 100;

                //p[0] = (uint8_t)(tmp_64 >> (3*8));
                //p[1] = (uint8_t)(tmp_64 >> (2*8));
                //p[2] = (uint8_t)(tmp_64 >> (1*8));
                //p[3] = (uint8_t)(tmp_64 >> (0*8));

                //转bcd
                if(sign_flag)
                {
                    HEX_BCD(p, (uint32_t)tmp_64);
                }
                else
                {
                    HEX_BCD_Signed((int32_t)tmp_64 , p , 4);
                }
                

                p += 4;
                src += 5;
            }
            else
            {

                for (i = 0; i < block_num; i++)
                {

                    tmp_64 = (int64_t)((int64_t)src[0]<<32)+ ((int64_t)src[1]<<24) + (src[2]<<16) + (src[3]<<8) + (src[4]);
                    if((sign_flag)&&(src[0] & 0x80))
                    {
                        tmp_64 += 0xFFFFFF0000000000;
                    }
                    tmp_64 /= 100;
                    
                    //p[0] = (uint8_t)(tmp_64 >> (3*8));
                    //p[1] = (uint8_t)(tmp_64 >> (2*8));
                    //p[2] = (uint8_t)(tmp_64 >> (1*8));
                    //p[3] = (uint8_t)(tmp_64 >> (0*8));

                    //转bcd
                    if(sign_flag)
                    {
                        HEX_BCD(p, (uint32_t)tmp_64);
                    }
                    else
                    {
                        HEX_BCD_Signed((int32_t)tmp_64 , p , 4);
                    }
                    

                    src += 5;
                    p += 4;
                }
            }
        }
        else if(attr == 4) // 高精度 不支持
        {
  
        }
        else  //不支持
        {
            
        }

    }
    return (p - des);
}



// record存储格式特殊处理的 oad
const struct format_conversion_t format_conversion_tab_645[] =
{
    {   0x201E0200,     6,      5,      fc_fun_01_645   },//YYMMDDhhmmss
    {   0x20200200,     6,      5,      fc_fun_01_645   },
    {   0x20210200,     5,      5,      fc_fun_01_645   },//YYMMDDhhmm
    {   0x40000200,     6,      5,      fc_fun_01_645   },
    
    {   0x200002FF,     0xFFFF/*5*/,      2,      fc_fun_04_645   },//电压
    {   0x200102FF,     0xFFFF/*7*/,      4,      fc_fun_05_645   },//电流
    {   0x200402FF,     0xFFFF/*12*/,     4,      fc_fun_06_645   },//功率    
    {   0x200A02FF,     0xFFFF/*8*/,      2,      fc_fun_07_645   },//功率因数

    {   0x330C0206,     4,      7,      fc_fun_02_645   },//事件清零数据标识
    {   0xF2050201,     1,      17,      fc_fun_03_645   },//继电器状态

    {   0x41160200,     6,      20,     fc_fun_08_645   },//结算日
    {   0x41120200,     1,      3,      fc_fun_09_645   },//有功组合模式字

    {   0x40130200,     1,      2,      fc_fun_10_645   },//1tag + 1 bcd
    {   0xFFFE0000,    40,      52,     fc_fun_11_645   },//645编程列表  
    


    
};

// record存储格式特殊处理的 oad 电能



//enum format_conversion_mode // record_change_format_645
//
//    RFC_WRITE,           //存储写入转换
//    RFC_READ,            //协议读出转换
//    RFC_GET_ORIGINAL_LEN,//获取原协议格式len
//    RFC_GET_STORE_LEN,   //获取存储格式len
//
int record_change_format_645(struct instance_t* instance, uint8_t *des,  uint8_t* src, uint32_t oad_req, uint32_t oad_src, enum format_conversion_mode mode)
{
    uint16_t i;
    uint8_t type, attr, index, len, block_num = (4+1);
    uint32_t addr, cmp_oad = 0;

    type = get_OAD_Class(oad_req);
    attr = get_OAD_Attribute(oad_req);
    index = get_OAD_Index(oad_req);
    switch(type)
    {
        case 0: //电能
        {
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 3); //读取费率数
            nd_get_index(instance, addr, &block_num, 1); 
            
            block_num += 1;   // 总加 费率个数
            if (mode == RFC_GET_ORIGINAL_LEN) 
            {
                if(attr == 2)//普通
                {
                    if(index)
                    {
                        len = (4); 
                    }
                    else
                    {
                        len = (4)*block_num; 
                    }
                }
                else if(attr == 4) // 高精度
                {
                    if(index)
                    {
                        len = (5);
                    }
                    else
                    {
                        len = (5)*block_num;
                    }
                }
                else  //不支持
                {
                    len = 0;
                }
                return len;
            }
            else if (mode == RFC_GET_STORE_LEN)
            {
                if(attr == 2)//普通
                {
                    if(index)
                    {
                        len = (5);
                    }
                    else
                    {
                        len = (5)*13;
                    }
                }
                else if(attr == 4) // 高精度
                {
                    if(index)
                    {
                        len = (5);
                    }
                    else
                    {
                        len = (5)*13;
                    }
                }
                else  //不支持
                {
                    len = 0;
                }
                return len;
            }
            else
            {
                return fc_fun_Energy_645(instance, oad_req, oad_src, des, src, mode);
            }
        }
            break;

        
        default:
        {
            for (i = 0; i < (sizeof(format_conversion_tab_645) / sizeof(format_conversion_tab_645[0])); i++)
            {
                cmp_oad = oad_req;
                if (0xFF == (uint8_t)(format_conversion_tab_645[i].oad)) //支持块/索引
                {
                    cmp_oad |= (uint32_t)0x000000FF;
                }
                if (0xFF == (uint8_t)(format_conversion_tab_645[i].oad >> 8))
                {
                    cmp_oad |= (uint32_t)0x000000FF<<8;
                }
                if (0xFF == (uint8_t)(format_conversion_tab_645[i].oad >> 16)) 
                {
                    cmp_oad |= (uint32_t)0x000000FF<<16;
                }
                if (0xFF == (uint8_t)(format_conversion_tab_645[i].oad >> 24)) 
                {
                    cmp_oad |= (uint32_t)0x000000FF<<24;
                }

                if ((format_conversion_tab_645[i].oad & (0x0000F000)) != 0 ) //指定方案  全字比对
                {
                    if ((cmp_oad ) == format_conversion_tab_645[i].oad)
                    {
                        break;
                    }
                }
                else
                {
                    if ((cmp_oad & (0xFFFF1FFF)) == format_conversion_tab_645[i].oad)
                    {
                        break;
                    }
                }

            }
            
            if (i >= (sizeof(format_conversion_tab_645) / sizeof(format_conversion_tab_645[0])))
            {
                return -1;
            }
            
            if (mode == RFC_GET_ORIGINAL_LEN)
            {
                if(format_conversion_tab_645[i].original_len == 0xFFFF) 
                {
                    return format_conversion_tab_645[i].format_fun(instance, oad_req, oad_src, des, src, mode, (struct format_conversion_t *)&format_conversion_tab_645[i]);

                }
                else
                {
                    return format_conversion_tab_645[i].original_len;
                }
                
            }
            else if (mode == RFC_GET_STORE_LEN)
            {
                if(format_conversion_tab_645[i].store_len == 0xFFFF) 
                {
                    return format_conversion_tab_645[i].format_fun(instance, oad_req, oad_src, des, src, mode, (struct format_conversion_t *)&format_conversion_tab_645[i]);

                }
                else
                {
                    return format_conversion_tab_645[i].store_len;
                }
            }
            else
            {
                if(format_conversion_tab_645[i].format_fun)
                {
                    return format_conversion_tab_645[i].format_fun(instance, oad_req, oad_src, des, src, mode, (struct format_conversion_t *)&format_conversion_tab_645[i]);
                }
            }

        }
        break;
    }
    return 1;
}

//返回1 为总累计次数    
//返回2 为总累计时间
//其他 -1
int event_acc_number_times_DI_check(struct instance_t* instance, unsigned long ID)
{
     //合法性外部已判断
    union dl645_DI_t DI;
    DI.di = ID;

    
    if(0x03300000 == (ID & 0xFFFF00FF ))
    {
        return 1; //总次数
    }


    switch(ID)
    {
#if 0    
        


        case 0x03300A00://无功组合1编程
        case 0x03300B00://无功组合2编程
        case 0x03301200://秘钥更新
        case 0x03301300://异常插卡
        case 0x03301400://非法插卡
        
        case 0x03301000://阶梯表编程
        case 0x03300F00://费率表编程
        case 0x03300E00://开端盖
        case 0x03300D00://开盖
        case 0x03300C00://结算日编程
        case 0x03300900://有功组合方式编程
        case 0x03300800://节假日编程
        case 0x03300700://周休日编程
        case 0x03300600://时区表编程
        case 0x03300500://时段表编程
        case 0x03300400://校时
        case 0x03300300://事件清零
        case 0x03300100://电表清零
        case 0x03300000://编程记录
#endif      
        case 0x03350000://恒定磁场
        case 0x03360000://负荷开关误动作
        case 0x03370000://电源异常

        
        case 0x1E000001://合闸
        case 0x1D000001://跳闸
        case 0x03110000://掉电
        case 0x19010001: //过流 
        case 0x19020001: //过流
        case 0x19030001: //过流
        {
            return 1; // 总次数
        }
        break;
        case 0x19010002: //过流 总累计时间
        case 0x19020002: //过流
        case 0x19030002: //过流
        {
            return 2;
        }
        break;

        
        
        default:
            break;
    }

    return -1;
}


//检查搜索的关联对象是否存在
uint8_t check_capture_exist_645(uint32_t rcsd_req, uint32_t rcsd_src)
{
    volatile   uint8_t type = 0, attr_req = 0, attr_src = 0, index_req = 0,index_src = 0, schem_req = 0, schem_src = 0, index_limit = 0;
    uint16_t oi_req = 0, oi_src = 0;

    //常规
    if (rcsd_req == rcsd_src)
    {
        return true;
    }


    //特殊
    type = get_OAD_Class(rcsd_req);
    attr_req = get_OAD_Attribute(rcsd_req);
    index_req = get_OAD_Index(rcsd_req);
    oi_req = get_OAD_OI(rcsd_req);
    schem_req = get_OAD_Schem(rcsd_req);
    
    attr_src = get_OAD_Attribute(rcsd_src);
    index_src = get_OAD_Index(rcsd_src);
    oi_src = get_OAD_OI(rcsd_src);
    schem_src = get_OAD_Schem(rcsd_src);

    if ((type) == 0) //电能
    {
        if( index_req > 13 )  return false; // 不存在的费率  // 或者改为 fee num
        if((oi_req == oi_src) && (schem_req == schem_src))
        {
            if (((attr_req == 2) || (attr_req == 4)) && ((attr_src == 2) || (attr_src == 4)))
            {
                if((index_req == index_src) || (index_src == 0))
                {
                    return true;
                }
            }
        }
    }
    else if ((type) == 2) //变量
    {
        switch(oi_req)
        {
            case 0x2000://电压    
            case 0x2001://电流
            case 0x2002://电压相角
            case 0x2003://电压电流相角
            case 0x2004://有功功率    
            case 0x2005://无功功率     
            case 0x2006://视在功率 
            case 0x200A://功率因素
                if((oi_req == oi_src) && (schem_req == schem_src))
                {
                    if (((attr_req == 2)) && ((attr_src == 2)))
                    {
                        if((oi_req >= 0x2000) && (oi_req <= 0x2003))
                        {
                            index_limit = 1;
                        }
                        else
                        {
                            index_limit = 2;
                        }
                        
                        if( index_req > index_limit ) // 不存在
                        {
                            return false;
                        }
                        
                        if((index_req == index_src) || (index_src == 0))
                        {
                            return true;
                        }                  
                    }
                }
            
            break;
            default:
                break;
        }

    }
    
    return false;
}




//协议列选打包    rcsd_req:列选oad，   rcsd_src:原关联对象oad，   
//mode:0，不拷贝数据，只计算长度   1，  解析数据，并返回长度 
//package_type,只有读块记录且需要间隔符时，才需置1。
int _record_data_package_01_645(struct instance_t* instance,  uint8_t *des, uint8_t *src, uint32_t *rcsd_req, uint32_t *rcsd_src, uint8_t mode, uint16_t *capture_645len,  uint32_t  ID)
{
    
    uint32_t i, k,z, n;
    uint8_t *p_des = des, *p_src = src;
    int single_len, store_len, orig_len;
    uint8_t tmpbuf[200];//单个oad最大长度
    uint16_t cap_645_len = 0;
    union dl645_DI_t DI;

    DI.di = ID;  
    
    for(i=1; i < (rcsd_req[0] + 1); i++)
    {

        cap_645_len = get_645record_rate_obj_len(instance, capture_645len[i]);
        
        p_src = src;

        for(k=1; k < (rcsd_src[0] + 1); k++)
        {
            if (true == check_capture_exist(rcsd_req[i], rcsd_src[k]))// 645请求 的不带方案属性特征
            //if (rcsd_req[i] == rcsd_src[k])  // 
            {
                //特殊rcsd长度处理
                single_len = record_change_format_645(instance, tmpbuf, p_src, rcsd_req[i], 0, RFC_GET_STORE_LEN);
                if (single_len < 0)
                {
                    single_len = nd_size(instance, tmpbuf,rcsd_src[k]);
                }

                if (single_len > 0)
                {
                    // 个别数据仍需特殊处理
                    for (n = 0; n < single_len; n++) // 数据全f时 转空
                    {
                        if (p_src[n] != 0xff)
                        {
                            break;
                        }
                    }
                    if (n >= single_len)
                    {
                        orig_len = cap_645_len;
                        if (mode)
                        {
                            memset(p_des, RECORED_FILL_NO_OBJ_645, orig_len);//数据补0  
                        }
                        p_des += orig_len; 
                        p_src += single_len;
                    }
                    else
                    {
                        orig_len = record_change_format_645(instance, tmpbuf, p_src, rcsd_req[i], rcsd_src[k], RFC_READ);
                        if (orig_len >= 0)
                        {
                            if (mode)
                                memcpy(p_des, tmpbuf, orig_len);
                        }
                        else
                        {
                            orig_len = single_len;
                            if (mode)
                                memcpy(p_des, p_src, single_len);
                        }

                       
                        p_des += orig_len;
                        p_src += single_len;
                    }
                    
                }
                else
                {
                    if (mode)
                    {
                        *p_des = TYPE_null;
                    }
                    p_des++;
                    p_src++;
                }
                break;
            }
            else
            {
                single_len = record_change_format_645(instance, tmpbuf, p_src, rcsd_src[k], 0,  RFC_GET_STORE_LEN);
                if (single_len < 0)
                {
                    single_len = nd_size(instance, tmpbuf,rcsd_src[k]);
                }

                if (single_len > 0)
                {
                    p_src += single_len;
                }
                else
                {
                    p_src++;
                }
            }
        }
        
        if (k >= (rcsd_src[0]+1))
        {
            orig_len = record_change_format_645(instance, tmpbuf, p_src, rcsd_req[i], 0, RFC_GET_ORIGINAL_LEN);
            if (orig_len < 0)
            {
                orig_len = cap_645_len;
            }


            if (mode)
            {
                memset(p_des, RECORED_FILL_NO_OBJ_645 , orig_len);//数据补0

                if(DI.elem.di3 == 0x05)//冻结  //单相
                {
                    if(rcsd_req[i] == 0x20050200)// 单相   变量数据      无功   后面补0xFF
                    {
                        memset(p_des, 0xFF , orig_len);//数据补0
                    }
                }
            }
            p_des += orig_len;
            
        }
    }  
    
    return (p_des - des);
}

//获取协议列选打包长度   rcsd_req:列选oad，   rcsd_src:原关联对象oad，   mode:0，不拷贝数据，只计算长度。  1，  解析数据，并返回长度 
int get_package_01_len_645(struct instance_t* instance,  uint8_t *des, uint8_t *src, uint32_t *rcsd_req, uint32_t *rcsd_src,  uint16_t *capture_645len,  uint32_t  ID)
{
    return _record_data_package_01_645(instance,  des, src, rcsd_req, rcsd_src, 0,  capture_645len,    ID);
}

//协议列选打包   rcsd_req:列选oad，   rcsd_src:原关联对象oad，   mode:0，不拷贝数据，只计算长度。  1，  解析数据，并返回长度 
int record_package_01_645(struct instance_t* instance,  uint8_t *des, uint8_t *src, uint32_t *rcsd_req, uint32_t *rcsd_src,  uint16_t *capture_645len,  uint32_t  ID)
{
    return _record_data_package_01_645(instance,  des, src, rcsd_req, rcsd_src, 1,  capture_645len,    ID);
}
//数据未产生，补0.
int record_response_null_645(struct instance_t* instance,  uint8_t *des, uint8_t *src, uint32_t *rcsd_req, uint32_t *rcsd_src, uint16_t *capture_645len,  uint32_t ID)
{
    union dl645_DI_t DI;
    int one_len = 0;

    DI.di = ID;
    
    one_len = get_package_01_len_645(instance, des, src, rcsd_req, rcsd_src, capture_645len,  ID);

    if(( one_len < 0) || ( one_len > 544))
    {
        return -1;
    }

    memset(des, 0,  one_len);


    if(DI.elem.di3 == 0x05)//冻结  //单相
    {
        if((DI.elem.di2 <= 7) && (DI.elem.di2 != 4)) //整点冻结、剩余金额日冻结 除外
        {
            if((DI.elem.di1 == 0x10) || (DI.elem.di1 == 0xFF))// 单相   变量数据 后面补18个0xFF
            {
                if(one_len > 18)
                {
                    memset(des + one_len - 18, 0xFF,  18);
                }
            }
        }
    }
    
    return one_len;
}

//645读记录型数据打包
int record_data_package645(struct instance_t* instance, uint32_t ID, uint8_t* buf,  struct rsd_info_t *rsd_info, uint32_t *rcsd_info, uint16_t *capture_645len,  uint8_t  package_type,   void *ctx)
{
#ifndef WIN32

    union dl645_DI_t DI;

    struct record_storage_645_id_info_t record_id_info;

    struct circular_info_t circular_info = {0};
    uint32_t *rcsd_req = (uint32_t *)rcsd_info; 
    uint8_t *ptr_tmp = instance->apdu_cache + 500;    
    uint32_t rcsd_src[50] = {0};
    uint16_t i, read_num, number = 0;
    uint16_t judge_size, one_len = 0,len = 0;
    uint8_t count = 0, rcsd_schem = 0, non_period = 0, check_result = 0;
    uint32_t oad_schem = 0, value = 0;
    uint8_t *p = (buf ); // 预留3个字节条数
    int ret = 0;

    
    //检查 读取的记录型oad合法性

    DI.di = ID;

    ret = get_record_645_id_info(instance, ID, &record_id_info);
    if(ret < 0)
    {
        return -2; //无请求数据
    }



    //---获取条数
    
    circular_info.start_mark = rsd_info->start_mark;
    circular_info.end_mark = rsd_info->end_mark;
    circular_info.mode =   rsd_info->mode;

    switch(rsd_info->select)
    {
        case 0:
            //ret = GET_record_sel_0(instance, pipeline, mesg);
            //circular_info.curr_num--;
            break;
            
        case 1:
        case 2:
            if((1 == non_period) && ((rsd_info->mode == 2) || (rsd_info->mode == 3))) //方法1/2 非周期性类 按时间搜索时 由协议筛选
            {
                circular_info.mode = 0;
            }
            ret = nd_log_output(instance, record_id_info.id_info.save_index,  ptr_tmp, 1, &circular_info);
            circular_info.mode =   rsd_info->mode; 
            
            if ((0 == non_period) && (rsd_info->select == 1) && ((rsd_info->mode == 2) || (rsd_info->mode == 3))) //方法1  周期性冻结 按时间搜索 返回最近一条
            {
                if(circular_info.valid_num > 1)
                {
                    circular_info.valid_num = 1;
                }
            }
            break;
            
        case 9: //上n次
            ret = nd_log_output(instance, record_id_info.id_info.save_index,  ptr_tmp, 1, &circular_info);
            if ((rsd_info->last_no == 0) || (rsd_info->last_no > circular_info.valid_num))
            {
                return record_response_null_645(instance, p, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
            }
            circular_info.curr_num = circular_info.valid_num - rsd_info->last_no;
            circular_info.valid_num = circular_info.curr_num + 1;
            break;
        default:
            break;
    }

    
    if (circular_info.unit_size > (cache_max-500))
    {
        return record_response_null_645(instance,  p, ptr_tmp, rcsd_req, rcsd_src, capture_645len, ID);
    }

    if (circular_info.valid_num == 0)
    {
        return record_response_null_645(instance, p, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
    }
    //------------
    
    //获取关联对象
    get_capture_list(instance, record_id_info.oad698, rcsd_src, 3);

    check_result = 0;
    read_num = circular_info.valid_num;
    for (i = circular_info.curr_num; i < read_num; i++)
    {
        v85xxp_clear_watchdog();
        //获取1条记录内容
        ret = nd_log_output(instance, record_id_info.id_info.save_index,  ptr_tmp, circular_info.unit_size, &circular_info);
        if(ret < 0)
        {
            return record_response_null_645(instance, p, ptr_tmp, rcsd_req, rcsd_src, capture_645len, ID);
        }
        

        //-----长度判断
        if (one_len == 0)
        {   
            //预算打包长度
            one_len = get_package_01_len_645(instance, p+len, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
        }
        

        judge_size = (p - buf + len + one_len);
        

        if((judge_size) > (cache_max-500))
        {
            //return -DAR_BUFFER_OVERFLOW;
        }
        //-----------

        //----协议筛选
        if (rsd_info->interval_special)
        {
            rsd_info->interval = rsd_info->interval_special;
        }
        if (rsd_info->interval > 0)
        {
            value = get_interval_basic_value(ptr_tmp, rsd_info);
            
            if (number == 0)
            {
                circular_info.start_mark = value;
            }
            
            if (value == circular_info.start_mark)
            {
                one_len = record_package_01_645(instance, p+len, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
                len += one_len;
                number++;
                circular_info.start_mark += rsd_info->interval;
            }
            else if (value > circular_info.start_mark)
            {
                if (((value - circular_info.start_mark) % rsd_info->interval) == 0)
                {
                    one_len = record_package_01_645(instance, p+len, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
                    len += one_len;
                    number++;
                    circular_info.start_mark = value + rsd_info->interval;
                }
                else
                {
                    // 方法2读取 周期性冻结 返回应为 以搜索范围内第一条时间点作为起始时间 + 整数倍间隔时间
                    circular_info.start_mark += (((value - circular_info.start_mark) / rsd_info->interval) + 1) * rsd_info->interval;
            
                    // 方法2读取 周期性冻结 返回应为 读取起始时间 + 整数倍间隔时间
                    //circular_info.start_mark = circular_info.start_mark + rsd_info->interval;  
                }
            }
        }
        else
        {
            //协议判断是否符合要求
            check_result = select_record_time_check(ptr_tmp, non_period, rsd_info);
            if ( check_result > 0 )
            {
                if ((check_result == 1) && (number >= 1)) // 至少已搜索到一条
                {
                    len -= one_len; // 
                }
            
                //开始打包
                one_len = record_package_01_645(instance, p+len, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
                len += one_len;
                number++;
            }
        }

    }      
        
    return (int)len;
#endif
}


int record_data_package645_packageAA(struct instance_t* instance, uint8_t* dest, uint8_t* src, uint16_t datalen, struct record_storage_645_id_info_t  *info)
{
    uint8_t i, index_num = info->package_num;
    uint16_t  dest_single_len = 0, src_single_len = 0;
    uint8_t *p = dest;
    uint8_t *p_src = src;

    if((index_num == 0)||(index_num > 50))
    {
        return -1;
    }
    
    if (info->rd_645obj_package_len == NULL)
    {
        return -1;
    }

    for(i = 0; i < index_num; i++)
    {

        src_single_len = get_645record_rate_obj_len(instance, info->rd_645obj_package_len[i]);
        dest_single_len = src_single_len;
        if(info->rd_645obj_package_len[i] & BIT14)
        {
            dest_single_len = 0;
        }
        ndv_memcopy(p, p_src, dest_single_len);
        p += dest_single_len;
        p_src += src_single_len;
        *p++ = 0xAA;

        if (datalen >= src_single_len)
        {
            datalen -= src_single_len;
        }
        else
        {
            return -1;
        }
    }
    
    return  (p-dest);

}


//冻结、结算、事件记录读取
// 返回0 正常应答        返回＞0 异常应答
int read_05( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx, uint8_t *tx_len)
{
    union dl645_DI_t DI;
    struct record_storage_645_id_info_t  info;
    int ret;
    uint32_t rcsd_645req[51] = {0};
    uint16_t rcsd_645len[51] = {0};
    struct rsd_info_t rsd_info;
    int total_len = 0, data_len = 0;
    uint8_t *ptr_tmp = instance->apdu_cache;
    uint8_t di1_index = 0;

    uint8_t *p_err = (p_tx - 4);
    uint32_t sequence = 0;
    uint8_t last_no = 0, ten_read_cycles = 1; //仅有子项的事件支持
    uint8_t i,temp = 12;
    uint32_t addr = 0, oad = 0;


    memset(&rsd_info, 0,sizeof(rsd_info));

    
    DI.di = ID;

    ret = get_record_645_id_info(instance, ID, &info);
    if (ret <= 0)
    {
        return response_x_byte_err_645(p_err, RS_ERR_ID, 1,  tx_len);

        //return -2; //无请求数据
    }


    if((DI.elem.di3 != 0x05) && (DI.elem.di3 != 0x00))// 事件
    {
        ret = event_acc_number_times_DI_check(instance, ID); //事件累计时间 总次数 判断
        if ((ret == 1) || (ret == 2))
        {
            sequence = 0;
            if(ret == 1) //总次数
            {
                nd_get_index(instance, info.id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));
            }
            else
            {
                //事件累计时间/
                if (info.id_info.acc_time != __NULL)
                {
                    sequence = *info.id_info.acc_time;
                }
            }
            
            if (sequence > 999999)
            {
                sequence = 999999;
            }

            HEX_BCD(p_tx, sequence);
            p_tx += 3;
            total_len = 3;
            total_len +=  4; //ID
            *tx_len = total_len;
            //return (total_len);
            return 0;

        }
        
    }

    last_no = DI.elem.di0;
    
    if (DI.elem.di3 == 0x00)//历史电能
    {
        temp = 12;
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 3); //读取费率数
        nd_get_index(instance, addr, &temp, 1); 
        if(temp > 12)
        {
            temp = 4;
        }

        
        oad = 0x00000200 + ((uint32_t)(DI.elem.di2 << 4) << 16); // 00XX000200

        rcsd_645req[0] = 1;
        if(DI.elem.di1 == 0xFF) //单个电能 块
        {
           rcsd_645len[0] = (temp + 1) * 4;
           rcsd_645len[1] = (temp + 1) * 4;
        }
        else
        {
            oad += ((uint32_t)(DI.elem.di1) + 1); // 00XX0002YY 
            rcsd_645len[0] =  4;
            rcsd_645len[1] =  4;
        }
        rcsd_645req[1] = oad;

        //单个数据项历史集合 当前 + 12个历史 
        if(DI.elem.di0 == 0xFF)
        {
            ten_read_cycles = 12;    // 先读当前电能 再循环 12次
            last_no = 1; 
        }
    }
    else //冻结或事件
    {
         if((!info.rd_645obj_len)||(!info.rd_645obj)||(info.rd_645obj[0] > 50))
        {
            return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
            //return -1; 
        }
        
        if (DI.elem.di1 == 0xFF) //块
        {
            ndv_memcopy(rcsd_645req, (uint8_t *)info.rd_645obj, 4 * (info.rd_645obj[0]+1));
            ndv_memcopy(rcsd_645len, (uint8_t *)info.rd_645obj_len, 2 * (info.rd_645obj[0]+1));
        }    
        else
        {
            //di1 偏移处理
            di1_index = DI.elem.di1;
    
    
            if(DI.elem.di3 == 0x05) //冻结  子项从0开始 
            {
                rcsd_645req[0] = 1;
                rcsd_645req[1] = info.rd_645obj[di1_index+1];
                //第一字节总长
                rcsd_645len[1] = info.rd_645obj_len[di1_index+1];     
    
                if((DI.elem.di1 == 0x10)) //转换 读取的 645关联对象     如复合 id
                {
                    di1_index = 11;
    
                    rcsd_645req[0] = 2;
                    rcsd_645req[1] = info.rd_645obj[di1_index+1];
                    rcsd_645req[2] = info.rd_645obj[di1_index+2];
                    //第一字节总长
                    rcsd_645len[1] = info.rd_645obj_len[di1_index+1];
                    rcsd_645len[2] = info.rd_645obj_len[di1_index+2];
    
                }
            }
            else //事件
            {
    
                if((info.package_num == 1)) // 过流   子项从1开始
                { 
                    rcsd_645req[0] = 1;
                    rcsd_645req[1] = info.rd_645obj[di1_index];
                    //第一字节总长
                    rcsd_645len[1] = info.rd_645obj_len[di1_index];  
    
                    if(DI.elem.di0 == 0xFF)
                    {
                        ten_read_cycles = 10;
                        last_no = 1;
                    }
                }
                else if((info.package_num == 0)) //掉电 无   子项 
                {
                    ndv_memcopy(rcsd_645req, (uint8_t *)info.rd_645obj, 4 * (info.rd_645obj[0]+1));
                    ndv_memcopy(rcsd_645len, (uint8_t *)info.rd_645obj_len, 2 * (info.rd_645obj[0]+1)); 
                }
                else
                {
    
                }
            }
        }
    }

    rsd_info.select = 9;
    rsd_info.last_no = last_no;
    rsd_info.start_mark = 0;
    rsd_info.end_mark = 0xffffffffU;
    rsd_info.mode = 0;


    if((ten_read_cycles == 0) || (ten_read_cycles > 12))
    {
        ten_read_cycles = 1;
        if ((ten_read_cycles == 12) && (DI.elem.di3 == 0x00))
        {
            //先读取当前电能
            nd_get(instance, rcsd_645req[1], ptr_tmp, 9, __NULL);
            data_len = fc_fun_Energy_645(instance, rcsd_645req[1], rcsd_645req[1],p_tx, ptr_tmp, RFC_READ);
            
            total_len += data_len;
            p_tx += data_len;            
        }
    }

    for(i=0; i < ten_read_cycles; i++)
    {

        data_len = record_data_package645(instance, ID, ptr_tmp,  &rsd_info, rcsd_645req, rcsd_645len,  info.package_type , __NULL);
        if (data_len < 0)
        {
            return response_x_byte_err_645(p_err, (uint8_t)(-data_len), 1,  tx_len);
            //return data_len;
        }


        if (info.package_type == PACKAGE645_FREEZE)
        {
            data_len = record_data_package645_packageAA(instance, p_tx, ptr_tmp,  data_len, &info);
            if (data_len < 0)
            {
                return response_x_byte_err_645(p_err, (uint8_t)(-data_len), 1,  tx_len);
                //return data_len;
            }
        }
        else
        {
            ndv_memcopy(p_tx, ptr_tmp,  data_len);
        }
        
        total_len += data_len;
        p_tx += data_len;
        
        rsd_info.last_no++;
    }

    total_len +=  4; //ID
    *tx_len = total_len;


    //return (total_len);
    return 0;

}

// -1 无数据
int load_search_copy_first_time(struct instance_t* instance, uint8_t *buf, struct load_rsd_info_t *load_rsd_info, struct record_storage_645_id_info_t  *info)
{
    uint32_t start_sec = 0, first_sec = 0, spec_sec = 0, end_sec = 0, cur_sec = 0, tmp_sec = 0;  
    struct circular_info_t circular_info = {0};
    uint8_t *ptr_tmp = buf;//instance->apdu_cache;  
    uint8_t tmpbuf[100];
    int ret = 0, orig_len = 0;

    memset((uint8_t *)&circular_info, 0, sizeof(circular_info));
    circular_info.start_mark = 0;
    circular_info.end_mark = 0xFFFFFFFF;
    circular_info.mode =   2;
    ret = nd_log_output(instance, info->id_info.save_index,  ptr_tmp, 1, &circular_info);

    if (circular_info.valid_num == 0)
    {
        return -1;
    }
    ret = nd_log_output(instance, info->id_info.save_index,  ptr_tmp, circular_info.unit_size, &circular_info);
    
    first_sec = buf_to_uint32_msb((ptr_tmp + CIRCULAR_OFFSET_TIME)); //首条记录时间
    //fc_fun_01_645(instance, uint32_t oad_req, uint32_t oad_src, uint8_t * des, uint8_t * src, enum format_conversion_mode mode, struct format_conversion_t * fc)

    //获取指定的首条时间  //mmhhddMMYY   
    orig_len = record_change_format_645(instance, tmpbuf, (ptr_tmp + 4 + CIRCULAR_OFFSET_SEQUENCE), 0x20210200, 0x20210200, RFC_READ);
    if(orig_len > 5)
    {
        return -1;
    }


    start_sec = 0; //负荷记录起始时间         默认0
    
    cur_sec = rtc_mktime(&instance->time); //系统当前时间
    
    spec_sec = load_rsd_info->rsd_info.start_mark; //指定 

    end_sec =  load_rsd_info->rsd_info.end_mark; 

    if (start_sec > cur_sec) return -1; //起始时间与当前时间
    if (start_sec > end_sec) return -1; //起始时间与指定结束时间
    if (first_sec > end_sec) return -1; //第一条比与指定结束
    if (spec_sec > cur_sec) return -1;  //指定起始时间与当前时间比
    if (start_sec > first_sec)          //起始时间与第一条负荷记录时间
    {
        first_sec = start_sec;
    }
    if (first_sec > spec_sec)           //第一条负荷记录时间与指定起始时间
    {
        tmp_sec = first_sec;     
    }
    else
    {
        tmp_sec = spec_sec;    
    }


    if (0 == ((tmp_sec - start_sec) % load_rsd_info->rsd_info.interval))
    {
        load_rsd_info->rsd_info.start_mark = tmp_sec;
    }
    else
    {
        load_rsd_info->rsd_info.start_mark = tmp_sec + (load_rsd_info->rsd_info.interval - ((tmp_sec - start_sec) % load_rsd_info->rsd_info.interval));

        if (load_rsd_info->rsd_info.start_mark > cur_sec)
        {
            return -1;
        }               
    }


    if (end_sec > cur_sec)//指定结束时间与当前时间比
    {
        load_rsd_info->rsd_info.end_mark = cur_sec;
    }
    else
    {
        load_rsd_info->rsd_info.end_mark = end_sec;
    }

    

    load_rsd_info->read_num = (load_rsd_info->rsd_info.end_mark - load_rsd_info->rsd_info.start_mark)/load_rsd_info->rsd_info.interval;
    if (((load_rsd_info->rsd_info.end_mark - load_rsd_info->rsd_info.start_mark)%load_rsd_info->rsd_info.interval) != 0)
    {
        load_rsd_info->read_num += 1;
    }

    if (0 == load_rsd_info->read_num)
    {
        return -1;
    }

    memset((uint8_t *)&circular_info, 0, sizeof(circular_info));

    circular_info.start_mark = load_rsd_info->rsd_info.start_mark;
    circular_info.end_mark = load_rsd_info->rsd_info.end_mark;
    circular_info.mode =   2;
    ret = nd_log_output(instance, info->id_info.save_index,  ptr_tmp, 1, &circular_info);
    if(ret < 0)
    {
        return -1;
    }
    
    ndv_memcopy(ptr_tmp,  tmpbuf, orig_len);    

    return orig_len;
}

//新负荷记录补数据.  src_buf 缓存长度 大于1500字节
int load_response_fill_645(struct instance_t* instance,  uint8_t *des, uint8_t *src_buf, uint32_t *rcsd_req, uint32_t *rcsd_src, uint16_t *capture_645len,  uint32_t ID, struct rsd_info_t *rsd_info)
{
    union dl645_DI_t DI;
    int one_len = 0;
    uint8_t *p = des;
    int ret;
    uint8_t *ptr_tmp = src_buf;

    struct circular_info_t circular_info = {0};
    struct record_storage_645_id_info_t record_id_info;

    DI.di = ID;
    

    ret = get_record_645_id_info(instance, ID, &record_id_info);
    if(ret < 0)
    {
        return 0; //无请求数据
    }


    if ((DI.elem.di1 == 0x06) || (DI.elem.di1 == 0x07))//电能补上一条数据 
    {
        circular_info.start_mark = 0;
        circular_info.end_mark = rsd_info->start_mark;
        circular_info.mode =   2;
        ret = nd_log_output(instance, record_id_info.id_info.save_index,  ptr_tmp, 1, &circular_info);

        circular_info.curr_num = circular_info.valid_num - 1;
        //circular_info.valid_num = circular_info.curr_num + 1;
        ret = nd_log_output(instance, record_id_info.id_info.save_index,  ptr_tmp, circular_info.unit_size, &circular_info);

        //开始打包
        one_len = record_package_01_645(instance, p, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
        if(( one_len < 0) || ( one_len > 544))
        {
            return 0;
        }        
        p += one_len;
        
    }
    else//瞬时量补FF
    {
        //预算打包长度
        one_len = get_package_01_len_645(instance, p, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
        if(( one_len < 0) || ( one_len > 544))
        {
            return 0;
        }        
        memset(p, 0xFF, one_len);
        p += one_len;
    }


    return (int)(p-des);
}

int search_load_scheme_No(struct instance_t* instance, uint32_t rcsd_req, uint8_t *scheme, uint32_t *period_sec, struct record_storage_645_id_info_t  *info)
{
    struct circular_info_t circular_info = {0};
    uint8_t i,k,scheme_no = 0;
    uint32_t tmpU32 = 0,rcsd_src[50] = {0};
    uint8_t period_buf[1 + 3 * 8];
    int ret = 0;

    //方案
    for(i = 0; i < 8; i++)
    {
        //获取配置关联对象
        get_capture_list(instance, (info->oad698 +(i<<13)), rcsd_src, 2);
        
        for(k=1; k < (rcsd_src[0] + 1); k++)
        {
            if (true == check_capture_exist(rcsd_req, ignore_OAD_Schem(rcsd_src[k])))
            {
                scheme_no = i;
            }
        }
    }

    //周期
    nd_get_index(instance, info->id_info.period_index , &period_buf[0], sizeof(period_buf));

    tmpU32 = buf_to_uint16_msb(&period_buf[scheme_no * 3 + 1]);  // scheme_no * 3 + 1    and   scheme_no * 3 + 2
    tmpU32 = tmpU32 * 60;
    if(tmpU32 == 0)
    {
        tmpU32 = 15*60;
    }
    
    *period_sec = tmpU32;
    *scheme = scheme_no;

    return 1;
}


//645读负荷数据
int load_data_package645(struct instance_t* instance, uint32_t ID, uint8_t* buf,  struct load_rsd_info_t *load_rsd_info, struct record_storage_645_id_info_t *record_id_info, uint32_t *rcsd_info, uint16_t *capture_645len,  uint8_t  package_type,   void *ctx)
{
#ifndef WIN32

    union dl645_DI_t DI;

    struct circular_info_t circular_info = {0};
    uint32_t *rcsd_req = (uint32_t *)rcsd_info; 
    uint8_t *ptr_tmp = instance->apdu_cache + 500;    
    uint32_t rcsd_src[50] = {0};
    uint16_t i, read_num, number = 0;
    uint16_t judge_size, one_len = 0,len = 0;
    uint8_t count = 0, rcsd_schem = 0, non_period = 0, check_result = 0;
    uint32_t oad_schem = 0, value = 0;
    uint8_t *p = (buf ); // 预留3个字节条数
    int ret = 0;
    struct rsd_info_t *rsd_info = &load_rsd_info->rsd_info;
    
    //检查 读取的记录型oad合法性

    DI.di = ID;

    //ret = get_record_645_id_info(instance, ID, &record_id_info);
    //if(ret < 0)
    //{
    //    return -2; //无请求数据
    //}


    //---获取条数
    
    circular_info.start_mark = rsd_info->start_mark;
    circular_info.end_mark = rsd_info->end_mark;
    circular_info.mode =   rsd_info->mode;

    switch(rsd_info->select)
    {
        case 0:
            //ret = GET_record_sel_0(instance, pipeline, mesg);
            //circular_info.curr_num--;
            break;
            
        case 1:
        case 2:
            if((1 == non_period) && ((rsd_info->mode == 2) || (rsd_info->mode == 3))) //方法1/2 非周期性类 按时间搜索时 由协议筛选
            {
                circular_info.mode = 0;
            }
            ret = nd_log_output(instance, record_id_info->id_info.save_index,  ptr_tmp, 1, &circular_info);
            circular_info.mode =   rsd_info->mode; 
            
            if ((0 == non_period) && (rsd_info->select == 1) && ((rsd_info->mode == 2) || (rsd_info->mode == 3))) //方法1  周期性冻结 按时间搜索 返回最近一条
            {
                if(circular_info.valid_num > 1)
                {
                    circular_info.valid_num = 1;
                }
            }
            break;
            
        case 9: //上n次
            ret = nd_log_output(instance, record_id_info->id_info.save_index,  ptr_tmp, 1, &circular_info);
            if ((rsd_info->last_no == 0) || (rsd_info->last_no > circular_info.valid_num))
            {
                return record_response_null_645(instance, p, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
            }
            circular_info.curr_num = circular_info.valid_num - rsd_info->last_no;
            circular_info.valid_num = circular_info.curr_num + 1;
            break;
        default:
            break;
    }

    
    if (circular_info.unit_size > (cache_max-500))
    {
        return -DAR_BUFFER_OVERFLOW;
        //return record_response_null_645(instance,  p, ptr_tmp, rcsd_req, rcsd_src, capture_645len, ID);
    }

    if (circular_info.valid_num == 0)
    {
        //return record_response_null_645(instance, p, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
    }
    //------------
    
    //获取关联对象
    get_capture_list(instance, record_id_info->oad698, rcsd_src, 3);

    check_result = 0;
    //read_num = circular_info.valid_num;
    read_num = load_rsd_info->read_num;
    for (i = 0; i < read_num; i++)
    {
        if(number == 0)
        {
            //获取指定的首条时间  //mmhhddMMYY   
            ptr_tmp[0] = 0x1C;
            uint32_to_buf_msb(circular_info.start_mark, &ptr_tmp[1]);
            len = record_change_format_645(instance, p+len, ptr_tmp, 0x20210200, 0x20210200, RFC_READ);
            if(len > 5)
            {
                return -DAR_BUFFER_OVERFLOW;
            }
        }

        v85xxp_clear_watchdog();
        //获取1条记录内容
        ret = nd_log_output(instance, record_id_info->id_info.save_index,  ptr_tmp, circular_info.unit_size, &circular_info);
        if(ret < 0)
        {
            return -DAR_BUFFER_OVERFLOW;
            //return record_response_null_645(instance, p, ptr_tmp, rcsd_req, rcsd_src, capture_645len, ID);
        }
        

        //-----长度判断
        if (one_len == 0)
        {   
            //预算打包长度
            one_len = get_package_01_len_645(instance, p+len, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
        }
        
        judge_size = (p - buf + len + one_len);
        
        if((judge_size) > (cache_max-500)) 
        {
            return -DAR_BUFFER_OVERFLOW;
        }
        //-----------

        //----协议筛选
        if (rsd_info->interval_special)
        {
            rsd_info->interval = rsd_info->interval_special;
        }
        if (rsd_info->interval > 0)
        {
            value = get_interval_basic_value(ptr_tmp, rsd_info);
#if 0
            if (number == 0)
            {
                circular_info.start_mark = value;
            }
#endif            
            if (value == circular_info.start_mark)
            {
                one_len = record_package_01_645(instance, p+len, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
            }
            else //补数据
            {
                one_len = load_response_fill_645(instance, p+len, ptr_tmp, rcsd_req, rcsd_src, capture_645len, ID, rsd_info);
            }
            //if(one_len < 0) one_len = 0;
            len += one_len;
            number++;
            circular_info.start_mark += rsd_info->interval;
        }
        else
        {
            //协议判断是否符合要求
            check_result = select_record_time_check(ptr_tmp, non_period, rsd_info);
            if ( check_result > 0 )
            {
                if ((check_result == 1) && (number >= 1)) // 至少已搜索到一条
                {
                    len -= one_len; // 
                }
            
                //开始打包
                one_len = record_package_01_645(instance, p+len, ptr_tmp, rcsd_req, rcsd_src, capture_645len,  ID);
                len += one_len;
                number++;
            }
        }
        
        circular_info.curr_num = 0; 
        circular_info.valid_num = 0; //

    }      
        

    return (int)len;
#endif
}


//新负荷记录读取
// 返回0 正常应答        返回＞0 异常应答

int read_06( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx, uint8_t *tx_len)
{
    union dl645_DI_t DI;
    struct record_storage_645_id_info_t  info;
    int ret;
    uint32_t rcsd_645req[51] = {0};
    uint16_t rcsd_645len[51] = {0};
    struct rsd_info_t rsd_info;
    struct load_rsd_info_t load_rsd_info;
    int total_len = 0, data_len = 0;
    uint8_t *ptr_tmp = instance->apdu_cache; //至少保证 500字节
    uint8_t di0_index = 0;

    uint8_t *p_err = (p_tx - 4);
    uint32_t sequence = 0;
    uint8_t i,temp = 0xFF;
    uint32_t addr = 0, oad = 0;

    uint16_t ptr_offset = 0;
    uint8_t read_num = 0, scheme = 0;
    uint8_t *p_rx_cache = p_rx + 4; // 
    struct tm tm_search;
    uint32_t TSStart = 0,TSEnd = 0,TRStart = 0,TREnd = 0;
    uint32_t period = 15*60;
    
    
    memset(&load_rsd_info, 0,sizeof(load_rsd_info));

    
    DI.di = ID;

    ret = get_record_645_id_info(instance, ID, &info);
    if (ret <= 0)
    {
        return response_x_byte_err_645(p_err, RS_ERR_ID, 1,  tx_len);
    }

    // ---------------------
    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LoadRec_Mode); //读取负荷模式字
    nd_get_index(instance, addr, &temp, 1); 
    info.mark_para = temp;
    if(0 == (info.mark_para & (1 << info.mark_bit)))
    {
        return response_x_byte_err_645(p_err, RS_ERR_ID, 1,  tx_len);
    }

    // -------------------- --------------------
    if((!info.rd_645obj_len)||(!info.rd_645obj)||(info.rd_645obj[0] > 50))
    {
        return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
    }

    if (DI.elem.di0 == 0xFF) //块  三相
    {
        ndv_memcopy(rcsd_645req, (uint8_t *)info.rd_645obj, 4 * (info.rd_645obj[0]+1));
        ndv_memcopy(rcsd_645len, (uint8_t *)info.rd_645obj_len, 2 * (info.rd_645obj[0]+1));
    }    
    else
    {
        //di0 偏移处理
        di0_index = DI.elem.di0;
        if((info.package_num == 1)) // 子项从1开始
        { 
            rcsd_645req[0] = 1;
            rcsd_645req[1] = info.rd_645obj[di0_index];
            //第一字节总长
            rcsd_645len[1] = info.rd_645obj_len[di0_index];  

        }
        else if((info.package_num == 0xFF)) //常规子项 
        {
            rcsd_645req[0] = 1;
            rcsd_645req[1] = info.rd_645obj[di0_index+1];
            //第一字节总长
            rcsd_645len[1] = info.rd_645obj_len[di0_index+1];     
        }
        else
        {
            return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
        }
    }
    // -------------------- --------------------


    //搜索符合的方案及周期
    ret = search_load_scheme_No(instance, rcsd_645req[1], &scheme, &period, &info);
    if (ret < 0)
    {
        scheme = 0;
        period = 15*60; 
    }

    //添加方案信息
    info.oad698 |= ((uint32_t)scheme << 13);
    get_record_id_info(instance, info.oad698, &info.id_info);
    for(i = 1; i < (rcsd_645req[0]+1); i++)
    {
        rcsd_645req[i] |= ((uint32_t)scheme << 13);
    }
    
    //获取rsd_info ---------------------------
    read_num = BCD_BYTE(*p_rx_cache++);
    if(read_num > 99)
    {
        read_num = 99;
    }
    
    
    tm_search.tm_sec = 0;
    tm_search.tm_min = BCD_BYTE(*p_rx_cache++);
    tm_search.tm_hour = BCD_BYTE(*p_rx_cache++);
    tm_search.tm_mday = BCD_BYTE(*p_rx_cache++);
    tm_search.tm_mon = BCD_BYTE(*p_rx_cache++) - 1;
    tm_search.tm_year = BCD_BYTE(*p_rx_cache++) + 2000 - 1900;

    if (false == rtc_tm_valid(&tm_search))
    {
        return response_x_byte_err_645(p_err, RS_ERR_OTHERS, 1,  tx_len);
    }


    TSStart = rtc_mktime(&tm_search);
    TSEnd =  TSStart + read_num * period;

    load_rsd_info.rsd_info.select = 2;
    load_rsd_info.rsd_info.last_no = 0;
    load_rsd_info.rsd_info.start_mark = TSStart;
    load_rsd_info.rsd_info.end_mark = TSEnd;
    load_rsd_info.rsd_info.interval = period;
    load_rsd_info.rsd_info.mode = 2;   
    //---------------------------

    data_len = load_search_copy_first_time(instance, ptr_tmp, &load_rsd_info, &info);
    if (data_len < 0)
    {
        *tx_len = 0; //无数据内容
        return 0;
    }
    //ndv_memcopy(p_tx, ptr_tmp,  data_len);
    //total_len += data_len;
    //p_tx += data_len;


    //for(i=0; i < read_num; i++)
    {
        data_len = load_data_package645(instance, ID, ptr_tmp,  &load_rsd_info, &info, rcsd_645req, rcsd_645len,  info.package_type , __NULL);
        if (data_len < 0)
        {
            return response_x_byte_err_645(p_err, (uint8_t)(-data_len), 1,  tx_len);
            //return data_len;
        }


        ndv_memcopy(p_tx, ptr_tmp,  data_len);
        
        
        total_len += data_len;
        p_tx += data_len;
       
    }

    total_len +=  4; //ID
    *tx_len = total_len;


    //return (total_len);
    return 0;


}


