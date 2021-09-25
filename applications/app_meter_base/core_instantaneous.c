/**
 * @ingroup  instantaneous
 * @brief    瞬时量.
 *
 * @{
 * @file
 *
 */
#include "hfp_system.h"
#include "Instantaneous.h"
#include "lib_calclib.h"
#include "fs.h"


extern uint8 sys_power_off_check(void);

void reset_instaneous(void)
{
    uint8               i;
    struct phase_real_t *which_instantaneous;

    for (i = 0; i < max_real_ops_attr.phase; i++)
    {
        which_instantaneous = instantaneous_callback.get_instantaneous_memory(i);

        if (which_instantaneous)
        {
            hfp_buffer_clear((uint8 *)which_instantaneous->real, max_real_ops_attr.category << 2);
            
            
        }
    }
}


void instantaneous_init(void)
{
    reset_instaneous();
}


void instantaneous_power_on(void)
{
    plugin_instantaneous.emu_reset_config();                               /**< 初始化计量芯片 */
    plugin_instantaneous.emu_calibration_parameter_config();               /**< 检查校准参数 */
    plugin_instantaneous.instantaneous_parameter_check(1);                  /**< 上电恢复参数 */
    reset_instaneous();                                                    /**< 复位瞬时量数据 */
    instantaneous_callback.instantaneous_poweron_hook();
}


uint16 instantaneous_power_off(uint8 *buf)
{
    uint16 len = 0;
    if(__NULL != instantaneous_callback.instantaneous_poweroff_hook)
	{
	    len = instantaneous_callback.instantaneous_poweroff_hook(buf);
	}
    
    reset_instaneous();
    
	return len;
}

void cal_quadrant(struct phase_real_t *instantaneous)
{
     const uint8 a[2][2] = { {1,4},{2,3} };
     uint8 c;
     uint8 d;

     c = (instantaneous->real[POW_ACT]  < 0 );
     d = (instantaneous->real[POW_REACT] < 0);
     instantaneous->quadrant = a[c][d];
}

void instantaneous_proc(void)
{
    uint8               i;
    struct phase_real_t *which_instantaneous;
    
    plugin_instantaneous.instantaneous_parameter_check(0);
    
    for (i = 0; i < max_real_ops_attr.phase; i++)
    {
        if (sys_power_off_check())
        {
            return;
        }
        which_instantaneous = instantaneous_callback.get_instantaneous_memory(i);
        plugin_machine.clear_watchdog();
        if (which_instantaneous)
        {
            plugin_instantaneous.instantaneous_value_calculate(which_instantaneous, i);
            if(instantaneous_callback.cal_instantaneous_hook)
            {
                instantaneous_callback.cal_instantaneous_hook(which_instantaneous, i);
            }
            cal_quadrant(which_instantaneous);
        }
    }
}


void instantaneous_core_handle(uint16 msg)
{
    if(plugin_instantaneous.instantaneous_handle)
    {
        plugin_instantaneous.instantaneous_handle(msg);
    }
}

void timing_awaken_instantaneous(void)
{
    if(instantaneous_callback.awake)
    {
        instantaneous_callback.awake();
    }
}

const struct standalone_timing_node_t CODE instantaneous_mount_node =
{
    0x02, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99,
};
const struct module_execute_t CODE instantaneous_execute_list[] =
{
    { &instantaneous_mount_node, instantaneous_proc, },

    { __NULL,                    __NULL },
};


const struct module_t CODE core_instantaneous =
{
    instantaneous_execute_list,
    instantaneous_init,
    __NULL,
    instantaneous_power_on,
    instantaneous_core_handle,
    __NULL,
    instantaneous_power_off,
    timing_awaken_instantaneous,
    __NULL,
};
/** @} */
