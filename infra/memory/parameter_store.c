#include "infra/memory/parameter_store.h"

#include <string.h>

static nd_param_record_t *find_record(nd_param_table_t *table, uint16_t id)
{
    for (size_t index = 0; index < table->count; index++) {
        if (table->records[index].id == id) {
            return &table->records[index];
        }
    }
    return NULL;
}

static int record_read(void *ctx, uint16_t id, uint8_t *buffer, size_t length)
{
    nd_param_table_t *table = ctx;
    nd_param_record_t *record = find_record(table, id);

    if (record == NULL || length > record->length) {
        return -1;
    }
    memcpy(buffer, record->data, length);
    return 0;
}

static int record_write(void *ctx, uint16_t id, const uint8_t *buffer, size_t length)
{
    nd_param_table_t *table = ctx;
    nd_param_record_t *record = find_record(table, id);

    if (record == NULL || length > record->length) {
        return -1;
    }
    memcpy(record->data, buffer, length);
    return 0;
}

void nd_param_table_init(nd_param_table_t *table, nd_param_record_t *records,
                         size_t count)
{
    table->port.ctx = table;
    table->port.read = record_read;
    table->port.write = record_write;
    table->records = records;
    table->count = count;
}

nd_parameter_store_t *nd_param_table_port(nd_param_table_t *table)
{
    return &table->port;
}
