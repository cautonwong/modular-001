#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_push.h"

#include <string.h>

#include "targets/idis_ip_meter_freertos/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter_freertos/mapper_idis/nd_idis_ids.h"

#define ND_IDIS_PUSH_MAGIC 0x50555332u

typedef struct {
    uint32_t magic;
    nd_push_setup_t setups[ND_PUSH_SETUPS];
    uint8_t object_count[ND_PUSH_SETUPS];
    nd_idis_push_object_t objects[ND_PUSH_SETUPS][ND_IDIS_PUSH_OBJECTS_MAX];
    uint8_t window_count[ND_PUSH_SETUPS];
    nd_idis_push_window_t windows[ND_PUSH_SETUPS][ND_IDIS_PUSH_WINDOWS_MAX];
    uint8_t protection_count[ND_PUSH_SETUPS];
    uint8_t protection_len[ND_PUSH_SETUPS][ND_IDIS_PUSH_PROTECTION_MAX];
    uint8_t protection[ND_PUSH_SETUPS][ND_IDIS_PUSH_PROTECTION_MAX]
                     [ND_IDIS_PUSH_WINDOW_LEN];
    uint8_t gateway_len[ND_PUSH_SETUPS];
    uint8_t gateway[ND_PUSH_SETUPS][ND_IDIS_PUSH_GATEWAY_LEN];
} nd_idis_push_record_t;

void nd_idis_push_init(nd_idis_push_t *push)
{
    if (push != NULL) {
        memset(push, 0, sizeof(*push));
        nd_push_init(&push->state);
    }
}

static int setup_index(const nd_obis_t *ln)
{
    if (ln == NULL || ln->a != 0 || ln->b > 7 || ln->c != 25 || ln->d != 9
        || ln->e != 0 || ln->f != 255) {
        return -1;
    }
    return ln->b;
}

static nd_push_setup_t *setup_for(nd_idis_objects_t *objects,
                                  const nd_obis_t *ln)
{
    int index = setup_index(ln);
    return index < 0 ? NULL : &objects->push.state.setups[index];
}

static void read_now(nd_idis_objects_t *objects, nd_datetime_t *now)
{
    *now = (nd_datetime_t){0};
    if (objects->clock.port != NULL && objects->clock.port->read != NULL) {
        nd_dlms_datetime_t value;
        if (objects->clock.port->read(objects->clock.port->ctx, &value) == 0) {
            now->year = value.year;
            now->month = value.month;
            now->day = value.day;
            now->hour = value.hour;
            now->minute = value.minute;
            now->second = value.second;
        }
    }
}

static int read_unsigned(nd_axdr_reader_t *in, uint32_t *value)
{
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag == ND_AXDR_UNSIGNED) {
        *value = nd_axdr_get_u8(in);
    } else if (tag == ND_AXDR_INTEGER) {
        *value = (uint8_t)nd_axdr_get_u8(in);
    } else if (tag == ND_AXDR_LONG_UNSIGNED) {
        *value = nd_axdr_get_u16(in);
    } else if (tag == ND_AXDR_DOUBLE_LONG_UNSIGNED) {
        *value = nd_axdr_get_u32(in);
    } else {
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static void put_object(nd_axdr_writer_t *out,
                       const nd_idis_push_object_t *object)
{
    nd_axdr_put_structure_header(out, 5);
    nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(out, object->class_id);
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, 6);
    nd_axdr_put_octets(out, (const uint8_t *)&object->ln, 6);
    nd_axdr_put_tag(out, ND_AXDR_INTEGER);
    nd_axdr_put_u8(out, object->attribute_index);
    nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(out, object->data_index);
    nd_axdr_put_structure_header(out, 2);
    nd_axdr_put_tag(out, ND_AXDR_ENUM);
    nd_axdr_put_u8(out, 0); /* restriction: none */
    nd_axdr_put_tag(out, ND_AXDR_NULL);
}

