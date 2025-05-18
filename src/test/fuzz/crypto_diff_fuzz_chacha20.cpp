// Copyright (c) 2020-2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

/*
From https://cr.yp.to/chacha.html
chacha-merged.c version 20080118
D. J. Bernstein
Public domain.
*/

typedef unsigned int u32;
typedef unsigned char u8;

#define U8C(v) (v##U)
#define U32C(v) (v##U)

#define U8V(v) ((u8)(v)&U8C(0xFF))
#define U32V(v) ((u32)(v)&U32C(0xFFFFFFFF))

#define ROTL32(v, n) (U32V((v) << (n)) | ((v) >> (32 - (n))))

#define U8TO32_LITTLE(p)                                              \
    (((u32)((p)[0])) | ((u32)((p)[1]) << 8) | ((u32)((p)[2]) << 16) | \
     ((u32)((p)[3]) << 24))

#define U32TO8_LITTLE(p, v)      \
    do {                         \
        (p)[0] = U8V((v));       \
        (p)[1] = U8V((v) >> 8);  \
        (p)[2] = U8V((v) >> 16); \
        (p)[3] = U8V((v) >> 24); \
    } while (0)

/* ------------------------------------------------------------------------- */
/* Data structures */

typedef struct
{
    u32 input[16];
} ECRYPT_ctx;

/* ------------------------------------------------------------------------- */
/* Mandatory functions */

void ECRYPT_keysetup(
    ECRYPT_ctx* ctx,
    const u8* key,
    u32 keysize, /* Key size in bits. */
    u32 ivsize); /* IV size in bits. */

void ECRYPT_ivsetup(
    ECRYPT_ctx* ctx,
    const u8* iv);

void ECRYPT_encrypt_bytes(
    ECRYPT_ctx* ctx,
    const u8* plaintext,
    u8* ciphertext,
    u32 msglen); /* Message length in bytes. */

/* ------------------------------------------------------------------------- */

/* Optional features */

void ECRYPT_keystream_bytes(
    ECRYPT_ctx* ctx,
    u8* keystream,
    u32 length); /* Length of keystream in bytes. */

/* ------------------------------------------------------------------------- */

#define ROTATE(v, c) (ROTL32(v, c))
#define XOR(v, w) ((v) ^ (w))
#define PLUS(v, w) (U32V((v) + (w)))
#define PLUSONE(v) (PLUS((v), 1))

#define QUARTERROUND(a, b, c, d) \
    a = PLUS(a, b); d = ROTATE(XOR(d, a), 16);   \
    c = PLUS(c, d); b = ROTATE(XOR(b, c), 12);   \
    a = PLUS(a, b); d = ROTATE(XOR(d, a), 8);    \
    c = PLUS(c, d); b = ROTATE(XOR(b, c), 7);

static const char sigma[] = "expand 32-byte k";
static const char tau[] = "expand 16-byte k";

void ECRYPT_keysetup(ECRYPT_ctx* x, const u8* k, u32 kbits, u32 ivbits)
{
    const char* constants;

    x->input[4] = U8TO32_LITTLE(k + 0);
    x->input[5] = U8TO32_LITTLE(k + 4);
    x->input[6] = U8TO32_LITTLE(k + 8);
    x->input[7] = U8TO32_LITTLE(k + 12);
    if (kbits == 256) { /* recommended */
        k += 16;
        constants = sigma;
    } else { /* kbits == 128 */
        constants = tau;
    }
    x->input[8] = U8TO32_LITTLE(k + 0);
    x->input[9] = U8TO32_LITTLE(k + 4);
    x->input[10] = U8TO32_LITTLE(k + 8);
    x->input[11] = U8TO32_LITTLE(k + 12);
    x->input[0] = U8TO32_LITTLE(constants + 0);
    x->input[1] = U8TO32_LITTLE(constants + 4);
    x->input[2] = U8TO32_LITTLE(constants + 8);
    x->input[3] = U8TO32_LITTLE(constants + 12);
}

void ECRYPT_ivsetup(ECRYPT_ctx* x, const u8* iv)
{
    x->input[12] = 0;
    x->input[13] = 0;
    x->input[14] = U8TO32_LITTLE(iv + 0);
    x->input[15] = U8TO32_LITTLE(iv + 4);
}

