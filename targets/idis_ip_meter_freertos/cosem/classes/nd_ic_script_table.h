#ifndef ND_IC_SCRIPT_TABLE_H
#define ND_IC_SCRIPT_TABLE_H

#include <stdint.h>

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/store.h"

#define ND_SCRIPT_PARAM_MAX 32
#define ND_SCRIPT_TABLE_MAX 10
#define ND_SCRIPT_RAM_SCRIPTS 8
#define ND_SCRIPT_RAM_ACTIONS 12

typedef struct {
    uint8_t service_id; /* 1 = SET, 2 = ACTION */
    uint16_t class_id;
    nd_obis_t ln;
    uint8_t member; /* attribute or method identifier */
    uint8_t param_len;
    uint8_t param[ND_SCRIPT_PARAM_MAX];
} nd_script_action_t;

typedef struct {
    uint16_t id;
    uint8_t action_count;
    const nd_script_action_t *actions;
} nd_script_t;

typedef struct {
    nd_obis_t ln;
    uint8_t script_count;
    const nd_script_t *scripts;
} nd_script_table_t;

typedef struct {
    uint16_t id;
    uint8_t action_count;
    uint8_t first_action;
} nd_script_slot_t;

typedef struct {
    const nd_script_table_t *tables;
    uint8_t count;
    const nd_cosem_port_t *port;
    /* RAM overrides written through attribute 2; a valid table replaces the
     * predefined script list until the next reset. */
    uint8_t ram_valid[ND_SCRIPT_TABLE_MAX];
    uint8_t ram_script_count[ND_SCRIPT_TABLE_MAX];
    nd_script_slot_t ram_scripts[ND_SCRIPT_TABLE_MAX][ND_SCRIPT_RAM_SCRIPTS];
    nd_script_action_t ram_actions[ND_SCRIPT_TABLE_MAX][ND_SCRIPT_RAM_ACTIONS];
    /* When bound, a successful attribute 2 SET is written here immediately. */
    nd_store_t *store;
    uint32_t store_offset;
} nd_script_state_t;

/* Uses the predefined Pack3 script tables until attribute 2 is written. */
void nd_idis_script_init(nd_script_state_t *state);

void nd_idis_script_bind_port(nd_script_state_t *state,
                              const nd_cosem_port_t *port);

/* Binds the parameter store used to persist RAM overrides. */
void nd_idis_script_bind_store(nd_script_state_t *state, nd_store_t *store,
                               uint32_t offset);

int nd_idis_script_save(const nd_script_state_t *state, nd_store_t *store,
                        uint32_t offset);
int nd_idis_script_load(nd_script_state_t *state, nd_store_t *store,
                        uint32_t offset);

int nd_ic_script_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                     uint8_t attribute_id, const nd_axdr_reader_t *selector,
                     nd_axdr_writer_t *out);
int nd_ic_script_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                     uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_ic_script_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                        uint8_t method_id, nd_axdr_reader_t *in,
                        nd_axdr_writer_t *out);

/* Runs every action of one script through the COSEM port. */
int nd_idis_script_count(const nd_script_state_t *state,
                         const nd_obis_t *table_ln);
int nd_idis_script_id(const nd_script_state_t *state, const nd_obis_t *table_ln,
                      uint8_t index);
int nd_idis_script_execute(const nd_script_state_t *state,
                           const nd_cosem_port_t *port, uint8_t client_sap,
                           const nd_obis_t *table_ln, uint16_t script_id);

#endif
