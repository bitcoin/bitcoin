#include "miner.h"
#include "algo-gate-api.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "sph_whirlpool.h"

void whirlpoolx_hash(void *state, const void *input)
{
    sph_whirlpool_context ctx_whirlpool;

    unsigned char hash[64];
    unsigned char hash_xored[32];

    sph_whirlpool1_init(&ctx_whirlpool);
    sph_whirlpool1(&ctx_whirlpool, input, 80);
    sph_whirlpool1_close(&ctx_whirlpool, hash);

    // compress the 48 first bytes of the hash to 32
    for (int i = 0; i < 32; i++)
        hash_xored[i] = hash[i] ^ hash[i + 16];

    memcpy(state, hash, 32);
}

int scanhash_whirlpoolx(int thr_id, struct work* work, uint32_t max_nonce, unsigned long *hashes_done)
{
    uint32_t _ALIGN(128) endiandata[20];
    uint32_t* pdata = work->data;
    uint32_t* ptarget = work->target;
    const uint32_t first_nonce = pdata[19];
    uint32_t n = first_nonce - 1;

//	if (opt_benchmark)
//		((uint32_t*)ptarget)[7] = 0x0000ff;

    for (int i=0; i < 19; i++)
        be32enc(&endiandata[i], pdata[i]);

    do {
        const uint32_t Htarg = ptarget[7];
        uint32_t vhash[8];
        pdata[19] = ++n;
        be32enc(&endiandata[19], n );
        whirlpoolx_hash(vhash, endiandata);

        if (vhash[7] <= Htarg && fulltest(vhash, ptarget))
        {
//			work_set_target_ratio(work, vhash);
            *hashes_done = n - first_nonce + 1;
            return true;
        }

    } while ( n < max_nonce && !work_restart[thr_id].restart);

    *hashes_done = n - first_nonce + 1;
    pdata[19] = n;
    return 0;
}

bool register_whirlpoolx_algo( algo_gate_t* gate )
{
    algo_not_tested();
    gate->scanhash  = (void*)&scanhash_whirlpoolx;
    gate->hash      = (void*)&whirlpoolx_hash;
    return true;
};
