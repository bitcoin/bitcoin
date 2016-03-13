/**
 * @file /cryptron/ecies.c
 *
 * @brief ECIES encryption/decryption functions.
 *
 * $Author: Ladar Levison $
 * $Website: http://lavabit.com $
 *
 */

#include "ies.h"
#include <iostream>
#include <vector>

#define SET_ERROR(string) \
    sprintf(error, "%s %s:%d", (string), __FILE__, __LINE__)
#define SET_OSSL_ERROR(string) \
    sprintf(error, "%s {error = %s} %s:%d", (string), ERR_error_string(ERR_get_error(), NULL), __FILE__, __LINE__)

/* Copyright (c) 1998-2011 The OpenSSL Project. All rights reserved.
 * Taken from openssl/crypto/ecdh/ech_kdf.c in github:openssl/openssl
 * ffa08b3242e0f10f1fef3c93ef3f0b51de8c27a9 */

/* Key derivation function from X9.62/SECG */
/* Way more than we will ever need */
#define ECDH_KDF_MAX (1 << 30)
int ECDH_KDF_X9_62(unsigned char *out, size_t outlen,
                   const unsigned char *Z, size_t Zlen,
                   const unsigned char *sinfo, size_t sinfolen,
                   const EVP_MD *md)
{
    EVP_MD_CTX mctx;
    int rv = 0;
    unsigned int i;
    size_t mdlen;
    unsigned char ctr[4];
    if (sinfolen > ECDH_KDF_MAX || outlen > ECDH_KDF_MAX || Zlen > ECDH_KDF_MAX)
        return 0;
    mdlen = EVP_MD_size(md);
    EVP_MD_CTX_init(&mctx);
    for (i = 1;;i++)
    {
        unsigned char mtmp[EVP_MAX_MD_SIZE];
        EVP_DigestInit_ex(&mctx, md, NULL);
        ctr[3] = i & 0xFF;
        ctr[2] = (i >> 8) & 0xFF;
        ctr[1] = (i >> 16) & 0xFF;
        ctr[0] = (i >> 24) & 0xFF;
        if (!EVP_DigestUpdate(&mctx, Z, Zlen))
            goto err;
        if (!EVP_DigestUpdate(&mctx, ctr, sizeof(ctr)))
            goto err;
        if (!EVP_DigestUpdate(&mctx, sinfo, sinfolen))
            goto err;
        if (outlen >= mdlen)
        {
            if (!EVP_DigestFinal(&mctx, out, NULL))
                goto err;
            outlen -= mdlen;
            if (outlen == 0)
                break;
            out += mdlen;
        }
        else
        {
            if (!EVP_DigestFinal(&mctx, mtmp, NULL))
                goto err;
            memcpy(out, mtmp, outlen);
            OPENSSL_cleanse(mtmp, mdlen);
            break;
        }
    }
    rv = 1;
  err:
    EVP_MD_CTX_cleanup(&mctx);
    return rv;
}

static size_t envelope_key_len(const ies_ctx_t *ctx)
{
    return EVP_CIPHER_key_length(ctx->cipher) + EVP_MD_size(ctx->md);
}

static EC_KEY * ecies_key_create(const EC_KEY *user, char *error) {

    const EC_GROUP *group;
    EC_KEY *key = NULL;

    if (!(key = EC_KEY_new())) {
        SET_OSSL_ERROR("EC_KEY_new failed");
        return NULL;
    }

    if (!(group = EC_KEY_get0_group(user))) {
        SET_ERROR("The user key does not have group");
        EC_KEY_free(key);
        return NULL;
    }

    if (EC_KEY_set_group(key, group) != 1) {
        SET_OSSL_ERROR("EC_KEY_set_group failed");
        EC_KEY_free(key);
        return NULL;
    }

    if (EC_KEY_generate_key(key) != 1) {
        SET_OSSL_ERROR("EC_KEY_generate_key failed");
        EC_KEY_free(key);
        return NULL;
    }

    return key;
}

