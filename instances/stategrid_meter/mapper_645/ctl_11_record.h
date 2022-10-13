#include "nidavellir.h"
#include "645id_table.h"
#include "gs_record.h"
#include "fr_axdr.h"
#include "fr_698.h"


#define RECORED_FILL_NO_OBJ_645  0x00
#define RECORED_FILL_NO_DATA_645  0x00


union dl645_DI_t
{
    struct 
    {
        uint8_t  di0; 
        uint8_t  di1; 
        uint8_t  di2;
        uint8_t  di3;
    }elem;

    uint32_t di;
};





extern int record_data_package645(struct instance_t* instance, uint32_t ID, uint8_t* buf,  struct rsd_info_t *rsd_info, uint32_t *rcsd_info, uint16_t *capture_645len,  uint8_t  packageAA,   void *ctx);
extern int record_data_package645_packageAA(struct instance_t* instance, uint8_t* dest, uint8_t* src, uint16_t datalen, struct record_storage_645_id_info_t  *info);



