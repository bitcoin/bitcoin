/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/partition.h"

#ifdef HAVE_STATISTICS
/*
 * __bam_stat --
 *	Gather/print the btree statistics
 *
 * PUBLIC: int __bam_stat __P((DBC *, void *, u_int32_t));
 */
int
__bam_stat(dbc, spp, flags)
	DBC *dbc;
	void *spp;
	u_int32_t flags;
{
	BTMETA *meta;
	BTREE *t;
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_BTREE_STAT *sp;
	DB_LOCK lock, metalock;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *h;
	db_pgno_t pgno;
	int ret, t_ret, write_meta;

	dbp = dbc->dbp;
	env = dbp->env;

	meta = NULL;
	t = dbp->bt_internal;
	sp = NULL;
	LOCK_INIT(metalock);
	LOCK_INIT(lock);
	mpf = dbp->mpf;
	h = NULL;
	ret = write_meta = 0;

	cp = (BTREE_CURSOR *)dbc->internal;

	/* Allocate and clear the structure. */
	if ((ret = __os_umalloc(env, sizeof(*sp), &sp)) != 0)
		goto err;
	memset(sp, 0, sizeof(*sp));

	/* Get the metadata page for the entire database. */
	pgno = PGNO_BASE_MD;
	if ((ret = __db_lget(dbc, 0, pgno, DB_LOCK_READ, 0, &metalock)) != 0)
		goto err;
	if ((ret = __memp_fget(mpf, &pgno,
	     dbc->thread_info, dbc->txn, 0, &meta)) != 0)
		goto err;

	if (flags == DB_FAST_STAT)
		goto meta_only;

	/* Walk the metadata free list, counting pages. */
	for (sp->bt_free = 0, pgno = meta->dbmeta.free; pgno != PGNO_INVALID;) {
		++sp->bt_free;

		if ((ret = __memp_fget(mpf, &pgno,
		     dbc->thread_info, dbc->txn, 0, &h)) != 0)
			goto err;

		pgno = h->next_pgno;
		if ((ret = __memp_fput(mpf,
		    dbc->thread_info, h, dbc->priority)) != 0)
			goto err;
		h = NULL;
	}

	/* Get the root page. */
	pgno = cp->root;
	if ((ret = __db_lget(dbc, 0, pgno, DB_LOCK_READ, 0, &lock)) != 0)
		goto err;
	if ((ret = __memp_fget(mpf, &pgno,
	     dbc->thread_info, dbc->txn, 0, &h)) != 0)
		goto err;

	/* Get the levels from the root page. */
	sp->bt_levels = h->level;

	/* Discard the root page. */
	ret = __memp_fput(mpf, dbc->thread_info, h, dbc->priority);
	h = NULL;
	if ((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;
	if (ret != 0)
		goto err;

	/* Walk the tree. */
	if ((ret = __bam_traverse(dbc,
	    DB_LOCK_READ, cp->root, __bam_stat_callback, sp)) != 0)
		goto err;

#ifdef HAVE_COMPRESSION
	if (DB_IS_COMPRESSED(dbp) && (ret = __bam_compress_count(dbc,
	    &sp->bt_nkeys, &sp->bt_ndata)) != 0)
		goto err;
#endif

	/*
	 * Get the subdatabase metadata page if it's not the same as the
	 * one we already have.
	 */
	write_meta = !F_ISSET(dbp, DB_AM_RDONLY) &&
	    (!MULTIVERSION(dbp) || dbc->txn != NULL);
meta_only:
	if (t->bt_meta != PGNO_BASE_MD || write_meta) {
		ret = __memp_fput(mpf, dbc->thread_info, meta, dbc->priority);
		meta = NULL;
		if ((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			goto err;

		if ((ret = __db_lget(dbc,
		    0, t->bt_meta, write_meta ? DB_LOCK_WRITE : DB_LOCK_READ,
		    0, &metalock)) != 0)
			goto err;
		if ((ret = __memp_fget(mpf, &t->bt_meta,
		     dbc->thread_info, dbc->txn,
		    write_meta ? DB_MPOOL_DIRTY : 0, &meta)) != 0)
			goto err;
	}
	if (flags == DB_FAST_STAT) {
		if (dbp->type == DB_RECNO ||
		    (dbp->type == DB_BTREE && F_ISSET(dbp, DB_AM_RECNUM))) {
			if ((ret = __db_lget(dbc, 0,
			    cp->root, DB_LOCK_READ, 0, &lock)) != 0)
				goto err;
			if ((ret = __memp_fget(mpf, &cp->root,
			     dbc->thread_info, dbc->txn, 0, &h)) != 0)
				goto err;

			sp->bt_nkeys = RE_NREC(h);
		} else
			sp->bt_nkeys = meta->dbmeta.key_count;

		sp->bt_ndata = dbp->type == DB_RECNO ?
		   sp->bt_nkeys : meta->dbmeta.record_count;
	}

	/* Get metadata page statistics. */
	sp->bt_metaflags = meta->dbmeta.flags;
	sp->bt_minkey = meta->minkey;
	sp->bt_re_len = meta->re_len;
	sp->bt_re_pad = meta->re_pad;
	/*
	 * Don't take the page number from the meta-data page -- that value is
	 * only maintained in the primary database, we may have been called on
	 * a subdatabase.  (Yes, I read the primary database meta-data page
	 * earlier in this function, but I'm asking the underlying cache so the
	 * code for the Hash and Btree methods is the same.)
	 */
	if ((ret = __memp_get_last_pgno(dbp->mpf, &pgno)) != 0)
		goto err;
	sp->bt_pagecnt = pgno + 1;
	sp->bt_pagesize = meta->dbmeta.pagesize;
	sp->bt_magic = meta->dbmeta.magic;
	sp->bt_version = meta->dbmeta.version;

	if (write_meta != 0) {
		meta->dbmeta.key_count = sp->bt_nkeys;
		meta->dbmeta.record_count = sp->bt_ndata;
	}

	*(DB_BTREE_STAT **)spp = sp;

err:	/* Discard the second page. */
	if ((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;
	if (h != NULL && (t_ret = __memp_fput(mpf,
	    dbc->thread_info, h, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;

	/* Discard the metadata page. */
	if ((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
		ret = t_ret;
	if (meta != NULL && (t_ret = __memp_fput(mpf,
	    dbc->thread_info, meta, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;

	if (ret != 0 && sp != NULL) {
		__os_ufree(env, sp);
		*(DB_BTREE_STAT **)spp = NULL;
	}

	return (ret);
}

/*
 * __bam_stat_print --
 *	Display btree/recno statistics.
 *
 * PUBLIC: int __bam_stat_print __P((DBC *, u_int32_t));
 */
int
__bam_stat_print(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	static const FN fn[] = {
		{ BTM_DUP,	"duplicates" },
		{ BTM_RECNO,	"recno" },
		{ BTM_RECNUM,	"record-numbers" },
		{ BTM_FIXEDLEN,	"fixed-length" },
		{ BTM_RENUMBER,	"renumber" },
		{ BTM_SUBDB,	"multiple-databases" },
		{ BTM_DUPSORT,	"sorted duplicates" },
		{ BTM_COMPRESS,	"compressed" },
		{ 0,		NULL }
	};
	DB *dbp;
	DB_BTREE_STAT *sp;
	ENV *env;
	int lorder, ret;
	const char *s;

	dbp = dbc->dbp;
	env = dbp->env;
#ifdef HAVE_PARTITION
	if (DB_IS_PARTITIONED(dbp)) {
		if ((ret = __partition_stat(dbc, &sp, flags)) != 0)
			return (ret);
	} else
#endif
	if ((ret = __bam_stat(dbc, &sp, LF_ISSET(DB_FAST_STAT))) != 0)
		return (ret);

	if (LF_ISSET(DB_STAT_ALL)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Default Btree/Recno database information:");
	}

	__db_msg(env, "%lx\tBtree magic number", (u_long)sp->bt_magic);
	__db_msg(env, "%lu\tBtree version number", (u_long)sp->bt_version);

	(void)__db_get_lorder(dbp, &lorder);
	switch (lorder) {
	case 1234:
		s = "Little-endian";
		break;
	case 4321:
		s = "Big-endian";
		break;
	default:
		s = "Unrecognized byte order";
		break;
	}
	__db_msg(env, "%s\tByte order", s);
	__db_prflags(env, NULL, sp->bt_metaflags, fn, NULL, "\tFlags");
	if (dbp->type == DB_BTREE)
		__db_dl(env, "Minimum keys per-page", (u_long)sp->bt_minkey);
	if (dbp->type == DB_RECNO) {
		__db_dl(env,
		    "Fixed-length record size", (u_long)sp->bt_re_len);
		__db_msg(env,
		    "%#x\tFixed-length record pad", (u_int)sp->bt_re_pad);
	}
	__db_dl(env,
	    "Underlying database page size", (u_long)sp->bt_pagesize);
	if (dbp->type == DB_BTREE)
		__db_dl(env, "Overflow key/data size",
		    ((BTREE_CURSOR *)dbc->internal)->ovflsize);
	__db_dl(env, "Number of levels in the tree", (u_long)sp->bt_levels);
	__db_dl(env, dbp->type == DB_BTREE ?
	    "Number of unique keys in the tree" :
	    "Number of records in the tree", (u_long)sp->bt_nkeys);
	__db_dl(env,
	    "Number of data items in the tree", (u_long)sp->bt_ndata);

	__db_dl(env,
	    "Number of tree internal pages", (u_long)sp->bt_int_pg);
	__db_dl_pct(env,
	    "Number of bytes free in tree internal pages",
	    (u_long)sp->bt_int_pgfree,
	    DB_PCT_PG(sp->bt_int_pgfree, sp->bt_int_pg, sp->bt_pagesize), "ff");

	__db_dl(env,
	    "Number of tree leaf pages", (u_long)sp->bt_leaf_pg);
	__db_dl_pct(env, "Number of bytes free in tree leaf pages",
	    (u_long)sp->bt_leaf_pgfree, DB_PCT_PG(
	    sp->bt_leaf_pgfree, sp->bt_leaf_pg, sp->bt_pagesize), "ff");

	__db_dl(env,
	    "Number of tree duplicate pages", (u_long)sp->bt_dup_pg);
	__db_dl_pct(env,
	    "Number of bytes free in tree duplicate pages",
	    (u_long)sp->bt_dup_pgfree,
	    DB_PCT_PG(sp->bt_dup_pgfree, sp->bt_dup_pg, sp->bt_pagesize), "ff");

	__db_dl(env,
	    "Number of tree overflow pages", (u_long)sp->bt_over_pg);
	__db_dl_pct(env, "Number of bytes free in tree overflow pages",
	    (u_long)sp->bt_over_pgfree, DB_PCT_PG(
	    sp->bt_over_pgfree, sp->bt_over_pg, sp->bt_pagesize), "ff");
	__db_dl(env, "Number of empty pages", (u_long)sp->bt_empty_pg);

	__db_dl(env, "Number of pages on the free list", (u_long)sp->bt_free);

	__os_ufree(env, sp);

	return (0);
}

/*
 * __bam_stat_callback --
 *	Statistics callback.
 *
 * PUBLIC: int __bam_stat_callback __P((DBC *, PAGE *, void *, int *));
 */
int
__bam_stat_callback(dbc, h, cookie, putp)
	DBC *dbc;
	PAGE *h;
	void *cookie;
	int *putp;
{
	DB *dbp;
	DB_BTREE_STAT *sp;
	db_indx_t indx, *inp, top;
	u_int8_t type;

	dbp = dbc->dbp;
	sp = cookie;
	*putp = 0;
	top = NUM_ENT(h);
	inp = P_INP(dbp, h);

	switch (TYPE(h)) {
	case P_IBTREE:
	case P_IRECNO:
		++sp->bt_int_pg;
		sp->bt_int_pgfree += P_FREESPACE(dbp, h);
		break;
	case P_LBTREE:
		if (top == 0)
			++sp->bt_empty_pg;

		/* Correct for on-page duplicates and deleted items. */
		for (indx = 0; indx < top; indx += P_INDX) {
			type = GET_BKEYDATA(dbp, h, indx + O_INDX)->type;
			/* Ignore deleted items. */
			if (B_DISSET(type))
				continue;

			/* Ignore duplicate keys. */
			if (indx + P_INDX >= top ||
			    inp[indx] != inp[indx + P_INDX])
				++sp->bt_nkeys;

			/* Ignore off-page duplicates. */
			if (B_TYPE(type) != B_DUPLICATE)
				++sp->bt_ndata;
		}

		++sp->bt_leaf_pg;
		sp->bt_leaf_pgfree += P_FREESPACE(dbp, h);
		break;
	case P_LRECNO:
		if (top == 0)
			++sp->bt_empty_pg;

		/*
		 * If walking a recno tree, then each of these items is a key.
		 * Otherwise, we're walking an off-page duplicate set.
		 */
		if (dbp->type == DB_RECNO) {
			/*
			 * Correct for deleted items in non-renumbering Recno
			 * databases.
			 */
			if (F_ISSET(dbp, DB_AM_RENUMBER)) {
				sp->bt_nkeys += top;
				sp->bt_ndata += top;
			} else
				for (indx = 0; indx < top; indx += O_INDX) {
					type = GET_BKEYDATA(dbp, h, indx)->type;
					if (!B_DISSET(type)) {
						++sp->bt_ndata;
						++sp->bt_nkeys;
					}
				}

			++sp->bt_leaf_pg;
			sp->bt_leaf_pgfree += P_FREESPACE(dbp, h);
		} else {
			sp->bt_ndata += top;

			++sp->bt_dup_pg;
			sp->bt_dup_pgfree += P_FREESPACE(dbp, h);
		}
		break;
	case P_LDUP:
		if (top == 0)
			++sp->bt_empty_pg;

		/* Correct for deleted items. */
		for (indx = 0; indx < top; indx += O_INDX)
			if (!B_DISSET(GET_BKEYDATA(dbp, h, indx)->type))
				++sp->bt_ndata;

		++sp->bt_dup_pg;
		sp->bt_dup_pgfree += P_FREESPACE(dbp, h);
		break;
	case P_OVERFLOW:
		++sp->bt_over_pg;
		sp->bt_over_pgfree += P_OVFLSPACE(dbp, dbp->pgsize, h);
		break;
	default:
		return (__db_pgfmt(dbp->env, h->pgno));
	}
	return (0);
}

/*
 * __bam_print_cursor --
 *	Display the current internal cursor.
 *
 * PUBLIC: void __bam_print_cursor __P((DBC *));
 */
void
__bam_print_cursor(dbc)
	DBC *dbc;
{
	static const FN fn[] = {
		{ C_DELETED,	"C_DELETED" },
		{ C_RECNUM,	"C_RECNUM" },
		{ C_RENUMBER,	"C_RENUMBER" },
		{ 0,		NULL }
	};
	ENV *env;
	BTREE_CURSOR *cp;

	env = dbc->env;
	cp = (BTREE_CURSOR *)dbc->internal;

	STAT_ULONG("Overflow size", cp->ovflsize);
	if (dbc->dbtype == DB_RECNO)
		STAT_ULONG("Recno", cp->recno);
	STAT_ULONG("Order", cp->order);
	__db_prflags(env, NULL, cp->flags, fn, NULL, "\tInternal Flags");
}

#else /* !HAVE_STATISTICS */

int
__bam_stat(dbc, spp, flags)
	DBC *dbc;
	void *spp;
	u_int32_t flags;
{
	COMPQUIET(spp, NULL);
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbc->env));
}

int
__bam_stat_print(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbc->env));
}
#endif

#ifndef HAVE_BREW
/*
 * __bam_key_range --
 *	Return proportion of keys relative to given key.  The numbers are
 *	slightly skewed due to on page duplicates.
 *
 * PUBLIC: int __bam_key_range __P((DBC *, DBT *, DB_KEY_RANGE *, u_int32_t));
 */
int
__bam_key_range(dbc, dbt, kp, flags)
	DBC *dbc;
	DBT *dbt;
	DB_KEY_RANGE *kp;
	u_int32_t flags;
{
	BTREE_CURSOR *cp;
	EPG *sp;
	double factor;
	int exact, ret;

	COMPQUIET(flags, 0);

	if ((ret = __bam_search(dbc, PGNO_INVALID,
	    dbt, SR_STK_ONLY, 1, NULL, &exact)) != 0)
		return (ret);

	cp = (BTREE_CURSOR *)dbc->internal;
	kp->less = kp->greater = 0.0;

	factor = 1.0;

	/* Correct the leaf page. */
	cp->csp->entries /= 2;
	cp->csp->indx /= 2;
	for (sp = cp->sp; sp <= cp->csp; ++sp) {
		/*
		 * At each level we know that pages greater than indx contain
		 * keys greater than what we are looking for and those less
		 * than indx are less than.  The one pointed to by indx may
		 * have some less, some greater or even equal.  If indx is
		 * equal to the number of entries, then the key is out of range
		 * and everything is less.
		 */
		if (sp->indx == 0)
			kp->greater += factor * (sp->entries - 1)/sp->entries;
		else if (sp->indx == sp->entries)
			kp->less += factor;
		else {
			kp->less += factor * sp->indx / sp->entries;
			kp->greater += factor *
			    ((sp->entries - sp->indx) - 1) / sp->entries;
		}
		factor *= 1.0/sp->entries;
	}

	/*
	 * If there was an exact match then assign 1 n'th to the key itself.
	 * Otherwise that factor belongs to those greater than the key, unless
	 * the key was out of range.
	 */
	if (exact)
		kp->equal = factor;
	else {
		if (kp->less != 1)
			kp->greater += factor;
		kp->equal = 0;
	}

	BT_STK_CLR(cp);

	return (0);
}
#endif

/*
 * __bam_traverse --
 *	Walk a Btree database.
 *
 * PUBLIC: int __bam_traverse __P((DBC *, db_lockmode_t,
 * PUBLIC:     db_pgno_t, int (*)(DBC *, PAGE *, void *, int *), void *));
 */
int
__bam_traverse(dbc, mode, root_pgno, callback, cookie)
	DBC *dbc;
	db_lockmode_t mode;
	db_pgno_t root_pgno;
	int (*callback)__P((DBC *, PAGE *, void *, int *));
	void *cookie;
{
	BINTERNAL *bi;
	BKEYDATA *bk;
	DB *dbp;
	DB_LOCK lock;
	DB_MPOOLFILE *mpf;
	PAGE *h;
	RINTERNAL *ri;
	db_indx_t indx, *inp;
	int already_put, ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	already_put = 0;

	if ((ret = __db_lget(dbc, 0, root_pgno, mode, 0, &lock)) != 0)
		return (ret);
	if ((ret = __memp_fget(mpf, &root_pgno,
	     dbc->thread_info, dbc->txn, 0, &h)) != 0) {
		(void)__TLPUT(dbc, lock);
		return (ret);
	}

	switch (TYPE(h)) {
	case P_IBTREE:
		for (indx = 0; indx < NUM_ENT(h); indx += O_INDX) {
			bi = GET_BINTERNAL(dbp, h, indx);
			if (B_TYPE(bi->type) == B_OVERFLOW &&
			    (ret = __db_traverse_big(dbc,
			    ((BOVERFLOW *)bi->data)->pgno,
			    callback, cookie)) != 0)
				goto err;
			if ((ret = __bam_traverse(
			    dbc, mode, bi->pgno, callback, cookie)) != 0)
				goto err;
		}
		break;
	case P_IRECNO:
		for (indx = 0; indx < NUM_ENT(h); indx += O_INDX) {
			ri = GET_RINTERNAL(dbp, h, indx);
			if ((ret = __bam_traverse(
			    dbc, mode, ri->pgno, callback, cookie)) != 0)
				goto err;
		}
		break;
	case P_LBTREE:
		inp = P_INP(dbp, h);
		for (indx = 0; indx < NUM_ENT(h); indx += P_INDX) {
			bk = GET_BKEYDATA(dbp, h, indx);
			if (B_TYPE(bk->type) == B_OVERFLOW &&
			    (indx + P_INDX >= NUM_ENT(h) ||
			    inp[indx] != inp[indx + P_INDX])) {
				if ((ret = __db_traverse_big(dbc,
				    GET_BOVERFLOW(dbp, h, indx)->pgno,
				    callback, cookie)) != 0)
					goto err;
			}
			bk = GET_BKEYDATA(dbp, h, indx + O_INDX);
			if (B_TYPE(bk->type) == B_DUPLICATE &&
			    (ret = __bam_traverse(dbc, mode,
			    GET_BOVERFLOW(dbp, h, indx + O_INDX)->pgno,
			    callback, cookie)) != 0)
				goto err;
			if (B_TYPE(bk->type) == B_OVERFLOW &&
			    (ret = __db_traverse_big(dbc,
			    GET_BOVERFLOW(dbp, h, indx + O_INDX)->pgno,
			    callback, cookie)) != 0)
				goto err;
		}
		break;
	case P_LDUP:
	case P_LRECNO:
		for (indx = 0; indx < NUM_ENT(h); indx += O_INDX) {
			bk = GET_BKEYDATA(dbp, h, indx);
			if (B_TYPE(bk->type) == B_OVERFLOW &&
			    (ret = __db_traverse_big(dbc,
			    GET_BOVERFLOW(dbp, h, indx)->pgno,
			    callback, cookie)) != 0)
				goto err;
		}
		break;
	default:
		return (__db_pgfmt(dbp->env, h->pgno));
	}

	ret = callback(dbc, h, cookie, &already_put);

err:	if (!already_put && (t_ret = __memp_fput(mpf,
	    dbc->thread_info, h, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __TLPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}
