#ifndef __INCLUDE_MEMORY_H__
#define __INCLUDE_MEMORY_H__
struct instance_t;

struct layout_t
{   
    uint32_t start;
    uint32_t end;
    uint32_t start_;
    uint32_t end_;
};

int init_memory(struct instance_t *instance,struct layout_t *layouts,int layout_n);
void *sb_malloc(struct instance_t *instance,memory_id_t memory_id,uint32_t size);

#endif