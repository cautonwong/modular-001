#include "nidavellir.h"
#include "645id_table.h"
#include "gs_record.h"
#include "fr_axdr.h"
#include "fr_698.h"
#include "ctl_03_SafeAuthen.h"



void location_data_swap(uint8_t *pdata, uint8_t len)
{
    uint8_t cnt;

 
    for (cnt = 0; cnt < 2; cnt++)
    {
        swap_buffer(pdata + 4*cnt, 4);
    }

    swap_buffer(pdata + 4*cnt, 3);
}
void fee_data_swap(uint8_t *pdata, uint8_t len, uint8_t interval)
{
    uint8_t cnt;

    for (cnt = 0; cnt < ((len)/interval); cnt++)
    {
        swap_buffer(pdata + interval*cnt, interval);
    }
}

void step_fee_swap(uint8_t *pdata, uint8_t len)
{
    uint8_t cnt, index;

    for (cnt = 0; cnt < ((len -18)/4); cnt++)
    {
        swap_buffer(pdata + 4*cnt, 4);
    }

    for (index = 0; index < 6; index++) /*结算日*/
    {
        swap_buffer(pdata + 4*cnt + 3*index, 3);
    }
}
void dis_data_swap(uint8_t *pdata, uint8_t len)
{

    swap_buffer(pdata, 4);
}



void decrypt_data_swap(uint8_t *pdata, uint8_t len, uint32_t id)
{
    uint8_t id_tmp[4];

    id_tmp[0] = (uint8_t)(id>>24);
    id_tmp[1] = (uint8_t)(id>>16);
    id_tmp[2] = (uint8_t)(id>>8);
    id_tmp[3] = (uint8_t)(id);

    if (len > 4)
    {
        ndv_memcopy(pdata, pdata + 4, len-4);
        len -= 4;
    }


    if (0x04 == id_tmp[0])
    {
        switch(id_tmp[1])
        {
            case 0x00:/*电能表位置信息*/
                if ((0x04 == id_tmp[2]) && (0x0f == id_tmp[3]))
                {
                    location_data_swap(pdata, len);
                }
                else
                {
                    swap_buffer(pdata, len);
                }
                break;
            case 0x01:  /*时区表*/
            case 0x02:  /*时段表*/
            case 0x07:  /*阶梯时区表*/
                fee_data_swap(pdata, len, 3);
                break;
            case 0x05: /*费率电价*/
                fee_data_swap(pdata, len, 4);
                break;
            case 0x06:  /*阶梯表*/
                step_fee_swap(pdata, len);
                break;
            case 0x04:  /*显示项*/
                if ((0x01 == id_tmp[2]) || (0x02 == id_tmp[2]))
                {
                    dis_data_swap(pdata, len);
                }
                else
                {
                    swap_buffer(pdata, len);
                }
                break;
            default:
                swap_buffer(pdata, len);
                break;
        }
    }
    else
    {
        swap_buffer(pdata, len);
    }
}

