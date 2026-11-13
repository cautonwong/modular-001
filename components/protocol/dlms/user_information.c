#include "application/ports/fifo.h"
#include "fr_dlms_pipeline.h"
#include "fr_dlms_message.h"
#include "__enums__.h"
#include "__errorcodes__.h"
#include "__internal_api__.h"
#include "nx_dlms.h"

static const uint8_t EMPTY_SYSTEM_TITLE[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

int _parseUserInformation(struct instance_t* instance, struct pipeline_t* pipeline, nd_dlms_msg_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    int ret;
    uint8_t response;
    uint16_t pduSize;
    uint8_t ch, len, tag;
    uint32_t value;
    len = fifoBuf_getByte(fifo);
    // Encoding the choice for user information
    tag = fifoBuf_getByte(fifo);
    if (tag != 0x4)
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    len = fifoBuf_getByte(fifo);
    // Tag for xDLMS-Initate.response
    tag = fifoBuf_getByte(fifo);
    DLMS_SECURITY security = DLMS_SECURITY_NONE;
    DLMS_SECURITY_SUITE suite = DLMS_SECURITY_SUITE_V0;
    uint64_t invocationCounter = 0;
#ifndef DLMS_IGNORE_HIGH_GMAC
    if (tag == DLMS_COMMAND_GLO_INITIATE_RESPONSE ||
        tag == DLMS_COMMAND_GLO_INITIATE_REQUEST ||
        tag == DLMS_COMMAND_DED_INITIATE_RESPONSE ||
        tag == DLMS_COMMAND_DED_INITIATE_REQUEST ||
        tag == DLMS_COMMAND_GENERAL_GLO_CIPHERING ||
        tag == DLMS_COMMAND_GENERAL_DED_CIPHERING)
    {
        // *command = (uint8_t)tag;
#ifndef DLMS_IGNORE_MALLOC
#if 0
        if ((ret = cip_decrypt(&settings->cipher,
            settings->sourceSystemTitle,
            &settings->cipher.blockCipherKey,
            data,
            &security,
            &suite,
            &invocationCounter)) != 0)
        {
            return ret;
        }
#endif
#else
        if ((ret = cip_decrypt(&settings->cipher,
            settings->sourceSystemTitle,
            settings->cipher.blockCipherKey,
            data,
            &security,
            &suite,
            &invocationCounter)) != 0)
        {
            return DLMS_ERROR_CODE_INVALID_DECIPHERING_ERROR;
        }
#endif //DLMS_IGNORE_MALLOC

        if (expectedSecurityPolicy != 0xFF && security != expectedSecurityPolicy << 4)
        {
            return DLMS_ERROR_CODE_INVALID_DECIPHERING_ERROR;
        }
        if (expectedSecuritySuite != 0xFF && suite != expectedSecuritySuite)
        {
            return DLMS_ERROR_CODE_INVALID_SECURITY_SUITE;
        }
        if (expectedInvocationCounter != NULL)
        {
            if (invocationCounter < 1 + *expectedInvocationCounter)
            {
                return DLMS_ERROR_CODE_INVOCATION_COUNTER_TOO_SMALL;
            }
            //#ifdef DLMS_COSEM_INVOCATION_COUNTER_SIZE64
            *expectedInvocationCounter = 1 + invocationCounter;
            //#else
                        //* settings->expectedInvocationCounter = (uint32_t)(1 + invocationCounter);
            //#endif //DLMS_COSEM_INVOCATION_COUNTER_SIZE64

        }
        //If client system title doesn't match.
        if (expectedClientSystemTitle != NULL &&
            memcmp(expectedClientSystemTitle, EMPTY_SYSTEM_TITLE, 8) != 0 &&
            memcmp(sourceSystemTitle, expectedClientSystemTitle, 8) != 0)
        {
            return DLMS_ERROR_CODE_INVALID_DECIPHERING_ERROR;
        }

        //settings->cipher.security = security;
        //settings->cipher.suite = suite;
        tag = fifoBuf_getByte(fifo);
    }
#endif //DLMS_IGNORE_HIGH_GMAC
    response = tag == DLMS_COMMAND_INITIATE_RESPONSE;
    if (response)
    {
        // Optional usage field of the negotiated quality of service
        // component
        tag = fifoBuf_getByte(fifo);
        if (tag != 0)
        {
            qualityOfService = fifoBuf_getByte(fifo);
        }
    }
    else if (tag == DLMS_COMMAND_INITIATE_REQUEST)
    {
        // Optional usage field of the negotiated quality of service
        // component
        tag = fifoBuf_getByte(fifo);
        // Dedicated key.
#ifdef DLMS_IGNORE_HIGH_GMAC

#else
        if (tag != 0)
        {
            //Return error if ciphering is not used.
            if (!ciphered || cipher.security != DLMS_SECURITY_AUTHENTICATION_ENCRYPTION)
            {
                return DLMS_ERROR_CODE_INVALID_PARAMETER;
            }
            len = fifoBuf_getByte(fifo);
            if (len != 16)
            {
                return DLMS_ERROR_CODE_INVALID_PARAMETER;
            }
            //hlp_setObjectCount(8, data);
            //bb_get(data, cipher.dedicatedKey, len);
        }
        else
        {
            memset(cipher.dedicatedKey, 0, 8);
        }
#endif //DLMS_IGNORE_HIGH_GMAC
        // Optional usage field of the negotiated quality of service
        // component
        tag = fifoBuf_getByte(fifo);
        // Skip if used.
        if (tag != 0)
        {
            tag = fifoBuf_getByte(fifo);
        }
        // Optional usage field of the proposed quality of service component
        tag = fifoBuf_getByte(fifo);
        if (tag != 0)
        {
            qualityOfService = fifoBuf_getByte(fifo);
        }
    }
    else
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    // Get DLMS version number.
    if (1)//settings->server)
    {
        ch = fifoBuf_getByte(fifo);
        dlmsVersionNumber = ch;
    }
    else
    {
        ch = fifoBuf_getByte(fifo);
        if (ch < 6)
        {
            //Invalid DLMS version number.
            return DLMS_ERROR_CODE_INVALID_VERSION_NUMBER;
        }
    }

    // Tag for conformance block
    tag = fifoBuf_getByte(fifo);
    if (tag != 0x5F)
    {
        return DLMS_ERROR_CODE_INVALID_TAG;
    }
    // Old Way...
    tag = fifoBuf_getByte(fifo);
    if (tag == 0x1F)
    {
        len = fifoBuf_getByte(fifo);
    }
    // The number of unused bits in the bit string.
    tag = fifoBuf_getByte(fifo);
    uint8_t v;
    uint32_t tmp;
    v = fifoBuf_getByte(fifo);
    value = hlp_swapBits(v);
    v = fifoBuf_getByte(fifo);
    tmp = hlp_swapBits(v);
    tmp <<= 8;
    value |= tmp;
    v = fifoBuf_getByte(fifo);
    tmp = hlp_swapBits(v);
    tmp <<= 16;
    value |= tmp;
    if (1)//settings->server)
    {
        negotiatedConformance = (DLMS_CONFORMANCE)(value & proposedConformance);
        //Remove general protection if ciphered connection is not used.
        if (!ciphered && (negotiatedConformance & DLMS_CONFORMANCE_GENERAL_PROTECTION) != 0)
        {
            negotiatedConformance &= ~DLMS_CONFORMANCE_GENERAL_PROTECTION;
        }
    }
    else
    {
        negotiatedConformance = (DLMS_CONFORMANCE)value;
    }
    if (1)//settings->server)
    {
        pduSize = (fifoBuf_getByte(fifo) << 8) | fifoBuf_getByte(fifo);
        //If client asks too high PDU.
        if (pduSize > maxServerPDUSize)
        {
            pduSize = maxServerPDUSize;
        }
        maxPduSize = pduSize;
    }
    else
    {
        pduSize = (fifoBuf_getByte(fifo) << 8) | fifoBuf_getByte(fifo);
        maxPduSize = pduSize;
    }
    if (response)
    {
        // VAA Name
        uint16_t vaa;
        vaa = (fifoBuf_getByte(fifo) << 8) | fifoBuf_getByte(fifo);
        if (vaa == 0x0007)
        {
            // If LN
            if (!useLogicalNameReferencing)
            {
                //Invalid VAA.
                return DLMS_ERROR_CODE_INVALID_PARAMETER;
            }
        }
        else if (vaa == 0xFA00)
        {
            // If SN
            if (useLogicalNameReferencing)
            {
                //Invalid VAA.
                return DLMS_ERROR_CODE_INVALID_PARAMETER;
            }
        }
        else
        {
            // Unknown VAA.
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
    }
    return 0;
}

int _getUserInformation(struct instance_t* instance, struct pipeline_t* pipeline, nd_dlms_msg_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    int ret = 0;
    // Tag for xDLMS-Initiate
    fifoBuf_putByte(cache, DLMS_COMMAND_INITIATE_RESPONSE);
    //NegotiatedQualityOfService
    if (qualityOfService == 0)
    {
        // Not used.
        fifoBuf_putByte(cache, 0x00);
    }
    else
    {
        fifoBuf_putByte(cache, 0x01);
        fifoBuf_putByte(cache, qualityOfService);
    }
    fifoBuf_putByte(cache, 0x06);
    fifoBuf_putByte(cache, 0x5f);
    fifoBuf_putByte(cache, 0x1f);
    fifoBuf_putByte(cache, 0x04);
    fifoBuf_putByte(cache, 0x00);
    // DLMS Version Number

    apdu_setConformanceToArray(negotiatedConformance, cache);
    ringbuffer_putU16(cache, maxPduSize);
    // VAA Name VAA name (0x0007 for LN referencing and 0xFA00 for SN)
    ringbuffer_putU16(cache, useLogicalNameReferencing ? 0x0007 : 0xFA00);
#if 0
    if (isCiphered(&settings->cipher))
    {
        uint8_t cmd;
        if (command == DLMS_COMMAND_GENERAL_DED_CIPHERING)
        {
            cmd = DLMS_COMMAND_GENERAL_DED_CIPHERING;
        }
        else if (command == DLMS_COMMAND_GENERAL_GLO_CIPHERING)
        {
            cmd = DLMS_COMMAND_GENERAL_GLO_CIPHERING;
        }
        else
        {
            cmd = DLMS_COMMAND_GLO_INITIATE_RESPONSE;
        }
        ret = cip_encrypt(&settings->cipher,
            settings->cipher.security,
            DLMS_COUNT_TYPE_PACKET,
            settings->cipher.invocationCounter,
            cmd,
            settings->cipher.systemTitle,
            settings->cipher.blockCipherKey,
            data);
    }
#endif //DLMS_IGNORE_HIGH_GMAC
    return ret;
}
