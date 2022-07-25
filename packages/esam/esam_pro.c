#include "type_def.h"
#include "nidavellir.h"
#include "cpu_surport.h"
#include "time_link.h"
#include "pin_function_def.h"
#include "fr_698_esam.h"
#include "io_type.h"

extern int openM_ioctl(int fd, int request, void *param);

enum verification_choice_e
{
   verification_choice_SID_MAC,
   verification_choice_RN,
   verification_choice_RN_MAC,
   verification_choice_SID,
};

enum sec
{
    SECURITY_CHOICE_0_Plaintext  = 0,    //明文数据单元
    SECURITY_CHOICE_1_Ciphertext = 1,    //密文数据单元
};



static int __comm_esam_handle(uint8_t *head, struct esam_698_data_t *para, uint8_t type, void *esam_hal)
{
    struct esam_698_output_t *esam_out = para->output;
    struct esam_698_input_t *esam_in = para->input;
    struct esam_data_t esam_data = {0};
    
    uint16_t len = 0;
    uint16_t sw = 0;
	
    if(esam_in->Dispersion_factor_len){
        memcpy(esam_in->data_buf , esam_in->Dispersion_factor , esam_in->Dispersion_factor_len);
        len += esam_in->Dispersion_factor_len;
    }
    
    if(esam_in->RN_Data_len){
        memcpy(esam_in->data_buf , esam_in->RN_Data , esam_in->RN_Data_len);
        len += esam_in->RN_Data_len;
    }

    if(esam_in->AttachData_len > 2){
        memcpy(esam_in->data_buf , esam_in->AttachData + 2 , esam_in->AttachData_len - 2);
        len = esam_in->AttachData_len - 2;
    }

    if(esam_in->data_len){
        memcpy(&esam_in->data_buf[len] , esam_in->data , esam_in->data_len);
        len += esam_in->data_len;
    }
    
    if(esam_in->MAC_len){
        memcpy(&esam_in->data_buf[len] , esam_in->MAC_data , esam_in->MAC_len);
        len += esam_in->MAC_len;
    }
    
    if(esam_in->AttachData_len >= 2){
        len = (esam_in->AttachData[0]<<8)+esam_in->AttachData[1];
    }

    esam_data.input  = esam_in;
    esam_data.output = esam_out;
    esam_data.head   = head;
    esam_data.len    = len;
    esam_data.sw     = 0;
    int ret = openM_ioctl(0, esam_comm_handle_cmd, &esam_data);
    sw = esam_data.sw;
    len = esam_data.len; 
   
    if(sw == 0x9000){
        if(type){
            memcpy(esam_out->data , esam_in->data_buf , len);
        }
        esam_out->data_len = len;
        esam_out->esam_err_no = 0x9000;
        return 0;
    }else{
        esam_out->esam_err_no = sw;
        return -1;
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
static int f_esam_type_get_esam_all_info1(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input;
    uint8_t head[4] = {0x80, 0x36, 0x00, 0xff};
    
    esam_in->AttachData_len = 0;
    esam_in->data_len = 0;
    esam_in->MAC_len = 0;
      
    return  __comm_esam_handle(head, para, 1, esam_hal);
}

static uint8_t f_esam_type_get_esam_all_info2(struct esam_698_data_t *para , void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input;
    uint8_t head[4] = {0x80, 0x36, 0x00, 0xfe};
    esam_in->AttachData_len = 0;
    esam_in->data_len = 0;
    esam_in->MAC_len = 0;
      
    return  __comm_esam_handle(head, para, 1, esam_hal);
}

static uint8_t f_esam_type_broadcast(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input;    
    uint8_t head[4];
    	
    memcpy(head , (uint8_t *)&esam_in->sid_id , 4);
    
    //esam_in->AttachData_len = 0;
    //esam_in->data_len = 0;
    //esam_in->MAC_len = 0;
      
    return  __comm_esam_handle(head, para, 1, esam_hal);
}

static uint8_t f_esam_type_point_get_data2(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input;    
    uint8_t head[4];
	
    memcpy(head , (uint8_t *)&esam_in->sid_id , 4);
    
    //esam_in->AttachData_len = 0;
    //esam_in->data_len = 0;
    if(esam_in->cmd_type == 1)//数据验证信息为SID_MAC
    {
    }
    else{
        esam_in->MAC_len = 0;
    }
      
    return  __comm_esam_handle(head, para, 1, esam_hal);
}




//////////////////////////////////////////////////////////////////////////////////
static int f_esam_698_power_ctrl_on(struct esam_698_data_t *para, void *esam_hal)
{
    return openM_ioctl(0, esam_poweron_cmd, NULL);
}

static int f_esam_698_power_ctrl_off(struct esam_698_data_t *para, void *esam_hal)
{
    return openM_ioctl(0, esam_poweroff_cmd, NULL);
}

static int f_esam_type_read_esam_no(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input; 
    uint8_t head[4] = {0x80, 0x2C, 0x00, 0x01};
    uint8_t tx_data[7] = { 0, 5, 0, 0, 0, 0, 0x0D };

    esam_in->RN_Data_len = 0;
    esam_in->data_len = 7;
    esam_in->Dispersion_factor_len = 0;
    
    esam_in->data = tx_data;

    return  __comm_esam_handle(head, para, 7, esam_hal);
}

static int f_esam_type_read_esam_data(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input; 
    uint8_t head[4];
    
    memcpy(head , (uint8_t *)&esam_in->sid_id , 4);

    esam_in->RN_Data_len = 0;
//    esam_in->data_len = 0;
    esam_in->MAC_len = 0;
    esam_in->Dispersion_factor_len = 0;

    return  __comm_esam_handle(head, para, 1, esam_hal);
}

static int f_esam_type_meet_consult(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_output_t *esam_out = para->output;
    struct esam_698_input_t *esam_in = para->input;
    uint8_t head[4] = {0x81, 0x02, 0x00, 0x00};
    
    if(esam_in->cmd_type == 1)//终端会话
    {
        head[0] = 0x82;
    }
    
    esam_in->RN_Data_len = 0;
    esam_in->AttachData_len = 0;
    esam_in->data_len = 32;
    esam_in->MAC_len = 4;
    esam_in->Dispersion_factor_len = 0;
      
    if(0 == __comm_esam_handle(head, para, 0, esam_hal)){
        if(esam_in->cmd_type == 1){//终端会话
            esam_out->data_len = 64;
        }else{
            esam_out->data_len = 48;
        }
    
        memcpy(esam_out->data, esam_in->data_buf , esam_out->data_len);
        esam_out->MAC_len = 4;
        memcpy(esam_out->MAC_data, esam_in->data_buf+esam_out->data_len , esam_out->MAC_len);
        return 0;
    }
    return -1;
}

static int f_esam_type_get_esam_p2_info2(struct esam_698_data_t *para , void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input;
    
    uint8_t head[4] = {0x80, 0x36, 0x00, 0x00};    
    head[3] = esam_in->cmd_type;
    
    esam_in->RN_Data_len = 0;
    esam_in->AttachData_len = 0;
    esam_in->data_len = 0;
    esam_in->MAC_len = 0;
    esam_in->Dispersion_factor_len = 0;
      
    return  __comm_esam_handle(head, para, 1, esam_hal);
}


static  int f_esam_type_ver_rn_mac(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_output_t *esam_out = para->output;
    struct esam_698_input_t *esam_in = para->input;    
    uint8_t head[4] = {0x80, 0x0E, 0x40, 0x02};
              
    if(esam_in->cmd_type == 1){
        head[1] = 0x0E;  //电表计算明文+MAC
    }
    else if(esam_in->cmd_type == 2){
        head[1] = 0x0A;  //电表计算密文
    }
    else if(esam_in->cmd_type == 3){
        head[1] = 0x10;  //电表计算密文+MAC
    }else{
        return -1;
    }

    esam_in->AttachData_len = 0;
    esam_in->MAC_len = 0;
    esam_in->Dispersion_factor_len = 0;
      
    if(0 == __comm_esam_handle(head, para, 0, esam_hal)){
        if(esam_in->cmd_type == 1){
            memcpy(esam_out->MAC_data, esam_in->data_buf , 4); //电表计算明文+MAC
            esam_out->MAC_len = 4;
        }
        else if(esam_in->cmd_type == 2)
        {
            memcpy(esam_out->data, esam_in->data_buf , esam_out->data_len); //电表计算密文
        }
        else
        {
            esam_out->data_len = esam_out->data_len - 4;
            memcpy(esam_out->data, esam_in->data_buf , esam_out->data_len); //电表计算密文
            memcpy(esam_out->MAC_data, &esam_in->data_buf[esam_out->data_len] , 4); //电表计算密文+MAC
            esam_out->MAC_len = 4;
        }
        return 0;
    }
    return -1;
}

static  int f_esam_type_ver_sid_mac(struct esam_698_data_t *para, void *esam_hal)
{
//    struct esam_698_output_t *esam_out = para->output;
    struct esam_698_input_t *esam_in = para->input;    
    uint8_t head[4] = {0};
         
    mem_swap_cpy(head, (uint8_t *)&esam_in->sid_id , 4);
    esam_in->RN_Data_len = 0;
    esam_in->Dispersion_factor_len = 0;
      
    if(0 == __comm_esam_handle(head, para, 1, esam_hal)){
        return 0;
    }
    return -1;
}

static int f_esam_type_sid_mac(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input; 
    struct esam_698_output_t *esam_out = para->output;
    uint8_t head[4] = {0x81,0x1c,0x00,0x00};
    
    if(1 == esam_in->cmd_type){
        head[3] = 0x11;
    }
    else if(2 == esam_in->cmd_type){
        head[3] = 0xA6;
    }
    else if(3 == esam_in->cmd_type){
        head[3] = 0xA7;
    }else{
        return -1;
    }
    
    esam_in->RN_Data_len = 0;
    esam_in->AttachData_len = 0;
    esam_in->MAC_len = 0;
    esam_in->Dispersion_factor_len = 0;
    
    if(0 == __comm_esam_handle(head, para, 0, esam_hal)){
        if(2 == esam_in->cmd_type){
            memcpy(esam_out->data, esam_in->data_buf , esam_out->data_len); 
            esam_out->MAC_len = 0;
            return 0;
        }
        if((3 == esam_in->cmd_type) || (1 == esam_in->cmd_type)){
            memcpy(esam_out->data, esam_in->data_buf , esam_out->data_len); 
            esam_out->MAC_len = 4;
            esam_out->data_len = esam_out->data_len - esam_out->MAC_len;
            memcpy(esam_out->MAC_data, &esam_in->data_buf[esam_out->data_len] , esam_out->MAC_len);
            return 0;
        }
    }
    return -1;
}


static  int f_esam_type_write(struct esam_698_data_t *para, void *esam_hal)
{
//    struct esam_698_output_t *esam_out = para->output;
    struct esam_698_input_t *esam_in = para->input;    
    uint8_t head[4] = {0};
         
    mem_swap_cpy(head, (uint8_t *)&esam_in->sid_id , 4);
    esam_in->RN_Data_len = 0;
    esam_in->Dispersion_factor_len = 0;
      
    if(0 == __comm_esam_handle(head, para, 1, esam_hal)){
        return 0;
    }
    return -1;
}

static  int f_esam_type_software_check(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input; 
//    struct esam_698_output_t *esam_out = para->output;
    uint8_t head[4] = {0x80,0x0A,0x48,0x00};
    
    if((esam_in->cmd_type > 0x08) && (esam_in->cmd_type < 0x0E))
    {
        head[3] = esam_in->cmd_type;
    }
    else{
        return -1;
    }

    esam_in->AttachData_len = 0;
    esam_in->MAC_len = 0;
    if(0 == __comm_esam_handle(head, para, 1, esam_hal)){
        return 0;
    }
    return -1;
}


static  int f_esam_type_upreport(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input; 
    struct esam_698_output_t *esam_out = para->output;
    uint8_t head[4] = {0x80,0x14,0x01,0x03};

    esam_in->Dispersion_factor_len = 0;
    esam_in->RN_Data_len = 0;
    esam_in->AttachData_len = 0;
    esam_in->MAC_len = 0;
    
    if(0 == __comm_esam_handle(head, para, 0, esam_hal)){
        esam_out->data_len = 16;
        memcpy(esam_out->data, esam_in->data_buf , esam_out->data_len); 
        return 0;
    }
    return -1;
}



static  int f_esam_type_upreport_check(struct esam_698_data_t *para, void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input; 
//    struct esam_698_output_t *esam_out = para->output;
    uint8_t head[4] = {0x80,0x0e,0x40,0x81};

    esam_in->Dispersion_factor_len = 0;
    esam_in->RN_Data_len = 0;
    esam_in->AttachData_len = 0;
    esam_in->MAC_len = 0;
    
    if(0 == __comm_esam_handle(head, para, 0, esam_hal)){
        return 0;
    }
    return -1;
}


typedef int(*pFunc)(struct esam_698_data_t *para, void *esam_hal);


struct Func_esam_698_t
{
    int(*pFunc)(struct esam_698_data_t *para, void *esam_hal);
};


static const struct Func_esam_698_t Func_esam[] = 
{
    {f_esam_698_power_ctrl_on},
    {f_esam_698_power_ctrl_off},
    {f_esam_type_read_esam_no},
    {f_esam_type_read_esam_data},
    {f_esam_type_meet_consult},
    {f_esam_type_get_esam_p2_info2},
    {f_esam_type_ver_rn_mac},
    {f_esam_type_ver_sid_mac},
    {f_esam_type_sid_mac}, 
    {f_esam_type_write},
    {f_esam_type_software_check},
    {f_esam_type_upreport},
    {f_esam_type_upreport_check}
};


int esam_pro_handle(int fd, int request, void *param)
{
    if((request < esam_type_max_cmd) && (request >= esam_type_base)){
        return Func_esam[request - esam_type_base].pFunc((struct esam_698_data_t *)param, 0);
    }
    return -1;
}

