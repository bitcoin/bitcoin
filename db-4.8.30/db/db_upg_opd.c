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

static int __db_build_bi __P((DB *, DB_FH *, PAGE *, PAGE *, u_int32_t, int *));
static int __db_build_ri __P((DB *, DB_FH *, PAGE *, PAGE *, u_int32_t, int *));
static int __db_up_ovref __P((DB *, DB_FH *, db_pgno_t));

#define	GET_PAGE(dbp, fhp, pgno, page) {				\
	if ((ret = __os_seek(						\
	    dbp->env, fhp, pgno, (dbp)->pgsize, 0)) != 0)		\
		goto err;						\
	if ((ret = __os_read(dbp->env,				\
	    fhp, page, (dbp)->pgsize, &n)) != 0)			\
		goto err;						\
}
#define	PUT_PAGE(dbp, fhp, pgno, page) {				\
	if ((ret = __os_seek(						\
	    dbp->env, fhp, pgno, (dbp)->pgsize, 0)) != 0)		\
		goto err;						\
	if ((ret = __os_write(dbp->env,				\
	    fhp, page, (dbp)->pgsize, &n)) != 0)			\
		goto err;						\
}

/*
 * __db_31_offdup --
 *	Convert 3.0 off-page duplicates to 3.1 off-page duplicates.
 *
 * PUBLIC: int __db_31_offdup __P((DB *, char *, DB_FH *, int, db_pgno_t *));
 */
