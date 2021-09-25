/**
* @defgroup    energy_plugin Plugin Energy
* @ingroup   plugins
* @see           energy
* @brief       hfp 计量插件
 *
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "energy.h"
#include "fs.h"
#include "lib_calclib.h"

#define EC_SAVE_FLAG    0xcafebabe

static uint8 last_power_sta[4] = {0x00, 0x00 , 0x00 , 0x00};

uint8 get_index_combination_mode(uint8 index)
{
    if(index < 7)
    {
        return combination_mode[index];
    }
    return 0;
}

void check_meter_parament(void)
{
    uint8 i;
    struct access_pattern_t  access_pattern;
    
    access_pattern.offset = 0;
    access_pattern.unit = (enum hfp_unit_type_t)0;
    access_pattern.decimals = 0;
    access_pattern.format = (enum data_format_t)0;
    access_pattern.ops_mode = 0;
    access_pattern.len = 0;
   
    for (i = 0; i < energy_parameter_num; i++)
    {
        if (*(energy_parameter[i].mac) != energy_parameter[i].mac_calculate((uint8 *)energy_parameter[i].parameter_data, energy_parameter[i].len))
        {
            access_pattern.offset = energy_parameter[i].offset;
            if (true != hfp_read(energy_parameter[i].parameter_fsid, (uint8 *)energy_parameter[i].parameter_data, energy_parameter[i].len, &access_pattern))
            {
                hfp_code_to_ram_copy((uint8 *)energy_parameter[i].parameter_data, (const uint8 CODE *)energy_parameter[i].parameter_default, energy_parameter[i].len);
            }
            *(energy_parameter[i].mac) = energy_parameter[i].mac_calculate((uint8 *)energy_parameter[i].parameter_data, energy_parameter[i].len);
        }
    }
}

uint8 energy_eeprom_correct(void *para, uint8  *energy_data, uint8 energy_len)
{
    uint8  *p, i, num, flag[2];
    uint16 fs_id, checksum, rchecksum;
    uint32 addr;
    uint32 buffer[2][9] = {0};
    struct file_stat_t file;
    const struct hfp_device_t   CODE *device;
    uint8	(*read)	(uint32 addr, uint8 *buf, uint16 len, const struct hfp_device_t CODE *device);
    uint8	(*write)(uint32 addr, uint8 *buf, uint16 len, const struct hfp_device_t CODE *device);
    struct ec_correct_t
    {
        uint8  type;
        uint16 fs_id;
        uint16 fs_id2;
    };

    struct ec_correct_t *ec_correct = (struct ec_correct_t*)para;

    fs_id = ec_correct->fs_id;
    
    if (ec_correct->type)
    {
        fs_id = ec_correct->fs_id2;
    }

    num = 2;
    
    do
    {
        if(false == hfp_stat(fs_id, &file))
    	{
    		return false;
    	}
        
        addr    = file.info.addr;
        device  = file.info.device;
        read    = file.info.device->driver->read;
        write   = file.info.device->driver->write;

        hfp_buffer_clear(flag, 0x02);
        
        for (i = 0; i < 2; i++)
        {
            p = (uint8 *)&buffer[i][0];
            if(true == read(addr, p, energy_len + 2 ,device))
    		{
    			checksum = 0xA5A5;
                checksum += hfp_do_sum(p, (uint16)energy_len);
                p += energy_len;
                hfp_memcopy((uint8 *)&rchecksum, p, 0x02);
                hfp_swap_buffer((uint8 *)&rchecksum, 0x02);
            	if (checksum == rchecksum)
            	{
            		flag[i] = 0xA5;
            	}
    		}
            addr = file.info.addr_bak;
        }

        if ((!flag[0]) && (flag[1]))
        {
            addr    = file.info.addr;
            p       = (uint8 *)&buffer[1][0];
            write(addr, p, energy_len + 2 ,device); /*仅恢复主区或备份区中错误数据*/
        }
        else if ((flag[0]) && (!flag[1]))
        {
            addr    = file.info.addr_bak;
            p       = (uint8 *)&buffer[0][0];
            write(addr, p, energy_len + 2 ,device);/*仅恢复主区或备份区中错误数据*/
        }
        else
        {
            if ((!flag[0]) && (!flag[1]))
            {
                if (ec_correct->type)
                {
                    fs_id = ec_correct->fs_id;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                p = (uint8 *)&buffer[0][0];/*EEPROM中两份电量均校验正确，则默认使用主区恢复RAM电量数据*/
                
                if (ec_correct->type && (fs_id == ec_correct->fs_id2))
                {
                    hfp_write(ec_correct->fs_id, p, energy_len, __NULL);
                }
                
                break;
            }
        }
    }while(num--);

    /*EEPROM中电量校验正确，则恢复至RAM*/
    hfp_memcopy((uint8 *)energy_data, p, energy_len);

    if (ec_correct->type)/*EC3余数部分恢复成功后将掉电余数保存内容校验写错，防止短接VCPU后错误恢复*/
    {
        pulse_allsave_state = 0;
        
        hfp_buffer_clear(p, energy_len + 2);
        
        hfp_write_shortcut(ec_correct->fs_id2, p, energy_len + 2, 0x00);
    }

    return true;
}

