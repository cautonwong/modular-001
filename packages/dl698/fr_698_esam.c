#include "nidavellir.h"
#include "fr_698.h"
#include "fr_698_esam.h"
#include "io_type.h"

enum verification_choice_e
{
   verification_choice_SID_MAC,
   verification_choice_RN,
   verification_choice_RN_MAC,
   verification_choice_SID,
};

int openM_ioctl(int fd, int request, void *param);


uint16_t software_esam_check(struct instance_t* instance, uint8_t *key, uint8_t *rn, uint8_t *data, uint8_t p2, uint16_t datalen, uint8_t *outbuf)
{
    int fd = 0;
    int request = esam_type_softwarecheck_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    
    input.cmd_type      = p2;//这里填秘钥索引
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    input.Dispersion_factor = key;//这里填比对因子内容。
    input.Dispersion_factor_len = 8;
    input.RN_Data       = rn;//这里填随机数
    input.RN_Data_len   = 16;  //这里填随机数长度
    input.data          = data;//这里填密文
    input.data_len      = datalen;//这里填密文长度
    output.data         = outbuf;  
    
    param.input  = (struct esam_698_input_t *)&input;
    param.output = (struct esam_698_output_t *)&output;    
    openM_ioctl(fd, request, (void *)&param);
    
    if(output.esam_err_no == 0x9000){
        return output.data_len;
    }
    
    return 0;
}



//no
uint16_t write_esam(struct instance_t* instance, uint32_t addr, uint8_t *buf, uint16_t len)
{
    int fd = 0;
    int request = esam_type_write_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};

    input.data      = buf;
    input.data_len  = len;
    input.sid_id    = addr;
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.data = buf;
    
    param.input  = (struct esam_698_input_t *)&input;
    param.output = (struct esam_698_output_t *)&output;
    openM_ioctl(fd, request, (void *)&param);

    if(output.esam_err_no == 0x9000){
        return output.data_len;
    }
    return 0xff;
}

//
uint16_t esam_key_updata(struct instance_t* instance, uint8_t *buf, uint16_t len)
{
    int fd = 0;
    int request = esam_type_write_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    uint16_t offset = 0; 
    uint16_t data_len = 0;
    uint32_t addr = 0;
    
    offset = axdr_decode(TYPE_octet_string, buf, &data_len);
    input.data      = buf+offset;
    input.data_len  = data_len;
    offset = offset + data_len;
    mem_swap_cpy((uint8_t*)&addr, buf + offset + 1, 4);//sid
    //if(812E0000 != addr){
    //    return 0xff;
    //}
    input.sid_id    = addr;
    offset += 5;
    
    input.AttachData_len = buf[offset];
    input.AttachData = buf + offset + 1;
    offset += buf[offset] + 1;
    
    input.MAC_data = buf + offset + 1;
    input.MAC_len = 4;
    offset += 4 + 1;
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.data = buf;
    
    param.input  = (struct esam_698_input_t *)&input;
    param.output = (struct esam_698_output_t *)&output;
    openM_ioctl(fd, request, (void *)&param);

    if(output.esam_err_no == 0x9000){
        return offset;
    }
    return 0xff;
}

//ok
uint8_t read_esam_no(struct instance_t* instance, uint8_t *esam_info)
{
    int fd = 0;
    int request = esam_type_read_esam_no_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    struct mbase_control_block_t* mbase_cb;
    
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.data  = esam_info;
    
    param.input  = (struct esam_698_input_t *)&input;
    param.output = (struct esam_698_output_t *)&output;
    openM_ioctl(fd, request, (void *)&param);
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    mbase_cb->esam_chip_abnormal = output.esam_chip_abnormal;
    
    return output.data_len;
}


//ok
uint8_t read_esam_data(struct instance_t* instance, uint32_t addr, uint8_t *buf, uint16_t len)
{
    int fd = 0;
    int request = esam_type_read_esam_data_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    struct mbase_control_block_t* mbase_cb;
    
    input.data      = buf;
    input.data_len  = len;
    input.sid_id    = addr;
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.data = buf;
    
    param.input  = (struct esam_698_input_t *)&input;
    param.output = (struct esam_698_output_t *)&output;
    openM_ioctl(fd, request, (void *)&param);
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    mbase_cb->esam_chip_abnormal = output.esam_chip_abnormal;
    return output.data_len;
}


//ok
uint8_t esam_consult_key(struct instance_t* instance, uint8_t cmd, uint8_t *p)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};

    request = esam_type_meet_consult_cmd;
    input.cmd_type = cmd;// 1-from 终端  0-from 主站
    input.data     = p;
    input.data_len = 32;
    input.MAC_data = p + 32 + 1;
    input.MAC_len  = 4;
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.data     = p;
    if(cmd == 1){// 1-from 终端  0-from 主站
        output.MAC_data = p+64;
    }else{
        output.MAC_data = p+48;
    }
    
    param.input  = (struct esam_698_input_t *)&input;
    param.output = (struct esam_698_output_t *)&output;
    openM_ioctl(fd, request, (void *)&param);
    return (output.data_len + output.MAC_len) ;
}


