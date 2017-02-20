/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Olson.
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
#include "dbinc/db_am.h"
#include "dbinc/mp.h"

/*
 * Big key/data code.
 *
 * Big key and data entries are stored on linked lists of pages.  The initial
 * reference is a structure with the total length of the item and the page
 * number where it begins.  Each entry in the linked list contains a pointer
 * to the next page of data, and so on.
 */

/*
 * __db_goff --
 *	Get an offpage item.
 *
 * PUBLIC: int __db_goff __P((DBC *,
 * PUBLIC:     DBT *, u_int32_t, db_pgno_t, void **, u_int32_t *));
 */
int
__db_goff(dbc, dbt, tlen, pgno, bpp, bpsz)
	DBC *dbc;
	DBT *dbt;
	u_int32_t tlen;
	db_pgno_t pgno;
	void **bpp;
	u_int32_t *bpsz;
{
	DB *dbp;
	DB_MPOOLFILE *mpf;
	DB_TXN *txn;
	DBC_INTERNAL *cp;
	ENV *env;
	PAGE *h;
	DB_THREAD_INFO *ip;
	db_indx_t bytes;
	u_int32_t curoff, needed, start;
	u_int8_t *p, *src;
	int ret;

	dbp = dbc->dbp;
	cp = dbc->internal;
	env = dbp->env;
	ip = dbc->thread_info;
	mpf = dbp->mpf;
	txn = dbc->txn;

	/*
	 * Check if the buffer is big enough; if it is not and we are
	 * allowed to malloc space, then we'll malloc it.  If we are
	 * not (DB_DBT_USERMEM), then we'll set the dbt and return
	 * appropriately.
	 */
	if (F_ISSET(dbt, DB_DBT_PARTIAL)) {
		start = dbt->doff;
		if (start > tlen)
			needed = 0;
		else if (dbt->dlen > tlen - start)
			needed = tlen - start;
		else
			needed = dbt->dlen;
	} else {
		start = 0;
		needed = tlen;
	}

	/*
	 * If the caller has not requested any data, return success. This
	 * "early-out" also avoids setting up the streaming optimization when
	 * no page would be retrieved. If it were removed, the streaming code
	 * should only initialize when needed is not 0.
	 */
	if (needed == 0) {
		dbt->size = 0;
		return (0);
	}

	if (F_ISSET(dbt, DB_DBT_USERCOPY))
		goto skip_alloc;

	/* Allocate any necessary memory. */
	if (F_ISSET(dbt, DB_DBT_USERMEM)) {
		if (needed > dbt->ulen) {
			dbt->size = needed;
			return (DB_BUFFER_SMALL);
		}
	} else if (F_ISSET(dbt, DB_DBT_MALLOC)) {
		if ((ret = __os_umalloc(env, needed, &dbt->data)) != 0)
			return (ret);
	} else if (F_ISSET(dbt, DB_DBT_REALLOC)) {
		if ((ret = __os_urealloc(env, needed, &dbt->data)) != 0)
			return (ret);
	} else if (bpsz != NULL && (*bpsz == 0 || *bpsz < needed)) {
		if ((ret = __os_realloc(env, needed, bpp)) != 0)
			return (ret);
		*bpsz = needed;
		dbt->data = *bpp;
	} else if (bpp != NULL)
		dbt->data = *bpp;
	else {
		DB_ASSERT(env,
		    F_ISSET(dbt,
		    DB_DBT_USERMEM | DB_DBT_MALLOC | DB_DBT_REALLOC) ||
		    bpsz != NULL || bpp != NULL);
		return (DB_BUFFER_SMALL);
	}

skip_alloc:
	/* Set up a start page in the overflow chain if streaming. */
	if (cp->stream_start_pgno != PGNO_INVALID &&
	    pgno == cp->stream_start_pgno && start >= cp->stream_off &&
	    start < cp->stream_off + P_MAXSPACE(dbp, dbp->pgsize)) {
		pgno = cp->stream_curr_pgno;
		curoff = cp->stream_off;
	} else {
		cp->stream_start_pgno = cp->stream_curr_pgno = pgno;
		cp->stream_off = curoff = 0;
	}

	/*
	 * Step through the linked list of pages, copying the data on each
	 * one into the buffer.  Never copy more than the total data length.
	 */
	dbt->size = needed;
	for (p = dbt->data; pgno != PGNO_INVALID && needed > 0;) {
		if ((ret = __memp_fget(mpf,
		    &pgno, ip, txn, 0, &h)) != 0)
			return (ret);
		DB_ASSERT(env, TYPE(h) == P_OVERFLOW);

		/* Check if we need any bytes from this page. */
		if (curoff + OV_LEN(h) >= start) {
			bytes = OV_LEN(h);
			src = (u_int8_t *)h + P_OVERHEAD(dbp);
			if (start > curoff) {
				src += start - curoff;
				bytes -= start - curoff;
			}
			if (bytes > needed)
				bytes = needed;
			if (F_ISSET(dbt, DB_DBT_USERCOPY)) {
				/*
				 * The offset into the DBT is the total size
				 * less the amount of data still needed.  Care
				 * needs to be taken if doing a partial copy
				 * beginning at an offset other than 0.
				 */
				if ((ret = env->dbt_usercopy(
				    dbt, dbt->size - needed,
				    src, bytes, DB_USERCOPY_SETDATA)) != 0) {
					(void)__memp_fput(mpf,
					    ip, h, dbp->priority);
					return (ret);
				}
			} else
				memcpy(p, src, bytes);
			p += bytes;
			needed -= bytes;
		}
		cp->stream_off = curoff;
		curoff += OV_LEN(h);
		cp->stream_curr_pgno = pgno;
		pgno = h->next_pgno;
		(void)__memp_fput(mpf, ip, h, dbp->priority);
	}

	return (0);
}