int
__db_31_offdup(dbp, real_name, fhp, sorted, pgnop)
	DB *dbp;
	char *real_name;
	DB_FH *fhp;
	int sorted;
	db_pgno_t *pgnop;
{
	PAGE *ipage, *page;
	db_indx_t indx;
	db_pgno_t cur_cnt, i, next_cnt, pgno, *pgno_cur, pgno_last;
	db_pgno_t *pgno_next, pgno_max, *tmp;
	db_recno_t nrecs;
	size_t n;
	int level, nomem, ret;

	ipage = page = NULL;
	pgno_cur = pgno_next = NULL;

	/* Allocate room to hold a page. */
	if ((ret = __os_malloc(dbp->env, dbp->pgsize, &page)) != 0)
		goto err;

	/*
	 * Walk the chain of 3.0 off-page duplicates.  Each one is converted
	 * in place to a 3.1 off-page duplicate page.  If the duplicates are
	 * sorted, they are converted to a Btree leaf page, otherwise to a
	 * Recno leaf page.
	 */
	for (nrecs = 0, cur_cnt = pgno_max = 0,
	    pgno = *pgnop; pgno != PGNO_INVALID;) {
		if (pgno_max == cur_cnt) {
			pgno_max += 20;
			if ((ret = __os_realloc(dbp->env, pgno_max *
			    sizeof(db_pgno_t), &pgno_cur)) != 0)
				goto err;
		}
		pgno_cur[cur_cnt++] = pgno;

		GET_PAGE(dbp, fhp, pgno, page);
		nrecs += NUM_ENT(page);
		LEVEL(page) = LEAFLEVEL;
		TYPE(page) = sorted ? P_LDUP : P_LRECNO;
		/*
		 * !!!
		 * DB didn't zero the LSNs on off-page duplicates pages.
		 */
		ZERO_LSN(LSN(page));
		PUT_PAGE(dbp, fhp, pgno, page);

		pgno = NEXT_PGNO(page);
	}

	/* If we only have a single page, it's easy. */
	if (cur_cnt <= 1)
		goto done;

	/*
	 * pgno_cur is the list of pages we just converted.  We're
	 * going to walk that list, but we'll need to create a new
	 * list while we do so.
	 */
	if ((ret = __os_malloc(dbp->env,
	    cur_cnt * sizeof(db_pgno_t), &pgno_next)) != 0)
		goto err;

	/* Figure out where we can start allocating new pages. */
	if ((ret = __db_lastpgno(dbp, real_name, fhp, &pgno_last)) != 0)
		goto err;

	/* Allocate room for an internal page. */
	if ((ret = __os_malloc(dbp->env, dbp->pgsize, &ipage)) != 0)
		goto err;
	PGNO(ipage) = PGNO_INVALID;

	/*
	 * Repeatedly walk the list of pages, building internal pages, until
	 * there's only one page at a level.
	 */
	for (level = LEAFLEVEL + 1; cur_cnt > 1; ++level) {
		for (indx = 0, i = next_cnt = 0; i < cur_cnt;) {
			if (indx == 0) {
				P_INIT(ipage, dbp->pgsize, pgno_last,
				    PGNO_INVALID, PGNO_INVALID,
				    level, sorted ? P_IBTREE : P_IRECNO);
				ZERO_LSN(LSN(ipage));

				pgno_next[next_cnt++] = pgno_last++;
			}

			GET_PAGE(dbp, fhp, pgno_cur[i], page);

			/*
			 * If the duplicates are sorted, put the first item on
			 * the lower-level page onto a Btree internal page. If
			 * the duplicates are not sorted, create an internal
			 * Recno structure on the page.  If either case doesn't
			 * fit, push out the current page and start a new one.
			 */
			nomem = 0;
			if (sorted) {
				if ((ret = __db_build_bi(
				    dbp, fhp, ipage, page, indx, &nomem)) != 0)
					goto err;
			} else
				if ((ret = __db_build_ri(
				    dbp, fhp, ipage, page, indx, &nomem)) != 0)
					goto err;
			if (nomem) {
				indx = 0;
				PUT_PAGE(dbp, fhp, PGNO(ipage), ipage);
			} else {
				++indx;
				++NUM_ENT(ipage);
				++i;
			}
		}

		/*
		 * Push out the last internal page.  Set the top-level record
		 * count if we've reached the top.
		 */
		if (next_cnt == 1)
			RE_NREC_SET(ipage, nrecs);
		PUT_PAGE(dbp, fhp, PGNO(ipage), ipage);

		/* Swap the current and next page number arrays. */
		cur_cnt = next_cnt;
		tmp = pgno_cur;
		pgno_cur = pgno_next;
		pgno_next = tmp;
	}

done:	*pgnop = pgno_cur[0];

err:	if (pgno_cur != NULL)
		__os_free(dbp->env, pgno_cur);
	if (pgno_next != NULL)
		__os_free(dbp->env, pgno_next);
	if (ipage != NULL)
		__os_free(dbp->env, ipage);
	if (page != NULL)
		__os_free(dbp->env, page);

	return (ret);
}

/*
 * __db_build_bi --
 *	Build a BINTERNAL entry for a parent page.
 */
