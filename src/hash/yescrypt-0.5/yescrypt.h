/*-
 * Copyright 2009 Colin Percival
 * Copyright 2013,2014 Alexander Peslyak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file was originally written by Colin Percival as part of the Tarsnap
 * online backup system.
 */
#ifndef _YESCRYPT_H_
#define _YESCRYPT_H_

#include <stdint.h>
#include <stdlib.h> /* for size_t */

/**
 * Internal type used by the memory allocator.  Please do not use it directly.
 * Use yescrypt_shared_t and yescrypt_local_t as appropriate instead, since
 * they might differ from each other in a future version.
 */
typedef struct {
	void * base, * aligned;
	size_t base_size, aligned_size;
} yescrypt_region_t;

/**
 * Types for shared (ROM) and thread-local (RAM) data structures.
 */
typedef yescrypt_region_t yescrypt_shared1_t;
typedef struct {
	yescrypt_shared1_t shared1;
	uint32_t mask1;
} yescrypt_shared_t;
typedef yescrypt_region_t yescrypt_local_t;

/**
 * Possible values for yescrypt_init_shared()'s flags argument.
 */
typedef enum {
	YESCRYPT_SHARED_DEFAULTS = 0,
	YESCRYPT_SHARED_PREALLOCATED = 0x100
} yescrypt_init_shared_flags_t;

/**
 * Possible values for the flags argument of yescrypt_kdf(),
 * yescrypt_gensalt_r(), yescrypt_gensalt().  These may be OR'ed together,
 * except that YESCRYPT_WORM and YESCRYPT_RW are mutually exclusive.
 * Please refer to the description of yescrypt_kdf() below for the meaning of
 * these flags.
 */
typedef enum {
/* public */
	YESCRYPT_WORM = 0,
	YESCRYPT_RW = 1,
	YESCRYPT_PARALLEL_SMIX = 2,
	YESCRYPT_PWXFORM = 4,
/* private */
	__YESCRYPT_INIT_SHARED_1 = 0x10000,
	__YESCRYPT_INIT_SHARED_2 = 0x20000,
	__YESCRYPT_INIT_SHARED = 0x30000
} yescrypt_flags_t;

#define YESCRYPT_KNOWN_FLAGS \
	(YESCRYPT_RW | YESCRYPT_PARALLEL_SMIX | YESCRYPT_PWXFORM | \
	__YESCRYPT_INIT_SHARED)

/**
 * yescrypt_init_shared(shared, param, paramlen, N, r, p, flags, mask,
 *     buf, buflen):
 * Optionally allocate memory for and initialize the shared (ROM) data
 * structure.  The parameters N, r, and p must satisfy the same conditions as
 * with crypto_scrypt().  param and paramlen specify a local parameter with
 * which the ROM is seeded.  If buf is not NULL, then it is used to return
 * buflen bytes of message digest for the initialized ROM (the caller may use
 * this to verify that the ROM has been computed in the same way that it was on
 * a previous run).
 *
 * Return 0 on success; or -1 on error.
 *
 * If bit YESCRYPT_SHARED_PREALLOCATED in flags is set, then memory for the
 * ROM is assumed to have been preallocated by the caller, with
 * shared->shared1.aligned being the start address of the ROM and
 * shared->shared1.aligned_size being its size (which must be consistent with
 * N, r, and p).  This may be used e.g. when the ROM is to be placed in a SysV
 * shared memory segment allocated by the caller.
 *
 * mask controls the frequency of ROM accesses by yescrypt_kdf().  Normally it
 * should be set to 1, to interleave RAM and ROM accesses, which works well
 * when both regions reside in the machine's RAM anyway.  Other values may be
 * used e.g. when the ROM is memory-mapped from a disk file.  Recommended mask
 * values are powers of 2 minus 1 or minus 2.  Here's the effect of some mask
 * values:
 * mask	value	ROM accesses in SMix 1st loop	ROM accesses in SMix 2nd loop
 *	0		0				1/2
 *	1		1/2				1/2
 *	2		0				1/4
 *	3		1/4				1/4
 *	6		0				1/8
 *	7		1/8				1/8
 *	14		0				1/16
 *	15		1/16				1/16
 *	1022		0				1/1024
 *	1023		1/1024				1/1024
 *
 * Actual computation of the ROM contents may be avoided, if you don't intend
 * to use a ROM but need a dummy shared structure, by calling this function
 * with NULL, 0, 0, 0, 0, YESCRYPT_SHARED_DEFAULTS, 0, NULL, 0 for the
 * arguments starting with param and on.
 *
 * MT-safe as long as shared is local to the thread.
 */
