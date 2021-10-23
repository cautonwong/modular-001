#include "kernel\nidavellir.h"
#include "fr_axdr.h"
#include "fr_698.h"




int nd_get(struct instance_t* instance, uint32_t oad, uint8_t* p, uint16_t len)
{
    uint8_t* pdata = mock_ptr_type(uint8_t*);
    int r = mock_type(int);
    if (r > 0)
    {
        memcpy(p, pdata, r);
    }
    return r;
}


int nd_set(struct instance_t* instance, uint32_t oad, uint8_t* p, uint16_t len)
{
    return 0;
}

int nd_ioctrl(struct instance_t* instance, uint32_t oad, uint8_t* p, uint16_t len)
{
    return 0;
}
