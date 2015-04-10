#include "scrypt.h"

extern "C" void scrypt_core(uint32_t *X, uint32_t *V);

/* cpu and memory intensive function to transform a 80 byte buffer into a 32 byte output
   scratchpad size needs to be at least 63 + (128 * r * p) + (256 * r + 64) + (128 * r * N) bytes
   r = 1, p = 1, N = 1024
 */
uint256 scrypt_blockhash(const uint8_t* input)
{
    uint8_t scratchpad[SCRYPT_BUFFER_SIZE];
    uint32_t X[32];
    uint256 result = 0;

    uint32_t *V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

    PKCS5_PBKDF2_HMAC((const char*)input, 80, input, 80, 1, EVP_sha256(), 128, (unsigned char *)X);
    scrypt_core(X, V);
    PKCS5_PBKDF2_HMAC((const char*)input, 80, (const unsigned char*)X, 128, 1, EVP_sha256(), 32, (unsigned char*)&result);

    return result;
}
