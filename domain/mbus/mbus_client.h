#ifndef ND_DOMAIN_MBUS_CLIENT_H
#define ND_DOMAIN_MBUS_CLIENT_H

#include <stddef.h>
#include <stdint.h>

#include "domain/common/calendar.h"

#define ND_MBUS_CHANNELS 4
#define ND_MBUS_VALUES 8
#define ND_MBUS_KEY_MAX 16

typedef struct {
    uint8_t primary_address;
    uint32_t identification_number;
    uint16_t manufacturer_id;
    uint8_t version;
    uint8_t device_type;
} nd_mbus_identity_t;

typedef struct {
    int32_t value;
    int8_t scaler;
    uint8_t unit;
} nd_mbus_value_t;

/* One readout of an M-Bus slave, already converted to COSEM units. */
typedef struct {
    nd_mbus_identity_t identity;
    nd_datetime_t time;
    uint8_t access_number;
    uint8_t status;
    uint8_t alarm;
    uint16_t configuration;
    uint8_t value_count;
    nd_mbus_value_t values[ND_MBUS_VALUES];
} nd_mbus_readout_t;

typedef enum {
    ND_MBUS_COMMAND_INSTALL = 1,
    ND_MBUS_COMMAND_DEINSTALL,
    ND_MBUS_COMMAND_RESET_ALARM,
    ND_MBUS_COMMAND_SYNC_CLOCK
} nd_mbus_command_t;

/*
 * Link port. The target binds this to a wired or wireless M-Bus driver; the
 * domain only sees readouts and commands, never EN 13757 frames.
 */
typedef struct {
    void *ctx;
    int (*read)(void *ctx, uint8_t channel, nd_mbus_readout_t *out);
    int (*write)(void *ctx, uint8_t channel, const uint8_t *data,
                 size_t length);
    int (*command)(void *ctx, uint8_t channel, nd_mbus_command_t command,
                   uint8_t argument);
} nd_mbus_port_t;

typedef struct {
    uint8_t primary_address;
    uint8_t port_index; /* 1-based M-Bus master port setup reference */
    uint32_t identification_number;
    uint16_t manufacturer_id;
    uint8_t version;
    uint8_t device_type;
    uint8_t access_number;
    uint8_t status;
    uint8_t alarm;
    uint16_t configuration;
    uint32_t capture_period;
    uint8_t key_status; /* 0 none, 1 set, 2 transferred, 3 set+transferred, 4 in use */
    uint8_t key_set;
    uint8_t key_transferred;
    uint8_t key[ND_MBUS_KEY_MAX];
} nd_mbus_client_t;

typedef struct {
    nd_mbus_client_t clients[ND_MBUS_CHANNELS];
    const nd_mbus_port_t *port;
} nd_mbus_state_t;

void nd_mbus_init(nd_mbus_state_t *state);
void nd_mbus_bind_port(nd_mbus_state_t *state, const nd_mbus_port_t *port);

/*
 * Installs a slave on a channel. address 0 picks the first free primary
 * address (1..250). Fails when the channel already has an address or no slave
 * answers.
 */
int nd_mbus_install(nd_mbus_state_t *state, uint8_t channel, uint8_t address);
int nd_mbus_deinstall(nd_mbus_state_t *state, uint8_t channel);

/* Reads the slave and copies the identity/status and values. */
int nd_mbus_capture(nd_mbus_state_t *state, uint8_t channel,
                    nd_mbus_readout_t *readout);

int nd_mbus_reset_alarm(nd_mbus_state_t *state, uint8_t channel);
int nd_mbus_sync_clock(nd_mbus_state_t *state, uint8_t channel);
int nd_mbus_data_send(nd_mbus_state_t *state, uint8_t channel,
                      const uint8_t *data, size_t length);

int nd_mbus_set_key(nd_mbus_state_t *state, uint8_t channel,
                    const uint8_t *key, size_t length);
int nd_mbus_transfer_key(nd_mbus_state_t *state, uint8_t channel,
                         const uint8_t *key, size_t length);

#endif
