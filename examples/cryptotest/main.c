/*
 * Copyright (C) 2018 FU berlin
 * Copyright (C) 2018 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     unittests
 * @{
 *
 * @file
 * @brief       crypto speed test
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 * @}
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "xtimer.h"
#include "thread.h"
#include "ps.h"

#include "random.h"

#ifdef MODULE_TWEETNACL
#include <tweetnacl.h>
#endif
#ifdef MODULE_HACL
#include <haclnacl.h>
#endif
#ifdef MODULE_MBEDTLS
#include <mbedtls/ecdsa.h>
#include <mbedtls/sha256.h>
#elif MODULE_TINYCRYPT
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dsa.h>
#include <tinycrypt/ecc_dh.h>
#include <tinycrypt/sha256.h>
#elif MODULE_C25519
#include "edsign.h"
#include "ed25519.h"
#elif MODULE_MONOCYPHER
#include "monocypher.h"
#elif MODULE_QDSA
#include "sign.h"
#elif MODULE_LIBHYDROGEN
#include "hydrogen.h"
#endif


#if defined(MODULE_TWEETNACL) || defined(MODULE_HACL)
#define SECRETKEYBYTES crypto_sign_SECRETKEYBYTES
#define PUBLICKEYBYTES crypto_sign_PUBLICKEYBYTES
#define SMLEN (sizeof(message) + crypto_sign_BYTES)
#elif defined(MODULE_TINYCRYPT)
#define SECRETKEYBYTES  32
#define PUBLICKEYBYTES  64
#elif defined(MODULE_C25519)
#define SECRETKEYBYTES  EDSIGN_SECRET_KEY_SIZE
#define PUBLICKEYBYTES  EDSIGN_PUBLIC_KEY_SIZE
#elif defined(MODULE_MONOCYPHER)
#define SECRETKEYBYTES  32
#define PUBLICKEYBYTES  32
#elif defined(MODULE_QDSA)
#define  SECRETKEYBYTES 64
#define  PUBLICKEYBYTES 32
#define SMLEN (sizeof(message) + 64)
#endif

static uint8_t message[200];
static volatile size_t mlen = sizeof(message);

#ifndef MODULE_MBEDTLS
#ifndef MODULE_LIBHYDROGEN
#ifdef DO_SIGN
static unsigned char sign_sk[SECRETKEYBYTES];
static unsigned char sign_pk[PUBLICKEYBYTES];
#else
static unsigned char sign_pk[] = {
    0x37, 0xcc, 0x72, 0x62, 0x84, 0xb2, 0x68, 0xce, 0x7e, 0x3d, 0x14, 0xaf,
    0x82, 0xc9, 0x31, 0x5c, 0x59, 0xb0, 0x3f, 0x92, 0xb9, 0xf1, 0xbb, 0xd4,
    0x01, 0x8f, 0x6d, 0x25, 0xfa, 0x6f, 0xfd, 0xf6
};
#endif
#endif
#endif

#if  defined(MODULE_HACL) || defined(MODULE_TWEETNACL)
static unsigned char verify_result[SMLEN];
static unsigned long long int smlen = SMLEN;
static unsigned long long int verify_result_len;
#ifdef DO_SIGN
static unsigned char sm[SMLEN];
#else
static unsigned char sm[SMLEN] = {
    0x48, 0x55, 0xcc, 0xca, 0x51, 0xcc, 0x2e, 0x29, 0x0e, 0x6e, 0x1e, 0x34,
    0x72, 0xc6, 0xb0, 0x29, 0xb7, 0x42, 0xe7, 0x76, 0x93, 0x50, 0x1c, 0x46,
    0xa2, 0x86, 0xd5, 0x5f, 0x3d, 0x0a, 0x1b, 0x05, 0xae, 0xa9, 0x06, 0x6b,
    0x86, 0xb0, 0x2c, 0x31, 0x25, 0x19, 0xc6, 0x42, 0xf0, 0xf7, 0xb7, 0xe5,
    0xc7, 0x06, 0x48, 0x79, 0x85, 0x67, 0x43, 0xab, 0x0e, 0x56, 0x7b, 0x25,
    0xf2, 0x28, 0x65, 0x0c, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa
};
#endif /* DO_SIGN */
#endif

static uint32_t middle, after = 0;
#ifdef DEVELHELP
static int tstart, tmiddle, tafter = 0;
static int stacksz;
#endif
#ifdef DO_SIGN
static uint32_t before = 0;
#ifdef DEVELHELP
static int tbefore = 0;
#endif
#endif

