#include <stdint.h>
#include <string.h>
#include <openssl/sha.h>

#include "hash_address.h"
#include "utils.h"
#include "params.h"
#include "hash.h"

#define SPX_SHA256_BLOCK_BYTES 64
#define SPX_SHA256_OUTPUT_BYTES 32  /* This does not necessarily equal SPX_N */

#if SPX_SHA256_OUTPUT_BYTES < SPX_N
    #error Linking against SHA-256 with N larger than 32 bytes is not supported
#endif

static void addr_to_bytes(unsigned char *bytes, const uint32_t addr[8])
{
    int i;

    for (i = 0; i < 8; i++) {
        ull_to_bytes(bytes + i*4, 4, addr[i]);
    }
}

/**
 * Note that inlen should be sufficiently small that it still allows for
 * an array to be allocated on the stack. Typically 'in' is merely a seed.
 * Outputs outlen number of bytes
 */
static void mgf1(unsigned char *out, unsigned long outlen,
                 const unsigned char *in, unsigned long inlen)
{
    unsigned char inbuf[inlen + 4];
    unsigned char outbuf[SPX_SHA256_OUTPUT_BYTES];
    unsigned long i;

    memcpy(inbuf, in, inlen);

    /* While we can fit in at least another full block of SHA256 output.. */
    for (i = 0; (i+1)*SPX_SHA256_OUTPUT_BYTES <= outlen; i++) {
        ull_to_bytes(inbuf + inlen, 4, i);
        SHA256(inbuf, inlen + 4, out);
        out += SPX_SHA256_OUTPUT_BYTES;
    }
    /* Until we cannot anymore, and we fill the remainder. */
    ull_to_bytes(inbuf + inlen, 4, i);
    SHA256(inbuf, inlen + 4, outbuf);
    memcpy(out, outbuf, outlen - i*SPX_SHA256_OUTPUT_BYTES);
}

/* For SHA256, there is no immediate reason to initialize at the start,
   so this function is an empty operation. */
void initialize_hash_function(const unsigned char *pub_seed,
                              const unsigned char *sk_seed)
{
    (void)pub_seed; /* Suppress an 'unused parameter' warning. */
    (void)sk_seed; /* Suppress an 'unused parameter' warning. */
}

/*
 * Computes PRF(key, addr), given a secret key of SPX_N bytes and an address
 */
void prf_addr(unsigned char *out, const unsigned char *key,
              const uint32_t addr[8])
{
    unsigned char buf[SPX_SHA256_BLOCK_BYTES + SPX_ADDR_BYTES];
    unsigned char outbuf[SPX_SHA256_OUTPUT_BYTES];

    /* We need to pad out the first block so that the key and the input are in
       separate compression function calls */
    memcpy(buf, key, SPX_N);
    memset(buf + SPX_N, 0, SPX_SHA256_BLOCK_BYTES - SPX_N);

    addr_to_bytes(buf + SPX_SHA256_BLOCK_BYTES, addr);

    SHA256(buf, SPX_SHA256_BLOCK_BYTES + SPX_ADDR_BYTES, outbuf);
    memcpy(out, outbuf, SPX_N);
}

/**
 * Computes the message-dependent randomness R, using a secret seed as a key
 * for HMAC, and an optional randomization value prefixed to the message.
 * This requires m to have at least SPX_SHA256_BLOCK_BYTES + SPX_N space
 * available in front of the pointer, i.e. before the message to use for the
 * prefix. This is necessary to prevent having to move the message around (and
 * allocate memory for it).
 */