// 密文解密
// <0 异常     0:未处理     >1 解密正常
int8_t security_decode_645(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{

    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret=0;
    uint8_t *apdu = p_rx;  
    uint8_t *p_tx_cache = p_tx;
    uint8_t *p_err = (p_tx);
    uint8_t buffer_1[160]={0}, buffer_2[160]={0};
    uint32_t addr = 0;  
    uint16_t txlen = 0;       
    uint8_t password[4] = {0};
    
    union dl645_DI_t DI;
	uint8_t tx_apdu_len = 0;   
    uint8_t k1_offset = 0, k1_len = 0, decode_len = 0;


	switch (ctx->ctrl)
	{
	case 0x1c:  //跳合闸、报警、保电
		addr = 0x8012C082;
		break;
	case 0x14: //二类参数更新
        addr = 0x8012C083;
		break;
	case 0x1A:  //电表清零
		addr = 0x8012C084;		
		break;
    case 0x19:  //需量清零
    case 0x1b:  //事件清零
		addr = 0x8012C085;
		break;
	default:
		return -RS_ERR_OTHERS;   
		break;
	}


    if (ctx->ctrl == 0x14)
    {
        if(ctx->apdu_rx_len < 16)
        {
            return -RS_ERR_OTHERS;
        }
        
        //memcpy(p_tx, apdu, 4);
        //p_tx += 4;
        
        memcpy((unsigned char*)&DI.di, apdu, 4);
        apdu += 4; 
        k1_offset += 4;

        memcpy((unsigned char*)&password[0], apdu, 4);
        apdu += 4; 
        k1_offset += 4;
        
        memcpy((unsigned char*)mbase_cb->handler_code, apdu, 4);
        apdu += 4; 
        k1_offset += 4;
        
        k1_len = (ctx->apdu_rx_len - k1_offset - 4);
        mem_swap_cpy(&buffer_2[0], (uint8_t*)apdu, k1_len);//密文
        apdu += k1_len;
        
        mem_swap_cpy(&buffer_2[k1_len], (uint8_t*)apdu, 4);//mac
        apdu += 4;

        txlen = k1_len + 4; 

    }
    else
    {
        if(ctx->apdu_rx_len < 8)
        {
            return -RS_ERR_OTHERS;
        }
        

        //memcpy(p_tx, apdu, 4);
        //p_tx += 4;
        
        memcpy((unsigned char*)&password[0], apdu, 4);
        apdu += 4; 
        k1_offset += 4;
        
        memcpy((unsigned char*)mbase_cb->handler_code, apdu, 4);
        apdu += 4; 
        k1_offset += 4;
        
        k1_len = (ctx->apdu_rx_len - k1_offset);
        mem_swap_cpy(&buffer_2[0], (uint8_t*)apdu, k1_len);//密文
        apdu += k1_len;
        
        txlen = k1_len; 


    }


    ndv_memcopy(buffer_1, buffer_2, txlen); 
    ret = write_esam(instance, addr , buffer_1 , txlen);  
    if (ret < 0)
    {
        return -SC_ERR_ESAM;
    }
    decode_len = (uint8_t)ret;    
    
    if(ctx->ctrl == 0x1b) //事件清零
    {
        mem_swap_cpy(&p_rx[8], buffer_1, decode_len);
        swap_buffer(&p_rx[12], 8);
        ctx->apdu_rx_len = 8 + decode_len;
    }
    else if(ctx->ctrl == 0x14) //二类参数更新
    {
        if((decode_len < 1) || ((buffer_1[0] + 1) != decode_len))
        {
            return -RS_ERR_OTHERS;
        }
        
        decode_len--;
        decrypt_data_swap(&buffer_1[1], decode_len, DI.di);
        ndv_memcopy(&p_rx[12], &buffer_1[1], decode_len);
        ctx->apdu_rx_len = 4 + 8 + (decode_len - 4); // -4 id
    }
    else
    {
        ndv_memcopy(&p_rx[8], buffer_1, decode_len);
        ctx->apdu_rx_len = 8 + decode_len;
    }
    *(p_rx - 1) = ctx->apdu_rx_len;
    
	return 1;
}



// 目前不支持 1类参数设置
// <0 异常     0:未处理     >1 解密正常
int8_t security_judge_645(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    int8_t err=0;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

	switch (ctx->password[0])
	{
	case DL645_PA_PLAIN_02:  
	case DL645_PA_PLAIN_04: 
        if((Cmp_Password(instance, ctx->password) == 0)) //开启明文设参 且密码正确
        {
            if((mbase_cb->metertype & F_Password_En) || ((ctx->ctrl == 0x1B) || (ctx->ctrl == 0x1C)))  //明文合闸 或开启明文设参
            {
                err = 1;
            }
        }
        else
        {
            err = (-RS_ERR_PASS);
        }
       
		break;
	case DL645_PA_SC_CIPHER:    //密文 二类参数
        if (mbase_cb->mac_suspend_state == true) //挂起
        {
            err =  (-RS_ERR_OTHERS);
        }
        else
        {
        	err =  security_decode_645(instance, p_rx, p_tx, tx_len, ctx);
        }

		break;
    case DL645_PA_SC_PLAINMAC:  // 一类参数     
	default:
		err =  (-RS_ERR_OTHERS);
		break;
	}

    if (mbase_cb->factor_mode == true) 
    {
        err = 1;
    }
    
	return err;  
}


/*
身份认证指令
input:
8密文1
8随机数1
8分散因子		

output:	
4随机数2
8ESAM序列号				
*/

// 返回0 正常应答          ＞0 其他异常应答          -1 不应答
int dl645_remote_authenticate(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    uint32_t Authen_Tmr = 0;

    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret=0;
    uint8_t *apdu = p_rx;  // 4 + 4 + 8密文 + 8 + 8
    uint8_t *p_tx_cache = p_tx;
    uint8_t buffer_1[64]={0}, buffer_2[64]={0};
    uint32_t addr = 0;  
    uint16_t txlen = 0;


    
    MeterWr645_obj_list_clear(instance, NULL, NULL);
    if(ctx->channel == chl_ir_698)
    {
        if (mbase_cb->infrared_auth_timeout == 0)
        {
            return SC_ERR_AUTHEN;
        }
    }


    mbase_cb->auth_timeout_645 = 0;
    mem_swap_cpy(&mbase_cb->rand8_645[0], &apdu[16], 8); //backup rand8


    ret = read_esam_no(instance, buffer_1);
    ndv_memcopy(&buffer_2[0], (uint8_t*)&buffer_1[5], 8);//表号, 8bytes
    ndv_memcopy(&buffer_2[8], (uint8_t*)&mbase_cb->rand8_645[0], 8);//rn

    
    ndv_memcopy(buffer_1, buffer_2, 16); // 表号，随机数
    addr = 0x80080803; //远程身份认证
    txlen = 0x0010;
    ret = write_esam(instance, addr , buffer_1 , txlen);  //80 08 08 03 00 10 + 表号，随机数，返回密文
    if (ret < 0)
    {
        return SC_ERR_AUTHEN;
    }

    
    mem_swap_cpy(&buffer_2[0], (uint8_t*)&apdu[8], 8);//k1 倒序
    if(0 != mem_compare(buffer_1, buffer_2, 8))
    {
        return SC_ERR_AUTHEN;
    }


    ret = read_esam_random(instance, 4,  buffer_1);  // rand4
    if (ret < 0)
    {
        return SC_ERR_AUTHEN;
    }
    mem_swap_cpy(p_tx, buffer_1, 4);// rand4
    p_tx += 4;

    ret = read_esam_serial_number(instance, buffer_1); //序列号 8
    if (ret < 0)
    {
        return SC_ERR_AUTHEN;
    }
    mem_swap_cpy(p_tx, buffer_1, 8);// rand4
    p_tx += 8;

    
#if 1   //获取645身份认证时效  
    ret = nd_get_index_e2p(instance, (uint8_t *)&buffer_1[0], mbase_block_parameter, AuthTime_DL645, 2); 
    Authen_Tmr = buf_to_uint16_lsb(&buffer_1[0]);
    if(ret < 0)  Authen_Tmr = 30;
#else   // 获取esam 会话时效 
    read_esam_info(instance, buffer_1, 5);
    Authen_Tmr = buf_to_uint32_msb(&buffer_1[0]);
#endif    
    Authen_Tmr = 60 * Authen_Tmr;

    mbase_cb->auth_timeout_645 = Authen_Tmr;

    *tx_len = (uint8_t)(p_tx - p_tx_cache);  
    
    return 0;
}

// 返回0 正常应答          ＞0 其他异常应答          -1 不应答
int dl645_esamcleartime(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret=0;
    uint8_t *apdu = p_rx; 
    uint8_t *p_tx_cache = p_tx;

    if(mbase_cb->auth_timeout_645 == 0)
    {
       return SC_ERR_OTHER;
    }

    mbase_cb->auth_timeout_645 = 0;

    *tx_len = (uint8_t)(p_tx - p_tx_cache);  
    
    return 0;
}

int dl645_Identity_auth_time_set(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{

    uint32_t time = 0;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret=0;
    uint8_t *apdu = p_rx; 
    uint8_t *p_tx_cache = p_tx;



    if(ctx->channel == chl_ir_698)
    {
        if (mbase_cb->infrared_auth_timeout == 0)
        {
            return SC_ERR_OTHER;
        }
    }

    if(mbase_cb->auth_timeout_645 == 0)
    {
       return SC_ERR_OTHER;
    }

    if (false == Isbcddataarray(apdu+8,2))
    {
        return SC_ERR_OTHER;
    }

    ndv_memcopy((uint8_t *)&time,apdu+8,2);
    time = word_bcd_to_hex((uint16_t)time);

    if(time==0x00)
    {
        return SC_ERR_OTHER;
    }
#if 0  
    memcopy((uint8_t*)&cmd[0],dlt645_esam_time_set,5);
    memswapcpy(cmd+5,comm_auth->linebuf+8,2);
    memswapcpy(cmd+7,comm_auth->linebuf+10,4);
    if (true != esam_ioctrl_interface(cmd, k2, &offset))
	{
		comm_auth->result = 0x0080;
		return false;
	}
#endif
              
    nd_set_index_e2p(instance, (uint8_t *) &time, mbase_block_parameter, AuthTime_DL645, 2); 
    mbase_cb->auth_timeout_645 = time * 60;

    trigger_MeterWr645_record(instance, 0x070001FF, 0);

    *tx_len = (uint8_t)(p_tx - p_tx_cache);   

    return 0;
}

int dl645_ir_atuh_request_step1(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{

    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret=0;
    uint8_t *apdu = p_rx;  
    uint8_t *p_tx_cache = p_tx;
    uint8_t buffer_1[64]={0}, buffer_2[64]={0};
    uint32_t addr = 0;  
    uint16_t txlen = 0, k1_len = 0;
    uint8_t meter_number[6];

    //仅限红外通道
    if(ctx->channel != chl_ir_698)
    {
        return SC_ERR_OTHER; 
    }   
    
    mbase_cb->infrared_auth_timeout = 0;

    ret = read_esam_no(instance, buffer_1);
    ndv_memcopy(&buffer_2[0], (uint8_t*)&buffer_1[5], 8);//表号, 8bytes
    ndv_memcopy(meter_number, (uint8_t*)&buffer_2[2], 6);//拷贝表号  6bytes
    
    mem_swap_cpy(&buffer_2[8], (uint8_t*)&apdu[8], 8);//rand1密文

    ndv_memcopy(buffer_1, buffer_2, 16); // 表号，rand1密文
    
    addr = 0x80080803; //表号分散内部认证返回Rand1 密文
    txlen = 0x0010;
    ret = write_esam(instance, addr , buffer_1 , txlen);  //80 08 08 03 00 10 + 表号，随机数，返回密文
    if (ret < 0)
    {
        return SC_ERR_OTHER;
    }

    k1_len = ret;


    // 6表号、8序列号、密文、8随机数

    mem_swap_cpy(p_tx, (uint8_t*)&meter_number[0], 6);// 
    p_tx += 6;

    ret = read_esam_serial_number(instance, buffer_2); //序列号 8
    if (ret < 0)
    {
        return SC_ERR_OTHER;
    }
    mem_swap_cpy(p_tx, buffer_2, 8);
    p_tx += 8;

    mem_swap_cpy(p_tx, buffer_1, k1_len);   //k1
    p_tx += k1_len;
    
    ret = read_esam_random(instance, 8,  buffer_2);  // rand8
    if (ret < 0)
    {
        return SC_ERR_OTHER;
    }
    mem_swap_cpy(p_tx, buffer_2, 8);// rand4
    p_tx += 8;

    mbase_cb->esam_power_close_dly = 60;

    *tx_len = (uint8_t)(p_tx - p_tx_cache); 
    return 0;
}

int dl645_ir_atuh_request_step2(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{

    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret=0;
    uint8_t *apdu = p_rx;  
    uint8_t *p_tx_cache = p_tx;
    uint8_t buffer_1[64]={0}, buffer_2[64]={0};
    uint32_t addr = 0, tempU32 = 0;  
    uint16_t txlen = 0, k1_len = 0;
    uint8_t meter_number[6];

    //仅限红外通道
    if(ctx->channel != chl_ir_698)
    {
        return SC_ERR_OTHER;  
    }  
    
    mbase_cb->infrared_auth_timeout = 0;

    mem_swap_cpy(&buffer_2[0], (uint8_t*)&apdu[8], 8);//k1

    ndv_memcopy(buffer_1, buffer_2, 8); // k1
    
    addr = 0x80060001; //ESAM 红外权限认证
    txlen = 0x0008;
    ret = write_esam(instance, addr , buffer_1 , txlen);  
    if (ret < 0)
    {
        return SC_ERR_OTHER;
    }


    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + IrAuthenTime); //红外认证剩余时效  698和645共用
    nd_get_index(instance, addr, buffer_1, 4); 
    
    mem_swap_cpy((uint8_t*)&tempU32, buffer_1, 4);

    mbase_cb->auth_timeout_645 = 0;
    mbase_cb->infrared_auth_timeout = tempU32 * 60;   // 红外认证剩余时间  698和645共用
     
    *tx_len = (uint8_t)(p_tx - p_tx_cache); 

    return 0;

}


//数据域长度：L =04H（数据标识）+04H（操作者代码）+m(数据长度)
// 返回0 正常应答          ＞0 其他异常应答          -1 不应答
int8_t safe_Authen03_645(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    union dl645_DI_t DI;
    int Err_Byte = 0;
    int ret;
    uint8_t *p_err = (p_tx);
	uint8_t tx_apdu_len = 0;

    if (ctx->SA == SA_TYPE_GENERAL645)  //通配地址
    {
        return response_x_byte_err_645(p_err, SC_ERR_OTHER, 2,  tx_len);
    }
    else if(ctx->SA == SA_TYPE_BROADECAST645)
    {
        return -1;
    }

    if(ctx->apdu_rx_len < 8)
    {
        return response_x_byte_err_645(p_err, SC_ERR_OTHER, 2,  tx_len);
    }

    if (mbase_cb->mac_suspend_state == true) //挂起
    {
        return response_x_byte_err_645(p_err, SC_ERR_HALT, 2,  tx_len);
    }

    memcpy(p_tx, p_rx, 4);
    p_tx += 4;
    *tx_len = 4;


	memcpy((unsigned char*)&DI.di, p_rx, 4);

    memcpy((unsigned char*)mbase_cb->handler_code, p_rx + 4, 4);


    switch(DI.di)
    {
        case 0x070000FF:  //身份认证指令
        {
            Err_Byte = dl645_remote_authenticate(instance, p_rx, p_tx, &tx_apdu_len, ctx);
        }
        break;
        case 0x070001FF:  //身份认证时效设置
        {
            Err_Byte = dl645_Identity_auth_time_set(instance, p_rx, p_tx, &tx_apdu_len, ctx);
        }
        break;  
        case 0x070002FF:  // 身份认证失效  待完善
        {
            Err_Byte = dl645_esamcleartime(instance, p_rx, p_tx, &tx_apdu_len, ctx);
        }
        break;  
        case 0x070003FF:  //红外认证请求-step2
        {
            Err_Byte = dl645_ir_atuh_request_step2(instance, p_rx, p_tx, &tx_apdu_len, ctx);
        }
        break;  

        case 0x078003FF:  //红外认证请求-step1
        {
            Err_Byte = dl645_ir_atuh_request_step1(instance, p_rx, p_tx, &tx_apdu_len, ctx);
        }
        break;  

        case 0x070101FF:  //开户
        case 0x070102FF:  //充值
        case 0x070103FF:  //钱包初始化命令
        case 0x078001FF:  //数据回抄
        case 0x078102FF:  //状态查询 
        default:
            if(mbase_cb->auth_timeout_645 == 0) // 后续每个增加case 需判断
            {
               Err_Byte = SC_ERR_OTHER;
            }
            Err_Byte = SC_ERR_OTHER;
            break;
    }

    if (Err_Byte == 0)
    {
        *tx_len = (*tx_len) + tx_apdu_len;
        return 0;
    }
    else if(Err_Byte > 0) //异常应答
    {
        return response_x_byte_err_645(p_err, (uint16_t)Err_Byte, 2,  tx_len);
    }
    else
    {
        return -1;
    }
    
    
}





