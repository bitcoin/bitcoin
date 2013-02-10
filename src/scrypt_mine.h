#ifndef SCRYPT_MINE_H
#define SCRYPT_MINE_H

#include <stdint.h>
#include <stdlib.h>

#include "util.h"
#include "net.h"

typedef struct
{
    unsigned int version;
    uint256 prev_block;
    uint256 merkle_root;
    unsigned int timestamp;
    unsigned int bits;
    unsigned int nonce;

} block_header;

void *scrypt_buffer_alloc();
void scrypt_buffer_free(void *scratchpad);

unsigned int scanhash_scrypt(block_header *pdata, void *scratchbuf,
    uint32_t max_nonce, uint32_t &hash_count,
    void *result, block_header *res_header);

void scrypt_hash(const void* input, size_t inputlen, uint32_t *res, void *scratchpad);

#endif // SCRYPT_MINE_H
