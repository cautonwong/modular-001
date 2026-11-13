#include "nidavellir.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "domain/load_control/relay_control_port.h"
#include "domain/load_control/legacy/load_control_applications.h"
#include "infra/kernel/resource_blocks.h"
#include "kernel/legacy_control_block.h"

/* Relay check counters are advanced by the GPIO interrupt handler. */
uint16_t relay_off_chk, relay_on_chk;

void save_relay_state(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t s_addr = 0, len = 0;

    s_addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
    nd_mbase_offset_cmd()  -  nd_mbase_offset_persistent_section_start());
    len = nd_mbase_offset_sta() - nd_mbase_offset_cmd() + sizeof(nd_mbase_sta(mbase_cb));
    nd_set_offset(instance, s_addr, (uint8_t*)&nd_mbase_cmd(mbase_cb), len);
}

//每秒调用
void do_relay(struct instance_t* instance, struct pipeline_t* pipeline)
{
    (void)pipeline;
    relay_control_second(instance);
}

void relay_led_ctl(struct instance_t* instance, struct pipeline_t* pipeline)
{
    (void)pipeline;
    relay_control_led(instance);
}