void ECRYPT_encrypt_bytes(ECRYPT_ctx* x, const u8* m, u8* c, u32 bytes)
{
    u32 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
    u32 j0, j1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12, j13, j14, j15;
    u8* ctarget = nullptr;
    u8 tmp[64];
    uint32_t i;

    if (!bytes) return;

    j0 = x->input[0];
    j1 = x->input[1];
    j2 = x->input[2];
    j3 = x->input[3];
    j4 = x->input[4];
    j5 = x->input[5];
    j6 = x->input[6];
    j7 = x->input[7];
    j8 = x->input[8];
    j9 = x->input[9];
    j10 = x->input[10];
    j11 = x->input[11];
    j12 = x->input[12];
    j13 = x->input[13];
    j14 = x->input[14];
    j15 = x->input[15];

    for (;;) {
        if (bytes < 64) {
            for (i = 0; i < bytes; ++i)
                tmp[i] = m[i];
            m = tmp;
            ctarget = c;
            c = tmp;
        }
        x0 = j0;
        x1 = j1;
        x2 = j2;
        x3 = j3;
        x4 = j4;
        x5 = j5;
        x6 = j6;
        x7 = j7;
        x8 = j8;
        x9 = j9;
        x10 = j10;
        x11 = j11;
        x12 = j12;
        x13 = j13;
        x14 = j14;
        x15 = j15;
        for (i = 20; i > 0; i -= 2) {
            QUARTERROUND(x0, x4, x8, x12)
            QUARTERROUND(x1, x5, x9, x13)
            QUARTERROUND(x2, x6, x10, x14)
            QUARTERROUND(x3, x7, x11, x15)
            QUARTERROUND(x0, x5, x10, x15)
            QUARTERROUND(x1, x6, x11, x12)
            QUARTERROUND(x2, x7, x8, x13)
            QUARTERROUND(x3, x4, x9, x14)
        }
        x0 = PLUS(x0, j0);
        x1 = PLUS(x1, j1);
        x2 = PLUS(x2, j2);
        x3 = PLUS(x3, j3);
        x4 = PLUS(x4, j4);
        x5 = PLUS(x5, j5);
        x6 = PLUS(x6, j6);
        x7 = PLUS(x7, j7);
        x8 = PLUS(x8, j8);
        x9 = PLUS(x9, j9);
        x10 = PLUS(x10, j10);
        x11 = PLUS(x11, j11);
        x12 = PLUS(x12, j12);
        x13 = PLUS(x13, j13);
        x14 = PLUS(x14, j14);
        x15 = PLUS(x15, j15);

        x0 = XOR(x0, U8TO32_LITTLE(m + 0));
        x1 = XOR(x1, U8TO32_LITTLE(m + 4));
        x2 = XOR(x2, U8TO32_LITTLE(m + 8));
        x3 = XOR(x3, U8TO32_LITTLE(m + 12));
        x4 = XOR(x4, U8TO32_LITTLE(m + 16));
        x5 = XOR(x5, U8TO32_LITTLE(m + 20));
        x6 = XOR(x6, U8TO32_LITTLE(m + 24));
        x7 = XOR(x7, U8TO32_LITTLE(m + 28));
        x8 = XOR(x8, U8TO32_LITTLE(m + 32));
        x9 = XOR(x9, U8TO32_LITTLE(m + 36));
        x10 = XOR(x10, U8TO32_LITTLE(m + 40));
        x11 = XOR(x11, U8TO32_LITTLE(m + 44));
        x12 = XOR(x12, U8TO32_LITTLE(m + 48));
        x13 = XOR(x13, U8TO32_LITTLE(m + 52));
        x14 = XOR(x14, U8TO32_LITTLE(m + 56));
        x15 = XOR(x15, U8TO32_LITTLE(m + 60));

        j12 = PLUSONE(j12);
        if (!j12) {
            j13 = PLUSONE(j13);
            /* stopping at 2^70 bytes per nonce is user's responsibility */
        }

        U32TO8_LITTLE(c + 0, x0);
        U32TO8_LITTLE(c + 4, x1);
        U32TO8_LITTLE(c + 8, x2);
        U32TO8_LITTLE(c + 12, x3);
        U32TO8_LITTLE(c + 16, x4);
        U32TO8_LITTLE(c + 20, x5);
        U32TO8_LITTLE(c + 24, x6);
        U32TO8_LITTLE(c + 28, x7);
        U32TO8_LITTLE(c + 32, x8);
        U32TO8_LITTLE(c + 36, x9);
        U32TO8_LITTLE(c + 40, x10);
        U32TO8_LITTLE(c + 44, x11);
        U32TO8_LITTLE(c + 48, x12);
        U32TO8_LITTLE(c + 52, x13);
        U32TO8_LITTLE(c + 56, x14);
        U32TO8_LITTLE(c + 60, x15);

        if (bytes <= 64) {
            if (bytes < 64) {
                for (i = 0; i < bytes; ++i)
                    ctarget[i] = c[i];
            }
            x->input[12] = j12;
            x->input[13] = j13;
            return;
        }
        bytes -= 64;
        c += 64;
        m += 64;
    }
}

