#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

/* Shared by the COSEM classes and the port, so one definition per image. */
const nd_obis_t ND_OBIS_ACTIVITY_CALENDARS[4] = {
    {0, 0, 13, 0, 0, 255}, {0, 0, 13, 0, 1, 255},
    {0, 0, 13, 0, 2, 255}, {0, 1, 13, 0, 1, 255},
};

const nd_obis_t ND_OBIS_SPECIAL_DAYS_TABLES[4] = {
    {0, 0, 11, 0, 0, 255}, {0, 0, 11, 0, 1, 255},
    {0, 0, 11, 0, 2, 255}, {0, 1, 11, 0, 1, 255},
};

int nd_obis_eq(const nd_obis_t *left, const nd_obis_t *right)
{
    return left->a == right->a && left->b == right->b && left->c == right->c
        && left->d == right->d && left->e == right->e && left->f == right->f;
}
