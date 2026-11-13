#ifndef ND_APPLICATION_ADAPTERS_DL698_AXDR_CODEC_H
#define ND_APPLICATION_ADAPTERS_DL698_AXDR_CODEC_H
#include <stdint.h>
#include "fr_axdr.h"
uint8_t axdr_decode(uint8_t type, const uint8_t *buf, uint16_t *len);
uint8_t axdr_len_encode(uint8_t* buf, uint16_t len);
uint16_t axdr_get_len(const uint8_t *data);
#endif
