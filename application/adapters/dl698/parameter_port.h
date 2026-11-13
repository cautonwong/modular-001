#ifndef ND_APPLICATION_ADAPTERS_DL698_PARAMETER_PORT_H
#define ND_APPLICATION_ADAPTERS_DL698_PARAMETER_PORT_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    DL698_PARAM_SECURITY_MODE_EXDATA = 0,
    DL698_PARAM_SECURITY_MODE_EN,
    DL698_PARAM_REPORT_FOLLOW_EN,
    DL698_PARAM_REPORT_CHANNEL,
    DL698_PARAM_REPORT_ACTIVE_EN,
    DL698_PARAM_REPORT_3320_LIST_IR,
    DL698_PARAM_REPORT_3320_LIST_RS485_1,
    DL698_PARAM_REPORT_3320_LIST_PLC,
    DL698_PARAM_REPORT_3320_LIST_RS485_2,
    DL698_PARAM_COUNT
} dl698_parameter_t;

typedef struct nd_dl698_parameter_port {
    void *ctx;
    int (*read)(void *ctx, dl698_parameter_t parameter, uint8_t *buffer,
                size_t length);
    int (*write)(void *ctx, dl698_parameter_t parameter,
                 const uint8_t *buffer, size_t length);
} nd_dl698_parameter_port_t;

void dl698_bind_parameters(const nd_dl698_parameter_port_t *port);

int dl698_parameter_read(dl698_parameter_t parameter, uint8_t *buffer,
                         size_t length);

int dl698_parameter_write(dl698_parameter_t parameter,
                          const uint8_t *buffer, size_t length);

#endif
