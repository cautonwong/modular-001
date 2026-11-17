#ifndef ND_RN8209_PINS_H
#define ND_RN8209_PINS_H

void nd_rn8209_reset_line_output(void);
void nd_rn8209_reset_line_input(void);
void nd_rn8209_reset_line_set(int high);
void nd_rn8209_tx_line_output(void);
void nd_rn8209_tx_line_set(int high);

#endif
