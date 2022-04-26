#include "nidavellir.h"
#include "fr_698.h"

int _f101_m127_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    struct mbase_control_block_t* mbase_cb;

    uint8_t  i, method, dar = 0, count, maxnum;
    uint16_t oad;
    uint8_t* p = buf;
    uint32_t temp = 0, s_index = 0, addr = 0;
    uint8_t* ptr_tmp = instance->apdu_cache;

    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    method = (unsigned char)((id & 0x0000FF00) >> 8);    //方法

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -1;
    }

    uint16_t oi, right;
    struct obj_right_t
    {
        uint16_t oi;
        uint16_t right;
    }obj;

    if ( (p[0] != TYPE_structure) || (p[1] != 2))
    {
        return -1;
    }

    p += 2;

    if ((p[0] != TYPE_OI) || (p[3] != TYPE_long_unsigned))
    {
        return -1;
    }
    p += 2;

    obj.oi = buf_to_uint16_msb(p + 1);
    obj.right = buf_to_uint16_msb(p + 4);
    p += 4;

    instance->apdu_cache[0] = 0;
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
    nd_get_index(instance, s_index, ptr_tmp, SecurityMode_EXData_LEN);

    maxnum = (SecurityMode_EXData_LEN - 1) / 4;
    count = *ptr_tmp++;

    if (count >= maxnum)
    {
        return -1;
    }

    for (i = 0; i < count; i++)
    {
        oi = buf_to_uint16_lsb(ptr_tmp);
        right = buf_to_uint16_lsb(ptr_tmp + 2);

        if (oi == obj.oi)
        {
            if (right == obj.right)
            {
                return (p - buf);
            }
            else
            {
                memcpy(ptr_tmp + 2, (uint8_t*)&obj.right, 2);

                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
                nd_set_index(instance, s_index, instance->apdu_cache, SecurityMode_EXData_LEN);

                return (p - buf);
            }
        }

        ptr_tmp += 4;
    }

    memcpy(ptr_tmp, (uint8_t*)&obj, 4);
    instance->apdu_cache[0]++;

    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
    nd_set_index(instance, s_index, instance->apdu_cache, SecurityMode_EXData_LEN);

    return (p - buf);
}
int _f101_m128_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx) 
{
    struct mbase_control_block_t* mbase_cb;
    uint8_t  i, dar = 0, count, maxnum;
    uint8_t* p = buf;
    uint32_t temp = 0, s_index = 0, addr = 0;
    uint8_t* ptr_tmp = instance->apdu_cache;
    uint16_t oi, tmp_oi;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -1;
    }

    if ( (p[0] != TYPE_OI))
    {
        return -1;
    }
    p++;

    oi = buf_to_uint16_msb(p);
    p += 2;
    instance->apdu_cache[0] = 0;
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
    nd_get_index(instance, s_index, ptr_tmp, SecurityMode_EXData_LEN);

    maxnum = (SecurityMode_EXData_LEN - 1) / 4;
    count = *ptr_tmp++;

    if ((count == 0) || (count > maxnum))
    {
        return -1;
    }

    for (i = 0; i < count; i++)
    {
        tmp_oi = buf_to_uint16_lsb(ptr_tmp);

        if (tmp_oi == oi)
        {
            break;
        }

        ptr_tmp += 4;
    }

    if (i == count)
    {
        return -1;
    }

    instance->apdu_cache[0]--;
    memcpy(ptr_tmp, ptr_tmp + 4, instance->apdu_cache + SecurityMode_EXData_LEN - ptr_tmp);

    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
    nd_set_index(instance, s_index, instance->apdu_cache, SecurityMode_EXData_LEN);

    return (p - buf);

}
int _f101_m129_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    struct mbase_control_block_t* mbase_cb;
    uint8_t  i, dar = 0, count, maxnum;
    uint8_t* p = buf;
    uint32_t temp = 0, s_index = 0, addr = 0;
    uint8_t* ptr_tmp = instance->apdu_cache;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -1;
    }

    uint8_t  num, write_flag, noadd_flag;
    uint16_t len, oi, right;

    struct obj_right_t
    {
        uint16_t oi;
        uint16_t right;
    }obj;


    if (p[0] != TYPE_array)
    {
        return -1;
    }
    p++;

    instance->apdu_cache[0] = 0;
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
    nd_get_index(instance, s_index, ptr_tmp, SecurityMode_EXData_LEN);

    num = *p++;

    count = *ptr_tmp++;
    len = count * 4 + 1;
    write_flag = 0;

    if (len > SecurityMode_EXData_LEN)
    {
        return -1;
    }

    while (num--)
    {
        noadd_flag = 0;
        ptr_tmp = instance->apdu_cache + 1;
        p += 2;

        if ((p[0] != TYPE_OI) || (p[3] != TYPE_long_unsigned))
        {
            return -1;
        }

        obj.oi = buf_to_uint16_msb(p + 1);
        p += 4;
        obj.right = buf_to_uint16_msb(p);
        p += 2;

        for (i = 0; i < count; i++)
        {
            oi = buf_to_uint16_lsb(ptr_tmp);
            right = buf_to_uint16_lsb(ptr_tmp + 2);

            if (oi == obj.oi)
            {
                if (right != obj.right)
                {
                    memcpy(ptr_tmp + 2, (uint8_t *)&obj.right, 2);
                    write_flag = 1;
                }

                noadd_flag = 1;
                break;
            }

            ptr_tmp += 4;
        }

        if (noadd_flag == 0)
        {
            count++;
            len = count * 4 + 1;

            if (len > SecurityMode_EXData_LEN)
            {
                return -1;
            }

            memcpy(ptr_tmp, (uint8_t*)&obj.oi, 2);
            memcpy(ptr_tmp + 2, (uint8_t*)&obj.right, 2);
            write_flag = 1;
        }
    }

    if (write_flag == 1)
    {
        instance->apdu_cache[0] = count;

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
        nd_set_index(instance, s_index, instance->apdu_cache, SecurityMode_EXData_LEN);

        return (p - buf);
    }

    return (p - buf);
}
int ACTION_F_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;
    
    struct context_698_t* p_ctx = (struct context_698_t*)ctx;
    struct fifo_t* cache = p_ctx->apdu;
    uint8_t *apdu_rx = p_ctx->apdu_d_rx + p_ctx->apdu_rx_offset;
    uint16_t offset = 0;
    
    uint8_t  method,dar=0,buffer[16],keybuf[128],len1=0,len2=0;
    uint16_t oad,pageid=0,txlen=0,rxlen=0;
    uint8_t *p = buf;
    uint32_t temp = 0,s_index=0,addr=0;
    uint8_t* ptr_tmp = instance->apdu_cache;

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
                rxlen = read_esam_data(instance, addr , keybuf , txlen);
                
                //cache->buf_ptr[cache->wr++] = 0;  //成功
                //cache->buf_ptr[cache->wr++] = 1;  //有数据
                cache->wr = cache->wr+2;
                cache->buf_ptr[cache->wr++] = TYPE_octet_string;  
                if( rxlen <= 127 )
                {
                    cache->buf_ptr[cache->wr++] = rxlen;  //长度
                }
                else if(rxlen <= 0xFF)
                {
                    cache->buf_ptr[cache->wr++] = 0x81;  //长度
                    cache->buf_ptr[cache->wr++] = rxlen;  //长度
                }
                memcpy((uint8_t*)&cache->buf_ptr[cache->wr], keybuf, rxlen);
                cache->wr = cache->wr + rxlen;
                
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
                write_esam(instance, addr , keybuf , txlen);
                addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Meter_Number);
                nd_set_index(instance, addr, (p - 23), 6); 
                return (p-buf);
            }
            else if(method == 0x05) //协商失效
            {
                p = p + 1;
                mbase_cb->authen_tmr = 0;
                mbase_cb->irauthen_tmr = 0;
                return (p-buf);
            }
            else if(method == 0x07) //秘钥更新
            {
                uint8_t *pdata = apdu_rx + 3;
                rxlen = esam_key_updata(instance, pdata, txlen);
                p = p + 1692;
                //成功后 再触发 密钥更新事件
                if(rxlen != 0xff)
                {
                    send_event_mesg_pro(instance, NULL, SecretKeyWrS, NULL, 1);
                }
                return (p-buf);
            }
            else if(method == 0x0B) //红外请求
            {
                read_esam_no(instance, buffer);
                memcpy(keybuf, (uint8_t*)&buffer[5], 8);//表号, 8bytes

                addr = 0x02003680;
                txlen = 0;
                write_esam(instance, addr , buffer , txlen);
                memcpy((uint8_t*)&keybuf[16], buffer, 8);   //ESAM序列号, 8bytes
                
                p = p + 2;
                memcpy((uint8_t*)&keybuf[8], p, 8); //随机数
                p = p + 8;
                memcpy(buffer, keybuf, 16); // 表号，随机数
                addr = 0x03080880;
                txlen = 0x0010;
                rxlen = write_esam(instance, addr , buffer , txlen);  //80 08 08 03 00 10 + 表号，随机数，返回密文
                
                addr = 0x08000480;
                txlen = 0;
                rxlen = write_esam(instance, addr , (uint8_t*)&keybuf[24] , txlen);  //返回随机数2
                
                cache->buf_ptr[cache->wr++] = 0;  //成功
                cache->buf_ptr[cache->wr++] = 1;  //有数据
                cache->buf_ptr[cache->wr++] = TYPE_structure;
                cache->buf_ptr[cache->wr++] = 4;
                cache->buf_ptr[cache->wr++] = TYPE_octet_string;
                cache->buf_ptr[cache->wr++] = 6;
                memcpy((uint8_t*)&cache->buf_ptr[cache->wr], (uint8_t*)&keybuf[2], 6);//表号
                cache->wr = cache->wr + 6;
                
                cache->buf_ptr[cache->wr++] = TYPE_octet_string;
                cache->buf_ptr[cache->wr++] = 8;
                memcpy((uint8_t*)&cache->buf_ptr[cache->wr], (uint8_t*)&keybuf[16], 8);//ESAM序列号
                cache->wr = cache->wr + 8;
                
                cache->buf_ptr[cache->wr++] = TYPE_octet_string;
                cache->buf_ptr[cache->wr++] = 8;
                memcpy((uint8_t*)&cache->buf_ptr[cache->wr], (uint8_t*)&buffer[0], 8);//随机数1密文
                cache->wr = cache->wr + 8;
                
                cache->buf_ptr[cache->wr++] = TYPE_RN;
                cache->buf_ptr[cache->wr++] = 8;
                memcpy((uint8_t*)&cache->buf_ptr[cache->wr], (uint8_t*)&keybuf[24], 8);//随机数2
                cache->wr = cache->wr + 8;
                return (p-buf);
            }
            else if(method == 0x0C) //红外认证
            {
                //80 06 00 01 00 08 + 密文
                p = p + 2;
                memcpy(buffer, p, 8); 
                p = p + 8;
                addr = 0x01000680;
                txlen = 8;
                rxlen = write_esam(instance, addr , buffer , txlen);
                if(rxlen == 0xff)
                {
                    return -1;
                }
                else
                {
                    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + IrAuthenTime);
                    nd_get_index(instance, addr, buffer, 4); 
                    mem_swap_cpy((uint8_t*)&mbase_cb->irauthen_tmr, buffer, 4);
                    mbase_cb->irauthen_tmr = mbase_cb->irauthen_tmr * 60;
                    return (p-buf);
                }
            }
            break;
        case 0xF101://安全模式字
        {
            if (method == 1) //复位
            {
                if ((*p) != TYPE_integer)
                {
                    return -1;
                }
                memset(ptr_tmp, 0, SecurityMode_EXData_LEN);
                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
                nd_set_index(instance, s_index, ptr_tmp, SecurityMode_EXData_LEN);
                p += 2;
                return (p - buf);
            }
            else if (method == 127)
            {
                return _f101_m127_action(instance, id, buf, buf_size, ctx);
            }
            else if (method == 128)
            {
                return _f101_m128_action(instance, id, buf, buf_size, ctx);
            }
            else if (method == 129)
            {
                return _f101_m129_action(instance, id, buf, buf_size, ctx);
            }
            else
                return -DAR_OBJECT_NOT_EXIST;
        }
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
        case 0xF202://红外
            if(method == 127)
            {
                p = p + 13;
                return -DAR_COMM_SPEED_CANNOT_CHANGE;
            }
            else
                return -DAR_OBJECT_NOT_EXIST;
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
                mbase_cb->MulPortMode = *p++;
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
                    if(0 != p[0]){
                        return -DAR_TYPE_NOT_MATCH;
                    }
                    if( mbase_cb->KeyPtr == 0 )
                        mbase_cb->KeyPtr = mbase_cb->lcd_keydis_list_num-1;
                    else
                        mbase_cb->KeyPtr--;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + KeyDisplayList + mbase_cb->KeyPtr);
                    nd_get_index(instance, s_index, (uint8_t*)&pageid, 2); 
                    mbase_cb->Page_ID = pageid;
                    mbase_cb->KeyInv = mbase_cb->lcd_keydis_keep_sec;
                    mbase_cb->BackLightTmr = 60;
                    return 1;
                case 0x04://上翻
                    if(0 != p[0]){
                        return -DAR_TYPE_NOT_MATCH;
                    }
                    if( mbase_cb->KeyPtr >= (mbase_cb->lcd_keydis_list_num-1) )
                        mbase_cb->KeyPtr = 0;
                    else
                        mbase_cb->KeyPtr++;
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + KeyDisplayList + mbase_cb->KeyPtr);
                    nd_get_index(instance, s_index, (uint8_t*)&pageid, 2); 
                    mbase_cb->Page_ID = pageid;
                    mbase_cb->KeyInv = mbase_cb->lcd_keydis_keep_sec;
                    mbase_cb->BackLightTmr = 60;
                    return 1;
                case 0x05://显示查看
                    break;
                case 0x06://全屏显示
                    mbase_cb->AutoPtr = 0xFF;
                    mbase_cb->Page_ID = 0xFFFF;
                    p++;
                    mem_swap_cpy((uint8_t*)&mbase_cb->AutoInv, p, 2);
                    p = p + 2;
                    
                    if((mbase_cb->lcd_poweron_all_dis_time > 30) || (mbase_cb->lcd_poweron_all_dis_time == 0))
                        mbase_cb->BackLightTmr = 5;
                    else
                        mbase_cb->BackLightTmr = mbase_cb->lcd_poweron_all_dis_time;
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
