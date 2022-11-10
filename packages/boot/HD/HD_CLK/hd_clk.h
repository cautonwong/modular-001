/*!
 * \file      hd_clk.h
 *
 * \brief     hd_clk implementation
 *
 * \endcode
 *
 * \author	   
 */
/* Include ----------------------------------------------------------------------*/
#ifndef _HD_CLK_H_
#define _HD_CLK_H_
/* Macro ------------------------------------------------------------------------*/

/* type prototype ---------------------------------------------------------------*/
typedef struct
{
	/*!
	 * \brief	CLK init when power on
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitPwrOn)(void);
	/*!
	 * \brief	CLK init when power down
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitPwrDown)(void);
}HD_CLK_API_t;
/* Public Variables -------------------------------------------------------------*/
extern const HD_CLK_API_t g_stClkApi;
/* End --------------------------------------------------------------------------*/
#endif
