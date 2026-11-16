#ifndef ND_TARGET_STATEGRID_UPGRADE_INFO_H
#define ND_TARGET_STATEGRID_UPGRADE_INFO_H

#include <stdint.h>

#define upgrade_setsize 40

struct upgrade_info_t
{
    uint16_t magic_num;
    uint16_t file_verification;
    uint16_t frm_size;
    uint16_t frm_num;
    uint32_t file_size;
    uint16_t cur_verification;
    uint16_t frm_no_last;
    uint32_t flash_addr;
    uint8_t type;
    uint8_t status;
    uint8_t remained_1;
    uint8_t remained_2;
    uint8_t bits[upgrade_setsize];
};

#endif
