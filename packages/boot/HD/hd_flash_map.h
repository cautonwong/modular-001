/*!
 * \file      hd_flash_map.h
 *
 * \brief     hd_flash_map implementation
 *
 * \endcode
 *
 * \author	  hxiii 
 */
/* Include ----------------------------------------------------------------------*/
#ifndef _HD_FLASH_MAP_H_
#define _HD_FLASH_MAP_H_
/* Macro ------------------------------------------------------------------------*/
#define EX_FLASH_PAGE_SIZE                        (0x100)
#define EX_FLASH_SECTOR_SIZE                      (0x1000)
#define EX_FLASH_BLOCK_64K_SIZE                   (0x10000)
#define EX_FLASH_DVK_LEN                          (EX_FLASH_SECTOR_SIZE * (1024) - 1)
#define EX_FLASH_ADDR_MAX_LEN                     (2)
/* Addr -------------------------------------------------------------------------*/
#define ADDR_START_EX                             (0x00000000)
#define EX_FLASH_SECTOR_0                         (ADDR_START_EX + EX_FLASH_SECTOR_SIZE * 0)
#define ADDR_CODE_OLD_IMAGE                       (EX_FLASH_SECTOR_0)
#define EX_FLASH_SECTOR_256                       (ADDR_START_EX + EX_FLASH_SECTOR_SIZE * 256)
#define ADDR_CODE_OLD_IMAGE_END                   (EX_FLASH_SECTOR_256)

#define ADDR_CODE_NEW_IMAGE                       (ADDR_CODE_OLD_IMAGE_END)
#define EX_FLASH_SECTOR_512                       (ADDR_START_EX + EX_FLASH_SECTOR_SIZE * 512)
#define ADDR_CODE_NEW_IMAGE_END                   (EX_FLASH_SECTOR_512)

#define ADDR_CODE_OLD_IMAGE_SIZE                  (ADDR_CODE_NEW_IMAGE_END)
#define EX_FLASH_SECTOR_513                       (ADDR_START_EX + EX_FLASH_SECTOR_SIZE * 513)
#define ADDR_CODE_OLD_IMAGE_SIZE_END              (EX_FLASH_SECTOR_513)

#define ADDR_FLASH_END                            (EX_FLASH_DVK_LEN)
/* End --------------------------------------------------------------------------*/
#endif

