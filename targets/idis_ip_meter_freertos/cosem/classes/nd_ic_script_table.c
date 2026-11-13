#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_script_table.h"

#include <stddef.h>
#include <string.h>

#include "targets/idis_ip_meter_freertos/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter_freertos/mapper_idis/nd_idis_ids.h"

#define ND_IDIS_SCRIPT_MAGIC 0x53435231u

typedef struct {
    uint32_t magic;
    uint8_t ram_valid[ND_SCRIPT_TABLE_MAX];
    uint8_t ram_script_count[ND_SCRIPT_TABLE_MAX];
    nd_script_slot_t ram_scripts[ND_SCRIPT_TABLE_MAX][ND_SCRIPT_RAM_SCRIPTS];
    nd_script_action_t ram_actions[ND_SCRIPT_TABLE_MAX][ND_SCRIPT_RAM_ACTIONS];
} nd_idis_script_record_t;

#define A_SET(cls, ln, member, len, ...) \
    {1, cls, ln, member, len, {__VA_ARGS__}}
#define A_ACT(cls, ln, member, len, ...) \
    {2, cls, ln, member, len, {__VA_ARGS__}}
#define A_EMPTY(service, cls, ln, member) {service, cls, ln, member, 0, {0}}

/* Tariffication script table: script N activates mask "N" on both register
 * activation objects. The mask names are configuration data that the CTI can
 * replace; they are kept one character long for the skeleton. */
static const nd_script_action_t tariff_actions_1[] = {
    A_SET(ND_IC_REGISTER_ACTIVATION, ND_OBIS_TARIFF_ACTIVE_MASK_1, 4, 3,
          0x09, 0x01, '1'),
    A_SET(ND_IC_REGISTER_ACTIVATION, ND_OBIS_TARIFF_ACTIVE_MASK_2, 4, 3,
          0x09, 0x01, '1'),
};
static const nd_script_action_t tariff_actions_2[] = {
    A_SET(ND_IC_REGISTER_ACTIVATION, ND_OBIS_TARIFF_ACTIVE_MASK_1, 4, 3,
          0x09, 0x01, '2'),
    A_SET(ND_IC_REGISTER_ACTIVATION, ND_OBIS_TARIFF_ACTIVE_MASK_2, 4, 3,
          0x09, 0x01, '2'),
};
static const nd_script_action_t tariff_actions_3[] = {
    A_SET(ND_IC_REGISTER_ACTIVATION, ND_OBIS_TARIFF_ACTIVE_MASK_1, 4, 3,
          0x09, 0x01, '3'),
    A_SET(ND_IC_REGISTER_ACTIVATION, ND_OBIS_TARIFF_ACTIVE_MASK_2, 4, 3,
          0x09, 0x01, '3'),
};
static const nd_script_action_t tariff_actions_4[] = {
    A_SET(ND_IC_REGISTER_ACTIVATION, ND_OBIS_TARIFF_ACTIVE_MASK_1, 4, 3,
          0x09, 0x01, '4'),
    A_SET(ND_IC_REGISTER_ACTIVATION, ND_OBIS_TARIFF_ACTIVE_MASK_2, 4, 3,
          0x09, 0x01, '4'),
};
static const nd_script_t tariff_scripts[] = {
    {1, 2, tariff_actions_1},
    {2, 2, tariff_actions_2},
    {3, 2, tariff_actions_3},
    {4, 2, tariff_actions_4},
};

/* MDI reset / end of billing period. */
static const nd_script_action_t mdi_actions[] = {
    A_EMPTY(2, ND_IC_PROFILE_GENERIC, ND_OBIS_BILLING_PROFILE, 2),
};
static const nd_script_t mdi_scripts[] = {
    {1, 1, mdi_actions},
};