static int
__db_build_bi(dbp, fhp, ipage, page, indx, nomemp)
	DB *dbp;
	DB_FH *fhp;
	PAGE *ipage, *page;
	u_int32_t indx;
	int *nomemp;
{
	BINTERNAL bi, *child_bi;
	BKEYDATA *child_bk;
	u_int8_t *p;
	int ret;
	db_indx_t *inp;

	inp = P_INP(dbp, ipage);
	switch (TYPE(page)) {
	case P_IBTREE:
		child_bi = GET_BINTERNAL(dbp, page, 0);
		if (P_FREESPACE(dbp, ipage) < BINTERNAL_PSIZE(child_bi->len)) {
			*nomemp = 1;
			return (0);
		}
		inp[indx] =
		    HOFFSET(ipage) -= BINTERNAL_SIZE(child_bi->len);
		p = P_ENTRY(dbp, ipage, indx);

		bi.len = child_bi->len;
		B_TSET(bi.type, child_bi->type);
		bi.pgno = PGNO(page);
		bi.nrecs = __bam_total(dbp, page);
		memcpy(p, &bi, SSZA(BINTERNAL, data));
		p += SSZA(BINTERNAL, data);
		memcpy(p, child_bi->data, child_bi->len);

		/* Increment the overflow ref count. */
		if (B_TYPE(child_bi->type) == B_OVERFLOW)
			if ((ret = __db_up_ovref(dbp, fhp,
			    ((BOVERFLOW *)(child_bi->data))->pgno)) != 0)
				return (ret);
		break;
	case P_LDUP:
		child_bk = GET_BKEYDATA(dbp, page, 0);
		switch (B_TYPE(child_bk->type)) {
		case B_KEYDATA:
			if (P_FREESPACE(dbp, ipage) <
			    BINTERNAL_PSIZE(child_bk->len)) {
				*nomemp = 1;
				return (0);
			}
			inp[indx] =
			    HOFFSET(ipage) -= BINTERNAL_SIZE(child_bk->len);
			p = P_ENTRY(dbp, ipage, indx);

			bi.len = child_bk->len;
			B_TSET(bi.type, child_bk->type);
			bi.pgno = PGNO(page);
			bi.nrecs = __bam_total(dbp, page);
			memcpy(p, &bi, SSZA(BINTERNAL, data));
			p += SSZA(BINTERNAL, data);
			memcpy(p, child_bk->data, child_bk->len);
			break;
		case B_OVERFLOW:
			if (P_FREESPACE(dbp, ipage) <
			    BINTERNAL_PSIZE(BOVERFLOW_SIZE)) {
				*nomemp = 1;
				return (0);
			}
			inp[indx] =
			    HOFFSET(ipage) -= BINTERNAL_SIZE(BOVERFLOW_SIZE);
			p = P_ENTRY(dbp, ipage, indx);

			bi.len = BOVERFLOW_SIZE;
			B_TSET(bi.type, child_bk->type);
			bi.pgno = PGNO(page);
			bi.nrecs = __bam_total(dbp, page);
			memcpy(p, &bi, SSZA(BINTERNAL, data));
			p += SSZA(BINTERNAL, data);
			memcpy(p, child_bk, BOVERFLOW_SIZE);

			/* Increment the overflow ref count. */
			if ((ret = __db_up_ovref(dbp, fhp,
			    ((BOVERFLOW *)child_bk)->pgno)) != 0)
				return (ret);
			break;
		default:
			return (__db_pgfmt(dbp->env, PGNO(page)));
		}
		break;
	default:
		return (__db_pgfmt(dbp->env, PGNO(page)));
	}

	return (0);
}

/*
 * __db_build_ri --
 *	Build a RINTERNAL entry for an internal parent page.
 */
static int
__db_build_ri(dbp, fhp, ipage, page, indx, nomemp)
	DB *dbp;
	DB_FH *fhp;
	PAGE *ipage, *page;
	u_int32_t indx;
	int *nomemp;
{
	RINTERNAL ri;
	db_indx_t *inp;

	COMPQUIET(fhp, NULL);
	inp = P_INP(dbp, ipage);
	if (P_FREESPACE(dbp, ipage) < RINTERNAL_PSIZE) {
		*nomemp = 1;
		return (0);
	}

	ri.pgno = PGNO(page);
	ri.nrecs = __bam_total(dbp, page);
	inp[indx] = HOFFSET(ipage) -= RINTERNAL_SIZE;
	memcpy(P_ENTRY(dbp, ipage, indx), &ri, RINTERNAL_SIZE);

	return (0);
}

/*
 * __db_up_ovref --
 *	Increment/decrement the reference count on an overflow page.
 */
static int
__db_up_ovref(dbp, fhp, pgno)
	DB *dbp;
	DB_FH *fhp;
	db_pgno_t pgno;
{
	PAGE *page;
	size_t n;
	int ret;

	/* Allocate room to hold a page. */
	if ((ret = __os_malloc(dbp->env, dbp->pgsize, &page)) != 0)
		return (ret);

	GET_PAGE(dbp, fhp, pgno, page);
	++OV_REF(page);
	PUT_PAGE(dbp, fhp, pgno, page);

err:	__os_free(dbp->env, page);

	return (ret);
}
