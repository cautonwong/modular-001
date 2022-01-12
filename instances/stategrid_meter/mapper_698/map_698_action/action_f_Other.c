#include "nidavellir.h"
#include "fr_698.h"



int ACTION_F_Other(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t  method,dar=0,buffer[15],keybuf[30],len1=0,len2=0;
    uint16_t oad,pageid=0,txlen=0;
    uint8_t *p = buf;
    uint32_t temp = 0,s_index=0,addr=0;
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    method = (unsigned char)((id & 0x0000FF00) >> 8);    //方法
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    switch (oad)
    {
        case 0xF100://ESAM
            if(method == 0x03)  //ESAM数据读取
            {
                p++;
                memcpy((uint8_t*)&addr, p, 4);//sid
                p = p + 4;
                txlen = *p++;
                memcpy(keybuf, p, txlen);
                p = p + txlen;
                read_esam_data(addr , keybuf , txlen);
                
                return (p-buf);
            }
            else if(method == 0x04) //ESAM数据更新
            {
                p = p + 3;
                len1 = *p++;
                memcpy(buffer, p, len1);//oad+data
                p = p + len1;
                p++;
                memcpy((uint8_t*)&addr, p, 4);//sid
                p = p + 4;
                len2 = *p++;
                mem_swap_cpy((uint8_t*)&txlen, p, 2);//len
                p = p + 2;
                memcpy(keybuf, p, len2-2);
                p = p + len2-2;
                p++;
                memcpy((uint8_t*)&keybuf[len2-2], buffer, len1);
                memcpy((uint8_t*)&keybuf[len2-2+len1], p, 4);//MAC
                p = p + 4;
                write_esam(addr , keybuf , txlen);
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Number);
                nd_set_index(instance, addr, (p - 23), 6); 
                return (p-buf);
            }
            break;
        case 0xF201://RS485
            if(method == 127)
            {
                p = p + 3;
                mem_swap_cpy((uint8_t*)&temp, p, 4);
                p = p + 5;
                if(temp != 0xF2010201)
                {
                    dar = DAR_OBJECT_NOT_EXIST;
                    return -1;
                }
                if((*p > 10) || (*(p+1) != 2) || (*(p+2) != 8) || (*(p+3) != 1) || (*(p+4) != 0) || (*(p+6) != 0))
                {
                    dar = DAR_BUFFER_OVERFLOW;
                    return -1;
                }
                mbase_cb->Baut_rs485 = *p;
                p = p + 7;
            }
            else
            {
                dar = DAR_OBJECT_NOT_EXIST;
                return -1;
            }
            return (p-buf);
            break;
        case 0xF205://继电器输出
            if(method == 127)
            {
                p = p + 3;
                mem_swap_cpy((uint8_t*)&temp, p, 4);
                p = p + 5;
                if(temp != 0xF2050201)
                {
                    dar = DAR_OBJECT_NOT_EXIST;
                    return -1;
                }
                if((*p != 0) && (*p != 1))
                {
                    dar = DAR_BUFFER_OVERFLOW;
                    return -1;
                }
                mbase_cb->relay_ctl_mode = *p++;
                p = p + 2;
            }
            else
            {
                dar = DAR_OBJECT_NOT_EXIST;
                return -1;
            }
            return (p-buf);
        case 0xF207://多功能端子
            if(method == 127)
            {
                p = p + 3;
                mem_swap_cpy((uint8_t*)&temp, p, 4);
                p = p + 5;
                if(temp != 0xF2070201)
                {
                    dar = DAR_OBJECT_NOT_EXIST;
                    return -1;
                }
                if((*p != F_PortSec) && (*p != F_PortTouCut))
                {
                    dar = DAR_BUFFER_OVERFLOW;
                    return -1;
                }
                mbase_cb->MulPortMode = *p;
                p = p + 2;
            }
            else
            {
                dar = DAR_OBJECT_NOT_EXIST;
                return -1;
            }
            return (p-buf);
        case 0xF209://载波模块
            if(method == 128)
            {
                p = p + 3;
                mem_swap_cpy((uint8_t*)&temp, p, 4);
                p = p + 5;
                if(temp != 0xF2090201)
                {
                    dar = DAR_OBJECT_NOT_EXIST;
                    return -1;
                }
                if((*p > 10) || (*(p+1) != 2) || (*(p+2) != 8) || (*(p+3) != 1) || (*(p+4) != 0))
                {
                    dar = DAR_BUFFER_OVERFLOW;
                    return -1;
                }
                mbase_cb->Baut_plc = *p;
                p = p + 5;
            }
            else
            {
                dar = DAR_OBJECT_NOT_EXIST;
                return -1;
            }
            return (p-buf);
        case 0xF301://按键循显
            switch(method)
            {
                case 0x03://下翻
                    if( mbase_cb->KeyPtr == 0 )
                        mbase_cb->KeyPtr = mbase_cb->lcd_keydis_list_num-1;
                    else
                        mbase_cb->KeyPtr--;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + KeyDisplayList + mbase_cb->KeyPtr);
                    nd_get_index(instance, s_index, &pageid, 2); 
                    mbase_cb->Page_ID = pageid;
                    mbase_cb->KeyInv = mbase_cb->lcd_keydis_keep_sec;
                    return (p-buf);
                case 0x04://上翻
                    if( mbase_cb->KeyPtr >= (mbase_cb->lcd_keydis_list_num-1) )
                        mbase_cb->KeyPtr = 0;
                    else
                        mbase_cb->KeyPtr++;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + KeyDisplayList + mbase_cb->KeyPtr);
                    nd_get_index(instance, s_index, &pageid, 2); 
                    mbase_cb->Page_ID = pageid;
                    mbase_cb->KeyInv = mbase_cb->lcd_keydis_keep_sec;
                    return (p-buf);
                case 0x05://显示查看
                    break;
                case 0x06://全屏显示
                    mbase_cb->AutoPtr = 0xFF;
                    mbase_cb->Page_ID = 0xFFFF;
                    p++;
                    mem_swap_cpy((uint8_t*)&mbase_cb->AutoInv, p, 2);
                    p = p + 2;
                    return (p-buf);
                default:
                    break;
            }
        default:
            dar = DAR_OBJECT_NOT_EXIST;
            break;
    }
    return 0x01;
}
