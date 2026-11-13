#ifndef ND_APPLICATION_APPS_DISPLAY_APPLICATIONS_H
#define ND_APPLICATION_APPS_DISPLAY_APPLICATIONS_H

#include <stdint.h>

struct instance_t;
struct pipeline_t;
struct message_t;

void Display(struct instance_t *instance, struct pipeline_t *pipeline);
void Key_Display(struct instance_t *instance, struct pipeline_t *pipeline,
                 uint8_t ctr);
void AllDisplayLCD(struct instance_t *instance, struct pipeline_t *pipeline);
int backlight_pro(struct instance_t *instance, struct pipeline_t *task,
                  struct message_t *mesg);
void check_RtcBat_volt(struct instance_t *instance);
unsigned short display_oad_to_id(unsigned char *oad, unsigned char *oad_son,
                                 unsigned char Sequ, unsigned char len);
uint8_t display_id_to_oad(unsigned short id, unsigned char *oad);

#endif