//ok
uint8_t read_esam_info(struct instance_t* instance, uint8_t *esam_info, uint8_t index)
{
    int fd = 0;
    int request = esam_type_get_esam_p2_info_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    struct mbase_control_block_t* mbase_cb;
    
    input.cmd_type = index;
    output.data    = esam_info;
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度

    param.input  = (struct esam_698_input_t *)&input;
    param.output = (struct esam_698_output_t *)&output;    
    openM_ioctl(fd, request, (void *)&param);
        
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    mbase_cb->esam_chip_abnormal = output.esam_chip_abnormal;
    if(output.data_len > 0)
        return output.data_len;
    
    return 0;
}


//ok
uint8_t get_key_status(struct instance_t* instance)
{
    uint8_t Buf[16] = {0};
    uint32_t keystate=0;
	
	read_esam_info(instance, Buf,0X04);//读密钥状态
    memcpy((uint8_t*)&keystate, (uint8_t*)&Buf[0], 4);

	if( (keystate >= 0x7FFFFFFF) && (Buf[4] >= 0xF0) ) 
        return false; //修改35条密钥以上，为私钥状态
	else 
        return true;
}

//OK
void esam_698_init(uint8_t power_mode)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    
    if(power_mode == AC_POWER_ON)
    {
        request = esam_type_poweron_cmd;
        openM_ioctl(fd, request, (void *)&param);
    }
}


int fr_698_handle_esam(struct esam_698_input_t *p_input, struct esam_698_output_t *p_output)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;

    if(0 == p_input->cmd_type){
      
        if((verification_choice_RN == p_input->verification_choice)||
          (verification_choice_RN_MAC == p_input->verification_choice))
        {
            request = esam_type_rn_mac_cmd;
            if(SECURITY_CHOICE_0_Plaintext == p_input->security_choice) {
                p_input->cmd_type = 1;
            }
        }
        
        if((verification_choice_SID_MAC == p_input->verification_choice)||
           (verification_choice_SID == p_input->verification_choice)){
            request = esam_type_sid_mac_cmd;
            
            if((SECURITY_CHOICE_1_Ciphertext == p_input->security_choice) &&
               (verification_choice_SID == p_input->verification_choice)){
                p_input->cmd_type = 2;
                request = esam_type_sid_mac_cmd;
            }
            if((SECURITY_CHOICE_1_Ciphertext == p_input->security_choice) &&
               (verification_choice_SID_MAC == p_input->verification_choice)){
                p_input->cmd_type = 3;
                request = esam_type_sid_mac_cmd;
            }
            if((SECURITY_CHOICE_0_Plaintext == p_input->security_choice) &&
               (verification_choice_SID_MAC == p_input->verification_choice)){
                p_input->cmd_type = 1;
                request = esam_type_sid_mac_cmd;
            }
        }
        p_output->MAC_len = 4;
    }else{
      
        if((verification_choice_RN == p_input->verification_choice)||
          (verification_choice_RN_MAC == p_input->verification_choice))
        {
            request = esam_type_rn_mac_cmd;
            if(SECURITY_CHOICE_0_Plaintext == p_input->security_choice) {
                p_input->cmd_type = 1;
            }
        }
        
        if((verification_choice_SID_MAC == p_input->verification_choice) ||
           (verification_choice_SID == p_input->verification_choice)){
            request = esam_type_ver_sid_mac_cmd;
            
            if(SECURITY_CHOICE_1_Ciphertext == p_input->security_choice) {
                p_input->cmd_type = 1;
            }
        }
    }
    param.input = p_input;
    param.output = p_output;
    return openM_ioctl(fd, request, (void *)&param);
}

//主动上报  
uint8_t get_upreport_esam_rn_mac(struct instance_t* instance, uint8_t *apdu, uint8_t apdu_len , uint8_t *dst)
{
    int fd = 0;
    int request = esam_type_upreport_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    
    input.data = apdu;
    input.data_len = apdu_len;
    param.input = &input;
    
    output.data = dst;
    param.output = &output;
    
    return openM_ioctl(fd, request, (void *)&param);
}
//主动上报验证
uint8_t check_upreport_esam_rn_mac(struct instance_t* instance, uint8_t *apdu, uint8_t apdu_len)
{
    int fd = 0;
    int request = esam_type_upreportcheck_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    
    input.data = apdu;
    input.data_len = apdu_len;
    param.input = &input;

    return openM_ioctl(fd, request, (void *)&param);
}

