#include "domain/mbus/mbus_client.h"

#include <string.h>

void nd_mbus_init(nd_mbus_state_t *state)
{
    if (state != NULL) {
        memset(state, 0, sizeof(*state));
    }
}

void nd_mbus_bind_port(nd_mbus_state_t *state, const nd_mbus_port_t *port)
{
    if (state != NULL) {
        state->port = port;
    }
}

static nd_mbus_client_t *client_for(nd_mbus_state_t *state, uint8_t channel)
{
    if (state == NULL || channel < 1 || channel > ND_MBUS_CHANNELS) {
        return NULL;
    }
    return &state->clients[channel - 1];
}

static int command(nd_mbus_state_t *state, uint8_t channel,
                   nd_mbus_command_t command, uint8_t argument)
{
    if (state->port == NULL || state->port->command == NULL) {
        return -1;
    }
    return state->port->command(state->port->ctx, channel, command, argument);
}

static uint8_t free_address(const nd_mbus_state_t *state)
{
    for (uint16_t candidate = 1; candidate <= 250; candidate++) {
        int used = 0;
        for (uint8_t channel = 0; channel < ND_MBUS_CHANNELS; channel++) {
            if (state->clients[channel].primary_address == candidate) {
                used = 1;
                break;
            }
        }
        if (!used) {
            return (uint8_t)candidate;
        }
    }
    return 0;
}

int nd_mbus_install(nd_mbus_state_t *state, uint8_t channel, uint8_t address)
{
    nd_mbus_client_t *client = client_for(state, channel);
    if (client == NULL || address > 250 || client->primary_address != 0) {
        return -1;
    }
    uint8_t selected = address;
    if (selected == 0) {
        selected = free_address(state);
        if (selected == 0) {
            return -1;
        }
    }
    if (command(state, channel, ND_MBUS_COMMAND_INSTALL, selected) != 0) {
        return -1;
    }
    client->primary_address = selected;
    client->identification_number = 0;
    client->manufacturer_id = 0;
    client->version = 0;
    client->device_type = 0;
    client->access_number = 0;
    client->status = 0;
    client->alarm = 0;
    client->configuration = 0;
    return 0;
}

int nd_mbus_deinstall(nd_mbus_state_t *state, uint8_t channel)
{
    nd_mbus_client_t *client = client_for(state, channel);
    if (client == NULL || client->primary_address == 0) {
        return -1;
    }
    if (command(state, channel, ND_MBUS_COMMAND_DEINSTALL, 0) != 0) {
        return -1;
    }
    uint8_t port_index = client->port_index;
    memset(client, 0, sizeof(*client));
    client->port_index = port_index;
    return 0;
}

static int same_identity(const nd_mbus_client_t *client,
                         const nd_mbus_identity_t *identity)
{
    return client->identification_number == identity->identification_number
        && client->manufacturer_id == identity->manufacturer_id
        && client->version == identity->version
        && client->device_type == identity->device_type;
}

int nd_mbus_capture(nd_mbus_state_t *state, uint8_t channel,
                    nd_mbus_readout_t *readout)
{
    nd_mbus_client_t *client = client_for(state, channel);
    if (client == NULL || client->primary_address == 0) {
        return -1;
    }
    if (state->port == NULL || state->port->read == NULL) {
        return -1;
    }
    nd_mbus_readout_t value;
    if (state->port->read(state->port->ctx, channel, &value) != 0
        || value.value_count > ND_MBUS_VALUES) {
        return -1;
    }
    if (client->identification_number != 0
        && !same_identity(client, &value.identity)) {
        return -1;
    }
    client->identification_number = value.identity.identification_number;
    client->manufacturer_id = value.identity.manufacturer_id;
    client->version = value.identity.version;
    client->device_type = value.identity.device_type;
    client->access_number = value.access_number;
    client->status = value.status;
    client->alarm = value.alarm;
    client->configuration = value.configuration;
    if (client->key_status == 3) {
        client->key_status = 4; /* key used by the last encrypted readout */
    }
    if (readout != NULL) {
        *readout = value;
    }
    return 0;
}

int nd_mbus_reset_alarm(nd_mbus_state_t *state, uint8_t channel)
{
    nd_mbus_client_t *client = client_for(state, channel);
    if (client == NULL || client->primary_address == 0) {
        return -1;
    }
    if (command(state, channel, ND_MBUS_COMMAND_RESET_ALARM, 0) != 0) {
        return -1;
    }
    client->alarm = 0;
    return 0;
}

int nd_mbus_sync_clock(nd_mbus_state_t *state, uint8_t channel)
{
    nd_mbus_client_t *client = client_for(state, channel);
    if (client == NULL || client->primary_address == 0) {
        return -1;
    }
    return command(state, channel, ND_MBUS_COMMAND_SYNC_CLOCK, 0);
}

int nd_mbus_data_send(nd_mbus_state_t *state, uint8_t channel,
                      const uint8_t *data, size_t length)
{
    nd_mbus_client_t *client = client_for(state, channel);
    if (client == NULL || client->primary_address == 0 || data == NULL
        || length == 0) {
        return -1;
    }
    if (state->port == NULL || state->port->write == NULL) {
        return -1;
    }
    return state->port->write(state->port->ctx, channel, data, length);
}

static void update_key_status(nd_mbus_client_t *client)
{
    if (client->key_set && client->key_transferred) {
        client->key_status = 3;
    } else if (client->key_set) {
        client->key_status = 1;
    } else if (client->key_transferred) {
        client->key_status = 2;
    } else {
        client->key_status = 0;
    }
}

int nd_mbus_set_key(nd_mbus_state_t *state, uint8_t channel,
                    const uint8_t *key, size_t length)
{
    nd_mbus_client_t *client = client_for(state, channel);
    if (client == NULL || length > ND_MBUS_KEY_MAX
        || (length > 0 && key == NULL)) {
        return -1;
    }
    memset(client->key, 0, sizeof(client->key));
    if (length > 0) {
        memcpy(client->key, key, length);
    }
    client->key_set = length > 0 ? 1 : 0;
    update_key_status(client);
    return 0;
}

int nd_mbus_transfer_key(nd_mbus_state_t *state, uint8_t channel,
                         const uint8_t *key, size_t length)
{
    nd_mbus_client_t *client = client_for(state, channel);
    (void)key;
    if (client == NULL || length > ND_MBUS_KEY_MAX) {
        return -1;
    }
    /* The transferred bytes are the operational key encrypted with the
     * default key; only the fact of the transfer is tracked here. */
    client->key_transferred = length > 0 ? 1 : 0;
    update_key_status(client);
    return 0;
}