uint8 energy_crc_check_correct(struct energy_object_t *energy, uint8 check_mode , uint8 energy_mode , uint8 correct_mode)
{
    uint16 crc_temp;
    uint8  energy_len;
#if 0
    uint32 buffer[9];
#endif
    uint8  *energy_data;
    uint16 *crc;
    struct ec_correct_t
    {
        uint8  type;
        uint16 fs_id;
        uint16 fs_id2;
    };

    struct ec_correct_t ec_correct;
    
	if (__NULL == energy)
	{
		return false;
	}
    if (energy_cfg.max_tariff > 9)
    {
        return false;
    }

    hfp_buffer_clear((uint8 *)&ec_correct, sizeof(struct ec_correct_t));
    
    if (energy_mode == 0)
    {
        energy_data         = (uint8 *)energy->root->ec_kwh;
        energy_len          = energy_cfg.max_tariff << 2;
        crc                 = (uint16 *)&(energy->var->ec_kwh_crc);
        ec_correct.fs_id    = energy->energy_fsid->energy_class_id;
    }
    else
    {
        energy_data         = (uint8 *)energy->root->ec_pulse;
        energy_len          = energy_cfg.max_tariff << 1;
        crc                 = (uint16 *)&(energy->var->ec_pulse_crc);
        ec_correct.fs_id    = energy->energy_fsid->pulse_class_id;
        if (!correct_mode)
        {
            ec_correct.type     = 1;
            ec_correct.fs_id2   = energy->energy_fsid->pulse_class_id2;
        }
    }
    crc_temp = energy_cfg.energy_callback->energy_mac_calculate(energy_data, energy_len);
    if (check_mode == CRC_UPDATA)
    {
        *crc = crc_temp;
    }
    else
    {
        if (*crc != crc_temp)
        {
            if (energy_eeprom_correct(&ec_correct, energy_data, energy_len))
            {
         #if 0
                hfp_memcopy((uint8 *)energy_data, (uint8 *)buffer, energy_len);
         #endif
                *crc = energy_cfg.energy_callback->energy_mac_calculate(energy_data, energy_len);
            }
            
            return false;
        }
    }
    
    return true;
}


extern uint8 account_deduction(uint8 type);

struct dec_data_t
{
    uint32 price_send_cnts;
    uint32 price_dec_cnts;
    uint32 stair_send_cnts;
    uint32 stair_dec_cnts;
};
extern struct dec_data_t dec_data;

