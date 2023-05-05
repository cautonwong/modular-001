
#include "string.h"
#include "Includes.H"
#include "target.h"
#include "updata.h"

#define FALSE	            (0)
#define TRUE	            (1)
#define XM_OK			    (0x00)
#define XM_ADDRESS_ERROR	(0x01)
#define XM_COMMS_ERROR		(0x02)
#define XM_TIMEOUT		    (0x04)
#define XM_PROG_FAIL		(0x08)

#define FLASH_ADR           APP_CODE_START_ADDR
//#define DLT97_SIZE          158

#define		Comm_MaxSize			240//212
#define		Comm_MinSize			12//68 A0 A1 A2 A3 A4 A5 68 CTL LEN....CS 16
#define 	RS_ID				10

#define 	RsPLC_Head1    				rev_buf[0]
#define 	RsPLC_Addr    				rev_buf[1]
#define 	RsPLC_Head2   				rev_buf[7]
#define 	RsPLC_Ctl     				rev_buf[8]
#define 	RsPLC_Len     				rev_buf[9]
#define 	RsPLC_ID0    				rev_buf[10]
#define 	RsPLC_ID1    				rev_buf[11]
#define 	RsPLC_Pass	    			rev_buf[12]
#define 	RsPLC_DataR    				rev_buf[12]
#define		RsPLC_User                  rev_buf[16]
#define 	RsPLC_DataW    				rev_buf[20]
#define 	RsPLC_CRC     				rev_buf[RS_ID+RsPLC_Len]
#define 	RsPLC_End     				rev_buf[RS_ID+RsPLC_Len+1]

unsigned char rev_buf[240];
unsigned char send_buf[30];
extern uint8_t HD_Storage_Access(uint32_t Addr, uint8_t *pDataBuf, uint32_t Len, HD_IIC_Operation_e Operate);
extern uint8_t DF_Storage_Access(uint32_t Addr, uint8_t *pDataBuf, uint32_t Len, uint8_t Operate);
extern uint8_t recovery(uint8_t *para);