static int yescrypt_init_shared(yescrypt_shared_t * __shared,
    const uint8_t * __param, size_t __paramlen,
    uint64_t __N, uint32_t __r, uint32_t __p,
    yescrypt_init_shared_flags_t __flags, uint32_t __mask,
    uint8_t * __buf, size_t __buflen);

/**
 * yescrypt_free_shared(shared):
 * Free memory that had been allocated with yescrypt_init_shared().
 *
 * Return 0 on success; or -1 on error.
 *
 * MT-safe as long as shared is local to the thread.
 */
static int yescrypt_free_shared(yescrypt_shared_t * __shared);

/**
 * yescrypt_init_local(local):
 * Initialize the thread-local (RAM) data structure.  Actual memory allocation
 * is currently fully postponed until a call to yescrypt_kdf() or yescrypt_r().
 *
 * Return 0 on success; or -1 on error.
 *
 * MT-safe as long as local is local to the thread.
 */
static int yescrypt_init_local(yescrypt_local_t * __local);

/**
 * yescrypt_free_local(local):
 * Free memory that may have been allocated for an initialized thread-local
 * (RAM) data structure.
 *
 * Return 0 on success; or -1 on error.
 *
 * MT-safe as long as local is local to the thread.
 */
static int yescrypt_free_local(yescrypt_local_t * __local);