/* Disconnector script table. */
static const nd_script_action_t disconnect_actions_1[] = {
    A_SET(ND_IC_DISCONNECT_CONTROL, ND_OBIS_DISCONNECT_CONTROL, 3, 2, 0x16, 0x02),
};
static const nd_script_action_t disconnect_actions_2[] = {
    A_SET(ND_IC_DISCONNECT_CONTROL, ND_OBIS_DISCONNECT_CONTROL, 3, 2, 0x16, 0x01),
};
static const nd_script_action_t disconnect_actions_3[] = {
    A_EMPTY(2, ND_IC_DISCONNECT_CONTROL, ND_OBIS_DISCONNECT_CONTROL, 1),
};
static const nd_script_action_t disconnect_actions_4[] = {
    A_EMPTY(2, ND_IC_DISCONNECT_CONTROL, ND_OBIS_DISCONNECT_CONTROL, 2),
};
static const nd_script_t disconnect_scripts[] = {
    {1, 1, disconnect_actions_1},
    {2, 1, disconnect_actions_2},
    {3, 1, disconnect_actions_3},
    {4, 1, disconnect_actions_4},
};

/* Load management script table, relay 1 then relay 2. */
static const nd_script_action_t load_actions_1[] = {
    A_SET(ND_IC_DISCONNECT_CONTROL, ND_OBIS_LOAD_CONTROL_1, 3, 2, 0x16, 0x02),
};
static const nd_script_action_t load_actions_2[] = {
    A_SET(ND_IC_DISCONNECT_CONTROL, ND_OBIS_LOAD_CONTROL_1, 3, 2, 0x16, 0x01),
};
static const nd_script_action_t load_actions_3[] = {
    A_EMPTY(2, ND_IC_DISCONNECT_CONTROL, ND_OBIS_LOAD_CONTROL_1, 1),
};
static const nd_script_action_t load_actions_4[] = {
    A_EMPTY(2, ND_IC_DISCONNECT_CONTROL, ND_OBIS_LOAD_CONTROL_1, 2),
};
static const nd_script_action_t load_actions_5[] = {
    A_SET(ND_IC_DISCONNECT_CONTROL, ND_OBIS_LOAD_CONTROL_2, 3, 2, 0x16, 0x02),
};
static const nd_script_action_t load_actions_6[] = {
    A_SET(ND_IC_DISCONNECT_CONTROL, ND_OBIS_LOAD_CONTROL_2, 3, 2, 0x16, 0x01),
};
static const nd_script_action_t load_actions_7[] = {
    A_EMPTY(2, ND_IC_DISCONNECT_CONTROL, ND_OBIS_LOAD_CONTROL_2, 1),
};
static const nd_script_action_t load_actions_8[] = {
    A_EMPTY(2, ND_IC_DISCONNECT_CONTROL, ND_OBIS_LOAD_CONTROL_2, 2),
};
static const nd_script_t load_scripts[] = {
    {1, 1, load_actions_1}, {2, 1, load_actions_2},
    {3, 1, load_actions_3}, {4, 1, load_actions_4},
    {5, 1, load_actions_5}, {6, 1, load_actions_6},
    {7, 1, load_actions_7}, {8, 1, load_actions_8},
};

/* Image activation. */
static const nd_script_action_t image_actions[] = {
    A_ACT(ND_IC_IMAGE_TRANSFER, ND_OBIS_IMAGE_TRANSFER, 4, 2, 0x0F, 0x00),
};
static const nd_script_t image_scripts[] = {
    {1, 1, image_actions},
};

/* Push script table. */
static const nd_script_action_t push_actions_1[] = {
    A_ACT(ND_IC_PUSH_SETUP, ND_OBIS_PUSH_1, 1, 2, 0x11, 0x00),
};
static const nd_script_action_t push_actions_2[] = {
    A_ACT(ND_IC_PUSH_SETUP, ND_OBIS_PUSH_2, 1, 2, 0x11, 0x00),
};
static const nd_script_action_t push_actions_3[] = {
    A_ACT(ND_IC_PUSH_SETUP, ND_OBIS_PUSH_3, 1, 2, 0x11, 0x00),
};
static const nd_script_action_t push_actions_4[] = {
    A_ACT(ND_IC_PUSH_SETUP, ND_OBIS_PUSH_4, 1, 2, 0x11, 0x00),
};
static const nd_script_action_t push_actions_5[] = {
    A_ACT(ND_IC_PUSH_SETUP, ND_OBIS_PUSH_0, 1, 2, 0x11, 0x00),
};
static const nd_script_action_t push_actions_6[] = {
    A_ACT(ND_IC_PUSH_SETUP, ND_OBIS_PUSH_7, 1, 2, 0x11, 0x00),
};
static const nd_script_action_t push_actions_7[] = {
    A_ACT(ND_IC_PUSH_SETUP, ND_OBIS_PUSH_5, 1, 2, 0x11, 0x00),
};
static const nd_script_action_t push_actions_8[] = {
    A_ACT(ND_IC_PUSH_SETUP, ND_OBIS_PUSH_6, 1, 2, 0x11, 0x00),
};
static const nd_script_t push_scripts[] = {
    {1, 1, push_actions_1}, {2, 1, push_actions_2},
    {3, 1, push_actions_3}, {4, 1, push_actions_4},
    {5, 1, push_actions_5}, {6, 1, push_actions_6},
    {7, 1, push_actions_7}, {8, 1, push_actions_8},
};

