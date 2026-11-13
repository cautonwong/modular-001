#include <stddef.h>
#include "domain/security/security_request.h"

#include "domain/security/security_mode.h"

#define ND_SECURITY_SA_UNICAST 0u
#define ND_SECURITY_SA_WILDCARD 1u
#define ND_SECURITY_SA_GROUP 2u
#define ND_SECURITY_SA_BROADCAST 3u
#define ND_SECURITY_OAD_BROADCAST_TIME 0x40007F00u

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

    if (request->oad == ND_SECURITY_OAD_BROADCAST_TIME) {
        int cipher_sidmac = (request->security_choice
                             == ND_SECURITY_CHOICE_CIPHER)
                            && (request->verification_choice
                                == ND_SECURITY_AUTH_SID_MAC);

        if (request->sa_type == ND_SECURITY_SA_UNICAST) {
            if (!request->connected || !cipher_sidmac) {
                return ND_SECURITY_REQUEST_REJECTED;
            }
        } else if (request->sa_type == ND_SECURITY_SA_BROADCAST) {
            if (!cipher_sidmac) {
                return ND_SECURITY_REQUEST_SILENT;
            }
        } else if (request->sa_type == ND_SECURITY_SA_WILDCARD) {
            return ND_SECURITY_REQUEST_REJECTED;
        } else {
            return ND_SECURITY_REQUEST_SILENT;
        }
    }

    return ND_SECURITY_REQUEST_ALLOW;
}
