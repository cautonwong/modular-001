/*!
 * \file      main.c
 *
 * \brief     main implementation
 *
 * \endcode
 *
 * \author	  
 */
/* Include ----------------------------------------------------------------------*/
#include "Includes.H"

/* type prototype ---------------------------------------------------------------*/
#define UPDATE_SUCCESS         (0x5aa55aa5)
#define UPDATE_DONE            (0xa55aa55a)


#define EEADDR_UpDataFlg       0                      // Éı¼¶ĞÅÏ¢µØÖ· ///* Éı¼¶±êÖ¾Î» *///


enum image_transfer_mode_t
{
	mode_01 = 0x01,/*0x01å‡çº§æ¨¡å¼ åœ¨çº¿å‡çº§ï¼Œplc imageBlockSizeå›ºå®šä¸º128å­—èŠ‚*/
	mode_02 = 0x02,/*0x02å‡çº§æ¨¡å¼ åœ¨çº¿å‡çº§ï¼Œrs485 imageBlockSizeå›ºå®šä¸º196å­—èŠ‚*/ 
	mode_03 = 0x03,/*0x03å‡çº§æ¨¡å¼     åœ¨çº¿å‡çº§ï¼ŒIR imageBlockSizeå›ºå®šä¸º128å­—èŠ‚*/
	mode_81 = 0x81,/*0x01å‡çº§æ¨¡å¼ æœ¬åœ°å‡çº§ï¼Œplc imageBlockSizeå›ºå®šä¸º128å­—èŠ‚*/
	mode_82 = 0x82,/*0x02å‡çº§æ¨¡å¼ æœ¬åœ°å‡çº§ï¼Œrs485 imageBlockSizeå›ºå®šä¸º196å­—èŠ‚*/ 
	mode_83 = 0x83,/*0x03å‡çº§æ¨¡å¼     æœ¬åœ°å‡çº§ï¼ŒIR imageBlockSizeå›ºå®šä¸º128å­—èŠ‚*/
};

extern unsigned char DLT97Prog(unsigned char program_ch, unsigned char block_size, unsigned int write_crc);
extern void Uart0Init(void);
extern void Uart4Init(void);
extern void Uart5Init(void);
extern void HD_CLK_InitPwrOn(void);
extern uint8_t HD_Storage_Access(uint32_t Addr, uint8_t *pDataBuf, uint32_t Len, HD_IIC_Operation_e Operate);
typedef struct
{
    uint16_t FileCrc;
    uint32_t FlgUpdate;
    uint32_t FileSize;
}GS_File_Ctrl_t;
/* Variables --------------------------------------------------------------------*/
GS_File_Ctrl_t FileCtrl;
uint8_t CodeBuff[256];
/* Const ------------------------------------------------------------------------*/
struct image_transfer_state_t
{
	unsigned char   transfer_enable;	/*	0x5a: Ê¹ÄÜ´ò¿ª  0x00:Ê¹ÄÜ¹Ø±Õ 						*/
	unsigned char   transfer_mode;		/*	0x01Éı¼¶Ä£Ê½Ò»ÎªÕë¶ÔÔØ²¨¡¢RFµÈÍ¨ĞÅ·½Ê½£¬imageBlockSize
                                            ¹Ì¶¨Îª64×Ö½Ú£»0x02Éı¼¶Ä£Ê½¶şÎªÕë¶Ô¹âµç¡¢RS485¡¢GPRSµÈÍ¨
                                            ĞÅ·½Ê½£¬imageBlockSize¹Ì¶¨Îª192×Ö½Ú¡£*/
	unsigned char   block_size;			/*	block³¤¶È					  						*/
	unsigned char   last_blocksize;		/*	×îºóÒ»Ö¡³¤¶È										*/

	unsigned char   file_type;			/*	ÎÄ¼şÀàĞÍ											*/
	unsigned char   encryption_type;	/*	¼ÓÃÜÀàĞÍ											*/
	unsigned char   check_type;			/*	Ğ£ÑéÀàĞÍ											*/
	unsigned char   version[20];		/*	°æ±¾ĞÅÏ¢£¬¸ß×Ö½ÚÔÚºó								*/
	unsigned char   code_crc[16];		/*	³ÌĞòĞ£ÑéÂë£¬¸ß×Ö½ÚÔÚºó								*/

