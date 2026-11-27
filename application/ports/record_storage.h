#ifndef ND_APPLICATION_PORTS_RECORD_STORAGE_H
#define ND_APPLICATION_PORTS_RECORD_STORAGE_H

#include <stdint.h>

#ifndef ND_MUST_USE
#if defined(__GNUC__)
#define ND_MUST_USE __attribute__((warn_unused_result))
#else
#define ND_MUST_USE
#endif
#endif

/* GCC does not accept a (void) cast for warn_unused_result; legacy call
 * sites assign and discard so check:unused-result can freeze them. */
#define ND_RECORD_STORAGE_DISCARD(expr) \
    do { int nd_discarded_result = (expr); (void)nd_discarded_result; } while (0)

enum nd_record_format_mode {
    ND_RECORD_WRITE = 0,
    ND_RECORD_READ,
    ND_RECORD_GET_ORIGINAL_LEN,
    ND_RECORD_GET_STORE_LEN,
};

typedef struct nd_record_storage_port {
    void *ctx;
    int (*save_index)(void *ctx, uint32_t oad, uint32_t *index);
    int (*change_format)(void *ctx, uint8_t *des, const uint8_t *src,
                         uint32_t oad_req, uint32_t oad_src, int mode);
    int (*clear_program_list)(void *ctx, void *pipeline, void *mesg);
    int (*trigger_645_record)(void *ctx, uint32_t di, uint8_t tag);
} nd_record_storage_port_t;

ND_MUST_USE int nd_record_storage_save_index(const nd_record_storage_port_t *port,
                                 uint32_t oad, uint32_t *index);
ND_MUST_USE int nd_record_storage_change_format(const nd_record_storage_port_t *port,
                                    uint8_t *des, const uint8_t *src,
                                    uint32_t oad_req, uint32_t oad_src,
                                    int mode);
ND_MUST_USE int nd_record_storage_clear_program_list(const nd_record_storage_port_t *port,
                                         void *pipeline, void *mesg);
ND_MUST_USE int nd_record_storage_trigger_645_record(
    const nd_record_storage_port_t *port, uint32_t di, uint8_t tag);

#endif
