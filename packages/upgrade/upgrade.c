#include "nidavellir.h"

//测试某位是否为1
#define	TEST_BIT_BYTE(a,x)	((a)[(x)/8] & (1<<((x)%8)))

//设置某位为1
#define	SET_BIT_BYTE(a,x)	((a)[(x)/8] |= (1<<((x)%8)))

//清除某位为0
#define	CLR_BIT_BYTE(a,x)	((a)[(x)/8] &= (~(1<<((x)%8))))

/*
 * 0x304000ff
 * 起始升级帧：
 * 上位机发：68 11 11 11 11 11 11  68 7A len1 len2 32 33 73 63  data cs 16
 * len1 len2:如256字节，len1=00 len2-01
 * data: 升级文件前256字节
 * 表回：：68 11 11 11 11 11 11  68 FA len1 len2 32 33 73 63  ret cs 16
 * ret: 0--成功，其他为错误码

 * 0x304000ee
 * 升级帧：
 * 上位机发：68 11 11 11 11 11 11  68 7A len1 len2 21 33 73 63  index1 index2 data cs 16
 * len1 len2:如256字节，len1=00 len2-01;
 * index:2字节，帧序号(0-N), 如序号8  index1=0x08 index2=00;
 * data: 从升级文件256字节开始，分帧下发
 * 表回：：68 11 11 11 11 11 11  68 FA len1 len2 21 33 73 63  ret cs 16
 * ret: 0--成功，其他为错误码

 * 0x304000dd
 * 位图帧：
 * 上位机发：68 11 11 11 11 11 11  68 7A len1 len2 10 33 73 63  bitnum1 bitnum2 cs 16
 * len1 len2:如256字节，len1=00 len2-01;
 * bitnum:2字节，帧序号(0-N), 如8帧 bitnum1=0x11 bitnum2=00;
 * 表回：68 11 11 11 11 11 11  68 FA len1 len2 10 33 73 63  bitnum1 bitnum2 data cs 16
 * bitnum:2字节，帧序号(0-N), 如8帧 bitnum1=0x11 bitnum2=00;
 * data: 位图信息bitmap
 */


int upgrade_file_handle(struct instance_t* instance, uint8_t *data, uint16_t len, uint8_t *data_tx, uint8_t *tx_len)
{
    uint8_t factor_mode = 0;
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    if(mbase_cb->factor_mode == true){
        factor_mode = true;
    }
    
    struct upgrade_info_t *_info = (struct upgrade_info_t *)&instance->upgrade_info;
    
    uint16_t frm_no = 0;
    uint16_t frm_size = 0;
    uint32_t oad = 0;
    memcpy(&oad, data, 4);    
    memcpy(data_tx, data, 4); 
    uint8_t offset_tx = 4;
    uint16_t offset = 4;
    if(oad == 0x304000ff){
        memset((uint8_t*)&_info->magic_num, 0, sizeof(struct upgrade_info_t));
        memcpy((uint8_t*)&_info->magic_num, data+offset, 12);
        offset+=12;
        data_tx[offset_tx] = 1;
        if(0xef87 == _info->magic_num){
            _info->status = 0x01;
            _info->frm_no_last = 0;
            _info->cur_verification = 0x9efc;
            //存储_info
            data_tx[offset_tx] = 0;
        }
        offset_tx++;
    }
    
    if(oad == 0x304000ee){
        if(0xef87 != _info->magic_num){
            //读取_info
        }
        data_tx[offset_tx] = 1;
        if(0xef87 == _info->magic_num){
             memcpy(&frm_no, data+offset, 2);
             offset+=2;
             memcpy(&frm_size, data+offset, 2);
             offset+=2;
             data_tx[offset_tx] = 2;
             if((frm_size == (len-offset)) && (frm_no < _info->frm_num) && (frm_size <=_info->frm_size)) {
                data+=offset;
                int ret = 0;//dev_write(NULL, frm_no*_info->frm_size, data+offset, frm_size);
                data_tx[offset_tx] = 3;
                if(ret){
                    CLR_BIT_BYTE(_info->bits, frm_no);
                    _info->status = 0x02;
                    data_tx[offset_tx] = 0;
                    if(_info->frm_no_last == frm_no){
                        _info->frm_no_last++;
                        for(int i =0; i <frm_size; i++){
                            _info->cur_verification = update_crc16(_info->cur_verification, data[i]);
                        }
                    }else{
                        //清cur_verification
                    }
                    //存储_info
                }
            }
        }
        offset_tx++;
    }
    if(0xef87 == _info->magic_num){
        if(_info->cur_verification == _info->file_verification){
            //准备升级
        }
    }
                 
    if(oad == 0x304000dd){
        
    }
    *tx_len = offset_tx;
    return 0; 
}
                 
int upgrade_check_handle(struct instance_t* instance)
{
    uint8_t factor_mode = 0;
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    if(mbase_cb->factor_mode == true){
        factor_mode = true;
    }
    
    struct upgrade_info_t *_info = (struct upgrade_info_t *)&instance->upgrade_info;
    
    
    if(0xef87 != _info->magic_num){
        //读取_info
    }
    if(0xef87 == _info->magic_num){
      
    }
    return 0; 
}