/************************************************************
CRC16码表
************************************************************/
static unsigned int const c16tbl[256] =
{
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

void UART0_Send_Byte(unsigned char data)//485  发送
{
    unsigned long tick;
    
    UART0->DATA = data;
    tick=0x3ffff;
    while(((UART0->STATE & BIT5) == 0) && (tick))
    {
        g_stSystemApi.WDT_Clear();//清看门狗
        tick--;
    }
    
    UART0->STATE = BIT5;
    
}

void UART4_Send_Byte(unsigned char data)//BLE  发送
{
    unsigned long tick;
    
    UART4->DATA = data;
    tick=0x3ffff;
    while(((UART4->STATE & BIT5) == 0) && (tick))
    {
        g_stSystemApi.WDT_Clear();//清看门狗
        tick--;
    }
    
    UART4->STATE |= BIT5;
    
}

void UART5_Send_Byte(unsigned char data)//红外  发送
{
    unsigned long tick;
    
    UART5->DATA = data;
    tick=0x3ffff;
    while(((UART5->STATE & BIT5) == 0) && (tick))
    {
        g_stSystemApi.WDT_Clear();//清看门狗
        tick--;
    }
    
    UART5->STATE |= BIT5;
    
}

void Send_Byte(unsigned char program_ch, unsigned char data)
{
    if(program_ch == 0x01)//载波
    {
        UART4_Send_Byte(data);
    }
    else if(program_ch == 0x02)//485
    {
        UART0_Send_Byte(data);
    }
	else if(program_ch == 0x03)//红外
    {
        UART5_Send_Byte(data);
    }
    
}

void send_response(unsigned long total_block, unsigned long current_block, unsigned char* addr_buf, unsigned char program_ch)
{
    unsigned char ucChecksum = 0,temp[8]={0,},i;

    if(program_ch == 2)    
    {
        GPIOA->DAT &= ~BIT7; //485数据发送
    }
    
    HBF_LOOP_LBF((unsigned char*)&temp[0],(unsigned char*)&total_block,4);
    HBF_LOOP_LBF((unsigned char*)&temp[4],(unsigned char*)&current_block,4);

    Send_Byte(program_ch, 0x68);ucChecksum += 0x68;
    for(i=0;i<6;i++)
    {
        Send_Byte(program_ch, addr_buf[i]);ucChecksum += addr_buf[i];
    }
    
    Send_Byte(program_ch, 0x68);ucChecksum += 0x68;
    Send_Byte(program_ch, 0x84);ucChecksum += 0x84;
    Send_Byte(program_ch, 0x0A);ucChecksum += 0x0A;
    Send_Byte(program_ch, 0x30);ucChecksum += 0x30;
    Send_Byte(program_ch, 0xFD);ucChecksum += 0xFD;
    for(i=0;i<8;i++)
    {
        temp[i] = temp[i] + 0x33;
        Send_Byte(program_ch, temp[i]);ucChecksum += temp[i];
    }
    
    Send_Byte(program_ch, ucChecksum);
    Send_Byte(program_ch, 0x16);
    
    if(program_ch == 2)    
    {
        GPIOA->DAT |= BIT7; //485数据接收
        UART0->CTRL |= BIT1; //打开接收
        //UART0->CTRL &= ~BIT0;    //关闭发送
    }
    else if(program_ch == 0x01)
    {
        UART4->CTRL |= BIT1; //载波打开接收
        //UART4->CTRL &= ~BIT0;    //关闭发送
    }
	else if(program_ch == 0x03)
	{
		UART5->CTRL |= BIT1; 	//红外打开接收
        UART5->CTRL &= ~BIT0;    //关闭发送
	}
}

unsigned char recvice_byte(unsigned char program_ch)
{
    unsigned long tick=0xffFFfff;
    unsigned char ptr = 0;
    
    if(program_ch == 2)    
    {   
        if((UART0->CTRL & BIT1) == 0)
        {
            GPIOA->DAT |= BIT7; //485数据接收
            UART0->CTRL |= BIT1; //打开接收
            UART0->CTRL &= ~BIT0;    //关闭发送
        }
    }
    else if(program_ch == 0x01)
    {
        if((UART4->CTRL & BIT1) == 0)
        {
            UART4->CTRL |= BIT1; //载波打开接收
            UART4->CTRL &= ~BIT0;    //关闭发送
        }
    }
	else if(program_ch == 0x03)
	{
        if((UART4->CTRL & BIT1) == 0)
        {
            UART5->CTRL |= BIT1; 	//红外打开接收
            UART5->CTRL &= ~BIT0;    //关闭发送
        }      
	}
    
    while(tick)//接收数据
    {
        g_stSystemApi.WDT_Clear();//清看门狗
        if(program_ch == 0x01)//BLE
        {
            if(UART4->STATE & BIT1)
            {
                rev_buf[ptr] = UART4->DATA;
                if( (RsPLC_Head1 != 0x68) || ((ptr == 7) && (RsPLC_Head2 != 0x68)) || (ptr >= (Comm_MaxSize-1)) )
                {
                    ptr = 0;
                    continue;
                }
                ptr++;
                if( ptr >= Comm_MinSize )
                {
                    if( ptr > (RsPLC_Len+Comm_MinSize) ) ptr = 0;
                    else if( ptr == (RsPLC_Len+Comm_MinSize) )
                    {
                        if( RsPLC_End == 0x16 )
                        {
                                UART4->CTRL &= ~BIT1; //关闭接收
                                UART4->CTRL |= BIT0;    //打开发送
                                return 1;//数据接收完成
                        }
                        else ptr = 0;
                    }
                }
            }
        }
        else if(program_ch == 0x02)//485
        {
            if(UART0->STATE & BIT1)
            {
                rev_buf[ptr] = UART0->DATA;
                if( (RsPLC_Head1 != 0x68) || ((ptr == 7) && (RsPLC_Head2 != 0x68)) || (ptr >= (Comm_MaxSize-1)) )
                {
                    ptr = 0;
                    continue;
                }
                ptr++;
                if( ptr >= Comm_MinSize )
                {
                    if( ptr > (RsPLC_Len+Comm_MinSize) ) ptr = 0;
                    else if( ptr == (RsPLC_Len+Comm_MinSize) )
                    {
                        if( RsPLC_End == 0x16 )
                        {
                                UART0->CTRL &= ~BIT1; //关闭接收
                                UART0->CTRL |= BIT0;    //打开发送
                                return 1;//数据接收完成
                        }
                        else ptr = 0;
                    }
                }
            }
        }
		else if(program_ch == 0x03)//红外
        {
            if(UART5->STATE & BIT1)
            {
                rev_buf[ptr] = UART5->DATA;
                if( (RsPLC_Head1 != 0x68) || ((ptr == 7) && (RsPLC_Head2 != 0x68)) || (ptr >= (Comm_MaxSize-1)) )
                {
                    ptr = 0;
                    continue;
                }
                ptr++;
                if( ptr >= Comm_MinSize )
                {
                    if( ptr > (RsPLC_Len+Comm_MinSize) ) ptr = 0;
                    else if( ptr == (RsPLC_Len+Comm_MinSize) )
                    {
                        if( RsPLC_End == 0x16 )
                        {
                                UART5->CTRL &= ~BIT1; //关闭接收
                                UART5->CTRL |= BIT0;    //打开发送
                                return 1;//数据接收完成
                        }
                        else ptr = 0;
                    }
                }
            }
        }
        tick--;
        
    }
	UART5->STATE = 0x3C;
    UART4->STATE = 0x3C;
    UART0->STATE = 0x3C;
    return 0;
}

void Erase_app()
{
    for(uint16_t Cnt = 0; Cnt < 464; Cnt++) //
    {
        g_stSystemApi.Flash_EraseSector(APP_CODE_START_ADDR + Cnt * 512);
        g_stSystemApi.WDT_Clear();
    }
}
/*========================================================================
*函 数 名: DLT97Prog
*
*参    数: flashStartAddress:升级数据的起始地址, program_ch:升级通道, write_crc:CRC16校验码
*
*功能描述: 通过97协议来升级程序
*
*返 回 值: XM_OK:升级成功，其它:升级失败
*
*作    者: 
========================================================================*/
unsigned char DLT97Prog(unsigned char program_ch, unsigned char block_size, unsigned short write_crc)
{
    unsigned char commom_buf[7] = {0},wTableNo=0, fota_flg = 0;
    unsigned long total_block = 0,current_block = 0,i,ProgNum=0,address=0;
    unsigned short tmp=0,cal_crc = 0xFFFF, cal_crc1 = write_crc;
 
    
    while (1)
    {
        //接收完数据后先判断帧格式是否正确
        if (recvice_byte(program_ch) != 1)
        {
            continue;//return XM_COMMS_ERROR;
        }

        //判断表地址是否正确
        //EERead(commom_buf, Comm_Number, 6);
#if 0
        HD_Storage_Access(0x010C, commom_buf, 7, STORAGE_READ);
        if (memcmp((unsigned char *)&commom_buf[1], (unsigned char *)&rev_buf[1], 6) != 0)
        {
            continue;//return XM_COMMS_ERROR;
        }
#endif
        for (i = 0; i < rev_buf[9]; i++)
        {
            rev_buf[10+i] -= 0x33;
        }

        //判断升级的数据ID是否正确
        if ((rev_buf[10] != 0xFD) || (rev_buf[11] != 0xCA))
        {
            continue;//return XM_COMMS_ERROR;
        }

        //block总数
        total_block = ((unsigned long)rev_buf[16] << 24) + ((unsigned long)rev_buf[17] << 16) + ((unsigned long)rev_buf[18] << 8) + rev_buf[19];
        if(total_block == 0){
            continue;
        }
        
        //当前block号
        current_block = ((unsigned long)rev_buf[20] << 24) + ((unsigned long)rev_buf[21] << 16) + ((unsigned long)rev_buf[22] << 8) + rev_buf[23];
        if ((current_block == ProgNum) && (ProgNum < (total_block - 1)))
        {            
            g_stSystemApi.WDT_Clear();
            
            if(0 == current_block){
                if(0 == updata(rev_buf+24)){
                    fota_flg = 1;
                }else{
                    Erase_app();
                }
            }
            if(fota_flg){
                (*((uint32_t *)0x40000028)) |= 16;
                DF_Storage_Access(0+current_block*(block_size), (uint8_t*)&rev_buf[24], block_size, 0x30);
                (*((uint32_t *)0x40000028)) &= ~16;
            }else{
                fota_flg = 0;
                GPIOE->DAT |= BIT12;    //拉闸灯
                g_stSystemApi.Flash_ProgramByte(FLASH_ADR+current_block*(block_size), (uint8_t*)&rev_buf[24], block_size);
                GPIOE->DAT &= ~BIT12;    //拉闸灯
            }
            ProgNum++;
            memcpy((unsigned char*)&rev_buf[12],(unsigned char*)&rev_buf[16],8);
            send_response(total_block, current_block, (unsigned char*)&rev_buf[1],program_ch);//响应帧
        }
        //最后一包数据
        else if (current_block == (total_block-1))
        {
            g_stSystemApi.WDT_Clear();
            
            if(fota_flg){
                (*((uint32_t *)0x40000028)) |= 16;
                DF_Storage_Access(0+current_block*(block_size), (uint8_t*)&rev_buf[24], rev_buf[9]-14, 0x30);
                (*((uint32_t *)0x40000028)) &= ~16;
                cal_crc = updata(NULL);
                if (0 == cal_crc){
                    send_response(total_block, current_block, (unsigned char*)&rev_buf[1],program_ch);//响应帧
                    return 0;
                }else{
                    send_response(cal_crc1, cal_crc, (unsigned char*)&rev_buf[1],program_ch);//响应帧
                    while(1);
                }
            }else{
                GPIOE->DAT |= BIT12; 
                g_stSystemApi.Flash_ProgramByte(FLASH_ADR+current_block*(block_size), (uint8_t*)&rev_buf[24], rev_buf[9]-14);
                GPIOE->DAT &= ~BIT12; 
            
                address = APP_CODE_START_ADDR+current_block*(block_size) + rev_buf[9]-14;
                for (i = APP_CODE_START_ADDR; i < address; i++)
                {
                    tmp = *((const unsigned char   *)(i));
                    wTableNo = (cal_crc & 0xff) ^ (tmp & 0xff);
                    cal_crc  = ((cal_crc >> 8) & 0xff) ^ c16tbl[wTableNo];
                }

                cal_crc = (((unsigned int)(cal_crc >> 8) & 0x00ff) | ((unsigned int)(cal_crc << 8) & 0xFF00));
                if (cal_crc1 == cal_crc)
                {
                    send_response(total_block, current_block, (unsigned char*)&rev_buf[1],program_ch);//响应帧
                    return 0;
                }
                else
                {
                    send_response(cal_crc1, cal_crc, (unsigned char*)&rev_buf[1],program_ch);//响应帧
                    while(1);
                }
            }
        }
        else
        {
            send_response(total_block, ProgNum, (unsigned char*)&rev_buf[1],program_ch);//响应帧
        }
    }
}

