#ifndef ND_INFRA_KERNEL_RESOURCE_BLOCKS_H
#define ND_INFRA_KERNEL_RESOURCE_BLOCKS_H

enum resourenc_block_name
{
    resource_block_name = 0x01,
    control_block_cache,
    control_block_698_name,
    mbase_control_block_name,
    mbase_block_static,
    mbase_block_static_eprom,           /* main area */
    mbase_block_static_eprom_bp,        /* backup area, follows the main area */
    mbase_block_cache,
    mbase_block_persistent,
    mbase_block_persistent_eprom,
    mbase_block_event,
    mbase_block_parameter,              /* no backup */
    mbase_block_parameter_1,            /* double backup, main area */
    mbase_block_parameter_1_bp,         /* double backup, backup area */
    mbase_block_event_flash1,           /* shared with upgrade, first flash block */
    mbase_block_event_flash2,
    public_block_flash,
};

enum resourenc_map_name
{
    res_map_698 = 0x01,
};

enum device_id_t
{
    DEV_EEPROM = 1,
    DEV_FLASH,
    DEV_EMU,
    DEV_RAM,
};

#endif
