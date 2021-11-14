#ifndef __U_TEST_H__
#define __U_TEST_H__

#include "mxml.h"

#define TEST_source 1
#define TEST_expectation 2

#define TEST_frame_check 1
#define TEST_frame_check_no 2

#define TEST_response 1
#define TEST_response_no 2

int get_test_suites_num(char* u_test_file);

int get_test_suite_frame(char* u_test_file, uint8_t* data, uint16_t index, uint8_t type);

int get_test_suite_cmd(char* u_test_file, uint8_t type, uint16_t index);

int get_test_suite_apdu(char* u_test_file, uint8_t* data, uint8_t type, uint16_t index);

int get_test_suite_auth(char* u_test_file, uint8_t type, uint16_t index);


#endif