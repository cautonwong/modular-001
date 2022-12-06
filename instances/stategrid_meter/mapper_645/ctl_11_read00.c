#include "nidavellir.h"
#include "645id_table.h"

int read_00( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx)
{
    uint8_t IDL0, IDL1, data[100]={0,}, len = 0, i, fv=0;
    uint16_t IDH=0;
    uint32_t oad698=0, temp=0, s_index=0;
    
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    IDL0 = ID & 0x000000FF; //当前or历史
    IDL1 = (uint8_t)((ID & 0x0000FF00) >> 8);   //数据块or总费率
    IDH = (uint16_t)((ID & 0xFFFF0000) >> 16);
    
    switch(IDH)
    {
        case 0x0000:    //组合
            oad698 = 0x00000200;
            break;
        case 0x0001:    //正向
            oad698 = 0x00100200;
            break;
        case 0x0002:    //反向
            oad698 = 0x00200200;
            break;
        case 0x000C:    //月度用电量
            oad698 = 0x20310200;
            break;
        case 0x000B:    //结算周期用电量
            oad698 = 0x20310200;
            break;    
        default:
            return -1;
            break;
    }
    if(IDL1 == 0xFF)    //数据块
    {
        oad698 = oad698 & 0xFFFFFF00;
    }
    else
    {
        if((IDH == 0x0000) || (IDH == 0x0001) || (IDH == 0x0002))
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para+3);//读取费率数
            nd_get_index(instance, s_index, (uint8_t *)&fv, 1);
            if(IDL1 > fv) return -1;
            oad698 = oad698 | (IDL1+1); //费率
        }
    }
    
    if(IDL0 == 0x00)//当前
    {
        if(IDL1 == 0xFF)    //数据块
        {
            nd_get(instance, oad698, data, 9, NULL);
            
            for(i=0; i<*(data+1); i++)
            {
                mem_swap_cpy((uint8_t*)&temp, data + 2 + 5*i + 1, 4);
                HEX_BCD(p_tx+i*4, temp);
                len = len + 4;
            }
        }
        else
        {
            nd_get(instance, oad698, data, 9, NULL);
            mem_swap_cpy((uint8_t*)&temp, data+1, 4);
            HEX_BCD(p_tx, temp);
            len = 4;
        }
    }
    else
    {   
        if(IDL1 == 0xFF)    //数据块
        {
            len = read_record_data_form_698_protocol(instance, data, 0x50050200, oad698, IDL0, 100);
            for(i=0; i<*(data+2); i++)
            {
                mem_swap_cpy((uint8_t*)&temp, data + 3 + 5*i + 1, 4);
                HEX_BCD(p_tx+i*4, temp);
                len = len + 4;
            }
        }
        else
        {
            len = read_record_data_form_698_protocol(instance, data, 0x50050200, oad698, IDL0, 20);
            mem_swap_cpy((uint8_t*)&temp, data+2, 4);
            HEX_BCD(p_tx, temp);
            len = 4;
        }
    }
    
    return len;
}
