#include "targets/stategrid_meter/app/dl698_parameter_adapter.h"

#include "nidavellir.h"
#include "port.h"

#include "application/adapters/dl698/parameter_port.h"

static int parameter_index(dl698_parameter_t parameter, uint16_t *id)
{
    switch (parameter) {
    case DL698_PARAM_SECURITY_MODE_EXDATA:
        *id = SecurityMode_EXData;
        break;
    case DL698_PARAM_SECURITY_MODE_EN:
        *id = SecurityMode_En;
        break;
    case DL698_PARAM_REPORT_FOLLOW_EN:
        *id = FollowReport_en;
        break;
    case DL698_PARAM_REPORT_CHANNEL:
        *id = Report_channel;
        break;
    case DL698_PARAM_REPORT_ACTIVE_EN:
        *id = ActiveReport_en;
        break;
    case DL698_PARAM_REPORT_3320_LIST_IR:
        *id = Report3320_OADList1;
        break;
    case DL698_PARAM_REPORT_3320_LIST_RS485_1:
        *id = Report3320_OADList2;
        break;
    case DL698_PARAM_REPORT_3320_LIST_PLC:
        *id = Report3320_OADList3;
        break;
    case DL698_PARAM_REPORT_3320_LIST_RS485_2:
        *id = Report3320_OADList4;
        break;
    default:
        return -1;
    }
    return 0;
}

static uint32_t parameter_address(uint16_t id)
{
    return ((uint32_t)RES_DEV_EEPROM << 24)
           + ((uint32_t)mbase_block_parameter << 16) + id;
}

static int read_parameter(void *ctx, dl698_parameter_t parameter,
                          uint8_t *buffer, size_t length)
{
    uint16_t id;

    if (parameter_index(parameter, &id) != 0) {
        return -1;
    }
    return nd_get_index(ctx, parameter_address(id), buffer, (uint16_t)length) < 0
           ? -1 : 0;
}

static int write_parameter(void *ctx, dl698_parameter_t parameter,
                           const uint8_t *buffer, size_t length)
{
    uint16_t id;

    if (parameter_index(parameter, &id) != 0) {
        return -1;
    }
    return nd_set_index(ctx, parameter_address(id), (uint8_t *)buffer,
                        (uint16_t)length) < 0 ? -1 : 0;
}

void dl698_parameter_bind(struct instance_t *instance)
{
    static nd_dl698_parameter_port_t port;

    port.ctx = instance;
    port.read = read_parameter;
    port.write = write_parameter;
    dl698_bind_parameters(&port);
}