#ifdef MODULE_TINYCRYPT
static uint8_t signature[128];
static uint8_t digest[32];
static struct tc_sha256_state_struct sha;
#endif

#ifdef MODULE_LIBHYDROGEN
static hydro_sign_keypair keypair;
static uint8_t signature[hydro_sign_BYTES];
#define CONTEXT "RIOT"
#endif

#ifdef MODULE_MBEDTLS
static uint8_t signature[MBEDTLS_ECDSA_MAX_LEN];
mbedtls_ecdsa_context ctx_sign, ctx_verify;
mbedtls_sha256_context sha256_ctx;
static uint8_t digest[32];
static size_t sig_len;

static int getrandom(void *ctx, unsigned char* buf, size_t len)
{
    (void)ctx;
    random_bytes((uint8_t*)buf, len);
    return len;
}
void mbedtls_platform_zeroize(void *buf, size_t len)
{
        volatile unsigned char *p = (unsigned char*)buf; while( len-- ) *p++ = 0;
}


#endif

#ifdef MODULE_RELIC
static bn_t sign_sk;
static ec_t sign_pk;

static bn_t signature1;
static bn_t signature2;
#endif

#if defined(MODULE_C25519) || defined(MODULE_MONOCYPHER)
#ifdef DO_SIGN
static uint8_t signature[64];
#else
static uint8_t signature[64] = {
    0x48, 0x55, 0xcc, 0xca, 0x51, 0xcc, 0x2e, 0x29, 0x0e, 0x6e, 0x1e, 0x34,
    0x72, 0xc6, 0xb0, 0x29, 0xb7, 0x42, 0xe7, 0x76, 0x93, 0x50, 0x1c, 0x46,
    0xa2, 0x86, 0xd5, 0x5f, 0x3d, 0x0a, 0x1b, 0x05, 0xae, 0xa9, 0x06, 0x6b,
    0x86, 0xb0, 0x2c, 0x31, 0x25, 0x19, 0xc6, 0x42, 0xf0, 0xf7, 0xb7, 0xe5,
    0xc7, 0x06, 0x48, 0x79, 0x85, 0x67, 0x43, 0xab, 0x0e, 0x56, 0x7b, 0x25,
    0xf2, 0x28, 0x65, 0x0c
};
#endif
#endif

#ifdef MODULE_QDSA
static unsigned char verify_result[SMLEN];
static unsigned char sm[SMLEN];
long long unsigned int smlen = 0;
#endif

#ifndef MODULE_MBEDTLS
#ifndef MODULE_LIBHYDROGEN
#ifdef DO_SIGN
static void gen_keypair(uint8_t *pk, uint8_t *sk)
{
#if  defined(MODULE_HACL) || defined(MODULE_TWEETNACL)
    crypto_sign_keypair(pk, sk);
#elif defined(MODULE_TINYCRYPT)
    uECC_make_key(pk, sk, &curve_secp256r1);
#elif defined(MODULE_C25519)
    random_bytes(sk, sizeof(sk));
    ed25519_prepare(sk);
    edsign_sec_to_pub(pk, sk);
#elif defined(MODULE_MONOCYPHER)
    random_bytes(sk, sizeof(sk));
    crypto_sign_public_key(pk, sk);
#elif defined(MODULE_QDSA)
    random_bytes(sk, 32);
    keypair(pk, sk);
#endif
}
#endif
#endif
#endif

#ifdef MODULE_TINYCRYPT
int default_CSPRNG(uint8_t *buf, size_t len)
{
    random_bytes(buf, len);
    return 1;
}
#endif

void print_bstr(uint8_t *d, size_t l)
{
    for (size_t i = 0; i<l; i++) {
        printf("0x%.2x, ", d[i]);
    }
}

int main(void)
{
#ifdef DEVELHELP
    thread_t *p = (thread_t *)sched_threads[sched_active_pid];
    stacksz = p->stack_size;
    tstart = stacksz - thread_measure_stack_free(p->stack_start);
#endif

#ifdef MODULE_MBEDTLS
    mbedtls_ecdsa_init(&ctx_sign);
    mbedtls_ecdsa_init(&ctx_verify);
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, &ctx_sign, getrandom, NULL );
#endif

#ifdef MODULE_LIBHYDROGEN
    hydro_sign_keygen(&keypair);
#endif

    memset(message, 0xaa, mlen);
#ifdef DO_SIGN
#if  defined(MODULE_HACL) || defined(MODULE_TWEETNACL)
    memset(sm, '\0', SMLEN);
#endif
    /* Creating keypair ... */
#ifndef MODULE_MBEDTLS
#ifndef MODULE_LIBHYDROGEN
    gen_keypair(sign_pk, sign_sk);
