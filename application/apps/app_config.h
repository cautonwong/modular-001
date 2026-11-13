#ifndef ND_APPLICATION_APPS_APP_CONFIG_H
#define ND_APPLICATION_APPS_APP_CONFIG_H

#include <stdint.h>

#define _DEBUG_TEST 0
#if (_DEBUG_TEST == 1)
    #define PWD_TEST_ON 1
    #define FREEZ_TEST_OVERTURN 0
    #define RESET_INFO 0
    #define TEST_RELAY_LED 0
#else
    #define PWD_TEST_ON 0
    #define FREEZ_TEST_OVERTURN 0
    #ifndef RESET_INFO
        #define RESET_INFO 0
    #endif
#endif

#define CF_M_NORMAL_698 1
#define CF_M_FD_645 (CF_M_NORMAL_698 + 1)
#define CF_M_STANDARD CF_M_NORMAL_698

#if (CF_M_STANDARD == CF_M_NORMAL_698)
    #define DEF_BAUT_PLC 10
    #define DEF_645_FIRST_ZONE_SET_EN 0
    #define DEF_645_PRIVATE_KEY_METERCLR_EN 0
#elif (CF_M_STANDARD == CF_M_FD_645)
    #define DEF_BAUT_PLC 6
    #define DEF_645_FIRST_ZONE_SET_EN 1
    #define DEF_645_PRIVATE_KEY_METERCLR_EN 1
#else
    #define DEF_BAUT_PLC 10
    #define DEF_645_FIRST_ZONE_SET_EN 0
    #define DEF_645_PRIVATE_KEY_METERCLR_EN 0
#endif

#define METER_TYPE_CONFIG (0x0801)
#define RtcBat_Low_Level_Dwn 290
#define RtcBat_Low_Level_Up 300

#ifndef __no_init
    #define __no_init
#endif

#if RESET_INFO
extern __no_init uint32_t resetinfo_file;
extern __no_init uint32_t resetinfo_line;
#endif

#endif
