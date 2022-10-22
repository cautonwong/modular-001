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

extern int esam_pro_handle(struct instance_t* instance, int fd, int request, void *param);


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
    esam_pro_handle(instance, fd, request, (void *)&param);
    
    if(output.esam_err_no == 0x9000){
        return output.data_len;
    }
    
    return 0;
}

int write_esam(struct instance_t* instance, uint32_t addr, uint8_t *buf, uint16_t len)
{
    int fd = 0;
    int request = esam_type_write_cmd;
    int ret;
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
    ret = esam_pro_handle(instance, fd, request, (void *)&param);

    if(ret < 0)
    {
        return ret;
    }

    if(output.esam_err_no == 0x9000){
        return output.data_len;
    }
    return -1;
}

int esam_key_updata(struct instance_t* instance, uint8_t *buf, uint16_t len)
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
    esam_pro_handle(instance, fd, request, (void *)&param);

    if(output.esam_err_no == 0x9000){
        return offset;
    }
    return -1;
}

int read_esam_no(struct instance_t* instance, uint8_t *esam_info)
{
    int fd = 0;
    int request = esam_type_read_esam_no_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    struct mbase_control_block_t* mbase_cb;
    uint8_t tmpbuf[64] = {0};
    int ret;

    input.data_buf      = tmpbuf; //临时缓冲区
    input.data_buf_len  = 64;            //临时缓冲长度
    //input.data_buf      = instance->apdu_cache; //临时缓冲区
    //input.data_buf_len  = cache_max;            //临时缓冲长度

    output.data  = esam_info;
    
    param.input  = (struct esam_698_input_t *)&input;
    param.output = (struct esam_698_output_t *)&output;
    ret = esam_pro_handle(instance, fd, request, (void *)&param);

    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    mbase_cb->esam_chip_abnormal = output.esam_chip_abnormal;

    if (ret < 0)
    {
        return ret;
    }
    return output.data_len;
}

int read_esam_data(struct instance_t* instance, uint32_t addr, uint8_t *buf, uint16_t len)
{
    int fd = 0;
    int request = esam_type_read_esam_data_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    struct mbase_control_block_t* mbase_cb;
    uint8_t tmpbuf[164] = {0};
    int ret = 0;
    
    input.data      = buf;
    input.data_len  = len;
    input.sid_id    = addr;

    input.data_buf      = tmpbuf; //临时缓冲区
    input.data_buf_len  = 164;            //临时缓冲长度
    //input.data_buf      = instance->apdu_cache; //临时缓冲区
    //input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.data = buf;
    
    param.input  = (struct esam_698_input_t *)&input;
    param.output = (struct esam_698_output_t *)&output;
    ret = esam_pro_handle(instance, fd, request, (void *)&param);
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    mbase_cb->esam_chip_abnormal = output.esam_chip_abnormal;

    if (ret < 0)
    {
        return ret;
    }
    
    return output.data_len;
}


int esam_consult_key(struct instance_t* instance, uint8_t cmd, uint8_t *p)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    int ret = 0;
    
    request = esam_type_meet_consult_cmd;
    input.cmd_type = cmd;
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
    ret = esam_pro_handle(instance, fd, request, (void *)&param);
    if (ret < 0) //错误
    {
        return ret;
    }
    
    return (output.data_len + output.MAC_len) ;
}


int read_esam_info(struct instance_t* instance, uint8_t *esam_info, uint8_t index)
{
    int fd = 0;
    int request = esam_type_get_esam_p2_info_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
    struct mbase_control_block_t* mbase_cb;
    uint8_t tmpbuf[64] = {0};
    int ret;
    
    input.cmd_type = index;
    output.data    = esam_info;

    input.data_buf      = tmpbuf; //临时缓冲区
    input.data_buf_len  = 64;            //临时缓冲长度
    //input.data_buf      = instance->apdu_cache; //临时缓冲区
    //input.data_buf_len  = cache_max;            //临时缓冲长度

    param.input  = (struct esam_698_input_t *)&input;
    param.output = (struct esam_698_output_t *)&output;    
    ret = esam_pro_handle(instance, fd, request, (void *)&param);

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    mbase_cb->esam_chip_abnormal = output.esam_chip_abnormal;

    if(ret < 0)
    {
        return ret;
    }

    return output.data_len;
   
}

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


