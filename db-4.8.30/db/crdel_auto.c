/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"
#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_dispatch.h"
#include "dbinc/db_am.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

/*
 * PUBLIC: int __crdel_metasub_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __crdel_metasub_args **));
 */
int
__crdel_metasub_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__crdel_metasub_args **argpp;
{
	__crdel_metasub_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__crdel_metasub_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	argp->txnp->td = td;
	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);
	if (dbpp != NULL) {
		*dbpp = NULL;
		ret = __dbreg_id_to_db(
		    env, argp->txnp, dbpp, argp->fileid, 1);
	}

	LOGCOPY_32(env, &uinttmp, bp);
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->page, 0, sizeof(argp->page));
	LOGCOPY_32(env,&argp->page.size, bp);
	bp += sizeof(u_int32_t);
	argp->page.data = bp;
	bp += argp->page.size;
	if (LOG_SWAPPED(env) && dbpp != NULL && *dbpp != NULL) {
		int t_ret;
		if ((t_ret = __db_pageswap(*dbpp, (PAGE *)argp->page.data,
		    (size_t)argp->page.size, NULL, 1)) != 0)
			return (t_ret);
	}

	LOGCOPY_TOLSN(env, &argp->lsn, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __crdel_metasub_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, const DBT *, DB_LSN *));
 */
