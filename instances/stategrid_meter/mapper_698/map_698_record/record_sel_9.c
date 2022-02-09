#include "nidavellir.h"
#include "fr_698.h"


#if 0
int _record_sel_9(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, void *rcsd_info)
{
    struct record_storage_id_info_t record_id_info; 
    struct circular_info_t circular_info = {0};
    uint32_t *rcsd_req = (uint32_t *)rcsd_info; // 待定
    uint8_t temp[2000]; //待调整 需要替换协议共用缓存 2k   请求关联对象
    uint32_t rcsd_src[50];//待调整   源关联对象
    int len = -1;
    
    int ret = get_record_id_info(instance, oad, &record_id_info);
    
    if(ret > 0)
    {
        ret = nd_log_output(instance, record_id_info.save_index,  temp, 512, &circular_info);
        circular_info.curr_num = circular_info.valid_num - rsd_info->last_no;
        ret = nd_log_output(instance, record_id_info.save_index,  temp, 512, &circular_info);

        if(ret < 0)
        {
            ret = -1;
        }
        else
        {
            get_capture_list(instance, oad, rcsd_src, 3);
            len = record_package_01(instance, buf+1, temp, rcsd_req, rcsd_src);
            buf[0] = 1;// 条数
            len += 1; //
        }
    }
    return len;
}
#endif

int GET_record_sel_9(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info)
{
    return record_data_package(instance, oad, buf, rsd_info, rcsd_info);
}
