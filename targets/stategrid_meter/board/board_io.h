#ifndef ND_BOARD_IO_H
#define ND_BOARD_IO_H

/* Semantic board controls used by the application and target layers. */
void nd_board_backlight_set(int on);
void nd_board_plc_event_set(int on);
void nd_board_relay_led_set(int on);
void nd_board_relay_feedback_input_enable(void);
int nd_board_relay_is_off(void);
void nd_board_cover_input_enable(void);
int nd_board_cover_closed(void);
void nd_board_key_input_enable(void);
int nd_board_key_released(void);
int nd_board_key_pressed(void);

#endif