uint8 energy_increment_save(struct energy_object_t *energy, struct energy_ops_attr_t *energy_ops_attr , uint8 pulse , uint8 hook)
{
    uint16 ec_add;
    uint8 i;
    uint8 tariff = energy_ops_attr->tariff;
    
    uint8 *p            = (uint8 *)energy_parameter[1].parameter_data;
    const uint8 *ptr    = (const uint8 *)energy_parameter[1].parameter_default;
    uint8 ac_combi_add;

    ac_combi_add = (*p) & ptr[energy_ops_attr->category + 1];      /*有功组合特征字 有加的情况*/ 
    
    if(pluse_inc.pluse_const == 0)
    {
        return false;
    }

    /*特征字减 不参与用电量运算*/
    if((ac_combi_add == 0)                                       
     &&(energy_ops_attr->category < 2)&&(energy_ops_attr->phase == 3)&&((hook == 1)||(hook == 2)))
    {
        return false;
    }
    
    if((energy_ops_attr->category < 2)&&(energy_ops_attr->phase == 3))
    {
        if (tariff == 0)
        {
            if (hook == 2)
            {
                for(i=0;i<pulse;i++)
                {
                    energy->root->ec_pulse[tariff]++;
                    if(0 == (energy->root->ec_pulse[tariff] % pluse_inc.pluse_const))
                    {
                        energy_cfg.plugin_energy->energy_crc_check_correct(energy, CRC_UPDATA , 0 , 1 );
                        energy_cfg.plugin_energy->energy_crc_check_correct(energy, CRC_UPDATA , 1 , 1 );
                        account_deduction(0x5A);
                        dec_data.stair_send_cnts++;
                    }
                }
            }
            else
            {
                energy->root->ec_pulse[tariff] += pulse;
            }
        }
        else
        {
            if (hook == 0) 
            {
                for(i=0;i<pulse;i++)
                {
                    energy->root->ec_pulse[tariff]++;
                    if(0 == (energy->root->ec_pulse[tariff] % pluse_inc.pluse_const))
                    {
                        account_deduction(0x69);
                        dec_data.price_send_cnts++;
                    }
                }
            }
            else
            {
                energy->root->ec_pulse[tariff] += pulse;
            }
        }
    }
    else
    {
        energy->root->ec_pulse[tariff] += pulse;
    }
       
    ec_add = energy->root->ec_pulse[energy_ops_attr->tariff] / pluse_inc.pluse_const;
    if (ec_add >= pluse_inc.inc_diff)
    {
        energy->root->ec_kwh[tariff]   += pluse_inc.inc_diff;
        energy->root->ec_pulse[tariff] -= pluse_inc.pluse_const * pluse_inc.inc_diff;
        return true;
    }
    return false;
}

#ifdef WIN32
void energy_pulse_count(struct energy_object_t *energy, struct energy_ops_attr_t *energy_ops_attr)
{
    uint32 *power_acc;
    uint32 value;
	uint32 imp_ws;

    imp_ws = 360000L / pluse_inc.pluse_const;

    if (energy)
    {
        if (energy->root->pluse_acc)
        {
            power_acc     = energy->root->pluse_acc;
            value         = hfp_get_combine_acc_energy(energy_ops_attr->phase, energy_ops_attr->category);
            (*power_acc) += value;
            if ((*power_acc) >= imp_ws)
            {
                value = (*power_acc) / imp_ws;
                energy->var->pluse_count += value;
                value         = value * imp_ws;
                (*power_acc) -= value;
            }
        }
    }
}
#else
extern const struct driver_t CODE emu_driver;
extern const struct hfp_device_t CODE device_emu;

const uint8 CODE pls_list_a[]=
{
    REAlID_PLS_P_A,
    REAlID_PLS_Q_A,
};
const uint8 CODE pls_list_b[]=
{
    REAlID_PLS_P_B,
    REAlID_PLS_Q_B,
};
const uint8 CODE pls_list_c[]=
{
    REAlID_PLS_P_C,
    REAlID_PLS_Q_C,
};
const uint8 CODE pls_list_t[]=
{
    REAlID_PLS_P_T,
    REAlID_PLS_Q_T,
};

