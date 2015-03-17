#ifndef SCRYPT_MINE_H
#define SCRYPT_MINE_H

#include <stdint.h>
#include <stdlib.h>

#include "util.h"
#include "net.h"

#define SCRYPT_BUFFER_SIZE (131072 + 63)

uint256 scrypt_blockhash(const uint8_t* input);

#ifdef USE_SSE2
void scrypt_detect_sse2();
#endif

#endif // SCRYPT_MINE_H
