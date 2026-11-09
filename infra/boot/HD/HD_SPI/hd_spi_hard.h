/*!
 * \file      hd_spi_hard.h
 *
 * \brief     hd_rtc_hard implementation
 *
 * \endcode
 *
 * \author	  hxiii 
 */
/* Include ----------------------------------------------------------------------*/
#ifndef _HD_SPI_HARD_H_
#define _HD_SPI_HARD_H_
/* Macro ------------------------------------------------------------------------*/

/* type prototype ---------------------------------------------------------------*/
typedef enum
{
    HD_FLASH_ERASE    = 0, //need erase when address is the start of sector
    HD_FLASH_NO_ERASE = 1,
}HD_Spi_Hw_Erase_e;

typedef struct
{
	/*!
	 * \brief	SPI(Hard) init when power on
	 * \param	none		
	 * \retval	none
	 */
    void (*inti_pwron)(void);
	/*!
	 * \brief	SPI(Hard) init when power down
	 * \param	none		
	 * \retval	none
	 */
    void (*init_pwroff)(void);

    uint8_t (*write)(uint32_t addr, uint8_t *p_data, uint16_t data_len, HD_Spi_Hw_Erase_e erase_flag);

    uint8_t (*read)(uint32_t addr, uint8_t *p_data, uint16_t data_len);
}HD_SPI_HARD_API_t;
/* Public Variables -------------------------------------------------------------*/
extern const HD_SPI_HARD_API_t spi_hw_api;
/* End --------------------------------------------------------------------------*/
#endif