const uint8 CODE *pls_list[]=
{
    pls_list_a,
    pls_list_b,
    pls_list_c,
    pls_list_t,
};
void energy_pulse_count(struct energy_object_t *energy, struct energy_ops_attr_t *energy_ops_attr)
{
    uint32 value;
    const uint8 CODE *pls_real_id = NULL;
    
    uint8 mask;
    
    if (energy)
    {
        if (energy->var)
        {
            mask = 0x01<<energy_ops_attr->category;
            if(hfp_get_combine_acc_energy(energy_ops_attr->phase, energy_ops_attr->category))
            {
                  pls_real_id = pls_list[energy_ops_attr->phase];
                  if (true == emu_driver.read(pls_real_id[energy_ops_attr->category > 1 ? 1:0], (uint8 *)&value, 4, &device_emu))
                  {
                      energy->var->pluse_count += value;
                  }
                  last_power_sta[energy_ops_attr->phase] |= mask;
            }
            else
            {
                  if(last_power_sta[energy_ops_attr->phase] & mask)
                  {
                      pls_real_id = pls_list[energy_ops_attr->phase];
                      if (true == emu_driver.read(pls_real_id[energy_ops_attr->category > 1 ? 1:0], (uint8 *)&value, 4, &device_emu))
                      {
                          energy->var->pluse_count += value;
                      }
                  }
                  last_power_sta[energy_ops_attr->phase] &= ~mask;
            }
        }
    }
}
#endif

void energy_time_save_1min(struct energy_object_t *energy)
{
   
   
}


void clear_energy(struct energy_object_t *energy , struct energy_ops_attr_t *energy_ops_attr)
{
    hfp_buffer_clear((uint8 *)energy->root->ec_kwh, energy_cfg.max_tariff << 2);
    hfp_write(energy->energy_fsid->energy_class_id, (uint8 *)&energy->root->ec_kwh[0], energy_cfg.max_tariff << 2, __NULL);
    hfp_buffer_clear((uint8 *)energy->root->ec_pulse, energy_cfg.max_tariff << 1);
    hfp_write(energy->energy_fsid->pulse_class_id, (uint8 *)&energy->root->ec_pulse[0], energy_cfg.max_tariff << 1, __NULL);
    hfp_write(energy->energy_fsid->pulse_class_id2, (uint8 *)&energy->root->ec_pulse[0], energy_cfg.max_tariff << 1, __NULL);
}





uint8 get_tariff_index(struct energy_object_t *energy, struct energy_ops_attr_t *energy_ops_attr)
{
    uint8 tariff_index;

    if (true != hfp_read(0x6F61, (uint8 *)&tariff_index, 1, __NULL))
    {
        tariff_index = 1;
    }
    return tariff_index;
}


uint8 cut_pulse(struct energy_object_t *energy, struct energy_ops_attr_t *energy_ops_attr)
{
    uint16 pluse = energy->var->pluse_count;
    
    energy->var->pluse_count -= pluse;
    
    return pluse;
}

uint32 get_memroy_energy(struct energy_object_t *energy , uint8 type , uint8 tariff)
{
    if(energy)
    {
        energy_crc_check_correct(energy,  CRC_CHECK , type, 1 );
        if(type)
        {
            return energy->root->ec_pulse[tariff];
        }
        return energy->root->ec_kwh[tariff];
    }
    return 0;
}

int32 combination_energy(struct energy_ops_attr_t *energy_ops_attr, uint8 hook_energy)
{
    uint8 i,j,k,bit_ctrl;
    int32 ec;
    int32 com_value = 0;
    uint8 com_bin_ctrl = 0;   
    struct energy_object_t *energy;

    
    if((energy_ops_attr->category > 0x0E) || (energy_ops_attr->category < 0x08))
    {
        return 0;
    }
    
    if (energy_ops_attr->category < 0x0B)
    {
        j = 2;
        k = 0;
    }
    else
    {
        j = 6;
        k = 2;
    }
    com_bin_ctrl = combination_mode[energy_ops_attr->category - 8];
    bit_ctrl = 0;
    for (i = k; i < j; i++)
    {
        energy = energy_cfg.energy_callback->get_energy_root(energy_ops_attr->phase, i , hook_energy);
        if(energy)
        {
            ec = get_memroy_energy(energy , energy_ops_attr->type , energy_ops_attr->tariff);
        }
        
        if (com_bin_ctrl & (0x01 << (bit_ctrl << 1)))       /**< 加组合判断 */
        {
            com_value += ec;                       /**< 瓦时电能加 */
        }
        if (com_bin_ctrl & (0x01 << ((bit_ctrl << 1) + 1))) /**< 减组合判断 */
        {
            com_value -= ec;                       /**< 瓦时电能减 */
        }
        bit_ctrl++;
    }
    return com_value;
}



