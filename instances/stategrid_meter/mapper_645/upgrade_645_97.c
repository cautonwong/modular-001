#include "upgrade_645_97.h"
#include "target.h"

//待调整     函数名替换

extern int PowerOff_Save_data(struct instance_t* instance,  struct pipeline_t* pipeline, struct message_t* mesg);


const struct mode_frame_len_t CODE mode_frame[] =
{
	{ mode_01,		64 },
	{ mode_02,		128 }, 
    { mode_03,      196 }, 

    { mode_81,      64 },
    { mode_82,      128 }, 
    { mode_83,      196 }, 
};

static const uint8_t CODE image_transfer_mode_num = sizeof(mode_frame) / sizeof(mode_frame[0]);

uint8_t upgrade_mode_judge(const uint8_t mode)
{
	int count;

	for (count = image_transfer_mode_num - 1; count >= 0; count--)
	{
		if (mode_frame[count].mode == mode)
		{
			return count;
		}
	}
	return 0xff;
}

static uint8_t get_UPmode_len(const uint8_t mode, uint8_t *len)
{
	uint8_t position;

	position = upgrade_mode_judge(mode);
	if (0xff == position)
	{
		return false;
	}
	else
	{
		*len = mode_frame[position].frame_len;
	}
	return true;
}

static uint8_t clear_image_data(struct instance_t* instance)
{
    uint8_t buf = 0xFF;
    //只需擦除第一个扇区， 其余的下载文件时自动清除
	nd_set_offset_flash(instance, IMAGE_DATA_ID_BLOCK, 0, &buf, 1, 1); //跨页擦除
	return true;
}

uint8_t set_image_state(struct instance_t* instance, const uint8_t state, struct image_transfer_state_t *image_transfer_state)
{
	if (state != image_transfer_state->transfer_enable)/*写升级参数*/
	{
		image_transfer_state->transfer_enable = state;
		
        if (0 > nd_set_index_e2p(instance, (uint8_t*)image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))
        {
            return false;
        }     
	}
	return true;
}



