#include "nidavellir.h"
#include "__enums__.h"
#include "__errorcodes__.h"
#include "__internal_api__.h"
#include "dlms\nx_dlms.h"

uint8_t hlp_swapBits(uint8_t value)
{
    uint8_t ret = 0, pos;
    for (pos = 0; pos != 8; ++pos)
    {
        ret = (uint8_t)((ret << 1) | (value & 0x01));
        value = (uint8_t)(value >> 1);
    }
    return ret;
}

// Server sender frame sequence starting number.
static const uint8_t SERVER_START_SENDER_FRAME_SEQUENCE = 0x1E;
// Server receiver frame sequence starting number.
static const uint8_t SERVER_START_RECEIVER_FRAME_SEQUENCE = 0xFE;
// Client sender frame sequence starting number.
static const uint8_t CLIENT_START_SENDER_FRAME_SEQUENCE = 0xFE;
// Client receiver frame sequence starting number.
static const uint8_t CLIENT_START_RCEIVER_FRAME_SEQUENCE = 0xE;
uint8_t senderFrame;
uint8_t receiverFrame;
DLMS_PRIORITY priority;
DLMS_SERVICE_CLASS serviceClass;
uint8_t invokeID;

uint8_t increaseReceiverSequence(uint8_t value)
{
    return ((value + 0x20) | 0x10 | (value & 0xE));
}

uint8_t increaseSendSequence(uint8_t value)
{
    return (uint8_t)((value & 0xF0) | ((value + 0x2) & 0xE));
}

void resetFrameSequence()
{
    senderFrame = SERVER_START_SENDER_FRAME_SEQUENCE;
    receiverFrame = SERVER_START_RECEIVER_FRAME_SEQUENCE;
}

DLMS_AUTHENTICATION authentication;
uint8_t ctoSChallenge[65];

DLMS_CONNECTION_STATE connected;
//Used max info TX.
unsigned short maxInfoTX;
//Used max info RX.
unsigned short maxInfoRX;
//Used max window size in TX.
uint8_t windowSizeTX;
//Used max window size in RX.
uint8_t windowSizeRX;
uint8_t ciphered;
int16_t userId;
uint8_t protocolVersion;
DLMS_ASSOCIATION_RESULT result;
uint8_t diagnostic;
uint8_t stoCChallenge[65];
uint8_t sourceSystemTitle[8];
uint8_t* expectedClientSystemTitle = NULL;
uint8_t qualityOfService;
uint8_t dlmsVersionNumber;
DLMS_CONFORMANCE negotiatedConformance;
DLMS_CONFORMANCE proposedConformance;
// Expected security suite.
// If Expected security suite is set client can't connect with other security suite.
uint8_t expectedSecuritySuite;
/////////////////////////////////////////////////////////////////////////
// Expected security policy.
// If Expected security policy is set client can't connect with other security policies.
uint8_t expectedSecurityPolicy;
uint32_t* expectedInvocationCounter = NULL;
/**
* Max PDU size used in communicating.
*/
uint16_t maxPduSize;

/**
* Max PDU size that server uses. Client can ask anything, but server will decide.
*/
uint16_t maxServerPDUSize;
uint8_t useLogicalNameReferencing = 1;
ciphering cipher;

