/**
 * @defgroup    instantaneous_plugin Plugin Instantaneous
 * @ingroup   plugins
 * @see           instantaneous
 * @brief       hfp 瞬时量插件
 *
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "Instantaneous.h"
#include "fs.h"
#include "lib_calclib.h"

/*******************************************************************************
 * 负载象限状态枚举
 ******************************************************************************/
enum
{
    reactQNull,
    reactQ1,
    reactQ2,
    reactQ3,
    reactQ4
};

#define   POWON_CONFIG    0x01
#define   TIMI_CHECK      0x02

extern uint8 sys_power_off_check(void);

uint8 emu_reset_config(void)
{
    uint8              temp_buffer[32];
    void               *parameter;
    struct file_stat_t stat_file;

    parameter = temp_buffer;
    if (true == hfp_stat(0x2005, (struct file_stat_t *)&stat_file))
    {
        if (stat_file.len > 32)
        {
            return false;
        }
    }
    if (true != hfp_read(0x2005, (uint8 *)temp_buffer, stat_file.len, __NULL))
    {
        parameter = __NULL;
    }

    return emu_driver.ioctrl(CTRL_DEVICE_OPEN, &device_emu, parameter);
}


static uint8 emu_calibration_parameter(uint8 mode)
{
    uint8              i;
    struct file_stat_t stat_file;
    uint8              temp_buffer[32];

    for (i = 0; i < 3; i++)
    {
        if (true == hfp_stat(0x2001 + i, (struct file_stat_t *)&stat_file))
        {
            if (stat_file.len > 32)
            {
                return false;
            }
        }

        if (true == hfp_read(0x2001 + i, (uint8 *)temp_buffer, stat_file.len, __NULL))
        {
            if (POWON_CONFIG == mode)
            {
                emu_driver.ioctrl(CTRL_EMU_ADJ_A_SET + i, &device_emu, temp_buffer);
            }
        }
        #if 0
        else
        {
            emu_driver.ioctrl(CTRL_EMU_ADJ_INIT, &device_emu, __NULL);
        }
        #endif
    }

    return true;
}


uint8 emu_calibration_parameter_config(void)
{
    emu_calibration_parameter(POWON_CONFIG);
    return true;
}


void instantaneous_parameter_check(uint8 mode)
{
    uint8 i;
    /*mode=1时表示不直接从外部存储器中读取参数*/
    for (i = 0; i < instantaneous_parameter_num; i++)
    {
        if (sys_power_off_check())
        {
            return;
        }
        if ((*(instantaneous_parameter[i].mac) != instantaneous_parameter[i].mac_calculate(instantaneous_parameter[i].parameter_data, instantaneous_parameter[i].len)) || (mode == 1))
        {
            if (true != hfp_read(instantaneous_parameter[i].parameter_fsid, (uint8 *)instantaneous_parameter[i].parameter_data, instantaneous_parameter[i].len, __NULL))
            {
                hfp_code_to_ram_copy((uint8 *)instantaneous_parameter[i].parameter_data, (const uint8 CODE *)instantaneous_parameter[i].parameter_default, instantaneous_parameter[i].len);
            }
            *(instantaneous_parameter[i].mac) = instantaneous_parameter[i].mac_calculate(instantaneous_parameter[i].parameter_data, instantaneous_parameter[i].len);
        }
    }
}

void instantaneous_plugin_handle(uint16 msg)
{
      static uint8 cnt = 0;
      if((hfp_system_state.power_state == ACPOWER_SUPPLY)&&(msg == SYS_MSGID_SEC))
      {
            emu_driver.ioctrl(CTRL_EMU_COMPENSATE, &device_emu, __NULL);
            
            if(false == emu_driver.ioctrl(CTRL_EMU_PARA_CHECK, &device_emu, __NULL))
            {
                cnt++;
            }
            else
            {
                cnt = 0;
            }
            
            if(cnt > 3)
            {
                emu_reset_config();
                
                emu_calibration_parameter(POWON_CONFIG);
                
                cnt = 0;
            }              
      }
}

int32 dec_convert(int32 value, struct unit_decmail_t *fs_unit, struct unit_decmail_t *read_unit)
{   
    if(fs_unit->unit == read_unit->unit - 1)
    {
        value /= 1000;
        return hfp_dec_convert(value, fs_unit->decmail, read_unit->decmail);
    }
    
    return hfp_dec_convert(value, fs_unit->decmail, read_unit->decmail);
}

