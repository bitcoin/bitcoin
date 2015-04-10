#ifndef SCRYPT_H
#define SCRYPT_H

#include <stdint.h>
#include <openssl/evp.h>
#include "uint256.h"

#define SCRYPT_BUFFER_SIZE (131072 + 63)

uint256 scrypt_blockhash(const uint8_t* input);

#endif // SCRYPT_H
