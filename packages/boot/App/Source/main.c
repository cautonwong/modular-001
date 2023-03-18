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


#define EEADDR_UpDataFlg       0                      // 升级信息地址 ///* 升级标志位 *///


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
	unsigned char   transfer_enable;	/*	0x5a: 使能打开  0x00:使能关闭 						*/
	unsigned char   transfer_mode;		/*	0x01升级模式一为针对载波、RF等通信方式，imageBlockSize
                                            固定为64字节；0x02升级模式二为针对光电、RS485、GPRS等通
                                            信方式，imageBlockSize固定为192字节。*/
	unsigned char   block_size;			/*	block长度					  						*/
	unsigned char   last_blocksize;		/*	最后一帧长度										*/

	unsigned char   file_type;			/*	文件类型											*/
	unsigned char   encryption_type;	/*	加密类型											*/
	unsigned char   check_type;			/*	校验类型											*/
	unsigned char   version[20];		/*	版本信息，高字节在后								*/
	unsigned char   code_crc[16];		/*	程序校验码，高字节在后								*/

	unsigned char   enable_rst[4];		/*	是否开始升级标志，0x5aa5校验通过开始升级，0000不升级*/
	unsigned char   block_num[4];		/*	block总数											*/

	unsigned char   datatime[4];		/*	激活时间，转换为秒*/
	unsigned char   image_enable[4];	/*处于升级模式*/
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
    if((image_transfer_state.transfer_enable == 0x09) && (image_transfer_state.file_type == 0xFD) && ((image_transfer_state.block_size == 0x80) || (image_transfer_state.block_size == 0xC0))
       && ((image_transfer_state.transfer_mode == 0x03) || (image_transfer_state.transfer_mode == 0x02) || (image_transfer_state.transfer_mode == 0x01)))
    {
        /******************** 接收报文，开始升级 *****************/ 
        if (0x01 == image_transfer_state.transfer_mode)
        {
            Uart4Init();    //PLC
        }
        else if (0x02 == image_transfer_state.transfer_mode)
        {
            Uart0Init();    //485通道
        }
		else if (0x03 == image_transfer_state.transfer_mode)
		{
			Uart5Init();    //红外通道
		}
        
        HBF_LOOP_LBF((unsigned char*)&write_crc, (unsigned char*)&image_transfer_state.code_crc[0], 2);
        fsl_status = DLT97Prog(image_transfer_state.transfer_mode, image_transfer_state.block_size, write_crc);
        
        //升级成功，清除升级使能、升级模式、升级配置文件
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
           || ((image_transfer_state.block_size == 0x80) || (image_transfer_state.block_size == 0xC0))   || (image_transfer_state.transfer_mode != 0) ){

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
