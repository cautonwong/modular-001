#include "kernel\nidavellir.h"

int init_memory(struct instance_t *instance,struct layout_t *layouts,int layout_n)
{
    int i = 0;
    uint32_t start = 0;
    uint32_t end = 0;
    uint32_t used = 0;
    uint32_t size = 0;
    uint8_t *p = instance->memory;
    struct layout_t *layout = NULL;

    for(i=0;i<layout_n;i++)
    {
        layout = &layouts[i];
        start = layout->start;
        end = layout->end;

        memcpy(&p[start],&layout->start_,sizeof(uint32_t));
        memcpy(&p[end]-sizeof(uint32_t),&layouts->end_,sizeof(uint32_t));
    }
    
    return 0;
}

void *sb_malloc(struct instance_t *instance,memory_id_t memory_id,uint32_t size)
{
    struct layout_t *layout = &instance->recipe->layout[memory_id];
    uint32_t start = layout->start;
    uint32_t end = layout->end;
    uint32_t used = instance->areas_used[memory_id];
    uint8_t *p = instance->memory+start+used;
    uint32_t free = end - sizeof(uint32_t) - start - used;

    if(free > size)
    {
        instance->areas_used[memory_id] += size;
        p += size;
        return p;
    }
    printf("error--no memory\n");
    return NULL;
}

int verify_memory()
{}

int show_memory_info()
{};


void *user_malloc(struct instance_t *instance,memory_id_t memory_id,uint32_t size)
{}
void *power_safe_malloc(struct instance_t *instance,memory_id_t memory_id,uint32_t size)
{}
/*
    0.isr
    1.work queue
    2.rt root
    3.workers root
    4.workers data
    5.power-safe data
    6.dev fifo
    7.cache
    8.stack  last
*/
