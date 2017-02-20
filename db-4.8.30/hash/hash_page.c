/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994
 *	Margo Seltzer.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994
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

/*
 * PACKAGE:  hashing
 *
 * DESCRIPTION:
 *	Page manipulation for hashing package.
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"

static int __hamc_delpg
    __P((DBC *, db_pgno_t, db_pgno_t, u_int32_t, db_ham_mode, u_int32_t *));
static int __ham_getindex_sorted
    __P((DBC *, PAGE *, const DBT *, int, int *, db_indx_t *));
static int __ham_getindex_unsorted
    __P((DBC *, PAGE *, const DBT *, int *, db_indx_t *));
static int __ham_sort_page_cursor __P((DBC *, PAGE *));

/*
 * PUBLIC: int __ham_item __P((DBC *, db_lockmode_t, db_pgno_t *));
 */
int
__ham_item(dbc, mode, pgnop)
	DBC *dbc;
	db_lockmode_t mode;
	db_pgno_t *pgnop;
{
	DB *dbp;
	HASH_CURSOR *hcp;
	db_pgno_t next_pgno;
	int ret;

	dbp = dbc->dbp;
	hcp = (HASH_CURSOR *)dbc->internal;

	if (F_ISSET(hcp, H_DELETED)) {
		__db_errx(dbp->env, "Attempt to return a deleted item");
		return (EINVAL);
	}
	F_CLR(hcp, H_OK | H_NOMORE);

	/* Check if we need to get a page for this cursor. */
	if ((ret = __ham_get_cpage(dbc, mode)) != 0)
		return (ret);

recheck:
	/* Check if we are looking for space in which to insert an item. */
	if (hcp->seek_size != 0 && hcp->seek_found_page == PGNO_INVALID &&
	    hcp->seek_size < P_FREESPACE(dbp, hcp->page)) {
		hcp->seek_found_page = hcp->pgno;
		hcp->seek_found_indx = NDX_INVALID;
	}

	/* Check for off-page duplicates. */
	if (hcp->indx < NUM_ENT(hcp->page) &&
	    HPAGE_TYPE(dbp, hcp->page, H_DATAINDEX(hcp->indx)) == H_OFFDUP) {
		memcpy(pgnop,
		    HOFFDUP_PGNO(H_PAIRDATA(dbp, hcp->page, hcp->indx)),
		    sizeof(db_pgno_t));
		F_SET(hcp, H_OK);
		return (0);
	}

	/* Check if we need to go on to the next page. */
	if (F_ISSET(hcp, H_ISDUP))
		/*
		 * ISDUP is set, and offset is at the beginning of the datum.
		 * We need to grab the length of the datum, then set the datum
		 * pointer to be the beginning of the datum.
		 */
		memcpy(&hcp->dup_len,
		    HKEYDATA_DATA(H_PAIRDATA(dbp, hcp->page, hcp->indx)) +
		    hcp->dup_off, sizeof(db_indx_t));

	if (hcp->indx >= (db_indx_t)NUM_ENT(hcp->page)) {
		/* Fetch next page. */
		if (NEXT_PGNO(hcp->page) == PGNO_INVALID) {
			F_SET(hcp, H_NOMORE);
			return (DB_NOTFOUND);
		}
		next_pgno = NEXT_PGNO(hcp->page);
		hcp->indx = 0;
		if ((ret = __ham_next_cpage(dbc, next_pgno)) != 0)
			return (ret);
		goto recheck;
	}

	F_SET(hcp, H_OK);
	return (0);
}

/*
 * PUBLIC: int __ham_item_reset __P((DBC *));
 */