int32 get_energy_decimal(int32 pulse, int8 decimals)
{
    if (pluse_inc.pluse_const)
    {
        if (decimals == 0)
        {
            return pulse / (pluse_inc.pluse_const * 100);
        }
        if (decimals == -1)
        {
            return pulse / (pluse_inc.pluse_const * 10);
        }
        if (decimals == -2)
        {
            return pulse / pluse_inc.pluse_const;
        }
        if (decimals == -3)
        {
            return (pulse * 10) / pluse_inc.pluse_const;
        }
        if (decimals == -4)
        {
            return (pulse * 100) / pluse_inc.pluse_const;
        }
    }
    return 0;
}

uint64 get_energy_other(uint64 ec, int8 decimals)
{
    if (decimals == 0)
    {
        return ec >> 12;
    }
    if (decimals == -1)
    {
        return ec >> 8;
    }
    if (decimals == -2)
    {
        return ec >> 4;
    }
    if (decimals == -3)
    {
        return ec;
    }
    return 0;
}

struct root_energy_t
{
    uint8 phase;
    uint8 hook;
};
const struct root_energy_t CODE root_energy_list[]=
{
  {0 , 0},
  {1 , 0}, 
  {2 , 0},
  {3 , 0},
  
  {3 , 1},
  {3 , 2},

};

uint8 read_energy(uint32 id, uint8 *dest, uint16 len, const struct hfp_device_t CODE *device)
{
    struct access_pattern_t *read;
    int64 ec;
    int32 pulse;    
    struct energy_object_t *energy;
    struct energy_ops_attr_t energy_ops_attr;
    uint32 pos_neg = 0;
    uint8 hook_energy;
    uint8 root;

    
    root    = (id & 0x0F00) >> 8;
    if(root < 6)
    {
        energy_ops_attr.phase = root_energy_list[root].phase;
        hook_energy = root_energy_list[root].hook;
    }
    else
    {
        return false;
    }

    energy_ops_attr.category = (id & 0x00F0) >> 4;
    energy_ops_attr.tariff   = id & 0x000F;
    if ((energy_ops_attr.category > 14)||((energy_ops_attr.tariff-1) > energy_cfg.max_tariff)||(energy_ops_attr.phase>3))
    {
        return false;
    }
    
    read = (struct access_pattern_t *)device;
    
    READ_TR:
    
    if (energy_ops_attr.category < 8)
    {
        energy = energy_cfg.energy_callback->get_energy_root(energy_ops_attr.phase, energy_ops_attr.category , hook_energy);
        ec = get_memroy_energy(energy , 0 , energy_ops_attr.tariff);
        pulse = get_memroy_energy(energy , 1 , energy_ops_attr.tariff);
    }
    else
    {
        energy_ops_attr.type = 0;
        ec = combination_energy(&energy_ops_attr , hook_energy);
        energy_ops_attr.type = 1;
        pulse = combination_energy(&energy_ops_attr , hook_energy);
    }

    if (read->format == FORM_BCD)
    {
        uint8 i;
        uint64 outdata = 0;
        
        ec  = hfp_dec_convert_typeint64(ec, -2, -3);
        
        pulse = get_energy_decimal(pulse, -3);
        
        ec += pulse;
        
        if (energy_ops_attr.category >= 8)
        {
            if(ec < 0)
            {
                ec = ~ec + 1;
                pos_neg = 1;
            }
        }

        if (energy_ops_attr.category >= 8)
        {  
            while(ec > 799999999)
            {
                ec -= 800000000;
            }
        }

        for (i = 0; i < 11; i++)
        {
            outdata += (ec % 10) << (i * 4);
            ec /= 10;
        }

        outdata = get_energy_other(outdata, read->decimals);

        if (-3 == read->decimals)
        {
            read->ops_mode = 0xFF;
            
            if(pos_neg == 1)
            {
                read->ops_mode = 0xA5;
            }
            
        }
        else
        {
            if(pos_neg == 1)
            {
                outdata |= 0x80000000;
            }
        }

        ec = outdata;
    }
    else
    {
        ec  = hfp_dec_convert_typeint64(ec, -2, read->decimals);
        
        pulse = get_energy_decimal(pulse, read->decimals);
        
        ec += pulse;
    }

    if (8 == read->len)
    {
        hfp_memcopy(dest, (uint8 *)&ec, 8);
    }
    else 
    {
        hfp_memcopy(dest, (uint8 *)&ec, 4);
        if (len >= 4)
        {
            len -= 4;
            if ((len) && (energy_ops_attr.tariff < energy_cfg.max_tariff))
            {
                dest += 4;
                energy_ops_attr.tariff++;
                goto READ_TR;
            }
        }
    }


    return true;
}

