#ifndef __FR_698_ESAM_H__
#define __FR_698_ESAM_H__

struct esam_698_input_t
{
    uint8_t cmd_type;           //区分请求类型 
    uint8_t security_choice;    //明文 密文
    uint8_t verification_choice;//验证方式选择
    uint8_t *data;              //明文/密文/协商...
    uint16_t data_len;
    uint32_t sid_id;
    uint8_t *AttachData;
    uint16_t AttachData_len;
    uint8_t *MAC_data;
    uint8_t MAC_len;
    uint8_t *RN_Data;
    uint16_t RN_Data_len;
    
    uint8_t *data_buf;      //临时缓冲区
    uint16_t data_buf_len;  //临时缓冲长度
};

struct esam_698_output_t
{
    int     esam_err_no;        //返回esam  负错误码 
    uint8_t security_choice;
    uint8_t verification_choice;
    uint8_t *data;
    uint16_t data_len;
    uint8_t *MAC_data;
    uint8_t MAC_len;
};

struct esam_698_data_t
{
    struct esam_698_input_t  *input;
    struct esam_698_output_t *output;
};

int fr_698_handle_esam(struct esam_698_input_t *p_input, struct esam_698_output_t *p_output);

#endif //__FR_698_ESAM_H__