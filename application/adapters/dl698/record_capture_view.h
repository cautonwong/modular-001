#ifndef ND_APPLICATION_DL698_RECORD_CAPTURE_VIEW_H
#define ND_APPLICATION_DL698_RECORD_CAPTURE_VIEW_H

#include <stdint.h>

#include "domain/events/record_capture.h"

uint32_t nd_record_capture_oad(nd_record_capture_kind_t kind, uint32_t oad,
                               uint8_t program_first_is_time_set);

uint8_t nd_record_capture_visible(uint8_t mode, uint32_t oad,
                                  nd_record_kind_t record_kind);

#endif
