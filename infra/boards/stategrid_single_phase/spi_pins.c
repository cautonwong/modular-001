#include "infra/boards/stategrid_single_phase/spi_pins.h"
#include "infra/boards/stategrid_single_phase/pin_ops.h"
#include "target.h"
#include "type_def.h"

void nd_spi_flash_power_set(int on)
{
    nd_pin_write(GPIOB_DAT, 8, on ? 0 : 1);
    nd_pin_reg_clear(GPIOB_OEN, BIT(8));
}

void nd_spi_cs_attention(void)
{
    nd_pin_reg_set(GPIOB_ATT, BIT(9));
}

void nd_spi_clk_attention(void)
{
    nd_pin_reg_set(GPIOB_ATT, BIT(10));
}

void nd_spi_mosi_attention(void)
{
    nd_pin_reg_set(GPIOB_ATT, BIT(12));
}

void nd_spi_miso_input(void)
{
    nd_pin_reg_set(GPIOB_IE, BIT(11));
}

void nd_spi_mosi_output(void)
{
    nd_pin_reg_clear(GPIOB_OEN, BIT(12));
}

void nd_spi_sck_output(void)
{
    nd_pin_reg_clear(GPIOB_OEN, BIT(10));
}

void nd_spi_cs_set(int high)
{
    nd_pin_reg_clear(GPIOB_OEN, BIT(9));
    nd_pin_write(GPIOB_DAT, 9, high);
}

void nd_spi_sck_set(int high)
{
    nd_pin_reg_clear(GPIOB_OEN, BIT(10));
    nd_pin_write(GPIOB_DAT, 10, high);
}

void nd_spi_mosi_set(int high)
{
    nd_pin_reg_clear(GPIOB_OEN, BIT(12));
    nd_pin_write(GPIOB_DAT, 12, high);
}

int nd_spi_miso_read(void)
{
    return nd_pin_read(GPIOB_STS, 11);
}