void esam_698_init(struct instance_t* instance, uint8_t power_mode)
{
    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;
    
    if(power_mode == AC_POWER_ON)
    {
        request = esam_type_poweron_cmd;
        esam_pro_handle(instance, fd, request, (void *)&param);
    }
}



void esam_698_power_on(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t esam_inf[45] = {0}; 
    uint32_t addr = 0;
    int ret = 0;

    mbase_cb->key_state = get_key_status(instance);//获取秘钥状态   

    ret = read_esam_info(instance, esam_inf,8); 
    if (ret < 0)
    {
        esam_inf[0] = 0;
    }
    mbase_cb->AuthOn = esam_inf[0];

    
    memset(esam_inf, 0, sizeof(esam_inf)); 
    ret = read_esam_no(instance, esam_inf);   // 获取表号   //ndv_memcopy(p, esam_inf+7, 6);
    if (ret > 0)
    {        
        if (mem_compare(mbase_cb->meterNo, &esam_inf[7], 6)) //优先使用 esam内部表号  
        {
            ndv_memcopy(mbase_cb->meterNo, &esam_inf[7], 6);  
            addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
            offsetof(struct mbase_control_block_t, meterNo));
            nd_set_offset(instance, addr, mbase_cb->meterNo, 6);     
        }
    }
}

//esam 电源控制(关闭)   秒调用 
void esam_698_power_supply_ctrl(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    int fd = 0;
    int request = 0;
    struct esam_698_data_t param;


    if (mbase_cb->esam_power_status == 0) //电源未开启
    {
        return;
    }
    
    if ((fr_698_current_connect_status_get(instance, pipeline, 0)== true) 
        || (fr_698_current_connect_status_get(instance, pipeline, 1)== true) 
        || (mbase_cb->term_auth_timeout != 0) || (mbase_cb->infrared_auth_timeout != 0)
        || (mbase_cb->esam_power_close_dly != 0))
    {
        return;
    }


    //关闭电源
    request = esam_type_poweroff_cmd;
    esam_pro_handle(instance, fd, request, (void *)&param);
    mbase_cb->esam_power_status = 0;
    //mbase_cb->esam_power_close_dly = 0;
}

int esam_self_check(struct instance_t* instance, struct pipeline_t* pipeline)
{
    uint8_t result = 0, temp[42] = {0};


    result = read_esam_no(instance, temp);
    
    if (result > 0)
    {
        return 1;
    }
    else
    {
        return -1;
    }
    
}


int fr_698_handle_esam(struct instance_t* instance, struct esam_698_input_t *p_input, struct esam_698_output_t *p_output)
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
    return esam_pro_handle(instance, fd, request, (void *)&param);
}

//主动上报  
uint8_t get_upreport_esam_rn_mac(struct instance_t* instance, uint8_t *apdu, uint8_t apdu_len , uint8_t *dst)
{
    int fd = 0;
    int request = esam_type_upreport_cmd;
    struct esam_698_data_t param;
    struct esam_698_input_t  input = {0};
    struct esam_698_output_t output= {0};
 
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    input.data = apdu;
    input.data_len = apdu_len;
    param.input = &input;
    
    output.data = dst;
    param.output = &output;
    
    return esam_pro_handle(instance, fd, request, (void *)&param);
}
//主动上报验证
int check_upreport_esam_rn_mac(struct instance_t* instance, struct esam_698_input_t *p_input, struct esam_698_output_t *p_output)
{
    int fd = 0;
    int request = esam_type_upreportcheck_cmd;
    struct esam_698_data_t param;
    //struct esam_698_input_t  input = {0};
    //struct esam_698_output_t output= {0};

    param.input = p_input;
    param.output = p_output;

    return esam_pro_handle(instance, fd, request, (void *)&param);
}


//esam模块上电期间 秒处理. 电源控制 和  esam自检(间隔15分，每02、17、32、47分的第35秒 )。
void esam_sec_pro(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t s_addr;
    
    if (mbase_cb->esam_power_close_dly > 0)   
    {
        mbase_cb->esam_power_close_dly--;
    }

    esam_698_power_supply_ctrl(instance, pipeline);
    
    if ((instance->time.tm_sec == 35) && ((instance->time.tm_min%15) == 2)) 
    {
        esam_self_check(instance, pipeline);
    }

}


