#ifndef ND_TARGET_STATEGRID_TARIFF_APPLICATIONS_H
#define ND_TARGET_STATEGRID_TARIFF_APPLICATIONS_H

#include <stdint.h>
void Rtc_Prossecc(void *instance, void *pipeline);
void TouFeePro(void *instance, void *pipeline);
void rtc_get_from_tm(void *instance, uint8_t *p);
unsigned char SeasonTab_Format_Check(void *instance,
                                     unsigned char *Addr, unsigned char *Len);
unsigned char TouTab_Format_Check(void *instance,
                                  unsigned char *Addr, unsigned char *Len);
unsigned long GetTotalMin(unsigned char *Addr);

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
