#ifndef ND_TARGET_STATEGRID_METER_MAPPER_698_FACTORY_CFG_H
#define ND_TARGET_STATEGRID_METER_MAPPER_698_FACTORY_CFG_H

#include <stdint.h>

/* Fixed parameter-block fields, padded from the short initializers. */
#define ND_PROTOCOL_LIST_BYTES 76
#define ND_REPORT_CHANNEL_BYTES 13

/* Configuration tables defined in factory_cfg.c. */
extern const uint8_t  init_AutoDisplayList[];
extern const uint8_t  init_KeyDisplayList[];
extern const uint8_t   init_Comm_Number[];//表地址
extern const uint8_t   init_User_Number[];//客户编号
extern const uint8_t   init_Meter_Num[];//计量元件数
extern const uint8_t   init_SeasonTable_Changetime[];//两套时区切换时间
extern const uint8_t   init_SeasonTable_Changetime[];//两套时段切换时间
extern const uint8_t   init_SeasonDay_Para[];//时区时段参数（年时区数、日时段表
extern const uint8_t   init_SecretKey_Num[];//秘钥总条数
extern const uint8_t   init_HoliDay_Table[];//公共假日表，10张，每张6字节
extern const uint8_t   init_Weekend[];//周休日特征字
extern const uint8_t   init_WeekendFee[];//周休日采用费率号
extern const uint8_t   init_Season_Table0[];//当前套时区表，≤14
extern const uint8_t   init_Season_Table0[];//备用套时区表，≤14
extern const uint8_t   init_Day_Table00[];//当前套日时段表数据
extern const uint8_t   init_Day_Table00[];//备用套日时段表数据
extern const uint8_t  init_Alarm_Credit1[];   //报警金额限值1
extern const uint8_t  init_Alarm_Credit2[];   //报警金额限值2
extern const uint8_t   init_Meter_CT[];//CT
extern const uint8_t   init_Meter_PT[];//PT
extern const uint8_t   init_Pulse_Width[];//脉冲宽度
extern const uint8_t   init_ASCII_AssetsMan[];//资产管理编码
extern const uint8_t   init_ASCII_Un[];//额定电压
extern const uint8_t  init_ASCII_Ib[];//额定电流
extern const uint8_t  init_ASCII_Imax[];//最大电流
extern const uint8_t    init_ASCII_Imin[];//最小电流
extern const uint8_t    init_ASCII_Itr[];//转折电流
extern const uint8_t  init_ASCII_AccuracyClass1[];//有功准确度等级
extern const uint8_t  init_Meter_Const1[];//有功脉冲常数
extern const uint8_t  init_ASCII_MeterType[];//电能表型号
extern const uint8_t  init_MeterRunWord[];//电能表运行特征字
extern const uint8_t  init_SoftWareNo[];//软件备案号
extern const uint8_t  init_Measure_Mode[];//有功组合特征字
extern const uint8_t  init_Balance_Time[];//结算日
extern const uint8_t  init_Meter_Password[];//应用连接密码
extern const uint8_t  init_Meter_02Password[];
extern const uint8_t  init_Meter_04Password[];
extern const uint8_t  init_Meter_Location1[];//设备地理位置经度
extern const uint8_t  init_Meter_Location2[];//设备地理位置纬度
extern const uint8_t  init_Meter_Location3[];//设备地理位置高度
extern const uint8_t  init_Device_descriptor[];//设备描述符
extern const uint8_t  init_Factory_No[]; //厂家代码
extern const uint8_t  init_Version_SoftWare[]; //软件版本
extern const uint8_t  init_Date_SoftWare[]; //软件版本日期
extern const uint8_t  init_Version_HardWare[]; //硬件版本
extern const uint8_t  init_Date_HardWare[]; //硬件版本日期
extern const uint8_t  init_Extend_Info[]; //扩展信息
extern const uint8_t  init_Produced_Date[]; //生产日期
extern const uint8_t  init_Produced_Date645[]; //生产日期
extern const uint8_t  init_Subset_List[]; //子设备列表(num+2*oi)
extern const uint8_t  init_Protocol_List[ND_PROTOCOL_LIST_BYTES]; //协议列表
extern const uint8_t  init_FollowReport_en[]; //允许跟随上报
extern const uint8_t  init_ActiveReport_en[]; //允许主动上报
extern const uint8_t  init_Comm_Master_en[]; //允许与主站通讯
extern const uint8_t  init_Report_channel[ND_REPORT_CHANNEL_BYTES]; //上报通道
extern const uint8_t  init_ImaxProtectValue[]; //继电器拉闸电流门限值
extern const uint8_t  init_ImaxProtectTime[]; //超电流门限保护延时时间
extern const uint8_t  init_Blt_Alarm_Byte[];
extern const uint8_t  init_IrAuthenTime[];//红外认证时效
extern const uint8_t  init_Pstart_En[];
extern const uint8_t init_Remote_RelayOffTime[];    //拉闸延时时间
extern const uint8_t init_HourFroze_StartTime[];  //整点冻结起始时间
extern const uint8_t init_DayFroze_Time[];
extern const uint8_t init_ReportRstTime[];
extern const uint8_t init_HourFroze_Inv[];  //整点冻结间隔时间
extern const uint8_t init_Step_Num[];
extern const uint8_t init_ASCII_ptl645_Version[];
extern const uint8_t init_ASCII_SoftWareNo[];
extern const uint8_t init_ASCII_HardWareNo[];
extern const uint8_t init_SoftWareNo_645[];
extern const uint8_t init_LoadRec_Mode[];
extern const uint8_t init_TimeFroze_Mode[];
extern const uint8_t init_InsFroze_Mode[];
extern const uint8_t init_AppointFroze_Mode[];
extern const uint8_t init_HourFroze_Mode[];
extern const uint8_t init_DayFroze_Mode[];
extern const uint8_t init_LoadRec_Time[];
extern const uint8_t init_LoadRec_Inv1[];
extern const uint8_t init_LoadRec_Inv2[];
extern const uint8_t init_LoadRec_Inv3[];
extern const uint8_t init_LoadRec_Inv4[];
extern const uint8_t init_ASCII_Factor[];
extern const uint8_t  init_Meter_Type[]; //表功能配置字
extern const uint8_t  init_ParaInitFinish_Flag[];// 恢复出厂已执行标志
extern const uint8_t init_EC_SaveDiff_E[];
extern const uint8_t FirmWareVersion[20];

#endif
