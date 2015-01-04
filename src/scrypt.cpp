#include <stdlib.h>

#include "scrypt.h"
#include "pbkdf2.h"

#include "util.h"
#include "net.h"

#define SCRYPT_BUFFER_SIZE (131072 + 63)

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

    PBKDF2_SHA256(input, 80, input, 80, 1, (uint8_t *)X, 128);
    scrypt_core(X, V);
    PBKDF2_SHA256(input, 80, (uint8_t *)X, 128, 1, (uint8_t*)&result, 32);

    return result;
}
