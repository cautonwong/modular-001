#ifndef __FR_698_ESAM_H__
#define __FR_698_ESAM_H__

#include <stdint.h>

struct instance_t;
struct pipeline_t;

/* fr_698.h leaves #pragma pack(1) active. Pin the ESAM wire structs so a
 * translation unit that includes fr_698.h first and one that does not still
 * agree; on ARM32 the packed and default layouts are identical. */
#pragma pack(push, 1)

struct esam_698_input_t
{
    uint8_t cmd_type;           //区分请求类型 
    uint8_t security_choice;    //明文 密文
    uint8_t verification_choice;//验证方式选择
    uint8_t MAC_len;
    
    uint8_t *data;              //明文/密文/协商...
    uint16_t data_len;
    uint16_t AttachData_len;

    uint32_t sid_id;
    uint8_t *AttachData;
    uint8_t *MAC_data;
    uint8_t *RN_Data;
    uint8_t *Dispersion_factor;
    uint8_t *data_buf;      //临时缓冲区
    uint16_t RN_Data_len;
    uint16_t data_buf_len;  //临时缓冲长度
    uint16_t Dispersion_factor_len;
    uint8_t from_type;
};

struct esam_698_output_t
{
    int     esam_err_no;        //返回esam  负错误码 
    uint8_t security_choice;
    uint8_t verification_choice;
    uint16_t data_len;
    uint8_t *data;
    uint8_t *MAC_data;
    uint8_t MAC_len;
    uint8_t esam_chip_abnormal; //硬件故障
};

struct esam_698_security_t
{
    uint8_t verification_choice;    
    uint8_t security_choice;
    uint8_t esam_req_cmd;
    uint8_t esam_sub_cmd;
};

struct esam_698_data_t
{
    struct esam_698_input_t  *input;
    struct esam_698_output_t *output;

};

struct esam_data_t
{
    struct esam_698_input_t  *input;
    struct esam_698_output_t *output;
    uint8_t *head;
    uint16_t len;
    uint16_t sw;
};

#pragma pack(pop)

int fr_698_handle_esam(struct instance_t* instance, struct esam_698_input_t *p_input, struct esam_698_output_t *p_output);

int read_esam_data(struct instance_t* instance, uint32_t addr, uint8_t *buf, uint16_t len);
int write_esam(struct instance_t* instance, uint32_t addr, uint8_t *buf, uint16_t len);
int read_esam_no(struct instance_t* instance, uint8_t *esam_info);
int read_esam_info(struct instance_t* instance, uint8_t *esam_info, uint8_t index);
int esam_consult_key(struct instance_t* instance, uint8_t cmd, uint8_t *p);
uint16_t software_esam_check(struct instance_t* instance, uint8_t *key, uint8_t *rn, uint8_t *data, uint8_t p2, uint16_t datalen, uint8_t *outbuf);
int esam_key_updata(struct instance_t* instance, uint8_t *buf, uint16_t len);
void esam_698_init(struct instance_t* instance, uint8_t power_mode);
uint8_t get_key_status(struct instance_t* instance);
uint32_t fr_698_current_esam_timeout_get(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t client_index);
int security_mode_word_check(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t cmd, uint32_t oad);
#endif //__FR_698_ESAM_H__