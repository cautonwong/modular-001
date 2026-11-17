#ifndef ND_APPLICATION_PORTS_UART_TIMEOUT_H
#define ND_APPLICATION_PORTS_UART_TIMEOUT_H

#include <stdint.h>

/* The target owns the UART receive queues; protocol components hand it the
 * product context and the channel id. */
int fr_uart_timeout(void *ctx, uint8_t pid);

#endif
