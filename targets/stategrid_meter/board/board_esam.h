#ifndef ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_ESAM_H
#define ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_ESAM_H

#include "infra/drivers/security/esam/esam.h"

struct stategrid_board_state;

nd_esam_t *board_esam(struct stategrid_board_state *state);

#endif