static const uint8_t EMPTY_SYSTEM_TITLE[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const uint8_t EMPTY_LN[6] = { 0, 0, 0, 0, 0, 0 };
static const uint8_t DEFAULT_ASSOCIATION[6] = { 0, 0, 40, 0, 0, 255 };
static const uint8_t EMPTY_KEY[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#ifdef WIN32
char* error_strings[] =
{
    "ok",
    "��ʼ��־����",
    "���ȱ�ʶ����",
    "ͷУ�����",
    "βУ�����",
    "������־����",
    "�����ִ���",
};
char* p_string[] = { "������Ӧ","����Ӧ" };
char* f_string[] = { "���δ���δ���","���δ��������" };

#endif

#define is_packet_len(l)  (((l)>>12) == 0x0a)
#define packet_len(l)  ((l)&0x3ff)
#define is_s_set(l)  ((((l)>>11)&0x01) == 0x01)
#define ca(l)  ((l)>>1)

#define is_packet_I(p)  (((p)&0x01) == 0x00)
#define I_RRR(p)  ((((p)>>5)&0x07))
#define I_P(p)  ((((p)>>4)&0x01))
#define I_F(p)  ((((p)>>4)&0x01))
#define I_SSS(p)  ((((p)>>1)&0x07))

#define is_packet_RR(p)  (((p)&0x0f) == 0x01)
#define RR_RRR(p)  ((((p)>>5)&0x07))
#define RR_P(p)  ((((p)>>4)&0x01))
#define RR_F(p)  ((((p)>>4)&0x01))

#define is_packet_RNR(p)  (((p)&0x0f) == 0x05)
#define RNR_RRR(p)  ((((p)>>5)&0x07))
#define RNR_P(p)  ((((p)>>4)&0x01))
#define RNR_F(p)  ((((p)>>4)&0x01))

#define is_packet_SNRM(p)  ( (((p)&0x0f) == 0x03) && ((((p)>>5)&0x07) == 0x04))
#define SNRM_P(p)  ((((p)>>4)&0x01))
#define is_packet_DISC(p)  ( (((p)&0x0f) == 0x03) && ((((p)>>5)&0x07) == 0x02))
#define DISC_P(p)  ((((p)>>4)&0x01))
#define is_packet_UA(p)  ( (((p)&0x0f) == 0x03) && ((((p)>>5)&0x07) == 0x03))
#define UA_F(p)  ((((p)>>4)&0x01))
#define is_packet_DM(p)  ( (((p)&0x0f) == 0x0f) && ((((p)>>5)&0x07) == 0x00))
#define DM_F(p)  ((((p)>>4)&0x01))

#define is_packet_FRMR(p)  ( (((p)&0x0f) == 0x07) && ((((p)>>5)&0x07) == 0x04))
#define FRMR_F(p)  ((((p)>>4)&0x01))
#define is_packet_UI(p)  ( (((p)&0x0f) == 0x03) && ((((p)>>5)&0x07) == 0x00))
#define UI_F(p)  ((((p)>>4)&0x01))
#define UI_P(p)  ((((p)>>4)&0x01))

extern uint16_t update_crc16(uint16_t init, uint8_t byte);
int __update_hcs_fcs(struct hdlc_control_block_t* ctx, uint8_t* byte)
{
    ctx->tx_len++;
    ctx->hcs = update_crc16(ctx->hcs, *byte);
    ctx->fcs = update_crc16(ctx->fcs, *byte);
    return 0;
}

int __update_fcs(struct hdlc_control_block_t* ctx, uint8_t* byte)
{
    ctx->tx_len++;
    ctx->fcs = update_crc16(ctx->fcs, *byte);
    return 0;
}

int __pack_pdu_byte(struct pipeline_t* pipeline, uint8_t* byte)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;

    append_byte(fifo, *byte, __update_fcs, ctx);
    return 0;
}

int __filter_21E(struct hdlc_control_block_t* ctx, uint8_t* b)
{
    return 0;
}

int __server_hdlc_ctrl(uint8_t c)
{
#ifdef WIN32
    printf("                    ֡������:0x%02x(", c);
#endif
    if (is_packet_I(c))
    {
#ifdef WIN32
        printf("I֡,%s,%d,%d)\n",p_string[I_P(c)],I_RRR(c),I_SSS(c));
#endif
        return 0;
    }
    if (is_packet_RR(c))
    {
#ifdef WIN32
        printf("RR֡,%s,%d)\n", p_string[RR_P(c)], RR_RRR(c));
#endif
        return 0;
    }

    if (is_packet_RNR(c))
    {
#ifdef WIN32
        printf("RNR֡,%s,%d)\n", p_string[RNR_P(c)], RNR_RRR(c));
#endif
        return 0;
    }
    if (is_packet_SNRM(c))
    {
#ifdef WIN32
        printf("SNRM֡,%s)\n", p_string[SNRM_P(c)]);
#endif
        return 0;
    }
    if (is_packet_DISC(c))
    {
#ifdef WIN32
        printf("DISC֡,%s)\n", p_string[DISC_P(c)]);
#endif
        return 0;
    }
    if (is_packet_UI(c))
    {
#ifdef WIN32
        printf("UI֡,%s)\n", p_string[UI_P(c)]);
#endif
        return 0;
    }

    return -6;
}

//��������
int __hdlc_filter(struct hdlc_control_block_t* ctx, uint8_t* b)
{
    uint8_t byte = *b;
    int r = 0;

    switch (ctx->step)
    {
    case 0:
        if (byte != 0x7e)
        {
            return -1;
        }
        ctx->step = 1;
        return 0;
    case 1:
        ctx->hcs = update_crc16(0xffff, byte);
        ctx->fcs = update_crc16(0xffff, byte);
        ctx->rx_len = byte;
        ctx->len_rxed = 1;
        ctx->step = 2;
        return 0;
    case 2:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        ctx->rx_len = (ctx->rx_len << 8) | byte;//���滹�ж����ֽ�
        if (!is_packet_len(ctx->rx_len))
        {
            ctx->step = 0;
            return -2;
        }
#ifdef WIN32
        printf("                    ֡��:0x%02x\n", packet_len(ctx->rx_len));
        printf("                    ��֡:0x%02x(0x01��ʾ��֡)\n", is_s_set(ctx->rx_len));
#endif
        ctx->len_rxed += 1;
        ctx->s_addr_n = 0;
        ctx->c_addr_n = 0;
        ctx->step = 3;
        return 0;
    case 3://Ŀ���ַ ����������ַ
        ctx->len_rxed += 1;
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        ctx->addr[ctx->s_addr_n++] = byte;
        if( (byte&0x01) == 1)
        {
            ctx->step = 4;
        }
        return 0;
    case 4:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        ctx->ca = byte;//�ͻ���
#ifdef WIN32
        printf("                    �ͻ���:0x%02x\n", ca(ctx->ca));
#endif
        ctx->len_rxed += 1;
        ctx->step = 5;
        return 0;
    case 5://������
        ctx->len_rxed += 1;
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        ctx->ctrl = byte;//������
        r = __server_hdlc_ctrl(ctx->ctrl);
        if (r < 0)
        {
            ctx->step = 0;
            return r;
        }
        ctx->step = 6;
        return 0;
    case 6:
        ctx->len_rxed += 1;
        ctx->hcs = get_crc16(ctx->hcs);
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (byte != (ctx->hcs & 0xff))
        {
            ctx->step = 0;
            return -3;
        }
        ctx->step = 7;
        return 0;
    case 7:
        ctx->len_rxed += 1;
        ctx->fcs = update_crc16(ctx->fcs, byte);
        if (byte != ((ctx->hcs >> 8) & 0xff))
        {
            ctx->step = 0;
            return -3;
        }
        
        if (ctx->len_rxed  == packet_len(ctx->rx_len))
        {
#ifdef WIN32
            printf("                    ��Ϣ��[0]:��\n");
#endif
            ctx->step = 11;//����Ϣ��
            return 0;
        }

        ctx->step = 8;
        ctx->info_len = ctx->len_rxed + 1;//��Ϣ�����ʼ
#ifdef WIN32
        printf("                    ��Ϣ��[%d]:", packet_len(ctx->rx_len) - ctx->info_len - 1);
#endif
        return 0;
    case 8://������Ϣ��
        ctx->len_rxed += 1;
        ctx->fcs = update_crc16(ctx->fcs, byte);
#ifdef WIN32
        printf("%02x", byte);
#endif
        if (ctx->len_rxed + 2 == packet_len(ctx->rx_len))
        {
#ifdef WIN32
            printf("\n");
#endif
            ctx->step = 9;//��Ϣ��������
        }
        return 0;
    case 9:
        ctx->fcs = get_crc16(ctx->fcs);
        if (byte != (ctx->fcs & 0xff))
        {
            ctx->step = 0;
            return -4;
        }
        ctx->step = 10;
        return 0;
    case 10:
        if (byte != ((ctx->fcs >> 8) & 0xff))
        {
            ctx->step = 0;
            return -4;
        }
        ctx->step = 11;
        return 0;
    case 11:
        if (byte != 0x7e)
        {
            ctx->step = 0;
            return -5;
        }
        return 1;//���յ�������һ֡
    }

    assert(0);
    return -1;
}


int fr_pack_hdlc_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    uint16_t len = mesg->len + ctx->s_addr_n + 2 + 2 + 1 +3;//mesg->len �Ǳ�����Ϣ��ĳ���
    len = (0x0a << 12) + (0 << 11) + len;//�Ƿ��֡ todo
    ctx->hcs = 0xffff;
    ctx->fcs = 0xffff;
    ctx->tx_len = 0;
    fifoBuf_putByte(fifo, 0x7e);
    ctx->tx_len++;
    append_byte(fifo, ((len >> 8) & 0xff), __update_hcs_fcs, ctx);
    append_byte(fifo, (len & 0xff), __update_hcs_fcs, ctx);
    append_byte(fifo, ctx->ca, __update_hcs_fcs, ctx);//��Ҫ�ͻ�����Ϣ todo
    append_data(fifo, ctx->addr, ctx->s_addr_n, __update_hcs_fcs, ctx);

    append_byte(fifo, mesg->ctrl, __update_hcs_fcs, ctx);

    ctx->hcs = get_crc16(ctx->hcs);
    append_byte(fifo, (ctx->hcs & 0xff), __update_fcs, ctx);
    append_byte(fifo, ((ctx->hcs >> 8) & 0xff), __update_fcs, ctx);
    return 1;
}

