#ifndef ND_EVENT_ID_H
#define ND_EVENT_ID_H

/*
 * Legacy record/event identifiers used by the meter event pipeline.
 *
 * The numeric values are part of the persisted and reported event model.
 * Keep the declaration order for the enum: the catalog arithmetic depends
 * on consecutive ids.
 */
enum record_flag_id_t
{
    /* class 24 */
    OverCurrentAS = 11,             /* 0x3005 A phase over-current start */
    OverCurrentAE = 12,             /* 0x3005 A phase over-current end */

    PowerOffS,                      /* 0x3011 meter power-off */
    PowerOffE,
    MeterWrS,                       /* 0x3012 meter programming */
    MeterWrR,                       /* special programming-list update */
    MeterClrS,                      /* 0x3013 meter clear */
    EventClrS,                      /* 0x3015 event clear */
    SetTimeS,                       /* 0x3016 time set */
    DayTabWrS,                      /* 0x3017 day table programming */
    SeasonTabWrS,                   /* 0x3018 season table programming */
    WeekTabWrS,                     /* 0x3019 week table programming */
    BillDayWrS,                     /* 0x301A billing day programming */
    CoverOpenS,                     /* 0x301B cover open */
    CoverOpenE,
    SwitchOffS,                     /* 0x301F trip */
    SwitchOnS,                      /* 0x3020 close */
    HolidayWrS,                     /* 0x3021 holiday programming */
    ActCombWrS,                     /* 0x3022 active combination programming */
    FeeTabWrS,                      /* 0x3024 tariff table programming */
    StepTabWrS,                     /* 0x3025 step table programming */
    SecretKeyWrS,                   /* 0x3026 key update */
    ErrPlugCardS,                   /* 0x3027 abnormal card insert */
    RechargeS,                      /* 0x3028 recharge */
    RefundS,                        /* 0x3029 refund */
    SwitchErrS,                     /* 0x302B switch misoperation */
    SwitchErrE,
    PowerAbnormalS,                 /* 0x302C abnormal power */
    PowerAbnormalE,
    RTCErrS,                        /* 0x302E clock fault */
    RTCErrE,
    EmuErrS,                        /* 0x302F metering chip fault */
    EmuErrE,
    BroadcastTimeS,                 /* 0x303C broadcast time set */
    ZeroLineErrS,                   /* 0x3040 neutral line abnormal */
    ZeroLineErrE,

    MeterWr645S,                    /* 0xFF31 645 programming */
    MeterWr645R,                    /* special programming-list update */
};

/* Freeze record ids: consecutive values are used by the catalog. */
#define MinuteFreeze1 170
#define MinuteFreeze2 171
#define MinuteFreeze3 172
#define MinuteFreeze4 173
#define MinuteFreeze5 174
#define MinuteFreeze6 175
#define MinuteFreeze7 176
#define MinuteFreeze8 177
#define HourFreeze 178
#define DayFreeze 179
#define BillFreeze 180
#define MonthFreeze 181
#define YearFreeze 182
#define SeasonTabCutFreeze 183
#define DayTabCutFreeze 184
#define FeePriceCutFreeze 185
#define StepCutFreeze 186
#define StepBillFreeze 187
#define InstantFreeze 188

#define RECORD_ID_END 200

#endif /* ND_EVENT_ID_H */
