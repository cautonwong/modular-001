#include "nidavellir.h"
#include "645id_table.h"

int read_02( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
	unsigned long StAddr,tmp_u32=0;
    unsigned char len;
    uint8_t tmpbuf[10] = {0};
    uint32_t s_index,temp=0;

	switch( ID )
	{   
        case 0x02010100:    //电压
            WORD_BCD2(p_tx, (uint16_t)mbase_cb->voltage_ac);
            return 2;
        case 0x0201FF00:    //电压数据块
            WORD_BCD2(p_tx, (uint16_t)mbase_cb->voltage_ac);
            memset(p_tx+2, 0xFF, 4);
            return 6;

        case 0x02020100:    //电流
        case 0x0202FF00:    //电流数据块
            if(mbase_cb->current_l < 0)
            {
                temp = ~mbase_cb->current_l + 1;
                HEX_BCD(p_tx, temp);
                *(p_tx+2) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, mbase_cb->current_l);
            }
            if(ID == 0x0202FF00)
            {
                memset(p_tx+3, 0xFF, 6);
                return 9;
            }
            else
                return 3;
            break;
            
        case 0x02030000:    //总有功功率 
        case 0x02030100:   
        case 0x0203FF00:    //有功功率数据块
            if(mbase_cb->active_power < 0)
            {
                temp = ~mbase_cb->active_power + 1;
                HEX_BCD(p_tx, temp);
                *(p_tx+2) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, mbase_cb->active_power);
            }
            if(ID == 0x0203FF00)
            {
                memcpy(p_tx+3, p_tx, 3);
                memset(p_tx+6, 0xFF, 6);
                return 12;
            }
            else
                return 3;
            break;
            
        case 0x02050000:    //总视在功率 
        case 0x02050100:   
        case 0x0205FF00:    //视在功率数据块
            HEX_BCD(p_tx, mbase_cb->apparent_power);
            if(ID == 0x0205FF00)
            {
                memcpy(p_tx+3, p_tx, 3);
                memset(p_tx+6, 0xFF, 6);
                return 12;
            }
            else
                return 3;
            break; 
            
        case 0x02060000:    //功率因数
        case 0x02060100:   
        case 0x0206FF00:    //功率因数数据块
            if(mbase_cb->power_factor < 0)
            {
                temp = ~mbase_cb->power_factor + 1;
                WORD_BCD2(p_tx, temp);
                *(p_tx+1) |= 0x80;
            }
            else
            {
                WORD_BCD2(p_tx, mbase_cb->power_factor);
            }
            if(ID == 0x0206FF00)
            {
                memcpy(p_tx+2, p_tx, 2);
                memset(p_tx+4, 0xFF, 4);
                return 8;
            }
            else
                return 2;
            break;
        case 0x02800001:    //零线电流
            if(mbase_cb->current_n < 0)
            {
                temp = ~mbase_cb->current_n + 1;
                HEX_BCD(p_tx, temp);
                *(p_tx+2) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, mbase_cb->current_n);
            }
            return 3;
        case 0x02800002:  //频率
            HEX_BCD(p_tx, mbase_cb->frequency);
            return 2;
            
        case 0x02800007:  //表内温度
            if(mbase_cb->temperature < 0)
            {
                temp = ~mbase_cb->temperature+1;
                HEX_BCD(p_tx, temp);
                *(p_tx+1) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, mbase_cb->temperature);
            }
            return 2;
            
        case 0x02800008:  //电池电压
            HEX_BCD(p_tx, mbase_cb->voltage_battery);
            return 2;  
            
        case 0x0280000A:  //电池使用时间
            StAddr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Bat_UseTime);
            nd_get_index(instance, StAddr, (uint8_t*)&tmp_u32, 4); 
            tmp_u32 = tmp_u32 / 60;
            HEX_BCD(p_tx, tmp_u32);
            //memcpy(p_tx, (unsigned char*)&tmp_u32, 4);
            return 4;    
            
        case 0x0280000B:  //当前阶梯电价
            //HEX_BCD(p_tx, mbase_cb->voltage_battery);
            return 4;   
            
        case 0x02800020:  //当前电价
            //HEX_BCD(p_tx, mbase_cb->voltage_battery);
            return 4; 
            
        case 0x02800021:  //当前费率电价
            //HEX_BCD(p_tx, mbase_cb->voltage_battery);
            return 4; 
        case 0x02800022:  //
            temp = ((mbase_cb->auth_timeout_645 +59)/60);
            if (temp > 9999)
            {
                temp = 9999;
            }
            temp = shorthex_to_bcd((uint16_t)temp);
            *p_tx = (uint8_t)temp;
            *(p_tx+1) =(uint8_t)(temp>>8);

            return 2;  
        case 0x02800023:  // 698共用
            temp = ((mbase_cb->infrared_auth_timeout +59)/60);
            if (temp > 99)
            {
                temp = 99;
            }
            temp = bytehex_to_bcd((uint8_t)temp);
            *p_tx = (uint8_t)temp;

            return 1;             
        default:
            *(p_tx-4) = RS_ERR_ID;
            return -1;
    }
}

