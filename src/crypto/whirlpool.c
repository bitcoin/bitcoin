#include "miner.h"
#include "algo-gate-api.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "sph_whirlpool.h"

typedef struct {
   sph_whirlpool_context   whirl1;
   sph_whirlpool_context   whirl2;
   sph_whirlpool_context   whirl3;
   sph_whirlpool_context   whirl4;
} whirlpool_ctx_holder;

static whirlpool_ctx_holder whirl_ctx;
static __thread sph_whirlpool_context whirl1_mid_ctx;

void init_whirlpool_ctx()
{
    sph_whirlpool1_init( &whirl_ctx.whirl1 );
    sph_whirlpool1_init( &whirl_ctx.whirl2 );
    sph_whirlpool1_init( &whirl_ctx.whirl3 );
    sph_whirlpool1_init( &whirl_ctx.whirl4 );
}

void whirlpool_hash(void *state, const void *input)
{
    whirlpool_ctx_holder ctx;
    memcpy( &ctx, &whirl_ctx, sizeof(whirl_ctx) );

    const int midlen = 64;
    const int tail   = 80 - midlen;
    unsigned char hash[128]; // uint32_t hashA[16], hashB[16];
#define hashB hash+64

    // copy cached midstate
    memcpy( &ctx.whirl1, &whirl1_mid_ctx, sizeof whirl1_mid_ctx );
    sph_whirlpool1( &ctx.whirl1, input + midlen, tail );
    sph_whirlpool1_close(&ctx.whirl1, hash);

    sph_whirlpool1(&ctx.whirl2, hash, 64);
    sph_whirlpool1_close(&ctx.whirl2, hashB);

    sph_whirlpool1(&ctx.whirl3, hashB, 64);
    sph_whirlpool1_close(&ctx.whirl3, hash);

    sph_whirlpool1(&ctx.whirl4, hash, 64);
    sph_whirlpool1_close(&ctx.whirl4, hash);

    memcpy(state, hash, 32);
}

void whirlpool_midstate( const void* input )
{
    memcpy( &whirl1_mid_ctx, &whirl_ctx.whirl1, sizeof whirl1_mid_ctx );
    sph_whirlpool1( &whirl1_mid_ctx, input, 64 );
}


int scanhash_whirlpool(int thr_id, struct work* work, uint32_t max_nonce, unsigned long *hashes_done)
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

    whirlpool_midstate( endiandata );

	do {
		const uint32_t Htarg = ptarget[7];
		uint32_t vhash[8];
                pdata[19] = ++n;
		be32enc(&endiandata[19], n );
		whirlpool_hash(vhash, endiandata);

		if (vhash[7] <= Htarg && fulltest(vhash, ptarget)) {
//			work_set_target_ratio(work, vhash);
            *hashes_done = n - first_nonce + 1;
			return true;
		}

	} while ( n < max_nonce && !work_restart[thr_id].restart);

	*hashes_done = n - first_nonce + 1;
	pdata[19] = n;
	return 0;
}

bool register_whirlpool_algo( algo_gate_t* gate )
{
    gate->scanhash  = (void*)&scanhash_whirlpool;
    gate->hash      = (void*)&whirlpool_hash;
    init_whirlpool_ctx();
    return true;
};
