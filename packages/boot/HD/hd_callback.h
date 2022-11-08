/*!
 * \file      hd_callback.h
 *
 * \brief     hd_callback implementation
 *
 * \endcode
 *
 * \author	  hxiii 
 */
/* Include ----------------------------------------------------------------------*/
#ifndef _HD_CALLBACK_H_
#define _HD_CALLBACK_H_
/* Macro ------------------------------------------------------------------------*/

/* type prototype ---------------------------------------------------------------*/
typedef struct
{
	/*!
	 * \brief	Timer init when power on
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitPwrOn)(void);
	/*!
	 * \brief	Timer init when power down
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitPwrDown)(void);
}HD_CALLBACK_API_t;

typedef struct
{
    void (*Rtc_CalInit)(void);
}HD_CALLBACK_Reg_t;
/* Public Variables -------------------------------------------------------------*/
extern const HD_CALLBACK_API_t g_stCallbackApi;
extern HD_CALLBACK_Reg_t       g_stCallbackReg;
/* End --------------------------------------------------------------------------*/
#endif
