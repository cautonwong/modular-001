#include "components/services/maintenance/nd_maintenance.h"

#include <stddef.h>

void nd_maintenance_init(nd_maintenance_t *maintenance)
{
    if (maintenance == NULL) {
        return;
    }
    *maintenance = (nd_maintenance_t){0};
}

void nd_maintenance_abort(nd_maintenance_t *maintenance)
{
    nd_maintenance_init(maintenance);
}

static int reject(nd_maintenance_t *maintenance, uint8_t byte)
{
    maintenance->checksum = 0;
    maintenance->state = byte == ND_MAINTENANCE_START ? 1u : 0u;
    return -1;
}

int nd_maintenance_feed(nd_maintenance_t *maintenance, uint8_t byte,
                        nd_maintenance_release_t release, void *ctx)
{
    if (maintenance == NULL || release == NULL) {
        return -1;
    }
    switch (maintenance->state) {
    case 0:
        if (byte == ND_MAINTENANCE_START) {
            maintenance->state = 1;
            maintenance->checksum = 0;
        }
        return 0;
    case 1:
        maintenance->checksum ^= byte;
        if (byte != ND_MAINTENANCE_LENGTH) {
            return reject(maintenance, byte);
        }
        maintenance->state = 2;
        return 0;
    case 2:
        maintenance->checksum ^= byte;
        if (byte != ND_MAINTENANCE_RELEASE) {
            return reject(maintenance, byte);
        }
        maintenance->state = 3;
        maintenance->confirmation = 0;
        return 0;
    case 3:
    case 4:
    case 5:
    case 6:
        maintenance->checksum ^= byte;
        maintenance->confirmation = (maintenance->confirmation << 8) | byte;
        maintenance->state++;
        return 0;
    case 7:
        if (byte != maintenance->checksum) {
            return reject(maintenance, byte);
        }
        maintenance->state = 0;
        return release(ctx, maintenance->confirmation) == 0 ? 1 : -1;
    default:
        maintenance->state = 0;
        return -1;
    }
}
