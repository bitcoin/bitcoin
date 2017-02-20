/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_verify.h"
#include "dbinc/db_am.h"
#include "dbinc/mp.h"
#include "dbinc/qam.h"
/*
 * __qam_vrfy_meta --
 *	Verify the queue-specific part of a metadata page.
 *
 * PUBLIC: int __qam_vrfy_meta __P((DB *, VRFY_DBINFO *, QMETA *,
 * PUBLIC:     db_pgno_t, u_int32_t));
 */
int
__qam_vrfy_meta(dbp, vdp, meta, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	QMETA *meta;
	db_pgno_t pgno;
	u_int32_t flags;
{
	ENV *env;
	QUEUE *qp;
	VRFY_PAGEINFO *pip;
	db_pgno_t *extents, extid, first, last;
	size_t len;
	int count, i, isbad, nextents, ret, t_ret;
	char *buf, **names;

	COMPQUIET(count, 0);

	env = dbp->env;
	qp = (QUEUE *)dbp->q_internal;
	extents = NULL;
	first = last = 0;
	isbad = 0;
	buf = NULL;
	names = NULL;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	/*
	 * Queue can't be used in subdatabases, so if this isn't set
	 * something very odd is going on.
	 */
	if (!F_ISSET(pip, VRFY_INCOMPLETE))
		EPRINT((env, "Page %lu: queue databases must be one-per-file",
		    (u_long)pgno));

	/*
	 * Because the metapage pointers are rolled forward by
	 * aborting transactions, the extent of the queue may
	 * extend beyond the allocated pages, so we do
	 * not check that meta_current is within the allocated
	 * pages.
	 */

	/*
	 * re_len:  If this is bad, we can't safely verify queue data pages, so
	 * return DB_VERIFY_FATAL
	 */
	if (DB_ALIGN(meta->re_len + sizeof(QAMDATA) - 1, sizeof(u_int32_t)) *
	    meta->rec_page + QPAGE_SZ(dbp) > dbp->pgsize) {
		EPRINT((env,
   "Page %lu: queue record length %lu too high for page size and recs/page",
		    (u_long)pgno, (u_long)meta->re_len));
		ret = DB_VERIFY_FATAL;
		goto err;
	} else {
		/*
		 * We initialize the Queue internal pointer;  we may need
		 * it when handling extents.  It would get set up in open,
		 * if we called open normally, but we don't.
		 */
		vdp->re_pad = meta->re_pad;
		qp->re_pad = (int)meta->re_pad;
		qp->re_len = vdp->re_len = meta->re_len;
		qp->rec_page = vdp->rec_page = meta->rec_page;
		qp->page_ext = vdp->page_ext = meta->page_ext;
	}

	/*
	 * There's no formal maximum extentsize, and a 0 value represents
	 * no extents, so there's nothing to verify.
	 *
	 * Note that since QUEUE databases can't have subdatabases, it's an
	 * error to see more than one QUEUE metadata page in a single
	 * verifier run.  Theoretically, this should really be a structure
	 * rather than a per-page check, but since we're setting qp fields
	 * here (and have only one qp to set) we raise the alarm now if
	 * this assumption fails.  (We need the qp info to be reasonable
	 * before we do per-page verification of queue extents.)
	 */
	if (F_ISSET(vdp, VRFY_QMETA_SET)) {
		isbad = 1;
		EPRINT((env,
		    "Page %lu: database contains multiple Queue metadata pages",
		    (u_long)pgno));
		goto err;
	}
	F_SET(vdp, VRFY_QMETA_SET);
	qp->page_ext = meta->page_ext;
	dbp->pgsize = meta->dbmeta.pagesize;
	qp->q_meta = pgno;
	qp->q_root = pgno + 1;
	vdp->first_recno = meta->first_recno;
	vdp->last_recno = meta->cur_recno;
	if (qp->page_ext != 0) {
		first = QAM_RECNO_EXTENT(dbp, vdp->first_recno);
		last = QAM_RECNO_EXTENT(dbp, vdp->last_recno);
	}

	/*
	 * Look in the data directory to see if there are any extents
	 * around that are not in the range of the queue.  If so,
	 * then report that and look there if we are salvaging.
	 */

	if ((ret = __db_appname(env,
	    DB_APP_DATA, qp->dir, NULL, &buf)) != 0)
		goto err;
	if ((ret = __os_dirlist(env, buf, 0, &names, &count)) != 0)
		goto err;
	__os_free(env, buf);
	buf = NULL;

	len = strlen(QUEUE_EXTENT_HEAD) + strlen(qp->name) + 1;
	if ((ret = __os_malloc(env, len, &buf)) != 0)
		goto err;
	len = (size_t)snprintf(buf, len, QUEUE_EXTENT_HEAD, qp->name);
	for (i = nextents = 0; i < count; i++) {
		if (strncmp(names[i], buf, len) == 0) {
			/* Only save extents out of bounds. */
			extid = (db_pgno_t)strtoul(&names[i][len], NULL, 10);
			if (qp->page_ext != 0 &&
			    (last > first ?
			    (extid >= first && extid <= last) :
			    (extid >= first || extid <= last)))
				continue;
			if (extents == NULL && (ret = __os_malloc(
			     env, (size_t)(count - i) * sizeof(extid),
			     &extents)) != 0)
				goto err;
			extents[nextents] = extid;
			nextents++;
		}
	}
	if (nextents > 0)
		__db_errx(env,
		     "Warning: %d extra extent files found", nextents);
	vdp->nextents = nextents;
	vdp->extents = extents;

err:	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	if (names != NULL)
		__os_dirfree(env, names, count);
	if (buf != NULL)
		__os_free(env, buf);
	if (ret != 0 && extents != NULL)
		__os_free(env, extents);
	if (LF_ISSET(DB_SALVAGE) &&
	   (t_ret = __db_salvage_markdone(vdp, pgno)) != 0 && ret == 0)
		ret = t_ret;
	return (ret == 0 && isbad == 1 ? DB_VERIFY_BAD : ret);
}

/*
 * __qam_meta2pgset --
 * For a given Queue meta page, add all of the db's pages to the pgset.  Dealing
 * with extents complicates things, as it is possible for there to be gaps in
 * the page number sequence (the user could have re-inserted record numbers that
 * had been on deleted extents) so we test the existence of each extent before
 * adding its pages to the pgset.  If there are no extents, just loop from
 * first_recno to last_recno.
 *
 * PUBLIC: int __qam_meta2pgset __P((DB *, VRFY_DBINFO *, DB *));
 */
int
__qam_meta2pgset(dbp, vdp, pgset)
	DB *dbp;
	VRFY_DBINFO *vdp;
	DB *pgset;
{
	DBC *dbc;
	PAGE *h;
	db_pgno_t first, last, pgno, pg_ext, stop;
	int ret, t_ret;
	u_int32_t i;

	ret = 0;
	h = NULL;
	if (vdp->last_recno <= vdp->first_recno)
		return 0;

	pg_ext = vdp->page_ext;

	first = QAM_RECNO_PAGE(dbp, vdp->first_recno);

	/*
	 * last_recno gives the next recno to be allocated, we want the last
	 * allocated recno.
	 */
	last = QAM_RECNO_PAGE(dbp, vdp->last_recno - 1);

	if (first == PGNO_INVALID || last == PGNO_INVALID)
		return (DB_VERIFY_BAD);

	pgno = first;
	if (first > last)
		stop = QAM_RECNO_PAGE(dbp, UINT32_MAX);
	else
		stop = last;

	/*
	 * If this db doesn't have extents, just add all page numbers from first
	 * to last.
	 */
	if (pg_ext == 0) {
		for (pgno = first; pgno <= stop; pgno++)
			if ((ret = __db_vrfy_pgset_inc(
			    pgset, vdp->thread_info, pgno)) != 0)
				break;
		if (first > last)
			for (pgno = 1; pgno <= last; pgno++)
				if ((ret = __db_vrfy_pgset_inc(
				    pgset, vdp->thread_info, pgno)) != 0)
					break;

		return ret;
	}

	if ((ret = __db_cursor(dbp, vdp->thread_info, NULL, &dbc, 0)) != 0)
		return (ret);
	/*
	 * Check if we can get the first page of each extent.  If we can, then
	 * add all of that extent's pages to the pgset.  If we can't, assume the
	 * extent doesn't exist and don't add any pages, if we're wrong we'll
	 * find the pages in __db_vrfy_walkpages.
	 */
begin:	for (; pgno <= stop; pgno += pg_ext) {
		if ((ret = __qam_fget(dbc, &pgno, 0, &h)) != 0) {
			if (ret == ENOENT || ret == DB_PAGE_NOTFOUND) {
				ret = 0;
				continue;
			}
			goto err;
		}
		if ((ret = __qam_fput(dbc, pgno, h, dbp->priority)) != 0)
			goto err;

		for (i = 0; i < pg_ext && pgno + i <= last; i++)
			if ((ret = __db_vrfy_pgset_inc(
			    pgset, vdp->thread_info, pgno + i)) != 0)
				goto err;

		/* The first recno won't always occur on the first page of the
		 * extent.  Back up to the beginning of the extent before the
		 * end of the loop so that the increment works correctly.
		 */
		if (pgno == first)
			pgno = pgno % pg_ext + 1;
	}

	if (first > last) {
		pgno = 1;
		first = last;
		stop = last;
		goto begin;
	}

err:
	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return ret;
}

/*
 * __qam_vrfy_data --
 *	Verify a queue data page.
 *
 * PUBLIC: int __qam_vrfy_data __P((DB *, VRFY_DBINFO *, QPAGE *,
 * PUBLIC:     db_pgno_t, u_int32_t));
 */
int
__qam_vrfy_data(dbp, vdp, h, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	QPAGE *h;
	db_pgno_t pgno;
	u_int32_t flags;
{
	DB fakedb;
	struct __queue fakeq;
	QAMDATA *qp;
	db_recno_t i;

	/*
	 * Not much to do here, except make sure that flags are reasonable.
	 *
	 * QAM_GET_RECORD assumes a properly initialized q_internal
	 * structure, however, and we don't have one, so we play
	 * some gross games to fake it out.
	 */
	fakedb.q_internal = &fakeq;
	fakedb.flags = dbp->flags;
	fakeq.re_len = vdp->re_len;

	for (i = 0; i < vdp->rec_page; i++) {
		qp = QAM_GET_RECORD(&fakedb, h, i);
		if ((u_int8_t *)qp >= (u_int8_t *)h + dbp->pgsize) {
			EPRINT((dbp->env,
		    "Page %lu: queue record %lu extends past end of page",
			    (u_long)pgno, (u_long)i));
			return (DB_VERIFY_BAD);
		}

		if (qp->flags & ~(QAM_VALID | QAM_SET)) {
			EPRINT((dbp->env,
			    "Page %lu: queue record %lu has bad flags (%#lx)",
			    (u_long)pgno, (u_long)i, (u_long)qp->flags));
			return (DB_VERIFY_BAD);
		}
	}

	return (0);
}

/*
 * __qam_vrfy_structure --
 *	Verify a queue database structure, such as it is.
 *
 * PUBLIC: int __qam_vrfy_structure __P((DB *, VRFY_DBINFO *, u_int32_t));
 */
int
__qam_vrfy_structure(dbp, vdp, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	u_int32_t flags;
{
	VRFY_PAGEINFO *pip;
	db_pgno_t i;
	int ret, isbad;

	isbad = 0;

	if ((ret = __db_vrfy_getpageinfo(vdp, PGNO_BASE_MD, &pip)) != 0)
		return (ret);

	if (pip->type != P_QAMMETA) {
		EPRINT((dbp->env,
		    "Page %lu: queue database has no meta page",
		    (u_long)PGNO_BASE_MD));
		isbad = 1;
		goto err;
	}

	if ((ret = __db_vrfy_pgset_inc(vdp->pgset, vdp->thread_info, 0)) != 0)
		goto err;

	for (i = 1; i <= vdp->last_pgno; i++) {
		/* Send feedback to the application about our progress. */
		if (!LF_ISSET(DB_SALVAGE))
			__db_vrfy_struct_feedback(dbp, vdp);

		if ((ret = __db_vrfy_putpageinfo(dbp->env, vdp, pip)) != 0 ||
		    (ret = __db_vrfy_getpageinfo(vdp, i, &pip)) != 0)
			return (ret);
		if (!F_ISSET(pip, VRFY_IS_ALLZEROES) &&
		    pip->type != P_QAMDATA) {
			EPRINT((dbp->env,
		    "Page %lu: queue database page of incorrect type %lu",
			    (u_long)i, (u_long)pip->type));
			isbad = 1;
			goto err;
		} else if ((ret = __db_vrfy_pgset_inc(vdp->pgset,
		    vdp->thread_info, i)) != 0)
			goto err;
	}

err:	if ((ret = __db_vrfy_putpageinfo(dbp->env, vdp, pip)) != 0)
		return (ret);
	return (isbad == 1 ? DB_VERIFY_BAD : 0);
}

/*
 * __qam_vrfy_walkqueue --
 *    Do a "walkpages" per-page verification pass over the set of Queue
 * extent pages.
 *
 * PUBLIC: int __qam_vrfy_walkqueue __P((DB *, VRFY_DBINFO *, void *,
 * PUBLIC:    int (*)(void *, const void *), u_int32_t));
 */
int
__qam_vrfy_walkqueue(dbp, vdp, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	DBC *dbc;
	ENV *env;
	PAGE *h;
	QUEUE *qp;
	VRFY_PAGEINFO *pip;
	db_pgno_t first, i, last, pg_ext, stop;
	int isbad, nextents, ret, t_ret;

	COMPQUIET(h, NULL);

	env = dbp->env;
	qp = dbp->q_internal;
	pip = NULL;
	pg_ext = qp->page_ext;
	isbad = ret = t_ret = 0;
	h = NULL;

	/* If this database has no extents, we've seen all the pages already. */
	if (pg_ext == 0)
		return (0);

	first = QAM_RECNO_PAGE(dbp, vdp->first_recno);
	last = QAM_RECNO_PAGE(dbp, vdp->last_recno);

	i = first;
	if (first > last)
		stop = QAM_RECNO_PAGE(dbp, UINT32_MAX);
	else
		stop = last;
	nextents = vdp->nextents;

	/* Verify/salvage each page. */
	if ((ret = __db_cursor(dbp, vdp->thread_info, NULL, &dbc, 0)) != 0)
		return (ret);
begin:	for (; i <= stop; i++) {
		/*
		 * If DB_SALVAGE is set, we inspect our database of completed
		 * pages, and skip any we've already printed in the subdb pass.
		 */
		if (LF_ISSET(DB_SALVAGE) && (__db_salvage_isdone(vdp, i) != 0))
			continue;
		if ((t_ret = __qam_fget(dbc, &i, 0, &h)) != 0) {
			if (t_ret == ENOENT || t_ret == DB_PAGE_NOTFOUND) {
				i += (pg_ext - ((i - 1) % pg_ext)) - 1;
				continue;
			}

			/*
			 * If an individual page get fails, keep going iff
			 * we're salvaging.
			 */
			if (LF_ISSET(DB_SALVAGE)) {
				if (ret == 0)
					ret = t_ret;
				continue;
			}
			h = NULL;
			ret = t_ret;
			goto err;
		}

		if (LF_ISSET(DB_SALVAGE)) {
			/*
			 * We pretty much don't want to quit unless a
			 * bomb hits.  May as well return that something
			 * was screwy, however.
			 */
			if ((t_ret = __db_salvage_pg(dbp,
			    vdp, i, h, handle, callback, flags)) != 0) {
				if (ret == 0)
					ret = t_ret;
				isbad = 1;
			}
		} else {
			/*
			 * If we are not salvaging, and we get any error
			 * other than DB_VERIFY_BAD, return immediately;
			 * it may not be safe to proceed.  If we get
			 * DB_VERIFY_BAD, keep going;  listing more errors
			 * may make it easier to diagnose problems and
			 * determine the magnitude of the corruption.
			 */
			if ((ret = __db_vrfy_common(dbp,
			    vdp, h, i, flags)) == DB_VERIFY_BAD)
				isbad = 1;
			else if (ret != 0)
				goto err;

			__db_vrfy_struct_feedback(dbp, vdp);

			if ((ret = __db_vrfy_getpageinfo(vdp, i, &pip)) != 0)
				goto err;
			if (F_ISSET(pip, VRFY_IS_ALLZEROES))
				goto put;
			if (pip->type != P_QAMDATA) {
				EPRINT((env,
		    "Page %lu: queue database page of incorrect type %lu",
				    (u_long)i, (u_long)pip->type));
				isbad = 1;
				goto err;
			}
			if ((ret = __db_vrfy_pgset_inc(vdp->pgset,
			    vdp->thread_info, i)) != 0)
				goto err;
			if ((ret = __qam_vrfy_data(dbp, vdp,
			    (QPAGE *)h, i, flags)) == DB_VERIFY_BAD)
				isbad = 1;
			else if (ret != 0)
				goto err;

put:			if ((ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0)
				goto err1;
			pip = NULL;
		}

		/* Again, keep going iff we're salvaging. */
		if ((t_ret = __qam_fput(dbc, i, h, dbp->priority)) != 0) {
			if (LF_ISSET(DB_SALVAGE)) {
				if (ret == 0)
					ret = t_ret;
				continue;
			}
			ret = t_ret;
			goto err1;
		}
	}

	if (first > last) {
		i = 1;
		stop = last;
		first = last;
		goto begin;
	}

	/*
	 * Now check to see if there were any lingering
	 * extents and dump their data.
	 */
	if (LF_ISSET(DB_SALVAGE) && nextents != 0) {
		nextents--;
		i = 1 +
		    vdp->extents[nextents] * vdp->page_ext;
		stop = i + vdp->page_ext;
		goto begin;
	}

	if (0) {
err:		if (h != NULL && (t_ret =
		    __qam_fput(dbc, i, h, dbp->priority)) != 0 && ret == 0)
			ret = t_ret;
		if (pip != NULL && (t_ret =
		    __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
			ret = t_ret;
	}
err1:	if (dbc != NULL && (t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;
	return ((isbad == 1 && ret == 0) ? DB_VERIFY_BAD : ret);
}

/*
 * __qam_salvage --
 *	Safely dump out all recnos and data on a queue page.
 *
 * PUBLIC: int __qam_salvage __P((DB *, VRFY_DBINFO *, db_pgno_t, PAGE *,
 * PUBLIC:     void *, int (*)(void *, const void *), u_int32_t));
 */
int
__qam_salvage(dbp, vdp, pgno, h, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	PAGE *h;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	DBT dbt, key;
	QAMDATA *qp, *qep;
	db_recno_t recno;
	int ret, err_ret, t_ret;
	u_int32_t pagesize, qlen;
	u_int32_t i;

	memset(&dbt, 0, sizeof(DBT));
	memset(&key, 0, sizeof(DBT));

	err_ret = ret = 0;

	pagesize = (u_int32_t)dbp->mpf->mfp->stat.st_pagesize;
	qlen = ((QUEUE *)dbp->q_internal)->re_len;
	dbt.size = qlen;
	key.data = &recno;
	key.size = sizeof(recno);
	recno = (pgno - 1) * QAM_RECNO_PER_PAGE(dbp) + 1;
	i = 0;
	qep = (QAMDATA *)((u_int8_t *)h + pagesize - qlen);
	for (qp = QAM_GET_RECORD(dbp, h, i); qp < qep;
	    recno++, i++, qp = QAM_GET_RECORD(dbp, h, i)) {
		if (F_ISSET(qp, ~(QAM_VALID|QAM_SET)))
			continue;
		if (!F_ISSET(qp, QAM_SET))
			continue;

		if (!LF_ISSET(DB_AGGRESSIVE) && !F_ISSET(qp, QAM_VALID))
			continue;

		dbt.data = qp->data;
		if ((ret = __db_vrfy_prdbt(&key,
		    0, " ", handle, callback, 1, vdp)) != 0)
			err_ret = ret;

		if ((ret = __db_vrfy_prdbt(&dbt,
		    0, " ", handle, callback, 0, vdp)) != 0)
			err_ret = ret;
	}

	if ((t_ret = __db_salvage_markdone(vdp, pgno)) != 0)
		return (t_ret);
	return ((ret == 0 && err_ret != 0) ? err_ret : ret);
}