static unsigned char *prepare_envelope_key(const ies_ctx_t *ctx, cryptogram_t *cryptogram, char *error)
{

    const size_t key_buf_len = envelope_key_len(ctx);
    const size_t ecdh_key_len = (EC_GROUP_get_degree(EC_KEY_get0_group(ctx->user_key)) + 7) / 8;
    unsigned char *envelope_key = NULL, *ktmp = NULL;
    EC_KEY *ephemeral = NULL;
    size_t written_length;

    /* High-level ECDH via EVP does not allow use of arbitrary KDF function.
     * We should use low-level API for KDF2
     * c.f. openssl/crypto/ec/ec_pmeth.c */
    if ((envelope_key = (unsigned char *) OPENSSL_malloc(key_buf_len)) == NULL) {
        SET_ERROR("Failed to allocate memory for envelope_key");
        goto err;
    }

    if (!(ephemeral = ecies_key_create(ctx->user_key, error))) {
        goto err;
    }

    /* key agreement and KDF
     * reference: openssl/crypto/ec/ec_pmeth.c */
    ktmp = (unsigned char *) OPENSSL_malloc(ecdh_key_len);
    if (ktmp == NULL) {
        SET_ERROR("No memory for ECDH temporary key");
        goto err;
    }

    if (ECDH_compute_key(ktmp, ecdh_key_len, EC_KEY_get0_public_key(ctx->user_key), ephemeral, NULL)
        != (int)ecdh_key_len) {
        SET_OSSL_ERROR("An error occurred while ECDH_compute_key");
        goto err;
    }

    /* equals to ISO 18033-2 KDF2 */
    if (!ECDH_KDF_X9_62(envelope_key, key_buf_len, ktmp, ecdh_key_len, 0, 0, ctx->kdf_md)) {
        SET_OSSL_ERROR("Failed to stretch with KDF2");
        goto err;
    }

    /* Store the public key portion of the ephemeral key. */
    written_length = EC_POINT_point2oct(
        EC_KEY_get0_group(ephemeral),
        EC_KEY_get0_public_key(ephemeral),
        POINT_CONVERSION_COMPRESSED,
        cryptogram_key_data(cryptogram),
        ctx->stored_key_length,
        NULL);
    if (written_length == 0) {
        SET_OSSL_ERROR("Error while recording the public portion of the envelope key");
        goto err;
    }
    if (written_length != ctx->stored_key_length) {
        SET_ERROR("Written envelope key length does not match with expected");
        goto err;
    }

    EC_KEY_free(ephemeral);
    OPENSSL_cleanse(ktmp, ecdh_key_len);
    OPENSSL_free(ktmp);

    return envelope_key;

  err:
    if (ephemeral)
        EC_KEY_free(ephemeral);
    if (envelope_key) {
        OPENSSL_cleanse(envelope_key, key_buf_len);
        OPENSSL_free(envelope_key);
    }
    if (ktmp) {
        OPENSSL_cleanse(ktmp, ecdh_key_len);
        OPENSSL_free(ktmp);
    }
    return NULL;
}

static int store_cipher_body(
    const ies_ctx_t *ctx,
    const unsigned char *envelope_key,
    const unsigned char *data,
    size_t length,
    cryptogram_t *cryptogram,
    char *error)
{
    int out_len, len_sum = 0;
    size_t expected_len = cryptogram_body_length(cryptogram);
    unsigned char iv[EVP_MAX_IV_LENGTH];
    EVP_CIPHER_CTX cipher;
    unsigned char *body;

    /* For now we use an empty initialization vector. */
    memset(iv, 0, EVP_MAX_IV_LENGTH);

    EVP_CIPHER_CTX_init(&cipher);
    body = cryptogram_body_data(cryptogram);

    if (EVP_EncryptInit_ex(&cipher, ctx->cipher, NULL, envelope_key, iv) != 1
        || EVP_EncryptUpdate(&cipher, body, &out_len, data, length) != 1) {
        SET_OSSL_ERROR("Error while trying to secure the data using the symmetric cipher");
        EVP_CIPHER_CTX_cleanup(&cipher);
        return 0;
    }

    if (expected_len < (size_t)out_len) {
        SET_ERROR("The symmetric cipher overflowed");
        EVP_CIPHER_CTX_cleanup(&cipher);
        return 0;
    }

    body += out_len;
    len_sum += out_len;
    if (EVP_EncryptFinal_ex(&cipher, body, &out_len) != 1) {
        SET_OSSL_ERROR("Error while finalizing the data using the symmetric cipher");
        EVP_CIPHER_CTX_cleanup(&cipher);
        cryptogram_free(cryptogram);
        return 0;
    }

    EVP_CIPHER_CTX_cleanup(&cipher);

    if (expected_len < (size_t)len_sum) {
        SET_ERROR("The symmetric cipher overflowed");
        return 0;
    }

    return 1;
}

