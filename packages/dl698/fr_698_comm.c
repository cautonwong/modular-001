#include "nidavellir.h"
#include "applications.h"
#include "fr_axdr.h"
#include "fr_698.h"


#ifdef WIN32_U_TEST

int __nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return resource_map_698(instance, id, buf, buf_size,ctx, RESOURCE_MAP_GET);
}

int __nd_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return resource_map_698(instance, id, buf, buf_size, ctx, RESOURCE_MAP_SET);
}

int __nd_action(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size, void* ctx)
{
    return resource_map_698(instance, id, buf, buf_size, ctx, RESOURCE_MAP_ACTION);
}

int16_t nd_get_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size)
{
    return 0;
}

int16_t nd_set_index(struct instance_t* instance, uint32_t index, uint8_t* buf, uint16_t buf_size)
{
    return 0;
}

#endif

#ifdef TX_APDU_EN
void apdu_send(uint8_t type, uint8_t head, uint8_t *apdu, uint16_t len)
{
    struct fifo_t* tx_fifo ;
    if(type == UART_MODULE_CHL){
        extern struct pipeline_t pl_plc_tx_698;
        tx_fifo = (struct fifo_t*)pl_plc_tx_698.out_queue;
    }else{
        return;
    }
    if(head == 0x55)
        ringbuffer_clear(tx_fifo);
    fifoBuf_putByte(tx_fifo, head);
    fifoBuf_putData(tx_fifo, apdu, len);
}
#endif


const uint8_t month_tab[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

uint8_t _date_time_check(uint16_t val, uint16_t min, uint16_t max, uint8_t type)
{
    uint16_t data = 0xFF;
    if ((val >= min) && (val <= max)){
        return true;
    }
    if (min == 2000) {
        data = 0xFFFF;
    }
    if ((val == data) && (type & TIME_WILDCARD)){
        return true;
    }
    return false;
}

uint8_t _time_day_check(uint16_t val, uint16_t month, uint16_t year, uint8_t type)
{
    uint8_t mday = 0;
    mday = month_tab[month - 1];
    if ((mday == 28) && ((year % 4) == 0)){
        mday++;
    }
    if ((val > 0) && (val <= mday)){
        return true;
    }
    if ((val == 0xff) && (type & TIME_WILDCARD)) {
        return true;
    }
    return false;
}

uint8_t dl_data_time_check(uint8_t* date_time, uint8_t type)
{
    uint16_t year = 0;
    year = ((uint16_t)date_time[0] << 8) + date_time[1];

    if (false == _date_time_check(year, 2000, 2099, type)) {
        return false;
    }
    if (false == _date_time_check(date_time[2], 1, 12, type)) {
        return false;
    }
    if (false == _time_day_check(date_time[3], date_time[2], year, type)) {
        return false;
    }

    if (false == _date_time_check(date_time[4], 0, 24, type)) {
        return false;
    }
    if (false == _date_time_check(date_time[5], 0, 60, type)) {
        return false;
    }
    if (false == _date_time_check(date_time[6], 0, 60, type)) {
        return false;
    }
    return true;
}

uint32_t dl_timeTag_interval(uint8_t* date, uint8_t type)
{
    uint8_t  unit = date[0];
    uint32_t interval = ((uint16_t)date[1] << 8) + date[2];

    if (unit == 0){
    }
    else if (unit == 1){
        interval *= 60;
    }
    else if (unit == 2){
        interval *= 3600;
    }
    else if (unit == 3){
        interval *= 86400;
    }
    else if (unit == 4){
        interval *= 86400ul * 30;
    }
    else if (unit == 5){
        interval *= 86400ul * 365;
    }
    else{
        interval = 0;
    }
    return interval;
}


uint32_t dl_datetime_to_second(uint8_t* date, uint8_t type)
{
    struct tm time = { 0 };
    uint16_t year = ((uint16_t)date[0] << 8) + date[1];
    time.tm_sec  = date[6];
    time.tm_min  = date[5];
    time.tm_hour = date[4];
    time.tm_mday = date[3];
    time.tm_mon  = date[2] - 1;
    time.tm_year = year - 1900;
    return rtc_mktime(&time);
}


int check_oad(uint32_t oad)
{
    uint16_t val = 0xF000&(oad>>16);
    if((val >= 0x8000) && (0xF000 != val)){
        return -DAR_OTHERS_ERROR;
    }
    return 0;
}
