#include "targets/stategrid_meter/app/record_storage_adapter.h"

#include "application/ports/record_storage.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "kernel/legacy_control_block.h"
#include "nidavellir.h"

static nd_record_storage_port_t port;

static int target_save_index(void *ctx, uint32_t oad, uint32_t *index)
{
    struct record_storage_id_info_t info;
    if (get_record_id_info((struct instance_t *)ctx, oad, &info) < 0) {
        return -1;
    }
    *index = info.save_index;
    return 0;
}

static int target_change_format(void *ctx, uint8_t *des, const uint8_t *src,
                                uint32_t oad_req, uint32_t oad_src, int mode)
{
    return record_change_format((struct instance_t *)ctx, des, (uint8_t *)src,
                                oad_req, oad_src,
                                (enum format_conversion_mode)mode);
}

static int target_clear_program_list(void *ctx, void *pipeline, void *mesg)
{
    return MeterWr_obj_list_clear((struct instance_t *)ctx,
                                  (struct pipeline_t *)pipeline,
                                  (struct message_t *)mesg);
}

static int target_trigger_645_record(void *ctx, uint32_t di, uint8_t tag)
{
    return trigger_MeterWr645_record((struct instance_t *)ctx, di, tag);
}

void stategrid_record_storage_bind(struct instance_t *instance)
{
    port.ctx = instance;
    port.save_index = target_save_index;
    port.change_format = target_change_format;
    port.clear_program_list = target_clear_program_list;
    port.trigger_645_record = target_trigger_645_record;
    nd_record_storage_bind(&port);
}
