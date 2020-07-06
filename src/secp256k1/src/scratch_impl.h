/**********************************************************************
 * Copyright (c) 2017 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_SCRATCH_IMPL_H_
#define _SECP256K1_SCRATCH_IMPL_H_

#include "scratch.h"

/* Using 16 bytes alignment because common architectures never have alignment
 * requirements above 8 for any of the types we care about. In addition we
 * leave some room because currently we don't care about a few bytes.
 * TODO: Determine this at configure time. */
#define ALIGNMENT 16

static secp256k1_scratch* secp256k1_scratch_create(const secp256k1_callback* error_callback, size_t max_size) {
    secp256k1_scratch* ret = (secp256k1_scratch*)checked_malloc(error_callback, sizeof(*ret));
    if (ret != NULL) {
        memset(ret, 0, sizeof(*ret));
        ret->max_size = max_size;
        ret->error_callback = error_callback;
    }
    return ret;
}

static void secp256k1_scratch_destroy(secp256k1_scratch* scratch) {
    if (scratch != NULL) {
        VERIFY_CHECK(scratch->frame == 0);
        free(scratch);
    }
}

static size_t secp256k1_scratch_max_allocation(const secp256k1_scratch* scratch, size_t objects) {
    size_t i = 0;
    size_t allocated = 0;
    for (i = 0; i < scratch->frame; i++) {
        allocated += scratch->frame_size[i];
    }
    if (scratch->max_size - allocated <= objects * ALIGNMENT) {
        return 0;
    }
    return scratch->max_size - allocated - objects * ALIGNMENT;
}

static int secp256k1_scratch_allocate_frame(secp256k1_scratch* scratch, size_t n, size_t objects) {
    VERIFY_CHECK(scratch->frame < SECP256K1_SCRATCH_MAX_FRAMES);

    if (n <= secp256k1_scratch_max_allocation(scratch, objects)) {
        n += objects * ALIGNMENT;
        scratch->data[scratch->frame] = checked_malloc(scratch->error_callback, n);
        if (scratch->data[scratch->frame] == NULL) {
            return 0;
        }
        scratch->frame_size[scratch->frame] = n;
        scratch->offset[scratch->frame] = 0;
        scratch->frame++;
        return 1;
    } else {
        return 0;
    }
}

static void secp256k1_scratch_deallocate_frame(secp256k1_scratch* scratch) {
    VERIFY_CHECK(scratch->frame > 0);
    scratch->frame -= 1;
    free(scratch->data[scratch->frame]);
}

static void *secp256k1_scratch_alloc(secp256k1_scratch* scratch, size_t size) {
    void *ret;
    size_t frame = scratch->frame - 1;
    size = ((size + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT;

    if (scratch->frame == 0 || size + scratch->offset[frame] > scratch->frame_size[frame]) {
        return NULL;
    }
    ret = (void *) ((unsigned char *) scratch->data[frame] + scratch->offset[frame]);
    memset(ret, 0, size);
    scratch->offset[frame] += size;

    return ret;
}

#endif
