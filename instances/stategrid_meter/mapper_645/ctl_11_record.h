


#define RECORED_FILL_NO_OBJ_645     0x00     //无关联对象
#define RECORED_FILL_UNGENERATEED_645  0x00  //关联对象存在，但未生成 对应数据全FF
#define RECORED_FILL_NO_DATA_645    0x00     //无记录数据





extern int record_data_package645(struct instance_t* instance, uint32_t ID, uint8_t* buf,  struct rsd_info_t *rsd_info, uint32_t *rcsd_info, uint16_t *capture_645len,  uint8_t  packageAA,   void *ctx);
extern int record_data_package645_packageAA(struct instance_t* instance, uint8_t* dest, uint8_t* src, uint16_t datalen, struct record_storage_645_id_info_t  *info);