/* IDIS function control script table: parameter is an array of
 * structure { visible-string name, boolean }. */
static const nd_script_action_t fc_actions_1[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_FUNCTION_CONTROL, 1, 28,
          0x01, 0x02,
          0x02, 0x02, 0x0A, 0x07, 'L', 'P', 'C', 'A', 'P', '_', '1', 0x03, 0x01,
          0x02, 0x02, 0x0A, 0x07, 'L', 'P', 'C', 'A', 'P', '_', '2', 0x03, 0x01),
};
static const nd_script_action_t fc_actions_2[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_FUNCTION_CONTROL, 1, 28,
          0x01, 0x02,
          0x02, 0x02, 0x0A, 0x07, 'L', 'P', 'C', 'A', 'P', '_', '1', 0x03, 0x00,
          0x02, 0x02, 0x0A, 0x07, 'L', 'P', 'C', 'A', 'P', '_', '2', 0x03, 0x00),
};
static const nd_script_action_t fc_actions_3[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_FUNCTION_CONTROL, 1, 15,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x07, 'L', 'P', 'C', 'A', 'P', '_', '1', 0x03, 0x01),
};
static const nd_script_action_t fc_actions_4[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_FUNCTION_CONTROL, 1, 15,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x07, 'L', 'P', 'C', 'A', 'P', '_', '1', 0x03, 0x00),
};
static const nd_script_action_t fc_actions_5[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_FUNCTION_CONTROL, 1, 15,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x07, 'L', 'P', 'C', 'A', 'P', '_', '2', 0x03, 0x01),
};
static const nd_script_action_t fc_actions_6[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_FUNCTION_CONTROL, 1, 15,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x07, 'L', 'P', 'C', 'A', 'P', '_', '2', 0x03, 0x00),
};
static const nd_script_action_t fc_actions_7[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_FUNCTION_CONTROL, 1, 16,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x08, 'I', 'F', '_', 'L', 'O', '_', '2', 'W',
          0x03, 0x01),
};
static const nd_script_action_t fc_actions_8[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_FUNCTION_CONTROL, 1, 16,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x08, 'I', 'F', '_', 'L', 'O', '_', '2', 'W',
          0x03, 0x00),
};
static const nd_script_t fc_scripts[] = {
    {1, 1, fc_actions_1}, {2, 1, fc_actions_2},
    {3, 1, fc_actions_3}, {4, 1, fc_actions_4},
    {5, 1, fc_actions_5}, {6, 1, fc_actions_6},
    {7, 1, fc_actions_7}, {8, 1, fc_actions_8},
};

