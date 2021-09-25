/**
 * @defgroup    tou_cfg Tou Cfg
 * @ingroup     cfg
 * @see     tou
 * @brief       tou≈‰÷√.
 *
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "tou.h"


extern struct tou_adapter_t tariff_adapter;

const struct tou_cfg_t CODE tou_cfg[] =
{
    &tariff_adapter,
};


const uint8 CODE tou_cfg_num = sizeof(tou_cfg) / sizeof(struct tou_cfg_t);
/**@}*/