int
__ham_item_reset(dbc)
	DBC *dbc;
{
	DB *dbp;
	DB_MPOOLFILE *mpf;
	HASH_CURSOR *hcp;
	int ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	hcp = (HASH_CURSOR *)dbc->internal;

	ret = 0;
	if (hcp->page != NULL)
		ret = __memp_fput(mpf,
		    dbc->thread_info, hcp->page, dbc->priority);

	if ((t_ret = __ham_item_init(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * PUBLIC: int __ham_item_init __P((DBC *));
 */
int
__ham_item_init(dbc)
	DBC *dbc;
{
	HASH_CURSOR *hcp;
	int ret;

	hcp = (HASH_CURSOR *)dbc->internal;

	/*
	 * If this cursor still holds any locks, we must release them if
	 * we are not running with transactions.
	 */
	ret = __TLPUT(dbc, hcp->lock);

	/*
	 * The following fields must *not* be initialized here because they
	 * may have meaning across inits.
	 *	hlock, hdr, split_buf, stats
	 */
	hcp->bucket = BUCKET_INVALID;
	hcp->lbucket = BUCKET_INVALID;
	LOCK_INIT(hcp->lock);
	hcp->lock_mode = DB_LOCK_NG;
	hcp->dup_off = 0;
	hcp->dup_len = 0;
	hcp->dup_tlen = 0;
	hcp->seek_size = 0;
	hcp->seek_found_page = PGNO_INVALID;
	hcp->seek_found_indx = NDX_INVALID;
	hcp->flags = 0;

	hcp->pgno = PGNO_INVALID;
	hcp->indx = NDX_INVALID;
	hcp->page = NULL;

	return (ret);
}

/*
 * Returns the last item in a bucket.
 *
 * PUBLIC: int __ham_item_last __P((DBC *, db_lockmode_t, db_pgno_t *));
 */
int
__ham_item_last(dbc, mode, pgnop)
	DBC *dbc;
	db_lockmode_t mode;
	db_pgno_t *pgnop;
{
	HASH_CURSOR *hcp;
	int ret;

	hcp = (HASH_CURSOR *)dbc->internal;
	if ((ret = __ham_item_reset(dbc)) != 0)
		return (ret);

	hcp->bucket = hcp->hdr->max_bucket;
	hcp->pgno = BUCKET_TO_PAGE(hcp, hcp->bucket);
	F_SET(hcp, H_OK);
	return (__ham_item_prev(dbc, mode, pgnop));
}

/*
 * PUBLIC: int __ham_item_first __P((DBC *, db_lockmode_t, db_pgno_t *));
 */
int
__ham_item_first(dbc, mode, pgnop)
	DBC *dbc;
	db_lockmode_t mode;
	db_pgno_t *pgnop;
{
	HASH_CURSOR *hcp;
	int ret;

	hcp = (HASH_CURSOR *)dbc->internal;
	if ((ret = __ham_item_reset(dbc)) != 0)
		return (ret);
	F_SET(hcp, H_OK);
	hcp->bucket = 0;
	hcp->pgno = BUCKET_TO_PAGE(hcp, hcp->bucket);
	return (__ham_item_next(dbc, mode, pgnop));
}

/*
 * __ham_item_prev --
 *	Returns a pointer to key/data pair on a page.  In the case of
 *	bigkeys, just returns the page number and index of the bigkey
 *	pointer pair.
 *
 * PUBLIC: int __ham_item_prev __P((DBC *, db_lockmode_t, db_pgno_t *));
 */
int
__ham_item_prev(dbc, mode, pgnop)
	DBC *dbc;
	db_lockmode_t mode;
	db_pgno_t *pgnop;
{
	DB *dbp;
	HASH_CURSOR *hcp;
	db_pgno_t next_pgno;
	int ret;

	hcp = (HASH_CURSOR *)dbc->internal;
	dbp = dbc->dbp;

	/*
	 * There are 5 cases for backing up in a hash file.
	 * Case 1: In the middle of a page, no duplicates, just dec the index.
	 * Case 2: In the middle of a duplicate set, back up one.
	 * Case 3: At the beginning of a duplicate set, get out of set and
	 *	back up to next key.
	 * Case 4: At the beginning of a page; go to previous page.
	 * Case 5: At the beginning of a bucket; go to prev bucket.
	 */
	F_CLR(hcp, H_OK | H_NOMORE | H_DELETED);

	if ((ret = __ham_get_cpage(dbc, mode)) != 0)
		return (ret);

	/*
	 * First handle the duplicates.  Either you'll get the key here
	 * or you'll exit the duplicate set and drop into the code below
	 * to handle backing up through keys.
	 */
	if (!F_ISSET(hcp, H_NEXT_NODUP) && F_ISSET(hcp, H_ISDUP)) {
		if (HPAGE_TYPE(dbp, hcp->page, H_DATAINDEX(hcp->indx)) ==
		    H_OFFDUP) {
			memcpy(pgnop,
			    HOFFDUP_PGNO(H_PAIRDATA(dbp, hcp->page, hcp->indx)),
			    sizeof(db_pgno_t));
			F_SET(hcp, H_OK);
			return (0);
		}

		/* Duplicates are on-page. */
		if (hcp->dup_off != 0) {
			memcpy(&hcp->dup_len, HKEYDATA_DATA(
			    H_PAIRDATA(dbp, hcp->page, hcp->indx))
			    + hcp->dup_off - sizeof(db_indx_t),
			    sizeof(db_indx_t));
			hcp->dup_off -=
			    DUP_SIZE(hcp->dup_len);
			return (__ham_item(dbc, mode, pgnop));
		}
	}

	/*
	 * If we get here, we are not in a duplicate set, and just need
	 * to back up the cursor.  There are still three cases:
	 * midpage, beginning of page, beginning of bucket.
	 */

	if (F_ISSET(hcp, H_DUPONLY)) {
		F_CLR(hcp, H_OK);
		F_SET(hcp, H_NOMORE);
		return (0);
	} else
		/*
		 * We are no longer in a dup set;  flag this so the dup code
		 * will reinitialize should we stumble upon another one.
		 */
		F_CLR(hcp, H_ISDUP);

	if (hcp->indx == 0) {		/* Beginning of page. */
		hcp->pgno = PREV_PGNO(hcp->page);
		if (hcp->pgno == PGNO_INVALID) {
			/* Beginning of bucket. */
			F_SET(hcp, H_NOMORE);
			return (DB_NOTFOUND);
		} else if ((ret =
		    __ham_next_cpage(dbc, hcp->pgno)) != 0)
			return (ret);
		else
			hcp->indx = NUM_ENT(hcp->page);
	}

	/*
	 * Either we've got the cursor set up to be decremented, or we
	 * have to find the end of a bucket.
	 */
	if (hcp->indx == NDX_INVALID) {
		DB_ASSERT(dbp->env, hcp->page != NULL);

		hcp->indx = NUM_ENT(hcp->page);
		for (next_pgno = NEXT_PGNO(hcp->page);
		    next_pgno != PGNO_INVALID;
		    next_pgno = NEXT_PGNO(hcp->page)) {
			if ((ret = __ham_next_cpage(dbc, next_pgno)) != 0)
				return (ret);
			hcp->indx = NUM_ENT(hcp->page);
		}

		if (hcp->indx == 0) {
			/* Bucket was empty. */
			F_SET(hcp, H_NOMORE);
			return (DB_NOTFOUND);
		}
	}

	hcp->indx -= 2;

	return (__ham_item(dbc, mode, pgnop));
}

/*
 * Sets the cursor to the next key/data pair on a page.
 *
 * PUBLIC: int __ham_item_next __P((DBC *, db_lockmode_t, db_pgno_t *));
 */
int
__ham_item_next(dbc, mode, pgnop)
	DBC *dbc;
	db_lockmode_t mode;
	db_pgno_t *pgnop;
{
	HASH_CURSOR *hcp;
	int ret;

	hcp = (HASH_CURSOR *)dbc->internal;

	if ((ret = __ham_get_cpage(dbc, mode)) != 0)
		return (ret);

	/*
	 * Deleted on-page duplicates are a weird case. If we delete the last
	 * one, then our cursor is at the very end of a duplicate set and
	 * we actually need to go on to the next key.
	 */
	if (F_ISSET(hcp, H_DELETED)) {
		if (hcp->indx != NDX_INVALID &&
		    F_ISSET(hcp, H_ISDUP) &&
		    HPAGE_TYPE(dbc->dbp, hcp->page, H_DATAINDEX(hcp->indx))
			== H_DUPLICATE && hcp->dup_tlen == hcp->dup_off) {
			if (F_ISSET(hcp, H_DUPONLY)) {
				F_CLR(hcp, H_OK);
				F_SET(hcp, H_NOMORE);
				return (0);
			} else {
				F_CLR(hcp, H_ISDUP);
				hcp->indx += 2;
			}
		} else if (!F_ISSET(hcp, H_ISDUP) && F_ISSET(hcp, H_DUPONLY)) {
			F_CLR(hcp, H_OK);
			F_SET(hcp, H_NOMORE);
			return (0);
		} else if (F_ISSET(hcp, H_ISDUP) &&
		    F_ISSET(hcp, H_NEXT_NODUP)) {
			F_CLR(hcp, H_ISDUP);
			hcp->indx += 2;
		}
		F_CLR(hcp, H_DELETED);
	} else if (hcp->indx == NDX_INVALID) {
		hcp->indx = 0;
		F_CLR(hcp, H_ISDUP);
	} else if (F_ISSET(hcp, H_NEXT_NODUP)) {
		hcp->indx += 2;
		F_CLR(hcp, H_ISDUP);
	} else if (F_ISSET(hcp, H_ISDUP) && hcp->dup_tlen != 0) {
		if (hcp->dup_off + DUP_SIZE(hcp->dup_len) >=
		    hcp->dup_tlen && F_ISSET(hcp, H_DUPONLY)) {
			F_CLR(hcp, H_OK);
			F_SET(hcp, H_NOMORE);
			return (0);
		}
		hcp->dup_off += DUP_SIZE(hcp->dup_len);
		if (hcp->dup_off >= hcp->dup_tlen) {
			F_CLR(hcp, H_ISDUP);
			hcp->indx += 2;
		}
	} else if (F_ISSET(hcp, H_DUPONLY)) {
		F_CLR(hcp, H_OK);
		F_SET(hcp, H_NOMORE);
		return (0);
	} else {
		hcp->indx += 2;
		F_CLR(hcp, H_ISDUP);
	}

	return (__ham_item(dbc, mode, pgnop));
}

/*
 * __ham_insertpair --
 *
 * Used for adding a pair of elements to a sorted page. We are guaranteed that
 * the pair will fit on this page.
 *
 * If an index is provided, then use it, otherwise lookup the index using
 * __ham_getindex. This saves a getindex call when inserting using a cursor.
 *
 * We're overloading the meaning of the H_OFFPAGE type here, which is a little
 * bit sleazy. When we recover deletes, we have the entire entry instead of
 * having only the DBT, so we'll pass type H_OFFPAGE to mean "copy the whole
 * entry" as opposed to constructing an H_KEYDATA around it. In the recovery
 * case it is assumed that a valid index is passed in, since a lookup using
 * the overloaded H_OFFPAGE key will be incorrect.
 *
 * PUBLIC: int __ham_insertpair __P((DBC *,
 * PUBLIC:     PAGE *p, db_indx_t *indxp, const DBT *, const DBT *, int, int));
 */
int
__ham_insertpair(dbc, p, indxp, key_dbt, data_dbt, key_type, data_type)
	DBC *dbc;
	PAGE *p;
	db_indx_t *indxp;
	const DBT *key_dbt, *data_dbt;
	int key_type, data_type;
{
	DB *dbp;
	u_int16_t n, indx;
	db_indx_t *inp;
	u_int32_t ksize, dsize, increase, distance;
	u_int8_t *offset;
	int i, match, ret;

	dbp = dbc->dbp;
	n = NUM_ENT(p);
	inp = P_INP(dbp, p);
	ksize = (key_type == H_OFFPAGE) ?
	    key_dbt->size : HKEYDATA_SIZE(key_dbt->size);
	dsize = (data_type == H_OFFPAGE) ?
	    data_dbt->size : HKEYDATA_SIZE(data_dbt->size);
	increase = ksize + dsize;

	if (indxp != NULL && *indxp != NDX_INVALID)
		indx = *indxp;
	else {
		if ((ret = __ham_getindex(dbc, p, key_dbt,
		    key_type, &match, &indx)) != 0)
			return (ret);
		/* Save the index for the caller */
		if (indxp != NULL)
			*indxp = indx;
		/* It is an error to insert a duplicate key */
		DB_ASSERT(dbp->env, match != 0);
	}

	/* Special case if the page is empty or inserting at end of page.*/
	if (n == 0 || indx == n) {
		inp[indx] = HOFFSET(p) - ksize;
		inp[indx+1] = HOFFSET(p) - increase;
	} else {
		/*
		 * Shuffle the data elements.
		 *
		 * For example, inserting an element that sorts between items
		 * 2 and 3 on a page:
		 * The copy starts from the beginning of the second item.
		 *
		 * ---------------------------
		 * |pgheader..
		 * |__________________________
		 * ||1|2|3|4|...
		 * |--------------------------
		 * |
		 * |__________________________
		 * |              ...|4|3|2|1|
		 * |--------------------------
		 * ---------------------------
		 *
		 * Becomes:
		 *
		 * ---------------------------
		 * |pgheader..
		 * |__________________________
		 * ||1|2|2a|3|4|...
		 * |--------------------------
		 * |
		 * |__________________________
		 * |           ...|4|3|2a|2|1|
		 * |--------------------------
		 * ---------------------------
		 *
		 * Index's 3,4 etc move down the page.
		 * The data for 3,4,etc moves up the page by sizeof(2a)
		 * The index pointers in 3,4 etc are updated to point at the
		 * relocated data.
		 * It is necessary to move the data (not just adjust the index)
		 * since the hash format uses consecutive data items to
		 * dynamically calculate the item size.
		 * An item in this example is a key/data pair.
		 */
		offset = (u_int8_t *)p + HOFFSET(p);
		if (indx == 0)
			distance = dbp->pgsize - HOFFSET(p);
		else
			distance = (u_int32_t)
			    (P_ENTRY(dbp, p, indx - 1) - offset);
		memmove(offset - increase, offset, distance);

		/* Shuffle the index array */
		memmove(&inp[indx + 2], &inp[indx],
		    (n - indx) * sizeof(db_indx_t));

		/* update the index array */
		for (i = indx + 2; i < n + 2; i++)
			inp[i] -= increase;

		/* set the new index elements. */
		inp[indx] = (HOFFSET(p) - increase) + distance + dsize;
		inp[indx + 1] = (HOFFSET(p) - increase) + distance;
	}

	HOFFSET(p) -= increase;
	/* insert the new elements */
	if (key_type == H_OFFPAGE)
		memcpy(P_ENTRY(dbp, p, indx), key_dbt->data, key_dbt->size);
	else
		PUT_HKEYDATA(P_ENTRY(dbp, p, indx), key_dbt->data,
		    key_dbt->size, key_type);
	if (data_type == H_OFFPAGE)
		memcpy(P_ENTRY(dbp, p, indx+1), data_dbt->data,
		    data_dbt->size);
	else
		PUT_HKEYDATA(P_ENTRY(dbp, p, indx+1), data_dbt->data,
		    data_dbt->size, data_type);
	NUM_ENT(p) += 2;

	/*
	 * If debugging a sorted hash page problem, this is a good place to
	 * insert a call to __ham_verify_sorted_page.
	 * It used to be called when diagnostic mode was enabled, but that
	 * causes problems in recovery if a custom comparator was used.
	 */
	return (0);
}

/*
 * __hame_getindex --
 *
 * The key_type parameter overloads the entry type to allow for comparison of
 * a key DBT that contains off-page data. A key that is not of type H_OFFPAGE
 * might contain data larger than the page size, since this routine can be
 * called with user-provided DBTs.
 *
 * PUBLIC: int __ham_getindex __P((DBC *,
 * PUBLIC:     PAGE *, const DBT *, int, int *, db_indx_t *));
 */
int
__ham_getindex(dbc, p, key, key_type, match, indx)
	DBC *dbc;
	PAGE *p;
	const DBT *key;
	int key_type, *match;
	db_indx_t *indx;
{
	/* Since all entries are key/data pairs. */
	DB_ASSERT(dbc->env, NUM_ENT(p)%2 == 0 );

	/* Support pre 4.6 unsorted hash pages. */
	if (p->type == P_HASH_UNSORTED)
		return (__ham_getindex_unsorted(dbc, p, key, match, indx));
	else
		return (__ham_getindex_sorted(dbc,
		    p, key, key_type, match, indx));
}

#undef	min
#define	min(a, b) (((a) < (b)) ? (a) : (b))

/*
 * Perform a linear search of an unsorted (pre 4.6 format) hash page.
 *
 * This routine is never used to generate an index for insertion, because any
 * unsorted page is sorted before we insert.
 *
 * Returns 0 if an exact match is found, with indx set to requested elem.
 * Returns 1 if the item did not exist, indx is set to the last element on the
 * page.
 */
static int
__ham_getindex_unsorted(dbc, p, key, match, indx)
	DBC *dbc;
	PAGE *p;
	const DBT *key;
	int *match;
	db_indx_t *indx;
{
	DB *dbp;
	DBT pg_dbt;
	HASH *t;
	db_pgno_t pgno;
	int i, n, res, ret;
	u_int32_t tlen;
	u_int8_t *hk;

	dbp = dbc->dbp;
	n = NUM_ENT(p);
	t = dbp->h_internal;
	res = 1;

	/* Do a linear search over the page looking for an exact match */
	for (i = 0; i < n; i+=2) {
		hk = H_PAIRKEY(dbp, p, i);
		switch (HPAGE_PTYPE(hk)) {
		case H_OFFPAGE:
			/* extract item length from possibly unaligned DBT */
			memcpy(&tlen, HOFFPAGE_TLEN(hk), sizeof(u_int32_t));
			if (tlen == key->size) {
				memcpy(&pgno,
				    HOFFPAGE_PGNO(hk), sizeof(db_pgno_t));
				if ((ret = __db_moff(dbc, key, pgno, tlen,
				    t->h_compare, &res)) != 0)
					return (ret);
			}
			break;
		case H_KEYDATA:
			if (t->h_compare != NULL) {
				DB_INIT_DBT(pg_dbt,
				    HKEYDATA_DATA(hk), key->size);
				if (t->h_compare(
				    dbp, key, &pg_dbt) != 0)
					break;
			} else if (key->size ==
			    LEN_HKEY(dbp, p, dbp->pgsize, i))
				res = memcmp(key->data, HKEYDATA_DATA(hk),
				    key->size);
			break;
		case H_DUPLICATE:
		case H_OFFDUP:
			/*
			 * These are errors because keys are never duplicated.
			 */
			 /* FALLTHROUGH */
		default:
			return (__db_pgfmt(dbp->env, PGNO(p)));
		}
		if (res == 0)
			break;
	}
	*indx = i;
	*match = (res == 0 ? 0 : 1);
	return (0);
}

/*
 * Perform a binary search of a sorted hash page for a key.
 * Return 0 if an exact match is found, with indx set to requested elem.
 * Return 1 if the item did not exist, indx will be set to the first element
 * greater than the requested item.
 */
static int
__ham_getindex_sorted(dbc, p, key, key_type, match, indxp)
	DBC *dbc;
	PAGE *p;
	const DBT *key;
	int key_type, *match;
	db_indx_t *indxp;
{
	DB *dbp;
	DBT tmp_dbt;
	HASH *t;
	HOFFPAGE *offp;
	db_indx_t indx;
	db_pgno_t off_pgno, koff_pgno;
	u_int32_t base, itemlen, lim, off_len;
	u_int8_t *entry;
	int res, ret;
	void *data;

	dbp = dbc->dbp;
	DB_ASSERT(dbp->env, p->type == P_HASH );

	t = dbp->h_internal;
	/* Initialize so the return params are correct for empty pages. */
	res = indx = 0;

	/* Do a binary search for the element. */
	DB_BINARY_SEARCH_FOR(base, lim, NUM_ENT(p), 2) {
		DB_BINARY_SEARCH_INCR(indx, base, lim, 2);
		data = HKEYDATA_DATA(H_PAIRKEY(dbp, p, indx));
		/*
		 * There are 4 cases here:
		 *  1) Off page key, off page match
		 *  2) Off page key, on page match
		 *  3) On page key, off page match
		 *  4) On page key, on page match
		 */
		entry = P_ENTRY(dbp, p, indx);
		if (*entry == H_OFFPAGE) {
			offp = (HOFFPAGE*)P_ENTRY(dbp, p, indx);
			(void)__ua_memcpy(&itemlen, HOFFPAGE_TLEN(offp),
			    sizeof(u_int32_t));
			if (key_type == H_OFFPAGE) {
				/*
				 * Case 1.
				 *
				 * If both key and cmp DBTs refer to different
				 * offpage items, it is necessary to compare
				 * the content of the entries, in order to be
				 * able to maintain a valid lexicographic sort
				 * order.
				 */
				(void)__ua_memcpy(&koff_pgno,
				    HOFFPAGE_PGNO(key->data),
				    sizeof(db_pgno_t));
				(void)__ua_memcpy(&off_pgno, HOFFPAGE_PGNO(offp),
				    sizeof(db_pgno_t));
				if (koff_pgno == off_pgno)
					res = 0;
				else {
					memset(&tmp_dbt, 0, sizeof(tmp_dbt));
					tmp_dbt.size = HOFFPAGE_SIZE;
					tmp_dbt.data = offp;
					if ((ret = __db_coff(dbc, key, &tmp_dbt,
					    t->h_compare, &res)) != 0)
						return (ret);
				}
			} else {
				/* Case 2 */
				(void)__ua_memcpy(&off_pgno,
				    HOFFPAGE_PGNO(offp), sizeof(db_pgno_t));
				if ((ret = __db_moff(dbc, key, off_pgno,
				    itemlen, t->h_compare, &res)) != 0)
					return (ret);
			}
		} else {
			itemlen = LEN_HKEYDATA(dbp, p, dbp->pgsize, indx);
			if (key_type == H_OFFPAGE) {
				/* Case 3 */
				tmp_dbt.data = data;
				tmp_dbt.size = itemlen;
				offp = (HOFFPAGE *)key->data;
				(void)__ua_memcpy(&off_pgno,
				    HOFFPAGE_PGNO(offp), sizeof(db_pgno_t));
				(void)__ua_memcpy(&off_len, HOFFPAGE_TLEN(offp),
				    sizeof(u_int32_t));
				if ((ret = __db_moff(dbc, &tmp_dbt, off_pgno,
				    off_len, t->h_compare, &res)) != 0)
					return (ret);
				/*
				 * Since we switched the key/match parameters
				 * in the __db_moff call, the result needs to
				 * be inverted.
				 */
				res = -res;
			} else if (t->h_compare != NULL) {
				/* Case 4, with a user comparison func */
				DB_INIT_DBT(tmp_dbt, data, itemlen);
				res = t->h_compare(dbp, key, &tmp_dbt);
			} else {
				/* Case 4, without a user comparison func */
				if ((res = memcmp(key->data, data,
				    min(key->size, itemlen))) == 0)
					res = itemlen > key->size ? 1 :
					    (itemlen < key->size ? -1 : 0);
			}
		}
		if (res == 0) {
			/* Found a match */
			*indxp = indx;
			*match = 0;
			return (0);
		} else if (res > 0)
			DB_BINARY_SEARCH_SHIFT_BASE(indx, base, lim, 2);
	}
	/*
	 * If no match was found, and the comparison indicates that the
	 * closest match was lexicographically less than the input key adjust
	 * the insertion index to be after the index of the closest match.
	 */
	if (res > 0)
	    indx += 2;
	*indxp = indx;
	*match = 1;
	return (0);
}

/*
 * PUBLIC: int __ham_verify_sorted_page __P((DBC *, PAGE *));
 *
 * The__ham_verify_sorted_page function is used to determine the correctness
 * of sorted hash pages. The checks are used by verification, they are
 * implemented in the hash code because they are also useful debugging aids.
 */
int
__ham_verify_sorted_page (dbc, p)
	DBC *dbc;
	PAGE *p;
{
	DB *dbp;
	DBT prev_dbt, curr_dbt;
	ENV *env;
	HASH *t;
	db_pgno_t tpgno;
	u_int32_t curr_len, prev_len, tlen;
	u_int16_t *indxp;
	db_indx_t i, n;
	int res, ret;
	char *prev, *curr;

	/* Validate that next, prev pointers are OK */
	n = NUM_ENT(p);
	dbp = dbc->dbp;
	DB_ASSERT(dbp->env, n%2 == 0 );

	env = dbp->env;
	t = dbp->h_internal;

	/* Disable verification if a custom comparator is supplied */
	if (t->h_compare != NULL)
	    return (0);

	/* Iterate through page, ensuring order */
	prev = (char *)HKEYDATA_DATA(H_PAIRKEY(dbp, p, 0));
	prev_len = LEN_HKEYDATA(dbp, p, dbp->pgsize, 0);
	for (i = 2; i < n; i+=2) {
		curr = (char *)HKEYDATA_DATA(H_PAIRKEY(dbp, p, i));
		curr_len = LEN_HKEYDATA(dbp, p, dbp->pgsize, i);

		if (HPAGE_TYPE(dbp, p, i-2) == H_OFFPAGE &&
		    HPAGE_TYPE(dbp, p, i) == H_OFFPAGE) {
			memset(&prev_dbt, 0, sizeof(prev_dbt));
			memset(&curr_dbt, 0, sizeof(curr_dbt));
			prev_dbt.size = curr_dbt.size = HOFFPAGE_SIZE;
			prev_dbt.data = H_PAIRKEY(dbp, p, i-2);
			curr_dbt.data = H_PAIRKEY(dbp, p, i);
			if ((ret = __db_coff(dbc,
			    &prev_dbt, &curr_dbt, t->h_compare, &res)) != 0)
				return (ret);
		} else if (HPAGE_TYPE(dbp, p, i-2) == H_OFFPAGE) {
			memset(&curr_dbt, 0, sizeof(curr_dbt));
			curr_dbt.size = curr_len;
			curr_dbt.data = H_PAIRKEY(dbp, p, i);
			memcpy(&tlen, HOFFPAGE_TLEN(H_PAIRKEY(dbp, p, i-2)),
			    sizeof(u_int32_t));
			memcpy(&tpgno, HOFFPAGE_PGNO(H_PAIRKEY(dbp, p, i-2)),
			    sizeof(db_pgno_t));
			if ((ret = __db_moff(dbc,
			    &curr_dbt, tpgno, tlen, t->h_compare, &res)) != 0)
				return (ret);
		} else if (HPAGE_TYPE(dbp, p, i) == H_OFFPAGE) {
			memset(&prev_dbt, 0, sizeof(prev_dbt));
			prev_dbt.size = prev_len;
			prev_dbt.data = H_PAIRKEY(dbp, p, i);
			memcpy(&tlen, HOFFPAGE_TLEN(H_PAIRKEY(dbp, p, i)),
			    sizeof(u_int32_t));
			memcpy(&tpgno, HOFFPAGE_PGNO(H_PAIRKEY(dbp, p, i)),
			    sizeof(db_pgno_t));
			if ((ret = __db_moff(dbc,
			    &prev_dbt, tpgno, tlen, t->h_compare, &res)) != 0)
				return (ret);
		} else
			res = memcmp(prev, curr, min(curr_len, prev_len));

		if (res == 0 && curr_len > prev_len)
			res = 1;
		else if (res == 0 && curr_len < prev_len)
			res = -1;

		if (res >= 0) {
			__db_msg(env, "key1: %s, key2: %s, len: %lu\n",
			    (char *)prev, (char *)curr,
			    (u_long)min(curr_len, prev_len));
			__db_msg(env, "curroffset %lu\n", (u_long)i);
			__db_msg(env, "indexes: ");
			for (i = 0; i < n; i++) {
				indxp = P_INP(dbp, p) + i;
				__db_msg(env, "%04X, ", *indxp);
			}
			__db_msg(env, "\n");
#ifdef HAVE_STATISTICS
			if ((ret = __db_prpage(dbp, p, DB_PR_PAGE)) != 0)
				return (ret);
#endif
			DB_ASSERT(dbp->env, res < 0);
		}

		prev = curr;
		prev_len = curr_len;
	}
	return (0);
}

/*
 * A wrapper for the __ham_sort_page function. Implements logging and cursor
 * adjustments associated with sorting a page outside of recovery/upgrade.
 */
static int
__ham_sort_page_cursor(dbc, page)
	DBC *dbc;
	PAGE *page;
{
	DB *dbp;
	DBT page_dbt;
	DB_LSN new_lsn;
	HASH_CURSOR *hcp;
	int ret;

	dbp = dbc->dbp;
	hcp = (HASH_CURSOR *)dbc->internal;

	if (DBC_LOGGING(dbc)) {
		page_dbt.size = dbp->pgsize;
		page_dbt.data = page;
		if ((ret = __ham_splitdata_log(dbp, dbc->txn,
		    &new_lsn, 0, SORTPAGE, PGNO(page),
		    &page_dbt, &LSN(page))) != 0)
			return (ret);
	} else
		LSN_NOT_LOGGED(new_lsn);
	/* Move lsn onto page. */
	LSN(page) = new_lsn;	/* Structure assignment. */

	/*
	 * Invalidate the saved index, it needs to be retrieved
	 * again once the page is sorted.
	 */
	hcp->seek_found_indx = NDX_INVALID;
	hcp->seek_found_page = PGNO_INVALID;

	return (__ham_sort_page(dbc, &hcp->split_buf, page));
}

/*
 * PUBLIC: int __ham_sort_page __P((DBC *,  PAGE **, PAGE *));
 *
 * Convert a page from P_HASH_UNSORTED into the sorted format P_HASH.
 *
 * All locking and logging is carried out be the caller. A user buffer can
 * optionally be passed in to save allocating a page size buffer for sorting.
 * This is allows callers to re-use the buffer pre-allocated for page splits
 * in the hash cursor. The buffer is optional since no cursor exists when in
 * the recovery or upgrade code paths.
 */
int
__ham_sort_page(dbc, tmp_buf, page)
	DBC *dbc;
	PAGE **tmp_buf;
	PAGE *page;
{
	DB *dbp;
	PAGE *temp_pagep;
	db_indx_t i;
	int ret;

	dbp = dbc->dbp;
	DB_ASSERT(dbp->env, page->type == P_HASH_UNSORTED);

	ret = 0;
	if (tmp_buf != NULL)
		temp_pagep = *tmp_buf;
	else if ((ret = __os_malloc(dbp->env, dbp->pgsize, &temp_pagep)) != 0)
	    return (ret);

	memcpy(temp_pagep, page, dbp->pgsize);

	/* Re-initialize the page. */
	P_INIT(page, dbp->pgsize,
	    page->pgno, page->prev_pgno, page->next_pgno, 0, P_HASH);

	for (i = 0; i < NUM_ENT(temp_pagep); i += 2)
		if ((ret =
		    __ham_copypair(dbc, temp_pagep, i, page, NULL)) != 0)
			break;

	if (tmp_buf == NULL)
		__os_free(dbp->env, temp_pagep);

	return (ret);
}

/*
 * PUBLIC: int __ham_del_pair __P((DBC *, int));
 */
int
__ham_del_pair(dbc, flags)
	DBC *dbc;
	int flags;
{
	DB *dbp;
	DBT data_dbt, key_dbt;
	DB_LSN new_lsn, *n_lsn, tmp_lsn;
	DB_MPOOLFILE *mpf;
	HASH_CURSOR *hcp;
	PAGE *n_pagep, *nn_pagep, *p, *p_pagep;
	db_ham_mode op;
	db_indx_t ndx;
	db_pgno_t chg_pgno, pgno, tmp_pgno;
	u_int32_t order;
	int ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	hcp = (HASH_CURSOR *)dbc->internal;
	n_pagep = p_pagep = nn_pagep = NULL;
	ndx = hcp->indx;

	if (hcp->page == NULL &&
	    (ret = __memp_fget(mpf, &hcp->pgno, dbc->thread_info, dbc->txn,
	    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &hcp->page)) != 0)
		return (ret);
	p = hcp->page;

	/*
	 * We optimize for the normal case which is when neither the key nor
	 * the data are large.  In this case, we write a single log record
	 * and do the delete.  If either is large, we'll call __big_delete
	 * to remove the big item and then update the page to remove the
	 * entry referring to the big item.
	 */
	if (!LF_ISSET(HAM_DEL_IGNORE_OFFPAGE) &&
	    HPAGE_PTYPE(H_PAIRKEY(dbp, p, ndx)) == H_OFFPAGE) {
		memcpy(&pgno, HOFFPAGE_PGNO(P_ENTRY(dbp, p, H_KEYINDEX(ndx))),
		    sizeof(db_pgno_t));
		ret = __db_doff(dbc, pgno);
	} else
		ret = 0;

	if (!LF_ISSET(HAM_DEL_IGNORE_OFFPAGE) && ret == 0)
		switch (HPAGE_PTYPE(H_PAIRDATA(dbp, p, ndx))) {
		case H_OFFPAGE:
			memcpy(&pgno,
			    HOFFPAGE_PGNO(P_ENTRY(dbp, p, H_DATAINDEX(ndx))),
			    sizeof(db_pgno_t));
			ret = __db_doff(dbc, pgno);
			break;
		case H_OFFDUP:
		case H_DUPLICATE:
			/*
			 * If we delete a pair that is/was a duplicate, then
			 * we had better clear the flag so that we update the
			 * cursor appropriately.
			 */
			F_CLR(hcp, H_ISDUP);
			break;
		default:
			/* No-op */
			break;
		}

	if (ret)
		return (ret);

	/* Now log the delete off this page. */
	if (DBC_LOGGING(dbc)) {
		key_dbt.data = P_ENTRY(dbp, p, H_KEYINDEX(ndx));
		key_dbt.size = LEN_HITEM(dbp, p, dbp->pgsize, H_KEYINDEX(ndx));
		data_dbt.data = P_ENTRY(dbp, p, H_DATAINDEX(ndx));
		data_dbt.size =
		    LEN_HITEM(dbp, p, dbp->pgsize, H_DATAINDEX(ndx));

		if ((ret = __ham_insdel_log(dbp,
		    dbc->txn, &new_lsn, 0, DELPAIR, PGNO(p), (u_int32_t)ndx,
		    &LSN(p), &key_dbt, &data_dbt)) != 0)
			return (ret);
	} else
		LSN_NOT_LOGGED(new_lsn);

	/* Move lsn onto page. */
	LSN(p) = new_lsn;
	/* Do the delete. */
	__ham_dpair(dbp, p, ndx);

	/*
	 * Mark item deleted so that we don't try to return it, and
	 * so that we update the cursor correctly on the next call
	 * to next.
	 */
	F_SET(hcp, H_DELETED);
	F_CLR(hcp, H_OK);

	/* Clear any cache streaming information. */
	hcp->stream_start_pgno = PGNO_INVALID;

	/*
	 * If we are locking, we will not maintain this, because it is
	 * a hot spot.
	 *
	 * XXX
	 * Perhaps we can retain incremental numbers and apply them later.
	 */
	if (!STD_LOCKING(dbc)) {
		if ((ret = __ham_dirty_meta(dbc, 0)) != 0)
			return (ret);
		--hcp->hdr->nelem;
	}

	/* The HAM_DEL_NO_CURSOR flag implies HAM_DEL_NO_RECLAIM. */
	if (LF_ISSET(HAM_DEL_NO_CURSOR))
		return (0);
	/*
	 * Update cursors that are on the page where the delete happened.
	 */
	if ((ret = __hamc_update(dbc, 0, DB_HAM_CURADJ_DEL, 0)) != 0)
		return (ret);

	/*
	 * If we need to reclaim the page, then check if the page is empty.
	 * There are two cases.  If it's empty and it's not the first page
	 * in the bucket (i.e., the bucket page) then we can simply remove
	 * it. If it is the first chain in the bucket, then we need to copy
	 * the second page into it and remove the second page.
	 * If its the only page in the bucket we leave it alone.
	 */
	if (LF_ISSET(HAM_DEL_NO_RECLAIM) ||
	    NUM_ENT(p) != 0 ||
	    (PREV_PGNO(p) == PGNO_INVALID && NEXT_PGNO(p) == PGNO_INVALID))
		return (0);

	if (PREV_PGNO(p) == PGNO_INVALID) {
		/*
		 * First page in chain is empty and we know that there
		 * are more pages in the chain.
		 */
		if ((ret = __memp_fget(mpf,
		    &NEXT_PGNO(p), dbc->thread_info, dbc->txn,
		    DB_MPOOL_DIRTY, &n_pagep)) != 0)
			return (ret);

		if (NEXT_PGNO(n_pagep) != PGNO_INVALID &&
		    (ret = __memp_fget(mpf, &NEXT_PGNO(n_pagep),
		    dbc->thread_info, dbc->txn,
		    DB_MPOOL_DIRTY, &nn_pagep)) != 0)
			goto err;

		if (DBC_LOGGING(dbc)) {
			key_dbt.data = n_pagep;
			key_dbt.size = dbp->pgsize;
			if ((ret = __ham_copypage_log(dbp,
			    dbc->txn, &new_lsn, 0, PGNO(p),
			    &LSN(p), PGNO(n_pagep), &LSN(n_pagep),
			    NEXT_PGNO(n_pagep),
			    nn_pagep == NULL ? NULL : &LSN(nn_pagep),
			    &key_dbt)) != 0)
				goto err;
		} else
			LSN_NOT_LOGGED(new_lsn);

		/* Move lsn onto page. */
		LSN(p) = new_lsn;	/* Structure assignment. */
		LSN(n_pagep) = new_lsn;
		if (NEXT_PGNO(n_pagep) != PGNO_INVALID)
			LSN(nn_pagep) = new_lsn;

		if (nn_pagep != NULL) {
			PREV_PGNO(nn_pagep) = PGNO(p);
			ret = __memp_fput(mpf,
			    dbc->thread_info, nn_pagep, dbc->priority);
			nn_pagep = NULL;
			if (ret != 0)
				goto err;
		}

		tmp_pgno = PGNO(p);
		tmp_lsn = LSN(p);
		memcpy(p, n_pagep, dbp->pgsize);
		PGNO(p) = tmp_pgno;
		LSN(p) = tmp_lsn;
		PREV_PGNO(p) = PGNO_INVALID;

		/*
		 * Update cursors to reflect the fact that records
		 * on the second page have moved to the first page.
		 */
		if ((ret = __hamc_delpg(dbc, PGNO(n_pagep),
		    PGNO(p), 0, DB_HAM_DELFIRSTPG, &order)) != 0)
			goto err;

		/*
		 * Update the cursor to reflect its new position.
		 */
		hcp->indx = 0;
		hcp->pgno = PGNO(p);
		hcp->order += order;

		if ((ret = __db_free(dbc, n_pagep)) != 0) {
			n_pagep = NULL;
			goto err;
		}
	} else {
		if ((ret = __memp_fget(mpf,
		    &PREV_PGNO(p), dbc->thread_info, dbc->txn,
		    DB_MPOOL_DIRTY, &p_pagep)) != 0)
			goto err;

		if (NEXT_PGNO(p) != PGNO_INVALID) {
			if ((ret = __memp_fget(mpf, &NEXT_PGNO(p),
			dbc->thread_info, dbc->txn,
			    DB_MPOOL_DIRTY, &n_pagep)) != 0)
				goto err;
			n_lsn = &LSN(n_pagep);
		} else {
			n_pagep = NULL;
			n_lsn = NULL;
		}

		NEXT_PGNO(p_pagep) = NEXT_PGNO(p);
		if (n_pagep != NULL)
			PREV_PGNO(n_pagep) = PGNO(p_pagep);

		if (DBC_LOGGING(dbc)) {
			if ((ret = __ham_newpage_log(dbp, dbc->txn,
			    &new_lsn, 0, DELOVFL, PREV_PGNO(p), &LSN(p_pagep),
			    PGNO(p), &LSN(p), NEXT_PGNO(p), n_lsn)) != 0)
				goto err;
		} else
			LSN_NOT_LOGGED(new_lsn);

		/* Move lsn onto page. */
		LSN(p_pagep) = new_lsn;	/* Structure assignment. */
		if (n_pagep)
			LSN(n_pagep) = new_lsn;
		LSN(p) = new_lsn;

		if (NEXT_PGNO(p) == PGNO_INVALID) {
			/*
			 * There is no next page; put the cursor on the
			 * previous page as if we'd deleted the last item
			 * on that page, with index after the last valid
			 * entry.
			 *
			 * The deleted flag was set up above.
			 */
			hcp->pgno = PGNO(p_pagep);
			hcp->indx = NUM_ENT(p_pagep);
			op = DB_HAM_DELLASTPG;
		} else {
			/*
			 * There is a next page, so put the cursor at
			 * the beginning of it.
			 */
			hcp->pgno = NEXT_PGNO(p);
			hcp->indx = 0;
			op = DB_HAM_DELMIDPG;
		}

		/*
		 * Since we are about to delete the cursor page and we have
		 * just moved the cursor, we need to make sure that the
		 * old page pointer isn't left hanging around in the cursor.
		 */
		hcp->page = NULL;
		chg_pgno = PGNO(p);
		ret = __db_free(dbc, p);
		if ((t_ret = __memp_fput(mpf, dbc->thread_info,
		       p_pagep, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
		if (n_pagep != NULL && (t_ret = __memp_fput(mpf,
		    dbc->thread_info, n_pagep, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			return (ret);
		if ((ret = __hamc_delpg(dbc,
		    chg_pgno, hcp->pgno, hcp->indx, op, &order)) != 0)
			return (ret);
		hcp->order += order;
	}
	return (ret);

err:	/* Clean up any pages. */
	if (n_pagep != NULL)
		(void)__memp_fput(mpf,
		    dbc->thread_info, n_pagep, dbc->priority);
	if (nn_pagep != NULL)
		(void)__memp_fput(mpf,
		    dbc->thread_info, nn_pagep, dbc->priority);
	if (p_pagep != NULL)
		(void)__memp_fput(mpf,
		    dbc->thread_info, p_pagep, dbc->priority);
	return (ret);
}

/*
 * __ham_replpair --
 *	Given the key data indicated by the cursor, replace part/all of it
 *	according to the fields in the dbt.
 *
 * PUBLIC: int __ham_replpair __P((DBC *, DBT *, u_int32_t));
 */
int
__ham_replpair(dbc, dbt, make_dup)
	DBC *dbc;
	DBT *dbt;
	u_int32_t make_dup;
{
	DB *dbp;
	DBC **carray, *dbc_n;
	DBT old_dbt, tdata, tmp, *new_dbt;
	DB_LSN	new_lsn;
	ENV *env;
	HASH_CURSOR *hcp, *cp;
	db_indx_t orig_indx;
	db_pgno_t off_pgno, orig_pgno;
	u_int32_t change;
	u_int32_t dup_flag, len, memsize, newlen;
	char tmp_ch;
	int beyond_eor, is_big, is_plus, ret, type, i, found, t_ret;
	u_int8_t *beg, *dest, *end, *hk, *src;
	void *memp;

	/*
	 * Most items that were already offpage (ISBIG) were handled before
	 * we get in here.  So, we need only handle cases where the old
	 * key is on a regular page.  That leaves us 6 cases:
	 * 1. Original data onpage; new data is smaller
	 * 2. Original data onpage; new data is the same size
	 * 3. Original data onpage; new data is bigger, but not ISBIG,
	 *    fits on page
	 * 4. Original data onpage; new data is bigger, but not ISBIG,
	 *    does not fit on page
	 * 5. Original data onpage; New data is an off-page item.
	 * 6. Original data was offpage; new item is smaller.
	 * 7. Original data was offpage; new item is supplied as a partial.
	 *
	 * Cases 1-3 are essentially the same (and should be the common case).
	 * We handle 4-6 as delete and add. 7 is generally a delete and add,
	 * unless it is an append, when we extend the offpage item, and
	 * update the HOFFPAGE item on the current page to have the new size
	 * via a delete/add.
	 */
	dbp = dbc->dbp;
	env = dbp->env;
	hcp = (HASH_CURSOR *)dbc->internal;
	carray = NULL;
	dbc_n = memp = NULL;
	found = 0;
	new_dbt = NULL;
	off_pgno = PGNO_INVALID;
	type = 0;

	/*
	 * We need to compute the number of bytes that we are adding or
	 * removing from the entry.  Normally, we can simply subtract
	 * the number of bytes we are replacing (dbt->dlen) from the
	 * number of bytes we are inserting (dbt->size).  However, if
	 * we are doing a partial put off the end of a record, then this
	 * formula doesn't work, because we are essentially adding
	 * new bytes.
	 */
	if (dbt->size > dbt->dlen) {
		change = dbt->size - dbt->dlen;
		is_plus = 1;
	} else {
		change = dbt->dlen - dbt->size;
		is_plus = 0;
	}

	hk = H_PAIRDATA(dbp, hcp->page, hcp->indx);
	is_big = HPAGE_PTYPE(hk) == H_OFFPAGE;

	if (is_big) {
		memcpy(&len, HOFFPAGE_TLEN(hk), sizeof(u_int32_t));
		memcpy(&off_pgno, HOFFPAGE_PGNO(hk), sizeof(db_pgno_t));
	} else
		len = LEN_HKEYDATA(dbp, hcp->page,
		    dbp->pgsize, H_DATAINDEX(hcp->indx));

	beyond_eor = dbt->doff + dbt->dlen > len;
	if (beyond_eor) {
		/*
		 * The change is beyond the end of record.  If change
		 * is a positive number, we can simply add the extension
		 * to it.  However, if change is negative, then we need
		 * to figure out if the extension is larger than the
		 * negative change.
		 */
		if (is_plus)
			change += dbt->doff + dbt->dlen - len;
		else if (dbt->doff + dbt->dlen - len > change) {
			/* Extension bigger than change */
			is_plus = 1;
			change = (dbt->doff + dbt->dlen - len) - change;
		} else /* Extension is smaller than change. */
			change -= (dbt->doff + dbt->dlen - len);
	}

	newlen = (is_plus ? len + change : len - change);
	if (is_big || beyond_eor || ISBIG(hcp, newlen) ||
	    (is_plus && change > P_FREESPACE(dbp, hcp->page))) {
		/*
		 * If we are in cases 4 or 5 then is_plus will be true.
		 * If we don't have a transaction then we cannot roll back,
		 * make sure there is enough room for the new page.
		 */
		if (is_plus && dbc->txn == NULL &&
		    dbp->mpf->mfp->maxpgno != 0 &&
		    dbp->mpf->mfp->maxpgno == dbp->mpf->mfp->last_pgno)
			return (__db_space_err(dbp));
		/*
		 * Cases 4-6 -- two subcases.
		 * A. This is not really a partial operation, but an overwrite.
		 *    Simple del and add works.
		 * B. This is a partial and we need to construct the data that
		 *    we are really inserting (yuck).
		 * In both cases, we need to grab the key off the page (in
		 * some cases we could do this outside of this routine; for
		 * cleanliness we do it here.  If you happen to be on a big
		 * key, this could be a performance hit).
		 */
		memset(&tmp, 0, sizeof(tmp));
		if ((ret = __db_ret(dbc, hcp->page, H_KEYINDEX(hcp->indx),
		    &tmp, &dbc->my_rkey.data, &dbc->my_rkey.ulen)) != 0)
			return (ret);

		/* Preserve duplicate info. */
		dup_flag = F_ISSET(hcp, H_ISDUP);
		/* Streaming insert. */
		if (is_big && !dup_flag && !DB_IS_PRIMARY(dbp) &&
		    F_ISSET(dbt, DB_DBT_PARTIAL) && dbt->doff == len) {
			/*
			 * If the cursor has not already cached the last page
			 * in the offpage chain, we need to walk the chain to
			 * be sure that the page has been read.
			 */
			if (hcp->stream_start_pgno != off_pgno ||
			    hcp->stream_off > dbt->doff || dbt->doff >
			    hcp->stream_off + P_MAXSPACE(dbp, dbp->pgsize)) {
				memset(&tdata, 0, sizeof (DBT));
				tdata.doff = dbt->doff - 1;
				/*
				 * Set the length to 1, to force __db_goff
				 * to do the traversal.
				 */
				tdata.dlen = tdata.ulen = 1;
				tdata.data = &tmp_ch;
				tdata.flags = DB_DBT_PARTIAL | DB_DBT_USERMEM;

				/*
				 * Read to the last page.  It will be cached
				 * in the cursor.
				 */
				if ((ret = __db_goff(dbc, &tdata, len,
				    off_pgno, NULL, NULL)) != 0)
					return (ret);
			}
			/*
			 * Since this is an append, dlen is irrelevant (there
			 * are no bytes to overwrite). We need the caller's
			 * DBT size to end up with the total size of the item.
			 * From now on, use dlen as the length of the user's
			 * data that we are going to append.
			 * Don't futz with the caller's DBT any more than we
			 * have to in order to send back the size.
			 */
			tdata = *dbt;
			tdata.dlen = dbt->size;
			tdata.size = newlen;
			new_dbt = &tdata;
			F_SET(new_dbt, DB_DBT_STREAMING);
			type = H_KEYDATA;
		}

		/*
		 * In cases 4-6, a delete and insert works, but we need to
		 * track and update any cursors pointing to the item being
		 * moved.
		 */
		orig_pgno = PGNO(hcp->page);
		orig_indx = hcp->indx;
		if ((ret = __ham_get_clist(dbp,
		    orig_pgno, orig_indx, &carray)) != 0)
			goto err;

		if (dbt->doff == 0 && dbt->dlen == len) {
			type = (dup_flag ? H_DUPLICATE : H_KEYDATA);
			new_dbt = dbt;
		} else if (!F_ISSET(dbt, DB_DBT_STREAMING)) {	/* Case B */
			type = HPAGE_PTYPE(hk) != H_OFFPAGE ?
			    HPAGE_PTYPE(hk) : H_KEYDATA;
			memset(&tdata, 0, sizeof(tdata));
			memsize = 0;
			if ((ret = __db_ret(dbc, hcp->page,
			    H_DATAINDEX(hcp->indx), &tdata,
			    &memp, &memsize)) != 0)
				goto err;

			/* Now shift old data around to make room for new. */
			if (is_plus) {
				if ((ret = __os_realloc(env,
				    tdata.size + change, &tdata.data)) != 0)
					return (ret);
				memp = tdata.data;
				memsize = tdata.size + change;
				memset((u_int8_t *)tdata.data + tdata.size,
				    0, change);
			}
			end = (u_int8_t *)tdata.data + tdata.size;

			src = (u_int8_t *)tdata.data + dbt->doff + dbt->dlen;
			if (src < end && tdata.size > dbt->doff + dbt->dlen) {
				len = tdata.size - (dbt->doff + dbt->dlen);
				if (is_plus)
					dest = src + change;
				else
					dest = src - change;
				memmove(dest, src, len);
			}
			memcpy((u_int8_t *)tdata.data + dbt->doff,
			    dbt->data, dbt->size);
			if (is_plus)
				tdata.size += change;
			else
				tdata.size -= change;
			new_dbt = &tdata;
		}
		if ((ret = __ham_del_pair(dbc, HAM_DEL_NO_CURSOR |
		    (F_ISSET(dbt, DB_DBT_STREAMING) ? HAM_DEL_IGNORE_OFFPAGE :
		    0))) != 0)
			goto err;
		/*
		 * Save the state of the cursor after the delete, so that we
		 * can adjust any cursors impacted by the delete. Don't just
		 * update the cursors now, to avoid ambiguity in reversing the
		 * adjustments during abort.
		 */
		if ((ret = __dbc_dup(dbc, &dbc_n, DB_POSITION)) != 0)
			goto err;
		if ((ret = __ham_add_el(dbc, &tmp, new_dbt, type)) != 0)
			goto err;
		F_SET(hcp, dup_flag);

		/*
		 * If the delete/insert pair caused the item to be moved
		 * to another location (which is possible for duplicate sets
		 * that are moved onto another page in the bucket), then update
		 * any impacted cursors.
		 */
		if (((HASH_CURSOR*)dbc_n->internal)->pgno != hcp->pgno ||
		    ((HASH_CURSOR*)dbc_n->internal)->indx != hcp->indx) {
			/*
			 * Set any cursors pointing to items in the moved
			 * duplicate set to the destination location and reset
			 * the deleted flag. This can't be done earlier, since
			 * the insert location is not computed until the actual
			 * __ham_add_el call is made.
			 */
			if (carray != NULL) {
				for (i = 0; carray[i] != NULL; i++) {
					cp = (HASH_CURSOR*)carray[i]->internal;
					cp->pgno = hcp->pgno;
					cp->indx = hcp->indx;
					F_CLR(cp, H_DELETED);
					found = 1;
				}
				/*
				 * Only log the update once, since the recovery
				 * code iterates through all open cursors and
				 * applies the change to all matching cursors.
				 */
				if (found && DBC_LOGGING(dbc) &&
				    IS_SUBTRANSACTION(dbc->txn)) {
					if ((ret =
					    __ham_chgpg_log(dbp,
					    dbc->txn, &new_lsn, 0,
					    DB_HAM_CHGPG, orig_pgno, hcp->pgno,
					    orig_indx, hcp->indx)) != 0)
						goto err;
				}
			}
			/*
			 * Update any cursors impacted by the delete. Do this
			 * after chgpg log so that recovery does not re-bump
			 * cursors pointing to the deleted item.
			 */
			ret = __hamc_update(dbc_n, 0, DB_HAM_CURADJ_DEL, 0);
		}

err:		if (dbc_n != NULL && (t_ret = __dbc_close(dbc_n)) != 0 &&
		    ret == 0)
			ret = t_ret;
		if (carray != NULL)
			__os_free(env, carray);
		if (memp != NULL)
			__os_free(env, memp);
		return (ret);
	}

	/*
	 * Set up pointer into existing data. Do it before the log
	 * message so we can use it inside of the log setup.
	 */
	beg = HKEYDATA_DATA(H_PAIRDATA(dbp, hcp->page, hcp->indx));
	beg += dbt->doff;

	/*
	 * If we are going to have to move bytes at all, figure out
	 * all the parameters here.  Then log the call before moving
	 * anything around.
	 */
	if (DBC_LOGGING(dbc)) {
		old_dbt.data = beg;
		old_dbt.size = dbt->dlen;
		if ((ret = __ham_replace_log(dbp,
		    dbc->txn, &new_lsn, 0, PGNO(hcp->page),
		    (u_int32_t)H_DATAINDEX(hcp->indx), &LSN(hcp->page),
		    (int32_t)dbt->doff, &old_dbt, dbt, make_dup)) != 0)
			return (ret);
	} else
		LSN_NOT_LOGGED(new_lsn);

	LSN(hcp->page) = new_lsn;	/* Structure assignment. */

	__ham_onpage_replace(dbp, hcp->page, (u_int32_t)H_DATAINDEX(hcp->indx),
	    (int32_t)dbt->doff, change, is_plus, dbt);

	return (0);
}

/*
 * Replace data on a page with new data, possibly growing or shrinking what's
 * there.  This is called on two different occasions. On one (from replpair)
 * we are interested in changing only the data.  On the other (from recovery)
 * we are replacing the entire data (header and all) with a new element.  In
 * the latter case, the off argument is negative.
 * pagep: the page that we're changing
 * ndx: page index of the element that is growing/shrinking.
 * off: Offset at which we are beginning the replacement.
 * change: the number of bytes (+ or -) that the element is growing/shrinking.
 * dbt: the new data that gets written at beg.
 *
 * PUBLIC: void __ham_onpage_replace __P((DB *, PAGE *, u_int32_t,
 * PUBLIC:     int32_t, u_int32_t,  int, DBT *));
 */
void
__ham_onpage_replace(dbp, pagep, ndx, off, change, is_plus, dbt)
	DB *dbp;
	PAGE *pagep;
	u_int32_t ndx;
	int32_t off;
	u_int32_t change;
	int is_plus;
	DBT *dbt;
{
	db_indx_t i, *inp;
	int32_t len;
	size_t pgsize;
	u_int8_t *src, *dest;
	int zero_me;

	pgsize = dbp->pgsize;
	inp = P_INP(dbp, pagep);
	if (change != 0) {
		zero_me = 0;
		src = (u_int8_t *)(pagep) + HOFFSET(pagep);
		if (off < 0)
			len = inp[ndx] - HOFFSET(pagep);
		else if ((u_int32_t)off >=
		    LEN_HKEYDATA(dbp, pagep, pgsize, ndx)) {
			len = (int32_t)(HKEYDATA_DATA(P_ENTRY(dbp, pagep, ndx))
			    + LEN_HKEYDATA(dbp, pagep, pgsize, ndx) - src);
			zero_me = 1;
		} else
			len = (int32_t)(
			    (HKEYDATA_DATA(P_ENTRY(dbp, pagep, ndx)) + off) -
			    src);
		if (is_plus)
			dest = src - change;
		else
			dest = src + change;
		memmove(dest, src, (size_t)len);
		if (zero_me)
			memset(dest + len, 0, change);

		/* Now update the indices. */
		for (i = ndx; i < NUM_ENT(pagep); i++) {
			if (is_plus)
				inp[i] -= change;
			else
				inp[i] += change;
		}
		if (is_plus)
			HOFFSET(pagep) -= change;
		else
			HOFFSET(pagep) += change;
	}
	if (off >= 0)
		memcpy(HKEYDATA_DATA(P_ENTRY(dbp, pagep, ndx)) + off,
		    dbt->data, dbt->size);
	else
		memcpy(P_ENTRY(dbp, pagep, ndx), dbt->data, dbt->size);
}

/*
 * PUBLIC: int __ham_split_page __P((DBC *, u_int32_t, u_int32_t));
 */
int
__ham_split_page(dbc, obucket, nbucket)
	DBC *dbc;
	u_int32_t obucket, nbucket;
{
	DB *dbp;
	DBC **carray, *tmp_dbc;
	DBT key, page_dbt;
	DB_LOCK block;
	DB_LSN new_lsn;
	DB_MPOOLFILE *mpf;
	ENV *env;
	HASH_CURSOR *hcp, *cp;
	PAGE **pp, *old_pagep, *temp_pagep, *new_pagep;
	db_indx_t n, dest_indx;
	db_pgno_t bucket_pgno, npgno, next_pgno;
	u_int32_t big_len, len;
	int found, i, ret, t_ret;
	void *big_buf;

	dbp = dbc->dbp;
	carray = NULL;
	env = dbp->env;
	mpf = dbp->mpf;
	hcp = (HASH_CURSOR *)dbc->internal;
	temp_pagep = old_pagep = new_pagep = NULL;
	npgno = PGNO_INVALID;
	LOCK_INIT(block);

	bucket_pgno = BUCKET_TO_PAGE(hcp, obucket);
	if ((ret = __db_lget(dbc,
	    0, bucket_pgno, DB_LOCK_WRITE, 0, &block)) != 0)
		goto err;
	if ((ret = __memp_fget(mpf, &bucket_pgno, dbc->thread_info, dbc->txn,
	    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &old_pagep)) != 0)
		goto err;

	/* Sort any unsorted pages before doing a hash split. */
	if (old_pagep->type == P_HASH_UNSORTED)
		if ((ret = __ham_sort_page_cursor(dbc, old_pagep)) != 0)
			return (ret);

	/* Properly initialize the new bucket page. */
	npgno = BUCKET_TO_PAGE(hcp, nbucket);
	if ((ret = __memp_fget(mpf, &npgno, dbc->thread_info, dbc->txn,
	    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &new_pagep)) != 0)
		goto err;
	P_INIT(new_pagep,
	    dbp->pgsize, npgno, PGNO_INVALID, PGNO_INVALID, 0, P_HASH);

	temp_pagep = hcp->split_buf;
	memcpy(temp_pagep, old_pagep, dbp->pgsize);

	if (DBC_LOGGING(dbc)) {
		page_dbt.size = dbp->pgsize;
		page_dbt.data = old_pagep;
		if ((ret = __ham_splitdata_log(dbp,
		    dbc->txn, &new_lsn, 0, SPLITOLD,
		    PGNO(old_pagep), &page_dbt, &LSN(old_pagep))) != 0)
			goto err;
	} else
		LSN_NOT_LOGGED(new_lsn);

	LSN(old_pagep) = new_lsn;	/* Structure assignment. */

	P_INIT(old_pagep, dbp->pgsize, PGNO(old_pagep), PGNO_INVALID,
	    PGNO_INVALID, 0, P_HASH);

	big_len = 0;
	big_buf = NULL;
	memset(&key, 0, sizeof(key));
	while (temp_pagep != NULL) {
		if ((ret = __ham_get_clist(dbp,
		    PGNO(temp_pagep), NDX_INVALID, &carray)) != 0)
			goto err;

		for (n = 0; n < (db_indx_t)NUM_ENT(temp_pagep); n += 2) {
			if ((ret = __db_ret(dbc, temp_pagep, H_KEYINDEX(n),
			    &key, &big_buf, &big_len)) != 0)
				goto err;

			if (__ham_call_hash(dbc, key.data, key.size) == obucket)
				pp = &old_pagep;
			else
				pp = &new_pagep;

			/*
			 * Figure out how many bytes we need on the new
			 * page to store the key/data pair.
			 */
			len = LEN_HITEM(dbp, temp_pagep, dbp->pgsize,
			    H_DATAINDEX(n)) +
			    LEN_HITEM(dbp, temp_pagep, dbp->pgsize,
			    H_KEYINDEX(n)) +
			    2 * sizeof(db_indx_t);

			if (P_FREESPACE(dbp, *pp) < len) {
				if (DBC_LOGGING(dbc)) {
					page_dbt.size = dbp->pgsize;
					page_dbt.data = *pp;
					if ((ret = __ham_splitdata_log(dbp,
					    dbc->txn, &new_lsn, 0,
					    SPLITNEW, PGNO(*pp), &page_dbt,
					    &LSN(*pp))) != 0)
						goto err;
				} else
					LSN_NOT_LOGGED(new_lsn);
				LSN(*pp) = new_lsn;
				if ((ret =
				    __ham_add_ovflpage(dbc, *pp, 1, pp)) != 0)
					goto err;
			}

			dest_indx = NDX_INVALID;
			if ((ret = __ham_copypair(dbc, temp_pagep,
			    H_KEYINDEX(n), *pp, &dest_indx)) != 0)
			    goto err;

			/*
			 * Update any cursors that were pointing to items
			 * shuffled because of this insert.
			 * Use __hamc_update, since the cursor adjustments are
			 * the same as those required for an insert. The
			 * overhead of creating a cursor is worthwhile to save
			 * replicating the adjustment functionality.
			 * Adjusting shuffled cursors needs to be done prior to
			 * adjusting any cursors that were pointing to the
			 * moved item.
			 * All pages in a bucket are sorted, but the items are
			 * not sorted across pages within a bucket. This means
			 * that splitting the first page in a bucket into two
			 * new buckets won't require any cursor shuffling,
			 * since all inserts will be appends. Splitting of the
			 * second etc page from the initial bucket could
			 * cause an item to be inserted at any location on a
			 * page (since items already inserted from page 1 of
			 * the initial bucket may overlap), so only adjust
			 * cursors for the second etc pages within a bucket.
			 */
			if (PGNO(temp_pagep) != bucket_pgno) {
				if ((ret = __db_cursor_int(dbp,
				    dbc->thread_info, dbc->txn, dbp->type,
				    PGNO_INVALID, 0, DB_LOCK_INVALIDID,
				    &tmp_dbc)) != 0)
					goto err;
				hcp = (HASH_CURSOR*)tmp_dbc->internal;
				hcp->pgno = PGNO(*pp);
				hcp->indx = dest_indx;
				hcp->dup_off = 0;
				hcp->order = 0;
				if ((ret = __hamc_update(
				    tmp_dbc, len, DB_HAM_CURADJ_ADD, 0)) != 0)
					goto err;
				if ((ret = __dbc_close(tmp_dbc)) != 0)
					goto err;
			}
			/* Update any cursors pointing at the moved item. */
			if (carray != NULL) {
				found = 0;
				for (i = 0; carray[i] != NULL; i++) {
					cp =
					    (HASH_CURSOR *)carray[i]->internal;
					if (cp->pgno == PGNO(temp_pagep) &&
					    cp->indx == n) {
						cp->pgno = PGNO(*pp);
						cp->indx = dest_indx;
						found = 1;
					}
				}
				/*
				 * Only log the update once, since the recovery
				 * code iterates through all open cursors and
				 * applies the change to all matching cursors.
				 */
				if (found && DBC_LOGGING(dbc) &&
				    IS_SUBTRANSACTION(dbc->txn)) {
					if ((ret =
					    __ham_chgpg_log(dbp,
					    dbc->txn, &new_lsn, 0,
					    DB_HAM_SPLIT, PGNO(temp_pagep),
					    PGNO(*pp), n, dest_indx)) != 0)
						goto err;
				}
			}
		}
		next_pgno = NEXT_PGNO(temp_pagep);

		/* Clear temp_page; if it's a link overflow page, free it. */
		if (PGNO(temp_pagep) != bucket_pgno && (ret =
		    __db_free(dbc, temp_pagep)) != 0) {
			temp_pagep = NULL;
			goto err;
		}

		if (next_pgno == PGNO_INVALID)
			temp_pagep = NULL;
		else if ((ret = __memp_fget(mpf,
		    &next_pgno, dbc->thread_info, dbc->txn,
		    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &temp_pagep)) != 0)
			goto err;

		if (temp_pagep != NULL) {
			if (DBC_LOGGING(dbc)) {
				page_dbt.size = dbp->pgsize;
				page_dbt.data = temp_pagep;
				if ((ret = __ham_splitdata_log(dbp,
				    dbc->txn, &new_lsn, 0,
				    SPLITOLD, PGNO(temp_pagep),
				    &page_dbt, &LSN(temp_pagep))) != 0)
					goto err;
			} else
				LSN_NOT_LOGGED(new_lsn);
			LSN(temp_pagep) = new_lsn;
		}

		if (carray != NULL)	/* We never knew its size. */
			__os_free(env, carray);
		carray = NULL;
	}
	if (big_buf != NULL)
		__os_free(env, big_buf);

	/*
	 * If the original bucket spanned multiple pages, then we've got
	 * a pointer to a page that used to be on the bucket chain.  It
	 * should be deleted.
	 */
	if (temp_pagep != NULL && PGNO(temp_pagep) != bucket_pgno &&
	    (ret = __db_free(dbc, temp_pagep)) != 0) {
		temp_pagep = NULL;
		goto err;
	}

	/*
	 * Write new buckets out.
	 */
	if (DBC_LOGGING(dbc)) {
		page_dbt.size = dbp->pgsize;
		page_dbt.data = old_pagep;
		if ((ret = __ham_splitdata_log(dbp, dbc->txn,
		    &new_lsn, 0, SPLITNEW, PGNO(old_pagep), &page_dbt,
		    &LSN(old_pagep))) != 0)
			goto err;
		LSN(old_pagep) = new_lsn;

		page_dbt.data = new_pagep;
		if ((ret = __ham_splitdata_log(dbp, dbc->txn, &new_lsn, 0,
		    SPLITNEW, PGNO(new_pagep), &page_dbt,
		    &LSN(new_pagep))) != 0)
			goto err;
		LSN(new_pagep) = new_lsn;
	} else {
		LSN_NOT_LOGGED(LSN(old_pagep));
		LSN_NOT_LOGGED(LSN(new_pagep));
	}

	ret = __memp_fput(mpf, dbc->thread_info, old_pagep, dbc->priority);
	if ((t_ret = __memp_fput(mpf,
	    dbc->thread_info, new_pagep, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;

	if (0) {
err:		if (old_pagep != NULL)
			(void)__memp_fput(mpf,
			    dbc->thread_info, old_pagep, dbc->priority);
		if (new_pagep != NULL) {
			P_INIT(new_pagep, dbp->pgsize,
			     npgno, PGNO_INVALID, PGNO_INVALID, 0, P_HASH);
			(void)__memp_fput(mpf,
			    dbc->thread_info, new_pagep, dbc->priority);
		}
		if (temp_pagep != NULL && PGNO(temp_pagep) != bucket_pgno)
			(void)__memp_fput(mpf,
			    dbc->thread_info, temp_pagep, dbc->priority);
	}
	if ((t_ret = __TLPUT(dbc, block)) != 0 && ret == 0)
		ret = t_ret;
	if (carray != NULL)		/* We never knew its size. */
		__os_free(env, carray);
	return (ret);
}

/*
 * Add the given pair to the page.  The page in question may already be
 * held (i.e. it was already gotten).  If it is, then the page is passed
 * in via the pagep parameter.  On return, pagep will contain the page
 * to which we just added something.  This allows us to link overflow
 * pages and return the new page having correctly put the last page.
 *
 * PUBLIC: int __ham_add_el __P((DBC *, const DBT *, const DBT *, int));
 */
int
__ham_add_el(dbc, key, val, type)
	DBC *dbc;
	const DBT *key, *val;
	int type;
{
	const DBT *pkey, *pdata;
	DB *dbp;
	DBT key_dbt, data_dbt;
	DB_LSN new_lsn;
	DB_MPOOLFILE *mpf;
	HASH_CURSOR *hcp;
	HOFFPAGE doff, koff;
	db_pgno_t next_pgno, pgno;
	u_int32_t data_size, key_size;
	u_int32_t pages, pagespace, pairsize, rectype;
	int do_expand, data_type, is_keybig, is_databig, key_type, match, ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	hcp = (HASH_CURSOR *)dbc->internal;
	do_expand = 0;

	pgno = hcp->seek_found_page != PGNO_INVALID ?
	    hcp->seek_found_page : hcp->pgno;
	if (hcp->page == NULL && (ret = __memp_fget(mpf, &pgno,
	    dbc->thread_info, dbc->txn, DB_MPOOL_CREATE, &hcp->page)) != 0)
		return (ret);

	key_size = HKEYDATA_PSIZE(key->size);
	data_size = HKEYDATA_PSIZE(val->size);
	is_keybig = ISBIG(hcp, key->size);
	is_databig = ISBIG(hcp, val->size);
	if (is_keybig)
		key_size = HOFFPAGE_PSIZE;
	if (is_databig)
		data_size = HOFFPAGE_PSIZE;

	pairsize = key_size + data_size;

	/* Advance to first page in chain with room for item. */
	while (H_NUMPAIRS(hcp->page) && NEXT_PGNO(hcp->page) != PGNO_INVALID) {
		/*
		 * This may not be the end of the chain, but the pair may fit
		 * anyway.  Check if it's a bigpair that fits or a regular
		 * pair that fits.
		 */
		if (P_FREESPACE(dbp, hcp->page) >= pairsize)
			break;
		next_pgno = NEXT_PGNO(hcp->page);
		if ((ret = __ham_next_cpage(dbc, next_pgno)) != 0)
			return (ret);
	}

	/*
	 * Check if we need to allocate a new page.
	 */
	if (P_FREESPACE(dbp, hcp->page) < pairsize) {
		do_expand = 1;
		if ((ret = __memp_dirty(mpf, &hcp->page,
		    dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
			return (ret);
		if ((ret = __ham_add_ovflpage(dbc,
		    (PAGE *)hcp->page, 1, (PAGE **)&hcp->page)) != 0)
			return (ret);
		hcp->pgno = PGNO(hcp->page);
	}

	/*
	 * If we don't have a transaction then make sure we will not
	 * run out of file space before updating the key or data.
	 */
	if (dbc->txn == NULL &&
	    dbp->mpf->mfp->maxpgno != 0 && (is_keybig || is_databig)) {
		pagespace = P_MAXSPACE(dbp, dbp->pgsize);
		pages = 0;
		if (is_databig)
			pages = ((data_size - 1) / pagespace) + 1;
		if (is_keybig) {
			pages += ((key->size - 1) / pagespace) + 1;
			if (pages >
			    (dbp->mpf->mfp->maxpgno - dbp->mpf->mfp->last_pgno))
				return (__db_space_err(dbp));
		}
	}

	if ((ret = __memp_dirty(mpf,
	    &hcp->page, dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
		return (ret);

	/*
	 * Update cursor.
	 */
	hcp->indx = hcp->seek_found_indx;
	F_CLR(hcp, H_DELETED);
	if (is_keybig) {
		koff.type = H_OFFPAGE;
		UMRW_SET(koff.unused[0]);
		UMRW_SET(koff.unused[1]);
		UMRW_SET(koff.unused[2]);
		if ((ret = __db_poff(dbc, key, &koff.pgno)) != 0)
			return (ret);
		koff.tlen = key->size;
		key_dbt.data = &koff;
		key_dbt.size = sizeof(koff);
		pkey = &key_dbt;
		key_type = H_OFFPAGE;
	} else {
		pkey = key;
		key_type = H_KEYDATA;
	}

	if (is_databig) {
		doff.type = H_OFFPAGE;
		UMRW_SET(doff.unused[0]);
		UMRW_SET(doff.unused[1]);
		UMRW_SET(doff.unused[2]);
		if ((ret = __db_poff(dbc, val, &doff.pgno)) != 0)
			return (ret);
		doff.tlen = val->size;
		data_dbt.data = &doff;
		data_dbt.size = sizeof(doff);
		pdata = &data_dbt;
		data_type = H_OFFPAGE;
	} else {
		pdata = val;
		data_type = type;
	}

	/* Sort any unsorted pages before doing the insert. */
	if (((PAGE *)hcp->page)->type == P_HASH_UNSORTED)
		if ((ret = __ham_sort_page_cursor(dbc, hcp->page)) != 0)
			return (ret);

	/*
	 * If inserting on the page found initially, then use the saved index.
	 * If inserting on a different page resolve the index now so it can be
	 * logged.
	 * The page might be different, if P_FREESPACE constraint failed (due
	 * to a partial put that increases the data size).
	 */
	if (PGNO(hcp->page) != hcp->seek_found_page) {
	    if ((ret = __ham_getindex(dbc, hcp->page, pkey,
		    key_type, &match, &hcp->seek_found_indx)) != 0)
			return (ret);
	    hcp->seek_found_page = PGNO(hcp->page);

	    DB_ASSERT(dbp->env, hcp->seek_found_indx <= NUM_ENT(hcp->page));
	}

	if (DBC_LOGGING(dbc)) {
		rectype = PUTPAIR;
		if (is_databig)
			rectype |= PAIR_DATAMASK;
		if (is_keybig)
			rectype |= PAIR_KEYMASK;
		if (type == H_DUPLICATE)
			rectype |= PAIR_DUPMASK;

		if ((ret = __ham_insdel_log(dbp, dbc->txn, &new_lsn, 0,
		    rectype, PGNO(hcp->page), (u_int32_t)hcp->seek_found_indx,
		    &LSN(hcp->page), pkey, pdata)) != 0)
			return (ret);
	} else
		LSN_NOT_LOGGED(new_lsn);

	/* Move lsn onto page. */
	LSN(hcp->page) = new_lsn;	/* Structure assignment. */

	if ((ret = __ham_insertpair(dbc, hcp->page,
	    &hcp->seek_found_indx, pkey, pdata, key_type, data_type)) != 0)
		return (ret);

	/*
	 * Adjust any cursors that were pointing at items whose indices were
	 * shuffled due to the insert.
	 */
	if ((ret = __hamc_update(dbc, pairsize, DB_HAM_CURADJ_ADD, 0)) != 0)
		return (ret);

	/*
	 * For splits, we are going to update item_info's page number
	 * field, so that we can easily return to the same page the
	 * next time we come in here.  For other operations, this doesn't
	 * matter, since this is the last thing that happens before we return
	 * to the user program.
	 */
	hcp->pgno = PGNO(hcp->page);
	/*
	 * When moving an item from one page in a bucket to another, due to an
	 * expanding on page duplicate set, or a partial put that increases the
	 * size of an item. The destination index needs to be saved so that the
	 * __ham_replpair code can update any cursors impacted by the move. For
	 * other operations, this does not matter, since this is the last thing
	 * that happens before we return to the user program.
	 */
	hcp->indx = hcp->seek_found_indx;

	/*
	 * XXX
	 * Maybe keep incremental numbers here.
	 */
	if (!STD_LOCKING(dbc)) {
		if ((ret = __ham_dirty_meta(dbc, 0)) != 0)
			return (ret);
		hcp->hdr->nelem++;
	}

	if (do_expand || (hcp->hdr->ffactor != 0 &&
	    (u_int32_t)H_NUMPAIRS(hcp->page) > hcp->hdr->ffactor))
		F_SET(hcp, H_EXPAND);
	return (0);
}

/*
 * Special insert pair call -- copies a key/data pair from one page to
 * another.  Works for all types of hash entries (H_OFFPAGE, H_KEYDATA,
 * H_DUPLICATE, H_OFFDUP).  Since we log splits at a high level, we
 * do not need to do any logging here.
 *
 * dest_indx is an optional parameter, it serves several purposes:
 * * ignored if NULL
 * * Used as an insert index if non-null and not NDX_INVALID
 * * Populated with the insert index if non-null and NDX_INVALID
 *
 * PUBLIC: int __ham_copypair __P((DBC *, PAGE *, u_int32_t,
 * PUBLIC:     PAGE *, db_indx_t *));
 */
int
__ham_copypair(dbc, src_page, src_ndx, dest_page, dest_indx)
	DBC *dbc;
	PAGE *src_page;
	u_int32_t src_ndx;
	PAGE *dest_page;
	db_indx_t *dest_indx;
{
	DB *dbp;
	DBT tkey, tdata;
	db_indx_t kindx, dindx;
	int ktype, dtype, ret;

	dbp = dbc->dbp;
	ret = 0;
	memset(&tkey, 0, sizeof(tkey));
	memset(&tdata, 0, sizeof(tdata));

	ktype = HPAGE_TYPE(dbp, src_page, H_KEYINDEX(src_ndx));
	dtype = HPAGE_TYPE(dbp, src_page, H_DATAINDEX(src_ndx));
	kindx = H_KEYINDEX(src_ndx);
	dindx = H_DATAINDEX(src_ndx);
	if (ktype == H_OFFPAGE) {
		tkey.data = P_ENTRY(dbp, src_page, kindx);
		tkey.size = LEN_HITEM(dbp, src_page, dbp->pgsize, kindx);
	} else {
		tkey.data = HKEYDATA_DATA(P_ENTRY(dbp, src_page, kindx));
		tkey.size = LEN_HKEYDATA(dbp, src_page, dbp->pgsize, kindx);
	}
	if (dtype == H_OFFPAGE) {
		tdata.data = P_ENTRY(dbp, src_page, dindx);
		tdata.size = LEN_HITEM(dbp, src_page, dbp->pgsize, dindx);
	} else {
		tdata.data = HKEYDATA_DATA(P_ENTRY(dbp, src_page, dindx));
		tdata.size = LEN_HKEYDATA(dbp, src_page, dbp->pgsize, dindx);
	}
	if ((ret = __ham_insertpair(dbc, dest_page, dest_indx,
	    &tkey, &tdata, ktype, dtype)) != 0)
	    return (ret);

	return (ret);
}

/*
 * __ham_add_ovflpage --
 *
 * Returns:
 *	0 on success: pp points to new page; !0 on error, pp not valid.
 *
 * PUBLIC: int __ham_add_ovflpage __P((DBC *, PAGE *, int, PAGE **));
 */
int
__ham_add_ovflpage(dbc, pagep, release, pp)
	DBC *dbc;
	PAGE *pagep;
	int release;
	PAGE **pp;
{
	DB *dbp;
	DB_LSN new_lsn;
	DB_MPOOLFILE *mpf;
	PAGE *new_pagep;
	int ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;

	DB_ASSERT(dbp->env, IS_DIRTY(pagep));

	if ((ret = __db_new(dbc, P_HASH, NULL, &new_pagep)) != 0)
		return (ret);

	if (DBC_LOGGING(dbc)) {
		if ((ret = __ham_newpage_log(dbp, dbc->txn, &new_lsn, 0,
		    PUTOVFL, PGNO(pagep), &LSN(pagep), PGNO(new_pagep),
		    &LSN(new_pagep), PGNO_INVALID, NULL)) != 0) {
			(void)__memp_fput(mpf,
			    dbc->thread_info, pagep, dbc->priority);
			return (ret);
		}
	} else
		LSN_NOT_LOGGED(new_lsn);

	/* Move lsn onto page. */
	LSN(pagep) = LSN(new_pagep) = new_lsn;
	NEXT_PGNO(pagep) = PGNO(new_pagep);

	PREV_PGNO(new_pagep) = PGNO(pagep);

	if (release)
		ret = __memp_fput(mpf, dbc->thread_info, pagep, dbc->priority);

	*pp = new_pagep;
	return (ret);
}

/*
 * PUBLIC: int __ham_get_cpage __P((DBC *, db_lockmode_t));
 */
int
__ham_get_cpage(dbc, mode)
	DBC *dbc;
	db_lockmode_t mode;
{
	DB *dbp;
	DB_LOCK tmp_lock;
	DB_MPOOLFILE *mpf;
	HASH_CURSOR *hcp;
	int ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	hcp = (HASH_CURSOR *)dbc->internal;
	ret = 0;

	/*
	 * There are four cases with respect to buckets and locks.
	 * 1. If there is no lock held, then if we are locking, we should
	 *    get the lock.
	 * 2. If there is a lock held, it's for the current bucket, and it's
	 *    for the right mode, we don't need to do anything.
	 * 3. If there is a lock held for the current bucket but it's not
	 *    strong enough, we need to upgrade.
	 * 4. If there is a lock, but it's for a different bucket, then we need
	 *    to release the existing lock and get a new lock.
	 */
	LOCK_INIT(tmp_lock);
	if (STD_LOCKING(dbc)) {
		if (hcp->lbucket != hcp->bucket) {	/* Case 4 */
			if ((ret = __TLPUT(dbc, hcp->lock)) != 0)
				return (ret);
			LOCK_INIT(hcp->lock);
			hcp->stream_start_pgno = PGNO_INVALID;
		}

		/*
		 * See if we have the right lock.  If we are doing
		 * dirty reads we assume the write lock has been downgraded.
		 */
		if ((LOCK_ISSET(hcp->lock) &&
		    ((hcp->lock_mode == DB_LOCK_READ ||
		    F_ISSET(dbp, DB_AM_READ_UNCOMMITTED)) &&
		    mode == DB_LOCK_WRITE))) {
			/* Case 3. */
			tmp_lock = hcp->lock;
			LOCK_INIT(hcp->lock);
		}

		/* Acquire the lock. */
		if (!LOCK_ISSET(hcp->lock))
			/* Cases 1, 3, and 4. */
			if ((ret = __ham_lock_bucket(dbc, mode)) != 0)
				return (ret);

		if (ret == 0) {
			hcp->lock_mode = mode;
			hcp->lbucket = hcp->bucket;
			/* Case 3: release the original lock. */
			if ((ret = __ENV_LPUT(dbp->env, tmp_lock)) != 0)
				return (ret);
		} else if (LOCK_ISSET(tmp_lock))
			hcp->lock = tmp_lock;
	}

	if (ret == 0 && hcp->page == NULL) {
		if (hcp->pgno == PGNO_INVALID)
			hcp->pgno = BUCKET_TO_PAGE(hcp, hcp->bucket);
		if ((ret = __memp_fget(mpf,
		    &hcp->pgno, dbc->thread_info, dbc->txn,
		    (mode == DB_LOCK_WRITE ? DB_MPOOL_DIRTY : 0) |
		    DB_MPOOL_CREATE, &hcp->page)) != 0)
			return (ret);
	}
	return (0);
}

/*
 * Get a new page at the cursor, putting the last page if necessary.
 * If the flag is set to H_ISDUP, then we are talking about the
 * duplicate page, not the main page.
 *
 * PUBLIC: int __ham_next_cpage __P((DBC *, db_pgno_t));
 */
int
__ham_next_cpage(dbc, pgno)
	DBC *dbc;
	db_pgno_t pgno;
{
	DB *dbp;
	DB_MPOOLFILE *mpf;
	HASH_CURSOR *hcp;
	PAGE *p;
	int ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	hcp = (HASH_CURSOR *)dbc->internal;

	if (hcp->page != NULL && (ret = __memp_fput(mpf,
	    dbc->thread_info, hcp->page, dbc->priority)) != 0)
		return (ret);
	hcp->stream_start_pgno = PGNO_INVALID;
	hcp->page = NULL;

	if ((ret = __memp_fget(mpf, &pgno, dbc->thread_info, dbc->txn,
	    DB_MPOOL_CREATE, &p)) != 0)
		return (ret);

	hcp->page = p;
	hcp->pgno = pgno;
	hcp->indx = 0;

	return (0);
}

/*
 * __ham_lock_bucket --
 *	Get the lock on a particular bucket.
 *
 * PUBLIC: int __ham_lock_bucket __P((DBC *, db_lockmode_t));
 */
int
__ham_lock_bucket(dbc, mode)
	DBC *dbc;
	db_lockmode_t mode;
{
	HASH_CURSOR *hcp;
	db_pgno_t pgno;
	int gotmeta, ret;

	hcp = (HASH_CURSOR *)dbc->internal;
	gotmeta = hcp->hdr == NULL ? 1 : 0;
	if (gotmeta)
		if ((ret = __ham_get_meta(dbc)) != 0)
			return (ret);
	pgno = BUCKET_TO_PAGE(hcp, hcp->bucket);
	if (gotmeta)
		if ((ret = __ham_release_meta(dbc)) != 0)
			return (ret);

	ret = __db_lget(dbc, 0, pgno, mode, 0, &hcp->lock);

	hcp->lock_mode = mode;
	return (ret);
}

/*
 * __ham_dpair --
 *	Delete a pair on a page, paying no attention to what the pair
 *	represents.  The caller is responsible for freeing up duplicates
 *	or offpage entries that might be referenced by this pair.
 *
 *	Recovery assumes that this may be called without the metadata
 *	page pinned.
 *
 * PUBLIC: void __ham_dpair __P((DB *, PAGE *, u_int32_t));
 */
void
__ham_dpair(dbp, p, indx)
	DB *dbp;
	PAGE *p;
	u_int32_t indx;
{
	db_indx_t delta, n, *inp;
	u_int8_t *dest, *src;

	inp = P_INP(dbp, p);
	/*
	 * Compute "delta", the amount we have to shift all of the
	 * offsets.  To find the delta, we just need to calculate
	 * the size of the pair of elements we are removing.
	 */
	delta = H_PAIRSIZE(dbp, p, dbp->pgsize, indx);

	/*
	 * The hard case: we want to remove something other than
	 * the last item on the page.  We need to shift data and
	 * offsets down.
	 */
	if ((db_indx_t)indx != NUM_ENT(p) - 2) {
		/*
		 * Move the data: src is the first occupied byte on
		 * the page. (Length is delta.)
		 */
		src = (u_int8_t *)p + HOFFSET(p);

		/*
		 * Destination is delta bytes beyond src.  This might
		 * be an overlapping copy, so we have to use memmove.
		 */
		dest = src + delta;
		memmove(dest, src, inp[H_DATAINDEX(indx)] - HOFFSET(p));
	}

	/* Adjust page metadata. */
	HOFFSET(p) = HOFFSET(p) + delta;
	NUM_ENT(p) = NUM_ENT(p) - 2;

	/* Adjust the offsets. */
	for (n = (db_indx_t)indx; n < (db_indx_t)(NUM_ENT(p)); n++)
		inp[n] = inp[n + 2] + delta;

}

/*
 * __hamc_delpg --
 *
 * Adjust the cursors after we've emptied a page in a bucket, taking
 * care that when we move cursors pointing to deleted items, their
 * orders don't collide with the orders of cursors on the page we move
 * them to (since after this function is called, cursors with the same
 * index on the two pages will be otherwise indistinguishable--they'll
 * all have pgno new_pgno).  There are three cases:
 *
 *	1) The emptied page is the first page in the bucket.  In this
 *	case, we've copied all the items from the second page into the
 *	first page, so the first page is new_pgno and the second page is
 *	old_pgno.  new_pgno is empty, but can have deleted cursors
 *	pointing at indx 0, so we need to be careful of the orders
 *	there.  This is DB_HAM_DELFIRSTPG.
 *
 *	2) The page is somewhere in the middle of a bucket.  Our caller
 *	can just delete such a page, so it's old_pgno.  old_pgno is
 *	empty, but may have deleted cursors pointing at indx 0, so we
 *	need to be careful of indx 0 when we move those cursors to
 *	new_pgno.  This is DB_HAM_DELMIDPG.
 *
 *	3) The page is the last in a bucket.  Again the empty page is
 *	old_pgno, and again it should only have cursors that are deleted
 *	and at indx == 0.  This time, though, there's no next page to
 *	move them to, so we set them to indx == num_ent on the previous
 *	page--and indx == num_ent is the index whose cursors we need to
 *	be careful of.  This is DB_HAM_DELLASTPG.
 */
static int
__hamc_delpg(dbc, old_pgno, new_pgno, num_ent, op, orderp)
	DBC *dbc;
	db_pgno_t old_pgno, new_pgno;
	u_int32_t num_ent;
	db_ham_mode op;
	u_int32_t *orderp;
{
	DB *dbp, *ldbp;
	DBC *cp;
	DB_LSN lsn;
	DB_TXN *my_txn;
	ENV *env;
	HASH_CURSOR *hcp;
	db_indx_t indx;
	u_int32_t order;
	int found, ret;

	/* Which is the worrisome index? */
	indx = (op == DB_HAM_DELLASTPG) ? num_ent : 0;

	dbp = dbc->dbp;
	env = dbp->env;

	my_txn = IS_SUBTRANSACTION(dbc->txn) ? dbc->txn : NULL;
	MUTEX_LOCK(env, env->mtx_dblist);
	/*
	 * Find the highest order of any cursor our movement
	 * may collide with.
	 */
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (order = 1;
	    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(cp, &ldbp->active_queue, links) {
			if (cp == dbc || cp->dbtype != DB_HASH)
				continue;
			hcp = (HASH_CURSOR *)cp->internal;
			if (hcp->pgno == new_pgno &&
			    !MVCC_SKIP_CURADJ(cp, new_pgno)) {
				if (hcp->indx == indx &&
				    F_ISSET(hcp, H_DELETED) &&
				    hcp->order >= order)
					order = hcp->order + 1;
				DB_ASSERT(env, op != DB_HAM_DELFIRSTPG ||
				    hcp->indx == NDX_INVALID ||
				    (hcp->indx == 0 &&
				    F_ISSET(hcp, H_DELETED)));
			}
		}
		MUTEX_UNLOCK(env, dbp->mutex);
	}

	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (found = 0;
	    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(cp, &ldbp->active_queue, links) {
			if (cp == dbc || cp->dbtype != DB_HASH)
				continue;

			hcp = (HASH_CURSOR *)cp->internal;

			if (hcp->pgno == old_pgno &&
			    !MVCC_SKIP_CURADJ(cp, old_pgno)) {
				switch (op) {
				case DB_HAM_DELFIRSTPG:
					/*
					 * We're moving all items,
					 * regardless of index.
					 */
					hcp->pgno = new_pgno;

					/*
					 * But we have to be careful of
					 * the order values.
					 */
					if (hcp->indx == indx)
						hcp->order += order;
					break;
				case DB_HAM_DELMIDPG:
					hcp->pgno = new_pgno;
					DB_ASSERT(env, hcp->indx == 0 &&
					    F_ISSET(hcp, H_DELETED));
					hcp->order += order;
					break;
				case DB_HAM_DELLASTPG:
					hcp->pgno = new_pgno;
					DB_ASSERT(env, hcp->indx == 0 &&
					    F_ISSET(hcp, H_DELETED));
					hcp->indx = indx;
					hcp->order += order;
					break;
				default:
					return (__db_unknown_path(
					    env, "__hamc_delpg"));
				}
				if (my_txn != NULL && cp->txn != my_txn)
					found = 1;
			}
		}
		MUTEX_UNLOCK(env, dbp->mutex);
	}
	MUTEX_UNLOCK(env, env->mtx_dblist);

	if (found != 0 && DBC_LOGGING(dbc)) {
		if ((ret = __ham_chgpg_log(dbp, my_txn, &lsn, 0, op,
		    old_pgno, new_pgno, indx, order)) != 0)
			return (ret);
	}
	*orderp = order;
	return (0);
}
