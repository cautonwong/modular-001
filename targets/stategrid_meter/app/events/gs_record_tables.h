#ifndef ND_TARGET_STATEGRID_METER_EVENTS_GS_RECORD_TABLES_H
#define ND_TARGET_STATEGRID_METER_EVENTS_GS_RECORD_TABLES_H

#include <stdint.h>

/* One-byte count plus fifteen four-byte OADs. The event parameter block
 * stores this fixed-size list, so the initializer arrays are padded to it. */
#define ND_EVENT_OAD_LIST_BYTES 61
/* Fifteen two-byte minute-freeze depth slots. */
#define ND_MINUTE_FREEZE_DEPTH_BYTES 30
/* Fixed season-table backup field, padded from the short initializer. */
#define ND_BEFOR_SET_SEASON_T_BYTES 114
/* Explicit security-mode parameter cache. */
#define ND_SECURITY_MODE_EXDATA_BYTES 101

/* Event and freeze tables defined in gs_record.c. */
extern const uint8_t  init_MinuteFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_HourFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_DayFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_BillFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_MonthFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_SeasonTabCutFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_DayTabCutFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_FeePriceCutFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_StepCutFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_StepBillFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_InstantFreeze_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_Event_ReportFlag_3[];
extern const uint8_t  init_Event_ReportFlag_1[];
extern const uint8_t  init_Event_ReportFlag_0[];
extern const uint8_t  init_Event_ReportMode_1[];
extern const uint8_t  init_Event_ReportMode_0[];
extern const uint8_t  init_MinuteFreeze_period[];
extern const uint8_t  init_HourFreeze_period[];
extern const uint8_t  init_DayFreeze_period[];
extern const uint8_t  init_BillFreeze_period[];
extern const uint8_t  init_MonthFreeze_period[];
extern const uint8_t  init_SeasonTabCutFreeze_period[];
extern const uint8_t  init_DayTabCutFreeze_period[];
extern const uint8_t  init_FeePriceCutFreeze_period[];
extern const uint8_t  init_StepCutFreeze_period[];
extern const uint8_t  init_StepBillFreeze_period[];
extern const uint8_t  init_InstantFreeze_period[];
extern const uint8_t  init_MinuteFreeze_depth[ND_MINUTE_FREEZE_DEPTH_BYTES];
extern const uint8_t  init_HourFreeze_depth[];
extern const uint8_t  init_DayFreeze_depth[];
extern const uint8_t  init_BillFreeze_depth[];
extern const uint8_t  init_MonthFreeze_depth[];
extern const uint8_t  init_SeasonTabCutFreeze_depth[];
extern const uint8_t  init_DayTabCutFreeze_depth[];
extern const uint8_t  init_FeePriceCutFreeze_depth[];
extern const uint8_t  init_StepCutFreeze_depth[];
extern const uint8_t  init_StepBillFreeze_depth[];
extern const uint8_t  init_InstantFreeze_depth[];
extern const uint8_t  init_BeforSet_SeasonT[ND_BEFOR_SET_SEASON_T_BYTES];
extern const uint8_t  init_SecurityMode_En[];
extern const uint8_t  init_SecurityMode_EXData[ND_SECURITY_MODE_EXDATA_BYTES];
extern const uint8_t  init_OverCurrentAS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_PowerOffS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_MeterWrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_MeterClrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_EventClrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_SetTimeS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_DayTabWrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_SeasonTabWrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_WeekTabWrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_BillDayWrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_CoverOpenS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_SwitchOffS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_SwitchOnS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_HolidayWrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_ActCombWrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_FeeTabWrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_StepTabWrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_SecretKeyWrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_ErrPlugCardS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_RechargeS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_RefundS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_SwitchErrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_PowerAbnormalS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_RTCErrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_EmuErrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_BroadcastTimeS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_ZeroLineErrS_oad[ND_EVENT_OAD_LIST_BYTES];
extern const uint8_t  init_AuthTime_DL645[];

#endif