void gen_message_random(unsigned char *R, const unsigned char *sk_prf,
                        const unsigned char *optrand,
                        unsigned char *m, unsigned long long mlen)
{
    unsigned char outbuf[SPX_SHA256_OUTPUT_BYTES];
    unsigned char tmp[SPX_SHA256_BLOCK_BYTES + SPX_SHA256_OUTPUT_BYTES];
    unsigned char *m_with_prefix = m - SPX_SHA256_BLOCK_BYTES - SPX_N;
    int i;

    /* This implements HMAC-SHA256 */
    memcpy(m_with_prefix, sk_prf, SPX_N);
    memset(m_with_prefix + SPX_N, 0, SPX_SHA256_BLOCK_BYTES - SPX_N);
    for (i = 0; i < SPX_SHA256_BLOCK_BYTES; i++) {
        m_with_prefix[i] ^= 0x36;
    }
    memcpy(m_with_prefix + SPX_SHA256_BLOCK_BYTES, optrand, SPX_N);

    SHA256(m_with_prefix, mlen + SPX_SHA256_BLOCK_BYTES + SPX_N,
           tmp + SPX_SHA256_BLOCK_BYTES);

    memcpy(tmp, sk_prf, SPX_N);
    memset(tmp + SPX_N, 0, SPX_SHA256_BLOCK_BYTES - SPX_N);
    for (i = 0; i < SPX_SHA256_BLOCK_BYTES; i++) {
        tmp[i] ^= 0x5c;
    }

    SHA256(tmp, SPX_SHA256_BLOCK_BYTES, outbuf);

    memcpy(R, outbuf, SPX_N);
}

/**
 * Computes the message hash using R, the public key, and the message.
 * Notably, it requires m to have SPX_N + SPX_PK_BYTES bytes of space available
 * in front of the pointer, i.e. before the message, to use for the prefix.
 * This is necessary to prevent having to move the message around (and
 * allocate memory for it).
 * Outputs the message digest and the index of the leaf. The index is split in
 * the tree index and the leaf index, for convenient copying to an address.
 */
void hash_message(unsigned char *digest, uint64_t *tree, uint32_t *leaf_idx,
                  const unsigned char *R, const unsigned char *pk,
                  unsigned char *m, unsigned long long mlen)
{
#define SPX_TREE_BITS (SPX_TREE_HEIGHT * (SPX_D - 1))
#define SPX_TREE_BYTES ((SPX_TREE_BITS + 7) / 8)
#define SPX_LEAF_BITS SPX_TREE_HEIGHT
#define SPX_LEAF_BYTES ((SPX_LEAF_BITS + 7) / 8)
#define SPX_DGST_BYTES (SPX_FORS_MSG_BYTES + SPX_TREE_BYTES + SPX_LEAF_BYTES)

    unsigned char seed[SPX_SHA256_OUTPUT_BYTES];
    unsigned char buf[SPX_DGST_BYTES];
    unsigned char *bufp = buf;

    memcpy(m - SPX_N - SPX_PK_BYTES, R, SPX_N);
    memcpy(m - SPX_PK_BYTES, pk, SPX_PK_BYTES);

    /* By doing this in two steps, we prevent hashing the message twice;
       otherwise each iteration in MGF1 would hash the message again. */
    SHA256(m - SPX_N - SPX_PK_BYTES, mlen + SPX_N + SPX_PK_BYTES, seed);

    mgf1(bufp, SPX_DGST_BYTES, seed, SPX_SHA256_OUTPUT_BYTES);

    memcpy(digest, bufp, SPX_FORS_MSG_BYTES);
    bufp += SPX_FORS_MSG_BYTES;

#if SPX_TREE_BITS > 64
    #error For given height and depth, 64 bits cannot represent all subtrees
#endif

    *tree = bytes_to_ull(bufp, SPX_TREE_BYTES);
    *tree &= (~(uint64_t)0) >> (64 - SPX_TREE_BITS);
    bufp += SPX_TREE_BYTES;

    *leaf_idx = bytes_to_ull(bufp, SPX_LEAF_BYTES);
    *leaf_idx &= (~(uint32_t)0) >> (32 - SPX_LEAF_BITS);
}

/**
 * Takes an array of inblocks concatenated arrays of SPX_N bytes.
 */
void thash(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const unsigned char *pub_seed, uint32_t addr[8])
{
    unsigned char buf[SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N];
    unsigned char outbuf[SPX_SHA256_OUTPUT_BYTES];
    unsigned char bitmask[inblocks * SPX_N];
    unsigned int i;

    memcpy(buf, pub_seed, SPX_N);
    addr_to_bytes(buf + SPX_N, addr);

    mgf1(bitmask, inblocks * SPX_N, buf, SPX_N + SPX_ADDR_BYTES);

    for (i = 0; i < inblocks * SPX_N; i++) {
        buf[SPX_N + SPX_ADDR_BYTES + i] = in[i] ^ bitmask[i];
    }

    SHA256(buf, SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N, outbuf);
    memcpy(out, outbuf, SPX_N);
}