/* M-Bus disconnector script table, channels 1 and 2. */
static const nd_script_action_t mbus_disc_actions_1[] = {
    A_SET(ND_IC_DISCONNECT_CONTROL, ND_OBIS_MBUS_DISCONNECT_1, 3, 2, 0x16, 0x02),
};
static const nd_script_action_t mbus_disc_actions_2[] = {
    A_SET(ND_IC_DISCONNECT_CONTROL, ND_OBIS_MBUS_DISCONNECT_1, 3, 2, 0x16, 0x01),
};
static const nd_script_action_t mbus_disc_actions_3[] = {
    A_EMPTY(2, ND_IC_DISCONNECT_CONTROL, ND_OBIS_MBUS_DISCONNECT_1, 1),
};
static const nd_script_action_t mbus_disc_actions_4[] = {
    A_EMPTY(2, ND_IC_DISCONNECT_CONTROL, ND_OBIS_MBUS_DISCONNECT_1, 2),
};
static const nd_script_action_t mbus_disc_actions_5[] = {
    A_SET(ND_IC_DISCONNECT_CONTROL, ND_OBIS_MBUS_DISCONNECT_2, 3, 2, 0x16, 0x02),
};
static const nd_script_action_t mbus_disc_actions_6[] = {
    A_SET(ND_IC_DISCONNECT_CONTROL, ND_OBIS_MBUS_DISCONNECT_2, 3, 2, 0x16, 0x01),
};
static const nd_script_action_t mbus_disc_actions_7[] = {
    A_EMPTY(2, ND_IC_DISCONNECT_CONTROL, ND_OBIS_MBUS_DISCONNECT_2, 1),
};
static const nd_script_action_t mbus_disc_actions_8[] = {
    A_EMPTY(2, ND_IC_DISCONNECT_CONTROL, ND_OBIS_MBUS_DISCONNECT_2, 2),
};
static const nd_script_t mbus_disc_scripts[] = {
    {1, 1, mbus_disc_actions_1}, {2, 1, mbus_disc_actions_2},
    {3, 1, mbus_disc_actions_3}, {4, 1, mbus_disc_actions_4},
    {5, 1, mbus_disc_actions_5}, {6, 1, mbus_disc_actions_6},
    {7, 1, mbus_disc_actions_7}, {8, 1, mbus_disc_actions_8},
};

/* M-Bus function control script table. */
static const nd_script_action_t mbus_fc_actions_1[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_MBUS_FUNCTION_CONTROL, 1, 16,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x08, 'L', 'P', 'C', 'A', 'P', '_', 'M', '1',
          0x03, 0x01),
};
static const nd_script_action_t mbus_fc_actions_2[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_MBUS_FUNCTION_CONTROL, 1, 16,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x08, 'L', 'P', 'C', 'A', 'P', '_', 'M', '1',
          0x03, 0x00),
};
static const nd_script_action_t mbus_fc_actions_3[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_MBUS_FUNCTION_CONTROL, 1, 16,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x08, 'L', 'P', 'C', 'A', 'P', '_', 'M', '2',
          0x03, 0x01),
};
static const nd_script_action_t mbus_fc_actions_4[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_MBUS_FUNCTION_CONTROL, 1, 16,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x08, 'L', 'P', 'C', 'A', 'P', '_', 'M', '2',
          0x03, 0x00),
};
static const nd_script_action_t mbus_fc_actions_5[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_MBUS_FUNCTION_CONTROL, 1, 16,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x08, 'L', 'P', 'C', 'A', 'P', '_', 'M', '3',
          0x03, 0x01),
};
static const nd_script_action_t mbus_fc_actions_6[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_MBUS_FUNCTION_CONTROL, 1, 16,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x08, 'L', 'P', 'C', 'A', 'P', '_', 'M', '3',
          0x03, 0x00),
};
static const nd_script_action_t mbus_fc_actions_7[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_MBUS_FUNCTION_CONTROL, 1, 16,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x08, 'L', 'P', 'C', 'A', 'P', '_', 'M', '4',
          0x03, 0x01),
};
static const nd_script_action_t mbus_fc_actions_8[] = {
    A_ACT(ND_IC_FUNCTION_CONTROL, ND_OBIS_MBUS_FUNCTION_CONTROL, 1, 16,
          0x01, 0x01, 0x02, 0x02, 0x0A, 0x08, 'L', 'P', 'C', 'A', 'P', '_', 'M', '4',
          0x03, 0x00),
};
static const nd_script_t mbus_fc_scripts[] = {
    {1, 1, mbus_fc_actions_1}, {2, 1, mbus_fc_actions_2},
    {3, 1, mbus_fc_actions_3}, {4, 1, mbus_fc_actions_4},
    {5, 1, mbus_fc_actions_5}, {6, 1, mbus_fc_actions_6},
    {7, 1, mbus_fc_actions_7}, {8, 1, mbus_fc_actions_8},
};

