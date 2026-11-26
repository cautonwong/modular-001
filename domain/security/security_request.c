#include <stddef.h>
#include "domain/security/security_request.h"

#include "domain/security/security_mode.h"

static int request_needs_application_link(const nd_security_request_t *request)
{
    return (request->security_choice == ND_SECURITY_CHOICE_CIPHER)
           || (request->verification_choice == ND_SECURITY_AUTH_SID_MAC)
           || (request->verification_choice == ND_SECURITY_AUTH_RN_MAC)
           || (request->verification_choice == ND_SECURITY_AUTH_SID);
}

int nd_security_request_check(const nd_security_request_t *request)
{
    int judge;

    if (request == NULL) {
        return ND_SECURITY_REQUEST_DENIED;
    }

    judge = !(request->report_response
              || (request->sa_type == ND_SECURITY_SA_BROADCAST)
              || (request->sa_type == ND_SECURITY_SA_GROUP));

    if ((request->security_choice == ND_SECURITY_CHOICE_PLAIN)
        && (request->verification_choice == ND_SECURITY_AUTH_RN)
        && request->get_request && request->plain_read_allowed
        && (request->any_connected || request->term_auth_timeout)) {
        judge = 0;
    }

    if (judge) {
        if (request_needs_application_link(request)
            && !request->connected) {
            return ND_SECURITY_REQUEST_DENIED;
        }

        if (request->security_choice == ND_SECURITY_CHOICE_PLAIN) {
            if (request->client_index == 0u) {
                if (request->auth_on && !request->connected) {
                    if (request->channel_is_ir) {
                        if (!request->infrared_auth_timeout) {
                            return ND_SECURITY_REQUEST_DENIED;
                        }
                    } else {
                        return ND_SECURITY_REQUEST_DENIED;
                    }
                }
            } else if (request->auth_on && !request->connected
                       && !request->term_auth_timeout) {
                return ND_SECURITY_REQUEST_DENIED;
            }
        }
    }

    return ND_SECURITY_REQUEST_ALLOW;
}
