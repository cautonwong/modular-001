#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>

void test_msta_read_meter_data_periodicly(void **state) 
{
    struct instance_t* instance;
    instance = (struct instance_t*)*state;
    struct tm time = 
    {
        .tm_year = 2021 - 1900,   /* years are counted from 1900 */
        .tm_mon = 3,                      /* 0 = January, 11 = December */
        .tm_mday = 30,
        .tm_hour = 23,
        .tm_min = 59,
        .tm_sec = 57
    };
    init_test_environment();
    nidavellir_run_for_n_seconds_from_time(instance, 60*60, &time);
    nidavellir_run(instance);
    nidavellir_run(instance);

}
