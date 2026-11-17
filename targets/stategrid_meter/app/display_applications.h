#ifndef ND_TARGET_STATEGRID_METER_DISPLAY_APPLICATIONS_H
#define ND_TARGET_STATEGRID_METER_DISPLAY_APPLICATIONS_H
#include "targets/stategrid_meter/app/message.h"

#include <stdint.h>
struct message_t;

void Display(void *instance, void *pipeline);
void Key_Display(void *instance, void *pipeline,
                 uint8_t ctr);
void AllDisplayLCD(void *instance, void *pipeline);
unsigned short display_oad_to_id(unsigned char *oad, unsigned char *oad_son,
                                 unsigned char Sequ, unsigned char len);
uint8_t display_id_to_oad(unsigned short id, unsigned char *oad);

#endif
