#ifndef ND_TARGET_STATEGRID_METER_APP_RAM_BUDGET_H
#define ND_TARGET_STATEGRID_METER_APP_RAM_BUDGET_H

#include <stddef.h>
#include <stdint.h>

#include "application/adapters/dl698/fr_698_context.h"
#include "components/core/fifo/fifo.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "targets/stategrid_meter/tools/app_resource_tool/resource_tool.h"

/*
 * stategrid_meter allocates every object from one arena. The first
 * STG_RAM_METADATA_SIZE bytes hold the context, the block devices and the
 * resource-block metadata; each add_block takes its size out of the rest.
 * __nd_malloc rounds every request up to 4 bytes, so each term below rounds
 * the same way. The static asserts fail the build when a block's inner
 * allocations no longer fit or the blocks no longer fit the arena. sizeof is
 * the compiling toolchain's: the host simulation and the Cortex-M0 build each
 * check their own layout.
 */
#define STG_RAM_ARENA_SIZE (24u * 1024u)
/* The relay load-control arena lives in the context, so the metadata block
 * carries its 128-byte buffer on top of the 2 KiB. */
#define STG_RAM_METADATA_SIZE (2u * 1024u + 128u)
#define STG_RAM_CACHE_BLOCK_SIZE 2108u
#define STG_RAM_MBASE_BLOCK_SIZE (3u * 1024u)

/*
 * RS485-1, the PLC module and IR carry the three 698 channels this product
 * has; a fourth channel is not a product configuration. The 698 block is
 * sized from this count and stategrid_meter_entry.c asserts the channel
 * table has the same number of links, so adding a channel means bumping
 * this count, which resizes the block and re-checks the budget.
 */
#define STG_698_LINK_COUNT 3u

#define STG_ALIGN4(n) (((n) + 3u) & ~3u)

#define STG_698_CHANNEL_SIZE                  \
    (STG_ALIGN4(sizeof(struct context_698_t)) \
     + 3u * STG_ALIGN4(sizeof(struct fifo_t)) \
     + STG_ALIGN4(Max698_apdu) + 2u * STG_ALIGN4(Max698len))

#define STG_698_BLOCK_SIZE \
    ((uint32_t)(STG_698_LINK_COUNT * STG_698_CHANNEL_SIZE))

/*
 * Cache block contents: the RTC and key FIFOs, the APDU buffer and one
 * 8-byte FIFO per 698 channel in communication_channel_init.
 */
#define STG_RAM_CACHE_RTC_SIZE 4u
#define STG_RAM_CACHE_KEY_SIZE 8u
#define STG_RAM_CACHE_APDU_SIZE ((uint32_t)cache_max)
#define STG_RAM_CACHE_CHANNEL_SIZE 8u
#define STG_RAM_CACHE_USED                                      \
    (STG_ALIGN4(STG_RAM_CACHE_RTC_SIZE)                          \
     + STG_ALIGN4(STG_RAM_CACHE_KEY_SIZE)                        \
     + STG_ALIGN4(STG_RAM_CACHE_APDU_SIZE)                       \
     + STG_698_LINK_COUNT * STG_ALIGN4(STG_RAM_CACHE_CHANNEL_SIZE))

_Static_assert(STG_RAM_CACHE_USED <= STG_RAM_CACHE_BLOCK_SIZE,
               "stategrid_meter cache block allocations exceed the block");

/*
 * Metadata contents: the context, four block_device_t records (the RAM
 * device, the EEPROM and flash devices, and the 698 map) and seventeen
 * resource_block_t records (the root block, seven add_block, three
 * add_inside_block and six register_parameter/register_log).
 */
#define STG_RAM_METADATA_DEVICES 4u
#define STG_RAM_METADATA_BLOCKS 17u
#define STG_RAM_METADATA_USED                                            \
    (STG_ALIGN4(sizeof(stategrid_context_t))                             \
     + STG_RAM_METADATA_DEVICES * STG_ALIGN4(sizeof(struct block_device_t)) \
     + STG_RAM_METADATA_BLOCKS * STG_ALIGN4(sizeof(struct resource_block_t)))

_Static_assert(STG_RAM_METADATA_USED <= STG_RAM_METADATA_SIZE,
               "stategrid_meter metadata allocations exceed the metadata block");

/*
 * The mbase block holds the static, cache and persistent sections placed by
 * add_inside_block at offsets 0, static size and static plus cache size.
 */
#define STG_RAM_MBASE_STATIC_SIZE                                        \
    ((uint32_t)(nd_mbase_offset_static_section_end()                     \
                - nd_mbase_offset_static_section_start() + 1u))
#define STG_RAM_MBASE_CACHE_SIZE                                        \
    ((uint32_t)(nd_mbase_offset_cache_section_end()                      \
                - nd_mbase_offset_cache_section_start() + 1u))
#define STG_RAM_MBASE_PERSISTENT_SIZE                                   \
    ((uint32_t)(nd_mbase_offset_persistent_section_end()                 \
                - nd_mbase_offset_persistent_section_start() + 1u))
#define STG_RAM_MBASE_USED                                              \
    (STG_RAM_MBASE_STATIC_SIZE + STG_RAM_MBASE_CACHE_SIZE               \
     + STG_RAM_MBASE_PERSISTENT_SIZE)

_Static_assert(STG_RAM_MBASE_USED <= STG_RAM_MBASE_BLOCK_SIZE,
               "stategrid_meter mbase sections exceed the mbase block");

_Static_assert(STG_RAM_METADATA_SIZE + STG_RAM_CACHE_BLOCK_SIZE
                       + STG_698_BLOCK_SIZE + STG_RAM_MBASE_BLOCK_SIZE
                   <= STG_RAM_ARENA_SIZE,
               "stategrid_meter RAM blocks exceed the nd_memory arena");

#endif /* ND_TARGET_STATEGRID_METER_APP_RAM_BUDGET_H */
