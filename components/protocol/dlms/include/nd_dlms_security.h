#ifndef ND_DLMS_SECURITY_H
#define ND_DLMS_SECURITY_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *ctx;
    int (*aes_gcm_encrypt)(void *ctx, const uint8_t key[16],
                           const uint8_t *iv, size_t iv_len,
                           const uint8_t *aad, size_t aad_len,
                           const uint8_t *in, size_t len,
                           uint8_t *out, uint8_t tag[16]);
    int (*aes_gcm_decrypt)(void *ctx, const uint8_t key[16],
                           const uint8_t *iv, size_t iv_len,
                           const uint8_t *aad, size_t aad_len,
                           const uint8_t *in, size_t len,
                           uint8_t *out, const uint8_t tag[16]);
    int (*sha256)(void *ctx, const uint8_t *in, size_t len, uint8_t out[32]);
    int (*ecdsa_verify)(void *ctx, const uint8_t *pub, size_t pub_len,
                        const uint8_t *hash, size_t hash_len,
                        const uint8_t *sig, size_t sig_len);
    int (*random)(void *ctx, uint8_t *out, size_t len);
    /* Optional: generate an ECDSA key pair and a PKCS#10 request. */
    int (*key_pair_generate)(void *ctx, uint8_t *public_key,
                             size_t *public_len);
    int (*certificate_request)(void *ctx, const uint8_t *public_key,
                               size_t public_len, uint8_t *request,
                               size_t *request_len);
} nd_crypto_port_t;

typedef struct {
    uint8_t policy;
    uint8_t suite;
    uint8_t guek[16];
    uint8_t gubk[16];
    uint8_t gak[16];
    uint8_t master[16];
    uint8_t client_system_title[8];
    uint8_t server_system_title[8];
    uint32_t fc_tx;
    uint32_t fc_rx_unicast;
    uint32_t fc_rx_broadcast;
    uint8_t fc_rx_unicast_valid;
    uint8_t fc_rx_broadcast_valid;
    uint8_t hls_secret[16];
} nd_dlms_security_ctx_t;

/* Prototype replay window. Returns 0 on accept, -1 on replay. */
int nd_dlms_security_accept_fc(nd_dlms_security_ctx_t *ctx, uint32_t frame_counter,
                               int broadcast);

/* Returns the current TX counter, then increments it. */
uint32_t nd_dlms_security_next_tx(nd_dlms_security_ctx_t *ctx);

#endif
