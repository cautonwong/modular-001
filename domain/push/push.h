#ifndef ND_DOMAIN_PUSH_PUSH_H
#define ND_DOMAIN_PUSH_PUSH_H

#include <stddef.h>
#include <stdint.h>

#include "domain/common/calendar.h"

#define ND_PUSH_SETUPS 8
#define ND_PUSH_DESTINATION_MAX 32

typedef struct {
    uint8_t transport_service; /* enum: 0 TCP, 1 UDP, 4 SMS, 5 HDLC, ... */
    uint8_t message_type;      /* enum: 4 A-XDR encoded xDLMS APDU */
    uint8_t destination[ND_PUSH_DESTINATION_MAX];
    uint8_t destination_length;
} nd_push_destination_t;

typedef struct {
    /* Set by the COSEM layer when push_object_list is not empty. */
    uint8_t enabled;
    nd_push_destination_t destination;
    uint8_t port_reference[6];
    uint8_t push_client_sap;
    uint16_t randomisation_start_interval;
    uint8_t number_of_retries;
    uint32_t repetition_delay;
    uint32_t attempts;
    uint32_t pushes; /* confirmed attempts */
    uint32_t failures;
    uint8_t pending;      /* a retry is scheduled */
    uint8_t retries_left;
    uint16_t retry_countdown; /* seconds until the next attempt */
    nd_datetime_t last_confirmation;
    uint8_t has_confirmation;
} nd_push_setup_t;

/*
 * Transport adapter. The adapter builds the Data-Notification APDU from the
 * setup and returns 0 when the lower layer confirmed the transmission.
 */
typedef struct {
    void *ctx;
    int (*send)(void *ctx, uint8_t setup_index, const nd_push_setup_t *setup);
} nd_push_port_t;

typedef struct {
    nd_push_setup_t setups[ND_PUSH_SETUPS];
    const nd_push_port_t *port;
} nd_push_state_t;

void nd_push_init(nd_push_state_t *state);
void nd_push_bind_port(nd_push_state_t *state, const nd_push_port_t *port);

/* push (data) method: an object list of 0 entries disables the setup. */
int nd_push_trigger(nd_push_state_t *state, uint8_t setup_index,
                    const nd_datetime_t *now);
/* Processes scheduled retries; returns the number of attempts made. */
int nd_push_on_second(nd_push_state_t *state, const nd_datetime_t *now);
/* reset (data): clears counters and any pending retry. */
int nd_push_reset(nd_push_state_t *state, uint8_t setup_index);

#endif
