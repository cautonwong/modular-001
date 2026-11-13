#ifndef ND_APPLICATION_DL698_REPORT_ENCODER_H
#define ND_APPLICATION_DL698_REPORT_ENCODER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
    size_t apdu_start;
    size_t length_offset;
} nd_report_encoder_t;

int nd_report_encoder_init(nd_report_encoder_t *encoder, uint8_t *buffer,
                           size_t capacity);
int nd_report_encoder_append(nd_report_encoder_t *encoder, const void *data,
                             size_t length);
int nd_report_encoder_begin_3011(nd_report_encoder_t *encoder, uint8_t piid,
                                 int with_length_prefix);
int nd_report_encoder_3011_rcsd(nd_report_encoder_t *encoder,
                                const uint32_t *oads, uint8_t count);
int nd_report_encoder_finish_3011(nd_report_encoder_t *encoder);
int nd_report_encoder_begin_normal(nd_report_encoder_t *encoder, uint8_t piid,
                                   uint8_t count);
int nd_report_encoder_3320(nd_report_encoder_t *encoder, const uint8_t *items,
                           uint8_t count);
int nd_report_encoder_2015(nd_report_encoder_t *encoder, uint32_t value);
int nd_report_encoder_end_normal(nd_report_encoder_t *encoder);
int nd_report_encoder_rn_mac_tail(nd_report_encoder_t *encoder,
                                  const uint8_t *random12,
                                  const uint8_t *esam_tail,
                                  int esam_abnormal);

#endif
