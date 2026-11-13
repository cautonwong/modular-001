#ifndef ND_DOMAIN_TARIFF_LEGACY_APPLICATIONS_H
#define ND_DOMAIN_TARIFF_LEGACY_APPLICATIONS_H

#include <stdint.h>

struct instance_t;
struct pipeline_t;

void Rtc_Prossecc(struct instance_t *instance, struct pipeline_t *pipeline);
void TouFeePro(struct instance_t *instance, struct pipeline_t *pipeline);
void rtc_get_from_tm(struct instance_t *instance, uint8_t *p);
uint8_t Date_Format_Check(unsigned char *Addr);
uint8_t Time_Format_Check(unsigned char *Addr);

#define MaxSeasonNum 14
#define MaxTouNum 14
#define MaxFeeNum 12
#define MaxHolidayNum 20
#define MaxTouTabNum 8
#define MaxStepNum 6

#define F_TOUChange 0x01
#define F_DAYTABChange 0x02
#define F_TOUFeeChangeTm 0x04
#define F_ChangeTmOn 0x08

#endif
