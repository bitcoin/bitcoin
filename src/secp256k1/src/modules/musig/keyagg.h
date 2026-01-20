/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_MUSIG_KEYAGG_H
#define SECP256K1_MODULE_MUSIG_KEYAGG_H

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_musig.h"

#include "../../group.h"
#include "../../scalar.h"

typedef struct {
    secp256k1_ge pk;
    /* If there is no "second" public key, second_pk is set to the point at
     * infinity */
    secp256k1_ge second_pk;
    unsigned char pks_hash[32];
    /* tweak is identical to value tacc[v] in the specification. */
    secp256k1_scalar tweak;
    /* parity_acc corresponds to (1 - gacc[v])/2 in the spec. So if gacc[v] is
     * -1, parity_acc is 1. Otherwise, parity_acc is 0. */
    int parity_acc;
} secp256k1_keyagg_cache_internal;

static int secp256k1_keyagg_cache_load(const secp256k1_context* ctx, secp256k1_keyagg_cache_internal *cache_i, const secp256k1_musig_keyagg_cache *cache);

static void secp256k1_musig_keyaggcoef(secp256k1_scalar *r, const secp256k1_keyagg_cache_internal *cache_i, secp256k1_ge *pk);

#endif
