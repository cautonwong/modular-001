#include "kernel\nidavellir.h"
//dependencies
    //1.user define
    //2.mesg[秒]

#define   HoliDay_Num         0x0000
#define   HoliDay_Table       0x0001
#define   Weekend             0x0003
#define   Season_Table_Num    0x0004
#define   MaxSeasonNum        0x0005
#define   Season_Table0       0x0006
#define   TOU_TABLE_LEN       144
#define   Day_Tou_Num         0x0007
#define   MaxTouNum           14
#define   Day_Table00         0x0008



struct fee_t
{
    unsigned char   DayTableNo;//日表序号
    unsigned char	TOUNo;//费率序号
    unsigned char   STEPNo;//阶梯序号
    unsigned long	TOUPrice;//费率电价
    unsigned long	STEPPrice;//阶梯电价
    unsigned char   SeasonTable;//时区表主副表序号
    unsigned char   DayTable;//日表主副表序号
    unsigned char   TouPriceTable;//费率电价表主副表序号
    unsigned char   StepTable;//阶梯表主副表序号
    unsigned char	StepSeason;//阶梯时区表主副表序号
    unsigned long	Price;//当前电价
    unsigned char	StepNum;//阶梯数
};

struct mbase_control_block_t
{
    int id;
    //电量数据
    //瞬时量数据
    //需量数据
    //事件负荷冻结数据
    //费率数据
    struct fee_t Fee;
    //费用数据
};








unsigned char FeeTable_CmpFind(unsigned char Num, unsigned char* Buffer, unsigned int Now)
{
    unsigned char i;
    unsigned int Int, Int1;

    //比较查找
    for (i = 0; i < Num; i++)
    {
        Int = ((unsigned int)Buffer[3 * i + 2] << 8) + (unsigned int)Buffer[3 * i + 1];
        Int1 = ((unsigned int)Buffer[3 * ((i + 1) % Num) + 2] << 8) + (unsigned int)Buffer[3 * ((i + 1) % Num) + 1];
        if (Int < Int1)
        {
            if ((Now >= Int) && (Now < Int1))
                break;
        }
        else if (Int == Int1)
        {
            if (Now >= Int)
                break;
        }
        else
        {
            if ((Now >= Int) || (Now < Int1))//过0点判断
                break;
        }
    }
    if (i >= Num)
        i = Num - 1;

    if (Num == 1) return 0;
    else return i;
}

void GetFeeTab(struct instance_t* instance , struct pipeline_t * pipeline)
{
    unsigned char i, Weekstate, Num0;
    unsigned int Num, Num1 = 0;
    unsigned char* Buffer = instance->cache;
    struct mbase_control_block_t * mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    unsigned short Now;

    //公假日表
    __read((unsigned char*)&Num, HoliDay_Num, 2);
    if (Num > 14) Num = 14;
    while (Num > 0)
    {
        if (Num > 25)
            Num0 = 25;
        else
            Num0 = (unsigned char)Num;
        __read(Buffer, HoliDay_Table + Num1 * 4, 4 * Num0);
        for (i = 0; i < Num0; i++)
        {
            if ((Buffer[4 * i + 3] == instance->time.tm_year) && (Buffer[4 * i + 2] == instance->time.tm_mon) && (Buffer[4 * i + 1] == instance->time.tm_mday))
            {
                mbase_control_block->Fee.DayTableNo = Buffer[4 * i + 0];//年月日相同，获取日时段表号
                return;
            }
        }
        Num -= Num0;
        Num1 += Num0;
    }

    //周休日表
    __read(Buffer, Weekend, 2);
    Weekstate = 0x01;
    Weekstate <<= instance->time.tm_wday;
    if ((Weekstate & Buffer[0]) == 0)//0表示休息日
    {
        mbase_control_block->Fee.DayTableNo = Buffer[1];
        return;
    }

    //时区表
    if (mbase_control_block->Fee.SeasonTable > 1) mbase_control_block->Fee.SeasonTable = 0;
    __read((unsigned char*)&Num, Season_Table_Num, 1);//读取时区表个数
    if (Num == 0)
        mbase_control_block->Fee.DayTableNo = 1;
    else
    {
        if (Num > MaxSeasonNum) Num = MaxSeasonNum;
        __read(Buffer, Season_Table0 + TOU_TABLE_LEN * mbase_control_block->Fee.SeasonTable, (unsigned char)Num * 3);//读取当前时区表
        Now = ((unsigned int)instance->time.tm_mon << 8) + (unsigned int)instance->time.tm_mday;
        i = FeeTable_CmpFind(Num, Buffer,  Now);//比较查找时区表
        mbase_control_block->Fee.DayTableNo = Buffer[3 * i];
    }
}

void GetTOUFee(struct instance_t* instance, struct pipeline_t* pipeline)
{
    unsigned char i, Num;

    unsigned char* Buffer = instance->cache;
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    unsigned short Now;



    if (mbase_control_block->Fee.DayTable > 1) mbase_control_block->Fee.DayTable = 0;
    if (mbase_control_block->Fee.DayTableNo == 0) mbase_control_block->Fee.DayTableNo = 1;
    if (mbase_control_block->Fee.DayTableNo > 14) mbase_control_block->Fee.DayTableNo = 14;
    __read((unsigned char*)&Num, Day_Tou_Num, 1);
    if (Num == 0)
        mbase_control_block->Fee.TOUNo = 1;
    else
    {
        if (Num > MaxTouNum) Num = MaxTouNum;
        __read(Buffer, Day_Table00 + TOU_TABLE_LEN * mbase_control_block->Fee.DayTable + ((unsigned int)(mbase_control_block->Fee.DayTableNo - 1)) * 3 * MaxTouNum, 3 * Num);//¶ÁÈ¡µ±Ç°ÈÕÊ±¶Î±í
        Now = ((unsigned int)instance->time.tm_hour << 8) + (unsigned int)instance->time.tm_min;
        i = FeeTable_CmpFind(Num, Buffer, Now);//比较查找时段表
        mbase_control_block->Fee.TOUNo = BCD_BYTE(Buffer[3 * i]);
        FeeNo = mbase_control_block->Fee.TOUNo;
        if ((mbase_control_block->Fee.TOUNo == 0) || (mbase_control_block->Fee.TOUNo > 12))
            mbase_control_block->Fee.TOUNo = 1;
    }
}


void TouFeePro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{

    GetFeeTab(instance, pipeline);

    GetTOUFee(instance, pipeline);

}


int meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int r = 0;
    //cmd len data cmd 接入校验
    //调用领域层完成业务  要反映用例场景
    switch (mesg->ctrl)
    {
    case 0x86://采样的外设数据抵达
        //refresh_control_data(instance,pipeline,mesg);
        break;
    case 0x87://外设控制命令  比如校表  可以异步的话就单独弄个tx_entry?
        //controlDevice(instance,pipeline,mesg);
        break;
    case 0x88://control_block控制命令 比如清零
        //controlData(instance,pipeline,mesg);
        break;
    case 0x89://秒信号  刷新内部计时器  比如需量和事件
        //controlStatus(instance,pipeline,mesg);
        TouFeePro(instance, pipeline , mesg);
        break;
    default:
        break;
    }

    return 0;
}