static const nd_script_table_t predefined_tables[] = {
    {ND_OBIS_SCRIPT_TARIFF, 4, tariff_scripts},
    {ND_OBIS_SCRIPT_MDI, 1, mdi_scripts},
    {ND_OBIS_SCRIPT_DISCONNECT, 4, disconnect_scripts},
    {ND_OBIS_SCRIPT_LOAD_MGMT, 8, load_scripts},
    {ND_OBIS_SCRIPT_IMAGE, 1, image_scripts},
    {ND_OBIS_SCRIPT_PUSH, 8, push_scripts},
    {ND_OBIS_SCRIPT_FUNCTION_CONTROL, 8, fc_scripts},
    {ND_OBIS_SCRIPT_MBUS_DISCONNECT, 8, mbus_disc_scripts},
    {ND_OBIS_SCRIPT_MBUS_FUNCTION_CONTROL, 8, mbus_fc_scripts},
    /* M-Bus profile control table content is not printed in the profile. */
    {ND_OBIS_SCRIPT_MBUS_PROFILE_CONTROL, 0, NULL},
};

void nd_idis_script_init(nd_script_state_t *state)
{
    if (state == NULL) {
        return;
    }
    state->tables = predefined_tables;
    state->count = (uint8_t)(sizeof(predefined_tables)
                             / sizeof(predefined_tables[0]));
    state->port = NULL;
    state->store = NULL;
    state->store_offset = 0;
    for (uint8_t index = 0; index < ND_SCRIPT_TABLE_MAX; index++) {
        state->ram_valid[index] = 0;
        state->ram_script_count[index] = 0;
    }
}

void nd_idis_script_bind_port(nd_script_state_t *state,
                              const nd_cosem_port_t *port)
{
    if (state != NULL) {
        state->port = port;
    }
}

void nd_idis_script_bind_store(nd_script_state_t *state, nd_store_t *store,
                               uint32_t offset)
{
    if (state != NULL) {
        state->store = store;
        state->store_offset = offset;
    }
}

int nd_idis_script_save(const nd_script_state_t *state, nd_store_t *store,
                        uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    uint32_t magic = ND_IDIS_SCRIPT_MAGIC;
    if (nd_store_write(store, offset, (const uint8_t *)&magic,
                       sizeof(magic)) != 0
        || nd_store_write(store,
                          offset
                              + offsetof(nd_idis_script_record_t, ram_valid),
                          state->ram_valid, sizeof(state->ram_valid)) != 0
        || nd_store_write(
               store,
               offset + offsetof(nd_idis_script_record_t, ram_script_count),
               state->ram_script_count, sizeof(state->ram_script_count)) != 0
        || nd_store_write(store,
                          offset + offsetof(nd_idis_script_record_t,
                                            ram_scripts),
                          (const uint8_t *)state->ram_scripts,
                          sizeof(state->ram_scripts)) != 0
        || nd_store_write(store,
                          offset + offsetof(nd_idis_script_record_t,
                                            ram_actions),
                          (const uint8_t *)state->ram_actions,
                          sizeof(state->ram_actions)) != 0) {
        return -1;
    }
    return 0;
}

static int record_is_valid(const nd_idis_script_record_t *record)
{
    for (uint8_t table = 0; table < ND_SCRIPT_TABLE_MAX; table++) {
        if (record->ram_valid[table] > 1
            || record->ram_script_count[table] > ND_SCRIPT_RAM_SCRIPTS) {
            return 0;
        }
        for (uint8_t script = 0;
             script < record->ram_script_count[table]; script++) {
            const nd_script_slot_t *slot = &record->ram_scripts[table][script];
            if (slot->action_count > ND_SCRIPT_RAM_ACTIONS
                || slot->first_action > ND_SCRIPT_RAM_ACTIONS
                || slot->action_count
                       > ND_SCRIPT_RAM_ACTIONS - slot->first_action) {
                return 0;
            }
        }
    }
    return 1;
}

int nd_idis_script_load(nd_script_state_t *state, nd_store_t *store,
                        uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_script_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_SCRIPT_MAGIC
        || !record_is_valid(&record)) {
        return -1;
    }
    for (uint8_t table = 0; table < ND_SCRIPT_TABLE_MAX; table++) {
        state->ram_valid[table] = record.ram_valid[table];
        state->ram_script_count[table] = record.ram_script_count[table];
        if (!state->ram_valid[table]) {
            continue;
        }
        memcpy(state->ram_scripts[table], record.ram_scripts[table],
               sizeof(state->ram_scripts[table]));
        memcpy(state->ram_actions[table], record.ram_actions[table],
               sizeof(state->ram_actions[table]));
    }
    return 0;
}