#endif
#endif
#ifdef DEVELHELP
    tbefore = stacksz - thread_measure_stack_free(p->stack_start);
#endif
    before = xtimer_now_usec();

    /* Sign */
#if  defined(MODULE_HACL) || defined(MODULE_TWEETNACL)
    crypto_sign(sm, &smlen, (const uint8_t *)message, sizeof(message), sign_sk);
#elif defined(MODULE_TINYCRYPT)
    tc_sha256_init(&sha);
    tc_sha256_update(&sha, message, sizeof(message));
    tc_sha256_final(digest, &sha);
    uECC_sign(sign_sk, digest, 32, signature, &curve_secp256r1);
#elif defined(MODULE_MBEDTLS)
    mbedtls_sha256_starts( &sha256_ctx, 0 );
    mbedtls_sha256_update( &sha256_ctx, message, sizeof( message ) );
    mbedtls_sha256_finish( &sha256_ctx, digest);
    mbedtls_ecdsa_write_signature(&ctx_sign, MBEDTLS_MD_SHA256, digest, sizeof(digest), signature, &sig_len, NULL, NULL);
#elif defined(MODULE_C25519)
	edsign_sign(signature, sign_pk, sign_sk, message, sizeof(message));
#elif defined(MODULE_MONOCYPHER)
    crypto_sign(signature, sign_sk, sign_pk, message, sizeof(message));
#elif defined(MODULE_QDSA)
    sign(sm, &smlen, message, sizeof(message), sign_pk, sign_sk);
#elif defined(MODULE_LIBHYDROGEN)
    hydro_sign_create(signature, message, mlen, CONTEXT, keypair.sk);
#endif
#endif /* DO_SIGN */
    middle = xtimer_now_usec();
#ifdef DEVELHELP
    tmiddle = stacksz - thread_measure_stack_free(p->stack_start);
#endif
    /* Verifying... */
#if  defined(MODULE_HACL) || defined(MODULE_TWEETNACL)
    int res = crypto_sign_open(verify_result, &verify_result_len, sm, smlen, sign_pk);
#elif defined(MODULE_TINYCRYPT)
    tc_sha256_init(&sha);
    tc_sha256_update(&sha, message, sizeof(message));
    tc_sha256_final(digest, &sha);
    int res = uECC_verify(sign_pk, digest, 32, signature, &curve_secp256r1);
#elif defined(MODULE_MBEDTLS)
    mbedtls_sha256_starts( &sha256_ctx, 0 );
    mbedtls_sha256_update( &sha256_ctx, message, sizeof(message));
    mbedtls_sha256_finish( &sha256_ctx, digest);
    int res = mbedtls_ecdsa_read_signature( &ctx_verify, digest, sizeof(digest), signature, sig_len);
#elif defined(MODULE_C25519)
    int res = edsign_verify(signature, sign_pk, message, sizeof(message));
#elif defined(MODULE_MONOCYPHER)
    int res = crypto_check(signature, sign_pk, message, sizeof(message));
#elif defined(MODULE_QDSA)
    int res = verify(verify_result, 0, sm, smlen, sign_pk);
#elif defined(MODULE_LIBHYDROGEN)
    int res = hydro_sign_verify(signature, message, sizeof(message), CONTEXT, keypair.pk);
#endif
    after = xtimer_now_usec();
#ifdef DEVELHELP
    tafter = stacksz - thread_measure_stack_free(p->stack_start);
#endif

    printf("Res: %d, "
#ifdef DO_SIGN
            "before: %" PRIu32
#endif
            "middle: %" PRIu32", after: %" PRIu32"\n", res,
#ifdef DO_SIGN
            before,
#endif
            middle, after);

#ifdef DEVELHELP
    printf("Stack start: %i, "
#ifdef DO_SIGN
           "before: %i, "
#endif
           "middle: %i, after: %i\n", tstart,
#ifdef DO_SIGN
           tbefore,
#endif
           tmiddle, tafter);
#endif
    printf("\n");
    printf("Timing:"
#ifdef DO_SIGN
        "Result:\t%luus\t"
#endif
        "%luus\n",
#ifdef DO_SIGN
        (long unsigned)((middle - before)),
#endif
        (long unsigned)((after - middle)));
#ifdef DEVELHELP
    printf("stack :"
#ifdef DO_SIGN
           "Result:\t%iB\t"
#endif
           "%iB\n",
#ifdef DO_SIGN
           (tmiddle - tstart),
#endif
           (tafter - tstart));
#endif

    while(1) {}
    return 0;
}
