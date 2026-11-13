#ifndef __INTERNAL_API_H__
#define __INTERNAL_API_H__

#include "fr_dlms_message.h"
#include "nx_dlms.h"
extern uint8_t sourceSystemTitle[8];
extern uint8_t *expectedClientSystemTitle;
extern uint8_t expectedSecuritySuite;
extern uint8_t expectedSecurityPolicy;
extern uint32_t *expectedInvocationCounter;
extern uint8_t ciphered;
extern uint8_t qualityOfService;
extern uint8_t dlmsVersionNumber;
extern DLMS_CONFORMANCE negotiatedConformance;
extern DLMS_CONFORMANCE proposedConformance;
extern uint16_t maxPduSize;
extern uint16_t maxServerPDUSize;
extern uint8_t useLogicalNameReferencing;
extern ciphering cipher;

int _parseUserInformation(struct instance_t* instance, struct pipeline_t* pipeline, nd_dlms_msg_t* mesg);
int _getUserInformation(struct instance_t* instance, struct pipeline_t* pipeline, nd_dlms_msg_t* mesg);

#endif