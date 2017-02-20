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
#include "dbinc/btree.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/qam.h"

static int __qam_bulk __P((DBC *, DBT *, u_int32_t));
static int __qamc_close __P((DBC *, db_pgno_t, int *));
static int __qamc_del __P((DBC *, u_int32_t));
static int __qamc_destroy __P((DBC *));
static int __qamc_get __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
static int __qamc_put __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
static int __qam_consume __P((DBC *, QMETA *, db_recno_t));
static int __qam_getno __P((DB *, const DBT *, db_recno_t *));

#define	DONT_NEED_LOCKS(dbc) ((dbc)->txn == NULL ||			\
	F_ISSET(dbc, DBC_READ_COMMITTED | DBC_READ_UNCOMMITTED))

/*
 * __qam_position --
 *	Position a queued access method cursor at a record.  This returns
 *	the page locked.  *exactp will be set if the record is valid.
 * PUBLIC: int __qam_position
 * PUBLIC:      __P((DBC *, db_recno_t *, u_int32_t, int *));
 */
int
__qam_position(dbc, recnop, get_mode, exactp)
	DBC *dbc;		/* open cursor */
	db_recno_t *recnop;	/* pointer to recno to find */
	u_int32_t get_mode;	/* flags to __memp_fget */
	int *exactp;		/* indicate if it was found */
{
	DB *dbp;
	QAMDATA  *qp;
	QUEUE_CURSOR *cp;
	db_pgno_t pg;
	int ret;

	dbp = dbc->dbp;
	cp = (QUEUE_CURSOR *)dbc->internal;

	/* Fetch the page for this recno. */
	cp->pgno = pg = QAM_RECNO_PAGE(dbp, *recnop);

	cp->page = NULL;
	*exactp = 0;
	if ((ret = __qam_fget(dbc, &pg, get_mode, &cp->page)) != 0) {
		if (!FLD_ISSET(get_mode, DB_MPOOL_CREATE) &&
		    (ret == DB_PAGE_NOTFOUND || ret == ENOENT))
			ret = 0;
		return (ret);
	}
	cp->indx = QAM_RECNO_INDEX(dbp, pg, *recnop);

	if (PGNO(cp->page) == 0) {
		/*
		 * We have read an uninitialized page: set the page number if
		 * we're creating the page.  Otherwise, we know that the record
		 * doesn't exist yet.
		 */
		if (!FLD_ISSET(get_mode, DB_MPOOL_CREATE)) {
			*exactp = 0;
			return (0);
		}
		DB_ASSERT(dbp->env, FLD_ISSET(get_mode, DB_MPOOL_CREATE));
		PGNO(cp->page) = pg;
		TYPE(cp->page) = P_QAMDATA;
	}

	qp = QAM_GET_RECORD(dbp, cp->page, cp->indx);
	*exactp = F_ISSET(qp, QAM_VALID) ? 1 : 0;

	return (ret);
}

/*
 * __qam_pitem --
 *	Put an item on a queue page.  Copy the data to the page and set the
 *	VALID and SET bits.  If logging and the record was previously set,
 *	log that data, otherwise just log the new data.
 *
 *   pagep must be write locked
 *
 * PUBLIC: int __qam_pitem
 * PUBLIC:    __P((DBC *,  QPAGE *, u_int32_t, db_recno_t, DBT *));
 */
int
__qam_pitem(dbc, pagep, indx, recno, data)
	DBC *dbc;
	QPAGE *pagep;
	u_int32_t indx;
	db_recno_t recno;
	DBT *data;
{
	DB *dbp;
	DBT olddata, pdata, *datap;
	ENV *env;
	QAMDATA *qp;
	QUEUE *t;
	u_int8_t *dest, *p;
	int allocated, ret;

	dbp = dbc->dbp;
	env = dbp->env;
	t = (QUEUE *)dbp->q_internal;
	allocated = ret = 0;

	if (data->size > t->re_len)
		return (__db_rec_toobig(env, data->size, t->re_len));
	qp = QAM_GET_RECORD(dbp, pagep, indx);

	p = qp->data;
	datap = data;
	if (F_ISSET(data, DB_DBT_PARTIAL)) {
		if (data->doff + data->dlen > t->re_len) {
			__db_errx(env,
		"%s: data offset plus length larger than record size of %lu",
			    "Record length error", (u_long)t->re_len);
			return (EINVAL);
		}

		if (data->size != data->dlen)
			return (__db_rec_repl(env, data->size, data->dlen));

		if (data->size == t->re_len)
			goto no_partial;

		/*
		 * If we are logging, then we have to build the record
		 * first, otherwise, we can simply drop the change
		 * directly on the page.  After this clause, make
		 * sure that datap and p are set up correctly so that
		 * copying datap into p does the right thing.
		 *
		 * Note, I am changing this so that if the existing
		 * record is not valid, we create a complete record
		 * to log so that both this and the recovery code is simpler.
		 */

		if (DBC_LOGGING(dbc) || !F_ISSET(qp, QAM_VALID)) {
			datap = &pdata;
			memset(datap, 0, sizeof(*datap));

			if ((ret = __os_malloc(env,
			    t->re_len, &datap->data)) != 0)
				return (ret);
			allocated = 1;
			datap->size = t->re_len;

			/*
			 * Construct the record if it's valid, otherwise set it
			 * all to the pad character.
			 */
			dest = datap->data;
			if (F_ISSET(qp, QAM_VALID))
				memcpy(dest, p, t->re_len);
			else
				memset(dest, (int)t->re_pad, t->re_len);

			dest += data->doff;
			memcpy(dest, data->data, data->size);
		} else {
			datap = data;
			p += data->doff;
		}
	}

no_partial:
	if (DBC_LOGGING(dbc)) {
		olddata.size = 0;
		if (F_ISSET(qp, QAM_SET)) {
			olddata.data = qp->data;
			olddata.size = t->re_len;
		}
		if ((ret = __qam_add_log(dbp, dbc->txn, &LSN(pagep),
		    0, &LSN(pagep), pagep->pgno,
		    indx, recno, datap, qp->flags,
		    olddata.size == 0 ? NULL : &olddata)) != 0)
			goto err;
	} else if (!F_ISSET((dbc), DBC_RECOVER))
		LSN_NOT_LOGGED(LSN(pagep));

	F_SET(qp, QAM_VALID | QAM_SET);
	memcpy(p, datap->data, datap->size);
	if (!F_ISSET(data, DB_DBT_PARTIAL))
		memset(p + datap->size,
		    (int)t->re_pad, t->re_len - datap->size);

err:	if (allocated)
		__os_free(env, datap->data);

	return (ret);
}
/*
 * __qamc_put
 *	Cursor put for queued access method.
 *	BEFORE and AFTER cannot be specified.
 */
