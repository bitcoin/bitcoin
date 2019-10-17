/**********************************************************************
 * Copyright (c) 2017 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_SCRATCH_IMPL_H_
#define _SECP256K1_SCRATCH_IMPL_H_

#include "util.h"
#include "scratch.h"

static size_t secp256k1_scratch_checkpoint(const secp256k1_callback* error_callback, const secp256k1_scratch* scratch) {
    if (memcmp(scratch->magic, "scratch", 8) != 0) {
        secp256k1_callback_call(error_callback, "invalid scratch space");
        return 0;
    }
    return scratch->alloc_size;
}

static void secp256k1_scratch_apply_checkpoint(const secp256k1_callback* error_callback, secp256k1_scratch* scratch, size_t checkpoint) {
    if (memcmp(scratch->magic, "scratch", 8) != 0) {
        secp256k1_callback_call(error_callback, "invalid scratch space");
        return;
    }
    if (checkpoint > scratch->alloc_size) {
        secp256k1_callback_call(error_callback, "invalid checkpoint");
        return;
    }
    scratch->alloc_size = checkpoint;
}

static size_t secp256k1_scratch_max_allocation(const secp256k1_callback* error_callback, const secp256k1_scratch* scratch, size_t objects) {
    if (memcmp(scratch->magic, "scratch", 8) != 0) {
        secp256k1_callback_call(error_callback, "invalid scratch space");
        return 0;
    }
    if (scratch->max_size - scratch->alloc_size <= objects * (ALIGNMENT - 1)) {
        return 0;
    }
    return scratch->max_size - scratch->alloc_size - objects * (ALIGNMENT - 1);
}

static void *secp256k1_scratch_alloc(const secp256k1_callback* error_callback, secp256k1_scratch* scratch, size_t size) {
    void *ret;
    size = ROUND_TO_ALIGN(size);

    if (memcmp(scratch->magic, "scratch", 8) != 0) {
        secp256k1_callback_call(error_callback, "invalid scratch space");
        return NULL;
    }

    if (size > scratch->max_size - scratch->alloc_size) {
        return NULL;
    }
    ret = (void *) ((char *) scratch->data + scratch->alloc_size);
    memset(ret, 0, size);
    scratch->alloc_size += size;

    return ret;
}

#endif
