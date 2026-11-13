#include "targets/stategrid_meter/app/metering_input_adapter.h"
#include "domain/metering/metering_input.h"
#include "infra/boards/stategrid_single_phase/meter_config.h"
#include "cpu_surport.h"

static int board_read(void *ctx, nd_metering_input_t input, int32_t *value)
{
    uint8_t channel;

    (void)ctx;
    switch (input)
    {
        case ND_METERING_INPUT_TEMPERATURE:
            channel = ND_ADC_CHANNEL_TEMPERATURE;
            break;
        case ND_METERING_INPUT_BATTERY:
            channel = ND_ADC_CHANNEL_BATTERY;
            break;
        default:
            return 0;
    }
    if (!mcu_adc_start_once(channel))
    {
        return 0;
    }
    return read_mcu_adc_measure_value(channel, value) != 0;
}

static const nd_metering_input_port_t metering_input_port = {
    .ctx = NULL,
    .read = board_read,
};

void stategrid_metering_input_bind(void)
{
    nd_metering_input_bind(&metering_input_port);
}