static int store_mac_tag(const ies_ctx_t *ctx, const unsigned char *envelope_key, cryptogram_t *cryptogram, char *error) {
    const size_t key_offset = EVP_CIPHER_key_length(ctx->cipher);
    const size_t key_length = EVP_MD_size(ctx->md);
    const size_t mac_length = cryptogram_mac_length(cryptogram);
    unsigned int out_len;
    HMAC_CTX hmac;

    HMAC_CTX_init(&hmac);

    /* Generate hash tag using encrypted data */
    if (HMAC_Init_ex(&hmac, envelope_key + key_offset, key_length, ctx->md, NULL) != 1
        || HMAC_Update(&hmac, cryptogram_body_data(cryptogram), cryptogram_body_length(cryptogram)) != 1
        || HMAC_Final(&hmac, cryptogram_mac_data(cryptogram), &out_len) != 1) {
        SET_OSSL_ERROR("Unable to generate tag");
        HMAC_CTX_cleanup(&hmac);
        return 0;
    }

    HMAC_CTX_cleanup(&hmac);

    if (out_len != mac_length) {
        SET_ERROR("MAC length expectation does not meet");
        return 0;
    }

    return 1;
}

cryptogram_t * ecies_encrypt(const ies_ctx_t *ctx, const unsigned char *data, size_t length, char *error) {

    if (!ctx || !data || !length) {
        SET_ERROR("Invalid arguments");
        return NULL;
    }

    const size_t block_length = EVP_CIPHER_block_size(ctx->cipher);
    const size_t mac_length = EVP_MD_size(ctx->md);
    cryptogram_t *cryptogram = NULL;
    unsigned char *envelope_key = NULL;

    if (block_length == 0 || block_length > EVP_MAX_BLOCK_LENGTH) {
        SET_ERROR("Derived block size is incorrect");
        return NULL;
    }

    cryptogram = cryptogram_alloc(ctx->stored_key_length,
                                  mac_length,
                                  length + (length % block_length ? (block_length - (length % block_length)) : 0));
    if (!cryptogram) {
        SET_ERROR("Unable to allocate a cryptogram_t buffer to hold the encrypted result.");
        goto err;
    }

    if ((envelope_key = prepare_envelope_key(ctx, cryptogram, error)) == NULL) {
        goto err;
    }

    if (!store_cipher_body(ctx, envelope_key, data, length, cryptogram, error)) {
        goto err;
    }

    if (!store_mac_tag(ctx, envelope_key, cryptogram, error)) {
        goto err;
    }

    OPENSSL_cleanse(envelope_key, envelope_key_len(ctx));
    OPENSSL_free(envelope_key);

    return cryptogram;

  err:
    if (cryptogram)
        cryptogram_free(cryptogram);
    if (envelope_key) {
        OPENSSL_cleanse(envelope_key, envelope_key_len(ctx));
        OPENSSL_free(envelope_key);
    }
    return NULL;
}