int fr_pack_hdlc_apdu(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;

    fifo_popData(ctx->apdu, mesg->len, __pack_pdu_byte, pipeline);
    return 1;
}

int fr_pack_hdlc_tail(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    ctx->fcs = get_crc16(ctx->fcs);
    fifoBuf_putByte(fifo, ctx->fcs & 0xff);
    ctx->tx_len++;
    fifoBuf_putByte(fifo, ((ctx->fcs >> 8) & 0xff));
    ctx->tx_len++;
    fifoBuf_putByte(fifo, 0x7e);
    ctx->tx_len++;
    return 1;
}

int apdu_parseUserInformation(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
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

int apdu_parseApplicationContextName(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    int ret;
    uint8_t len, ch;
    // Get length.
    len = fifoBuf_getByte(fifo);
    ch = fifoBuf_getByte(fifo);
    if (ch != 0x6)
    {
        //Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
#ifndef DLMS_IGNORE_HIGH_GMAC
    //if (settings->server)
    {
      //  settings->cipher.security = DLMS_SECURITY_NONE;
    }
#endif //DLMS_IGNORE_HIGH_GMAC
    // Object ID length.
    len = fifoBuf_getByte(fifo);
    ch = fifoBuf_getByte(fifo);
    if (ch != 0x60)
    {
        //Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    ch = fifoBuf_getByte(fifo);
    if (ch != 0x85)
    {
        //Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    ch = fifoBuf_getByte(fifo);
    if (ch != 0x74)
    {
        //Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }

    ch = fifoBuf_getByte(fifo);
    if (ch != 0x05)
    {
        //Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    ch = fifoBuf_getByte(fifo);
    if (ch != 0x08)
    {
        //Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    ch = fifoBuf_getByte(fifo);
    if (ch != 0x01)
    {
        //Encoding failed. Not an Object ID.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    ch = fifoBuf_getByte(fifo);
    if (useLogicalNameReferencing)
    {
        ciphered = ch == 3;
        if (ch == 1 || ciphered)
        {
            return 0;
        }
        return DLMS_ERROR_CODE_FALSE;
    }
    ciphered = ch == 4;
    if (ch == 2 || ciphered)
    {
        return 0;
    }
    return DLMS_ERROR_CODE_FALSE;
}

uint8_t isCiphered(ciphering* cipher)
{
    return cipher->security != DLMS_SECURITY_NONE;
}

int apdu_generateApplicationContextName(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    int ret;
    uint8_t application_context_name_tag[] = {
    (BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_APPLICATION_CONTEXT_NAME),
    0x09,
     BER_TYPE_OBJECT_IDENTIFIER,
    0x07,0x60,0x85,0x74,0x05,0x08,0x01
    };
    //ProtocolVersion
    if (protocolVersion != 0)
    {
        fifoBuf_putByte(cache, BER_TYPE_CONTEXT | PDU_TYPE_PROTOCOL_VERSION);
        fifoBuf_putByte(cache, 2);
            //Un-used bits.
        fifoBuf_putByte(cache, 2);
        fifoBuf_putByte(cache, protocolVersion);
    }
    uint8_t is_ciphered;
    is_ciphered = ciphered != DLMS_SECURITY_NONE;

    // Application context name tag
    fifoBuf_putData(cache, application_context_name_tag, sizeof(application_context_name_tag));

    if (useLogicalNameReferencing)
    {
        fifoBuf_putByte(cache, is_ciphered ? 0x03 : 0x01);
    }
    else
    {
        fifoBuf_putByte(cache, is_ciphered ? 0x04 : 0x02);
    }
    // Add system title.
    if ((ciphered || authentication == DLMS_AUTHENTICATION_HIGH_GMAC))
    {
        // Add calling-AP-title
        fifoBuf_putByte(cache, (BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | 6));
        // LEN
        fifoBuf_putByte(cache, (2 + 8));
        fifoBuf_putByte(cache, BER_TYPE_OCTET_STRING);
        // LEN
        fifoBuf_putByte(cache, 8);
        fifoBuf_putData(cache, cipher.systemTitle, 8);
    }
    //Add CallingAEInvocationId.
    if (userId != -1 &&cipher.security != DLMS_SECURITY_NONE)
    {
        fifoBuf_putByte(cache, (BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLING_AE_INVOCATION_ID));
            //LEN
        fifoBuf_putByte(cache, 3);
        fifoBuf_putByte(cache, BER_TYPE_INTEGER);
            //LEN
        fifoBuf_putByte(cache, 1);
        fifoBuf_putByte(cache, userId);
    }
    return 0;
}

int apdu_setConformanceToArray(uint32_t value, struct fifo_t* data)
{
    fifoBuf_putByte(data, hlp_swapBits((uint8_t)(value & 0xFF)));
    value >>= 8;
    fifoBuf_putByte(data, hlp_swapBits((uint8_t)(value & 0xFF)));
    value >>= 8;
    fifoBuf_putByte(data, hlp_swapBits((uint8_t)(value & 0xFF)));
    return 0;
}

int apdu_getUserInformation(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
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

int apdu_generateAARE(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    int ret;
    // Set AARE tag and length 0x61
    fifoBuf_putByte(cache, BER_TYPE_APPLICATION | BER_TYPE_CONSTRUCTED | PDU_TYPE_APPLICATION_CONTEXT_NAME);
    // Length is updated later.
    uint16_t len_ptr = cache->wr;
    fifoBuf_putByte(cache, 0);
    apdu_generateApplicationContextName(instance, pipeline, mesg);
    // Result 0xA2
    fifoBuf_putByte(cache, BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | BER_TYPE_INTEGER);
    fifoBuf_putByte(cache, 3);
    // Tag
    fifoBuf_putByte(cache, BER_TYPE_INTEGER);
    // Choice for result (INTEGER, universal)
    fifoBuf_putByte(cache, 1);
    // ResultValue
    fifoBuf_putByte(cache, result);
    // SourceDiagnostic
    fifoBuf_putByte(cache, 0xA3);
    // len
    fifoBuf_putByte(cache, 5);
    // Tag
    if ((diagnostic & 0x80) == 0)
    {
        fifoBuf_putByte(cache, 0xA1);
    }
    else
    {
        fifoBuf_putByte(cache, 0xA2);
        diagnostic &= ~0x80;
    }
    fifoBuf_putByte(cache, 3);
    fifoBuf_putByte(cache, 2);
    // Choice for result (INTEGER, universal)
    fifoBuf_putByte(cache, 1);
    // diagnostic
    fifoBuf_putByte(cache, diagnostic);
    // SystemTitle
#ifndef DLMS_IGNORE_HIGH_GMAC
    if (authentication == DLMS_AUTHENTICATION_HIGH_GMAC
        || isCiphered(&cipher))
    {
        fifoBuf_putByte(cache, BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLED_AP_INVOCATION_ID);
        fifoBuf_putByte(cache, 2 + 8);
        fifoBuf_putByte(cache, BER_TYPE_OCTET_STRING);
        fifoBuf_putByte(cache, 8);
        fifoBuf_putByte(cache, cipher.systemTitle, 8);
    }
#endif //LMS_IGNORE_HIGH_GMAC

    //Add CalledAEInvocationId.
    if (userId != -1 && cipher.security != DLMS_SECURITY_NONE)
    {
        fifoBuf_putByte(cache, BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLING_AE_QUALIFIER);
        //LEN
        fifoBuf_putByte(cache, 3);
        fifoBuf_putByte(cache, BER_TYPE_INTEGER);
        //LEN
        fifoBuf_putByte(cache, 1);
        fifoBuf_putByte(cache,userId);
    }
    if (authentication > DLMS_AUTHENTICATION_LOW)
    {
        //if (stoCChallenge.size == 0)
        {
            //return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
        // Add server ACSE-requirenents field component.
        fifoBuf_putByte(cache, 0x88);
        fifoBuf_putByte(cache, 0x02);
        fifoBuf_putByte(cache, 0x07);
        fifoBuf_putByte(cache, 0x80);
        fifoBuf_putByte(cache, 0x89);
        fifoBuf_putByte(cache, 0x07);
        fifoBuf_putByte(cache, 0x60);
        fifoBuf_putByte(cache, 0x85);
        fifoBuf_putByte(cache, 0x74);
        fifoBuf_putByte(cache, 0x05);
        fifoBuf_putByte(cache, 0x08);
        fifoBuf_putByte(cache, 0x02);
        fifoBuf_putByte(cache, authentication);
        fifoBuf_putByte(cache, 0xaa);
        fifoBuf_putByte(cache, 0x02 + stoCChallenge[0]);//
        fifoBuf_putByte(cache, BER_TYPE_CONTEXT);
        fifoBuf_putByte(cache, stoCChallenge[0]);
        fifoBuf_putByte(cache, 0xaa);
        fifoBuf_putData(cache, stoCChallenge + 1, stoCChallenge[0]);
    }
    if (result == DLMS_ASSOCIATION_RESULT_ACCEPTED || !isCiphered(&cipher))
    {
        // Add User Information. Tag 0xBE
        fifoBuf_putByte(cache, BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_USER_INFORMATION);
#if 0
        if (encryptedData != NULL && encryptedData->size != 0)
        {
            bb_capacity(&tmp, 2 + encryptedData->size);
            bb_setUInt8(&tmp, DLMS_COMMAND_GLO_INITIATE_RESPONSE);
            hlp_setObjectCount(encryptedData->size, &tmp);
            bb_set2(&tmp, encryptedData, 0, encryptedData->size);
        }
        else
#endif
        {
            //if (errorData != NULL && errorData->size != 0)
            {
               // bb_set2(&tmp, errorData, 0, errorData->size);
            }
            //else
            {
                uint8_t ptr = cache->wr;
                fifoBuf_putByte(cache, 0x00);//len
                fifoBuf_putByte(cache, BER_TYPE_OCTET_STRING);
                fifoBuf_putByte(cache, 0x00);
                apdu_getUserInformation(instance, pipeline, mesg);
                cache->buf_ptr[ptr] = cache->wr - ptr + 2;//�����ֽڳ���
                cache->buf_ptr[ptr+2] = cache->wr - ptr;
            }
        }
    }
    cache->buf_ptr[len_ptr] = (cache->wr - len_ptr);
    return 0;
}

int serve_aarq(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint8_t len;
    uint16_t data_len = 0;
    uint16_t i = 0;
    uint8_t tag;
    int ret;

    if ((connected & DLMS_CONNECTION_STATE_HDLC) == 0)
    {
        return DLMS_ERROR_CODE_REJECTED;
    }

    len = fifoBuf_getByte(fifo);

    if (len > 0x80)
    {
        if (len == 0x81)
        {
            data_len = fifoBuf_getByte(fifo);
        }
        else if (len == 0x82)
        {
            data_len = (fifoBuf_getByte(fifo) << 8) | fifoBuf_getByte(fifo);
        }
        else if (len == 0x84)
        {
            data_len = (fifoBuf_getByte(fifo) << 24) | (fifoBuf_getByte(fifo) << 16) |
                (fifoBuf_getByte(fifo) << 8) | fifoBuf_getByte(fifo);
        }
        else
        {
            //ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
    }
    else
    {
        data_len = len;
    }

    //while (i < data_len)
    data_len = fifoBuf_end(fifo, data_len);
    while (fifo->rd != data_len)
    {
        tag = fifoBuf_getByte(fifo);
        switch (tag)
        {
        case BER_TYPE_CONTEXT: //0x80
            tag = fifoBuf_getByte(fifo);
            tag = fifoBuf_getByte(fifo);
            tag = fifoBuf_getByte(fifo);
            if(tag != 0x80)//0x84
            {
                diagnostic = 0x80 | DLMS_ACSE_SERVICE_PROVIDER_NO_COMMON_ACSE_VERSION;
                result = DLMS_ASSOCIATION_RESULT_PERMANENT_REJECTED;
                //return 0;
            }
            protocolVersion = tag;
            break;
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_APPLICATION_CONTEXT_NAME://0xA1
        {
            if ((ret = apdu_parseApplicationContextName(instance, pipeline, mesg)) != 0)
            {
                diagnostic = DLMS_SOURCE_DIAGNOSTIC_APPLICATION_CONTEXT_NAME_NOT_SUPPORTED;
                result = DLMS_ASSOCIATION_RESULT_PERMANENT_REJECTED;
                return 0;
            }
            if (ciphered)
            {
                //afu = (DLMS_AFU_MISSING)(DLMS_AFU_MISSING_SENDER_ACSE_REQUIREMENTS | DLMS_AFU_MISSING_MECHANISM_NAME | DLMS_AFU_MISSING_CALLING_AUTHENTICATION_VALUE);
            }
        }
        break;
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLED_AP_TITLE://0xa2
            // Get len.
            len = fifoBuf_getByte(fifo);
            if (len != 3)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            // Choice for result.
            tag = fifoBuf_getByte(fifo);
            if (1)//settings->server)
            {
                if (tag != BER_TYPE_OCTET_STRING)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                // Get len.
                len = fifoBuf_getByte(fifo);
                //Ignore if client sends CalledAPTitle.
                fifoBuf_removeData(fifo, len);
            }
            else
            {
                if (tag != BER_TYPE_INTEGER)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                // Get len.
                len = fifoBuf_getByte(fifo);
                if (len != 1)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                tag = fifoBuf_getByte(fifo);
                result = (DLMS_ASSOCIATION_RESULT)tag;
            }
            break;
            // 0xA3 SourceDiagnostic
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLED_AE_QUALIFIER:
            len = fifoBuf_getByte(fifo);
            tag = fifoBuf_getByte(fifo);
            len = fifoBuf_getByte(fifo);

            if (1)//settings->server)
            {
                if (tag != BER_TYPE_OCTET_STRING)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                //Ignore if client sends CalledAEQualifier.
                fifoBuf_removeData(fifo, len);
            }
            else
            {
                tag = fifoBuf_getByte(fifo);// Result source diagnostic component.
                
                if (tag != BER_TYPE_INTEGER)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                len = fifoBuf_getByte(fifo);
                if (len != 1)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                tag = fifoBuf_getByte(fifo);
                diagnostic = (DLMS_SOURCE_DIAGNOSTIC)tag;
            }
            break;
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLED_AP_INVOCATION_ID:// 0xA4 Result
            // Get len.
            len = fifoBuf_getByte(fifo);
            if (1)//settings->server)
            {
                if (len != 3)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                // ACSE service user tag.
                tag = fifoBuf_getByte(fifo);
                if (tag != BER_TYPE_INTEGER)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                len = fifoBuf_getByte(fifo);
                //Ignore if client sends CalledAEQualifier.
                fifoBuf_removeData(fifo, len);
            }
            else
            {
                if (len != 0xA)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                // Choice for result (Universal, Octet string type)
                tag = fifoBuf_getByte(fifo);
                if (tag != BER_TYPE_OCTET_STRING)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                // responding-AP-title-field
                // Get len.
                len = fifoBuf_getByte(fifo);
                //If system title is invalid.
                if (len != 8)
                {
                    memset(sourceSystemTitle, 0, 8);
                }
                else
                {
                    fifoBuf_getData(fifo, sourceSystemTitle, len);
                }
            }
            break;
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLING_AP_TITLE:// 0xA6 Client system title.
            len = fifoBuf_getByte(fifo);
            tag = fifoBuf_getByte(fifo);
            len = fifoBuf_getByte(fifo);

            if (ciphered && len != 8)
            {
                diagnostic = DLMS_SOURCE_DIAGNOSTIC_CALLING_AP_TITLE_NOT_RECOGNIZED;
                result = DLMS_ASSOCIATION_RESULT_PERMANENT_REJECTED;
                return 0;
            }
            fifoBuf_getData(fifo, sourceSystemTitle, len);
            break;
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_SENDER_ACSE_REQUIREMENTS:// 0xAA Server system title.
            len = fifoBuf_getByte(fifo);
            tag = fifoBuf_getByte(fifo);
            len = fifoBuf_getByte(fifo);
            stoCChallenge[0] = len;
            fifoBuf_getData(fifo, stoCChallenge+1, len);
            break;
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLING_AE_INVOCATION_ID://Client AE Invocation id.
            len = fifoBuf_getByte(fifo);
            tag = fifoBuf_getByte(fifo);
            len = fifoBuf_getByte(fifo);
            len = fifoBuf_getByte(fifo);
            break;
            
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLED_AE_INVOCATION_ID://0xA5 //Client CalledAeInvocationId.
            if (1)//settings->server)
            {
                len = fifoBuf_getByte(fifo);
                //Invalid length.
                if (len != 3)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                len = fifoBuf_getByte(fifo);
                //Invalid length.
                if (len != 2)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                len = fifoBuf_getByte(fifo);
                //Invalid tag length.
                if (len != 1)
                {
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                    break;
                }
                //Get value.
                len = fifoBuf_getByte(fifo);
            }
            else
            {
                len = fifoBuf_getByte(fifo);
                tag = fifoBuf_getByte(fifo);
                len = fifoBuf_getByte(fifo);
                len = fifoBuf_getByte(fifo);
                if (ciphered)
                {
                    userId = len;
                }
            }
            break;
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLING_AE_QUALIFIER://0xA7 //Server RespondingAEInvocationId.
            len = fifoBuf_getByte(fifo);
            tag = fifoBuf_getByte(fifo);
            len = fifoBuf_getByte(fifo);
            len = fifoBuf_getByte(fifo);
            if (ciphered && len == 0)
            {
                userId = len;
            }
            break;
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLING_AP_INVOCATION_ID://0xA8
            tag = fifoBuf_getByte(fifo);
            if (tag != 3)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            len = fifoBuf_getByte(fifo);
            if (len != 2)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            //Invalid tag length.
            len = fifoBuf_getByte(fifo);
            if (len != 1)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            //Get value.
            len = fifoBuf_getByte(fifo);
            break;
            
        case (uint16_t)BER_TYPE_CONTEXT | (uint8_t)PDU_TYPE_SENDER_ACSE_REQUIREMENTS://  0x8A or 0x88
        case (uint16_t)BER_TYPE_CONTEXT | (uint8_t)PDU_TYPE_CALLING_AP_INVOCATION_ID:
            // Get sender ACSE-requirements field component.
            len = fifoBuf_getByte(fifo);
            if (len != 2)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            tag = fifoBuf_getByte(fifo);
            if (tag != BER_TYPE_OBJECT_DESCRIPTOR)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
                break;
            }
            //Get only value because client app is sending system title with LOW authentication.
            tag = fifoBuf_getByte(fifo);
            if (ciphered && tag == 0x80)
            {
                //afu &= ~DLMS_AFU_MISSING_SENDER_ACSE_REQUIREMENTS;
            }
            break;
            //  0x8B or 0x89
        case (uint16_t)BER_TYPE_CONTEXT | (uint8_t)PDU_TYPE_MECHANISM_NAME:
        case (uint16_t)BER_TYPE_CONTEXT | (uint8_t)PDU_TYPE_CALLING_AE_INVOCATION_ID:
            tag = fifoBuf_getByte(fifo);
            tag = fifoBuf_getByte(fifo);
            if (tag != 0x60)
            {
                return DLMS_ERROR_CODE_INVALID_TAG;
            }
            tag = fifoBuf_getByte(fifo);
            if (tag != 0x85)
            {
                return DLMS_ERROR_CODE_INVALID_TAG;
            }
            tag = fifoBuf_getByte(fifo);
            if (tag != 0x74)
            {
                return DLMS_ERROR_CODE_INVALID_TAG;
            }
            tag = fifoBuf_getByte(fifo);
            if (tag != 0x05)
            {
                return DLMS_ERROR_CODE_INVALID_TAG;
            }
            tag = fifoBuf_getByte(fifo);
            if (tag != 0x08)
            {
                return DLMS_ERROR_CODE_INVALID_TAG;
            }
            tag = fifoBuf_getByte(fifo);
            if (tag != 0x02)
            {
                return DLMS_ERROR_CODE_INVALID_TAG;
            }
            tag = fifoBuf_getByte(fifo);
            switch (tag)
            {
            case DLMS_AUTHENTICATION_NONE:
            case DLMS_AUTHENTICATION_LOW:
            case DLMS_AUTHENTICATION_HIGH:
            case DLMS_AUTHENTICATION_HIGH_MD5:
            case DLMS_AUTHENTICATION_HIGH_SHA1:
            case DLMS_AUTHENTICATION_HIGH_SHA256:
            case DLMS_AUTHENTICATION_HIGH_GMAC:
                break;
            default:
                return DLMS_ERROR_CODE_INVALID_TAG;
            }
            authentication = (DLMS_AUTHENTICATION)tag;
#ifndef DLMS_IGNORE_HIGH_GMAC
            uint8_t invalidSystemTitle;
            invalidSystemTitle = memcmp(sourceSystemTitle, EMPTY_SYSTEM_TITLE, 8) == 0;
            if (1 && authentication == DLMS_AUTHENTICATION_HIGH_GMAC && invalidSystemTitle)
            {
                diagnostic = DLMS_SOURCE_DIAGNOSTIC_CALLING_AP_TITLE_NOT_RECOGNIZED;
                result = DLMS_ASSOCIATION_RESULT_PERMANENT_REJECTED;
                return 0;
            }
#endif //DLMS_IGNORE_HIGH_GMAC
            if (ciphered)
            {
                //afu &= ~DLMS_AFU_MISSING_MECHANISM_NAME;
            }
            break;
            // 0xAC
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_CALLING_AUTHENTICATION_VALUE:
            len = fifoBuf_getByte(fifo);
            tag = fifoBuf_getByte(fifo);
            if (tag != 0x80)
            {
                ret = DLMS_ERROR_CODE_INVALID_TAG;
            }
            else// if ((ret = bb_getUInt8(buff, &len)) == 0)
            {
                len = fifoBuf_getByte(fifo);

                if (authentication == DLMS_AUTHENTICATION_LOW)
                {
                }
                else
                {
                    if (len < 8 || len > 64)
                    {
                        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
                    }
                    else
                    {
                        ctoSChallenge[0] = len;
                        fifoBuf_getData(fifo, ctoSChallenge+1, len);
                    }
                }
            }

            if (ciphered)
            {
                //afu &= ~DLMS_AFU_MISSING_CALLING_AUTHENTICATION_VALUE;
            }
            break;
            // 0xBE
        case BER_TYPE_CONTEXT | BER_TYPE_CONSTRUCTED | PDU_TYPE_USER_INFORMATION:
            if (result != DLMS_ASSOCIATION_RESULT_ACCEPTED
                && diagnostic != DLMS_SOURCE_DIAGNOSTIC_NONE)
            {
            }
            if ((ret = apdu_parseUserInformation(instance, pipeline, mesg)) != 0)
            {
                if (ret == DLMS_ERROR_CODE_INVOCATION_COUNTER_TOO_SMALL ||
                    ret == DLMS_ERROR_CODE_INVALID_DECIPHERING_ERROR ||
                    ret == DLMS_ERROR_CODE_INVALID_SECURITY_SUITE)
                {
                    return ret;
                }
                if (ciphered)
                {
                    ret = DLMS_ERROR_CODE_INVALID_DECIPHERING_ERROR;
                }
                else
                {
                    //Return confirmed service error.
                    ret = DLMS_ERROR_CODE_INVALID_TAG;
                }
                break;
            }
            break;
        default:
            break;
        }
    }
    
    connected |= DLMS_CONNECTION_STATE_DLMS;
    fifoBuf_putByte(cache, 0xe6);
    fifoBuf_putByte(cache, 0xe7);
    fifoBuf_putByte(cache, 0x00);
    apdu_generateAARE(instance, pipeline, mesg);
    mesg->len = cache->wr;
    return 1;
}

int serve_GetRequest(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu; 
    int ret;
    DLMS_GET_COMMAND_TYPE type;
    uint8_t invokeId, ch;
    // Get type.
    type = (DLMS_GET_COMMAND_TYPE)fifoBuf_getByte(fifo);
    // Get invoke ID and priority.
    invokeId = fifoBuf_getByte(fifo);
    //updateInvokeId(settings, invokeId);
    if ((invokeId & 0x80) != 0) {
        priority = DLMS_PRIORITY_HIGH;
    }
    else {
        priority = DLMS_PRIORITY_NORMAL;
    }
    if ((invokeId & 0x40) != 0) {
        serviceClass = DLMS_SERVICE_CLASS_CONFIRMED;
    }
    else {
        serviceClass = DLMS_SERVICE_CLASS_UN_CONFIRMED;
    }
    invokeID = (uint8_t)(invokeId & 0xF);
    // GetRequest normal
    /*
    if (type == DLMS_GET_COMMAND_TYPE_NORMAL)
    {
        ret = svr_getRequestNormal(settings, data, invokeId);
        if (ret == DLMS_ERROR_CODE_INVALID_COMMAND)
        {
            return ret;
        }
    }
    else if (type == DLMS_GET_COMMAND_TYPE_NEXT_DATA_BLOCK)
    {
        // Get request for next data block
        ret = svr_getRequestNextDataBlock(settings, data, invokeId);
    }
    else if (type == DLMS_GET_COMMAND_TYPE_WITH_LIST)
    {
        if ((settings->base.negotiatedConformance & DLMS_CONFORMANCE_MULTIPLE_REFERENCES) == 0)
        {
            return DLMS_ERROR_CODE_INVALID_COMMAND;
        }
        // Get request with a list.
        ret = svr_getRequestWithList(settings, data, invokeId);
    }
    else
    {
        ret = DLMS_ERROR_CODE_READ_WRITE_DENIED;
        type = DLMS_GET_COMMAND_TYPE_NORMAL;
    }
    if (ret != 0)
    {
        //If error is not DLMS error code.
        if (!(ret > 0 && ret <= DLMS_ERROR_CODE_OTHER_REASON))
        {
            // Access Error : Device reports Read-Write denied.
            ret = DLMS_ERROR_CODE_READ_WRITE_DENIED;
        }
        gxLNParameters p;
        params_initLN(&p, &settings->base, invokeId, DLMS_COMMAND_GET_RESPONSE, type, NULL, data, ret, settings->info.encryptedCommand, 0, 0);
        bb_clear(data);
        ret = dlms_getLNPdu(&p, data);
    }*/
    return 1;
}

int serve_iframe(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    fifoBuf_removeData(fifo, ctx->info_len);
    uint8_t cmd = 0;
    int r = 0;

    //�Ϸ����ж� todo
    uint8_t expected;
    if ((senderFrame & 0x1) == 0)
    {
        expected = increaseReceiverSequence(increaseSendSequence(receiverFrame));
        if (ctx->ctrl == expected)
        {
            receiverFrame = ctx->ctrl;
        }
        if(receiverFrame == CLIENT_START_RCEIVER_FRAME_SEQUENCE)
        {
            receiverFrame = ctx->ctrl;
        }
    }

    cmd = fifoBuf_getByte(fifo);//0xe6
    cmd = fifoBuf_getByte(fifo);//0xe6
    cmd = fifoBuf_getByte(fifo);//0x00
    cmd = fifoBuf_getByte(fifo);//

    switch (cmd)
    {
    case DLMS_COMMAND_AARQ:
        r = serve_aarq(instance, pipeline, mesg);
        break;
    case DLMS_COMMAND_GET_REQUEST:
        r = serve_GetRequest(instance, pipeline, mesg);
        break;
        
        
    default:
        break;
    }
    
    senderFrame = increaseReceiverSequence(increaseSendSequence(senderFrame));
    //senderFrame = increaseSendSequence((uint8_t)settings->senderFrame);
    mesg->ctrl = senderFrame;
    return 1;
}

int serve_snrm(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    fifoBuf_removeData(fifo, ctx->info_len);
    struct fifo_t* cache = ctx->apdu;

    //����Ϣ��  ����Ĭ��ֵ todo
        
    uint8_t id,len,id_len;
    uint32_t data;
    int r = 0;
    fifoBuf_getByte(fifo);//FromatID 0x81
    fifoBuf_getByte(fifo);//Group ID 0x80
    len = fifoBuf_getByte(fifo);//Group len
    len = fifoBuf_end(fifo, len);
    while ((r == 0 ) && (fifo->rd != len))
    {
        id = fifoBuf_getByte(fifo);
        id_len = fifoBuf_getByte(fifo);

        if (id_len == 1)
            ringbuffer_popU8(fifo, &data);  
        if (id_len == 2)
            ringbuffer_getU16(fifo, &data);
        if (id_len == 4)
            ringbuffer_getU32(fifo, &data);

        switch (id)
        {
        case HDLC_INFO_MAX_INFO_TX:
            maxInfoRX = (uint16_t)data;
            break;
        case HDLC_INFO_MAX_INFO_RX:
            maxInfoTX = (uint16_t)data;
            break;
        case HDLC_INFO_WINDOW_SIZE_TX:
            windowSizeRX = (uint8_t)data;
            break;
        case HDLC_INFO_WINDOW_SIZE_RX:
            windowSizeTX = (uint8_t)data;
            break;
        default:
            r = DLMS_ERROR_CODE_INVALID_PARAMETER;//�쳣����  todo
            break;
        }
    }
    
    //Э�� ȡСֵ todo
    fifoBuf_putByte(cache, 0x81);//FromatID
    fifoBuf_putByte(cache, 0x80);//GroupID
    fifoBuf_putByte(cache, 0);////GroupLen  
    fifoBuf_putByte(cache, HDLC_INFO_MAX_INFO_TX);
    if (maxInfoTX < 0x100)
    {
        fifoBuf_putByte(cache, 1);
        fifoBuf_putByte(cache, maxInfoTX);
    }
    else
    {
        fifoBuf_putByte(cache, 2);
        ringbuffer_putU16(cache, maxInfoTX);
    }

    fifoBuf_putByte(cache, HDLC_INFO_MAX_INFO_RX);
    if (maxInfoTX < 0x100)
    {
        fifoBuf_putByte(cache, 1);
        fifoBuf_putByte(cache, maxInfoRX);
    }
    else
    {
        fifoBuf_putByte(cache, 2);
        ringbuffer_putU16(cache, maxInfoRX);
    }

    fifoBuf_putByte(cache, HDLC_INFO_WINDOW_SIZE_TX);
    fifoBuf_putByte(cache, 4);
    ringbuffer_putU32(cache, windowSizeTX);

    fifoBuf_putByte(cache, HDLC_INFO_WINDOW_SIZE_RX);
    fifoBuf_putByte(cache, 4);
    ringbuffer_putU32(cache, windowSizeRX);

    len = (cache->wr - 3);
    cache->buf_ptr[2] = len;

    connected = DLMS_CONNECTION_STATE_HDLC;
    resetFrameSequence();
    mesg->ctrl = DLMS_COMMAND_UA;
    mesg->len = cache->wr;
    return 1;
}

int serve_disc(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    //fifoBuf_removeData(fifo, ctx->info_len);
    struct fifo_t* cache = ctx->apdu;

    //����Ϣ��  ����Ĭ��ֵ todo

    uint8_t len;
    int r = 0;
    fifoBuf_getByte(fifo);//FromatID 0x81
    fifoBuf_getByte(fifo);//Group ID 0x80
    len = fifoBuf_getByte(fifo);//Group len
    fifoBuf_putByte(cache, HDLC_INFO_WINDOW_SIZE_RX);
    fifoBuf_putByte(cache, 4);
    ringbuffer_putU32(cache, windowSizeRX);
    fifoBuf_putByte(cache, 0x81); // FromatID
    fifoBuf_putByte(cache, 0x80); // GroupID
    fifoBuf_putByte(cache, 0); // Length

    fifoBuf_putByte(cache, HDLC_INFO_MAX_INFO_TX);
    if (maxInfoTX < 0x100)
    {
        fifoBuf_putByte(cache, 1);
        fifoBuf_putByte(cache, maxInfoTX);
    }
    else
    {
        fifoBuf_putByte(cache, 2);
        ringbuffer_putU16(cache, maxInfoTX);
    }

    fifoBuf_putByte(cache, HDLC_INFO_MAX_INFO_RX);
    if (maxInfoTX < 0x100)
    {
        fifoBuf_putByte(cache, 1);
        fifoBuf_putByte(cache, maxInfoRX);
    }
    else
    {
        fifoBuf_putByte(cache, 2);
        ringbuffer_putU16(cache, maxInfoRX);
    }

    fifoBuf_putByte(cache, HDLC_INFO_WINDOW_SIZE_TX);
    fifoBuf_putByte(cache, 4);
    ringbuffer_putU32(cache, windowSizeTX);

    fifoBuf_putByte(cache, HDLC_INFO_WINDOW_SIZE_RX);
    fifoBuf_putByte(cache, 4);
    ringbuffer_putU32(cache, windowSizeRX);

    len = (cache->wr - 3);
    cache->buf_ptr[2] = len;
    connected = DLMS_CONNECTION_STATE_NONE;
    mesg->ctrl = DLMS_COMMAND_UA;
    mesg->len = cache->wr;
    return 1;
}

int serve_release_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct hdlc_control_block_t* ctx = (struct hdlc_control_block_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    //fifoBuf_removeData(fifo, ctx->info_len);
    struct fifo_t* cache = ctx->apdu;

    //����Ϣ��  ����Ĭ��ֵ todo

    uint8_t len;
    int r = 0;
    len = fifoBuf_getByte(fifo);
    fifoBuf_getByte(fifo);
    fifoBuf_getByte(fifo);
    fifoBuf_getByte(fifo);

    if (len != 3)
    {
        //user information
    }

    len = (cache->wr - 3);
    cache->buf_ptr[2] = len;
    connected &= ~DLMS_CONNECTION_STATE_DLMS;
    mesg->len = cache->wr;
    mesg->ctrl = DLMS_COMMAND_RELEASE_RESPONSE;
    return 1;
}

