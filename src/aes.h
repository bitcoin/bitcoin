#ifndef AES_H
#define AES_H

#include <stdint.h>

#define AES_PARALLEL_N 8
#define BLOCK_COUNT 256

void ExpandAESKey256_int(uint32_t *keys, const uint32_t *KeyBuf);
void AES256CBC_int(uint32_t** data, const uint32_t** next, const uint32_t* ExpandedKey, uint32_t* IV);

#endif		// __WOLF_AES_H
