
 
const unsigned char CODE PI_poweroffevent[] = { 0x02, 0x01, 0x00, 0x93, 0x01, 0x10, 0x93,}; // 电能表掉电事件 
const unsigned char CODE PI_programevent[] = { 0x00 }; // 电能表编程事件 
// 电能表清零事件 
const unsigned char CODE PI_meterclearenvet[] = 
{ 
    0x02, 
    0x01, 0x00, 0x93, 0x01, 0x10, 0x93,
}; 

const unsigned char CODE PI_eventclearevent[] = { 0x00 }; // 电能表事件清零事件 
 // 电能表校时事件 
const unsigned char CODE PI_timesetevent[] = { 0x02, 0x01, 0x00, 0x93, 0x01, 0x10, 0x93, };
const unsigned char CODE PI_daytabproevent[] = { 0x00 }; // 电能表时段表编程事件 
const unsigned char CODE PI_seasonproevent[] = { 0x01, 0x01, 0x1c, 0x4b, }; // 电能表时区表编程事件 
const unsigned char CODE PI_weekproevent[] = { 0x01, 0x01, 0x1d, 0x4b, }; // 电能表周休日编程事件 
const unsigned char CODE PI_billproevent[] = { 0x01, 0x01, 0x1e, 0x4b, }; // 电能表结算日编程事件 
// 电能表开盖事件 
const unsigned char CODE PI_coveropenevent[] = 
{ 
    0x04,
    0x01, 0x00, 0x93, 0x01, 0x10, 0x93, 
    0x04, 0x00, 0x93, 0x04, 0x10, 0x93, 
}; 


// 电能表跳闸事件 
const unsigned char CODE PI_relayoffevent[] = 
{ 
    0x02,
    0x01, 0x00, 0x93, 0x01, 0x10, 0x93, 
}; 
// 电能表合闸事件 
const unsigned char CODE PI_relayonevent[] = 
{ 
    0x02,
    0x01, 0x00, 0x93, 0x01, 0x10, 0x93, 
}; 
const unsigned char CODE PI_holidayproevent[] = { 0x00 }; // 电能表节假日编程事件 
const unsigned char CODE PI_comb0proevent[] = { 0x01, 0x01, 0x1f, 0x4b, }; // 电能表有功组合方式编程事件 
const unsigned char CODE PI_comb1proevent[] = { 0x02, 0x01, 0x20, 0x4b, 0x01, 0x21, 0x4b, }; // 电能表无功组合方式编程事件 
const unsigned char CODE PI_feeproevent[] = { 0x02, 0x01, 0x22, 0x4b, 0x01, 0x23, 0x4b, }; // 电能表费率参数表编程事件 
const unsigned char CODE PI_stepproevent[] = { 0x02, 0x01, 0x24, 0x4b, 0x01, 0x25, 0x4b, }; // 电能表阶梯表编程事件 
const unsigned char CODE PI_keyproevent[] = { 0x01, 0x01, 0x03, 0x5a, }; // 电能表密钥更新事件 
const unsigned char CODE PI_carderrevent[] = { 0x00 }; // 电能表异常插卡事件 
// 电能表购电记录 
const unsigned char CODE PI_buyevent[] = 
{ 
    0x06, 
    0x04, 0x04, 0x5f, 0x01, 0xe0, 0x6f, 0x01, 0xe6, 0x6f, 0x01, 0xe2, 0x6f, 0x04, 0xe1, 0x6f, 0x04, 0x07, 0x5f, 
}; 
const unsigned char CODE PI_refundevent[] = { 0x00 }; // 电能表退费记录 

  // 电能表负荷开关误动作事件 
const unsigned char CODE PI_relayerrevent[] = 
{ 
    0x05, 
    0x01, 0x1a, 0x4b, 0x01, 0x00, 0x93, 0x01, 0x10, 0x93, 0x04, 0x00, 0x93, 0x04, 0x10, 0x93,
};
const unsigned char CODE PI_powererrevent[] = { 0x02, 0x01, 0x00, 0x93, 0x01, 0x10, 0x93,  }; // 电能表电源异常事件 

const unsigned char CODE PI_timeerrevent[] = { 0x04, 0x01, 0x00, 0x93, 0x01, 0x10, 0x93, 0x04, 0x00, 0x93,0x04, 0x10, 0x93, }; // 电能表时钟故障事件 
const unsigned char CODE PI_meterchiperrevent[] = { 0x02, 0x01, 0x00, 0x93, 0x01, 0x10, 0x93, }; // 电能表计量芯片故障事件

const unsigned char CODE PI_broadtimesetevent[] = { 0x02,  0x01, 0x00, 0x93, 0x01, 0x10, 0x93, };  // 广播校时事件 
const unsigned char CODE PI_broadtimeseterrevent[] = { 0x02,  0x01, 0x00, 0x93, 0x01, 0x10, 0x93, };  // 广播校时异常事件事件 

// 电能表过流事件 
const unsigned char CODE PI_iovererrevent[] = 
{ 

    0x08, 
    0x01, 0x00, 0x93, 0x01, 0x10, 0x93, 
    0x01, 0x00, 0x72, 0x01, 0x01, 0x72, 0x01, 0x06, 0x72, 0x01, 0x0c, 0x72, 
    0x04, 0x00, 0x93, 0x04, 0x10, 0x93, 

}; 

const unsigned char CODE PI_mdovererrevent[] = { 0x00 }; // 电能表无功需量超限事件 
const unsigned char CODE PI_iberrevent[] = { 0x02, 0x01, 0x04, 0xC0, 0x01, 0x14, 0xC0, };// 零线电流异常事件默认关联对象 
// 事件上报方式为 主动
const unsigned char CODE PI_report_active[] = { 0x00 };
// 事件上报方式为 跟随
const unsigned char CODE PI_reportfollow[] = { 0x01 }; 


// DL/T 698.45 事件记录 - 配置参数-判定延时时间  0x50C0 - 0x50FF 
const unsigned char CODE PI_poweroffdly[] = { 0x01 }; // 电能表掉电事件 
const unsigned char CODE PI_relayerrdly[] = { 0x3c }; // 电能表负荷开关误动作事件 （数据快照未使用）
const unsigned char CODE PI_powererrdly[] = { 0x3c }; // 电能表电源异常事件 （数据快照未使用）
const unsigned char CODE PI_rtcerrdly[] = { 0x3c }; // 电能表时钟故障事件 （数据快照未使用）
const unsigned char CODE PI_chiperrdly[] = { 0x3c }; // 电能表计量芯片故障事件 （数据快照未使用）

// DL/T 698.45 分项事件记录 - 过流事件-配置参数  0x55EC - 0x55EF 
const unsigned char CODE PI_iovervalue[] = { 0x80, 0xfc, 0x0a, 0x00 }; // 电能表过流事件 - 电流触发下限 
const unsigned char CODE PI_ioverdly[] = { 0x3c }; // 电能表过流事件 - 判定延时时间 

