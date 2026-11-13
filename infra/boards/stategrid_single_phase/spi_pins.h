#ifndef ND_SPI_PINS_H
#define ND_SPI_PINS_H

void nd_spi_flash_power_set(int on);
void nd_spi_cs_attention(void);
void nd_spi_clk_attention(void);
void nd_spi_mosi_attention(void);
void nd_spi_miso_input(void);
void nd_spi_mosi_output(void);
void nd_spi_sck_output(void);
void nd_spi_cs_set(int high);
void nd_spi_sck_set(int high);
void nd_spi_mosi_set(int high);
int nd_spi_miso_read(void);

#endif