static int
__qamc_put(dbc, key, data, flags, pgnop)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
	db_pgno_t *pgnop;
{
	DB *dbp;
	DB_MPOOLFILE *mpf;
	ENV *env;
	QMETA *meta;
	QUEUE_CURSOR *cp;
	db_pgno_t pg;
	db_recno_t new_cur, new_first;
	u_int32_t opcode;
	int exact, ret, t_ret, writelock;

	dbp = dbc->dbp;
	env = dbp->env;
	mpf = dbp->mpf;
	if (pgnop != NULL)
		*pgnop = PGNO_INVALID;

	cp = (QUEUE_CURSOR *)dbc->internal;

	switch (flags) {
	case DB_KEYFIRST:
	case DB_KEYLAST:
	case DB_NOOVERWRITE:
	case DB_OVERWRITE_DUP:
		if ((ret = __qam_getno(dbp, key, &cp->recno)) != 0)
			return (ret);
		/* FALLTHROUGH */
	case DB_CURRENT:
		break;
	default:
		/* The interface shouldn't let anything else through. */
		return (__db_ferr(env, "DBC->put", 0));
	}

	/* Write lock the record. */
	if ((ret = __db_lget(dbc, LCK_COUPLE,
	    cp->recno, DB_LOCK_WRITE, DB_LOCK_RECORD, &cp->lock)) != 0)
		return (ret);

	if ((ret = __qam_position(dbc, &cp->recno,
	    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &exact)) != 0) {
		/* We could not get the page, we can release the record lock. */
		(void)__LPUT(dbc, cp->lock);
		return (ret);
	}

	if (exact != 0 && flags == DB_NOOVERWRITE)
		ret = DB_KEYEXIST;
	else
		/* Put the item on the page. */
		ret = __qam_pitem(dbc,
		     (QPAGE *)cp->page, cp->indx, cp->recno, data);

	if ((t_ret = __qam_fput(dbc,
	    cp->pgno, cp->page, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	cp->page = NULL;
	cp->lock_mode = DB_LOCK_WRITE;
	if (ret != 0)
		return (ret);

	/* We may need to reset the head or tail of the queue. */
	pg = ((QUEUE *)dbp->q_internal)->q_meta;

	writelock = 0;
	if ((ret = __db_lget(dbc, LCK_COUPLE,
	    pg,  DB_LOCK_READ, 0, &cp->lock)) != 0)
		return (ret);
	if ((ret = __memp_fget(mpf, &pg,
	    dbc->thread_info, dbc->txn, 0, &meta)) != 0)
		goto err;

	opcode = 0;
	new_cur = new_first = 0;

	/*
	 * If the put address is outside the queue, adjust the head and
	 * tail of the queue.  If the order is inverted we move
	 * the one which is closer.  The first case is when the
	 * queue is empty, move first and current to where the new
	 * insert is.
	 */

recheck:
	if (meta->first_recno == meta->cur_recno) {
		new_first = cp->recno;
		new_cur = cp->recno + 1;
		if (new_cur == RECNO_OOB)
			new_cur++;
		opcode |= QAM_SETFIRST;
		opcode |= QAM_SETCUR;
	} else {
		if (QAM_BEFORE_FIRST(meta, cp->recno)) {
			new_first = cp->recno;
			opcode |= QAM_SETFIRST;
		}

		if (QAM_AFTER_CURRENT(meta, cp->recno)) {
			new_cur = cp->recno + 1;
			if (new_cur == RECNO_OOB)
				new_cur++;
			opcode |= QAM_SETCUR;
		}
	}

	if (opcode == 0)
		goto done;

	/* Drop the read lock and get the a write lock on the meta page. */
	if (writelock == 0 && (ret = __db_lget(dbc, LCK_COUPLE_ALWAYS,
	     pg,  DB_LOCK_WRITE, 0, &cp->lock)) != 0)
		goto done;
	if (writelock++ == 0)
		goto recheck;

	if (((ret = __memp_dirty(mpf, &meta,
	    dbc->thread_info, dbc->txn, dbc->priority, DB_MPOOL_DIRTY)) != 0 ||
	    (DBC_LOGGING(dbc) &&
	    (ret = __qam_mvptr_log(dbp, dbc->txn,
	    &meta->dbmeta.lsn, 0, opcode, meta->first_recno,
	    new_first, meta->cur_recno, new_cur,
	    &meta->dbmeta.lsn, PGNO_BASE_MD)) != 0)))
		opcode = 0;

	if (opcode & QAM_SETCUR)
		meta->cur_recno = new_cur;
	if (opcode & QAM_SETFIRST)
		meta->first_recno = new_first;

done:	if (meta != NULL && (t_ret = __memp_fput(mpf,
	    dbc->thread_info, meta, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;

err:	/* Don't hold the meta page long term. */
	if ((t_ret = __LPUT(dbc, cp->lock)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __qam_append --
 *	Perform a put(DB_APPEND) in queue.
 *
 * PUBLIC: int __qam_append __P((DBC *, DBT *, DBT *));
 */
int
__qam_append(dbc, key, data)
	DBC *dbc;
	DBT *key, *data;
{
	DB *dbp;
	DB_LOCK lock;
	DB_MPOOLFILE *mpf;
	QMETA *meta;
	QPAGE *page;
	QUEUE *qp;
	QUEUE_CURSOR *cp;
	db_pgno_t pg, metapg;
	db_recno_t recno;
	int ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (QUEUE_CURSOR *)dbc->internal;

	/* Write lock the meta page. */
	metapg = ((QUEUE *)dbp->q_internal)->q_meta;
	if ((ret = __db_lget(dbc, 0, metapg,  DB_LOCK_WRITE, 0, &lock)) != 0)
		return (ret);
	if ((ret = __memp_fget(mpf, &metapg,
	    dbc->thread_info, dbc->txn, DB_MPOOL_DIRTY, &meta)) != 0)
		return (ret);

	/* Get the next record number. */
	recno = meta->cur_recno;
	meta->cur_recno++;
	if (meta->cur_recno == RECNO_OOB)
		meta->cur_recno++;
	if (meta->cur_recno == meta->first_recno) {
		meta->cur_recno--;
		if (meta->cur_recno == RECNO_OOB)
			meta->cur_recno--;

		if (ret == 0)
			ret = EFBIG;
		goto err;
	}

	if (QAM_BEFORE_FIRST(meta, recno))
		meta->first_recno = recno;

	/* Lock the record and release meta page lock. */
	ret = __db_lget(dbc, LCK_COUPLE_ALWAYS,
	    recno, DB_LOCK_WRITE, DB_LOCK_RECORD, &lock);
	/* Release the meta page. */
	if ((t_ret = __memp_fput(mpf,
	    dbc->thread_info, meta, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	meta = NULL;

	/*
	 * The application may modify the data based on the selected record
	 * number.  We always want to call this even if we ultimately end
	 * up aborting, because we are allocating a record number, regardless.
	 */
	if (dbc->dbp->db_append_recno != NULL &&
	    (t_ret = dbc->dbp->db_append_recno(dbc->dbp, data, recno)) != 0 &&
	    ret == 0)
		ret = t_ret;

	/*
	 * Capture errors from either the lock couple or the call to
	 * dbp->db_append_recno.
	 */
	if (ret != 0)
		goto err;

	cp->lock = lock;
	cp->lock_mode = DB_LOCK_WRITE;
	LOCK_INIT(lock);

	pg = QAM_RECNO_PAGE(dbp, recno);

	/* Fetch for write the data page. */
	if ((ret = __qam_fget(dbc, &pg,
	    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &page)) != 0)
		goto err;

	/* See if this is a new page. */
	if (page->pgno == 0) {
		page->pgno = pg;
		page->type = P_QAMDATA;
	}

	/* Put the item on the page and log it. */
	ret = __qam_pitem(dbc, page,
	    QAM_RECNO_INDEX(dbp, pg, recno), recno, data);

	if ((t_ret = __qam_fput(dbc,
	    pg, page, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;

	/* Return the record number to the user. */
	if (ret == 0 && key != NULL)
		ret = __db_retcopy(dbp->env, key,
		    &recno, sizeof(recno), &dbc->rkey->data, &dbc->rkey->ulen);

	/* Position the cursor on this record. */
	cp->recno = recno;

	/* See if we are leaving the extent. */
	qp = (QUEUE *) dbp->q_internal;
	if (qp->page_ext != 0 &&
	    (recno % (qp->page_ext * qp->rec_page) == 0 ||
	    recno == UINT32_MAX)) {
		if ((ret = __db_lget(dbc,
		    0, metapg, DB_LOCK_READ, 0, &lock)) != 0)
			goto err;
		if ((ret = __memp_fget(mpf, &metapg,
		    dbc->thread_info, dbc->txn, 0, &meta)) != 0)
			goto err;
		if (!QAM_AFTER_CURRENT(meta, recno))
			ret = __qam_fclose(dbp, pg);
	}

err:	/* Release the meta page. */
	if (meta != NULL && (t_ret = __memp_fput(mpf,
	    dbc->thread_info, meta, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __qamc_del --
 *	Qam cursor->am_del function
 */
static int
__qamc_del(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	DB *dbp;
	DBT data;
	DB_LOCK metalock;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	QAMDATA *qp;
	QMETA *meta;
	QUEUE_CURSOR *cp;
	db_pgno_t pg;
	db_recno_t first;
	int exact, ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (QUEUE_CURSOR *)dbc->internal;

	pg = ((QUEUE *)dbp->q_internal)->q_meta;
	/* Read lock the meta page. */
	if ((ret = __db_lget(dbc, 0, pg,  DB_LOCK_READ, 0, &metalock)) != 0)
		return (ret);

	if ((ret = __memp_fget(mpf, &pg,
	    dbc->thread_info, dbc->txn, 0, &meta)) != 0)
		return (ret);

	if (QAM_NOT_VALID(meta, cp->recno)) {
		ret = DB_NOTFOUND;
		goto err;
	}
	first = meta->first_recno;

	/* Don't hold the meta page long term. */
	if ((ret = __memp_fput(mpf,
	     dbc->thread_info, meta, dbc->priority)) != 0)
		goto err;
	meta = NULL;
	if ((ret = __LPUT(dbc, metalock)) != 0)
		goto err;

	/* Get the record. */
	if ((ret = __db_lget(dbc, LCK_COUPLE,
	    cp->recno, DB_LOCK_WRITE, DB_LOCK_RECORD, &cp->lock)) != 0)
		goto err;
	cp->lock_mode = DB_LOCK_WRITE;

	/* Find the record; delete only deletes exact matches. */
	if ((ret = __qam_position(dbc, &cp->recno,
	    DB_MPOOL_DIRTY, &exact)) != 0)
		goto err;

	if (!exact) {
		ret = DB_NOTFOUND;
		goto err;
	}

	pagep = cp->page;
	qp = QAM_GET_RECORD(dbp, pagep, cp->indx);

	if (DBC_LOGGING(dbc)) {
		if (((QUEUE *)dbp->q_internal)->page_ext == 0 ||
		    ((QUEUE *)dbp->q_internal)->re_len == 0) {
			if ((ret = __qam_del_log(dbp,
			    dbc->txn, &LSN(pagep), 0, &LSN(pagep),
			    pagep->pgno, cp->indx, cp->recno)) != 0)
				goto err;
		} else {
			data.size = ((QUEUE *)dbp->q_internal)->re_len;
			data.data = qp->data;
			if ((ret = __qam_delext_log(dbp,
			    dbc->txn, &LSN(pagep), 0, &LSN(pagep),
			    pagep->pgno, cp->indx, cp->recno, &data)) != 0)
				goto err;
		}
	} else
		LSN_NOT_LOGGED(LSN(pagep));

	F_CLR(qp, QAM_VALID);
	if ((ret = __qam_fput(dbc,
	    cp->pgno, cp->page, dbc->priority)) != 0)
		goto err;
	cp->page = NULL;

	/*
	 * Other threads cannot move first_recno past
	 * our position while we have the record locked.
	 * If it's pointing at the deleted record then lock
	 * the metapage and check again as lower numbered
	 * record may have been inserted.
	 */
	if (LF_ISSET(DB_CONSUME) || cp->recno == first) {
		pg = ((QUEUE *)dbp->q_internal)->q_meta;
		if ((ret =
		    __db_lget(dbc, 0, pg,  DB_LOCK_WRITE, 0, &metalock)) != 0)
			goto err;
		if ((ret = __memp_fget(mpf, &pg,
		    dbc->thread_info, dbc->txn, 0, &meta)) != 0)
			goto err;
		if (LF_ISSET(DB_CONSUME) || cp->recno == meta->first_recno)
			ret = __qam_consume(dbc, meta, meta->first_recno);
	}

err:	if (meta != NULL && (t_ret = __memp_fput(mpf, dbc->thread_info,
	    meta, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	/* Don't hold the meta page long term. */
	if ((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
		ret = t_ret;

	if (cp->page != NULL &&
	    (t_ret = __qam_fput(dbc,
	    cp->pgno, cp->page, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	cp->page = NULL;

	return (ret);
}

#ifdef	DEBUG_WOP
#define	QDEBUG
#endif

/*
 * __qamc_get --
 *	Queue DBC->get function.
 */
static int
__qamc_get(dbc, key, data, flags, pgnop)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
	db_pgno_t *pgnop;
{
	DB *dbp;
	DBC *dbcdup;
	DBT tmp;
	DB_LOCK lock, metalock;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *pg;
	QAMDATA *qp;
	QMETA *meta;
	QUEUE *t;
	QUEUE_CURSOR *cp;
	db_lockmode_t lock_mode, meta_mode;
	db_pgno_t metapno;
	db_recno_t first;
	int exact, inorder, is_first, locked, ret, t_ret, wait, with_delete;
	int retrying;

	dbp = dbc->dbp;
	env = dbp->env;
	mpf = dbp->mpf;
	cp = (QUEUE_CURSOR *)dbc->internal;
	LOCK_INIT(lock);

	lock_mode = F_ISSET(dbc, DBC_RMW) ? DB_LOCK_WRITE : DB_LOCK_READ;
	meta_mode = DB_LOCK_READ;
	meta = NULL;
	*pgnop = 0;
	pg = NULL;
	retrying = t_ret = wait = with_delete = 0;

	if (flags == DB_CONSUME_WAIT) {
		wait = 1;
		flags = DB_CONSUME;
	}
	if (flags == DB_CONSUME) {
		with_delete = 1;
		flags = DB_FIRST;
		meta_mode = lock_mode = DB_LOCK_WRITE;
	}
	inorder = F_ISSET(dbp, DB_AM_INORDER) && with_delete;

	DEBUG_LREAD(dbc, dbc->txn, "qamc_get",
	    flags == DB_SET || flags == DB_SET_RANGE ? key : NULL, NULL, flags);

	/* Make lint and friends happy. */
	locked = 0;

	is_first = 0;
	first = 0;

	t = (QUEUE *)dbp->q_internal;
	metapno = t->q_meta;
	LOCK_INIT(metalock);

	/*
	 * Get the meta page first
	 */
	if (locked == 0 && (ret = __db_lget(dbc,
	    0, metapno, meta_mode, 0, &metalock)) != 0)
		goto err;
	locked = 1;
	if ((ret = __memp_fget(mpf, &metapno,
	     dbc->thread_info, dbc->txn, 0, &meta)) != 0)
		return (ret);

	/* Release any previous lock if not in a transaction. */
	if ((ret = __TLPUT(dbc, cp->lock)) != 0)
		goto err;

retry:	/* Update the record number. */
	switch (flags) {
	case DB_CURRENT:
		break;
	case DB_NEXT_DUP:
	case DB_PREV_DUP:
		ret = DB_NOTFOUND;
		goto err;
		/* NOTREACHED */
	case DB_NEXT:
	case DB_NEXT_NODUP:
		if (cp->recno != RECNO_OOB) {
			++cp->recno;
			/* Wrap around, skipping zero. */
			if (cp->recno == RECNO_OOB)
				cp->recno++;
			/*
			 * Check to see if we are out of data.
			 */
			if (QAM_AFTER_CURRENT(meta, cp->recno)) {
				pg = NULL;
				if (!wait) {
					ret = DB_NOTFOUND;
					goto err;
				}
				flags = DB_FIRST;
				/*
				 * If first is not set, then we skipped
				 * a locked record, go back and find it.
				 * If we find a locked record again
				 * wait for it.
				 */
				if (first == 0) {
					retrying = 1;
					goto retry;
				}

				if (CDB_LOCKING(env)) {
					/* Drop the metapage before we wait. */
					ret = __memp_fput(mpf, dbc->thread_info,
					    meta, dbc->priority);
					meta = NULL;
					if (ret != 0)
						goto err;
					if ((ret = __lock_get(
					    env, dbc->locker,
					    DB_LOCK_SWITCH, &dbc->lock_dbt,
					    DB_LOCK_WAIT, &dbc->mylock)) != 0)
						goto err;

					if ((ret = __lock_get(
					    env, dbc->locker,
					    DB_LOCK_UPGRADE, &dbc->lock_dbt,
					    DB_LOCK_WRITE, &dbc->mylock)) != 0)
						goto err;
					if ((ret = __memp_fget(mpf, &metapno,
					    dbc->thread_info,
					    dbc->txn, 0, &meta)) != 0)
						goto err;
					goto retry;
				}
				/*
				 * Wait for someone to update the meta page.
				 * This will probably mean there is something
				 * in the queue.  We then go back up and
				 * try again.
				 */
				if (locked == 0) {
					if ((ret = __db_lget(dbc, 0, metapno,
					    meta_mode, 0, &metalock)) != 0)
						goto err;
					locked = 1;
					if (cp->recno != RECNO_OOB &&
					    !QAM_AFTER_CURRENT(meta, cp->recno))
						goto retry;
				}
				/* Drop the metapage before we wait. */
				ret = __memp_fput(mpf,
				     dbc->thread_info, meta, dbc->priority);
				meta = NULL;
				if (ret != 0)
					goto err;
				if ((ret = __db_lget(dbc,
				    0, metapno, DB_LOCK_WAIT,
				    DB_LOCK_SWITCH, &metalock)) != 0) {
					if (ret == DB_LOCK_DEADLOCK)
						ret = DB_LOCK_NOTGRANTED;
					goto err;
				}
				if ((ret = __db_lget(dbc, 0,
				    PGNO_INVALID, DB_LOCK_WRITE,
				    DB_LOCK_UPGRADE, &metalock)) != 0) {
					if (ret == DB_LOCK_DEADLOCK)
						ret = DB_LOCK_NOTGRANTED;
					goto err;
				}
				if ((ret = __memp_fget(mpf,
				     &metapno, dbc->thread_info, dbc->txn,
				    0, &meta)) != 0)
					goto err;
				locked = 1;
				goto retry;
			}
			break;
		}
		/* FALLTHROUGH */
	case DB_FIRST:
		flags = DB_NEXT;
		is_first = 1;

		/* get the first record number */
		cp->recno = first = meta->first_recno;

		break;
	case DB_PREV:
	case DB_PREV_NODUP:
		if (cp->recno != RECNO_OOB) {
			if (cp->recno == meta->first_recno ||
			   QAM_BEFORE_FIRST(meta, cp->recno)) {
				ret = DB_NOTFOUND;
				goto err;
			}
			--cp->recno;
			/* Wrap around, skipping zero. */
			if (cp->recno == RECNO_OOB)
				--cp->recno;
			break;
		}
		/* FALLTHROUGH */
	case DB_LAST:
		if (meta->first_recno == meta->cur_recno) {
			ret = DB_NOTFOUND;
			goto err;
		}
		cp->recno = meta->cur_recno - 1;
		if (cp->recno == RECNO_OOB)
			cp->recno--;
		break;
	case DB_SET:
	case DB_SET_RANGE:
	case DB_GET_BOTH:
	case DB_GET_BOTH_RANGE:
		if ((ret = __qam_getno(dbp, key, &cp->recno)) != 0)
			goto err;
		break;
	default:
		ret = __db_unknown_flag(env, "__qamc_get", flags);
		goto err;
	}

	/* Don't hold the meta page long term. */
	if (locked) {
		if ((ret = __LPUT(dbc, metalock)) != 0)
			goto err;
		locked = 0;
	}

	if ((ret = __memp_fput(mpf,
	    dbc->thread_info, meta, dbc->priority)) != 0)
		goto err;
	meta = NULL;

	/* Lock the record. */
	if (((ret = __db_lget(dbc, LCK_COUPLE, cp->recno, lock_mode,
	    (with_delete && !inorder && !retrying) ?
	    DB_LOCK_NOWAIT | DB_LOCK_RECORD : DB_LOCK_RECORD,
	    &lock)) == DB_LOCK_DEADLOCK || ret == DB_LOCK_NOTGRANTED) &&
	    with_delete) {
#ifdef QDEBUG
		if (DBC_LOGGING(dbc))
			(void)__log_printf(env,
			    dbc->txn, "Queue S: %x %d %d",
			    dbc->locker ? dbc->locker->id : 0,
			    cp->recno, first);
#endif
		first = 0;
		if ((ret =
		    __db_lget(dbc, 0, metapno, meta_mode, 0, &metalock)) != 0)
			goto err;
		locked = 1;
		if ((ret = __memp_fget(mpf, &metapno,
		     dbc->thread_info, dbc->txn, 0, &meta)) != 0)
			goto err;
		goto retry;
	}

	if (ret != 0)
		goto err;

	/*
	 * In the DB_FIRST or DB_LAST cases we must wait and then start over
	 * since the first/last may have moved while we slept.  If we are
	 * reading in order and the first record was not there, we can skip it
	 * as it must have been aborted was was skipped by a non-queue insert
	 * or we could not have gotten its lock.  If we have the wrong
	 * record we release our locks and try again.
	 */
	switch (flags) {
	default:
		if (inorder) {
			if (first != cp->recno)
				break;
		} else if (with_delete || !is_first)
			break;
		/* FALLTHROUGH */
	case DB_SET:
	case DB_SET_RANGE:
	case DB_GET_BOTH:
	case DB_GET_BOTH_RANGE:
	case DB_LAST:
		if ((ret =
		    __db_lget(dbc, 0, metapno, meta_mode, 0, &metalock)) != 0)
			goto lerr;
		locked = 1;
		if ((ret = __memp_fget(mpf, &metapno,
		     dbc->thread_info, dbc->txn, 0, &meta)) != 0)
			goto lerr;
		if ((is_first && cp->recno != meta->first_recno) ||
		    (flags == DB_LAST && cp->recno != meta->cur_recno - 1)) {
			if ((ret = __LPUT(dbc, lock)) != 0)
				goto err;
			if (is_first)
				flags = DB_FIRST;
			goto retry;
		} else if (!is_first && flags != DB_LAST) {
			if (QAM_BEFORE_FIRST(meta, cp->recno)) {
				if (flags == DB_SET_RANGE ||
				    flags == DB_GET_BOTH_RANGE) {
					if ((ret = __LPUT(dbc, metalock)) != 0)
						goto err;
					locked = 0;
					cp->lock = lock;
					LOCK_INIT(lock);
					goto release_retry;
				}
				ret = DB_NOTFOUND;
				goto lerr;
			}
			if (QAM_AFTER_CURRENT(meta, cp->recno)) {
				ret = DB_NOTFOUND;
				goto lerr;
			}
		}
		/* Don't hold the meta page long term. */
		if ((ret = __LPUT(dbc, metalock)) != 0)
			goto err;
		locked = 0;
		if ((ret = __memp_fput(mpf,
		    dbc->thread_info, meta, dbc->priority)) != 0)
			goto err;
		meta = NULL;
	}

	/* Position the cursor on the record. */
	if ((ret = __qam_position(dbc, &cp->recno, 0, &exact)) != 0) {
		/* We cannot get the page, release the record lock. */
		(void)__LPUT(dbc, lock);
		goto err;
	}

	pg = cp->page;
	cp->lock = lock;
	cp->lock_mode = lock_mode;
	LOCK_INIT(lock);

	if (!exact) {
release_retry:	/* Release locks and retry, if possible. */
		if (pg != NULL)
			(void)__qam_fput(dbc, cp->pgno, pg, dbc->priority);
		cp->page = pg = NULL;
		if (with_delete) {
			if ((ret = __LPUT(dbc, cp->lock)) != 0)
				goto err1;
		} else if ((ret = __TLPUT(dbc, cp->lock)) != 0)
			goto err1;

		if (locked == 0 && (ret =
		    __db_lget(dbc, 0, metapno, meta_mode, 0, &metalock)) != 0)
			goto err1;
		locked = 1;
		if (meta == NULL && (ret = __memp_fget(mpf, &metapno,
		     dbc->thread_info, dbc->txn, 0, &meta)) != 0)
			goto err1;
		/*
		 * If we don't need locks and we are out of range
		 * then we can just skip to the FIRST/LAST record
		 * otherwise we must iterate to lock the records
		 * and get serializability.
		 */
		switch (flags) {
		case DB_NEXT:
		case DB_NEXT_NODUP:
			if (!with_delete)
				is_first = 0;
			if (QAM_BEFORE_FIRST(meta, cp->recno) &&
			    DONT_NEED_LOCKS(dbc))
				flags = DB_FIRST;
			break;
		case DB_LAST:
		case DB_PREV:
		case DB_PREV_NODUP:
			if (QAM_AFTER_CURRENT(meta, cp->recno) &&
			    DONT_NEED_LOCKS(dbc))
				flags = DB_LAST;
			else
				flags = DB_PREV;
			break;

		case DB_GET_BOTH_RANGE:
		case DB_SET_RANGE:
			if (QAM_BEFORE_FIRST(meta, cp->recno) &&
			    DONT_NEED_LOCKS(dbc))
				flags = DB_FIRST;
			else
				flags = DB_NEXT;
			break;

		default:
			/* this is for the SET and GET_BOTH cases */
			ret = DB_KEYEMPTY;
			goto err1;
		}
		retrying = 0;
		goto retry;
	}

	qp = QAM_GET_RECORD(dbp, pg, cp->indx);

	/* Return the data item. */
	if (flags == DB_GET_BOTH || flags == DB_GET_BOTH_RANGE) {
		/*
		 * Need to compare
		 */
		tmp.data = qp->data;
		tmp.size = t->re_len;
		if ((ret = __bam_defcmp(dbp, data, &tmp)) != 0) {
			if (flags == DB_GET_BOTH_RANGE)
				goto release_retry;
			ret = DB_NOTFOUND;
			goto err1;
		}
	}

	/* Return the key if the user didn't give us one. */
	if (key != NULL && !F_ISSET(key, DB_DBT_ISSET)) {
		if ((ret = __db_retcopy(dbp->env,
		    key, &cp->recno, sizeof(cp->recno),
		    &dbc->rkey->data, &dbc->rkey->ulen)) != 0)
			goto err1;
		F_SET(key, DB_DBT_ISSET);
	}

	if (data != NULL &&
	    !F_ISSET(dbc, DBC_MULTIPLE|DBC_MULTIPLE_KEY) &&
	    !F_ISSET(data, DB_DBT_ISSET)) {
		if ((ret = __db_retcopy(dbp->env, data, qp->data, t->re_len,
		    &dbc->rdata->data, &dbc->rdata->ulen)) != 0)
			goto err1;
		F_SET(data, DB_DBT_ISSET);
	}

	/* Finally, if we are doing DB_CONSUME mark the record. */
	if (with_delete) {
		/*
		 * Assert that we're not a secondary index.  Doing a DB_CONSUME
		 * on a secondary makes very little sense, since one can't
		 * DB_APPEND there;  attempting one should be forbidden by
		 * the interface.
		 */
		DB_ASSERT(env, !F_ISSET(dbp, DB_AM_SECONDARY));

		/*
		 * If we have any secondary indices, call __dbc_del_primary to
		 * delete the references to the item we're about to delete.
		 *
		 * Note that we work on a duplicated cursor, since the
		 * __db_ret work has already been done, so it's not safe
		 * to perform any additional ops on this cursor.
		 */
		if (DB_IS_PRIMARY(dbp)) {
			if ((ret = __dbc_idup(dbc,
			    &dbcdup, DB_POSITION)) != 0)
				goto err1;

			if ((ret = __qam_fput(dbc,
			    cp->pgno, cp->page, dbc->priority)) != 0)
				goto err1;
			cp->page = NULL;
			if (meta != NULL &&
			    (ret = __memp_fput(mpf,
			    dbc->thread_info, meta, dbc->priority)) != 0)
				goto err1;
			meta = NULL;
			if ((ret = __dbc_del_primary(dbcdup)) != 0) {
				/*
				 * The __dbc_del_primary return is more
				 * interesting.
				 */
				(void)__dbc_close(dbcdup);
				goto err1;
			}

			if ((ret = __dbc_close(dbcdup)) != 0)
				goto err1;
			if ((ret = __qam_fget(dbc,
			    &cp->pgno, DB_MPOOL_DIRTY, &cp->page)) != 0)
				goto err;
		} else if ((ret = __qam_dirty(dbc,
		     cp->pgno, &cp->page, dbc->priority)) != 0)
			goto err1;

		pg = cp->page;

		if (DBC_LOGGING(dbc)) {
			if (t->page_ext == 0 || t->re_len == 0) {
				if ((ret = __qam_del_log(dbp, dbc->txn,
				    &LSN(pg), 0, &LSN(pg),
				    pg->pgno, cp->indx, cp->recno)) != 0)
					goto err1;
			} else {
				tmp.data = qp->data;
				tmp.size = t->re_len;
				if ((ret = __qam_delext_log(dbp,
				   dbc->txn, &LSN(pg), 0, &LSN(pg),
				   pg->pgno, cp->indx, cp->recno, &tmp)) != 0)
					goto err1;
			}
		} else
			LSN_NOT_LOGGED(LSN(pg));

		F_CLR(qp, QAM_VALID);
		if ((ret = __qam_fput(dbc,
		    cp->pgno, cp->page, dbc->priority)) != 0)
			goto err;
		cp->page = NULL;

		/*
		 * Now we need to update the metapage
		 * first pointer. If we have deleted
		 * the record that is pointed to by
		 * first_recno then we move it as far
		 * forward as we can without blocking.
		 * The metapage lock must be held for
		 * the whole scan otherwise someone could
		 * do a random insert behind where we are
		 * looking.
		 */

		if (locked == 0 && (ret = __db_lget(
		    dbc, 0, metapno, meta_mode, 0, &metalock)) != 0)
			goto err1;
		locked = 1;
		if (meta == NULL &&
		     (ret = __memp_fget(mpf,
		     &metapno, dbc->thread_info, dbc->txn, 0, &meta)) != 0)
			goto err1;

#ifdef QDEBUG
		if (DBC_LOGGING(dbc))
			(void)__log_printf(env,
			    dbc->txn, "Queue D: %x %d %d %d",
			    dbc->locker ? dbc->locker->id : 0,
			    cp->recno, first, meta->first_recno);
#endif
		/*
		 * See if we deleted the "first" record.  If
		 * first is zero then we skipped something,
		 * see if first_recno has been move passed
		 * that to the record that we deleted.
		 */
		if (first == 0)
			first = cp->recno;
		if (first != meta->first_recno)
			goto done;

		if ((ret = __qam_consume(dbc, meta, first)) != 0)
			goto err1;
	}

done:
err1:	if (cp->page != NULL) {
		if ((t_ret = __qam_fput(dbc,
		    cp->pgno, cp->page, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;

		cp->page = NULL;
	}
	if (0) {
lerr:		(void)__LPUT(dbc, lock);
	}

err:	if (meta) {
		/* Release the meta page. */
		if ((t_ret = __memp_fput(mpf,
		    dbc->thread_info, meta, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;

		/* Don't hold the meta page long term. */
		if (locked)
			if ((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
				ret = t_ret;
	}
	DB_ASSERT(env, !LOCK_ISSET(metalock));

	return ((ret == DB_LOCK_NOTGRANTED && !F_ISSET(env->dbenv,
	    DB_ENV_TIME_NOTGRANTED)) ? DB_LOCK_DEADLOCK : ret);
}

/*
 * __qam_consume -- try to reset the head of the queue.
 *
 */
static int
__qam_consume(dbc, meta, first)
	DBC *dbc;
	QMETA *meta;
	db_recno_t first;
{
	DB *dbp;
	DB_LOCK lock, save_lock;
	DB_MPOOLFILE *mpf;
	QUEUE_CURSOR *cp;
	db_indx_t save_indx;
	db_pgno_t save_page;
	db_recno_t current, save_recno;
	u_int32_t rec_extent;
	int exact, ret, t_ret, wrapped;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (QUEUE_CURSOR *)dbc->internal;
	ret = 0;

	save_page = cp->pgno;
	save_indx = cp->indx;
	save_recno = cp->recno;
	save_lock = cp->lock;

	/*
	 * If we skipped some deleted records, we need to
	 * reposition on the first one.  Get a lock
	 * in case someone is trying to put it back.
	 */
	if (first != cp->recno) {
		ret = __db_lget(dbc, 0, first, DB_LOCK_READ,
		    DB_LOCK_NOWAIT | DB_LOCK_RECORD, &lock);
		if (ret == DB_LOCK_NOTGRANTED || ret == DB_LOCK_DEADLOCK) {
			ret = 0;
			goto done;
		}
		if (ret != 0)
			goto done;
		if (cp->page != NULL && (ret =
		    __qam_fput(dbc, cp->pgno, cp->page, dbc->priority)) != 0)
			goto done;
		cp->page = NULL;
		if ((ret = __qam_position(dbc,
		    &first, 0, &exact)) != 0 || exact != 0) {
			(void)__LPUT(dbc, lock);
			goto done;
		}
		if ((ret =__LPUT(dbc, lock)) != 0)
			goto done;
	}

	current = meta->cur_recno;
	wrapped = 0;
	if (first > current)
		wrapped = 1;
	rec_extent = meta->page_ext * meta->rec_page;

	/* Loop until we find a record or hit current */
	for (;;) {
		/*
		 * Check to see if we are moving off the extent
		 * and remove the extent.
		 * If we are moving off a page we need to
		 * get rid of the buffer.
		 * Wait for the lagging readers to move off the
		 * page.
		 */
		if (rec_extent != 0 &&
		    ((exact = (first % rec_extent == 0)) ||
		    (first % meta->rec_page == 0) ||
		    first == UINT32_MAX)) {
			if (exact == 1 && (ret = __db_lget(dbc,
			    0, cp->pgno, DB_LOCK_WRITE, 0, &cp->lock)) != 0)
				break;
#ifdef QDEBUG
			if (DBC_LOGGING(dbc))
				(void)__log_printf(dbp->env, dbc->txn,
				    "Queue R: %x %d %d %d",
				    dbc->locker ? dbc->locker->id : 0,
				    cp->pgno, first, meta->first_recno);
#endif
			if (cp->page != NULL && (ret = __qam_fput(dbc,
			    cp->pgno, cp->page, DB_PRIORITY_VERY_LOW)) != 0)
				break;
			cp->page = NULL;

			if (exact == 1) {
				ret = __qam_fremove(dbp, cp->pgno);
				if ((t_ret =
				    __LPUT(dbc, cp->lock)) != 0 && ret == 0)
					ret = t_ret;
			}
			if (ret != 0)
				break;
		} else if (cp->page != NULL && (ret = __qam_fput(dbc,
		     cp->pgno, cp->page, dbc->priority)) != 0)
			break;
		cp->page = NULL;
		first++;
		if (first == RECNO_OOB) {
			wrapped = 0;
			first++;
		}

		/*
		 * LOOP EXIT when we come move to the current
		 * pointer.
		 */
		if (!wrapped && first >= current)
			break;

		ret = __db_lget(dbc, 0, first, DB_LOCK_READ,
		    DB_LOCK_NOWAIT | DB_LOCK_RECORD, &lock);
		if (ret == DB_LOCK_NOTGRANTED || ret == DB_LOCK_DEADLOCK) {
			ret = 0;
			break;
		}
		if (ret != 0)
			break;

		if ((ret = __qam_position(dbc, &first, 0, &exact)) != 0) {
			(void)__LPUT(dbc, lock);
			break;
		}
		if ((ret =__LPUT(dbc, lock)) != 0 || exact) {
			if ((t_ret = __qam_fput(dbc, cp->pgno,
			    cp->page, dbc->priority)) != 0 && ret == 0)
				ret = t_ret;
			cp->page = NULL;
			break;
		}
	}

	cp->pgno = save_page;
	cp->indx = save_indx;
	cp->recno = save_recno;
	cp->lock = save_lock;

	/*
	 * We have advanced as far as we can.
	 * Advance first_recno to this point.
	 */
	if (ret == 0 && meta->first_recno != first) {
		if ((ret = __memp_dirty(mpf,
		    &meta, dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
			goto done;
#ifdef QDEBUG
		if (DBC_LOGGING(dbc))
			(void)__log_printf(dbp->env, dbc->txn,
			    "Queue M: %x %d %d %d",
			    dbc->locker ? dbc->locker->id : 0,
			    cp->recno, first, meta->first_recno);
#endif
		if (DBC_LOGGING(dbc)) {
			if ((ret = __qam_incfirst_log(dbp,
			    dbc->txn, &meta->dbmeta.lsn, 0,
			    cp->recno, PGNO_BASE_MD)) != 0)
				goto done;
		} else
			LSN_NOT_LOGGED(meta->dbmeta.lsn);
		meta->first_recno = first;
	}

done:
	return (ret);
}

static int
__qam_bulk(dbc, data, flags)
	DBC *dbc;
	DBT *data;
	u_int32_t flags;
{
	DB *dbp;
	DB_LOCK metalock, rlock;
	DB_MPOOLFILE *mpf;
	PAGE *pg;
	QAMDATA *qp;
	QMETA *meta;
	QUEUE_CURSOR *cp;
	db_indx_t indx;
	db_lockmode_t lkmode;
	db_pgno_t metapno;
	u_int32_t  *endp, *offp;
	u_int32_t pagesize, re_len, recs;
	u_int8_t *dbuf, *dp, *np;
	int exact, ret, t_ret, valid;
	int is_key, need_pg, size, space;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (QUEUE_CURSOR *)dbc->internal;

	lkmode = F_ISSET(dbc, DBC_RMW) ? DB_LOCK_WRITE : DB_LOCK_READ;

	pagesize = dbp->pgsize;
	re_len = ((QUEUE *)dbp->q_internal)->re_len;
	recs = ((QUEUE *)dbp->q_internal)->rec_page;
	metapno = ((QUEUE *)dbp->q_internal)->q_meta;

	is_key = LF_ISSET(DB_MULTIPLE_KEY) ? 1 : 0;
	size = 0;

	if ((ret = __db_lget(dbc, 0, metapno, DB_LOCK_READ, 0, &metalock)) != 0)
		return (ret);
	if ((ret = __memp_fget(mpf, &metapno,
	     dbc->thread_info, dbc->txn, 0, &meta)) != 0) {
		/* We did not fetch it, we can release the lock. */
		(void)__LPUT(dbc, metalock);
		return (ret);
	}

	dbuf = data->data;
	np = dp = dbuf;

	/* Keep track of space that is left.  There is an termination entry */
	space = (int)data->ulen;
	space -= (int)sizeof(*offp);

	/* Build the offset/size table from the end up. */
	endp = (u_int32_t *)((u_int8_t *)dbuf + data->ulen);
	endp--;
	offp = endp;
	/* Save the lock on the current position of the cursor. */
	rlock = cp->lock;
	LOCK_INIT(cp->lock);

next_pg:
	/* Wrap around, skipping zero. */
	if (cp->recno == RECNO_OOB)
		cp->recno++;
	if ((ret = __qam_position(dbc, &cp->recno, 0, &exact)) != 0)
		goto done;

	pg = cp->page;
	indx = cp->indx;
	need_pg = 1;

	do {
		/*
		 * If this page is a nonexistent page at the end of an
		 * extent, pg may be NULL.  A NULL page has no valid records,
		 * so just keep looping as though qp exists and isn't QAM_VALID;
		 * calling QAM_GET_RECORD is unsafe.
		 */
		valid = 0;

		if (pg != NULL) {
			if ((ret = __db_lget(dbc, LCK_COUPLE,
			    cp->recno, lkmode, DB_LOCK_RECORD, &rlock)) != 0)
				goto done;
			qp = QAM_GET_RECORD(dbp, pg, indx);
			if (F_ISSET(qp, QAM_VALID)) {
				valid = 1;
				space -= (int)
				    ((is_key ? 3 : 2) * sizeof(*offp));
				if (space < 0)
					goto get_space;
				if (need_pg) {
					dp = np;
					size = (int)pagesize - QPAGE_SZ(dbp);
					if (space < size) {
get_space:
						if (offp == endp) {
							data->size = (u_int32_t)
							    DB_ALIGN((u_int32_t)
							    size + pagesize,
							    sizeof(u_int32_t));
							ret = DB_BUFFER_SMALL;
							break;
						}
						if (indx != 0)
							indx--;
						cp->recno--;
						space = 0;
						break;
					}
					memcpy(dp,
					    (u_int8_t *)pg + QPAGE_SZ(dbp),
					    (u_int)size);
					need_pg = 0;
					space -= size;
					np += size;
				}
				if (is_key)
					*offp-- = cp->recno;
				*offp-- = (u_int32_t)((((u_int8_t *)qp -
				    (u_int8_t *)pg) - QPAGE_SZ(dbp)) +
				    (dp - dbuf) + SSZA(QAMDATA, data));
				*offp-- = re_len;
			}
		}
		if (!valid && is_key == 0) {
			*offp-- = 0;
			*offp-- = 0;
		}
		cp->recno++;
	} while (++indx < recs && cp->recno != RECNO_OOB &&
	    !QAM_AFTER_CURRENT(meta, cp->recno));

	if (cp->page != NULL) {
		if ((t_ret = __qam_fput(dbc,
		    cp->pgno, cp->page, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
		cp->page = NULL;
	}

	if (ret == 0 && space > 0 &&
	    (indx >= recs || cp->recno == RECNO_OOB) &&
	    !QAM_AFTER_CURRENT(meta, cp->recno))
		goto next_pg;

	/*
	 * Correct recno in two cases:
	 * 1) If we just wrapped fetch must start at record 1 not a FIRST.
	 * 2) We ran out of space exactly at the end of a page.
	 */
	if (cp->recno == RECNO_OOB || (space == 0 && indx == recs))
		cp->recno--;

	if (is_key == 1)
		*offp = RECNO_OOB;
	else
		*offp = (u_int32_t)-1;

done:	/* Release the meta page. */
	if ((t_ret = __memp_fput(mpf,
	     dbc->thread_info, meta, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
		ret = t_ret;

	cp->lock = rlock;

	return (ret);
}

/*
 * __qamc_close --
 *	Close down the cursor from a single use.
 */
static int
__qamc_close(dbc, root_pgno, rmroot)
	DBC *dbc;
	db_pgno_t root_pgno;
	int *rmroot;
{
	QUEUE_CURSOR *cp;
	int ret;

	COMPQUIET(root_pgno, 0);
	COMPQUIET(rmroot, NULL);

	cp = (QUEUE_CURSOR *)dbc->internal;

	/* Discard any locks not acquired inside of a transaction. */
	ret = __TLPUT(dbc, cp->lock);

	LOCK_INIT(cp->lock);
	cp->page = NULL;
	cp->pgno = PGNO_INVALID;
	cp->indx = 0;
	cp->lock_mode = DB_LOCK_NG;
	cp->recno = RECNO_OOB;
	cp->flags = 0;

	return (ret);
}

/*
 * __qamc_dup --
 *	Duplicate a queue cursor, such that the new one holds appropriate
 *	locks for the position of the original.
 *
 * PUBLIC: int __qamc_dup __P((DBC *, DBC *));
 */
int
__qamc_dup(orig_dbc, new_dbc)
	DBC *orig_dbc, *new_dbc;
{
	QUEUE_CURSOR *orig, *new;

	orig = (QUEUE_CURSOR *)orig_dbc->internal;
	new = (QUEUE_CURSOR *)new_dbc->internal;

	new->recno = orig->recno;

	return (0);
}

/*
 * __qamc_init
 *
 * PUBLIC: int __qamc_init __P((DBC *));
 */
int
__qamc_init(dbc)
	DBC *dbc;
{
	DB *dbp;
	QUEUE_CURSOR *cp;
	int ret;

	dbp = dbc->dbp;

	/* Allocate the internal structure. */
	cp = (QUEUE_CURSOR *)dbc->internal;
	if (cp == NULL) {
		if ((ret =
		    __os_calloc(dbp->env, 1, sizeof(QUEUE_CURSOR), &cp)) != 0)
			return (ret);
		dbc->internal = (DBC_INTERNAL *)cp;
	}

	/* Initialize methods. */
	dbc->close = dbc->c_close = __dbc_close_pp;
	dbc->cmp = __dbc_cmp_pp;
	dbc->count = dbc->c_count = __dbc_count_pp;
	dbc->del = dbc->c_del = __dbc_del_pp;
	dbc->dup = dbc->c_dup = __dbc_dup_pp;
	dbc->get = dbc->c_get = __dbc_get_pp;
	dbc->pget = dbc->c_pget = __dbc_pget_pp;
	dbc->put = dbc->c_put = __dbc_put_pp;
	dbc->am_bulk = __qam_bulk;
	dbc->am_close = __qamc_close;
	dbc->am_del = __qamc_del;
	dbc->am_destroy = __qamc_destroy;
	dbc->am_get = __qamc_get;
	dbc->am_put = __qamc_put;
	dbc->am_writelock = NULL;

	return (0);
}

/*
 * __qamc_destroy --
 *	Close a single cursor -- internal version.
 */
static int
__qamc_destroy(dbc)
	DBC *dbc;
{
	/* Discard the structures. */
	__os_free(dbc->env, dbc->internal);

	return (0);
}

/*
 * __qam_getno --
 *	Check the user's record number.
 */
static int
__qam_getno(dbp, key, rep)
	DB *dbp;
	const DBT *key;
	db_recno_t *rep;
{
	/* If passed an empty DBT from Java, key->data may be NULL */
	if (key->size != sizeof(db_recno_t)) {
		__db_errx(dbp->env, "illegal record number size");
		return (EINVAL);
	}

	if ((*rep = *(db_recno_t *)key->data) == 0) {
		__db_errx(dbp->env, "illegal record number of 0");
		return (EINVAL);
	}
	return (0);
}

/*
 * __qam_truncate --
 *	Truncate a queue database
 *
 * PUBLIC: int __qam_truncate __P((DBC *, u_int32_t *));
 */
int
__qam_truncate(dbc, countp)
	DBC *dbc;
	u_int32_t *countp;
{
	DB *dbp;
	DB_LOCK metalock;
	DB_MPOOLFILE *mpf;
	QMETA *meta;
	db_pgno_t metapno;
	u_int32_t count;
	int ret, t_ret;

	dbp = dbc->dbp;

	/* Walk the queue, counting rows. */
	for (count = 0;
	    (ret = __qamc_get(dbc, NULL, NULL, DB_CONSUME, &metapno)) == 0;)
		count++;
	if (ret != DB_NOTFOUND)
		return (ret);

	/* Update the meta page. */
	metapno = ((QUEUE *)dbp->q_internal)->q_meta;
	if ((ret =
	    __db_lget(dbc, 0, metapno, DB_LOCK_WRITE, 0, &metalock)) != 0)
		return (ret);

	mpf = dbp->mpf;
	if ((ret = __memp_fget(mpf, &metapno, dbc->thread_info, dbc->txn,
	    DB_MPOOL_DIRTY, &meta)) != 0) {
		/* We did not fetch it, we can release the lock. */
		(void)__LPUT(dbc, metalock);
		return (ret);
	}
	/* Remove the last extent file. */
	if (meta->cur_recno > 1 && ((QUEUE *)dbp->q_internal)->page_ext != 0) {
		if ((ret = __qam_fremove(dbp,
		    QAM_RECNO_PAGE(dbp, meta->cur_recno - 1))) != 0)
			goto err;
	}

	if (DBC_LOGGING(dbc)) {
		ret = __qam_mvptr_log(dbp, dbc->txn, &meta->dbmeta.lsn, 0,
		    QAM_SETCUR | QAM_SETFIRST | QAM_TRUNCATE, meta->first_recno,
		    1, meta->cur_recno, 1, &meta->dbmeta.lsn, PGNO_BASE_MD);
	} else
		LSN_NOT_LOGGED(meta->dbmeta.lsn);
	if (ret == 0)
		meta->first_recno = meta->cur_recno = 1;

err:	if ((t_ret = __memp_fput(mpf,
	     dbc->thread_info, meta, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __LPUT(dbc, metalock)) != 0 && ret == 0)
		ret = t_ret;

	if (countp != NULL)
		*countp = count;

	return (ret);
}

/*
 * __qam_delete --
 *	Queue fast delete function.
 *
 * PUBLIC: int __qam_delete __P((DBC *,  DBT *, u_int32_t));
 */
int
__qam_delete(dbc, key, flags)
	DBC *dbc;
	DBT *key;
	u_int32_t flags;
{
	QUEUE_CURSOR *cp;
	int ret;

	cp = (QUEUE_CURSOR *)dbc->internal;
	if ((ret = __qam_getno(dbc->dbp, key, &cp->recno)) != 0)
		goto err;

	ret = __qamc_del(dbc, flags);

err:	return (ret);
}
