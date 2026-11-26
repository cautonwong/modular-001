#ifndef ND_DOMAIN_SECURITY_SECURITY_REQUEST_H
#define ND_DOMAIN_SECURITY_SECURITY_REQUEST_H

#include <stdint.h>

enum {
    ND_SECURITY_REQUEST_DENIED = -1,
    ND_SECURITY_REQUEST_REJECTED = -2,
    ND_SECURITY_REQUEST_SILENT = 0,
    ND_SECURITY_REQUEST_ALLOW = 1
};

enum {
    ND_SECURITY_SA_UNICAST = 0,
    ND_SECURITY_SA_WILDCARD = 1,
    ND_SECURITY_SA_GROUP = 2,
    ND_SECURITY_SA_BROADCAST = 3
};

typedef struct {
    uint8_t security_choice;
    uint8_t verification_choice;
    uint8_t client_index;
    uint8_t sa_type;
    uint8_t channel_is_ir;
    uint8_t connected;
    uint8_t any_connected;
    uint8_t auth_on;
    uint8_t term_auth_timeout;
    uint8_t infrared_auth_timeout;
    uint8_t report_response;
    uint8_t get_request;
    uint8_t plain_read_allowed;
} nd_security_request_t;

int nd_security_request_check(const nd_security_request_t *request);

#endif
