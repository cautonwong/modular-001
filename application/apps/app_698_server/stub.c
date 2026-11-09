#include "nidavellir.h"


int16_t nd_set_offset(struct instance_t* instance, uint32_t offset, uint8_t* buf, uint16_t len)
{
}
int clr_record(struct instance_t* instance, uint32_t oad)
{
	
}
void send_event_mesg_pro(struct instance_t* instance, struct pipeline_t* task, uint16_t RecordId, uint8_t* payload, int mode)
{
	
}
int freeze_clr(struct instance_t* instance)
{
}

int event_clr(struct instance_t* instance)
{
	
}
uint8_t read_esam_data(uint32_t addr, uint8_t *buf, uint16_t len)
{
	
}

int32_t Get_Cal_Active_Energy(struct mbase_control_block_t* mbase_control_block, uint8_t index, uint8_t type)
{
}

void rtc_get_from_tm(struct instance_t* instance, uint8_t* p)
{

}
uint16_t write_esam(uint32_t addr, uint8_t* buf, uint16_t len)
{
}
uint8_t read_esam_no(uint8_t* esam_info) 
{
}

int get_event_para_pack(struct instance_t* instance, uint32_t oad, struct event_para_pack_t* para_pack)
{
}

int16_t nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{

}
uint8_t write_mcu_rtc(uint8_t addr, uint8_t* buf, uint16_t len)
{
}

uint8_t read_mcu_rtc(uint8_t addr, struct instance_t* instance)
{
}

unsigned short display_oad_to_id(unsigned char* oad, unsigned char len)
{
}

uint8_t read_esam_info(uint8_t* esam_info, uint8_t index)
{
}

uint8_t esam_consult_key(uint8_t cmd, uint8_t* p)
{

}

int16_t nd_size(struct instance_t* instance, uint32_t id)
{

}
void cpu_io_lowpower_cfg_chk(struct instance_t* instance)
{
}

void cpu_sleep_manage(struct instance_t* instance, struct pipeline_t* pipeline)
{
}
