#ifndef ND_IC_DISCONNECT_H
#define ND_IC_DISCONNECT_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/store.h"
#include "domain/load_control/disconnect.h"

int nd_ic_disconnect_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                         uint8_t attribute_id,
                         const nd_axdr_reader_t *selector,
                         nd_axdr_writer_t *out);
int nd_ic_disconnect_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                         uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_ic_disconnect_action(void *ctx, uint8_t client_sap,
                            const nd_obis_t *ln, uint8_t method_id,
                            nd_axdr_reader_t *in, nd_axdr_writer_t *out);
int nd_ic_disconnect_save(const nd_disconnect_snapshot_t *snapshot,
                          nd_store_t *store, uint32_t offset);
int nd_ic_disconnect_load(nd_disconnect_snapshot_t *snapshot,
                          nd_store_t *store, uint32_t offset);
/* Load control 1/2 plus the four M-Bus channels, in object order. */
int nd_ic_relays_save(const nd_disconnect_snapshot_t snapshots[6],
                      nd_store_t *store, uint32_t offset);
int nd_ic_relays_load(nd_disconnect_snapshot_t snapshots[6], nd_store_t *store,
                      uint32_t offset);

#endif
