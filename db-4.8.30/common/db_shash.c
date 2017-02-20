/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __db_tablesize --
 *	Choose a size for the hash table.
 *
 * PUBLIC: u_int32_t __db_tablesize __P((u_int32_t));
 */
u_int32_t
__db_tablesize(n_buckets)
	u_int32_t n_buckets;
{
	/*
	 * We try to be clever about how big we make the hash tables.  Use a
	 * prime number close to the "suggested" number of elements that will
	 * be in the hash table.  Use 32 as the minimum hash table size.
	 *
	 * Ref: Sedgewick, Algorithms in C, "Hash Functions"
	 *
	 * Up to ~250,000 buckets, we use powers of 2.  After that, we slow
	 * the rate of increase by half.  For each choice, we then use a
	 * nearby prime number as the hash value.
	 *
	 * If a terabyte is the maximum cache we'll see, and we assume there
	 * are 10 1K buckets on each hash chain, then 107374182 is the maximum
	 * number of buckets we'll ever need.
	 *
	 * We don't use the obvious static data structure because some C
	 * compilers (and I use the term loosely), can't handle them.
	 */
#define	HASH_SIZE(power, prime) {					\
	if ((power) >= n_buckets)					\
		return (prime);						\
}
	HASH_SIZE(32, 37);			/* 2^5 */
	HASH_SIZE(64, 67);			/* 2^6 */
	HASH_SIZE(128, 131);			/* 2^7 */
	HASH_SIZE(256, 257);			/* 2^8 */
	HASH_SIZE(512, 521);			/* 2^9 */
	HASH_SIZE(1024, 1031);			/* 2^10 */
	HASH_SIZE(2048, 2053);			/* 2^11 */
	HASH_SIZE(4096, 4099);			/* 2^12 */
	HASH_SIZE(8192, 8191);			/* 2^13 */
	HASH_SIZE(16384, 16381);		/* 2^14 */
	HASH_SIZE(32768, 32771);		/* 2^15 */
	HASH_SIZE(65536, 65537);		/* 2^16 */
	HASH_SIZE(131072, 131071);		/* 2^17 */
	HASH_SIZE(262144, 262147);		/* 2^18 */
	HASH_SIZE(393216, 393209);		/* 2^18 + 2^18/2 */
	HASH_SIZE(524288, 524287);		/* 2^19 */
	HASH_SIZE(786432, 786431);		/* 2^19 + 2^19/2 */
	HASH_SIZE(1048576, 1048573);		/* 2^20 */
	HASH_SIZE(1572864, 1572869);		/* 2^20 + 2^20/2 */
	HASH_SIZE(2097152, 2097169);		/* 2^21 */
	HASH_SIZE(3145728, 3145721);		/* 2^21 + 2^21/2 */
	HASH_SIZE(4194304, 4194301);		/* 2^22 */
	HASH_SIZE(6291456, 6291449);		/* 2^22 + 2^22/2 */
	HASH_SIZE(8388608, 8388617);		/* 2^23 */
	HASH_SIZE(12582912, 12582917);		/* 2^23 + 2^23/2 */
	HASH_SIZE(16777216, 16777213);		/* 2^24 */
	HASH_SIZE(25165824, 25165813);		/* 2^24 + 2^24/2 */
	HASH_SIZE(33554432, 33554393);		/* 2^25 */
	HASH_SIZE(50331648, 50331653);		/* 2^25 + 2^25/2 */
	HASH_SIZE(67108864, 67108859);		/* 2^26 */
	HASH_SIZE(100663296, 100663291);	/* 2^26 + 2^26/2 */
	HASH_SIZE(134217728, 134217757);	/* 2^27 */
	HASH_SIZE(201326592, 201326611);	/* 2^27 + 2^27/2 */
	HASH_SIZE(268435456, 268435459);	/* 2^28 */
	HASH_SIZE(402653184, 402653189);	/* 2^28 + 2^28/2 */
	HASH_SIZE(536870912, 536870909);	/* 2^29 */
	HASH_SIZE(805306368, 805306357);	/* 2^29 + 2^29/2 */
	HASH_SIZE(1073741824, 1073741827);	/* 2^30 */
	return (1073741827);
}

/*
 * __db_hashinit --
 *	Initialize a hash table that resides in shared memory.
 *
 * PUBLIC: void __db_hashinit __P((void *, u_int32_t));
 */
void
__db_hashinit(begin, nelements)
	void *begin;
	u_int32_t nelements;
{
	u_int32_t i;
	SH_TAILQ_HEAD(hash_head) *headp;

	headp = (struct hash_head *)begin;

	for (i = 0; i < nelements; i++, headp++)
		SH_TAILQ_INIT(headp);
}