void energy_clear_oper(void)
{
    uint8 i, j , k;
    struct energy_object_t *energy;
    struct energy_ops_attr_t energy_ops_attr;

    for(k = 0; k < energy_cfg.hook_num; k++)
    {
        for (i = 0; i < energy_cfg.max_phase; i++)
        {
            for (j = 0; j < energy_cfg.max_category; j++)
            {                         
                energy = energy_cfg.energy_callback->get_energy_root(i, j , k);
                if (energy)
                {
                    energy_ops_attr.category = j;
                    energy_ops_attr.phase = i;
                    clear_energy(energy , &energy_ops_attr);
                    energy_crc_check_correct(energy, CRC_UPDATA , 0, 1 );
                    energy_crc_check_correct(energy, CRC_UPDATA , 1, 1 );
                }
            }
        }
    }
    pulse_allsave_state =   0x3B69;
}
uint8 energy_io_ctrl(uint8 cmd, const struct hfp_device_t CODE *device , void *ctrl_parameter)
{
    switch(cmd)
    {
        case CTRL_DATA_CLEAR:
                energy_clear_oper();
            return true;
            break;
        default:
            break;
    }
    
    return false;
}

uint8 write_energy(uint32 id, uint8 *buf, uint16 len, const struct hfp_device_t CODE *device)
{
    int32 ec;

    struct energy_object_t *energy;
    struct energy_ops_attr_t energy_ops_attr;
    uint8 hook_energy;
    uint8 root;

    
    root    = (id & 0x0F00) >> 8;
    if(root < 6)
    {
        energy_ops_attr.phase = root_energy_list[root].phase;
        hook_energy = root_energy_list[root].hook;
    }
    else
    {
        return false;
    }

   
    energy_ops_attr.category = (id & 0x00F0) >> 4;
    energy_ops_attr.tariff   = id & 0x000F;
    if ((energy_ops_attr.category > 14)||((energy_ops_attr.tariff-1) > energy_cfg.max_tariff) || (energy_ops_attr.phase>3))
    {
        return false;
    }

    
    hfp_memcopy(&ec, (uint8 *)buf, 4);
    
    if (energy_ops_attr.category < 8)
    {
        energy = energy_cfg.energy_callback->get_energy_root(energy_ops_attr.phase, energy_ops_attr.category , hook_energy);
        
        energy->root->ec_pulse[energy_ops_attr.tariff] = 0;
        
        energy->root->ec_kwh[energy_ops_attr.tariff] = ec;
        /*如果写0，则认为是月度电能或者年度电能复位*/
        if(ec == 0)
        {
            clear_energy(energy , &energy_ops_attr);
        }
        
        energy_crc_check_correct(energy,  CRC_UPDATA , 0 , 1 );
        
        energy_crc_check_correct(energy,  CRC_UPDATA , 1 , 1 );
    }
    
    return true;

}
void energy_loop(void)
{
    uint16 note_id = 0;
    
    while(0x00 != (note_id =  hfp_notify(name_energy)))
	{
        switch(note_id)
        {
            case 0x2165:
            case 0x2166:
            case 0x2167:   
                combination_crc[0] = 0;
                combination_crc[3] = 0;
                combination_crc[4] = 0;
                check_meter_parament();
                break;
            case 0x2006:       
                pluse_inc_crc = 0;
                check_meter_parament();
                break;
            case 0x6F98:
                energy_clear_oper();
                break;
            default:
                break;
         }
    }
}