int
__crdel_metasub_log(dbp, txnp, ret_lsnp, flags, pgno, page, lsn)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	const DBT *page;
	DB_LSN * lsn;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	ENV *env;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	env = dbp->env;
	rlsnp = ret_lsnp;
	rectype = DB___crdel_metasub;
	npad = 0;
	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE) ||
	    F_ISSET(dbp, DB_AM_NOT_DURABLE)) {
		if (txnp == NULL)
			return (0);
		is_durable = 0;
	} else
		is_durable = 1;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		if (TAILQ_FIRST(&txnp->kids) != NULL &&
		    (ret = __txn_activekids(env, rectype, txnp)) != 0)
			return (ret);
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	DB_ASSERT(env, dbp->log_filename != NULL);
	if (dbp->log_filename->id == DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_lazy_id(dbp)) != 0)
		return (ret);

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (page == NULL ? 0 : page->size)
	    + sizeof(*lsn);
	if (CRYPTO_ON(env)) {
		npad = env->crypto_handle->adj_size(logrec.size);
		logrec.size += npad;
	}

	if (is_durable || txnp == NULL) {
		if ((ret =
		    __os_malloc(env, logrec.size, &logrec.data)) != 0)
			return (ret);
	} else {
		if ((ret = __os_malloc(env,
		    logrec.size + sizeof(DB_TXNLOGREC), &lr)) != 0)
			return (ret);
#ifdef DIAGNOSTIC
		if ((ret =
		    __os_malloc(env, logrec.size, &logrec.data)) != 0) {
			__os_free(env, lr);
			return (ret);
		}
#else
		logrec.data = lr->data;
#endif
	}
	if (npad > 0)
		memset((u_int8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (page == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &page->size);
		bp += sizeof(page->size);
		memcpy(bp, page->data, page->size);
		if (LOG_SWAPPED(env))
			if ((ret = __db_pageswap(dbp,
			    (PAGE *)bp, (size_t)page->size, (DBT *)NULL, 0)) != 0)
				return (ret);
		bp += page->size;
	}

	if (lsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(lsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, lsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, lsn);
	} else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);

	DB_ASSERT(env,
	    (u_int32_t)(bp - (u_int8_t *)logrec.data) <= logrec.size);

	if (is_durable || txnp == NULL) {
		if ((ret = __log_put(env, rlsnp,(DBT *)&logrec,
		    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
			*lsnp = *rlsnp;
			if (rlsnp != ret_lsnp)
				 *ret_lsnp = *rlsnp;
		}
	} else {
		ret = 0;
#ifdef DIAGNOSTIC
		/*
		 * Set the debug bit if we are going to log non-durable
		 * transactions so they will be ignored by recovery.
		 */
		memcpy(lr->data, logrec.data, logrec.size);
		rectype |= DB_debug_FLAG;
		LOGCOPY_32(env, logrec.data, &rectype);

		if (!IS_REP_CLIENT(env))
			ret = __log_put(env,
			    rlsnp, (DBT *)&logrec, flags | DB_LOG_NOCOPY);
#endif
		STAILQ_INSERT_HEAD(&txnp->logs, lr, links);
		F_SET((TXN_DETAIL *)txnp->td, TXN_DTL_INMEMORY);
		LSN_NOT_LOGGED(*ret_lsnp);
	}

#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)__crdel_metasub_print(env,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT, NULL);
#endif

#ifdef DIAGNOSTIC
	__os_free(env, logrec.data);
#else
	if (is_durable || txnp == NULL)
		__os_free(env, logrec.data);
#endif
	return (ret);
}

/*
 * PUBLIC: int __crdel_inmem_create_read __P((ENV *, void *,
 * PUBLIC:     __crdel_inmem_create_args **));
 */
int
__crdel_inmem_create_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__crdel_inmem_create_args **argpp;
{
	__crdel_inmem_create_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__crdel_inmem_create_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->name, 0, sizeof(argp->name));
	LOGCOPY_32(env,&argp->name.size, bp);
	bp += sizeof(u_int32_t);
	argp->name.data = bp;
	bp += argp->name.size;

	memset(&argp->fid, 0, sizeof(argp->fid));
	LOGCOPY_32(env,&argp->fid.size, bp);
	bp += sizeof(u_int32_t);
	argp->fid.data = bp;
	bp += argp->fid.size;

	LOGCOPY_32(env, &argp->pgsize, bp);
	bp += sizeof(argp->pgsize);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __crdel_inmem_create_log __P((ENV *, DB_TXN *,
 * PUBLIC:     DB_LSN *, u_int32_t, int32_t, const DBT *, const DBT *,
 * PUBLIC:     u_int32_t));
 */
int
__crdel_inmem_create_log(env, txnp, ret_lsnp, flags,
    fileid, name, fid, pgsize)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	const DBT *name;
	const DBT *fid;
	u_int32_t pgsize;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	u_int32_t zero, uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rlsnp = ret_lsnp;
	rectype = DB___crdel_inmem_create;
	npad = 0;
	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE)) {
		if (txnp == NULL)
			return (0);
		is_durable = 0;
	} else
		is_durable = 1;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		if (TAILQ_FIRST(&txnp->kids) != NULL &&
		    (ret = __txn_activekids(env, rectype, txnp)) != 0)
			return (ret);
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (name == NULL ? 0 : name->size)
	    + sizeof(u_int32_t) + (fid == NULL ? 0 : fid->size)
	    + sizeof(u_int32_t);
	if (CRYPTO_ON(env)) {
		npad = env->crypto_handle->adj_size(logrec.size);
		logrec.size += npad;
	}

	if (is_durable || txnp == NULL) {
		if ((ret =
		    __os_malloc(env, logrec.size, &logrec.data)) != 0)
			return (ret);
	} else {
		if ((ret = __os_malloc(env,
		    logrec.size + sizeof(DB_TXNLOGREC), &lr)) != 0)
			return (ret);
#ifdef DIAGNOSTIC
		if ((ret =
		    __os_malloc(env, logrec.size, &logrec.data)) != 0) {
			__os_free(env, lr);
			return (ret);
		}
#else
		logrec.data = lr->data;
#endif
	}
	if (npad > 0)
		memset((u_int8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	uinttmp = (u_int32_t)fileid;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (name == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &name->size);
		bp += sizeof(name->size);
		memcpy(bp, name->data, name->size);
		bp += name->size;
	}

	if (fid == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &fid->size);
		bp += sizeof(fid->size);
		memcpy(bp, fid->data, fid->size);
		bp += fid->size;
	}

	LOGCOPY_32(env, bp, &pgsize);
	bp += sizeof(pgsize);

	DB_ASSERT(env,
	    (u_int32_t)(bp - (u_int8_t *)logrec.data) <= logrec.size);

	if (is_durable || txnp == NULL) {
		if ((ret = __log_put(env, rlsnp,(DBT *)&logrec,
		    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
			*lsnp = *rlsnp;
			if (rlsnp != ret_lsnp)
				 *ret_lsnp = *rlsnp;
		}
	} else {
		ret = 0;
#ifdef DIAGNOSTIC
		/*
		 * Set the debug bit if we are going to log non-durable
		 * transactions so they will be ignored by recovery.
		 */
		memcpy(lr->data, logrec.data, logrec.size);
		rectype |= DB_debug_FLAG;
		LOGCOPY_32(env, logrec.data, &rectype);

		if (!IS_REP_CLIENT(env))
			ret = __log_put(env,
			    rlsnp, (DBT *)&logrec, flags | DB_LOG_NOCOPY);
#endif
		STAILQ_INSERT_HEAD(&txnp->logs, lr, links);
		F_SET((TXN_DETAIL *)txnp->td, TXN_DTL_INMEMORY);
		LSN_NOT_LOGGED(*ret_lsnp);
	}

#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)__crdel_inmem_create_print(env,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT, NULL);
#endif

#ifdef DIAGNOSTIC
	__os_free(env, logrec.data);
#else
	if (is_durable || txnp == NULL)
		__os_free(env, logrec.data);
#endif
	return (ret);
}

/*
 * PUBLIC: int __crdel_inmem_rename_read __P((ENV *, void *,
 * PUBLIC:     __crdel_inmem_rename_args **));
 */
int
__crdel_inmem_rename_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__crdel_inmem_rename_args **argpp;
{
	__crdel_inmem_rename_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__crdel_inmem_rename_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->oldname, 0, sizeof(argp->oldname));
	LOGCOPY_32(env,&argp->oldname.size, bp);
	bp += sizeof(u_int32_t);
	argp->oldname.data = bp;
	bp += argp->oldname.size;

	memset(&argp->newname, 0, sizeof(argp->newname));
	LOGCOPY_32(env,&argp->newname.size, bp);
	bp += sizeof(u_int32_t);
	argp->newname.data = bp;
	bp += argp->newname.size;

	memset(&argp->fid, 0, sizeof(argp->fid));
	LOGCOPY_32(env,&argp->fid.size, bp);
	bp += sizeof(u_int32_t);
	argp->fid.data = bp;
	bp += argp->fid.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __crdel_inmem_rename_log __P((ENV *, DB_TXN *,
 * PUBLIC:     DB_LSN *, u_int32_t, const DBT *, const DBT *, const DBT *));
 */
int
__crdel_inmem_rename_log(env, txnp, ret_lsnp, flags,
    oldname, newname, fid)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *oldname;
	const DBT *newname;
	const DBT *fid;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	u_int32_t zero, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rlsnp = ret_lsnp;
	rectype = DB___crdel_inmem_rename;
	npad = 0;
	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE)) {
		if (txnp == NULL)
			return (0);
		is_durable = 0;
	} else
		is_durable = 1;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		if (TAILQ_FIRST(&txnp->kids) != NULL &&
		    (ret = __txn_activekids(env, rectype, txnp)) != 0)
			return (ret);
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t) + (oldname == NULL ? 0 : oldname->size)
	    + sizeof(u_int32_t) + (newname == NULL ? 0 : newname->size)
	    + sizeof(u_int32_t) + (fid == NULL ? 0 : fid->size);
	if (CRYPTO_ON(env)) {
		npad = env->crypto_handle->adj_size(logrec.size);
		logrec.size += npad;
	}

	if (is_durable || txnp == NULL) {
		if ((ret =
		    __os_malloc(env, logrec.size, &logrec.data)) != 0)
			return (ret);
	} else {
		if ((ret = __os_malloc(env,
		    logrec.size + sizeof(DB_TXNLOGREC), &lr)) != 0)
			return (ret);
#ifdef DIAGNOSTIC
		if ((ret =
		    __os_malloc(env, logrec.size, &logrec.data)) != 0) {
			__os_free(env, lr);
			return (ret);
		}
#else
		logrec.data = lr->data;
#endif
	}
	if (npad > 0)
		memset((u_int8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (oldname == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &oldname->size);
		bp += sizeof(oldname->size);
		memcpy(bp, oldname->data, oldname->size);
		bp += oldname->size;
	}

	if (newname == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &newname->size);
		bp += sizeof(newname->size);
		memcpy(bp, newname->data, newname->size);
		bp += newname->size;
	}

	if (fid == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &fid->size);
		bp += sizeof(fid->size);
		memcpy(bp, fid->data, fid->size);
		bp += fid->size;
	}

	DB_ASSERT(env,
	    (u_int32_t)(bp - (u_int8_t *)logrec.data) <= logrec.size);

	if (is_durable || txnp == NULL) {
		if ((ret = __log_put(env, rlsnp,(DBT *)&logrec,
		    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
			*lsnp = *rlsnp;
			if (rlsnp != ret_lsnp)
				 *ret_lsnp = *rlsnp;
		}
	} else {
		ret = 0;
#ifdef DIAGNOSTIC
		/*
		 * Set the debug bit if we are going to log non-durable
		 * transactions so they will be ignored by recovery.
		 */
		memcpy(lr->data, logrec.data, logrec.size);
		rectype |= DB_debug_FLAG;
		LOGCOPY_32(env, logrec.data, &rectype);

		if (!IS_REP_CLIENT(env))
			ret = __log_put(env,
			    rlsnp, (DBT *)&logrec, flags | DB_LOG_NOCOPY);
#endif
		STAILQ_INSERT_HEAD(&txnp->logs, lr, links);
		F_SET((TXN_DETAIL *)txnp->td, TXN_DTL_INMEMORY);
		LSN_NOT_LOGGED(*ret_lsnp);
	}

#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)__crdel_inmem_rename_print(env,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT, NULL);
#endif

#ifdef DIAGNOSTIC
	__os_free(env, logrec.data);
#else
	if (is_durable || txnp == NULL)
		__os_free(env, logrec.data);
#endif
	return (ret);
}

/*
 * PUBLIC: int __crdel_inmem_remove_read __P((ENV *, void *,
 * PUBLIC:     __crdel_inmem_remove_args **));
 */
int
__crdel_inmem_remove_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__crdel_inmem_remove_args **argpp;
{
	__crdel_inmem_remove_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__crdel_inmem_remove_args) + sizeof(DB_TXN), &argp)) != 0)
		return (ret);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->name, 0, sizeof(argp->name));
	LOGCOPY_32(env,&argp->name.size, bp);
	bp += sizeof(u_int32_t);
	argp->name.data = bp;
	bp += argp->name.size;

	memset(&argp->fid, 0, sizeof(argp->fid));
	LOGCOPY_32(env,&argp->fid.size, bp);
	bp += sizeof(u_int32_t);
	argp->fid.data = bp;
	bp += argp->fid.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __crdel_inmem_remove_log __P((ENV *, DB_TXN *,
 * PUBLIC:     DB_LSN *, u_int32_t, const DBT *, const DBT *));
 */