int nd_ic_push_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                   uint8_t attribute_id, const nd_axdr_reader_t *selector,
                   nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;
    int index = setup_index(ln);
    nd_push_setup_t *setup = setup_for(objects, ln);
    (void)client_sap;
    (void)selector;
    if (setup == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    switch (attribute_id) {
    case 1:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
        break;
    case 2:
        nd_axdr_put_array_header(out, objects->push.object_count[index]);
        for (uint8_t object = 0; object < objects->push.object_count[index];
             object++) {
            put_object(out, &objects->push.objects[index][object]);
        }
        break;
    case 3:
        nd_axdr_put_structure_header(out, 3);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, setup->destination.transport_service);
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, setup->destination.destination_length);
        nd_axdr_put_octets(out, setup->destination.destination,
                           setup->destination.destination_length);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, setup->destination.message_type);
        break;
    case 4:
        nd_axdr_put_array_header(out, objects->push.window_count[index]);
        for (uint8_t window = 0;
             window < objects->push.window_count[index]; window++) {
            const nd_idis_push_window_t *item =
                &objects->push.windows[index][window];
            nd_axdr_put_structure_header(out, 2);
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, item->start_len);
            nd_axdr_put_octets(out, item->start, item->start_len);
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, item->end_len);
            nd_axdr_put_octets(out, item->end, item->end_len);
        }
        break;
    case 5:
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, setup->randomisation_start_interval);
        break;
    case 6:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, setup->number_of_retries);
        break;
    case 7:
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, (uint16_t)setup->repetition_delay);
        break;
    case 8: {
        int empty = 1;
        for (uint8_t byte = 0; byte < 6; byte++) {
            if (setup->port_reference[byte] != 0) {
                empty = 0;
                break;
            }
        }
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, empty ? 0 : 6);
        nd_axdr_put_octets(out, setup->port_reference, empty ? 0 : 6);
        break;
    }
    case 9:
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, setup->push_client_sap);
        break;
    case 10:
        nd_axdr_put_array_header(out, objects->push.protection_count[index]);
        for (uint8_t item = 0;
             item < objects->push.protection_count[index]; item++) {
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, objects->push.protection_len[index][item]);
            nd_axdr_put_octets(out, objects->push.protection[index][item],
                               objects->push.protection_len[index][item]);
        }
        break;
    case 11:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, objects->push.gateway_len[index]);
        nd_axdr_put_octets(out, objects->push.gateway[index],
                           objects->push.gateway_len[index]);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

static int parse_object(nd_axdr_reader_t *in, nd_idis_push_object_t *object)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) != 5) {
        return -1;
    }
    uint32_t class_id = 0;
    if (read_unsigned(in, &class_id) != 0 || class_id > 0xFFFF
        || nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
        || nd_axdr_get_length(in) != 6) {
        return -1;
    }
    uint8_t ln[6];
    nd_axdr_get_octets(in, ln, 6);
    uint32_t attribute = 0;
    if (read_unsigned(in, &attribute) != 0 || attribute > 0xFF) {
        return -1;
    }
    uint32_t data_index = 0;
    if (read_unsigned(in, &data_index) != 0 || data_index > 0xFFFF
        || nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) != 2
        || nd_axdr_get_tag(in) != ND_AXDR_ENUM) {
        return -1;
    }
    uint8_t restriction = nd_axdr_get_u8(in);
    if (restriction != 0 || nd_axdr_get_tag(in) != ND_AXDR_NULL) {
        return -1;
    }
    object->class_id = (uint16_t)class_id;
    memcpy(&object->ln, ln, sizeof(ln));
    object->attribute_index = (uint8_t)attribute;
    object->data_index = (uint16_t)data_index;
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int set_object_list(nd_idis_push_t *push, uint8_t index,
                           nd_axdr_reader_t *in)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
        return -1;
    }
    uint16_t count = nd_axdr_get_length(in);
    if (count > ND_IDIS_PUSH_OBJECTS_MAX) {
        return -1;
    }
    nd_idis_push_object_t parsed[ND_IDIS_PUSH_OBJECTS_MAX];
    for (uint16_t object = 0; object < count; object++) {
        if (parse_object(in, &parsed[object]) != 0) {
            return -1;
        }
    }
    if (!nd_axdr_reader_ok(in) || nd_axdr_remaining(in) != 0) {
        return -1;
    }
    for (uint16_t object = 0; object < count; object++) {
        push->objects[index][object] = parsed[object];
    }
    push->object_count[index] = (uint8_t)count;
    push->state.setups[index].enabled = count > 0 ? 1 : 0;
    return 0;
}

