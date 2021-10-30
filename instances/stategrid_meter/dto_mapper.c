struct map_struct_t
{
    uint32_t dto;//data transfer object
}map;

int dto_get_data(struct instance_t* instance,uint32_t dto,uint8_t* buf)
{
    switch(dto)
    {
        case 0x2000:
            memcpy(buf,&ctrl_block->Ua,4);
            //
            break;

    }
    return 0;
}

int dto_set_data(uint32_t dto,uint8_t* buf)
{
    switch(dto)
    {
        case 0x2000:
            memcpy(&ctrl_block->Ua,buf,4);
            break;
    }
    return 0;
}

int dto_size(uint32_t dto)
{
    return 0;
}

init_resource_manager(map);

//职责链模式  过滤器