int
__crdel_inmem_remove_log(env, txnp, ret_lsnp, flags,
    name, fid)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *name;
	const DBT *fid;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	u_int32_t zero, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rlsnp = ret_lsnp;
	rectype = DB___crdel_inmem_remove;
	npad = 0;
	ret = 0;

	if (LF_ISSET(DB_LOG_NOT_DURABLE)) {
		if (txnp == NULL)
			return (0);
		is_durable = 0;
	} else
		is_durable = 1;

	if (txnp == NULL) {
		txn_num = 0;
		lsnp = &null_lsn;
		null_lsn.file = null_lsn.offset = 0;
	} else {
		if (TAILQ_FIRST(&txnp->kids) != NULL &&
		    (ret = __txn_activekids(env, rectype, txnp)) != 0)
			return (ret);
		/*
		 * We need to assign begin_lsn while holding region mutex.
		 * That assignment is done inside the DbEnv->log_put call,
		 * so pass in the appropriate memory location to be filled
		 * in by the log_put code.
		 */
		DB_SET_TXN_LSNP(txnp, &rlsnp, &lsnp);
		txn_num = txnp->txnid;
	}

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(u_int32_t) + (name == NULL ? 0 : name->size)
	    + sizeof(u_int32_t) + (fid == NULL ? 0 : fid->size);
	if (CRYPTO_ON(env)) {
		npad = env->crypto_handle->adj_size(logrec.size);
		logrec.size += npad;
	}

	if (is_durable || txnp == NULL) {
		if ((ret =
		    __os_malloc(env, logrec.size, &logrec.data)) != 0)
			return (ret);
	} else {
		if ((ret = __os_malloc(env,
		    logrec.size + sizeof(DB_TXNLOGREC), &lr)) != 0)
			return (ret);
#ifdef DIAGNOSTIC
		if ((ret =
		    __os_malloc(env, logrec.size, &logrec.data)) != 0) {
			__os_free(env, lr);
			return (ret);
		}
#else
		logrec.data = lr->data;
#endif
	}
	if (npad > 0)
		memset((u_int8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

	if (name == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &name->size);
		bp += sizeof(name->size);
		memcpy(bp, name->data, name->size);
		bp += name->size;
	}

	if (fid == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &fid->size);
		bp += sizeof(fid->size);
		memcpy(bp, fid->data, fid->size);
		bp += fid->size;
	}

	DB_ASSERT(env,
	    (u_int32_t)(bp - (u_int8_t *)logrec.data) <= logrec.size);

	if (is_durable || txnp == NULL) {
		if ((ret = __log_put(env, rlsnp,(DBT *)&logrec,
		    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
			*lsnp = *rlsnp;
			if (rlsnp != ret_lsnp)
				 *ret_lsnp = *rlsnp;
		}
	} else {
		ret = 0;
#ifdef DIAGNOSTIC
		/*
		 * Set the debug bit if we are going to log non-durable
		 * transactions so they will be ignored by recovery.
		 */
		memcpy(lr->data, logrec.data, logrec.size);
		rectype |= DB_debug_FLAG;
		LOGCOPY_32(env, logrec.data, &rectype);

		if (!IS_REP_CLIENT(env))
			ret = __log_put(env,
			    rlsnp, (DBT *)&logrec, flags | DB_LOG_NOCOPY);
#endif
		STAILQ_INSERT_HEAD(&txnp->logs, lr, links);
		F_SET((TXN_DETAIL *)txnp->td, TXN_DTL_INMEMORY);
		LSN_NOT_LOGGED(*ret_lsnp);
	}

#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)__crdel_inmem_remove_print(env,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT, NULL);
#endif

#ifdef DIAGNOSTIC
	__os_free(env, logrec.data);
#else
	if (is_durable || txnp == NULL)
		__os_free(env, logrec.data);
#endif
	return (ret);
}

/*
 * PUBLIC: int __crdel_init_recover __P((ENV *, DB_DISTAB *));
 */
int
__crdel_init_recover(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __crdel_metasub_recover, DB___crdel_metasub)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __crdel_inmem_create_recover, DB___crdel_inmem_create)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __crdel_inmem_rename_recover, DB___crdel_inmem_rename)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __crdel_inmem_remove_recover, DB___crdel_inmem_remove)) != 0)
		return (ret);
	return (0);
}
