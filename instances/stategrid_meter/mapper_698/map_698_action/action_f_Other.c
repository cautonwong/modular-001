#include "nidavellir.h"
#include "fr_698.h"

#define PROGRAMEND1	0x03FFFF	//程序空间1结束地址
#define PROGRAMPROZ	0x03FFFF//0x03F800	//程序保护区起始地址
#define DefEsamProcLen  2000 //默认ESAM处理的最大长度：字节



void Xor_Data( unsigned char *BufA, unsigned char *BufB, uint16_t Num )
{
	uint16_t i;

	for( i=0; i<Num; i++ )
	{
		BufA[i] ^= BufB[i];
	}
}
uint16_t Byte2uShort(uint8_t* byte)
{
  return byte[1]+((uint16_t)byte[0]<<8);
}
uint32_t Byte2uInt(uint8_t* byte)
{
  return byte[3]+((uint32_t)byte[2]<<8) + ((uint32_t)byte[1]<<16) +((uint32_t)byte[0]<<24) ;
}
//文件传输类接口
uint16_t ActionFile( struct instance_t* instance, uint32_t id, uint8_t* inbuf, uint16_t buf_size, void *ctx, uint8_t* outbuf)
{
    struct mbase_control_block_t* mbase_cb;
	unsigned char i, result;
	//RetFrame Ret = {0,0,0,0};
	//union B16_B08  DataLen;
	unsigned char *Ptr;//,codebuf[400];
	unsigned char BufA[16], RnBuf[64];
//	unsigned char code *Code_Flash_Char;
    const unsigned char *Code_Flash_Char;
	unsigned char *RtBuf = instance->apdu_cache;
	uint16_t cc16,DataLen;
	uint32_t cc32;
    uint8_t *p = inbuf;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }

	if( !Byte2uShort(p+17) || ( Byte2uShort(p+17) > DefEsamProcLen ) || ((Byte2uShort(p+17)%64)!=0) || (p[3] != 0x00) ) 
	{
		return -DAR_RW_REJECT;//每次比较数据长度<1K 拒绝读写
	}
	
	result = p[5];
	mem_swap_cpy((uint8_t*)&DataLen, p+17, 2);	//比较的数据长度
  
	//判断地址是否异常和准备数据
	for( i=0; i<3; i++ )
	{
		if( i== 0 ) //比对因子8字节
		{
			Ptr = BufA;
			cc16 = 16; 
			cc32 = Byte2uInt(p+7);//比对因子起始地址
		}
		else if( i== 1 ) //随机数64/4字节
		{
			Ptr = RnBuf;
			cc16 = 64; 
			cc32 = Byte2uInt(p+12);//比对数据起始地址
		}
		else 				//比对数据
		{
			Ptr = RtBuf;
			cc16 = Byte2uShort( p+17); 
			cc32 = Byte2uInt(p+12);//比对数据起始地址
		}
		Code_Flash_Char = (const uint8_t*)cc32;
		if( (cc32 > PROGRAMEND1)|| p[7] || p[12] )
		{
			break;		//地址异常
		}
		else if(cc32+cc16>PROGRAMEND1+1)
		{
			memset(Ptr,0,cc16);
			memcpy(Ptr,Code_Flash_Char,PROGRAMEND1+1-cc32);
			Ptr[PROGRAMEND1+1-cc32] = 0x80;
		}
		else if( (cc32 < PROGRAMPROZ) && (cc32+cc16 > PROGRAMPROZ) )
		{
			memset(Ptr,0,cc16);
			memcpy(Ptr,Code_Flash_Char,PROGRAMPROZ-cc32);
			Ptr[PROGRAMPROZ-cc32] = 0x80;
		}
		else
		{
			memcpy(Ptr,Code_Flash_Char,cc16);
		}
	}
	
	if( i != 3 )
	{
		return -DAR_ADDRESS_ABNORMAL;   //地址异常
	}
	
	Xor_Data( BufA, BufA+8, 8 ); //16字节比对因子，前后8字节异或，得到8字节比对因子BufA

	for( i=1; i<4; i++ )
	{
		Xor_Data( RnBuf, RnBuf+16*i, 16 ); //随机数平均4等分，异或后得到data5
	}
	
	cc16 = cc16/4;
	for( i=1; i<4; i++ )
	{
		Xor_Data( RtBuf, RtBuf+cc16*i, cc16 ); //数据平均4等分，异或后得到data10
	}

	return software_esam_check(instance, BufA, RnBuf, RtBuf, result, cc16, outbuf);

}

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
        return -DAR_OTHERS_ERROR;
    }

    uint16_t oi, right;
    struct obj_right_t
    {
        uint16_t oi;
        uint16_t right;
    }obj;

    if ( (p[0] != TYPE_structure) || (p[1] != 2))
    {
        return -DAR_OTHERS_ERROR;
    }

    p += 2;

    if ((p[0] != TYPE_OI) || (p[3] != TYPE_long_unsigned))
    {
        return -DAR_OTHERS_ERROR;
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
        return -DAR_OTHERS_ERROR;
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
        return -DAR_OTHERS_ERROR;
    }

    if ( (p[0] != TYPE_OI))
    {
        return -DAR_OTHERS_ERROR;
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
        return -DAR_OTHERS_ERROR;
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
        return -DAR_OTHERS_ERROR;
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
        return -DAR_OTHERS_ERROR;
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
        return -DAR_OTHERS_ERROR;
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
        return -DAR_OTHERS_ERROR;
    }

    while (num--)
    {
        noadd_flag = 0;
        ptr_tmp = instance->apdu_cache + 1;
        p += 2;

        if ((p[0] != TYPE_OI) || (p[3] != TYPE_long_unsigned))
        {
            return -DAR_OTHERS_ERROR;
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
                return -DAR_OTHERS_ERROR;
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

//协商失效
int connect_failture(struct instance_t* instance, uint8_t* req_buf, struct context_698_t* ctx )
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct fifo_t* cache = ctx->apdu;
    uint8_t cilent_index, temp[42]={0};
    uint8_t *p_req = req_buf;
    uint8_t *p_res = &cache->buf_ptr[cache->wr];
    uint8_t *p_res_tmp = p_res;
    
    if (*p_req++ != 0)
    {
        return -DAR_OTHERS_ERROR;
    }
    
    if (CA_TYPE_TER(ctx->CA)) 
    {
        cilent_index = 1;
    }
    else
    {
        cilent_index = 0;
    }

    mbase_cb->connect_timeout[ctx->channel][cilent_index] = 0;
    mbase_cb->esam_connect_timeout[ctx->channel][cilent_index] = 0;
    mbase_cb->authen_tmr = 0; // 待调整
    mbase_cb->irauthen_tmr = 0;


    *p_res++ = TYPE_date_time_s;
    rtc_get_from_tm(instance,temp);
    *p_res++ = temp[6];
    *p_res++ = temp[5];
    *p_res++ = temp[4];
    *p_res++ = temp[3];
    *p_res++ = temp[2];
    *p_res++ = temp[1];
    *p_res++ = temp[0];

    cache->wr += (p_res - p_res_tmp);

    return (p_req - req_buf);
}




int ACTION_F_Other(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct pipeline_t* pipeline =  get_pipeline(instance, pid_meter_base); 
    struct context_698_t* p_ctx = (struct context_698_t*)ctx;
    struct fifo_t* cache = p_ctx->apdu;
    uint8_t *apdu_rx = p_ctx->apdu_d_rx + p_ctx->apdu_rx_offset;
    uint16_t offset = 0;
    
    uint8_t  method,dar=0,buffer[16],keybuf[128],len1=0,len2=0, cilent_index = 0,len=0;
    uint16_t oad,pageid=0,txlen=0,rxlen=0;
    int16_t complen;
    uint8_t *p = buf,disp_sequ=0;
    uint32_t temp = 0,s_index=0,addr=0,displayid=0,displayoad1=0,displayoad2=0;
    uint8_t* ptr_tmp = instance->apdu_cache;
    
    //struct fifo_t* cache_t = p_ctx->apdu;
    uint8_t* p_tx = cache->buf_ptr;

    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    method = (unsigned char)((id & 0x0000FF00) >> 8);    //方法
    
    
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }
    
    if (CA_TYPE_TER(p_ctx->CA)) 
    {
        cilent_index = 1;
    }
    else
    {
        cilent_index = 0;
    }
    
    switch (oad)
    {
        case 0xF001://软件比对
            if(method != 0x0a)  return -DAR_OTHERS_ERROR;
            
            complen = ActionFile(instance, id, p, buf_size, ctx, ptr_tmp);
            if(complen > 0)
            {
                //p[cache->wr++] = 0;  //成功
                //p[cache->wr++] = 1;  //有数据
                cache->wr = cache->wr + 2;
                p_tx[cache->wr++] = TYPE_structure;
                p_tx[cache->wr++] = 1;
                p_tx[cache->wr++] = TYPE_octet_string;
                
                if(complen <= 127) 
                {
                    p_tx[cache->wr++] = complen;
                }
                else if(complen <= 0xff)
                {
                    p_tx[cache->wr++] = 0x81;
                    p_tx[cache->wr++] = complen;
                }
                else 
                {
                    p_tx[cache->wr++] = 0x82;
                    p_tx[cache->wr++] = complen>>8;
                    p_tx[cache->wr++] = complen;
                }
                
                memcpy((uint8_t*)&p_tx[cache->wr], ptr_tmp, complen);
                cache->wr = cache->wr + complen;
                return (22);
            }
            else 
              return -DAR_OTHERS_ERROR;
            break;
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
                mem_swap_cpy((uint8_t*)&addr, p, 4);//sid
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
                return connect_failture(instance, buf, ctx); 
            }
            else if(method == 0x07) //秘钥更新
            {
                uint8_t *pdata = apdu_rx + 3;
                rxlen = esam_key_updata(instance, pdata, txlen);
                p = p + 1692;
                //成功后 再触发 密钥更新事件
                if(rxlen != 0xff)// 待调整  秘钥更新事件  缓存冲突
                {
                    send_event_mesg_pro(instance, NULL, SecretKeyWrS, NULL, 1);
                }
                return (p-buf);
            }
            else if(method == 0x09) //ESAM参数设置  待验证，在方法4的基础上改的
            {
                p = p + 3;
                len1 = *p++;
                memcpy(buffer, p, len1);//oad+data
                p = p + len1;
                p++;
                mem_swap_cpy((uint8_t*)&addr, p, 4);//sid
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
                
                if(((addr & 0x00000700) == 0x00000700)) // sid
                {
                    if(0 != read_esam_info(instance, keybuf, 0x08))
                    {
                        mbase_cb->esam_auth_enable = keybuf[0];
                    }
                    else      
                        mbase_cb->esam_auth_enable = 0;
                }
                return (p-buf);
            }
            else if(method == 0x0B) //红外请求
            {
                 if((cilent_index == 0) && (p_ctx->channel != chl_ir_698))//主站只能红外
                {
                    return -DAR_OTHERS_ERROR;
                }
                if((cilent_index == 1) && (p_ctx->channel == chl_ir_698))//终端不能红外
                {
                    return -DAR_OTHERS_ERROR;
                }
                
                read_esam_no(instance, buffer);
                memcpy(keybuf, (uint8_t*)&buffer[5], 8);//表号, 8bytes

                addr = 0x02003680;
                txlen = 0;
                write_esam(instance, addr , buffer , txlen);
                memcpy((uint8_t*)&keybuf[16], buffer, 8);   //ESAM序列号, 8bytes
                
                p = p + 2;
                memcpy((uint8_t*)&keybuf[8], p, 8); //随机数
                p = p + 8;
                
                if(cilent_index == 0)
                {
                    memcpy(buffer, keybuf, 16); // 表号，随机数
                    addr = 0x03080880;
                    txlen = 0x0010;
                    rxlen = write_esam(instance, addr , buffer , txlen);  //80 08 08 03 00 10 + 表号，随机数，返回密文
                }
                else
                {
                    memcpy(buffer, (uint8_t*)&keybuf[8], 8); // 随机数
                    addr = 0x04000880;
                    txlen = 0x0008;
                    rxlen = write_esam(instance, addr , buffer , txlen);  //0x80, 0x08, 0x00, 0x04, 0x00, 0x08 + ，随机数，返回密文
                }
             
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
                if((cilent_index == 0) && (p_ctx->channel != chl_ir_698))//主站只能红外
                {
                    return -DAR_OTHERS_ERROR;
                }
                if((cilent_index == 1) && (p_ctx->channel == chl_ir_698))//终端不能红外
                {
                    return -DAR_OTHERS_ERROR;
                }

                //80 06 00 01 00 08 + 密文
                p = p + 2;
                memcpy(buffer, p, 8); 
                p = p + 8;
                addr = 0x01000680;
                txlen = 8;
                rxlen = write_esam(instance, addr , buffer , txlen);
                if(rxlen == 0xff)
                {
                    return -DAR_OTHERS_ERROR;
                }
                else
                {
                    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + IrAuthenTime);
                    nd_get_index(instance, addr, buffer, 4); 
                    
                    mem_swap_cpy((uint8_t*)&temp, buffer, 4);
                    mem_swap_cpy((uint8_t*)&mbase_cb->irauthen_tmr, buffer, 4);
                    mbase_cb->irauthen_tmr = mbase_cb->irauthen_tmr * 60;
                   
                    if(cilent_index )
                    {
                        mbase_cb->term_auth_timeout = temp * 60;
                    }
                    else
                    {
                        mbase_cb->infrared_auth_timeout = temp * 60;
                    }                
                    
                    if(cilent_index)//终端需断开所有应用链接
                    {
                        fr_handle_disconnect_all_channel(instance,  pipeline, cilent_index);
                    }
                    else
                    {
                        fr_handle_disconnect2(instance,  pipeline, cilent_index,  p_ctx->channel);
                    }
                    
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
                    return -DAR_OTHERS_ERROR;
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
                    return -DAR_OBJECT_NOT_EXIST;
                }
                if((*p > 10) || (*(p+1) != 2) || (*(p+2) != 8) || (*(p+3) != 1) || (*(p+4) != 0) || (*(p+6) != 0))
                {
                    dar = DAR_BUFFER_OVERFLOW;
                    return -DAR_BUFFER_OVERFLOW;
                }
                mbase_cb->baud_chg = 0;
                if(mbase_cb->Baut_rs485 != *p)
                {
                    mbase_cb->Baut_rs485 = *p;
                    mbase_cb->baud_chg = 0x01;
                }
                
                p = p + 7;
            }
            else
            {
                dar = DAR_OBJECT_NOT_EXIST;
                return -DAR_OBJECT_NOT_EXIST;
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
                    return -DAR_OBJECT_NOT_EXIST;
                }
                if((*p != 0) && (*p != 1))
                {
                    dar = DAR_BUFFER_OVERFLOW;
                    return -DAR_BUFFER_OVERFLOW;
                }
                mbase_cb->relay_ctl_mode = *p++;
            }
            else
            {
                dar = DAR_OBJECT_NOT_EXIST;
                return -DAR_OBJECT_NOT_EXIST;
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
                    return -DAR_OBJECT_NOT_EXIST;
                }
                if((*p != F_PortSec) && (*p != F_PortTouCut))
                {
                    dar = DAR_BUFFER_OVERFLOW;
                    return -DAR_BUFFER_OVERFLOW;
                }
                mbase_cb->MulPortMode = *p++;
            }
            else
            {
                dar = DAR_OBJECT_NOT_EXIST;
                return -DAR_OBJECT_NOT_EXIST;
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
                    return -DAR_OBJECT_NOT_EXIST;
                }
                if((*p > 10) || (*(p+1) != 2) || (*(p+2) != 8) || (*(p+3) != 1) || (*(p+4) != 0))
                {
                    dar = DAR_BUFFER_OVERFLOW;
                    return -DAR_BUFFER_OVERFLOW;
                }
                mbase_cb->Baut_plc = *p;
                p = p + 5;
            }
            else
            {
                dar = DAR_OBJECT_NOT_EXIST;
                return -DAR_OBJECT_NOT_EXIST;
            }
            return (p-buf);
        case 0xF300://自动循显
        case 0xF301://按键循显
            switch(method)
            {
            case 0x03://下翻
                    if(oad == 0xF300)   return -DAR_RW_REJECT;
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
                    mbase_cb->AutoInv = mbase_cb->lcd_keydis_keep_sec;
                    mbase_cb->BackLightTmr = 60;
                    return 1;
            case 0x04://上翻
                    if(oad == 0xF300)   return -DAR_RW_REJECT;
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
                    mbase_cb->AutoInv = mbase_cb->lcd_keydis_keep_sec;
                    mbase_cb->BackLightTmr = 60;
                    return 1;
                case 0x05://显示查看
                    p=p+2;
                    if(*p == TYPE_CSD)    
                    {
                        if(*(p+1) == 0)
                            len = 4;
                        else if(*(p+1) == 1)
                            len = 8;
                    }
                    else 
                    {
                        return -1;
                    }
                    p = p + 2;
                    if(len == 4)
                    {
                        mem_swap_cpy((uint8_t*)&displayoad1, p, 4);
                    }
                    else if(len == 8)
                    {
                        mem_swap_cpy((uint8_t*)&displayoad1, p, 4);
                        p = p + 5;
                        mem_swap_cpy((uint8_t*)&displayoad2, p, 4);
                    }
                    else
                    {
                        return -1;
                    }
                    p = p + 5;
                    disp_sequ = *p;
                    p = p + 2;
                    displayid = display_oad_to_id((uint8_t*)&displayoad1, (uint8_t*)&displayoad2, disp_sequ, len);
                    if(displayid == 0)  return -DAR_BUFFER_OVERFLOW;
                    mbase_cb->Page_ID = displayid;
                    mem_swap_cpy((uint8_t*)&mbase_cb->AutoInv, p, 2);//持续时间
                    p=p+2;  
                    mbase_cb->BackLightTmr = mbase_cb->AutoInv;
                    return (p-buf);
                    break;
                case 0x06://全屏显示
                    mbase_cb->AutoPtr = 0xFF;
                    mbase_cb->Page_ID = 0xFFFF;
                    p++;
                    mem_swap_cpy((uint8_t*)&mbase_cb->AutoInv, p, 2);
                    p = p + 2;
                    
                    if((mbase_cb->AutoInv > 30) || (mbase_cb->AutoInv == 0))
                        mbase_cb->BackLightTmr = 5;
                    else
                        mbase_cb->BackLightTmr = mbase_cb->AutoInv;
                    return (p-buf);
                default:
                    break;
            }
        default:
            dar = DAR_OBJECT_NOT_EXIST;
            break;
    }
    return -DAR_OBJECT_NOT_EXIST;
}
