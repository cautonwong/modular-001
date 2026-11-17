#ifndef ND_TARGET_STATEGRID_METERING_RN8209_TARGET_H
#define ND_TARGET_STATEGRID_METERING_RN8209_TARGET_H

#include <stdint.h>

/* Product calibration and metering-chip constants. The chip protocol lives
 * in infra/drivers/metering/rn8209; these values belong to this meter. */
#define Fr_K 1.676191

#define REAL_K_U_INIT 0.018213639
#define REAL_K_I_INIT 0.28802
#define REAL_K_P_INIT 0.0017918671

#define IMP 2000

#define Hfconst_default (uint16_t)(0x082D * Fr_K)
#define Pstart_default (uint16_t)0x004C

struct Rn8209AdjSet_t {
    unsigned short Gpq;
    unsigned short Apos;
    unsigned short Irmsos;
    unsigned short Phs;
};

struct Rn8209AdjK_t {
    float u;
    float il;
    float in;
    float p;
};

extern struct Rn8209AdjK_t Rn8209Adj_K;

struct factor_adj_meter_t {
    unsigned char adjen;
    unsigned char adjsec;
    unsigned char adjflag;
    unsigned long Ua;
    unsigned long Ia;
    signed long Pa;
    signed long Paw;
    unsigned long Ub;
    unsigned long Ib;
    signed long Pb;
    signed long Pbw;
    unsigned long Uc;
    unsigned long Ic;
    signed long Pc;
    signed long Pcw;
    unsigned long Ian;
    unsigned char null[2];
};

struct ADJ_POFFSET_T {
    int32_t ps;
    int32_t power_avg;
    uint8_t tick;
    uint32_t flag;
};

extern struct ADJ_POFFSET_T adj_poffset;

#define F_ALadj 0x01
#define F_ANadj 0x10

#endif
