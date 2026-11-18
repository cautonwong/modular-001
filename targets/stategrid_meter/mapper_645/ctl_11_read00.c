#include "645id_table.h"
#include <stddef.h>
#include <stdbool.h>
#include "application/adapters/dl698/meter_base_view.h"
#include "port.h"
#include "targets/stategrid_meter/app/resource_blocks.h"

int read_00( void *instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx)
{
    uint8_t IDL0, IDL1, data[100]={0,}, len = 0, i, fv=0;
    uint16_t IDH=0;
    uint32_t oad698=0, temp=0, s_index=0;
    
    IDL0 = ID & 0x000000FF; //µ±Ç°orÀúÊ·
    IDL1 = (uint8_t)((ID & 0x0000FF00) >> 8);   //Êı¾İ¿éor×Ü·ÑÂÊ
    IDH = (uint16_t)((ID & 0xFFFF0000) >> 16);
    
    switch(IDH)
    {
        case 0x0000:    //×éºÏ
            oad698 = 0x00000200;
            break;
        case 0x0001:    //ÕıÏò
            oad698 = 0x00100200;
            break;
        case 0x0002:    //·´Ïò
            oad698 = 0x00200200;
            break;
        case 0x000C:    //ÔÂ¶ÈÓÃµçÁ¿
            oad698 = 0x20310200;
            break;
        case 0x000B:    //½áËãÖÜÆÚÓÃµçÁ¿
            oad698 = 0x20310200;
            break;    
        default:
            return -1;
            break;
    }
    if(IDL1 == 0xFF)    //Êı¾İ¿é
    {
        oad698 = oad698 & 0xFFFFFF00;
    }
    else
    {
        if((IDH == 0x0000) || (IDH == 0x0001) || (IDH == 0x0002))
        {
            s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para+3);//¶ÁÈ¡·ÑÂÊÊı
            nd_get_index(instance, s_index, (uint8_t *)&fv, 1);
            if(IDL1 > fv) return -1;
            oad698 = oad698 | (IDL1+1); //·ÑÂÊ
        }
    }
    
    if(IDL0 == 0x00)//µ±Ç°
    {
        if(IDL1 == 0xFF)    //Êı¾İ¿é
        {
            nd_get(instance, oad698, data, 9, NULL);
            
            for(i=0; i<*(data+1); i++)
            {
                mem_swap_cpy((uint8_t*)&temp, data + 2 + 5*i + 1, 4);
                if((IDH == 0x0000) || (IDH == 0x0003) || (IDH == 0x0004))//å¸¦ç¬¦å·
                {
                    HEX_BCD_Signed((int32_t)temp , p_tx+i*4 , 4);
                }
                else
                {
                    HEX_BCD(p_tx+i*4, temp);
                }
                len = len + 4;
            }
        }
        else
        {
            nd_get(instance, oad698, data, 9, NULL);
            mem_swap_cpy((uint8_t*)&temp, data+1, 4);
            if ((IDH == 0x0000) || (IDH == 0x0003) || (IDH == 0x0004))//å¸¦ç¬¦å·
            {
                HEX_BCD_Signed((int32_t)temp , p_tx , 4);
            }
            else
            {
                HEX_BCD(p_tx, temp);
            }
            len = 4;
        }
    }
    else
    {   
    }
    
    return len;
}