static int set_destination(nd_push_setup_t *setup, nd_axdr_reader_t *in)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) != 3
        || nd_axdr_get_tag(in) != ND_AXDR_ENUM) {
        return -1;
    }
    uint8_t transport = nd_axdr_get_u8(in);
    if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
        return -1;
    }
    uint16_t length = nd_axdr_get_length(in);
    if (length > ND_PUSH_DESTINATION_MAX) {
        return -1;
    }
    uint8_t destination[ND_PUSH_DESTINATION_MAX];
    nd_axdr_get_octets(in, destination, length);
    if (nd_axdr_get_tag(in) != ND_AXDR_ENUM) {
        return -1;
    }
    uint8_t message = nd_axdr_get_u8(in);
    if (!nd_axdr_reader_ok(in) || nd_axdr_remaining(in) != 0) {
        return -1;
    }
    setup->destination.transport_service = transport;
    setup->destination.message_type = message;
    memcpy(setup->destination.destination, destination, length);
    setup->destination.destination_length = (uint8_t)length;
    return 0;
}

int nd_ic_push_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                   uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_idis_objects_t *objects = ctx;
    int index = setup_index(ln);
    nd_push_setup_t *setup = setup_for(objects, ln);
    (void)client_sap;
    if (setup == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 2) {
        return set_object_list(&objects->push, (uint8_t)index, in) == 0
                   ? ND_OK
                   : ND_ERR_TYPE_MISMATCH;
    }
    if (attribute_id == 3) {
        return set_destination(setup, in) == 0 ? ND_OK
                                               : ND_ERR_TYPE_MISMATCH;
    }
    if (attribute_id == 4) {
        if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t count = nd_axdr_get_length(in);
        if (count > ND_IDIS_PUSH_WINDOWS_MAX) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_idis_push_window_t windows[ND_IDIS_PUSH_WINDOWS_MAX] = {0};
        for (uint16_t window = 0; window < count; window++) {
            if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
                || nd_axdr_get_length(in) < 2) {
                return ND_ERR_TYPE_MISMATCH;
            }
            for (uint8_t field = 0; field < 2; field++) {
                if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
                    return ND_ERR_TYPE_MISMATCH;
                }
                uint16_t length = nd_axdr_get_length(in);
                if (length > ND_IDIS_PUSH_WINDOW_LEN) {
                    return ND_ERR_TYPE_MISMATCH;
                }
                if (field == 0) {
                    nd_axdr_get_octets(in, windows[window].start, length);
                    windows[window].start_len = (uint8_t)length;
                } else {
                    nd_axdr_get_octets(in, windows[window].end, length);
                    windows[window].end_len = (uint8_t)length;
                }
            }
        }
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        memcpy(objects->push.windows[index], windows, sizeof(windows));
        objects->push.window_count[index] = (uint8_t)count;
        return ND_OK;
    }
    if (attribute_id == 10) {
        if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t count = nd_axdr_get_length(in);
        if (count > ND_IDIS_PUSH_PROTECTION_MAX) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t values[ND_IDIS_PUSH_PROTECTION_MAX][ND_IDIS_PUSH_WINDOW_LEN] = {0};
        uint8_t lengths[ND_IDIS_PUSH_PROTECTION_MAX] = {0};
        for (uint16_t item = 0; item < count; item++) {
            if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
                return ND_ERR_TYPE_MISMATCH;
            }
            uint16_t length = nd_axdr_get_length(in);
            if (length > ND_IDIS_PUSH_WINDOW_LEN) {
                return ND_ERR_TYPE_MISMATCH;
            }
            nd_axdr_get_octets(in, values[item], length);
            lengths[item] = (uint8_t)length;
        }
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        memcpy(objects->push.protection[index], values, sizeof(values));
        memcpy(objects->push.protection_len[index], lengths, sizeof(lengths));
        objects->push.protection_count[index] = (uint8_t)count;
        return ND_OK;
    }
    if (attribute_id == 11) {
        if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t length = nd_axdr_get_length(in);
        if (length > ND_IDIS_PUSH_GATEWAY_LEN) {
            return ND_ERR_TYPE_MISMATCH;
        }
        nd_axdr_get_octets(in, objects->push.gateway[index], length);
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        objects->push.gateway_len[index] = (uint8_t)length;
        return ND_OK;
    }
    if (attribute_id == 8) {
        if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint16_t length = nd_axdr_get_length(in);
        if (length != 0 && length != 6) {
            return ND_ERR_TYPE_MISMATCH;
        }
        uint8_t reference[6] = {0};
        nd_axdr_get_octets(in, reference, length);
        if (!nd_axdr_reader_ok(in)) {
            return ND_ERR_TYPE_MISMATCH;
        }
        memcpy(setup->port_reference, reference, sizeof(reference));
        return ND_OK;
    }
    if (attribute_id == 1 || attribute_id == 13) {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    uint32_t value = 0;
    if (read_unsigned(in, &value) != 0) {
        return ND_ERR_TYPE_MISMATCH;
    }
    if (attribute_id == 5) {
        setup->randomisation_start_interval = (uint16_t)value;
    } else if (attribute_id == 6) {
        if (value > 0xFF) {
            return ND_ERR_TYPE_MISMATCH;
        }
        setup->number_of_retries = (uint8_t)value;
    } else if (attribute_id == 7) {
        setup->repetition_delay = value;
    } else if (attribute_id == 9) {
        setup->push_client_sap = (uint8_t)value;
    } else {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return ND_OK;
}

int nd_ic_push_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                      uint8_t method_id, nd_axdr_reader_t *in,
                      nd_axdr_writer_t *out)
{
    nd_idis_objects_t *objects = ctx;
    (void)client_sap;
    (void)out;
    int index = setup_index(ln);
    if (objects == NULL || index < 0) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id == 1) {
        if (in != NULL && nd_axdr_remaining(in) > 0) {
            uint32_t ignored = 0;
            if (read_unsigned(in, &ignored) != 0) {
                return ND_ERR_TYPE_MISMATCH;
            }
        }
        nd_datetime_t now;
        read_now(objects, &now);
        return nd_push_trigger(&objects->push.state, (uint8_t)index, &now) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (method_id == 2) {
        return nd_push_reset(&objects->push.state, (uint8_t)index) == 0
                   ? ND_OK
                   : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_idis_push_save(const nd_idis_push_t *push, nd_store_t *store,
                      uint32_t offset)
{
    if (push == NULL || store == NULL) {
        return -1;
    }
    nd_idis_push_record_t record = {.magic = ND_IDIS_PUSH_MAGIC};
    for (uint8_t index = 0; index < ND_PUSH_SETUPS; index++) {
        record.setups[index] = push->state.setups[index];
        record.object_count[index] = push->object_count[index];
        for (uint8_t object = 0; object < ND_IDIS_PUSH_OBJECTS_MAX; object++) {
            record.objects[index][object] = push->objects[index][object];
        }
        record.window_count[index] = push->window_count[index];
        memcpy(record.windows[index], push->windows[index],
               sizeof(record.windows[index]));
        record.protection_count[index] = push->protection_count[index];
        memcpy(record.protection_len[index], push->protection_len[index],
               sizeof(record.protection_len[index]));
        memcpy(record.protection[index], push->protection[index],
               sizeof(record.protection[index]));
        record.gateway_len[index] = push->gateway_len[index];
        memcpy(record.gateway[index], push->gateway[index],
               sizeof(record.gateway[index]));
    }
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_push_load(nd_idis_push_t *push, nd_store_t *store, uint32_t offset)
{
    if (push == NULL || store == NULL) {
        return -1;
    }
    nd_idis_push_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_PUSH_MAGIC) {
        return -1;
    }
    for (uint8_t index = 0; index < ND_PUSH_SETUPS; index++) {
        push->state.setups[index] = record.setups[index];
        push->object_count[index] = record.object_count[index];
        for (uint8_t object = 0; object < ND_IDIS_PUSH_OBJECTS_MAX; object++) {
            push->objects[index][object] = record.objects[index][object];
        }
        push->window_count[index] = record.window_count[index];
        memcpy(push->windows[index], record.windows[index],
               sizeof(push->windows[index]));
        push->protection_count[index] = record.protection_count[index];
        memcpy(push->protection_len[index], record.protection_len[index],
               sizeof(push->protection_len[index]));
        memcpy(push->protection[index], record.protection[index],
               sizeof(push->protection[index]));
        push->gateway_len[index] = record.gateway_len[index];
        memcpy(push->gateway[index], record.gateway[index],
               sizeof(push->gateway[index]));
    }
    return 0;
}
