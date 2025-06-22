#include <stdio.h>
#include <string.h>

#include "../wots.h"
#include "../rng.h"
#include "../params.h"

int main()
{
    /* Make stdout buffer more responsive. */
    setbuf(stdout, NULL);

    unsigned char seed[SPX_N];
    unsigned char pub_seed[SPX_N];
    unsigned char pk1[SPX_WOTS_PK_BYTES];
    unsigned char pk2[SPX_WOTS_PK_BYTES];
    unsigned char sig[SPX_WOTS_BYTES];
    unsigned char m[SPX_N];
    uint32_t addr[8] = {0};

    randombytes(seed, SPX_N);
    randombytes(pub_seed, SPX_N);
    randombytes(m, SPX_N);
    randombytes((unsigned char *)addr, 8 * sizeof(uint32_t));

    printf("Testing WOTS signature and PK derivation.. ");

    wots_gen_pk(pk1, seed, pub_seed, addr);
    wots_sign(sig, m, seed, pub_seed, addr);
    wots_pk_from_sig(pk2, sig, m, pub_seed, addr);

    if (memcmp(pk1, pk2, SPX_WOTS_PK_BYTES)) {
        printf("failed!\n");
        return -1;
    }
    printf("successful.\n");
    return 0;
}
