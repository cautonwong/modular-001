

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

#include "../HD_GPIO/hd_gpio.h"
#include "../HD_IIC/hd_iic.h"
//#include "../HD_RTC/hd_rtc.h"
#include "../HD_SPI/hd_spi_hard.h"
#include "../HD_SPI/hd_spi_hard_typedef.h"
//#include "../HD_SPI/hd_spi_soft.h"
//#include "../HD_SPI/hd_spi_soft_typedef.h"
#include "../HD_SYSTEM/hd_system.h"
//#include "../HD_TIMER/hd_timer.h"
#include "../HD_UART/hd_uart.h"
#include "../HD_CLK/hd_clk.h"
//#include "hd_flash_map.h"

#define __DEBUG__    (0)
#if __DEBUG__
#include "target.h"
#include "hd_gpio_list.h"
#endif

#define PNULL   ((void*)0)
#define SHIELD_WARNING(X) (void)X

typedef void (*pFunc)(void);
