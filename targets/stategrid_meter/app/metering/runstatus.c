#include "nidavellir.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "application/adapters/dl698/meter_base_view.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "application/ports/resource_access.h"
#include "type_def.h"
#include "domain/common/status.h"
#include "domain/metering/run_status.h"
#include "fr_698.h"
#include "domain/load_control/relay_status.h"
#include "targets/stategrid_meter/app/events/events_applications.h"
#include "targets/stategrid_meter/app/metering/metering_applications.h"
#include "infra/kernel/resource_blocks.h"
#include "kernel/legacy_control_block.h"
#include "application/ports/platform.h"


void Meter_Run_State(void *instance, void *pipeline)
{
    struct mbase_control_block_t* mbase_cb;
    nd_run_status_inputs_t inputs = {0};

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    inputs.battery_mv = nd_mbase_voltage_battery(mbase_cb);
    inputs.power_direction = nd_mbase_power_dir(mbase_cb);
    inputs.switch_error = nd_mbase_SwitchErrS_status(mbase_cb) == 1;
    inputs.esam_error = nd_mbase_esam_chip_abnormal(mbase_cb) == ND_EVENT_SET;
    inputs.memory_error = nd_platform_device_error();
    inputs.rtc_error = nd_mbase_RTCErrS_status(mbase_cb) == 1;
    inputs.over_current = get_activeflag(instance, OverCurrentAS) == true;
    inputs.infrared_authenticated = nd_mbase_infrared_auth_timeout(mbase_cb) > 0;
    inputs.terminal_authenticated = nd_mbase_term_auth_timeout(mbase_cb) > 0;
    inputs.terminal_security = fr_698_current_connect_status_get(instance, NULL, 1) == true;
    inputs.master_security =
        fr_698_current_connect_status_get(instance, NULL, 0) == true ||
        nd_mbase_auth_timeout_645(mbase_cb) != 0;
    inputs.relay_alarm = (nd_mbase_sta(mbase_cb) & F_RemoteAlarm) != 0;
    inputs.relay_commanded_off = (nd_mbase_sta(mbase_cb) & REMOTE_CMD_REALAY_OFF_STA) != 0;
    inputs.protected_supply = (nd_mbase_sta(mbase_cb) & 0x01) != 0;
    inputs.relay_off = nd_relay_status_is_off();
    inputs.cover_open = get_activeflag(instance, CoverOpenS) == true;
    inputs.power_off = get_activeflag(instance, PowerOffS) == true;

    nd_run_status_update(&inputs, nd_mbase_run_state(mbase_cb));
}

//static 参数校验
int16_t Check_para_Pro(void *instance)
{
    struct mbase_control_block_t* mbase_cb;
    uint32_t check_sum;
    uint32_t addr = 0x00;
    size_t mbase_block_static_size = nd_mbase_offset_static_section_end() - nd_mbase_offset_static_section_start() + 1;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    /******************* mbase_control_block_t *******************/
    check_sum = do_block_static_checksum(instance);
    
    if(nd_mbase_static_chksum(mbase_cb) != check_sum) //RAM异常
    {
        get_block_frist_addr(instance, RES_DEV_RAM, mbase_block_static, &addr);//RAM对应的地址
        uint32_t res = get_res(DEV_EEPROM, mbase_block_static_eprom);
        nd_get_block(instance, res, (uint8_t*)addr, mbase_block_static_size); //从EE主区获取，覆盖至RAM
        check_sum = do_block_static_checksum(instance);
        if(nd_mbase_static_chksum(mbase_cb) != check_sum)    //主区效验错误
        {
            res = get_res(DEV_EEPROM, mbase_block_static_eprom_bp);
            nd_get_block(instance, res, (uint8_t*)addr, mbase_block_static_size);//从EE备份区获取，覆盖至RAM
            check_sum = do_block_static_checksum(instance);
            if(nd_mbase_static_chksum(mbase_cb) == check_sum)    //备份正确
            {
                //备份区覆盖主区   
                res = get_res(DEV_EEPROM, mbase_block_static_eprom);
                nd_set_block(instance, res,(uint8_t *)addr,mbase_block_static_size); //将RAM中的数据保存到EPROM;
                return 0;
            }
            else
            {
                return -1;
            }
        }
        else    //主区正确，获取备份数据
        {
            res = get_res(DEV_EEPROM, mbase_block_static_eprom_bp);
            nd_get_block(instance, res, (uint8_t*)addr, mbase_block_static_size);//从EE备份区获取，覆盖至RAM
            check_sum = do_block_static_checksum(instance);
            if(nd_mbase_static_chksum(mbase_cb) != check_sum)    //备份错误
            {
                //主区覆盖备份区
                res = get_res(DEV_EEPROM, mbase_block_static_eprom_bp);
                nd_set_block(instance, res,(uint8_t *)addr,mbase_block_static_size); //将RAM中的数据保存到EPROM;
                return 0;
            }
        }
    }
    return 1;
}