void ECRYPT_keystream_bytes(ECRYPT_ctx* x, u8* stream, u32 bytes)
{
    u32 i;
    for (i = 0; i < bytes; ++i)
        stream[i] = 0;
    ECRYPT_encrypt_bytes(x, stream, stream, bytes);
}

FUZZ_TARGET(crypto_diff_fuzz_chacha20)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    ECRYPT_ctx ctx;

    const std::vector<unsigned char> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, 32);
    ChaCha20 chacha20{MakeByteSpan(key)};
    ECRYPT_keysetup(&ctx, key.data(), key.size() * 8, 0);

    // ECRYPT_keysetup() doesn't set the counter and nonce to 0 while SetKey() does
    static const uint8_t iv[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ChaCha20::Nonce96 nonce{0, 0};
    uint32_t counter{0};
    ECRYPT_ivsetup(&ctx, iv);

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 3000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::vector<unsigned char> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, 32);
                chacha20.SetKey(MakeByteSpan(key));
                nonce = {0, 0};
                counter = 0;
                ECRYPT_keysetup(&ctx, key.data(), key.size() * 8, 0);
                // ECRYPT_keysetup() doesn't set the counter and nonce to 0 while SetKey() does
                uint8_t iv[8] = {0, 0, 0, 0, 0, 0, 0, 0};
                ECRYPT_ivsetup(&ctx, iv);
            },
            [&] {
                uint32_t iv_prefix = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
                uint64_t iv = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
                nonce = {iv_prefix, iv};
                counter = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
                chacha20.Seek(nonce, counter);
                ctx.input[12] = counter;
                ctx.input[13] = iv_prefix;
                ctx.input[14] = iv;
                ctx.input[15] = iv >> 32;
            },
            [&] {
                uint32_t integralInRange = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096);
                std::vector<uint8_t> output(integralInRange);
                chacha20.Keystream(MakeWritableByteSpan(output));
                std::vector<uint8_t> djb_output(integralInRange);
                ECRYPT_keystream_bytes(&ctx, djb_output.data(), djb_output.size());
                assert(output == djb_output);
                // DJB's version seeks forward to a multiple of 64 bytes after every operation. Correct for that.
                uint32_t old_counter = counter;
                counter += (integralInRange + 63) >> 6;
                if (counter < old_counter) ++nonce.first;
                if (integralInRange & 63) {
                    chacha20.Seek(nonce, counter);
                }
                assert(counter == ctx.input[12]);
            },
            [&] {
                uint32_t integralInRange = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096);
                std::vector<uint8_t> output(integralInRange);
                const std::vector<uint8_t> input = ConsumeFixedLengthByteVector(fuzzed_data_provider, output.size());
                chacha20.Crypt(MakeByteSpan(input), MakeWritableByteSpan(output));
                std::vector<uint8_t> djb_output(integralInRange);
                ECRYPT_encrypt_bytes(&ctx, input.data(), djb_output.data(), input.size());
                assert(output == djb_output);
                // DJB's version seeks forward to a multiple of 64 bytes after every operation. Correct for that.
                uint32_t old_counter = counter;
                counter += (integralInRange + 63) >> 6;
                if (counter < old_counter) ++nonce.first;
                if (integralInRange & 63) {
                    chacha20.Seek(nonce, counter);
                }
                assert(counter == ctx.input[12]);
            });
    }
}