/*
 * __db_poff --
 *	Put an offpage item.
 *
 * PUBLIC: int __db_poff __P((DBC *, const DBT *, db_pgno_t *));
 */
int
__db_poff(dbc, dbt, pgnop)
	DBC *dbc;
	const DBT *dbt;
	db_pgno_t *pgnop;
{
	DB *dbp;
	DBT tmp_dbt;
	DB_LSN null_lsn;
	DB_MPOOLFILE *mpf;
	PAGE *pagep, *lastp;
	db_indx_t pagespace;
	db_pgno_t pgno;
	u_int32_t space, sz, tlen;
	u_int8_t *p;
	int ret, t_ret;

	/*
	 * Allocate pages and copy the key/data item into them.  Calculate the
	 * number of bytes we get for pages we fill completely with a single
	 * item.
	 */
	dbp = dbc->dbp;
	lastp = NULL;
	mpf = dbp->mpf;
	pagespace = P_MAXSPACE(dbp, dbp->pgsize);
	p = dbt->data;
	sz = dbt->size;

	/*
	 * Check whether we are streaming at the end of the overflow item.
	 * If so, the last pgno and offset will be cached in the cursor.
	 */
	if (F_ISSET(dbt, DB_DBT_STREAMING)) {
		tlen = dbt->size - dbt->dlen;
		pgno = dbc->internal->stream_curr_pgno;
		if ((ret = __memp_fget(mpf, &pgno, dbc->thread_info,
		    dbc->txn, DB_MPOOL_DIRTY, &lastp)) != 0)
			return (ret);

		/*
		 * Calculate how much we can write on the last page of the
		 * overflow item.
		 */
		DB_ASSERT(dbp->env,
		    OV_LEN(lastp) == (tlen - dbc->internal->stream_off));
		space = pagespace - OV_LEN(lastp);

		/* Only copy as much data as we have. */
		if (space > dbt->dlen)
			space = dbt->dlen;

		if (DBC_LOGGING(dbc)) {
			tmp_dbt.data = dbt->data;
			tmp_dbt.size = space;
			ZERO_LSN(null_lsn);
			if ((ret = __db_big_log(dbp, dbc->txn,
			    &LSN(lastp), 0, DB_APPEND_BIG, pgno,
			    PGNO_INVALID, PGNO_INVALID, &tmp_dbt,
			    &LSN(lastp), &null_lsn, &null_lsn)) != 0)
				goto err;
		} else
			LSN_NOT_LOGGED(LSN(lastp));

		memcpy((u_int8_t *)lastp + P_OVERHEAD(dbp) + OV_LEN(lastp),
		    dbt->data, space);
		OV_LEN(lastp) += space;
		sz -= space + dbt->doff;
		p += space;
		*pgnop = dbc->internal->stream_start_pgno;
	}

	ret = 0;
	for (; sz > 0; p += pagespace, sz -= pagespace) {
		/*
		 * Reduce pagespace so we terminate the loop correctly and
		 * don't copy too much data.
		 */
		if (sz < pagespace)
			pagespace = sz;

		/*
		 * Allocate and initialize a new page and copy all or part of
		 * the item onto the page.  If sz is less than pagespace, we
		 * have a partial record.
		 */
		if ((ret = __db_new(dbc, P_OVERFLOW, NULL, &pagep)) != 0)
			break;
		if (DBC_LOGGING(dbc)) {
			tmp_dbt.data = p;
			tmp_dbt.size = pagespace;
			ZERO_LSN(null_lsn);
			if ((ret = __db_big_log(dbp, dbc->txn,
			    &LSN(pagep), 0, DB_ADD_BIG, PGNO(pagep),
			    lastp ? PGNO(lastp) : PGNO_INVALID,
			    PGNO_INVALID, &tmp_dbt, &LSN(pagep),
			    lastp == NULL ? &null_lsn : &LSN(lastp),
			    &null_lsn)) != 0) {
				(void)__memp_fput(mpf, dbc->thread_info,
				    pagep, dbc->priority);
				goto err;
			}
		} else
			LSN_NOT_LOGGED(LSN(pagep));

		/* Move LSN onto page. */
		if (lastp != NULL)
			LSN(lastp) = LSN(pagep);

		OV_LEN(pagep) = pagespace;
		OV_REF(pagep) = 1;
		memcpy((u_int8_t *)pagep + P_OVERHEAD(dbp), p, pagespace);

		/*
		 * If this is the first entry, update the user's info and
		 * initialize the cursor to allow for streaming of subsequent
		 * updates.  Otherwise, update the entry on the last page
		 * filled in and release that page.
		 */
		if (lastp == NULL) {
			*pgnop = PGNO(pagep);
			dbc->internal->stream_start_pgno =
			    dbc->internal->stream_curr_pgno = *pgnop;
			dbc->internal->stream_off = 0;
		} else {
			lastp->next_pgno = PGNO(pagep);
			pagep->prev_pgno = PGNO(lastp);
			if ((ret = __memp_fput(mpf,
			    dbc->thread_info, lastp, dbc->priority)) != 0) {
				lastp = NULL;
				goto err;
			}
		}
		lastp = pagep;
	}
err:	if (lastp != NULL) {
		if (ret == 0) {
			dbc->internal->stream_curr_pgno = PGNO(lastp);
			dbc->internal->stream_off = dbt->size - OV_LEN(lastp);
		}

		if ((t_ret = __memp_fput(mpf, dbc->thread_info, lastp,
		    dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
	}
	return (ret);
}

/*
 * __db_ovref --
 *	Decrement the reference count on an overflow page.
 *
 * PUBLIC: int __db_ovref __P((DBC *, db_pgno_t));
 */
int
__db_ovref(dbc, pgno)
	DBC *dbc;
	db_pgno_t pgno;
{
	DB *dbp;
	DB_MPOOLFILE *mpf;
	PAGE *h;
	int ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;

	if ((ret = __memp_fget(mpf, &pgno,
	     dbc->thread_info, dbc->txn, DB_MPOOL_DIRTY, &h)) != 0)
		return (ret);

	if (DBC_LOGGING(dbc)) {
		if ((ret = __db_ovref_log(dbp,
		    dbc->txn, &LSN(h), 0, h->pgno, -1, &LSN(h))) != 0) {
			(void)__memp_fput(mpf,
			     dbc->thread_info, h, dbc->priority);
			return (ret);
		}
	} else
		LSN_NOT_LOGGED(LSN(h));

	/*
	 * In BDB releases before 4.5, the overflow reference counts were
	 * incremented when an overflow item was split onto an internal
	 * page.  There was a lock race in that code, and rather than fix
	 * the race, we changed BDB to copy overflow items when splitting
	 * them onto internal pages.  The code to decrement reference
	 * counts remains so databases already in the field continue to
	 * work.
	 */
	--OV_REF(h);

	return (__memp_fput(mpf, dbc->thread_info, h, dbc->priority));
}

/*
 * __db_doff --
 *	Delete an offpage chain of overflow pages.
 *
 * PUBLIC: int __db_doff __P((DBC *, db_pgno_t));
 */
int
__db_doff(dbc, pgno)
	DBC *dbc;
	db_pgno_t pgno;
{
	DB *dbp;
	DBT tmp_dbt;
	DB_LSN null_lsn;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;

	do {
		if ((ret = __memp_fget(mpf, &pgno,
		     dbc->thread_info, dbc->txn, 0, &pagep)) != 0)
			return (ret);

		DB_ASSERT(dbp->env, TYPE(pagep) == P_OVERFLOW);
		/*
		 * If it's referenced by more than one key/data item,
		 * decrement the reference count and return.
		 */
		if (OV_REF(pagep) > 1) {
			(void)__memp_fput(mpf,
			    dbc->thread_info, pagep, dbc->priority);
			return (__db_ovref(dbc, pgno));
		}

		if ((ret = __memp_dirty(mpf, &pagep,
		    dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0) {
			if (pagep != NULL)
				(void)__memp_fput(mpf,
				    dbc->thread_info, pagep, dbc->priority);
			return (ret);
		}

		if (DBC_LOGGING(dbc)) {
			tmp_dbt.data = (u_int8_t *)pagep + P_OVERHEAD(dbp);
			tmp_dbt.size = OV_LEN(pagep);
			ZERO_LSN(null_lsn);
			if ((ret = __db_big_log(dbp, dbc->txn,
			    &LSN(pagep), 0, DB_REM_BIG,
			    PGNO(pagep), PREV_PGNO(pagep),
			    NEXT_PGNO(pagep), &tmp_dbt,
			    &LSN(pagep), &null_lsn, &null_lsn)) != 0) {
				(void)__memp_fput(mpf,
				    dbc->thread_info, pagep, dbc->priority);
				return (ret);
			}
		} else
			LSN_NOT_LOGGED(LSN(pagep));
		pgno = pagep->next_pgno;
		OV_LEN(pagep) = 0;
		if ((ret = __db_free(dbc, pagep)) != 0)
			return (ret);
	} while (pgno != PGNO_INVALID);

	return (0);
}

/*
 * __db_moff --
 *	Match on overflow pages.
 *
 * Given a starting page number and a key, return <0, 0, >0 to indicate if the
 * key on the page is less than, equal to or greater than the key specified.
 * We optimize this by doing chunk at a time comparison unless the user has
 * specified a comparison function.  In this case, we need to materialize
 * the entire object and call their comparison routine.
 *
 * __db_moff and __db_coff are generic functions useful in searching and
 * ordering off page items. __db_moff matches an overflow DBT with an offpage
 * item. __db_coff compares two offpage items for lexicographic sort order.
 *
 * PUBLIC: int __db_moff __P((DBC *, const DBT *, db_pgno_t, u_int32_t,
 * PUBLIC:     int (*)(DB *, const DBT *, const DBT *), int *));
 */
int
__db_moff(dbc, dbt, pgno, tlen, cmpfunc, cmpp)
	DBC *dbc;
	const DBT *dbt;
	db_pgno_t pgno;
	u_int32_t tlen;
	int (*cmpfunc) __P((DB *, const DBT *, const DBT *)), *cmpp;
{
	DB *dbp;
	DBT local_dbt;
	DB_MPOOLFILE *mpf;
	DB_THREAD_INFO *ip;
	PAGE *pagep;
	void *buf;
	u_int32_t bufsize, cmp_bytes, key_left;
	u_int8_t *p1, *p2;
	int ret;

	dbp = dbc->dbp;
	ip = dbc->thread_info;
	mpf = dbp->mpf;

	/*
	 * If there is a user-specified comparison function, build a
	 * contiguous copy of the key, and call it.
	 */
	if (cmpfunc != NULL) {
		memset(&local_dbt, 0, sizeof(local_dbt));
		buf = NULL;
		bufsize = 0;

		if ((ret = __db_goff(dbc,
		    &local_dbt, tlen, pgno, &buf, &bufsize)) != 0)
			return (ret);
		/* Pass the key as the first argument */
		*cmpp = cmpfunc(dbp, dbt, &local_dbt);
		__os_free(dbp->env, buf);
		return (0);
	}

	/* While there are both keys to compare. */
	for (*cmpp = 0, p1 = dbt->data,
	    key_left = dbt->size; key_left > 0 && pgno != PGNO_INVALID;) {
		if ((ret =
		    __memp_fget(mpf, &pgno, ip, dbc->txn, 0, &pagep)) != 0)
			return (ret);

		cmp_bytes = OV_LEN(pagep) < key_left ? OV_LEN(pagep) : key_left;
		tlen -= cmp_bytes;
		key_left -= cmp_bytes;
		for (p2 = (u_int8_t *)pagep + P_OVERHEAD(dbp);
		    cmp_bytes-- > 0; ++p1, ++p2)
			if (*p1 != *p2) {
				*cmpp = (long)*p1 - (long)*p2;
				break;
			}
		pgno = NEXT_PGNO(pagep);
		if ((ret = __memp_fput(mpf, ip, pagep, dbp->priority)) != 0)
			return (ret);
		if (*cmpp != 0)
			return (0);
	}
	if (key_left > 0)		/* DBT is longer than the page key. */
		*cmpp = 1;
	else if (tlen > 0)		/* DBT is shorter than the page key. */
		*cmpp = -1;
	else
		*cmpp = 0;

	return (0);
}

/*
 * __db_coff --
 *	Match two offpage dbts.
 *
 * The DBTs must both refer to offpage items.
 * The match happens a chunk (page) at a time unless a user defined comparison
 * function exists. It is not possible to optimize this comparison away when
 * a lexicographic sort order is required on mismatch.
 *
 * NOTE: For now this function only works for H_OFFPAGE type items. It would
 * be simple to extend it for use with B_OVERFLOW type items. It would only
 * require extracting the total length, and page number, dependent on the
 * DBT type.
 *
 * PUBLIC: int __db_coff __P((DBC *, const DBT *, const DBT *,
 * PUBLIC:     int (*)(DB *, const DBT *, const DBT *), int *));
 */
int
__db_coff(dbc, dbt, match, cmpfunc, cmpp)
	DBC *dbc;
	const DBT *dbt, *match;
	int (*cmpfunc) __P((DB *, const DBT *, const DBT *)), *cmpp;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_MPOOLFILE *mpf;
	DB_TXN *txn;
	DBT local_key, local_match;
	PAGE *dbt_pagep, *match_pagep;
	db_pgno_t dbt_pgno, match_pgno;
	u_int32_t cmp_bytes, dbt_bufsz, dbt_len, match_bufsz;
	u_int32_t match_len, max_data, page_sz;
	u_int8_t *p1, *p2;
	int ret;
	void *dbt_buf, *match_buf;

	dbp = dbc->dbp;
	ip = dbc->thread_info;
	txn = dbc->txn;
	mpf = dbp->mpf;
	page_sz = dbp->pgsize;
	*cmpp = 0;
	dbt_buf = match_buf = NULL;

	DB_ASSERT(dbp->env, HPAGE_PTYPE(dbt->data) == H_OFFPAGE);
	DB_ASSERT(dbp->env, HPAGE_PTYPE(match->data) == H_OFFPAGE);

	/* Extract potentially unaligned length and pgno fields from DBTs */
	memcpy(&dbt_len, HOFFPAGE_TLEN(dbt->data), sizeof(u_int32_t));
	memcpy(&dbt_pgno, HOFFPAGE_PGNO(dbt->data), sizeof(db_pgno_t));
	memcpy(&match_len, HOFFPAGE_TLEN(match->data), sizeof(u_int32_t));
	memcpy(&match_pgno, HOFFPAGE_PGNO(match->data), sizeof(db_pgno_t));
	max_data = (dbt_len < match_len ? dbt_len : match_len);

	/*
	 * If there is a custom comparator, fully resolve both DBTs.
	 * Then call the users comparator.
	 */
	if (cmpfunc != NULL) {
		memset(&local_key, 0, sizeof(local_key));
		memset(&local_match, 0, sizeof(local_match));
		dbt_buf = match_buf = NULL;
		dbt_bufsz = match_bufsz = 0;

		if ((ret = __db_goff(dbc, &local_key, dbt_len,
		    dbt_pgno, &dbt_buf, &dbt_bufsz)) != 0)
			goto err1;
		if ((ret = __db_goff(dbc, &local_match, match_len,
		    match_pgno, &match_buf, &match_bufsz)) != 0)
			goto err1;
		/* The key needs to be the first argument for sort order */
		*cmpp = cmpfunc(dbp, &local_key, &local_match);

err1:		if (dbt_buf != NULL)
			__os_free(dbp->env, dbt_buf);
		if (match_buf != NULL)
			__os_free(dbp->env, match_buf);
		return (ret);
	}

	/* Match the offpage DBTs a page at a time. */
	while (dbt_pgno != PGNO_INVALID && match_pgno != PGNO_INVALID) {
		if ((ret =
		    __memp_fget(mpf, &dbt_pgno, ip, txn, 0, &dbt_pagep)) != 0)
			return (ret);
		if ((ret =
		    __memp_fget(mpf, &match_pgno,
			ip, txn, 0, &match_pagep)) != 0) {
			(void)__memp_fput(
			    mpf, ip, dbt_pagep, DB_PRIORITY_UNCHANGED);
			return (ret);
		}
		cmp_bytes = page_sz < max_data ? page_sz : max_data;
		for (p1 = (u_int8_t *)dbt_pagep + P_OVERHEAD(dbp),
		    p2 = (u_int8_t *)match_pagep + P_OVERHEAD(dbp);
		    cmp_bytes-- > 0; ++p1, ++p2)
				if (*p1 != *p2) {
					*cmpp = (long)*p1 - (long)*p2;
					break;
				}

		dbt_pgno = NEXT_PGNO(dbt_pagep);
		match_pgno = NEXT_PGNO(match_pagep);
		max_data -= page_sz;
		if ((ret = __memp_fput(mpf,
		     ip, dbt_pagep, DB_PRIORITY_UNCHANGED)) != 0) {
			(void)__memp_fput(mpf,
			    ip, match_pagep, DB_PRIORITY_UNCHANGED);
			return (ret);
		}
		if ((ret = __memp_fput(mpf,
		    ip, match_pagep, DB_PRIORITY_UNCHANGED)) != 0)
			return (ret);
		if (*cmpp != 0)
			return (0);
	}

	/* If a lexicographic mismatch was found, then the result has already
	 * been returned. If the DBTs matched, consider the lengths of the
	 * items, and return appropriately.
	 */
	if (dbt_len > match_len) /* DBT is longer than the match key. */
		*cmpp = 1;
	else if (match_len > dbt_len) /* DBT is shorter than the match key. */
		*cmpp = -1;
	else
		*cmpp = 0;

	return (0);

}
