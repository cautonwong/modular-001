#include "domain/events/record_capture.h"

uint32_t nd_record_capture_oad(nd_record_capture_kind_t kind, uint32_t oad,
                               uint8_t program_first_is_time_set)
{
    switch (kind) {
    case ND_RECORD_CAPTURE_SET_TIME:
        if (oad == 0x201E0200u) {
            oad = 0x40002200u;
        } else if (oad == 0x20200200u) {
            oad = 0x40008200u;
        }
        break;
    case ND_RECORD_CAPTURE_PROGRAM:
        if (oad == 0x201E0200u && program_first_is_time_set) {
            oad = 0x40002200u;
        }
        break;
    default:
        if (oad == 0x40002200u) {
            oad = 0x201E0200u;
        } else if (oad == 0x40008200u) {
            oad = 0x20200200u;
        }
        break;
    }

    if ((oad & 0xF0000F00u) == 0x00000200u) {
        oad += 0x00000200u;
    }
    return oad;
}

static uint8_t end_only_oad(uint32_t oad)
{
    switch (oad) {
    case 0x33110206u: /* refund amount */
    case 0x33100206u: /* card serial */
    case 0x33100207u: /* card error word */
    case 0x33100208u: /* card command */
    case 0x33100209u: /* card error status */
    case 0x33150206u: /* holiday object */
    case 0x33150207u: /* holiday content */
    case 0x33140206u: /* time table object */
    case 0x33140207u: /* time table content */
    case 0x330C0206u: /* event clear */
    case 0x20220200u: /* event sequence */
    case 0xFFFE0001u: /* operator code */
        return 1;
    default:
        return 0;
    }
}

uint8_t nd_record_capture_visible(uint8_t mode, uint32_t oad,
                                  nd_record_kind_t record_kind)
{
    uint8_t scheme = (uint8_t)(0x07u & (uint8_t)(oad >> 13));
    uint8_t valid = 1;

    if (end_only_oad(oad)) {
        if (mode == ND_RECORD_CAPTURE_END_MODE) {
            valid = 0;
        }
    } else {
        switch (oad) {
        case 0x20210200u: /* freeze time */
        case 0x20230200u: /* freeze record */
        case 0x33020206u: /* event program */
        case 0xFFFE0000u: /* 645 program list */
            break;
        case 0x33000200u: /* channel report state */
        case 0x20240200u: /* event source */
        case 0x201E0200u: /* event start time */
            if (mode == ND_RECORD_CAPTURE_END_MODE) {
                valid = 0;
            }
            break;
        case 0x20200200u: /* event end time */
            if (mode == ND_RECORD_CAPTURE_START_MODE) {
                valid = 0;
            }
            if (record_kind == ND_RECORD_KIND_METER_WR
                || record_kind == ND_RECORD_KIND_METER_WR_645) {
                valid = 0;
            }
            break;
        default:
            if (mode == ND_RECORD_CAPTURE_START_MODE) {
                if (scheme == 0 || scheme == 3 || scheme == 4) {
                    valid = 0;
                }
            } else if (mode == ND_RECORD_CAPTURE_END_MODE) {
                if (scheme == 0 || scheme == 1 || scheme == 2) {
                    valid = 0;
                }
                if (record_kind == ND_RECORD_KIND_METER_WR_R
                    || record_kind == ND_RECORD_KIND_METER_WR_645_R) {
                    valid = 0;
                }
            } else if (mode != scheme) {
                valid = 0;
            }
            break;
        }
    }

    if (record_kind == ND_RECORD_KIND_SET_TIME) {
        valid = 1;
    }
    return valid;
}
