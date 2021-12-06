#include "nidavellir.h"
#include "fr_698.h"
#include "resource_tool.h"

int _record_sel_9(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t last_no, void *info)
{
    struct record_storage_id_info_t record_id_info; 
    struct circular_info_t circular_info = {0};
    
    int ret = get_record_id_info(oad, &record_id_info);
    
    if(ret > 0)
    {
        ret = nd_log_output(instance, record_id_info.save_index,  buf, 512, &circular_info);
        circular_info.curr_num = last_no;
        ret = nd_log_output(instance, record_id_info.save_index,  buf, 512, &circular_info);
        if(ret < 0)
        {
            ret = -1;
        }
    }
    return ret;
}

int GET_record_sel_9(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t last_no, void *info)
{
    return _record_sel_9(instance, oad, buf, last_no, info);
}
