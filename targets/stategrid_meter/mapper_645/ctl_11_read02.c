#include "targets/stategrid_meter/persistence/meter_record.h"
#include <stddef.h>
#include "targets/stategrid_meter/persistence/meter_base_view.h"
#include "645id_table.h"
#include "targets/stategrid_meter/app/resource_blocks.h"
#include "application/adapters/dl645/fr_645_env.h"

int read_02( void *instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx)
{
    const nd_resource_port_t *port = nd_fr645_resource_access(instance);
	unsigned long StAddr,tmp_u32=0;
    unsigned char len;
    uint8_t tmpbuf[10] = {0};
    uint32_t s_index,temp=0;

	switch( ID )
	{   
        case 0x02010100:    //��ѹ
            WORD_BCD2(p_tx, (uint16_t)nd_mb_voltage_ac(instance));
            return 2;
        case 0x0201FF00:    //��ѹ���ݿ�
            WORD_BCD2(p_tx, (uint16_t)nd_mb_voltage_ac(instance));
            memset(p_tx+2, 0xFF, 4);
            return 6;

        case 0x02020100:    //����
        case 0x0202FF00:    //�������ݿ�
            if(nd_mb_current_l(instance) < 0)
            {
                temp = ~nd_mb_current_l(instance) + 1;
                HEX_BCD(p_tx, temp);
                *(p_tx+2) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, nd_mb_current_l(instance));
            }
            if(ID == 0x0202FF00)
            {
                memset(p_tx+3, 0xFF, 6);
                return 9;
            }
            else
                return 3;
            break;
            
        case 0x02030000:    //���й����� 
        case 0x02030100:   
        case 0x0203FF00:    //�й��������ݿ�
            if(nd_mb_active_power(instance) < 0)
            {
                temp = ~nd_mb_active_power(instance) + 1;
                HEX_BCD(p_tx, temp);
                *(p_tx+2) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, nd_mb_active_power(instance));
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
            
        case 0x02050000:    //�����ڹ��� 
        case 0x02050100:   
        case 0x0205FF00:    //���ڹ������ݿ�
            HEX_BCD(p_tx, nd_mb_apparent_power(instance));
            if(ID == 0x0205FF00)
            {
                memcpy(p_tx+3, p_tx, 3);
                memset(p_tx+6, 0xFF, 6);
                return 12;
            }
            else
                return 3;
            break; 
            
        case 0x02060000:    //��������
        case 0x02060100:   
        case 0x0206FF00:    //�����������ݿ�
            if(nd_mb_power_factor(instance) < 0)
            {
                temp = ~nd_mb_power_factor(instance) + 1;
                WORD_BCD2(p_tx, temp);
                *(p_tx+1) |= 0x80;
            }
            else
            {
                WORD_BCD2(p_tx, nd_mb_power_factor(instance));
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
        case 0x02800001:    //���ߵ���
            if(nd_mb_current_n(instance) < 0)
            {
                temp = ~nd_mb_current_n(instance) + 1;
                HEX_BCD(p_tx, temp);
                *(p_tx+2) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, nd_mb_current_n(instance));
            }
            return 3;
        case 0x02800002:  //Ƶ��
            HEX_BCD(p_tx, nd_mb_frequency(instance));
            return 2;
            
        case 0x02800007:  //�����¶�
            if(nd_mb_temperature(instance) < 0)
            {
                temp = ~nd_mb_temperature(instance)+1;
                HEX_BCD(p_tx, temp);
                *(p_tx+1) |= 0x80;
            }
            else
            {
                HEX_BCD(p_tx, nd_mb_temperature(instance));
            }
            return 2;
            
        case 0x02800008:  //��ص�ѹ
            HEX_BCD(p_tx, nd_mb_voltage_battery(instance));
            return 2;  
            
        case 0x0280000A:  //���ʹ��ʱ��
            StAddr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Bat_UseTime);
            nd_get_index_port(port, StAddr, (uint8_t*)&tmp_u32, 4); 
            tmp_u32 = tmp_u32 / 60;
            HEX_BCD(p_tx, tmp_u32);
            //memcpy(p_tx, (unsigned char*)&tmp_u32, 4);
            return 4;    
            
        case 0x0280000B:  //��ǰ���ݵ��
            //HEX_BCD(p_tx, nd_mb_voltage_battery(instance));
            return 4;   
            
        case 0x02800020:  //��ǰ���
            //HEX_BCD(p_tx, nd_mb_voltage_battery(instance));
            return 4; 
            
        case 0x02800021:  //��ǰ���ʵ��
            //HEX_BCD(p_tx, nd_mb_voltage_battery(instance));
            return 4; 
        case 0x02800022:  //
            temp = ((nd_mb_auth_timeout_645(instance) +59)/60);
            if (temp > 9999)
            {
                temp = 9999;
            }
            temp = shorthex_to_bcd((uint16_t)temp);
            *p_tx = (uint8_t)temp;
            *(p_tx+1) =(uint8_t)(temp>>8);

            return 2;  
        case 0x02800023:  // 698����
            temp = ((nd_mb_infrared_auth_timeout(instance) +59)/60);
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