static uint8_t process_id_0xcb50(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
	uint8_t *pdata = p_rx + 0x04;
	struct image_transfer_state_t image_transfer_state;

	if (0x01 != pdata[0])
	{
		return false;
	}
	memset((uint8_t*)&image_transfer_state, 0x00, sizeof(struct image_transfer_state_t));
	image_transfer_state.transfer_enable = transfer_initiated;
    mbase_cb->UpGrade_CheckFlag = 0;
    if (0 > nd_set_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
	{
		return false;
	}
	p_tx[0] = transfer_initiated;
	*tx_len = 0x01;
	return true;
}

uint8_t get_imange_transfer_para(struct instance_t* instance, uint8_t *buff, const uint8_t len, const uint8_t offset)
{
	struct image_transfer_state_t image_transfer_state;

	memset((uint8_t *)&image_transfer_state, 0x00, sizeof(struct image_transfer_state_t));

    if (0 > nd_get_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
	{
		return false;
	}

    ndv_memcopy(buff, (uint8_t*)&image_transfer_state + offset, len);


	return true;
}

static uint8_t process_id_0xcb51(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
	uint8_t position;
	uint8_t transfer_state;
	uint8_t *pdata = p_rx + 0x04;

	struct image_transfer_state_t image_transfer_state;

	if (false == get_imange_transfer_para(instance, &transfer_state, 0x01, transfer_enable_offset))
	{
		return false;
	}

	if ( transfer_state < transfer_initiated )
	{
		return false;
	}

    if (0 > nd_get_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
	{
		return false;
	}
	position = upgrade_mode_judge(pdata[0]);
	if (0xff == position)
	{
		return false;
	}
	image_transfer_state.block_size = mode_frame[position].frame_len;
	image_transfer_state.transfer_mode = pdata[0];
	image_transfer_state.transfer_enable = transfer_set_mode;

    if (0 > nd_set_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))
	{
		return false;
	}
	p_tx[0] = image_transfer_state.transfer_mode;
	*tx_len = 0x01;
	return true;
}

static uint8_t process_id_0xcb53(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
	uint8_t *pdata = p_rx + 0x04;
    uint32_t second = 1;
	struct image_transfer_state_t image_transfer_state;

    mbase_cb->UpGrade_max_block_count = 0; 
    if (0 > nd_get_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
	{
		return false;
	}
	ndv_memcopy(&image_transfer_state.file_type, pdata, ctx->apdu_rx_len - 6);
	image_transfer_state.transfer_enable = image_identify;

    if (0 > nd_set_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))
	{
		return false;
	}

#if(UPGRADE_MODE == UPGRADE_MODE_ONLINE)//在线升级
    ndv_memcopy(image_transfer_state.datatime, (uint8_t *)&second, 0x04);
    uint32_t value = IMAGE_ENABLE_FLAG;
    ndv_memcopy(&image_transfer_state.image_enable,(uint8_t *)&value,0x04);
	if (false == set_image_state(instance, activation_initiated, &image_transfer_state)) // 跳到最后步
	{
		return false;
	}
	mbase_cb->UpGrade_CheckFlag = IMAGE_ENABLE_FLAG;
#endif
#if(UPGRADE_MODE == UPGRADE_MODE_LOCAL)//本地升级
    if (false == clear_image_data(instance))
    {
        return false;
    }
#endif

	*tx_len = 0x00;
	return true;
}


void reboot(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t value = IMAGE_ENABLE_FLAG;

    mbase_cb->frz_min_start_dlysec = 0; //单相
    
    PowerOff_Save_data(instance, NULL, NULL); //数据保存

    nd_set_index_e2p(instance, (uint8_t*)&value, IMAGE_PARA_BLOCK, IMAGE_PARA_ID2, 4);
    
    NVIC_SystemReset();  /* 直接复位CM3及外围电路 */
}

uint8_t meter_update_check(struct instance_t* instance)
{
//	uint32_t value;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
	uint32_t now_second = 0,check_second = 0;

	struct image_transfer_state_t image_transfer_state = {0};

    
	if(IMAGE_ENABLE_FLAG != mbase_cb->UpGrade_CheckFlag) 
	{
		return false;
	}

    if (0 > nd_get_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
	{
	    mbase_cb->UpGrade_CheckFlag = 0;
		return false;
	}
	
	if(activation_initiated != image_transfer_state.transfer_enable)
	{
	    mbase_cb->UpGrade_CheckFlag = 0;
		return false;
	}

	now_second = rtc_mktime(&instance->time);
	check_second = buf_to_uint32_lsb(image_transfer_state.datatime);
	if((check_second) && (now_second >= check_second))
	{
		memset(image_transfer_state.datatime, 0x00,0x04);
        
        if (0 > nd_set_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))
		{
			return false;
		}
		mbase_cb->UpGrade_CheckFlag = 0;
        //#endif
		reboot(instance);
	}
	return true;
}

static uint8_t image_crc_check(struct instance_t* instance, uint8_t *buff)
{
	uint8_t len, result;
	uint16_t crc16 = 0xFFFF;
	uint16_t image_crc;
	uint32_t data_len, data_len_count;

	struct image_transfer_state_t image_transfer_state;

    if (0 > nd_get_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
	{
		return check_other_reason;
	}

	//if (image_transfer_state.transfer_enable < transfer_bit_map)// transfer_image_data
	if (image_transfer_state.transfer_enable < transfer_image_data)// 
	{
		return check_other_reason;
	}
	data_len = buf_to_uint32_msb(image_transfer_state.block_num);
	if(data_len < 0x01)
	{
		return check_other_reason;
	}
	else
	{
		data_len = (data_len - 1)*image_transfer_state.block_size + image_transfer_state.last_blocksize;
	}
	data_len_count = 0x00;
	while (data_len_count < data_len)
	{
        mcu_clear_watchdog();
        
		if (data_len > data_len_count + 200)
		{
			len = 200;
		}
		else
		{
			len = (uint8_t)(data_len - data_len_count);
		}
		
		if (0 > nd_get_offset_flash(instance, IMAGE_DATA_ID_BLOCK, data_len_count + IMAGE_MAP_LEN, buff, len))
		{
			return check_other_reason;
		}
		crc16 = GetCRC16_mode2(buff, len, crc16);
		data_len_count += len;
	}
	image_crc = buf_to_uint16_lsb(image_transfer_state.code_crc);
	if (image_crc == crc16)
	{
		result = verification_successful;
	}
	else
	{
		result = verification_failed;
	}
	if (false == set_image_state(instance, result, &image_transfer_state))
	{
		return check_other_reason;
	}

    if (image_crc == crc16)
    {
        return check_successs;
    }
    else
    {
        return check_other_reason;
    }
}

static uint8_t process_id_0xcb55(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret = 0;

	uint8_t *pdata = p_rx + 4;

	p_tx[0] = image_crc_check(instance, pdata);
	*tx_len = 0x01;

	return true;
}

static uint8_t process_id_0xcb58(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret = 0;
    struct tm  upgrade_time;
	uint32_t second;
	uint8_t *pdata = p_rx + 4;
	struct image_transfer_state_t image_transfer_state;

#if(UPGRADE_MODE == UPGRADE_MODE_ONLINE)//在线升级 不支持 激活时间设置
        return false;
#endif

    if (0 > nd_get_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))
	{
		return false;
	}
	if (image_transfer_state.transfer_enable != verification_successful)
	{
		return false;
	}

    
    if(( Time_645Format_Check( pdata ) != 0 ) || ( Date_645Format_Check( pdata+3) != 0 ))
    {
        return false;
    }

    memset((uint8_t*)&upgrade_time, 0, sizeof(struct tm));

    upgrade_time.tm_sec = BCD_BYTE(pdata[0]);
    upgrade_time.tm_min = BCD_BYTE(pdata[1]);
    upgrade_time.tm_hour = BCD_BYTE(pdata[2]);
    upgrade_time.tm_mday = BCD_BYTE(pdata[4]);
    upgrade_time.tm_mon = BCD_BYTE(pdata[5]) - 1;
    upgrade_time.tm_year = BCD_BYTE(pdata[6]) + 2000 - 1900;

    second = rtc_mktime(&upgrade_time);
	ndv_memcopy(image_transfer_state.datatime, (uint8_t *)&second, 0x04);
    uint32_t value = IMAGE_ENABLE_FLAG;
    ndv_memcopy(&image_transfer_state.image_enable,(uint8_t *)&value,0x04);
	if (false == set_image_state(instance, activation_initiated, &image_transfer_state))
	{
		return false;
	}
	mbase_cb->UpGrade_CheckFlag = IMAGE_ENABLE_FLAG;
	
	*tx_len = 0x00;
	return true;
}

static uint8_t is_illagle_frame_len(struct image_transfer_state_t *image_transfer_state, 
	const uint8_t image_data_len,const uint32_t block_num, const uint32_t block_count)
{
	uint8_t len;

	if (false == get_UPmode_len(image_transfer_state->transfer_mode, &len))
	{
		return false;
	}

	if (image_data_len > len)/*帧长度合法性判断*/
	{
		return false;
	}
	if (((block_count + 1) != block_num) && (len != image_data_len))/*非最后一帧*/
	{
		return false;
	}
	if((block_count + 1) == block_num)/*最后一帧*/
	{
		image_transfer_state->last_blocksize = image_data_len;
	}
	return true;
}


static uint8_t process_id_0xcafd(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int ret = 0;

	uint8_t image_data_len = 0, bit_map = 0, erase_flag = 1;
	uint32_t current_block_num = 0, last_block_num = 0;
	uint32_t current_block_count = 0, bit_map_offset_addr = 0;
	uint32_t image_data_offset_addr = 0;
	uint8_t *pdata = p_rx + 0x04;
	struct image_transfer_state_t image_transfer_state; 
	uint32_t second = 1;

#if(UPGRADE_MODE == UPGRADE_MODE_ONLINE)//在线升级 不支持app文件传输
    return false;
#endif

	if (ctx->apdu_rx_len <= 14)
	{
		return false;
	}
    if (0 > nd_get_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))
	{
		return false;
	}
	if (image_transfer_state.transfer_enable < image_identify)
	{
		return false;
	}
	current_block_num = buf_to_uint32_msb(pdata);
	if (current_block_num > 5120)/*最大帧数 320*1024/64*/
	{
		return false;
	}
	current_block_count = buf_to_uint32_msb(pdata + 4);

	last_block_num = buf_to_uint32_msb(image_transfer_state.block_num);

	if (((0x00 != last_block_num) && (last_block_num != current_block_num))
		|| (current_block_count + 1 > current_block_num))/*帧合法性判断*/
	{
		return false;
	}
	image_data_len = ctx->apdu_rx_len - 14;

	if (false == is_illagle_frame_len(&image_transfer_state, image_data_len,
		current_block_num, current_block_count))
	{
		return false;
	}

	image_data_offset_addr = IMAGE_MAP_LEN + image_transfer_state.block_size * current_block_count;/*存储升级数据，升级数据区前1K为位图信息*/
	if(image_data_offset_addr + image_transfer_state.block_size >= 0x080000UL)/*升级区为512K,超过此地址不让写 512K - image_transfer_state.block_size*/
	{
		return false;
	}

    if (mbase_cb->UpGrade_max_block_count > current_block_count) //补发 不擦除
    {
        erase_flag = 0;
    }
    else
    {
        erase_flag = 1;
        mbase_cb->UpGrade_max_block_count = current_block_count;
    }


    if (0 > nd_set_offset_flash(instance, IMAGE_DATA_ID_BLOCK, image_data_offset_addr, pdata + 8, image_data_len, erase_flag)) //跨页擦除
	{
		return false;
	}

	bit_map_offset_addr = current_block_count / 8;/*位图处理*/

    if (0 > nd_get_offset_flash(instance, IMAGE_DATA_ID_BLOCK, bit_map_offset_addr, &bit_map, 0x01)) 
	{
		return false;
	}
	bit_map &= ~(0x80 >> (current_block_count % 8));

    nd_set_offset_flash(instance, IMAGE_DATA_ID_BLOCK, bit_map_offset_addr, &bit_map, 1, 0); //跨页不擦除
	

	if ((transfer_image_data != image_transfer_state.transfer_enable)
		||((current_block_count + 1) == current_block_num))/*第一次或者最后一帧 写升级参数*/
	{
		image_transfer_state.transfer_enable = transfer_image_data;
		ndv_memcopy((uint8_t *)image_transfer_state.block_num, pdata, 0x04);
        if (0 > nd_set_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
		{
			return false;
		}
	}
	ndv_memcopy(p_tx, pdata, 0x08);
	*tx_len = 0x08;

	mbase_cb->frz_min_start_dlysec = UPGRADE_FRZMIN_DLY_SEC;

    if(current_block_count == 0)//第一条 清分钟冻结  -单相 
    {
        modify_capture_clr_pro(instance, 0x50020200);
    }

    if(((current_block_count + 1) == current_block_num))    //最后一帧 触发校验
    {
        if (check_successs == image_crc_check(instance, pdata))
        {
            ndv_memcopy(image_transfer_state.datatime, (uint8_t *)&second, 0x04);
            uint32_t value = IMAGE_ENABLE_FLAG;
            ndv_memcopy(&image_transfer_state.image_enable,(uint8_t *)&value,0x04);
            if (false == set_image_state(instance, activation_initiated, &image_transfer_state)) // 跳到最后步
            {
                return false;
            }
            mbase_cb->UpGrade_CheckFlag = IMAGE_ENABLE_FLAG;
        }
        else
        {
            return false;
        }
    }

	
	return true;

}

static const struct pro_imange_transfer_id_t CODE imange_transfer_id_list[] =
{
	{ 0xcafd,		0x01,	0xff,		process_id_0xcafd },

	{ 0xcb50,		0x01,	0x07,		process_id_0xcb50 },

	{ 0xcb51,		0x01,	0x07,		process_id_0xcb51 },

//	{ 0xcb52,		0x00,	0x02,		process_id_0xcb52 },

	{ 0xcb53,		0x01,	0x2d,		process_id_0xcb53 },

//	{ 0xcb54,		0x00,	0x03,		process_id_0xcb54 },

	//{ 0xcb55,		0x01,	0x06,		process_id_0xcb55 },

//	{ 0xcb56,		0x00,	0x02,		process_id_0xcb56 },

//	{ 0xcb57,		0x00,	0x02,		process_id_0xcb57 },

	//{ 0xcb58,		0x01,	0x0d,		process_id_0xcb58 },
};

static const uint8_t imange_transfer_id_num = sizeof(imange_transfer_id_list) / sizeof(imange_transfer_id_list[0]);

const struct pro_imange_transfer_id_t * find_image_transfer_process(const uint16_t id)
{
	int count;

	for (count = imange_transfer_id_num - 1; count >= 0; count--)
	{
		if (id == imange_transfer_id_list[count].id)
		{
			return imange_transfer_id_list + count;
		}
	}
	return __NULL;
}


//read------------------------------------------------------------------------------------------------------------------------------------------------

static uint8_t process_id_0xcb52(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
	uint8_t transfer_state;
	uint8_t *pdata = p_tx;

	if (false == get_imange_transfer_para(instance, &transfer_state, 0x01, transfer_enable_offset))
	{
		return false;
	}

	if (transfer_state < transfer_set_mode)
	{
		return false;
	}

	if (false == get_imange_transfer_para(instance, &transfer_state, 0x01, transfer_mode_offset))
	{
		return false;
	}
	transfer_state = upgrade_mode_judge(transfer_state);
	if (0xff == transfer_state)
	{
		return false;
	}
	pdata[0] = 0x00;
	pdata[1] = mode_frame[transfer_state].frame_len;
	*tx_len = 0x02;

	return true;
}

static uint8_t process_id_0xcb54(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
	uint8_t count;
	uint8_t *p_rxdata = p_rx + 2;
	uint8_t *p_txdata = p_tx;
	uint16_t start_bit_map;
	uint16_t end_bit_map;
	struct image_transfer_state_t image_transfer_state;

    if (0 > nd_get_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
	{
		return false;
	}

	start_bit_map = buf_to_uint16_msb(p_rxdata);
	end_bit_map = buf_to_uint16_msb(p_rxdata + 2);
	if (end_bit_map == 0xffff)
	{
		end_bit_map = (uint16_t)(buf_to_uint32_msb(image_transfer_state.block_num) / 8);
	}
	if (start_bit_map > end_bit_map)
	{
		return false;
	}
	else
	{
		end_bit_map = end_bit_map - start_bit_map + 1;
		if (end_bit_map > 240)/* 256 - (4个字节前导符 + 12字节固定内容) */
		{
			return false;
		}
	}

    if (0 > nd_get_offset_flash(instance, IMAGE_DATA_ID_BLOCK, start_bit_map, p_txdata, end_bit_map))
    {
        return false;
    }

	for (count = 0; count < end_bit_map; count++)
	{
		*p_txdata++ ^= 0xff;
	}
	if (false == set_image_state(instance,transfer_bit_map, &image_transfer_state))
	{
		return false;
	}
	*tx_len = (uint8_t)end_bit_map;
	return true;
}



static uint8_t process_id_0xcb56(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
	uint8_t *pbuff = p_tx;

	struct image_transfer_state_t image_transfer_state;

    if (0 > nd_get_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
	{
		return false;
	}

	switch (image_transfer_state.transfer_enable)
	{
	case verification_initiated:
		pbuff[0] = image_verification_initiated;
		break;

	case verification_successful:
		pbuff[0] = image_verification_successful;
		break;

	default:
		pbuff[0] = image_verification_failed;
		break;
	}
	pbuff++;
	
	*tx_len = (pbuff-p_tx);
	return true;
}

static uint8_t process_id_0xcb57(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
	uint8_t *pbuff = p_tx;
	uint32_t block_size;
	uint32_t image_size;
	
	struct image_transfer_state_t image_transfer_state;

    if (0 > nd_get_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
	{
		return false;
	}

	if(image_transfer_state.transfer_enable <= transfer_bit_map)
	{
		return false;
	}
	block_size = buf_to_uint32_msb(image_transfer_state.block_num);
	if(block_size < 1)
	{
		return false;
	}
	image_size =  (block_size - 1) * image_transfer_state.block_size 
		+ image_transfer_state.last_blocksize;

	ndv_memcopy(pbuff, (uint8_t *)&image_size, 0x04);
	swap_buffer(pbuff,0x04);
	pbuff += 4;
	ndv_memcopy(pbuff + 4, image_transfer_state.version, 36);
	pbuff += 36;
	
	*tx_len = (pbuff-p_tx);
	return true;
}

const struct image_read_id_t CODE image_read_id_cb52_cfg =
{
    0xcb52,
    __NULL,
    process_id_0xcb52,
};

const struct image_read_id_t CODE image_read_id_cb54_cfg =
{
    0xcb54,
    __NULL,
    process_id_0xcb54,
};

const struct image_read_id_t CODE image_read_id_cb56_cfg =
{
    0xcb56,
    __NULL,
    process_id_0xcb56,
};

const struct image_read_id_t CODE image_read_id_cb57_cfg =
{
    0xcb57,
    __NULL,
    process_id_0xcb57,
};


static const struct image_read_id_t  *image_read_id_proc[] =
{
	&image_read_id_cb52_cfg,
	&image_read_id_cb54_cfg,
    &image_read_id_cb56_cfg,
	&image_read_id_cb57_cfg,
};

static const uint8_t  image_read_id_proc_num = sizeof(image_read_id_proc) / sizeof(image_read_id_proc[0]);

const struct image_read_id_t  *find_image_read_id_processor(uint16_t id)
{
    int i;

    for (i = image_read_id_proc_num - 1; i >= 0; i--)
    {
        if (image_read_id_proc[i]->dl645_id == id)
        {
            return image_read_id_proc[i];
        }
    }

    return __NULL;
}




//
//升级状态失效   _单相
uint8_t disable_upgrade(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct image_transfer_state_t image_transfer_state;
    memset((uint8_t*)&image_transfer_state, 0x00, sizeof(struct image_transfer_state_t));
    image_transfer_state.transfer_enable = transfer_not_initiated;
    mbase_cb->UpGrade_CheckFlag = 0;
    if (0 > nd_set_index_e2p(instance, (uint8_t*)&image_transfer_state, IMAGE_PARA_BLOCK, IMAGE_PARA_ID, sizeof(struct image_transfer_state_t)))	
	{
		return false;
	}    
    return true;
}

//秒处理
uint8_t upgrade_second_pro(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb->frz_min_start_dlysec > 0)
    {
        if(mbase_cb->frz_min_start_dlysec > UPGRADE_FRZMIN_DLY_SEC)
        {
            mbase_cb->frz_min_start_dlysec = 1;
        }
        mbase_cb->frz_min_start_dlysec--;
        if (mbase_cb->frz_min_start_dlysec == 0)
        {
            disable_upgrade(instance);//失效升级
        }
    }
    
    meter_update_check(instance); //升级
}
