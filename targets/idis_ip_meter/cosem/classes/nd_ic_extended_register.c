#include "targets/idis_ip_meter/cosem/classes/nd_ic_extended_register.h"

#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

#define ND_IDIS_EXTENDED_MAGIC 0x45585431u

typedef struct {
    uint32_t magic;
    nd_idis_extended_state_t items[ND_IDIS_MAX_DEMAND];
} nd_idis_extended_record_t;

static void put_datetime(nd_axdr_writer_t *out, const nd_datetime_t *time)
{
    uint8_t data[12] = {0};
    data[0] = (uint8_t)(time->year >> 8);
    data[1] = (uint8_t)time->year;
    data[2] = time->month;
    data[3] = time->day;
    data[5] = time->hour;
    data[6] = time->minute;
    data[7] = time->second;
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, sizeof(data));
    nd_axdr_put_octets(out, data, sizeof(data));
}

static int group_index(uint8_t group)
{
    switch (group) {
    case 1:
    case 2:
    case 3:
    case 4:
        return group - 1;
    case 9:
        return 4; /* apparent import */
    case 10:
        return 5; /* apparent export */
    default:
        return -1;
    }
}

static nd_idis_extended_state_t *state_for(void *ctx, const nd_obis_t *ln)
{
    /* Maximum demand LNs are 1-0:G.6.R.255: G selects the group (1 active
     * import, 2 active export, 3 reactive import, 4 reactive export, 9/10
     * apparent import/export) and R the rate (0 total, 1..4). */
    if (ctx == NULL || ln == NULL || ln->a != 1 || ln->b != 0
        || ln->d != 6 || ln->e > 4 || ln->f != 255) {
        return NULL;
    }
    int group = group_index(ln->c);
    if (group < 0) {
        return NULL;
    }
    return &((nd_idis_extended_state_t *)ctx)[group * 5 + ln->e];
}

int nd_ic_extended_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t attribute_id, const nd_axdr_reader_t *selector,
                       nd_axdr_writer_t *out)
{
    nd_idis_extended_state_t *state = state_for(ctx, ln);
    (void)client_sap;
    (void)selector;
    if (state == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    switch (attribute_id) {
    case 1:
        nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(out, 6);
        nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
        break;
    case 2:
        nd_axdr_put_tag(out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
        nd_axdr_put_u32(out, state->value);
        break;
    case 3:
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_INTEGER);
        nd_axdr_put_u8(out, 0);
        nd_axdr_put_tag(out, ND_AXDR_ENUM);
        nd_axdr_put_u8(out, 27); /* W */
        break;
    case 4:
        nd_axdr_put_tag(out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(out, state->status);
        break;
    case 5:
        put_datetime(out, &state->capture_time);
        break;
    default:
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    return nd_axdr_writer_ok(out) ? ND_OK
                                  : ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_extended_action(void *ctx, uint8_t client_sap,
                          const nd_obis_t *ln, uint8_t method_id,
                          nd_axdr_reader_t *in, nd_axdr_writer_t *out)
{
    nd_idis_extended_state_t *state = state_for(ctx, ln);
    (void)client_sap;
    (void)out;
    if (state == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (method_id != 1) { /* reset */
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (nd_axdr_get_tag(in) != ND_AXDR_INTEGER || nd_axdr_get_u8(in) != 0
        || !nd_axdr_reader_ok(in)) {
        return ND_ERR_TYPE_MISMATCH;
    }
    *state = (nd_idis_extended_state_t){0};
    return ND_OK;
}

int nd_idis_extended_update(nd_idis_extended_state_t *states, uint8_t group,
                            uint8_t rate, uint32_t value, uint8_t status,
                            const nd_datetime_t *capture_time)
{
    if (states == NULL || capture_time == NULL || rate > 4) {
        return 0;
    }
    int index = group_index(group);
    if (index < 0) {
        return 0;
    }
    nd_idis_extended_state_t *state = &states[index * 5 + rate];
    if (state->capture_time.year != 0 && value <= state->value) {
        return 0;
    }
    state->value = value;
    state->status = status;
    state->capture_time = *capture_time;
    return 1;
}

int nd_ic_extended_save(const nd_idis_extended_state_t *states,
                        nd_store_t *store, uint32_t offset)
{
    if (states == NULL || store == NULL) {
        return -1;
    }
    nd_idis_extended_record_t record = {.magic = ND_IDIS_EXTENDED_MAGIC};
    for (uint8_t index = 0; index < ND_IDIS_MAX_DEMAND; index++) {
        record.items[index] = states[index];
    }
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_ic_extended_load(nd_idis_extended_state_t *states, nd_store_t *store,
                        uint32_t offset)
{
    if (states == NULL || store == NULL) {
        return -1;
    }
    nd_idis_extended_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_EXTENDED_MAGIC) {
        return -1;
    }
    for (uint8_t index = 0; index < ND_IDIS_MAX_DEMAND; index++) {
        states[index] = record.items[index];
    }
    return 0;
}
