/*!
 * \file      hd_spi_hard_typedef.h
 *
 * \brief     hd_spi_hard implementation
 *
 * \endcode
 *
 * \author	  hxiii 
 */
/* Include ----------------------------------------------------------------------*/

/* Macro ------------------------------------------------------------------------*/
#define CMD_WRITE_ENABLE            (0x06)
#define CMD_WRITE_DISABLE           (0x04)
#define CMD_READ                    (0x03)
#define CMD_WRITE                   (0x02)
#define CMD_SECTOR_ERASE            (0x20)
#define CMD_READ_REGISTER           (0x05)
#define CMD_WRITE_REGISTER          (0x01)
#define CMD_BLOCK64K_ERASE          (0xD8)

//#define SPI_CS_LOW()                {IO_DATAFLASH_CS_L;SET_OUTPUT_DATAFLASH_CS;}
//#define SPI_CS_HIGH()               {IO_DATAFLASH_CS_H;SET_OUTPUT_DATAFLASH_CS;}

#define SPI_CS_LOW()                {IO_SPCS1_L;Set_Output_SPCS1;}
#define SPI_CS_HIGH()               {IO_SPCS1_H;Set_Output_SPCS1;}

#define SPI_WP_LOW()                {;}
#define SPI_WP_HIGH()               {;}
/* type prototype ---------------------------------------------------------------*/
/* Private Variables ------------------------------------------------------------*/
/* Function ---------------------------------------------------------------------*/
/*!
 * \brief	SPI(Hard) init when power on
 * \param	none		
 * \retval	none
 */
void hd_spi_hw_init_pwron(void);
/*!
 * \brief	SPI(Hard) init when power down
 * \param	none
 * \retval	none
 */
void hd_spi_hw_init_pwroff(void);

uint8_t hd_spi_hw_write(uint32_t addr, uint8_t *p_data, uint16_t data_len, HD_Spi_Hw_Erase_e erase_flag);

uint8_t hd_spi_hw_read(uint32_t addr, uint8_t *p_data, uint16_t data_len);
/* End --------------------------------------------------------------------------*/