const unsigned char CODE PI_iberrvalue[] = { 0x88, 0x13 }; // 零线电流异常事件不平衡限值 
const unsigned char CODE PI_iberrdly[] = { 0x3c }; // 零线电流异常事件 - 判定延时时间 
const unsigned char CODE PI_iberrvalue_I[] = { 0x10, 0x27, 0x00, 0x00 }; // 零线电流异常事件 - 电流触发下限 



// 冻结 冻结周期： 8组数据依次表示8种方案（每种方案3个字节，第一个字节为0，表示该周期值无效，非零值表示该周期值有效，后面2个字节表示周期值） 0x4900 - 0x490F 
const unsigned char CODE PI_instant_freeze[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 瞬时冻结 
const unsigned char CODE PI_min_freeze[] = { 7,   0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 分冻结 
const unsigned char CODE PI_hour_freeze[] = { 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 小时冻结 
const unsigned char CODE PI_day_freeze[] = { 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 日冻结 
const unsigned char CODE PI_bill_freeze[] = { 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 结算日冻结 
const unsigned char CODE PI_monthl_freeze[] = { 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 月冻结 
const unsigned char CODE PI_seasonchg_freeze[] = { 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 时区表切换冻结 
const unsigned char CODE PI_daytabchg_freeze[] = { 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 日时段表切换冻结 
const unsigned char CODE PI_feechg_freeze[] = { 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 费率电价切换冻结 
const unsigned char CODE PI_stepchg_freeze[] = { 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 阶梯切换冻结 
const unsigned char CODE PI_stepbill_freeze[] = { 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 阶梯结算冻结 

// 冻结 关联对象 0x4910 - 0x491F 
// 瞬时冻结 
const unsigned char CODE PI_instant_freeze_item[] = 
{ 
    0x03, 
    0x00, 0x2A, 0x70, 0x00, 0x2B, 0x70,  0x00, 0x06, 0x72,
}; 
// 分冻结 
const unsigned char CODE PI_min_freeze_item[] = 
{ 
   
    7,
    0x00, 0x50, 0x70,   // 正向有功电能总 高精度   
    0x00, 0x60, 0x70,   // 反向有功电能总 高精度   
    0x00, 0x00, 0x72, 0x00, 0x01, 0x72, 
    0x00, 0x14, 0xC0,  // 零线电流  只有直接式表支持  
    0x00, 0x06, 0x72,
    0x00, 0x0c, 0x72,
  
}; 
 // 小时冻结 
const unsigned char CODE PI_hour_freeze_item[] = 
{ 
    0x02, 
    0x00, 0x50, 0x70, 0x00, 0x60, 0x70,
};
 // 日冻结 
const unsigned char CODE PI_day_freeze_item[] = 
{ 
    5,
    0x00, 0x2A, 0x70,   // 正向有功电能 高精度group   

    0x00, 0x2B, 0x70,   // 反向有功电能 高精度group   

    0x00, 0x06, 0x72,   // 有功功率块   

    0x00, 0xdf, 0x6f, 
    0x00, 0xd4, 0x6f, 
}; 




 //*卡表专有日冻结关联对象
 const unsigned char CODE day_freeze_prepay_tab[] =
{
     0xdf, 0x6f, 0xd4, 0x6f,
};
 //*直接式专有分钟冻结关联对象
 const unsigned char CODE min_freeze_direct_tab[] =
{
     0x14, 0xC0, // 0x00, 0xC8, 
};
// 结算日冻结 
const unsigned char CODE PI_bill_freeze_item[] = 
{ 
 
    0x04, 
    0x00, 0x29, 0x70, 0x00, 0x2A, 0x70, 0x00, 0x2B, 0x70, 
    0x00, 0x00, 0x94,

}; 
 // 月冻结 
const unsigned char CODE PI_month_freeze_item[] = 
{ 

   0x03,
   0x00, 0x29, 0x70, 0x00, 0x2A, 0x70, 0x00, 0x2B, 0x70, 
  
  
};
// 时区表切换冻结 
const unsigned char CODE PI_seasonchg_freeze_item[] = 
{ 
    0x03, 
    0x00, 0x2A, 0x70, 0x00, 0x2B, 0x70, 0x00, 0x06, 0x72, 
};  
// 日时段表切换冻结 
const unsigned char CODE PI_daytabchg_freeze_item[] = 
{ 
    0x03, 
    0x00, 0x2A, 0x70, 0x00, 0x2B, 0x70, 0x00, 0x06, 0x72, 
}; 
// 费率电价切换冻结 
const unsigned char CODE PI_feechg_freeze_item[] = 
{ 
    0x03, 
    0x00, 0x2A, 0x70, 0x00, 0x2B, 0x70, 0x00, 0x06, 0x72, 
}; 
// 阶梯切换冻结 
const unsigned char CODE PI_stepchg_freeze_item[] = 
{ 
    0x03, 
    0x00, 0x2A, 0x70, 0x00, 0x2B, 0x70, 0x00, 0x06, 0x72, 
}; 
// 阶梯结算冻结 
const unsigned char CODE PI_stepbill_freeze_item[] = 
{ 
    0x01, 0x00, 0x00, 0x95,
}; 
// 冻结 存储深度 0x2F20 - 0x2F2F 
const unsigned char CODE PI_instant_freeze_deep[] = { 0x03, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, }; // 瞬时冻结 
const unsigned char CODE PI_min_freeze_deep[] = { 0xE0, 0x88, 0xE0, 0x88, 0xE0, 0x88, 0xE0, 0x88, 0xE0, 0x88, 0xE0, 0x88, 0xE0, 0x88, 0xE0, 0x88, };// 分钟冻结
const unsigned char CODE PI_hour_freeze_deep[] = { 0x02, 0xfe, 0x00, 0xfe, 0x00, }; // 小时冻结 
const unsigned char CODE PI_hour_freeze_deep[] = 
{   
    5,
    0x6D, 0x01, 0x6D, 0x01, 0x6D, 0x01, 0x6D, 0x01, 0x6D, 0x01, 
}; // 日冻结 

const unsigned char CODE PI_bill_freeze_deep[] = { 0x04, 0x0c, 0x00, 0x0c, 0x00,0x0c, 0x00,0x0c, 0x00, }; // 结算日冻结 
const unsigned char CODE PI_month_freeze_deep[] = { 0x03, 0x18, 0x00, 0x18, 0x00,0x18, 0x00, }; // 月冻结 
const unsigned char CODE PI_seasonchg_freeze_deep[] = { 0x03, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,  }; // 时区表切换冻结 
const unsigned char CODE PI_daytabchg_freeze_deep[] = { 0x03, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,  }; // 日时段表切换冻结 
const unsigned char CODE PI_feechg_freeze_deep[] = { 0x03, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,  }; // 费率电价切换冻结 
const unsigned char CODE PI_stepchg_freeze_deep[] = { 0x03, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,  }; // 阶梯切换冻结 
const unsigned char CODE PI_stepbill_freeze_deep[] = { 0x01, 0x04, 0x00, }; // 阶梯结算冻结 

//参数初始化
const unsigned char CODE PI_meteraddress[] = { 6, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 }; // 通信地址 
const unsigned char CODE PI_meternumbers[] = { 8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }; // 表号 
const unsigned char CODE PI_customerid[] = { 6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 客户编号 
const unsigned char CODE PI_location[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // 设备地理位置 
const unsigned char CODE PI_safeenable[] = { 1 }; // 安全模式使能 


// LCD参数 
const unsigned char CODE PI_displayalltime[] = { 5 };      // LCD参数 上电全显时间 
const unsigned char CODE PI_bg_button_time[] = { 0x3c, 0x00 };     // LCD参数 按键背光点亮时间 
const unsigned char CODE PI_bg_ir_time[] = { 0x0a, 0x00 };     // LCD参数 显示查看背光点亮时间 
const unsigned char CODE PI_buttontime[] = { 0x1e, 0x00 };     // LCD参数 无电按键屏幕驻留最大时间 
const unsigned char CODE PI_engdot[] = { 2 };      // LCD参数 显示电能小数位数 
const unsigned char CODE PI_powerdot[] = { 4 };      // LCD参数 显示功率（最大需量）小数位数 
const unsigned char CODE PI_display12means[] = { 0 };      // LCD参数 液晶①②字样意义 
#define PI_display12means_copy PI_display12means  // LCD参数 液晶①②字样意义 
const unsigned char CODE PI_changetimes[] = { 0, 0, 0, 0, 0 }; // 两套时区表切换时间 
#define PI_changetimes_copy1  PI_changetimes // 两套日时段切换时间 
#define PI_changetimes_copy2  PI_changetimes // 两套分时费率切换时间 
#define PI_changetimes_copy3  PI_changetimes // 两套阶梯电价切换时间 
// 时区时段数 
const unsigned char CODE PI_somenum[] = { 1, 2, 8, 4 }; // 年时区数14/日时段表数8/日时段数14/费率数63 
const unsigned char CODE PI_holidaynum[] = { 0 }; // 公共假日数256 
const unsigned char CODE PI_stepnum[] = { 0 }; // 阶梯数 
const unsigned char CODE PI_keynum[] = { 32 }; // 密钥总条数 
const unsigned char CODE PI_meterchipnum[] = { 1 }; // 计量元件数 
const unsigned char CODE PI_boardtimesetmode[] = { 0x01 }; // 广播校时出厂默认明文模式 
const unsigned char CODE PI_boardtimesettime1[] = { 0x3C, 0x00 }; // 广播校时出厂默认密文响应限值 
const unsigned char CODE PI_boardtimesettime2[] = { 0x2C, 0x01 }; // 广播校时出厂默认明文响应限值 

const unsigned char CODE PI_daytabchgflag[] = { 0x00 }; // 主备时段表切换标志 

const unsigned char CODE PI_holidaytab[] = { 0xff,0xff,0xff,0xff,0x00 }; // 公共假日表 
const unsigned char CODE PI_weekword[] = { 0x7f }; // 周休日特征字 
const unsigned char CODE PI_weekofnum[] = { 1 }; // 周休日釆用的日时段表号 
const unsigned char CODE PI_currseasontab[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }; // 当前套时区表 
//const unsigned char CODE PI_currseasontab[] = { 1, 1, 1, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 1, 2, 6, 1, 2 }; // 当前套时区表 
#define PI_currseasontab_copy  PI_currseasontab // 备用套时区表 
//  备用套单个索引 
const unsigned char CODE PI_bakitem1[] = {3,17,1,17,1,17,1};
const unsigned char CODE PI_bakitem2[] = {3,17,1,17,1,17,1};
#define PI_bakitem1_copy PI_bakitem1
#define PI_bakitem2_copy PI_bakitem2
     
// 当前套日时段表 
const unsigned char CODE PI_currdaytab[] = { 8, 0, 0, 1, 3, 0, 2, 6, 0, 3, 9, 0, 4, 12, 0, 1, 15, 0, 2, 18, 0, 3, 21, 0, 4, 21, 0, 4, 21, 0, 4, 21, 0, 4, 21, 0, 4, 21, 0, 4, 21, 0, 4 };
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
// 备用套日时段表 
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab
#define PI_currdaytab_copy PI_currdaytab

//时段表默认，设置8个时段，其余补0xff  
const unsigned char CODE inital_4B50[] = { 8,
DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 0,  DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 1,
DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 3,  DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 2,
DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 6,  DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 3,
DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 9,  DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 4,
DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 12, DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 1,
DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 15, DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 2,
DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 18, DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 3,
DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 21, DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 4,
0xff        , 0xff,             0xff,0xff,               0xff,0xff,              0xff,0xff,
0xff        , 0xff,             0xff,0xff,               0xff,0xff,              0xff,0xff,
0xff        , 0xff,             0xff,0xff,               0xff,0xff,              0xff,0xff,
0xff        , 0xff,             0xff,0xff,               0xff,0xff,              0xff,0xff,
0xff        , 0xff,             0xff,0xff,               0xff,0xff,              0xff,0xff,
0xff        , 0xff,             0xff,0xff,               0xff,0xff,              0xff,0xff,

//DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 21, DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 4,
//DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 21, DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 4,
//DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 21, DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 4,
//DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 21, DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 4,
//DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 21, DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 4,
//DT_STRUCTURE, 3, DT_INTEGER_UNSIGNED, 21, DT_INTEGER_UNSIGNED, 0, DT_INTEGER_UNSIGNED, 4,
};





// 当前套费率电价 
const unsigned char CODE PI_currfeepricetab[] =
{
    0x88, 0x13, 0x00, 0x00, //5000
    0x24, 0x13, 0x00, 0x00, //4900
    0xc0, 0x12, 0x00, 0x00, //4800
    0x5c, 0x12, 0x00, 0x00, //4700
    //4700
    0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00,
    0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00,
    0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00,
    0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00,
    0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00,
    0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00, 0x5c, 0x12, 0x00, 0x00,
};
// 备用套费率电价 
#define PI_currfeepricetab_copy  PI_currfeepricetab
const unsigned char CODE PI_ct[] = { 0x01, 0x00, 0x00, 0x00 }; // 电流互感器变比 
const unsigned char CODE PI_pt[] = { 0x01, 0x00, 0x00, 0x00 }; // 电压互感器变比 
// 当前套阶梯电价 
const unsigned char CODE PI_currstepprice[] =
{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

const unsigned char CODE PI_currstepprice1[] =
{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

const unsigned char CODE PI_yearbill[] =
{
    0x63, 0x63, 0x63,
    0x63, 0x63, 0x63,
    0x63, 0x63, 0x63,
    0x63, 0x63, 0x63,
};

// 备用套阶梯电价 
#define PI_currstepprice_copy  PI_currstepprice
#define PI_currstepprice1_copy  PI_currstepprice1
#define PI_yearbill_copy  PI_yearbill

// 报警金额限值 
const unsigned char CODE PI_alarmmoney[] =
{
    0xF4, 0x01, 0x00, 0x00, //报警金额限值1
    0x64, 0x00, 0x00, 0x00, //报警金额限值2
};
// 其它金额限值 
const unsigned char CODE PI_othermoney[] =
{
    0xE8, 0x03, 0x00, 0x00, //透支金额限值
    0xff, 0xe0, 0xf5, 0x05, //囤积金额限值
    0xC8, 0x00, 0x00, 0x00, //合闸允许金额限值
};
 const unsigned char CODE PI_plusetime[] = { 0x50, 0x00 }; // 校表脉冲宽度 
// 资产管理编码 
const unsigned char CODE PI_serinumber[] = 
{ 
    32, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  
const unsigned char CODE PI_ascii_v[] = { 6, '2', '2', '0','V',0x00, 0x00 }; // 额定电压 
const unsigned char CODE PI_ascii_ib[] = { 6, '5', 'A', 0x00, 0x00, 0x00, 0x00,  };  // 额定电流/基本电流 
const unsigned char CODE PI_ascii_imax[] = { 6, '6', '0', 'A',  0x00, 0x00, 0x00,  }; // 最大电流 
const unsigned char CODE PI_ascii_imin[] = { 6, '0', '.', '2', '5',   'A',  0x00,  }; // 最小电流  
const unsigned char CODE PI_ascii_itr[] = { 6, '0', '.', '5', 'A',   0x00, 0x00,  }; // 最转折电流
const unsigned char CODE PI_active_class[] = { 4, 'A', 0x00, 0x00, 0x00 };  // 有功准确度等级 
const unsigned char CODE PI_pluse[] = { 0xD0, 0x07, 0x00, 0x00 }; // 有功，无功 脉冲常数  2000


#define PI_pluse_copy PI_pluse
// 电表型号 
const unsigned char CODE PI_meter_model[] = 
{ 
    32, 
    'D', 'D', 'Z', 'Y', 'X', 'X', 'X', '-', 'M', 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00
}; 
const unsigned char CODE PI_aaaaaa[] = { 0x10, 'A', 'A', 'A', 'A', 'A', 'A',  'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',  'A', 'A' }; // 软件备案号 
const unsigned char CODE PI_activebyte[] = { 0x08, 0x05 }; // 有功组合方式特征字 
const unsigned char CODE PI_bgalarmword[] = { 0x20, 0x00, 0x00, 0x00, }; // 背光报警配置字 
const unsigned char CODE PI_billday[] = { 0, 1, 0x63, 0x63, 0x63, 0x63 }; // 结算日 
const unsigned char CODE PI_password[] = { 8, '0', '0', '0', '0', '0', '0', '0', '0' }; // 认证密码 

const unsigned char CODE PI_reportflag1[] = { 0 //0x03 }; // 上报标识
const unsigned char CODE PI_reportflag2[] = { 1 };// 上报标识
const unsigned char CODE PI_reportflag3[] = { 3 };// 上报标识
const unsigned char CODE PI_reportflag4[] = { 1 };// 上报标识
const unsigned char CODE PI_reportflag5[] = { 1 };// 上报标识

const unsigned char CODE PI_validflag[] = { 0x01 }; // 有效标识

const unsigned char CODE PI_485word[] = { 0x80, 0x25 }; // RS485 - 1波特率  9600 
const unsigned char CODE PI_plcword[] = { 0x80, 0x25 }; // 载波 - 波特率  9600 
const unsigned char CODE PI_module[] = { 0, 4, 0x0a, 0x02, 'x', 'x', 0x0a, 0x02, 'x', 'x', 26, 0xff, 0xff, 0xff, 0xff, 0xff, 18, 0x00, 0x00 }; // 载波/微功率无线接口 
const unsigned char CODE PI_devices[] = { 0x09, 'D', 'D', 'Z', 'Y', 'X', 'X', 'X' ,'-','M' }; // 设备描述符 32 bytes 
const unsigned char CODE PI_factory[] = { 0x04, 'X', 'X', 'X', 'X' }; // 厂商代码       visible-string(SIZE (4)) 
const unsigned char CODE PI_firmwarever[] = { 0x04, 'X', 'X', 'X', 'X' }; // 软件版本号     visible-string(SIZE (4)) 
const unsigned char CODE PI_firmwareverdate[] = { 0x06, '2', '1', '0', '7', '1', '7' }; // 软件版本日期   visible-string(SIZE (6)) 
const unsigned char CODE PI_hardwarever[] = { 0x04, 'X', 'X', 'X', 'X' }; // 硬件版本号     visible-string(SIZE (4)) 
const unsigned char CODE PI_hardwareverdate[] = { 0x06, '2', '1', '0', '7', '1', '7' }; // 硬件版本日期   visible-string(SIZE (6)) 
const unsigned char CODE PI_reserve[] = { 0x08, 'X', 'X', 'X', 'X','X', 'X', 'X', 'X' }; // 厂家扩展信息   visible-string(SIZE (8)) 
const unsigned char CODE PI_date_in_produced[] = { 0x00, 0x00, 0x00, 0x15, 0x08, 0xe4, 0x07 }; // 生产日期 - 设备的出厂日期，数据格式date_time_s 
const unsigned char CODE PI_subset[] = { 0x00 }; // 子设备列表 1+2*3  
const unsigned char CODE PI_protocolver[] = { 0x01, 0x14, 'D', 'L', '/', 'T', '6', '9', '8', '.', '4', '5' ,'-', '2' , '0', '1', '7', '-', '2', '0', '2', '0' }; // 支持规约列表1+16*3 
const unsigned char CODE PI_reportallowen[] = { 0x00 }; // 允许跟随上报 
const unsigned char CODE PI_reportactiveen[] = { 0x01 }; // 允许主动上报 
const unsigned char CODE PI_connecthosten[] = { 0x01 }; // 允许与主站通话 
//国网2020标准模块表上报通道默认只有模块
const unsigned char CODE PI_reportchanle[] = { 0x01, //0x01, 0x02, 0x01, 0xf2, 0x01, 0x02, 0x09, 0xf2, //0x02, 0x02, 0x01, 0xf2,}; // 上报通道 1+4*3 
const unsigned char CODE PI_reportitems[] =
{
    26,
    0x05, 0x30, 0x11, 0x30, 0x12, 0x30, 0x13, 0x30, 0x15, 0x30, 0x16, 0x30, 0x17, 0x30, 0x18, 0x30, 
    0x19, 0x30, 0x1A, 0x30, 0x1B, 0x30, 0x1F, 0x30, 0x20, 0x30, 0x21, 0x30, 0x22, 0x30, 0x24, 0x30, 
    0x25, 0x30, 0x26, 0x30, 0x27, 0x30, 0x28, 0x30, 0x29, 0x30, 0x2B, 0x30, 0x2C, 0x30, 0x2E, 0x30, 
    0x2F, 0x30, 0x30, 0x30,
};

const unsigned char CODE PI_reportallowsates[] = { 32, 0, 0, 0, 0 };    // 跟随上报状态字2 rs4851 
#define PI_reportallowsates_copy1 PI_reportallowsates                         // 跟随上报状态字3 模块 
#define PI_reportallowsates_copy2 PI_reportallowsates                         // 跟随上报状态字4 rs4852
#define PI_reportallowsates_copy3 PI_reportallowsates                         // 跟随上报状态字1 红外 


const unsigned char CODE PI_reportallowmodes[] = { 32, 0x00, 0x00, 0x00, 0x00 }; // 跟随上报模式字 
const unsigned char CODE PI_ir_auth_time[] = { 0x1e, 0x00, 0x00, 0x00 }; // 红外认证时效门限 

const unsigned char CODE PI_relayoff_imax[] = { 0xe0, 0x93, 0x04, 0x00 }; // 继电器拉闸控制电流门限值  - 继电器拉闸电流门限值 
const unsigned char CODE PI_relayoff_imaxdly[] = { 0xa0, 0x05 }; // 超拉闸控制电流门限保护延时时间  - 超电流门限保护延时时间 

const unsigned char CODE PI_keydisplay_time[] = { 0x3c, 0x00 }; // 按键显示退出时间 
const unsigned char CODE PI_autodisplay_time[] = { 0x05, 0x00 }; // 自动轮显每屏显示时间 

 // 自动循环显示对象FSID列表 
const unsigned char CODE PI_autodisplayitems[] = 
{ 
    0x07, 
    0xd3, 0x6f,  // 当前剩余金额
    0x62, 0x7B, 0x63, 0x7B, 0x64, 0x7B, 0x65, 0x7B, 0x66, 0x7B,   // 组合高精度总尖峰平谷
    //0x80, 0x93, 0x81, 0x93, 0x82, 0x93, 0x83, 0x93, 0x84, 0x93,  // 组合总尖峰平谷
    0xD5, 0x6F, //  电价
}; 

// 按键显示对象FSID列表 
const unsigned char CODE PI_keydisplayitems[] = 
{ 
    27, 

    0xd3, 0x6f,  // 当前剩余金额
    0x62, 0x7B, 0x63, 0x7B, 0x64, 0x7B, 0x65, 0x7B, 0x66, 0x7B,  // 组合高精度总尖峰平谷

    0xE0, 0x7B, 0xEC, 0x7B, 0xF8, 0x7B, 0x04, 0x7C, 0x10, 0x7C,  // 上1 高精度 组合总尖峰平谷 
    0xE1, 0x7B, 0xED, 0x7B, 0xF9, 0x7B, 0x05, 0x7C, 0x11, 0x7C,  // 上2 高精度组合总尖峰平谷
    //0x80, 0x93, 0x81, 0x93, 0x82, 0x93, 0x83, 0x93, 0x84, 0x93,  // 组合总尖峰平谷
    //0x14, 0x78, 0x15, 0x78, 0x16, 0x78, 0x17, 0x78, 0x18, 0x78,  // 上1 组合总尖峰平谷
    //0x39, 0x78, 0x44, 0x78, 0x50, 0x78, 0x5b, 0x78, 0x66, 0x78,  // 上2 组合总尖峰平谷
    
    0xD5, 0x6F, //  电价
    0x95, 0x79, //用户号 低8位
    0x94, 0x79, //用户号 高4位
     0x07, 0x7E, 0x06, 0x7E,
    0x03, 0xc6, 0x00, 0xc6, // 日期时间
    0x03, 0xc0, 0x04, 0xc0, 0x30, 0xc0, 0x35, 0xc0, 
};  
//*本地卡表独有显示项列表*
const unsigned char CODE PI_carddisplayitems[] = 
{ 
    0xd3, 0x6f, 0x10, 0x5f, 0x11, 0x5f, 0x12, 0x5f, 0x13, 0x5f,
    0x50, 0x5f, 0x51, 0x5f, 0x52, 0x5f, 0x53, 0x5f, 0xd5, 0x6f, 
    0xe0, 0x21, 0xe1, 0x21, 0xd4, 0x6f, 
    0x95, 0x79,0x94, 0x79, //用户号
};

const unsigned char CODE inital_report_enable[] = { 0x01 };



const unsigned char CODE PI_poweronreporttimes[] = {0x10,0x0E}; // 3600秒 掉电事件主动上报的上电时间阈值

// 小电流不滤除 AA开启  
const unsigned char CODE PI_i_start_en[] = {0x00};
// DL/T 698.45 事件记录 - 当前值-事件发生源NULL  多个事件占用 
const unsigned char CODE PI_event_source[] = {0x00};

typedef struct
{
	unsigned int          EEAddr;//参数在EE中存储地址
	unsigned int          Length;//参数长度
	const unsigned char*  DataBuff;//数据地址
}PARATAB;

const PARATAB  para_init_tab[] =
{
    { 0x24d2, sizeof(PI_safeenable), PI_safeenable }, // 安全模式使能 

    { 0x2522, sizeof(PI_reportallowsates), PI_reportallowsates }, // 跟随上报状态字2 rs4851 
    { 0x2523, sizeof(PI_reportallowmodes), PI_reportallowmodes }, // 跟随上报模式字 
    { 0x2524, sizeof(PI_ir_auth_time), PI_ir_auth_time }, // 红外认证时效门限 

    { 0x2527, sizeof(PI_reportallowsates_copy1), PI_reportallowsates_copy1 }, // 跟随上报状态字3 模块 
    { 0x2529, sizeof(PI_reportallowsates_copy3), PI_reportallowsates_copy3 }, // 跟随上报状态字1 红外 

    { 0x4200, 0xa700 + sizeof(PI_reportflag1), PI_reportflag1 }, //上报标识
    { 0x4580, 0x8A00 + sizeof(PI_reportflag1), PI_reportflag1 }, //上报标识
    { 0x4240, 0xa700 + sizeof(PI_validflag), PI_validflag }, //有效标识
    { 0x4590, 0x8A00 + sizeof(PI_validflag), PI_validflag }, //有效标识

    { 0x4207, sizeof(PI_reportflag3), PI_reportflag3 }, //掉电事件上报标识
    { 0x4211, sizeof(PI_reportflag4), PI_reportflag4 }, //开盖事件上报标识 
    { 0x4224, sizeof(PI_reportflag5), PI_reportflag5 }, //时钟故障事件上报标识
    { 0x4585, sizeof(PI_reportflag2), PI_reportflag2 }, //过流事件上报标识
    
        
    { 0x4D53, sizeof(PI_reportflag1), PI_reportflag1 }, //零线电流异常事件上报标识
    { 0x4D54, sizeof(PI_validflag), PI_validflag }, //零线电流异常事件有效标识

    { 0x4E5D, sizeof(PI_reportflag2), PI_reportflag2 }, //广播校时异常事件上报标识
    { 0x4E5E, sizeof(PI_validflag), PI_validflag }, //广播校时异常事件有效标识

    // DL/T 698.45 事件记录 - 上报方式  0x4E00 - 0x4E26    
    { 0x4E00, 0xA800 + sizeof(PI_reportfollow), PI_reportfollow}, // 默认全部事件为跟随上报 
    // DL/T 698.45 分项事件记录 - 有效标识  0x4E30 - 0x4E3f 
    { 0x4E30, 0x8A00 + sizeof(PI_reportfollow), PI_reportfollow}, // 默认全部分项事件为跟随上报 
    { 0x4E40, 1, PI_reportfollow}, // 2015的上报方式 

    { 0x4E07, sizeof(PI_report_active), PI_report_active}, //掉电事件上报方式
    { 0x4E11, sizeof(PI_report_active), PI_report_active }, //开盖事件上报方式 
    { 0x4E24, sizeof(PI_report_active), PI_report_active }, //时钟故障事件上报方式
    { 0x4E35, sizeof(PI_report_active), PI_report_active }, //过流事件上报方式
    { 0x4E5F, sizeof(PI_report_active), PI_report_active}, //广播校时异常事件上报方式

    { 0x4B00, sizeof(PI_reportitems), PI_reportitems }, // 事件上报对象使能列表 

    { 0x2101, sizeof(PI_meteraddress), PI_meteraddress }, // 通信地址 
    //{ 0x2107, sizeof(PI_meternumbers), PI_meternumbers }, // 表号 
    { 0x215f, sizeof(PI_customerid), PI_customerid }, // 客户编号 
    { 0x2160, sizeof(PI_location), PI_location }, // 设备地理位置 
    { 0x2200, sizeof(PI_displayalltime), PI_displayalltime }, // LCD参数 上电全显时间 
    { 0x2201, sizeof(PI_bg_button_time), PI_bg_button_time }, // LCD参数 按键背光点亮时间 
    { 0x2202, sizeof(PI_bg_ir_time), PI_bg_ir_time }, // LCD参数 显示查看背光点亮时间 
    { 0x2203, sizeof(PI_buttontime), PI_buttontime }, // LCD参数 无电按键屏幕驻留最大时间 
    { 0x2204, sizeof(PI_engdot), PI_engdot }, // LCD参数 显示电能小数位数 
    { 0x2205, sizeof(PI_powerdot), PI_powerdot }, // LCD参数 显示功率（最大需量）小数位数 
    { 0x5FE4, sizeof(PI_display12means_copy), PI_display12means_copy }, // LCD参数 液晶①②字样意义 
    { 0x3210, sizeof(PI_changetimes), PI_changetimes }, // 两套时区表切换时间 
    { 0x3211, sizeof(PI_changetimes_copy1), PI_changetimes_copy1 }, // 两套日时段切换时间 
    { 0x3212, sizeof(PI_changetimes_copy2), PI_changetimes_copy2 }, // 两套分时费率切换时间 
    { 0x3213, sizeof(PI_changetimes_copy3), PI_changetimes_copy3 }, // 两套阶梯电价切换时间 
    { 0x3200, sizeof(PI_somenum), PI_somenum }, // 年时区数/日时段表数/日时段数/费率数/公共假日数 
    { 0x3204, sizeof(PI_holidaynum), PI_holidaynum },
    { 0x3205, sizeof(PI_stepnum), PI_stepnum }, // 阶梯数 
    { 0x2143, sizeof(PI_keynum), PI_keynum }, // 密钥总条数 
    { 0x2211, sizeof(PI_meterchipnum), PI_meterchipnum }, // 计量元件数 
    
    //{0x5FD0,  sizeof(PI_boardtimesetmode), PI_boardtimesetmode },//  广播校时模式 
    {0x4E62,  sizeof(PI_boardtimesettime1), PI_boardtimesettime1 },//  广播校时最小偏差 
    {0x4E63,  sizeof(PI_boardtimesettime2), PI_boardtimesettime2 },//  广播校时最大偏差 

    //{ 0x301C, sizeof(inital_301C), inital_301C }, // 广播校时响应限值 
    { 0x3300, //2500x9400 + sizeof(PI_holidaytab), //NULLPI_holidaytab }, // 公共假日表 
    { 0x2176, sizeof(PI_weekword), PI_weekword }, // 周休日特征字 
    { 0x2177, sizeof(PI_weekofnum), PI_weekofnum }, // 周休日釆用的日时段表号 
    { 0x3220, sizeof(PI_currseasontab), PI_currseasontab }, // 当前套时区表 
    { 0x3230, sizeof(PI_currseasontab_copy), PI_currseasontab_copy }, // 备用套时区表 

    { 0x4B30, sizeof(PI_bakitem1_copy), PI_bakitem1_copy }, // 单个索引当前套时区表 
    { 0x4B31, 0x8D00 +sizeof(PI_bakitem2_copy), PI_bakitem2_copy }, // 单个当前套时区表 
    { 0x4B40, sizeof(PI_bakitem1), PI_bakitem1 }, // 单个索引备用套时区表 
    { 0x4B41, 0x8D00 +sizeof(PI_bakitem2), PI_bakitem2 }, // 单个备用套时区表 
    
    // 当前套日时段表 
    { 0x3240, sizeof(PI_currdaytab), PI_currdaytab },
    { 0x3241, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3242, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3243, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3244, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3245, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3246, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3247, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    // 备用套日时段表 
    { 0x3250, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3251, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3252, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3253, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3254, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3255, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3256, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },
    { 0x3257, sizeof(PI_currdaytab_copy), PI_currdaytab_copy },

    //  当前套时段表  
    { 0x4B50, 0x8800 +sizeof(inital_4B50), inital_4B50 },
    //  备用套时段表  
    { 0x4B60, 0x8800 +sizeof(inital_4B50), inital_4B50 },

    // 当前套阶梯电价 
    { 0x5F40, sizeof(PI_currstepprice), PI_currstepprice },
    { 0x5F50, sizeof(PI_currstepprice1), PI_currstepprice1 },
    { 0x5F60, sizeof(PI_yearbill), PI_yearbill },
    // 备用套阶梯电价 
    { 0x4EA0, sizeof(PI_currstepprice_copy), PI_currstepprice_copy },
    { 0x4EB0, sizeof(PI_currstepprice1_copy), PI_currstepprice1_copy },
    { 0x4EC0, sizeof(PI_yearbill_copy), PI_yearbill_copy },
    { 0x5F10, sizeof(PI_currfeepricetab), PI_currfeepricetab }, // 当前套费率电价 
    { 0x4E70, sizeof(PI_currfeepricetab_copy), PI_currfeepricetab_copy }, // 备用套费率电价 

    { 0x2146, sizeof(PI_ct), PI_ct }, // 电流互感器变比 
    { 0x2147, sizeof(PI_pt), PI_pt }, // 电压互感器变比 
    { 0x21E0, sizeof(PI_alarmmoney), PI_alarmmoney }, // 报警金额限值 
    { 0x21F0, sizeof(PI_othermoney), PI_othermoney }, // 其它金额限值  

    { 0x2142, sizeof(PI_plusetime), PI_plusetime }, // 校表脉冲宽度 
    { 0x2150, sizeof(PI_serinumber), PI_serinumber }, // 资产管理编码 
    { 0x2151, sizeof(PI_ascii_v), PI_ascii_v }, // 额定电压 
    { 0x2152, sizeof(PI_ascii_ib), PI_ascii_ib }, // 额定电流/基本电流 
    { 0x2153, sizeof(PI_ascii_imax), PI_ascii_imax }, // 最大电流 
    { 0x3030, sizeof(PI_ascii_imin), PI_ascii_imin }, // 最小电流 
    { 0x3031, sizeof(PI_ascii_itr), PI_ascii_itr }, // 最转折电流 

    { 0x2154, sizeof(PI_active_class), PI_active_class }, // 有功准确度等级 
    { 0x2156, sizeof(PI_pluse), PI_pluse }, // 有功脉冲常数  
    { 0x2158, sizeof(PI_meter_model), PI_meter_model }, // 电表型号 
    { 0x21DB, sizeof(PI_aaaaaa), PI_aaaaaa }, // 软件备案号 
    //{ 0x2148, sizeof(inital_6FA0), inital_6FA0 }, // 插卡状态字
    { 0x2165, sizeof(PI_activebyte), PI_activebyte }, // 有功组合方式特征字
 
    { 0x2168, sizeof(PI_bgalarmword), PI_bgalarmword }, // 背光报警配置字
    //{ 0x6FA0, 0x8700 + sizeof(inital_6FA0), inital_6FA0 }, // 电能表运行状态字1 .. 7
    { 0x2190, sizeof(PI_billday), PI_billday }, // 结算日 
    { 0x21DE, sizeof(PI_password), PI_password }, // 认证密码 

    { 0x4C2D, 1, NULL }, // 开关属性  enum{脉冲式（0），保持式（1）} 
    { 0x2272, sizeof(PI_485word), PI_485word }, // RS485 -1 波特率  
    { 0x2273, sizeof(PI_plcword), PI_plcword }, // 载波 - 波特率 
    { 0x2274, sizeof(PI_module), PI_module }, // 载波 - 版本信息 

    { 0x24e0, sizeof(PI_devices), PI_devices },// 设备描述符32bytes 
    { 0x24e1, sizeof(PI_factory), PI_factory },// 厂商代码 
    { 0x24e2, sizeof(PI_firmwarever), PI_firmwarever },// 软件版本号 
    { 0x24e3, sizeof(PI_firmwareverdate), PI_firmwareverdate },// 软件版本日期 
    { 0x24e4, sizeof(PI_hardwarever), PI_hardwarever },// 硬件版本号 
    { 0x24e5, sizeof(PI_hardwareverdate), PI_hardwareverdate },// 硬件版本日期 
    { 0x24e6, sizeof(PI_reserve), PI_reserve },// 厂家扩展信息 
    { 0x24e7, sizeof(PI_date_in_produced), PI_date_in_produced },// 生产日期 
    { 0x24e8, sizeof(PI_subset), PI_subset },// 子设备列表1+2*3 
    { 0x4ED9, sizeof(PI_protocolver), PI_protocolver },// 支持规约列表1+25*3 
    { 0x24ea, sizeof(PI_reportallowen), PI_reportallowen },// 允许跟随上报 
    { 0x24eb, sizeof(PI_reportactiveen), PI_reportactiveen },// 允许主动上报 
    { 0x24ec, sizeof(PI_connecthosten), PI_connecthosten },// 允许与主站通话 
    { 0x24ed, sizeof(PI_reportchanle), PI_reportchanle }, // 上报通道1+4*3 


    { 0x2144, sizeof(PI_keydisplay_time), PI_keydisplay_time }, // 按键显示退出时间 
    { 0x2145, sizeof(PI_autodisplay_time), PI_autodisplay_time }, // 自动轮显每屏显示时间 

    { 0x2300, sizeof(PI_autodisplayitems), PI_autodisplayitems }, // 自动循环显示对象FSID列表 
    { 0x2301, sizeof(PI_keydisplayitems), PI_keydisplayitems }, // 按键显示对象FSID列表 



    //事件相关参数初始化

 
    { 0x4007, sizeof(PI_poweroffevent), PI_poweroffevent }, // 电能表掉电事件 
    { 0x4008, sizeof(PI_programevent), PI_programevent }, // 电能表编程事件 
    { 0x4009, sizeof(PI_meterclearenvet), PI_meterclearenvet }, // 电能表清零事件 
 
    { 0x400B, sizeof(PI_eventclearevent), PI_eventclearevent }, // 电能表事件清零事件 
    { 0x400C, sizeof(PI_timesetevent), PI_timesetevent }, // 电能表校时事件 
    { 0x400D, sizeof(PI_daytabproevent), PI_daytabproevent }, // 电能表时段表编程事件 
    { 0x400E, sizeof(PI_seasonproevent), PI_seasonproevent }, // 电能表时区表编程事件 
    { 0x400F, sizeof(PI_weekproevent), PI_weekproevent }, // 电能表周休日编程事件 
    { 0x4010, sizeof(PI_billproevent), PI_billproevent }, // 电能表结算日编程事件 
    { 0x4011, sizeof(PI_coveropenevent), PI_coveropenevent }, // 电能表开盖事件 
 
    { 0x4015, sizeof(PI_relayoffevent), PI_relayoffevent }, // 电能表跳闸事件 
    { 0x4016, sizeof(PI_relayonevent), PI_relayonevent }, // 电能表合闸事件 
    { 0x4017, sizeof(PI_holidayproevent), PI_holidayproevent }, // 电能表节假日编程事件 
    { 0x4018, sizeof(PI_comb0proevent), PI_comb0proevent }, // 电能表有功组合方式编程事件 
    //{ 0x4019, sizeof(PI_comb1proevent), PI_comb1proevent }, // 电能表无功组合方式编程事件 
    { 0x401A, sizeof(PI_feeproevent), PI_feeproevent }, // 电能表费率参数表编程事件 
    { 0x401B, sizeof(PI_stepproevent), PI_stepproevent }, // 电能表阶梯表编程事件 
    { 0x401C, sizeof(PI_keyproevent), PI_keyproevent }, // 电能表密钥更新事件 
    { 0x401D, sizeof(PI_carderrevent), PI_carderrevent }, // 电能表异常插卡事件 
    { 0x401E, sizeof(PI_buyevent), PI_buyevent }, // 电能表购电记录 
    { 0x401F, sizeof(PI_refundevent), PI_refundevent }, // 电能表退费记录 
     { 0x4021, sizeof(PI_relayerrevent), PI_relayerrevent }, // 电能表负荷开关误动作事件 
    { 0x4022, sizeof(PI_powererrevent), PI_powererrevent }, // 电能表电源异常事件 
     { 0x4024, sizeof(PI_timeerrevent), PI_timeerrevent }, // 电能表时钟故障事件 
    { 0x4025, sizeof(PI_meterchiperrevent), PI_meterchiperrevent }, // 电能表计量芯片故障事件 
    { 0x4026, sizeof(PI_broadtimesetevent), PI_broadtimesetevent }, // 广播校时事件 
    { 0x4E5A, sizeof(PI_broadtimeseterrevent), PI_broadtimeseterrevent }, // 广播校时事件 
    // DL/T 698.45 分项事件记录 - 关联对象属性表  0x4500 - 0x450F 
     { 0x4505, sizeof(PI_iovererrevent), PI_iovererrevent }, // 电能表过流事件 
     { 0x4D50, sizeof(PI_iberrevent), PI_iberrevent }, // 零线电流异常事件关联对象 

    // DL/T 698.45 事件记录 - 配置参数-判定延时时间  0x50C0 - 0x50FF 
     { 0x50C7, sizeof(PI_poweroffdly), PI_poweroffdly }, // 电能表掉电事件 
     { 0x50E1, sizeof(PI_relayerrdly), PI_relayerrdly }, // 电能表负荷开关误动作事件 （数据快照未使用）
    { 0x50E2, sizeof(PI_powererrdly), PI_powererrdly }, // 电能表电源异常事件 （数据快照未使用）
     { 0x50E4, sizeof(PI_rtcerrdly), PI_rtcerrdly }, // 电能表时钟故障事件 （数据快照未使用）
    { 0x50E5, sizeof(PI_chiperrdly), PI_chiperrdly }, // 电能表计量芯片故障事件 （数据快照未使用）
     // DL/T 698.45 分项事件记录 - 过流事件-配置参数  0x55EC - 0x55EF 
    { 0x55EC, sizeof(PI_iovervalue), PI_iovervalue }, // 电能表过流事件 - 电流触发下限 
    { 0x55ED, sizeof(PI_ioverdly), PI_ioverdly }, // 电能表过流事件 - 判定延时时间 
     
    { 0x5FD3, sizeof(PI_iberrvalue), PI_iberrvalue }, // 零线电流异常事件不平衡率限值 
    { 0x5FD4, sizeof(PI_iberrdly), PI_iberrdly }, // 零线电流异常事件 - 判定延时时间 
    { 0x5FD5, sizeof(PI_iberrvalue_I), PI_iberrvalue_I }, // 零线电流异常事件 - 触发下限 


    // 冻结 冻结周期： 8组数据依次表示8种方案（每种方案3个字节，第一个字节为0，表示该周期值无效，非零值表示该周期值有效，后面2个字节表示周期值） 0x4900 - 0x490F 
    { 0x4900, sizeof(PI_instant_freeze), PI_instant_freeze }, // 瞬时冻结 
    { 0x4902, sizeof(PI_min_freeze), PI_min_freeze }, // 分冻结 
    { 0x4903, sizeof(PI_hour_freeze), PI_hour_freeze }, // 小时冻结 
    { 0x4904, sizeof(PI_day_freeze), PI_day_freeze }, // 日冻结 
    { 0x4905, sizeof(PI_bill_freeze), PI_bill_freeze }, // 结算日冻结 
    { 0x4906, sizeof(PI_monthl_freeze), PI_monthl_freeze }, // 月冻结 
    { 0x4908, sizeof(PI_seasonchg_freeze), PI_seasonchg_freeze }, // 时区表切换冻结 
    { 0x4909, sizeof(PI_daytabchg_freeze), PI_daytabchg_freeze }, // 日时段表切换冻结 
    { 0x490A, sizeof(PI_feechg_freeze), PI_feechg_freeze }, // 费率电价切换冻结 
    { 0x490B, sizeof(PI_stepchg_freeze), PI_stepchg_freeze }, // 阶梯切换冻结 
    { 0x490C, sizeof(PI_stepbill_freeze), PI_stepbill_freeze }, // 阶梯结算冻结 
    // 冻结 关联对象 0x4910 - 0x491F 
    { 0x4910, sizeof(PI_instant_freeze_item), PI_instant_freeze_item }, // 瞬时冻结 
    { 0x4912, sizeof(PI_min_freeze_item), PI_min_freeze_item }, // 分冻结 
    { 0x4913, sizeof(PI_hour_freeze_item), PI_hour_freeze_item }, // 小时冻结 
    { 0x4914, sizeof(PI_day_freeze_item), PI_day_freeze_item }, // 日冻结 
    { 0x4915, sizeof(PI_bill_freeze_item), PI_bill_freeze_item }, // 结算日冻结 
    { 0x4916, sizeof(PI_month_freeze_item), PI_month_freeze_item }, // 月冻结 
    { 0x4918, sizeof(PI_seasonchg_freeze_item), PI_seasonchg_freeze_item }, // 时区表切换冻结 
    { 0x4919, sizeof(PI_daytabchg_freeze_item), PI_daytabchg_freeze_item }, // 日时段表切换冻结 
    { 0x491A, sizeof(PI_feechg_freeze_item), PI_feechg_freeze_item }, // 费率电价切换冻结 
    { 0x491B, sizeof(PI_stepchg_freeze_item), PI_stepchg_freeze_item }, // 阶梯切换冻结 
    { 0x491C, sizeof(PI_stepbill_freeze_item), PI_stepbill_freeze_item }, // 阶梯结算冻结 
    // 冻结 存储深度 0x2F20 - 0x2F2F 
    { 0x4920, sizeof(PI_instant_freeze_deep), PI_instant_freeze_deep }, // 瞬时冻结 
    { 0x5FC6, sizeof(PI_min_freeze_deep), PI_min_freeze_deep }, // 分冻结 
    { 0x4923, sizeof(PI_hour_freeze_deep), PI_hour_freeze_deep }, // 小时冻结 
    { 0x4924, sizeof(PI_hour_freeze_deep), PI_hour_freeze_deep }, // 日冻结 
    { 0x4925, sizeof(PI_bill_freeze_deep), PI_bill_freeze_deep }, // 结算日冻结 
    { 0x4926, sizeof(PI_month_freeze_deep), PI_month_freeze_deep }, // 月冻结 
    { 0x4928, sizeof(PI_seasonchg_freeze_deep), PI_seasonchg_freeze_deep }, // 时区表切换冻结 
    { 0x4929, sizeof(PI_daytabchg_freeze_deep), PI_daytabchg_freeze_deep }, // 日时段表切换冻结 
    { 0x492A, sizeof(PI_feechg_freeze_deep), PI_feechg_freeze_deep }, // 费率电价切换冻结 
    { 0x492B, sizeof(PI_stepchg_freeze_deep), PI_stepchg_freeze_deep }, // 阶梯切换冻结 
    { 0x492C, sizeof(PI_stepbill_freeze_deep), PI_stepbill_freeze_deep }, // 阶梯结算冻结 

    { 0x21D7, sizeof(PI_relayoff_imax), PI_relayoff_imax }, // 继电器拉闸控制电流门限值  - 继电器拉闸电流门限值 
    { 0x21D8, sizeof(PI_relayoff_imaxdly), PI_relayoff_imaxdly }, // 超拉闸控制电流门限保护延时时间  - 超电流门限保护延时时间 

    { 0x4585, sizeof(inital_report_enable), inital_report_enable }, // 电能表过流事件  
    { 0x4211, sizeof(inital_report_enable), inital_report_enable }, // 电能表开盖事件 

    { 0x5FE5, sizeof(PI_poweronreporttimes), PI_poweronreporttimes }, // 掉电主动上报的阈值时间(秒)HEX 
    { 0x5FE6, sizeof(PI_i_start_en), PI_i_start_en }, // 小电流不滤除 AA开启     
    { 0x5FE7, sizeof(PI_event_source), PI_event_source },// DL/T 698.45 事件记录 - 当前值-事件发生源NULL  多个事件占用 

};                    

#define 	paranum		sizeof(para_init_tab)/sizeof(PARATAB)

void do_parainit(void)
{
	unsigned long i;
	
	for(i=0; i<paranum; i++)
	{
		//写数据
		if((i%20) == 0)
		{
			//清看门狗
		}
	}
	
}