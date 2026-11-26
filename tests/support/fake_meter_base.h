#ifndef ND_TESTS_FAKE_METER_BASE_H
#define ND_TESTS_FAKE_METER_BASE_H

#include <stdint.h>

void test_meter_base_reset(void);
void test_meter_base_set_valid(int value);
uint8_t *test_meter_base_tsa(void);
uint8_t *test_meter_base_commdisp_ir485(void);
uint8_t *test_meter_base_clientaddr(void);
uint8_t *test_meter_base_connect_status(void);
void test_meter_base_set_mac_suspend_state(int value);

#endif
