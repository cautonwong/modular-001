
#ifndef __UPDATA_H__
#define __UPDATA_H__

typedef enum Updata_err {
    ERR_NONE = 0u,
    ERR_FAIL = 1u,
} Updata_err_t;

uint8_t updata(uint8_t *para);

#endif /* __UPDATA_H__ */

