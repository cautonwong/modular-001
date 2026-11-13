#ifndef ND_IC_PROFILE_GENERIC_H
#define ND_IC_PROFILE_GENERIC_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_idis_profiles.h"

int nd_ic_profile_generic_get(void *ctx, uint8_t client_sap,
                              const nd_obis_t *ln, uint8_t attribute_id,
                              const nd_axdr_reader_t *selector,
                              nd_axdr_writer_t *out);

int nd_ic_profile_generic_action(void *ctx, uint8_t client_sap,
                                 const nd_obis_t *ln, uint8_t method_id,
                                 nd_axdr_reader_t *in, nd_axdr_writer_t *out);

#endif
