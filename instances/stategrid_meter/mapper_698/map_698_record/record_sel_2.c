#include "nidavellir.h"
#include "fr_698.h"


int _record_sel_2(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info)
{
    return 1;
#if 0
    struct record_storage_id_info_t record_id_info; 
    struct circular_info_t circular_info = {0};
    uint32_t *rcsd_req = (uint32_t *)rcsd_info; // 待定
    uint8_t temp[2000]; //待调整 需要替换协议共用缓存 2k   请求关联对象
    uint32_t rcsd_src[50];//待调整   源关联对象
    uint16_t i, read_num, number = 0;
    uint16_t one_len = 0,len = 0;
    uint8_t count = 0;
    uint8_t *p = (buf + 3); // 预留3个字节条数
    int ret = 0;
    
    ret = get_record_id_info(instance, oad, &record_id_info);
    if(ret < 0)
    {
        return -1;
    }

    circular_info.start_mark = rsd_info->start_mark;
    circular_info.end_mark = rsd_info->end_mark;
    circular_info.mode =   rsd_info->mode;

    ret = nd_log_output(instance, record_id_info.save_index,  temp, 1, &circular_info);

    get_capture_list(instance, oad, rcsd_src, 3);
    read_num = circular_info.valid_num;
    for (i = circular_info.curr_num; i < read_num; i++)
    {
        ret = nd_log_output(instance, record_id_info.save_index,  temp, circular_info.unit_size, &circular_info);
        if(ret < 0)
        {
            return -1;
        }
        
        //长度判断
        if (one_len == 0)
        {   
            //预算打包长度
            one_len = get_package_01_len(instance, p+len, temp, rcsd_req, rcsd_src);
        }

        if((p - buf + len + one_len) > 2000) //待调整
        {
            //DAR_BUFFER_OVERFLOW;
            return -1;
        }
        //开始打包
        one_len = record_package_01(instance, p+len, temp, rcsd_req, rcsd_src);
        len += one_len;
        number++;
    }      
        
    count = axdr_len_encode(p, number);

    if (count < 3)
    {
        memcpy(p + count, p + 3, len);
    }
    len += count;
             
    return len;
#endif
}

int GET_record_sel_2(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info,  void *ctx)
{
    return record_data_package(instance, oad, buf, rsd_info, rcsd_info,  ctx);
}
