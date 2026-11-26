#include "components/services/time_format/time_format.h"

#include "domain/common/calendar.h"

const uint8_t MonTab[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

unsigned char GetMonMaxDay(unsigned char *Addr)
{
    return (unsigned char)nd_calendar_days_in_month(Addr[0], Addr[1]);
}

unsigned char Date_Format_Check(unsigned char *Addr)
{
    if (Addr[0] < 1 || Addr[0] > 31 || Addr[1] < 1 || Addr[1] > 12) {
        return 1;
    }
    if (Addr[0] > GetMonMaxDay(Addr + 1)) {
        return 1;
    }
    return 0;
}

unsigned char Time_Format_Check(unsigned char *Addr)
{
    if (Addr[0] > 23 || Addr[1] > 59 || Addr[2] > 59) {
        return 1;
    }
    return 0;
}