uint8  power_on(void)
{
    last_power_sta[0] = 0;
    last_power_sta[1] = 0;
    last_power_sta[2] = 0;
    last_power_sta[3] = 0;
    
    return true;
}

static uint16 pulse_powoff_save(uint8 phase , uint8 *buf)
{
    uint32 j ,k , len = 0;
    uint16 index = 0;
    uint16 checksum;

    struct energy_object_t *energy;
    
    for (k = 0; k < energy_cfg.hook_num; k++)
    {
        for (j = 0; j < energy_cfg.max_category; j++)
        {
           energy = energy_cfg.energy_callback->get_energy_root(phase, j ,k);
           if(energy)
           {
               energy_crc_check_correct(energy, CRC_CHECK , 1 , 1 );  

               len = energy_cfg.max_tariff << 1;
               hfp_memcopy(buf , (uint8 *)energy->root->ec_pulse , len);
               checksum = 0xa5a5 + hfp_do_sum(buf, (uint16)len);
               buf += len;
               index += len;
               *(buf++) = (uint8)(checksum>>8);
               *(buf++) = (uint8)checksum;
               index += 2;
           }
        }
    }
    return index;
}
uint16 power_off(uint8 *buf)
{

    int8 i;
    uint16 len = 0;
    uint16 total_len = 0;
#ifndef WIN32    
    emu_driver.ioctrl(CTRL_DEVICE_CLOSE, &device_emu, __NULL);
#endif   
    if(pulse_allsave_state == 0x3B69)
    {
        return 0;
    }
    for (i = 0; i < energy_cfg.max_phase; i++)
    {
        len = pulse_powoff_save(i , buf);
        buf += len;
        total_len += len;
    }
#if 0
    for (i = 0; i < energy_cfg.max_phase; i++)
    {
        len = pulse_powoff_save(i , buf);
        buf += len;
        total_len += len;
    }
#endif
    pulse_allsave_state = 0x3B69;
    
    return total_len;

}

uint8  init(void)
{
    
    
    return true;
}


void energy_add_hook(struct energy_ops_attr_t *energy_ops_attr , uint8 pulse_add)
{
    struct energy_object_t *energy;
    uint16 k;
    
    uint32 category = energy_ops_attr->category;
    uint32 phase = energy_ops_attr->phase;
    uint32 tariff = energy_ops_attr->tariff;
    
    
    
    for(k=1;k<energy_cfg.hook_num;k++)
    {
        /*反向直接计入正向电能里面*/
        if(category<2)
        {
            category = 0;
        }
        energy = energy_cfg.energy_callback->get_energy_root(phase , category , k);
        if(energy)
        {
            if (0 != tariff)
            {
                energy->var->pulse_save_state = 0;
            }
            if (true == energy_increment_save(energy , energy_ops_attr , pulse_add , k))
            {
                energy->var->pulse_save_state = 0x9669;
            }
            if ((0x9669 == energy->var->pulse_save_state)&&(!tariff))
            {
                energy->var->pulse_save_state = 0;
                hfp_write(energy->energy_fsid->energy_class_id, (uint8 *)&energy->root->ec_kwh[0], energy_cfg.max_tariff << 2, __NULL);
                hfp_write(energy->energy_fsid->pulse_class_id, (uint8 *)&energy->root->ec_pulse[0], energy_cfg.max_tariff << 1, __NULL);
            }
            energy_crc_check_correct(energy,  CRC_UPDATA , 0 , 1 );
            energy_crc_check_correct(energy,  CRC_UPDATA , 1 , 1 );
        }
    }
}

const struct energy_interface_t CODE plugin_energy =
{
    energy_crc_check_correct,
    get_tariff_index,
    cut_pulse,
    energy_increment_save,
    energy_pulse_count,
    energy_time_save_1min,
    check_meter_parament,
    energy_loop,
    init,
    power_on,
    power_off,
    energy_add_hook,
};


const struct driver_t CODE energy_mem_driver =
{
    NULL,
    read_energy,
    write_energy,
    energy_io_ctrl,
};
/**@}*/
