#include "nidavellir.h"
#include "fr_698.h"
#include "fr_698_esam.h"
#include "io_type.h"




uint16_t write_esam(struct instance_t* instance, uint32_t addr, uint8_t *buf, uint16_t len)
{
    return 1;  
}

//ok
uint8_t read_esam_no(struct instance_t* instance, uint8_t *esam_info)
{
    int fd = 0;
    int request = esam_type_read_esam_data;
    struct esam_698_data_t param = {0};
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.data  = esam_info;
    
    param.input  = &input;
    param.output = &output;
    openM_ioctl(fd, request, (void *)&param);
    return 1;
}


//ok
uint8_t read_esam_data(struct instance_t* instance, uint32_t addr, uint8_t *buf, uint16_t len)
{
    int fd = 0;
    int request = esam_type_read_esam_data;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};

    input.data      = buf;
    input.data_len  = len;
    input.sid_id    = addr;
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.data = buf;
    
    param.input  = &input;
    param.output = &output;
    openM_ioctl(fd, request, (void *)&param);
    
    return 1;
}


//ok
uint8_t esam_consult_key(struct instance_t* instance, uint8_t cmd, uint8_t *p)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    
    if(cmd)//from 终端
        request = esam_type_terminal_meet_consult;
    else   //from 主站
        request = esam_type_hes_meet_consult;
    
    input.cmd_type = cmd;
    input.data     = p;
    input.data_len = 32;
    input.MAC_data = p + 32 + 1;
    input.MAC_len  = 4;
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.data     = p;
    
    param.input  = &input;
    param.output = &output;
    openM_ioctl(fd, request, (void *)&param);
    return 1;
}


//ok
uint8_t read_esam_info(struct instance_t* instance, uint8_t *esam_info, uint8_t index)
{
    int fd = 0;
    int request = esam_type_get_esam_p2_info;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    
    input.cmd_type = index;
    output.data    = esam_info;
    param.input    = &input;
    param.output   = &output;
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    openM_ioctl(fd, request, (void *)&param);
    if(output.data_len > 0)
        return 1;
    
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


void esam_698_init(void)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    //param.input = p_input;
    //param.output = p_output;
    //return openM_ioctl(fd, request, (void *)&param);
}


int fr_698_handle_esam(struct esam_698_input_t *p_input, struct esam_698_output_t *p_output)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    param.input = p_input;
    param.output = p_output;

    if(0 == p_input->cmd_type){//获取mac
        if(SECURITY_CHOICE_0_Plaintext == p_input->security_choice) {
            request = esam_type_plaintext_mac;
        }
        if(SECURITY_CHOICE_1_Ciphertext == p_input->security_choice) {
            request = esam_type_ciphertext_mac;
        }        
        p_output->MAC_len = 4;
        //request = 
    }else{
    //验证
      
    }

    return openM_ioctl(fd, request, (void *)&param);
}
