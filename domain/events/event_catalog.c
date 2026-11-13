#include "domain/events/event_catalog.h"

#include "domain/events/event_id.h"
#include "domain/events/record_capture.h"

static int make_entry(nd_event_catalog_entry_t *entry, uint32_t oad, uint8_t mode)
{
    if (entry == 0) {
        return 0;
    }
    entry->oad = oad;
    entry->mode = mode;
    return 1;
}

int nd_event_catalog_lookup(uint16_t record_id, nd_event_catalog_entry_t *entry)
{
    if (entry == 0) {
        return 0;
    }

    switch (record_id) {
    case OverCurrentAS:
    case OverCurrentAE:
        return make_entry(entry, 0x30050700u,
            (uint8_t)(ND_RECORD_CAPTURE_START_MODE + (record_id - OverCurrentAS)));

    case PowerOffS:
    case PowerOffE:
        return make_entry(entry, 0x30110200u,
            (uint8_t)(ND_RECORD_CAPTURE_START_MODE + (record_id - PowerOffS)));

    case MeterWrS:
    case MeterWrR:
        return make_entry(entry, 0x30120200u,
            (uint8_t)(ND_RECORD_CAPTURE_START_MODE + (record_id - MeterWrS)));

    case MeterWr645S:
    case MeterWr645R:
        return make_entry(entry, 0xFF310000u,
            (uint8_t)(ND_RECORD_CAPTURE_START_MODE + (record_id - MeterWr645S)));

    case MeterClrS:
        return make_entry(entry, 0x30130200u, ND_RECORD_CAPTURE_START_MODE);

    case EventClrS:
        return make_entry(entry, 0x30150200u, ND_RECORD_CAPTURE_START_MODE);

    case SetTimeS:
        return make_entry(entry, 0x30160200u, ND_RECORD_CAPTURE_START_MODE);

    case DayTabWrS:
    case SeasonTabWrS:
    case WeekTabWrS:
    case BillDayWrS:
        return make_entry(entry,
            0x30170200u + ((uint32_t)(record_id - DayTabWrS) << 16),
            ND_RECORD_CAPTURE_START_MODE);

    case CoverOpenS:
    case CoverOpenE:
        return make_entry(entry, 0x301B0200u,
            (uint8_t)(ND_RECORD_CAPTURE_START_MODE + (record_id - CoverOpenS)));

    case SwitchOffS:
    case SwitchOnS:
    case HolidayWrS:
    case ActCombWrS:
        return make_entry(entry,
            0x301F0200u + ((uint32_t)(record_id - SwitchOffS) << 16),
            ND_RECORD_CAPTURE_START_MODE);

    case FeeTabWrS:
    case StepTabWrS:
    case SecretKeyWrS:
    case ErrPlugCardS:
    case RechargeS:
    case RefundS:
        return make_entry(entry,
            0x30240200u + ((uint32_t)(record_id - FeeTabWrS) << 16),
            ND_RECORD_CAPTURE_START_MODE);

    case SwitchErrS:
    case SwitchErrE:
        return make_entry(entry, 0x302B0200u,
            (uint8_t)(ND_RECORD_CAPTURE_START_MODE + (record_id - SwitchErrS)));

    case PowerAbnormalS:
    case PowerAbnormalE:
        return make_entry(entry, 0x302C0200u,
            (uint8_t)(ND_RECORD_CAPTURE_START_MODE + (record_id - PowerAbnormalS)));

    case RTCErrS:
    case RTCErrE:
        return make_entry(entry, 0x302E0200u,
            (uint8_t)(ND_RECORD_CAPTURE_START_MODE + (record_id - RTCErrS)));

    case EmuErrS:
    case EmuErrE:
        return make_entry(entry, 0x302F0200u,
            (uint8_t)(ND_RECORD_CAPTURE_START_MODE + (record_id - EmuErrS)));

    case BroadcastTimeS:
        return make_entry(entry, 0x303C0200u, ND_RECORD_CAPTURE_START_MODE);

    case ZeroLineErrS:
    case ZeroLineErrE:
        return make_entry(entry, 0x30400200u,
            (uint8_t)(ND_RECORD_CAPTURE_START_MODE + (record_id - ZeroLineErrS)));

    case InstantFreeze:
        return make_entry(entry, 0x50000200u, 0);

    case HourFreeze:
    case DayFreeze:
    case BillFreeze:
    case MonthFreeze:
    case SeasonTabCutFreeze:
    case DayTabCutFreeze:
    case FeePriceCutFreeze:
    case StepCutFreeze:
        return make_entry(entry,
            0x50030200u + ((uint32_t)(record_id - HourFreeze) << 16), 0);

    case StepBillFreeze:
        return make_entry(entry, 0x50110200u, 0);

    default:
        break;
    }

    if ((record_id >= MinuteFreeze1) && (record_id <= MinuteFreeze8)) {
        uint8_t scheme = (uint8_t)(record_id - MinuteFreeze1);
        return make_entry(entry, 0x50020200u + ((uint32_t)scheme << 13), scheme);
    }

    return 0;
}
