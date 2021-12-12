#include "nidavellir.h"
#include "fr_698.h"
#include "resource_tool.h"

int _record_sel_9(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t last_no, void *info)
{
    struct record_storage_id_info_t record_id_info; 
    struct circular_info_t circular_info = {0};
    uint32_t *rcsd_req = (uint32_t *)info; // 待定
    uint8_t temp[2000]; //待调整 需要替换协议共用缓存 2k
    uint32_t rcsd_src[50];//待调整
    int len = -1;
    
    int ret = get_record_id_info(oad, &record_id_info);
    
    if(ret > 0)
    {
        ret = nd_log_output(instance, record_id_info.save_index,  temp, 512, &circular_info);
        circular_info.curr_num = circular_info.valid_num - last_no;
        ret = nd_log_output(instance, record_id_info.save_index,  temp, 512, &circular_info);

        if(ret < 0)
        {
            ret = -1;
        }
        else
        {
            get_record_rcsd_list(instance, oad, rcsd_src, 3);
            len = record_data_package_01(instance, buf+1, temp, rcsd_req, rcsd_src);
            buf[0] = 1;// 条数
            len += 1; //
        }
    }
    return len;
}

int GET_record_sel_9(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t last_no, void *info)
{
    return _record_sel_9(instance, oad, buf, last_no, info);
}
