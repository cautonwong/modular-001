#ifndef ND_IDIS_CAPTURE_H
#define ND_IDIS_CAPTURE_H

#include "components/protocol/dlms/include/nd_axdr.h"
#include "domain/freeze/capture.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_idis_profile.h"

int nd_idis_capture_read(const nd_capture_object_t *capture,
                         const nd_axdr_reader_t *in,
                         nd_capture_value_t *value);
int nd_idis_capture_write(const nd_capture_object_t *capture,
                          const nd_capture_value_t *value,
                          nd_axdr_writer_t *out);

#endif
