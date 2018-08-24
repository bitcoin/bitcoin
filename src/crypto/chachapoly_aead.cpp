// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chachapoly_aead.h>

#define __STDC_WANT_LIB_EXT1__ 1
#include <crypto/poly1305.h>
#include <string.h>

#include <compat/endian.h>
#include <support/cleanse.h>

#ifndef HAVE_TIMINGSAFE_BCMP

int timingsafe_bcmp(const unsigned char *b1, const unsigned char *b2, size_t n) {
  const unsigned char *p1 = b1, *p2 = b2;
  int ret = 0;

  for (; n > 0; n--)
    ret |= *p1++ ^ *p2++;
  return (ret != 0);
}

#endif /* TIMINGSAFE_BCMP */

int chacha20poly1305_init(struct chachapolyaead_ctx *ctx, const uint8_t *key,
                          int keylen) {
  if (keylen != (32 + 32)) /* 2 x 256 bit keys */
    return -1;
  chacha_keysetup(&ctx->main_ctx, key, 256);
  chacha_keysetup(&ctx->header_ctx, key + 32, 256);
  return 0;
}

int chacha20poly1305_crypt(struct chachapolyaead_ctx *ctx, uint32_t seqnr,
                           uint8_t *dest, const uint8_t *src, uint32_t len,
                           uint32_t aadlen, int is_encrypt) {
  const uint8_t one[8] = {1, 0, 0, 0, 0, 0, 0, 0}; /* NB little-endian */
  uint8_t expected_tag[POLY1305_TAGLEN], poly_key[POLY1305_KEYLEN];
  int r = 0;

  uint64_t seqnr64 = seqnr;
  uint64_t chacha_iv = htole64(seqnr64);
  memset(poly_key, 0, sizeof(poly_key));
  chacha_ivsetup(&ctx->main_ctx, (uint8_t *)&chacha_iv, NULL);
  chacha_encrypt_bytes(&ctx->main_ctx, poly_key, poly_key, sizeof(poly_key));

  if (!is_encrypt) {
    const uint8_t *tag = src + aadlen + len;

    poly1305_auth(expected_tag, src, aadlen + len, poly_key);
    if (timingsafe_bcmp(expected_tag, tag, POLY1305_TAGLEN) != 0) {
      r = -1;
      goto out;
    }
  }

  if (aadlen) {
    chacha_ivsetup(&ctx->header_ctx, (uint8_t *)&chacha_iv, NULL);
    chacha_encrypt_bytes(&ctx->header_ctx, src, dest, aadlen);
  }

  /* Set Chacha's block counter to 1 */
  chacha_ivsetup(&ctx->main_ctx, (uint8_t *)&chacha_iv, one);
  chacha_encrypt_bytes(&ctx->main_ctx, src + aadlen, dest + aadlen, len);

  /* If encrypting, calculate and append tag */
  if (is_encrypt) {
    poly1305_auth(dest + aadlen + len, dest, aadlen + len, poly_key);
  }
out:
  memory_cleanse(expected_tag, sizeof(expected_tag));
  memory_cleanse(&chacha_iv, sizeof(chacha_iv));
  memory_cleanse(poly_key, sizeof(poly_key));
  return r;
}

int chacha20poly1305_get_length24(struct chachapolyaead_ctx *ctx,
                                uint32_t *len24_out, uint32_t seqnr,
                                const uint8_t *ciphertext) {
  uint8_t buf[3];

  uint64_t seqnr64 = htole64(seqnr); // use LE for the nonce
  chacha_ivsetup(&ctx->header_ctx, (uint8_t *)&seqnr64, NULL);
  chacha_encrypt_bytes(&ctx->header_ctx, ciphertext, buf, 3);
  *len24_out = 0;
  *len24_out = buf[0] | (buf[1] << 8) | (buf[2] << 16);

  // convert to host endianness 32bit integer
  *len24_out = le32toh(*len24_out);
  return 0;
}
