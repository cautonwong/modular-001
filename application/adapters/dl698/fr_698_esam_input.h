#ifndef ND_APPLICATION_DL698_ESAM_INPUT_H
#define ND_APPLICATION_DL698_ESAM_INPUT_H

#include <stdint.h>

struct context_698_t;
struct instance_t;
struct esam_698_input_t;

void fr_698_esam_input_init(struct context_698_t* ctx,
                            struct esam_698_input_t* input, uint8_t cmd_type);
void fr_698_esam_input_buffers(struct instance_t* instance,
                               struct esam_698_input_t* input);
int fr_698_esam_input_sid(struct context_698_t* ctx,
                          struct esam_698_input_t* input);
int fr_698_esam_input_octet(struct context_698_t* ctx, uint16_t* offset,
                            uint8_t** data, uint16_t* payload_len);
void fr_698_esam_input_store_rn(struct context_698_t* ctx, const uint8_t* data,
                                uint16_t length);

#endif