uint8 read_instantaneous(uint32 id, uint8 *dest, uint16 len, const struct hfp_device_t CODE *device)
{
    int32 value = 0;
    struct access_pattern_t *read;
    struct phase_real_t     *which_instantaneous;
    const struct read_ops_t CODE *read_ops_list;
    const struct read_instantaneous_status_t CODE *read_status;
    struct unit_decmail_t rd_decmail , local_decimal;
      
    uint8 real_class = (uint8)((id & 0x0000000F));
    uint8 phase      = (uint8)((id & 0x000000F0) >> 4);
    uint8 ops_class  = (uint8)((id & 0x00000F00) >> 8);
    uint8 object_id  = (uint8)((id & 0x0000F000) >> 8);

    phase = phase % 4;

    which_instantaneous = instantaneous_callback.get_instantaneous_memory(phase);

    read_ops_list = which_instantaneous->read_ops_list;
    while (read_ops_list->get_value_ops)
    {
        value++;
        read_ops_list++;
    }
    if ((0xC0 == object_id) && (ops_class < value))
    {
        read  = (struct access_pattern_t *)device;
        rd_decmail.unit = read->unit;
        rd_decmail.decmail = read->decimals;

        value = which_instantaneous->read_ops_list[ops_class].get_value_ops(which_instantaneous, real_class, phase);
        if(true == which_instantaneous->read_ops_list[ops_class].get_local_decimal(real_class , phase , &local_decimal))
        {
            value = dec_convert(value, &local_decimal, &rd_decmail);
        }
        if (read->format == FORM_BCD)
        {
            hfp_longhex_to_bcd_signed(value , dest , len);
        }
        else
        {
            hfp_memcopy(dest, (uint8 *)&value, len);
        }
        return true;
    }
    
    object_id  = (uint8)((id & 0x0000FF00) >> 8);
    if(0x6F == object_id)
    {
        read_status = which_instantaneous->read_status_list;
        while(read_status->status_fs_id)
        {
            if(read_status->status_fs_id == id)
            {
                *dest = read_status->get_status_judge(which_instantaneous);
                return true;
            }
            read_status++;
        }
    }
    return false;
}

void ah_data_clear(void)
{
    uint8 i;
    struct phase_real_t     *which_instantaneous;

    for(i=0;i<3;i++)
    {
        which_instantaneous = instantaneous_callback.get_instantaneous_memory(i);
        if(which_instantaneous)
        {
            if(which_instantaneous->current_acc)
            {
                which_instantaneous->current_acc->milli_ampere_second = 0;
                which_instantaneous->current_acc->ampere_hour = 0;
            }
            hfp_write(0x2803+i, (uint8 *)which_instantaneous->current_acc, 8 , __NULL);
        }
    }
}

uint8 instantaneous_io_ctrl(uint8 cmd, const struct hfp_device_t CODE *device , void *ctrl_parameter)
{
    switch(cmd)
    {
        case CTRL_DATA_CLEAR:
                ah_data_clear();
            return true;
            break;
        default:
            break;
    }
    return false;
}

struct emu_parameter_t
{
    uint16 emu_reg_addr;
    uint8  fd_offset;
};


void get_instantaneous_notify(void)
{
    uint16 note_id = 0;
    
    while(0x00 != (note_id =  hfp_notify(name_instantaneous)))
	{
        switch(note_id)
        {
            case 0x2004:   
            case 0x2007:                   
                    instantaneous_parameter_check(1);
                    break;           
            case 0x2FF1:
                emu_calibration_oneself();
                break;
            case 0x6F98:
                ah_data_clear();
                break;
            default:
                break;
         }
    }
}

void instantaneous_real_calculate(struct phase_real_t *instantaneous, uint8 phase)
{
    uint8 j;
    int32 value;

    get_instantaneous_notify();
    
    
    for (j = 0; j < max_real_ops_attr.category; j++)
    {
        if(instantaneous->real_id[j] != 0xFF)
        {
            if (true == emu_driver.read(instantaneous->real_id[j], (uint8 *)&value, 4, &device_emu))
            {
                /*瞬时量前5个需要因传感器方案不同而设置不同的系数，后面的不需要设置系数，由计量驱动固定输出*/
                if(j < 5)
                {
                    instantaneous->real[j] = (int32)((instantaneous->real_fact[j] * value) + 0.5);
                }
                else
                {
                    instantaneous->real[j] = value;
                }
            }
        }
    }
}

const struct instantaneous_interface_t CODE plugin_instantaneous =
{
    emu_reset_config,
    emu_calibration_parameter_config,
    instantaneous_plugin_handle,
    instantaneous_parameter_check,
    emu_calibration_oneself,
    instantaneous_real_calculate,
};


const struct driver_t CODE instantaneous_mem_driver =
{
    NULL,
    read_instantaneous,
    NULL,
    instantaneous_io_ctrl,
};
/**@}*/