static int find_table_index(const nd_script_state_t *state,
                            const nd_obis_t *ln)
{
    for (uint8_t index = 0; state != NULL && index < state->count; index++) {
        if (nd_obis_eq(ln, &state->tables[index].ln)) {
            return index;
        }
    }
    return -1;
}

static uint8_t table_script_count(const nd_script_state_t *state,
                                  uint8_t table)
{
    return state->ram_valid[table] ? state->ram_script_count[table]
                                   : state->tables[table].script_count;
}

static uint16_t table_script_id(const nd_script_state_t *state, uint8_t table,
                                uint8_t index)
{
    return state->ram_valid[table] ? state->ram_scripts[table][index].id
                                   : state->tables[table].scripts[index].id;
}

static uint8_t table_script_actions(const nd_script_state_t *state,
                                    uint8_t table, uint8_t index,
                                    const nd_script_action_t **actions)
{
    if (state->ram_valid[table]) {
        const nd_script_slot_t *slot = &state->ram_scripts[table][index];
        *actions = slot->action_count > 0
            ? &state->ram_actions[table][slot->first_action]
            : NULL;
        return slot->action_count;
    }
    const nd_script_t *script = &state->tables[table].scripts[index];
    *actions = script->actions;
    return script->action_count;
}

static void put_logical_name(nd_axdr_writer_t *out, const nd_obis_t *ln)
{
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, 6);
    nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
}

