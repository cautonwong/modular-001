#ifndef ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_RN8209_H
#define ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_RN8209_H

#include "infra/drivers/metering/rn8209/rn8209.h"
#include "board_state.h"

void board_rn8209_init(nd_idis_board_state_t *state);
nd_rn8209_t *board_rn8209(nd_idis_board_state_t *state);

#endif
