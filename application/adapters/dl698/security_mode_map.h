#ifndef ND_APPLICATION_DL698_SECURITY_MODE_MAP_H
#define ND_APPLICATION_DL698_SECURITY_MODE_MAP_H

#include <stdint.h>

#include "domain/security/security_mode.h"
#include "domain/security/security_request.h"

uint16_t nd_security_mode_lookup(uint16_t oi);

int nd_security_plain_read_allowed(uint32_t oad);

int nd_security_infrared_oad_allowed(uint32_t oad);

uint8_t nd_security_choice_from_wire(uint8_t wire_choice);

uint8_t nd_security_verification_from_wire(uint8_t wire_choice);

uint8_t nd_security_sa_from_wire(uint8_t wire_sa);

int nd_security_broadcast_time_rule(uint32_t oad,
                                    const nd_security_request_t *request);

#endif