int nd_ic_script_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                     uint8_t attribute_id, const nd_axdr_reader_t *selector,
                     nd_axdr_writer_t *out)
{
    nd_script_state_t *state = ctx;
    (void)client_sap;
    (void)selector;
    if (state == NULL || ln == NULL || out == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    const nd_script_table_t *table = NULL;
    int table_index = find_table_index(state, ln);
    if (table_index >= 0) {
        table = &state->tables[table_index];
    }
    if (table == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id == 1) {
        put_logical_name(out, ln);
        return nd_axdr_writer_ok(out) ? ND_OK
                                      : ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    if (attribute_id != 2) {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    uint8_t script_count = table_script_count(state, (uint8_t)table_index);
    nd_axdr_put_array_header(out, script_count);
    for (uint8_t index = 0; index < script_count; index++) {
        const nd_script_action_t *actions = NULL;
        uint8_t action_count = table_script_actions(
            state, (uint8_t)table_index, index, &actions);
        nd_axdr_put_structure_header(out, 2);
        nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(out, table_script_id(state, (uint8_t)table_index,
                                             index));
        nd_axdr_put_array_header(out, action_count);
        for (uint8_t action = 0; action < action_count; action++) {
            const nd_script_action_t *item = &actions[action];
            nd_axdr_put_structure_header(out, 5);
            nd_axdr_put_tag(out, ND_AXDR_ENUM);
            nd_axdr_put_u8(out, item->service_id);
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, item->class_id);
            put_logical_name(out, &item->ln);
            nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
            nd_axdr_put_u16(out, item->member);
            nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
            nd_axdr_put_length(out, item->param_len);
            nd_axdr_put_octets(out, item->param, item->param_len);
        }
    }
    return nd_axdr_writer_ok(out) ? ND_OK : ND_ERR_OPERATION_NOT_POSSIBLE;
}

static int read_script_id(nd_axdr_reader_t *in, uint16_t *id)
{
    if (in == NULL) {
        return -1;
    }
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag == ND_AXDR_UNSIGNED) {
        *id = nd_axdr_get_u8(in);
    } else if (tag == ND_AXDR_LONG_UNSIGNED) {
        *id = nd_axdr_get_u16(in);
    } else if (tag == ND_AXDR_DOUBLE_LONG_UNSIGNED) {
        uint32_t value = nd_axdr_get_u32(in);
        if (value > 0xFFFF) {
            return -1;
        }
        *id = (uint16_t)value;
    } else {
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int read_index_value(nd_axdr_reader_t *in, uint32_t *value)
{
    uint8_t tag = nd_axdr_get_tag(in);
    if (tag == ND_AXDR_UNSIGNED) {
        *value = nd_axdr_get_u8(in);
    } else if (tag == ND_AXDR_LONG_UNSIGNED) {
        *value = nd_axdr_get_u16(in);
    } else if (tag == ND_AXDR_DOUBLE_LONG_UNSIGNED) {
        *value = nd_axdr_get_u32(in);
    } else {
        return -1;
    }
    return nd_axdr_reader_ok(in) ? 0 : -1;
}

static int parse_action(nd_axdr_reader_t *in, nd_script_action_t *out)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
        || nd_axdr_get_length(in) != 5) {
        return -1;
    }
    if (nd_axdr_get_tag(in) != ND_AXDR_ENUM) {
        return -1;
    }
    uint8_t service = nd_axdr_get_u8(in);
    if (service < 1 || service > 2) {
        return -1;
    }
    uint32_t class_id = 0;
    if (read_index_value(in, &class_id) != 0 || class_id > 0xFFFF) {
        return -1;
    }
    if (nd_axdr_get_tag(in) != ND_AXDR_OCTET_STRING
        || nd_axdr_get_length(in) != 6) {
        return -1;
    }
    uint8_t ln[6];
    nd_axdr_get_octets(in, ln, sizeof(ln));
    uint32_t member = 0;
    if (read_index_value(in, &member) != 0 || member < 1 || member > 0xFF) {
        return -1;
    }
    uint8_t tag = nd_axdr_get_tag(in);
    uint8_t param[ND_SCRIPT_PARAM_MAX];
    uint8_t param_len = 0;
    if (tag == ND_AXDR_OCTET_STRING) {
        uint16_t length = nd_axdr_get_length(in);
        if (length > ND_SCRIPT_PARAM_MAX) {
            return -1;
        }
        nd_axdr_get_octets(in, param, length);
        if (!nd_axdr_reader_ok(in)) {
            return -1;
        }
        if (length > 0) {
            nd_axdr_reader_t check;
            nd_axdr_reader_init(&check, param, length);
            if (nd_axdr_skip_value(&check) != 0
                || nd_axdr_remaining(&check) != 0) {
                return -1;
            }
        }
        param_len = (uint8_t)length;
    } else if (tag != ND_AXDR_NULL) {
        return -1;
    }
    if (!nd_axdr_reader_ok(in)) {
        return -1;
    }
    if (out != NULL) {
        out->service_id = service;
        out->class_id = (uint16_t)class_id;
        memcpy(&out->ln, ln, sizeof(ln));
        out->member = (uint8_t)member;
        out->param_len = param_len;
        memcpy(out->param, param, param_len);
    }
    return 0;
}

/*
 * Parses a scripts array in the same shape attribute 2 is read with. With
 * store == 0 it only validates, so a rejected SET leaves the old table intact.
 */
static int parse_scripts(nd_axdr_reader_t *in, nd_script_state_t *state,
                         uint8_t table, int store)
{
    if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
        return -1;
    }
    uint16_t count = nd_axdr_get_length(in);
    if (count > ND_SCRIPT_RAM_SCRIPTS) {
        return -1;
    }
    uint16_t ids[ND_SCRIPT_RAM_SCRIPTS];
    uint8_t actions_used = 0;
    for (uint16_t index = 0; index < count; index++) {
        if (nd_axdr_get_tag(in) != ND_AXDR_STRUCTURE
            || nd_axdr_get_length(in) != 2
            || nd_axdr_get_tag(in) != ND_AXDR_LONG_UNSIGNED) {
            return -1;
        }
        uint16_t id = nd_axdr_get_u16(in);
        for (uint16_t previous = 0; previous < index; previous++) {
            if (ids[previous] == id) {
                return -1;
            }
        }
        ids[index] = id;
        if (nd_axdr_get_tag(in) != ND_AXDR_ARRAY) {
            return -1;
        }
        uint16_t actions = nd_axdr_get_length(in);
        if (actions > ND_SCRIPT_RAM_ACTIONS - actions_used) {
            return -1;
        }
        for (uint16_t action = 0; action < actions; action++) {
            nd_script_action_t *slot =
                store ? &state->ram_actions[table][actions_used + action]
                      : NULL;
            if (parse_action(in, slot) != 0) {
                return -1;
            }
        }
        if (store) {
            state->ram_scripts[table][index] = (nd_script_slot_t){
                .id = id,
                .action_count = (uint8_t)actions,
                .first_action = actions_used,
            };
        }
        actions_used = (uint8_t)(actions_used + actions);
    }
    if (!nd_axdr_reader_ok(in) || nd_axdr_remaining(in) != 0) {
        return -1;
    }
    if (store) {
        state->ram_script_count[table] = (uint8_t)count;
        state->ram_valid[table] = 1;
    }
    return 0;
}

