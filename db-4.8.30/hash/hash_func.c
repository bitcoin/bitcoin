/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993
 *	Margo Seltzer.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Margo Seltzer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/hash.h"

/*
 * __ham_func2 --
 *	Phong Vo's linear congruential hash.
 *
 * PUBLIC: u_int32_t __ham_func2 __P((DB *, const void *, u_int32_t));
 */
#define	DCHARHASH(h, c)	((h) = 0x63c63cd9*(h) + 0x9c39c33d + (c))

u_int32_t
__ham_func2(dbp, key, len)
	DB *dbp;
	const void *key;
	u_int32_t len;
{
	const u_int8_t *e, *k;
	u_int32_t h;
	u_int8_t c;

	if (dbp != NULL)
		COMPQUIET(dbp, NULL);

	k = key;
	e = k + len;
	for (h = 0; k != e;) {
		c = *k++;
		if (!c && k > e)
			break;
		DCHARHASH(h, c);
	}
	return (h);
}

/*
 * __ham_func3 --
 *	Ozan Yigit's original sdbm hash.
 *
 * Ugly, but fast.  Break the string up into 8 byte units.  On the first time
 * through the loop get the "leftover bytes" (strlen % 8).  On every other
 * iteration, perform 8 HASHC's so we handle all 8 bytes.  Essentially, this
 * saves us 7 cmp & branch instructions.
 *
 * PUBLIC: u_int32_t __ham_func3 __P((DB *, const void *, u_int32_t));
 */
u_int32_t
__ham_func3(dbp, key, len)
	DB *dbp;
	const void *key;
	u_int32_t len;
{
	const u_int8_t *k;
	u_int32_t n, loop;

	if (dbp != NULL)
		COMPQUIET(dbp, NULL);

	if (len == 0)
		return (0);

#define	HASHC	n = *k++ + 65599 * n
	n = 0;
	k = key;

	loop = (len + 8 - 1) >> 3;
	switch (len & (8 - 1)) {
	case 0:
		do {
			HASHC;
	case 7:
			HASHC;
	case 6:
			HASHC;
	case 5:
			HASHC;
	case 4:
			HASHC;
	case 3:
			HASHC;
	case 2:
			HASHC;
	case 1:
			HASHC;
		} while (--loop);
	}
	return (n);
}

/*
 * __ham_func4 --
 *	Chris Torek's hash function.  Although this function performs only
 *	slightly worse than __ham_func5 on strings, it performs horribly on
 *	numbers.
 *
 * PUBLIC: u_int32_t __ham_func4 __P((DB *, const void *, u_int32_t));
 */
u_int32_t
__ham_func4(dbp, key, len)
	DB *dbp;
	const void *key;
	u_int32_t len;
{
	const u_int8_t *k;
	u_int32_t h, loop;

	if (dbp != NULL)
		COMPQUIET(dbp, NULL);

	if (len == 0)
		return (0);

#define	HASH4a	h = (h << 5) - h + *k++;
#define	HASH4b	h = (h << 5) + h + *k++;
#define	HASH4	HASH4b
	h = 0;
	k = key;

	loop = (len + 8 - 1) >> 3;
	switch (len & (8 - 1)) {
	case 0:
		do {
			HASH4;
	case 7:
			HASH4;
	case 6:
			HASH4;
	case 5:
			HASH4;
	case 4:
			HASH4;
	case 3:
			HASH4;
	case 2:
			HASH4;
	case 1:
			HASH4;
		} while (--loop);
	}
	return (h);
}

/*
 * Fowler/Noll/Vo hash
 *
 * The basis of the hash algorithm was taken from an idea sent by email to the
 * IEEE Posix P1003.2 mailing list from Phong Vo (kpv@research.att.com) and
 * Glenn Fowler (gsf@research.att.com).  Landon Curt Noll (chongo@toad.com)
 * later improved on their algorithm.
 *
 * The magic is in the interesting relationship between the special prime
 * 16777619 (2^24 + 403) and 2^32 and 2^8.
 *
 * This hash produces the fewest collisions of any function that we've seen so
 * far, and works well on both numbers and strings.
 *
 * PUBLIC: u_int32_t __ham_func5 __P((DB *, const void *, u_int32_t));
 */
u_int32_t
__ham_func5(dbp, key, len)
	DB *dbp;
	const void *key;
	u_int32_t len;
{
	const u_int8_t *k, *e;
	u_int32_t h;

	if (dbp != NULL)
		COMPQUIET(dbp, NULL);

	k = key;
	e = k + len;
	for (h = 0; k < e; ++k) {
		h *= 16777619;
		h ^= *k;
	}
	return (h);
}

/*
 * __ham_test --
 *
 * PUBLIC: u_int32_t __ham_test __P((DB *, const void *, u_int32_t));
 */
u_int32_t
__ham_test(dbp, key, len)
	DB *dbp;
	const void *key;
	u_int32_t len;
{
	COMPQUIET(dbp, NULL);
	COMPQUIET(len, 0);
	return ((u_int32_t)*(char *)key);
}
