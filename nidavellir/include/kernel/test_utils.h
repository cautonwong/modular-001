#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "testutil/cmocka.h"
extern int __read(uint32_t id, uint8_t* buf, int len);
extern int __write(uint32_t id, uint8_t *buf, int len);
extern int mock_id_data_ok(uint32_t id,size_t len);
extern int mock_id_data_error(uint32_t id);
extern int generate_data(uint8_t *data,size_t n,size_t len,size_t type);
extern int mock_time(struct tm* time, uint32_t id);
extern int init_test();
int mock_data_for_id_always(uint32_t id, size_t len);

#endif