static EC_KEY *ecies_key_create_public_octets(EC_KEY *user, unsigned char *octets, size_t length, char *error) {

    EC_KEY *key = NULL;
    EC_POINT *point = NULL;
    const EC_GROUP *group = NULL;

    if (!(key = EC_KEY_new())) {
        SET_OSSL_ERROR("Cannot create instance for ephemeral key");
        return NULL;
    }

    if (!(group = EC_KEY_get0_group(user))) {
        SET_ERROR("Cannot get group from user key");
        EC_KEY_free(key);
        return NULL;
    }

    if (EC_KEY_set_group(key, group) != 1) {
        SET_OSSL_ERROR("EC_KEY_set_group failed");
        EC_KEY_free(key);
        return NULL;
    }

    if (!(point = EC_POINT_new(group))) {
        SET_OSSL_ERROR("EC_POINT_new failed");
        EC_KEY_free(key);
        return NULL;
    }

    if (EC_POINT_oct2point(group, point, octets, length, NULL) != 1) {
        SET_OSSL_ERROR("EC_POINT_oct2point failed");
        EC_KEY_free(key);
        return NULL;
    }

    if (EC_KEY_set_public_key(key, point) != 1) {
        SET_OSSL_ERROR("EC_KEY_set_public_key failed");
        EC_POINT_free(point);
        EC_KEY_free(key);
        return NULL;
    }

    EC_POINT_free(point);

    if (EC_KEY_check_key(key) != 1) {
        SET_OSSL_ERROR("EC_KEY_check_key failed");
        EC_KEY_free(key);
        return NULL;
    }

    return key;
}

unsigned char *restore_envelope_key(const ies_ctx_t *ctx, const cryptogram_t *cryptogram, char *error)
{

    const size_t key_buf_len = envelope_key_len(ctx);
    const size_t ecdh_key_len = (EC_GROUP_get_degree(EC_KEY_get0_group(ctx->user_key)) + 7) / 8;
    EC_KEY *ephemeral = NULL, *user_copy = NULL;
    unsigned char *envelope_key = NULL, *ktmp = NULL;

    if ((envelope_key = (unsigned char *) OPENSSL_malloc(key_buf_len)) == NULL) {
        SET_ERROR("Failed to allocate memory for envelope_key");
        goto err;
    }

    if (!(user_copy = EC_KEY_new())) {
        SET_OSSL_ERROR("Failed to create instance for user key copy");
        goto err;
    }

    if (!(EC_KEY_copy(user_copy, ctx->user_key))) {
        SET_OSSL_ERROR("Failed to copy user key");
        goto err;
    }

    if (!(ephemeral = ecies_key_create_public_octets(user_copy, cryptogram_key_data(cryptogram), cryptogram_key_length(cryptogram), error))) {
        goto err;
    }

    /* key agreement and KDF
     * reference: openssl/crypto/ec/ec_pmeth.c */
    ktmp = (unsigned char *) OPENSSL_malloc(ecdh_key_len);
    if (ktmp == NULL) {
        SET_ERROR("No memory for ECDH temporary key");
        goto err;
    }

    if (ECDH_compute_key(ktmp, ecdh_key_len, EC_KEY_get0_public_key(ephemeral), user_copy, NULL)
        != (int)ecdh_key_len) {
        SET_OSSL_ERROR("An error occurred while ECDH_compute_key");
        goto err;
    }

    /* equals to ISO 18033-2 KDF2 */
    if (!ECDH_KDF_X9_62(envelope_key, key_buf_len, ktmp, ecdh_key_len, 0, 0, ctx->kdf_md)) {
        SET_OSSL_ERROR("Failed to stretch with KDF2");
        goto err;
    }

    EC_KEY_free(user_copy);
    EC_KEY_free(ephemeral);
    OPENSSL_cleanse(ktmp, ecdh_key_len);
    OPENSSL_free(ktmp);

    return envelope_key;

  err:
    if (ephemeral)
        EC_KEY_free(ephemeral);
    if (user_copy)
        EC_KEY_free(user_copy);
    if (envelope_key) {
        OPENSSL_cleanse(envelope_key, key_buf_len);
        OPENSSL_free(envelope_key);
    }
    if (ktmp) {
        OPENSSL_cleanse(ktmp, ecdh_key_len);
        OPENSSL_free(ktmp);
    }
    return NULL;
}

