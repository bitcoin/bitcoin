#ifndef _SHA512_H
#define _SHA512_H

#include <stdint.h>

//SHA-512 block size
#define SHA512_BLOCK_SIZE 128
//SHA-512 digest size
#define SHA512_DIGEST_SIZE 64

#define SHA512_PARALLEL_N 8

//SHA-512 related functions
void sha512Compute32b_parallel(
        uint64_t *data[SHA512_PARALLEL_N],
        uint64_t *digest[SHA512_PARALLEL_N]);

#endif