	unsigned char   enable_rst[4];		/*	ÊÇ·ñ¿ªÊ¼Éı¼¶±êÖ¾£¬0x5aa5Ğ£ÑéÍ¨¹ı¿ªÊ¼Éı¼¶£¬0000²»Éı¼¶*/
	unsigned char   block_num[4];		/*	block×ÜÊı											*/

	unsigned char   datatime[4];		/*	¼¤»îÊ±¼ä£¬×ª»»ÎªÃë*/
	unsigned char   image_enable[4];	/*´¦ÓÚÉı¼¶Ä£Ê½*/
};
/* End --------------------------------------------------------------------------*/

int main(void)
{
    unsigned char fsl_status=0;
    unsigned short    write_crc=0;
    struct image_transfer_state_t image_transfer_state;
    
    HD_CLK_InitPwrOn();
    g_stSystemApi.pInitPwrOn();
    g_stSystemApi.WDT_Clear();
    g_stSystemApi.Delay_ms(100);
          
    memset( (unsigned char*)&image_transfer_state, 0x00, sizeof(image_transfer_state));
    HD_Storage_Access(EEADDR_UpDataFlg, (unsigned char*)&image_transfer_state, 61, STORAGE_READ);
#if 0
    image_transfer_state.transfer_enable    = 0x03;
    image_transfer_state.file_type          = 0xFD;
    image_transfer_state.block_size         = 0xC0;
    image_transfer_state.transfer_mode      = 0x02;
#endif
    if((image_transfer_state.transfer_enable == 0x09) && (image_transfer_state.file_type == 0xFD) && ((image_transfer_state.block_size == 128) || (image_transfer_state.block_size == 196))
       && (((image_transfer_state.transfer_mode & 0x7F) == 0x03) || ((image_transfer_state.transfer_mode & 0x7F) == 0x02) || ((image_transfer_state.transfer_mode & 0x7F) == 0x01)))
    {
        /******************** ½ÓÊÕ±¨ÎÄ£¬¿ªÊ¼Éı¼¶ *****************/ 
        if (0x01 == (image_transfer_state.transfer_mode & 0x7F))
        {
            Uart4Init();    //PLC
        }
        else if (0x02 == (image_transfer_state.transfer_mode & 0x7F))
        {
            Uart0Init();    //485Í¨µÀ
        }
		else if (0x03 == (image_transfer_state.transfer_mode & 0x7F))
		{
			Uart5Init();    //ºìÍâÍ¨µÀ
		}
        
        HBF_LOOP_LBF((unsigned char*)&write_crc, (unsigned char*)&image_transfer_state.code_crc[0], 2);
        fsl_status = DLT97Prog((image_transfer_state.transfer_mode & 0x7F), image_transfer_state.block_size, write_crc);
        
        //Éı¼¶³É¹¦£¬Çå³ıÉı¼¶Ê¹ÄÜ¡¢Éı¼¶Ä£Ê½¡¢Éı¼¶ÅäÖÃÎÄ¼ş
        if(fsl_status == 0)
        {
            memset( (unsigned char*)&image_transfer_state, 0x00, sizeof(image_transfer_state));
            HD_Storage_Access(EEADDR_UpDataFlg, (unsigned char*)&image_transfer_state, sizeof(image_transfer_state), STORAGE_WRITE);
        }
        
        g_stSystemApi.Jump2App(APP_START_ADDR);
        
    }
    else
    {
        if(   (image_transfer_state.transfer_enable != 0) || (image_transfer_state.file_type != 0) 
           || ((image_transfer_state.block_size == 128) || (image_transfer_state.block_size == 196))   || (image_transfer_state.transfer_mode != 0) ){

            memset( (unsigned char*)&image_transfer_state, 0x00, sizeof(image_transfer_state));
            HD_Storage_Access(EEADDR_UpDataFlg, (unsigned char*)&image_transfer_state, sizeof(image_transfer_state), STORAGE_WRITE);
        }
        g_stSystemApi.Jump2App(APP_START_ADDR);
    }
    while(1);

}

void assert_errhandler(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
