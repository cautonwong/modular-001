#ifndef ND_IIC_PINS_H
#define ND_IIC_PINS_H

void nd_iic_eeprom_power_set(int on);
void nd_iic_sda_output(void);
void nd_iic_sda_input(void);
void nd_iic_sda_set(int high);
int nd_iic_sda_read(void);
void nd_iic_scl_output(void);
void nd_iic_scl_set(int high);

#endif
