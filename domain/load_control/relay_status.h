#ifndef ND_LOAD_CONTROL_RELAY_STATUS_H
#define ND_LOAD_CONTROL_RELAY_STATUS_H

typedef struct nd_relay_status_port {
    void *ctx;
    int (*is_off)(void *ctx);
} nd_relay_status_port_t;

void nd_relay_status_bind(const nd_relay_status_port_t *port);
int nd_relay_status_is_off(void);

#endif
