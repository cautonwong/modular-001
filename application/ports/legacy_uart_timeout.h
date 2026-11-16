#ifndef ND_COMPONENTS_PROTOCOL_LEGACY_UART_TIMEOUT_H
#define ND_COMPONENTS_PROTOCOL_LEGACY_UART_TIMEOUT_H

struct instance_t;
struct pipeline_t;

/* Implemented by the stategrid RTOS entry. Declared here so the protocol
 * components do not repeat it implicitly at each call site. */
int fr_uart_timeout(struct instance_t *instance, struct pipeline_t *pipeline);

#endif