static int verify_mac(const ies_ctx_t *ctx, const cryptogram_t *cryptogram, const unsigned char * envelope_key, char *error)
{
    const size_t key_offset = EVP_CIPHER_key_length(ctx->cipher);
    const size_t key_length = EVP_MD_size(ctx->md);
    const size_t mac_length = cryptogram_mac_length(cryptogram);
    unsigned int out_len;
    HMAC_CTX hmac;
    unsigned char md[EVP_MAX_MD_SIZE];

    HMAC_CTX_init(&hmac);

    /* Generate hash tag using encrypted data */
    if (HMAC_Init_ex(&hmac, envelope_key + key_offset, key_length, ctx->md, NULL) != 1
        || HMAC_Update(&hmac, cryptogram_body_data(cryptogram), cryptogram_body_length(cryptogram)) != 1
        || HMAC_Final(&hmac, md, &out_len) != 1) {
        SET_OSSL_ERROR("Unable to generate tag");
        HMAC_CTX_cleanup(&hmac);
        return 0;
    }

    HMAC_CTX_cleanup(&hmac);

    if (out_len != mac_length) {
        SET_ERROR("MAC length expectation does not meet");
        return 0;
    }

    if (memcmp(md, cryptogram_mac_data(cryptogram), mac_length) != 0) {
        SET_ERROR("MAC tag verification failed");
        return 0;
    }

    return 1;
}

unsigned char *decrypt_body(const ies_ctx_t *ctx, const cryptogram_t *cryptogram, const unsigned char *envelope_key, size_t *length, char *error)
{
    int out_len;
    size_t output_sum;
    const size_t body_length = cryptogram_body_length(cryptogram);
    unsigned char iv[EVP_MAX_IV_LENGTH], *block, *output;
    EVP_CIPHER_CTX cipher;

    if (!(output = (unsigned char*)malloc(body_length + 1))) {
        SET_ERROR("Failed to allocate memory for clear text");
        return NULL;
    }

    /* For now we use an empty initialization vector */
    memset(iv, 0, EVP_MAX_IV_LENGTH);
    memset(output, 0, body_length + 1);

    EVP_CIPHER_CTX_init(&cipher);

    block = output;
    if (EVP_DecryptInit_ex(&cipher, ctx->cipher, NULL, envelope_key, iv) != 1
        || EVP_DecryptUpdate(&cipher, block, &out_len, cryptogram_body_data(cryptogram), body_length) != 1) {
        SET_OSSL_ERROR("Unable to decrypt");
        EVP_CIPHER_CTX_cleanup(&cipher);
        free(output);
        return NULL;
    }
    output_sum = out_len;

    block += output_sum;
    if (EVP_DecryptFinal_ex(&cipher, block, &out_len) != 1) {
        printf("Unable to decrypt the data using the chosen symmetric cipher. {error = %s}\n", ERR_error_string(ERR_get_error(), NULL));
        EVP_CIPHER_CTX_cleanup(&cipher);
        free(output);
        return NULL;
    }
    output_sum += out_len;

    EVP_CIPHER_CTX_cleanup(&cipher);

    *length = output_sum;

    return output;
}

unsigned char * ecies_decrypt(const ies_ctx_t *ctx, const cryptogram_t *cryptogram, size_t *length, char *error)
{
    unsigned char *envelope_key = NULL, *output = NULL;

    if (!ctx || !cryptogram || !length || !error) {
        SET_ERROR("Invalid argument");
        goto err;
    }

    envelope_key = restore_envelope_key(ctx, cryptogram, error);
    if (envelope_key == NULL) {
        goto err;
    }

    if (!verify_mac(ctx, cryptogram, envelope_key, error)) {
        goto err;
    }

    if ((output = decrypt_body(ctx, cryptogram, envelope_key, length, error)) == NULL) {
        goto err;
    }

  err:
    OPENSSL_cleanse(envelope_key, envelope_key_len(ctx));
    OPENSSL_free(envelope_key);

    return output;
}

ies_ctx_t *create_context(EC_KEY *user_key)
{
    try {
      ies_ctx_t* ctx = new ies_ctx_t;
      ctx->cipher = EVP_aes_128_cbc();
      ctx->md = EVP_ripemd160();
      ctx->kdf_md = EVP_ripemd160();
      ctx->stored_key_length = 33;
      ctx->user_key = user_key;
      return ctx;
    }
    catch (const std::bad_alloc& e) {
      printf("Error: %s in %s:%d\n", e.what(), __FILE__, __LINE__);
// TODO: add checking to NULL where necessary
      return NULL;
    }
}
