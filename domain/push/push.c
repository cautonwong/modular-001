#include "domain/push/push.h"

#include <string.h>

void nd_push_init(nd_push_state_t *state)
{
    if (state != NULL) {
        memset(state, 0, sizeof(*state));
    }
}

void nd_push_bind_port(nd_push_state_t *state, const nd_push_port_t *port)
{
    if (state != NULL) {
        state->port = port;
    }
}

static nd_push_setup_t *setup_for(nd_push_state_t *state, uint8_t index)
{
    if (state == NULL || index >= ND_PUSH_SETUPS) {
        return NULL;
    }
    return &state->setups[index];
}

static void schedule_retry(nd_push_setup_t *setup)
{
    if (setup->retries_left == 0 || setup->number_of_retries == 0) {
        setup->pending = 0;
        return;
    }
    setup->pending = 1;
    setup->retry_countdown =
        setup->repetition_delay == 0 ? 1 : (uint16_t)setup->repetition_delay;
}

static int attempt(nd_push_state_t *state, uint8_t index,
                   nd_push_setup_t *setup, const nd_datetime_t *now)
{
    setup->attempts++;
    int confirmed = state->port != NULL && state->port->send != NULL
        && state->port->send(state->port->ctx, index, setup) == 0;
    if (confirmed) {
        setup->pushes++;
        setup->pending = 0;
        setup->retries_left = 0;
        setup->has_confirmation = 1;
        if (now != NULL) {
            setup->last_confirmation = *now;
        }
        return 0;
    }
    setup->failures++;
    schedule_retry(setup);
    return -1;
}

int nd_push_trigger(nd_push_state_t *state, uint8_t setup_index,
                    const nd_datetime_t *now)
{
    nd_push_setup_t *setup = setup_for(state, setup_index);
    if (setup == NULL || !setup->enabled) {
        return -1;
    }
    setup->retries_left = setup->number_of_retries;
    return attempt(state, setup_index, setup, now);
}

int nd_push_on_second(nd_push_state_t *state, const nd_datetime_t *now)
{
    int made = 0;
    for (uint8_t index = 0; state != NULL && index < ND_PUSH_SETUPS; index++) {
        nd_push_setup_t *setup = &state->setups[index];
        if (!setup->pending) {
            continue;
        }
        if (setup->retry_countdown > 0) {
            setup->retry_countdown--;
        }
        if (setup->retry_countdown > 0) {
            continue;
        }
        setup->retries_left--;
        (void)attempt(state, index, setup, now);
        made++;
    }
    return made;
}

int nd_push_reset(nd_push_state_t *state, uint8_t setup_index)
{
    nd_push_setup_t *setup = setup_for(state, setup_index);
    if (setup == NULL) {
        return -1;
    }
    setup->attempts = 0;
    setup->pushes = 0;
    setup->failures = 0;
    setup->pending = 0;
    setup->retries_left = 0;
    setup->retry_countdown = 0;
    setup->has_confirmation = 0;
    setup->last_confirmation = (nd_datetime_t){0};
    return 0;
}
