/**********************************************************************
 * Copyright (c) 2017 Andrew Poelstra	                              *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_SCRATCH_
#define _SECP256K1_SCRATCH_

#define SECP256K1_SCRATCH_MAX_FRAMES	5

/* The typedef is used internally; the struct name is used in the public API
 * (where it is exposed as a different typedef) */
typedef struct secp256k1_scratch_space_struct {
    void *data[SECP256K1_SCRATCH_MAX_FRAMES];
    size_t offset[SECP256K1_SCRATCH_MAX_FRAMES];
    size_t frame_size[SECP256K1_SCRATCH_MAX_FRAMES];
    size_t frame;
    size_t max_size;
    const secp256k1_callback* error_callback;
} secp256k1_scratch;

static secp256k1_scratch* secp256k1_scratch_create(const secp256k1_callback* error_callback, size_t max_size);

static void secp256k1_scratch_destroy(secp256k1_scratch* scratch);

/** Attempts to allocate a new stack frame with `n` available bytes. Returns 1 on success, 0 on failure */
static int secp256k1_scratch_allocate_frame(secp256k1_scratch* scratch, size_t n, size_t objects);

/** Deallocates a stack frame */
static void secp256k1_scratch_deallocate_frame(secp256k1_scratch* scratch);

/** Returns the maximum allocation the scratch space will allow */
static size_t secp256k1_scratch_max_allocation(const secp256k1_scratch* scratch, size_t n_objects);

/** Returns a pointer into the most recently allocated frame, or NULL if there is insufficient available space */
static void *secp256k1_scratch_alloc(secp256k1_scratch* scratch, size_t n);

#endif