/**
 * yescrypt_kdf(shared, local, passwd, passwdlen, salt, saltlen,
 *     N, r, p, t, flags, buf, buflen):
 * Compute scrypt(passwd[0 .. passwdlen - 1], salt[0 .. saltlen - 1], N, r,
 * p, buflen), or a revision of scrypt as requested by flags and shared, and
 * write the result into buf.  The parameters N, r, p, and buflen must satisfy
 * the same conditions as with crypto_scrypt().  t controls computation time
 * while not affecting peak memory usage.  shared and flags may request
 * special modes as described below.  local is the thread-local data
 * structure, allowing to preserve and reuse a memory allocation across calls,
 * thereby reducing its overhead.
 *
 * Return 0 on success; or -1 on error.
 *
 * t controls computation time.  t = 0 is optimal in terms of achieving the
 * highest area-time for ASIC attackers.  Thus, higher computation time, if
 * affordable, is best achieved by increasing N rather than by increasing t.
 * However, if the higher memory usage (which goes along with higher N) is not
 * affordable, or if fine-tuning of the time is needed (recall that N must be a
 * power of 2), then t = 1 or above may be used to increase time while staying
 * at the same peak memory usage.  t = 1 increases the time by 25% and
 * decreases the normalized area-time to 96% of optimal.  (Of course, in
 * absolute terms the area-time increases with higher t.  It's just that it
 * would increase slightly more with higher N*r rather than with higher t.)
 * t = 2 increases the time by another 20% and decreases the normalized
 * area-time to 89% of optimal.  Thus, these two values are reasonable to use
 * for fine-tuning.  Values of t higher than 2 result in further increase in
 * time while reducing the efficiency much further (e.g., down to around 50% of
 * optimal for t = 5, which runs 3 to 4 times slower than t = 0, with exact
 * numbers varying by the flags settings).
 *
 * Classic scrypt is available by setting t = 0 and flags to YESCRYPT_WORM and
 * passing a dummy shared structure (see the description of
 * yescrypt_init_shared() above for how to produce one).  In this mode, the
 * thread-local memory region (RAM) is first sequentially written to and then
 * randomly read from.  This algorithm is friendly towards time-memory
 * tradeoffs (TMTO), available both to defenders (albeit not in this
 * implementation) and to attackers.
 *
 * Setting YESCRYPT_RW adds extra random reads and writes to the thread-local
 * memory region (RAM), which makes TMTO a lot less efficient.  This may be
 * used to slow down the kinds of attackers who would otherwise benefit from
 * classic scrypt's efficient TMTO.  Since classic scrypt's TMTO allows not
 * only for the tradeoff, but also for a decrease of attacker's area-time (by
 * up to a constant factor), setting YESCRYPT_RW substantially increases the
 * cost of attacks in area-time terms as well.  Yet another benefit of it is
 * that optimal area-time is reached at an earlier time than with classic
 * scrypt, and t = 0 actually corresponds to this earlier completion time,
 * resulting in quicker hash computations (and thus in higher request rate
 * capacity).  Due to these properties, YESCRYPT_RW should almost always be
 * set, except when compatibility with classic scrypt or TMTO-friendliness are
 * desired.
 *
 * YESCRYPT_PARALLEL_SMIX moves parallelism that is present with p > 1 to a
 * lower level as compared to where it is in classic scrypt.  This reduces
 * flexibility for efficient computation (for both attackers and defenders) by
 * requiring that, short of resorting to TMTO, the full amount of memory be
 * allocated as needed for the specified p, regardless of whether that
 * parallelism is actually being fully made use of or not.  (For comparison, a
 * single instance of classic scrypt may be computed in less memory without any
 * CPU time overhead, but in more real time, by not making full use of the
 * parallelism.)  This may be desirable when the defender has enough memory
 * with sufficiently low latency and high bandwidth for efficient full parallel
 * execution, yet the required memory size is high enough that some likely
 * attackers might end up being forced to choose between using higher latency
 * memory than they could use otherwise (waiting for data longer) or using TMTO
 * (waiting for data more times per one hash computation).  The area-time cost
 * for other kinds of attackers (who would use the same memory type and TMTO
 * factor or no TMTO either way) remains roughly the same, given the same
 * running time for the defender.  In the TMTO-friendly YESCRYPT_WORM mode, as
 * long as the defender has enough memory that is just as fast as the smaller
 * per-thread regions would be, doesn't expect to ever need greater
 * flexibility (except possibly via TMTO), and doesn't need backwards
 * compatibility with classic scrypt, there are no other serious drawbacks to
 * this setting.  In the YESCRYPT_RW mode, which is meant to discourage TMTO,
 * this new approach to parallelization makes TMTO less inefficient.  (This is
 * an unfortunate side-effect of avoiding some random writes, as we have to in
 * order to allow for parallel threads to access a common memory region without
 * synchronization overhead.)  Thus, in this mode this setting poses an extra
 * tradeoff of its own (higher area-time cost for a subset of attackers vs.
 * better TMTO resistance).  Setting YESCRYPT_PARALLEL_SMIX also changes the
 * way the running time is to be controlled from N*r*p (for classic scrypt) to
 * N*r (in this modification).  All of this applies only when p > 1.  For
 * p = 1, this setting is a no-op.
 *
 * Passing a real shared structure, with ROM contents previously computed by
 * yescrypt_init_shared(), enables the use of ROM and requires YESCRYPT_RW for
 * the thread-local RAM region.  In order to allow for initialization of the
 * ROM to be split into a separate program, the shared->shared1.aligned and
 * shared->shared1.aligned_size fields may be set by the caller of
 * yescrypt_kdf() manually rather than with yescrypt_init_shared().
 *
 * local must be initialized with yescrypt_init_local().
 *
 * MT-safe as long as local and buf are local to the thread.
 */
static int yescrypt_kdf(const yescrypt_shared_t * __shared,
    yescrypt_local_t * __local,
    const uint8_t * __passwd, size_t __passwdlen,
    const uint8_t * __salt, size_t __saltlen,
    uint64_t __N, uint32_t __r, uint32_t __p, uint32_t __t,
    yescrypt_flags_t __flags,
    uint8_t * __buf, size_t __buflen);

#endif /* !_YESCRYPT_H_ */
