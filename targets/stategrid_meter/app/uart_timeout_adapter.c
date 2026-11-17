#include "application/ports/uart_timeout.h"
#include "cpu_surport.h"
#include "targets/stategrid_meter/app/pids.h"
#include "targets/stategrid_meter/app/stategrid_context.h"

int fr_uart_timeout(void *ctx, uint8_t pid)
{
    stategrid_context_t *context = ctx;
    uint8_t index;

    if (pid == STG_PID_RX_698) {
        index = 0;
    } else if (pid == STG_PID_PLC_RX_698) {
        index = 1;
    } else if (pid == STG_PID_IR_RX_698) {
        index = 2;
    } else {
        return 1;
    }

    if (fifoBuf_getUsed(&context->pipe_uart[index]) > 0) {
        uint8_t byte = fifoBuf_getByte(&context->pipe_uart[index]);

        if (IRQ_UART_RX_TM_OUT == byte) {
            return 1;
        }
    }
    return 0;
}
