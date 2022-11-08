#include "rom_api.h"
#include <stdio.h>
#include <string.h>


int eeprom_read(FLASH_ADDRESS address, uint8_t *data, int data_size){
    memcpy(data, (uint8_t *)address, data_size);
    return data_size;
}

int eeprom_write(FLASH_ADDRESS address, uint8_t *data, int data_size){
    memcpy((uint8_t *)address, data, data_size);
    return data_size;
}

int flash_read(FLASH_ADDRESS address, uint8_t *data, int data_size){
    memcpy(data, (uint8_t *)address, data_size);
    return data_size;
}

int flash_write(FLASH_ADDRESS address, uint8_t *data, int data_size){
    memcpy((uint8_t *)address, data, data_size);
    return data_size;
}