int nd_ic_script_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                     uint8_t attribute_id, nd_axdr_reader_t *in)
{
    nd_script_state_t *state = ctx;
    (void)client_sap;
    if (state == NULL || ln == NULL || in == NULL) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    if (attribute_id != 2) {
        return ND_ERR_ATTRIBUTE_UNDEFINED;
    }
    int table = find_table_index(state, ln);
    if (table < 0) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    nd_axdr_reader_t check = *in;
    if (parse_scripts(&check, NULL, 0, 0) != 0) {
        return ND_ERR_TYPE_MISMATCH;
    }
    if (parse_scripts(in, state, (uint8_t)table, 1) != 0) {
        return ND_ERR_TYPE_MISMATCH;
    }
    if (state->store != NULL
        && nd_idis_script_save(state, state->store, state->store_offset) != 0) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    return ND_OK;
}

int nd_idis_script_count(const nd_script_state_t *state,
                         const nd_obis_t *table_ln)
{
    if (state == NULL || table_ln == NULL) {
        return -1;
    }
    int table = find_table_index(state, table_ln);
    return table < 0 ? -1 : table_script_count(state, (uint8_t)table);
}

int nd_idis_script_id(const nd_script_state_t *state, const nd_obis_t *table_ln,
                      uint8_t index)
{
    if (state == NULL || table_ln == NULL) {
        return -1;
    }
    int table = find_table_index(state, table_ln);
    if (table < 0 || index >= table_script_count(state, (uint8_t)table)) {
        return -1;
    }
    return (int)table_script_id(state, (uint8_t)table, index);
}

int nd_idis_script_execute(const nd_script_state_t *state,
                           const nd_cosem_port_t *port, uint8_t client_sap,
                           const nd_obis_t *table_ln, uint16_t script_id)
{
    if (state == NULL || port == NULL || table_ln == NULL
        || (port->set == NULL && port->action == NULL)) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    int table = find_table_index(state, table_ln);
    if (table < 0) {
        return ND_ERR_OBJECT_UNDEFINED;
    }
    uint8_t script_count = table_script_count(state, (uint8_t)table);
    for (uint8_t index = 0; index < script_count; index++) {
        if (table_script_id(state, (uint8_t)table, index) != script_id) {
            continue;
        }
        const nd_script_action_t *actions = NULL;
        uint8_t action_count = table_script_actions(
            state, (uint8_t)table, index, &actions);
        for (uint8_t action = 0; action < action_count; action++) {
            const nd_script_action_t *item = &actions[action];
            nd_axdr_reader_t reader;
            nd_axdr_reader_init(&reader, item->param, item->param_len);
            uint8_t scratch[16];
            nd_axdr_writer_t out;
            nd_axdr_writer_init(&out, scratch, sizeof(scratch));
            int result;
            if (item->service_id == 1 && port->set != NULL) {
                result = port->set(port->ctx, client_sap, item->class_id,
                                   &item->ln, item->member, &reader);
            } else if (item->service_id == 2 && port->action != NULL) {
                result = port->action(port->ctx, client_sap, item->class_id,
                                      &item->ln, item->member, &reader, &out);
            } else {
                result = ND_ERR_OPERATION_NOT_POSSIBLE;
            }
            if (result != ND_OK) {
                return result;
            }
        }
        return ND_OK;
    }
    return ND_ERR_OPERATION_NOT_POSSIBLE;
}

int nd_ic_script_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                        uint8_t method_id, nd_axdr_reader_t *in,
                        nd_axdr_writer_t *out)
{
    nd_script_state_t *state = ctx;
    (void)out;
    if (state == NULL || ln == NULL || method_id != 1) {
        return ND_ERR_OPERATION_NOT_POSSIBLE;
    }
    uint16_t script_id;
    if (read_script_id(in, &script_id) != 0) {
        return ND_ERR_TYPE_MISMATCH;
    }
    return nd_idis_script_execute(state, state->port, client_sap, ln,
                                  script_id);
}
