#include "nidavellir.h"
#include "fr_698.h"
#include "fr_698_esam.h"

#define io_type_end      (0x10)        //IO请求类型结束


#define esam_type_get_esam_info         1
#define esam_type_get_esam_all_info2    (esam_type_get_esam_info+1)
#define esam_type_get_esam_p2_info      (esam_type_get_esam_all_info2+1)
#define esam_type_hes_meet_consult      (esam_type_get_esam_p2_info+1)
#define esam_type_terminal_meet_consult (esam_type_hes_meet_consult+1)
#define esam_type_plaintext_mac         (esam_type_terminal_meet_consult+1)
#define esam_type_ciphertext            (esam_type_plaintext_mac+1)
#define esam_type_ciphertext_mac        (esam_type_ciphertext+1)
#define esam_type_set_sid_mac           (esam_type_ciphertext_mac+1)
#define esam_type_set_sid               (esam_type_set_sid_mac+1)
#define esam_type_file_updata           (esam_type_set_sid+1)
#define esam_type_key_updata            (esam_type_file_updata+1)
#define esam_type_set_other             (esam_type_key_updata+1)
#define esam_type_ir_authen_request     (esam_type_set_other+1)
#define esam_type_ir_authen             (esam_type_ir_authen_request+1)
#define esam_type_upreport              (esam_type_ir_authen+1)
#define esam_type_terminal_id_authen    (esam_type_upreport+1)
#define esam_type_software_check        (esam_type_terminal_id_authen+1)




int openM_ioctl(int fd, int request, void *param)
{
    return 1;
}



uint16_t write_esam(uint32_t addr, uint8_t *buf, uint16_t len)
{
  
}


uint8_t read_esam_no(uint8_t *esam_info)
{
    int fd = 0;
    int request = esam_type_get_esam_info;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    
    output.data = esam_info;
    param.input  = &input;
    param.output = &output;
 
    return openM_ioctl(fd, request, (void *)&param);
}



uint8_t read_esam_data(uint32_t addr, uint8_t *buf, uint16_t len)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    //param.input = p_input;
    //param.output = p_output;

    return openM_ioctl(fd, request, (void *)&param);
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

uint8_t esam_consult_key(uint8_t cmd, uint8_t *p)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    
    input.cmd_type = cmd;
    input.data     = p;
    input.data_len = 32;
    input.MAC_data = p + 32 + 1;
    input.MAC_len  = 4;

    return openM_ioctl(fd, request, (void *)&param);
}




uint8_t read_esam_info(uint8_t *esam_info, uint8_t index)
{
    int fd = 0;
    int request = esam_type_get_esam_info;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    
    input.cmd_type = index;
    output.data = esam_info;
    param.input  = &input;
    param.output = &output;
 
    return openM_ioctl(fd, request, (void *)&param);
}


uint8_t get_key_status(void)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    
	uint8_t Buf[16];
    uint32_t keystate=0;
	
	read_esam_info(Buf,0X04);//读密钥状态
    memcpy((uint8_t*)&keystate, (uint8_t*)&Buf[0], 4);

	if( (keystate >= 0x7FFFFFFF) && (Buf[4] >= 0xF0) ) 
        return false; //修改35条密钥以上，为私钥状态
	else 
        return true;
}



int fr_698_handle_esam(struct esam_698_input_t *p_input, struct esam_698_output_t *p_output)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    param.input = p_input;
    param.output = p_output;
    
    if(4 == p_input->cmd_type)//获取mac
    {
        p_output->MAC_len = 4;
    }
    return 1;
    return openM_ioctl(fd, request, (void *)&param);
}
