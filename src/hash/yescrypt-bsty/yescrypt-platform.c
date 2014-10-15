/*-
 * Copyright 2013,2014 Alexander Peslyak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
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
 */

#include <sys/mman.h>
#include "yescrypt.h"
#define HUGEPAGE_THRESHOLD		(12 * 1024 * 1024)

#ifdef __x86_64__
#define HUGEPAGE_SIZE			(2 * 1024 * 1024)
#else
#undef HUGEPAGE_SIZE
#endif

static void *
alloc_region(yescrypt_region_t * region, size_t size)
{
	size_t base_size = size;
	uint8_t * base, * aligned;
#ifdef MAP_ANON
	int flags =
#ifdef MAP_NOCORE
	    MAP_NOCORE |
#endif
	    MAP_ANON | MAP_PRIVATE;
#if defined(MAP_HUGETLB) && defined(HUGEPAGE_SIZE)
	size_t new_size = size;
	const size_t hugepage_mask = (size_t)HUGEPAGE_SIZE - 1;
	if (size >= HUGEPAGE_THRESHOLD && size + hugepage_mask >= size) {
		flags |= MAP_HUGETLB;
/*
 * Linux's munmap() fails on MAP_HUGETLB mappings if size is not a multiple of
 * huge page size, so let's round up to huge page size here.
 */
		new_size = size + hugepage_mask;
		new_size &= ~hugepage_mask;
	}
	base = mmap(NULL, new_size, PROT_READ | PROT_WRITE, flags, -1, 0);
	if (base != MAP_FAILED) {
		base_size = new_size;
	} else
	if (flags & MAP_HUGETLB) {
		flags &= ~MAP_HUGETLB;
		base = mmap(NULL, size, PROT_READ | PROT_WRITE, flags, -1, 0);
	}

#else
	base = mmap(NULL, size, PROT_READ | PROT_WRITE, flags, -1, 0);
#endif
	if (base == MAP_FAILED)
		base = NULL;
	aligned = base;
#elif defined(HAVE_POSIX_MEMALIGN)
	if ((errno = posix_memalign((void **)&base, 64, size)) != 0)
		base = NULL;
	aligned = base;
#else
	base = aligned = NULL;
	if (size + 63 < size) {
		errno = ENOMEM;
	} else if ((base = malloc(size + 63)) != NULL) {
		aligned = base + 63;
		aligned -= (uintptr_t)aligned & 63;
	}
#endif
	region->base = base;
	region->aligned = aligned;
	region->base_size = base ? base_size : 0;
	region->aligned_size = base ? size : 0;
	return aligned;
}

static inline void
init_region(yescrypt_region_t * region)
{
	region->base = region->aligned = NULL;
	region->base_size = region->aligned_size = 0;
}

static int
free_region(yescrypt_region_t * region)
{
	if (region->base) {
#ifdef MAP_ANON
		if (munmap(region->base, region->base_size))
			return -1;
#else
		free(region->base);
#endif
	}
	init_region(region);
	return 0;
}

int
yescrypt_init_shared(yescrypt_shared_t * shared,
    const uint8_t * param, size_t paramlen,
    uint64_t N, uint32_t r, uint32_t p,
    yescrypt_init_shared_flags_t flags, uint32_t mask,
    uint8_t * buf, size_t buflen)
{
	yescrypt_shared1_t * shared1 = &shared->shared1;
	yescrypt_shared_t dummy, half1, half2;
	uint8_t salt[32];

	if (flags & YESCRYPT_SHARED_PREALLOCATED) {
		if (!shared1->aligned || !shared1->aligned_size)
			return -1;
	} else {
		init_region(shared1);
	}
	shared->mask1 = 1;
	if (!param && !paramlen && !N && !r && !p && !buf && !buflen)
		return 0;

	init_region(&dummy.shared1);
	dummy.mask1 = 1;
	if (yescrypt_kdf(&dummy, shared1,
	    param, paramlen, NULL, 0, N, r, p, 0,
	    YESCRYPT_RW | YESCRYPT_PARALLEL_SMIX | __YESCRYPT_INIT_SHARED_1,
	    salt, sizeof(salt)))
		goto out;

	half1 = half2 = *shared;
	half1.shared1.aligned_size /= 2;
	half2.shared1.aligned += half1.shared1.aligned_size;
	half2.shared1.aligned_size = half1.shared1.aligned_size;
	N /= 2;

	if (p > 1 && yescrypt_kdf(&half1, &half2.shared1,
	    param, paramlen, salt, sizeof(salt), N, r, p, 0,
	    YESCRYPT_RW | YESCRYPT_PARALLEL_SMIX | __YESCRYPT_INIT_SHARED_2,
	    salt, sizeof(salt)))
		goto out;

	if (yescrypt_kdf(&half2, &half1.shared1,
	    param, paramlen, salt, sizeof(salt), N, r, p, 0,
	    YESCRYPT_RW | YESCRYPT_PARALLEL_SMIX | __YESCRYPT_INIT_SHARED_1,
	    salt, sizeof(salt)))
		goto out;

	if (yescrypt_kdf(&half1, &half2.shared1,
	    param, paramlen, salt, sizeof(salt), N, r, p, 0,
	    YESCRYPT_RW | YESCRYPT_PARALLEL_SMIX | __YESCRYPT_INIT_SHARED_1,
	    buf, buflen))
		goto out;

	shared->mask1 = mask;

	return 0;

out:
	if (!(flags & YESCRYPT_SHARED_PREALLOCATED))
		free_region(shared1);
	return -1;
}

int
yescrypt_free_shared(yescrypt_shared_t * shared)
{
	return free_region(&shared->shared1);
}

int
yescrypt_init_local(yescrypt_local_t * local)
{
	init_region(local);
	return 0;
}

int
yescrypt_free_local(yescrypt_local_t * local)
{
	return free_region(local);
}
