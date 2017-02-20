/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"
#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"
#include "dbinc/fop.h"

/*
 * PUBLIC: int __fop_create_42_read __P((ENV *, void *,
 * PUBLIC:     __fop_create_42_args **));
 */
int
__fop_create_42_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__fop_create_42_args **argpp;
{
	__fop_create_42_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__fop_create_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->appname, bp);
	bp += sizeof(argp->appname);

	LOGCOPY_32(env, &argp->mode, bp);
	bp += sizeof(argp->mode);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __fop_create_read __P((ENV *, void *, __fop_create_args **));
 */
int
__fop_create_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__fop_create_args **argpp;
{
	__fop_create_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__fop_create_args) + sizeof(DB_TXN), &argp)) != 0)
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

	memset(&argp->dirname, 0, sizeof(argp->dirname));
	LOGCOPY_32(env,&argp->dirname.size, bp);
	bp += sizeof(u_int32_t);
	argp->dirname.data = bp;
	bp += argp->dirname.size;

	LOGCOPY_32(env, &argp->appname, bp);
	bp += sizeof(argp->appname);

	LOGCOPY_32(env, &argp->mode, bp);
	bp += sizeof(argp->mode);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __fop_create_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, const DBT *, u_int32_t, u_int32_t));
 */
int
__fop_create_log(env, txnp, ret_lsnp, flags,
    name, dirname, appname, mode)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *name;
	const DBT *dirname;
	u_int32_t appname;
	u_int32_t mode;
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
	rectype = DB___fop_create;
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
	    + sizeof(u_int32_t) + (dirname == NULL ? 0 : dirname->size)
	    + sizeof(u_int32_t)
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

	if (dirname == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &dirname->size);
		bp += sizeof(dirname->size);
		memcpy(bp, dirname->data, dirname->size);
		bp += dirname->size;
	}

	LOGCOPY_32(env, bp, &appname);
	bp += sizeof(appname);

	LOGCOPY_32(env, bp, &mode);
	bp += sizeof(mode);

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
		(void)__fop_create_print(env,
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
 * PUBLIC: int __fop_remove_read __P((ENV *, void *, __fop_remove_args **));
 */
int
__fop_remove_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__fop_remove_args **argpp;
{
	__fop_remove_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__fop_remove_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->appname, bp);
	bp += sizeof(argp->appname);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __fop_remove_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, const DBT *, u_int32_t));
 */
int
__fop_remove_log(env, txnp, ret_lsnp, flags,
    name, fid, appname)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *name;
	const DBT *fid;
	u_int32_t appname;
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
	rectype = DB___fop_remove;
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

	LOGCOPY_32(env, bp, &appname);
	bp += sizeof(appname);

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
		(void)__fop_remove_print(env,
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
 * PUBLIC: int __fop_write_42_read __P((ENV *, void *,
 * PUBLIC:     __fop_write_42_args **));
 */
int
__fop_write_42_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__fop_write_42_args **argpp;
{
	__fop_write_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__fop_write_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->appname, bp);
	bp += sizeof(argp->appname);

	LOGCOPY_32(env, &argp->pgsize, bp);
	bp += sizeof(argp->pgsize);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->pageno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->offset, bp);
	bp += sizeof(argp->offset);

	memset(&argp->page, 0, sizeof(argp->page));
	LOGCOPY_32(env,&argp->page.size, bp);
	bp += sizeof(u_int32_t);
	argp->page.data = bp;
	bp += argp->page.size;

	LOGCOPY_32(env, &argp->flag, bp);
	bp += sizeof(argp->flag);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __fop_write_read __P((ENV *, void *, __fop_write_args **));
 */
int
__fop_write_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__fop_write_args **argpp;
{
	__fop_write_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__fop_write_args) + sizeof(DB_TXN), &argp)) != 0)
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

	memset(&argp->dirname, 0, sizeof(argp->dirname));
	LOGCOPY_32(env,&argp->dirname.size, bp);
	bp += sizeof(u_int32_t);
	argp->dirname.data = bp;
	bp += argp->dirname.size;

	LOGCOPY_32(env, &argp->appname, bp);
	bp += sizeof(argp->appname);

	LOGCOPY_32(env, &argp->pgsize, bp);
	bp += sizeof(argp->pgsize);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->pageno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->offset, bp);
	bp += sizeof(argp->offset);

	memset(&argp->page, 0, sizeof(argp->page));
	LOGCOPY_32(env,&argp->page.size, bp);
	bp += sizeof(u_int32_t);
	argp->page.data = bp;
	bp += argp->page.size;

	LOGCOPY_32(env, &argp->flag, bp);
	bp += sizeof(argp->flag);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __fop_write_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, const DBT *, u_int32_t, u_int32_t,
 * PUBLIC:     db_pgno_t, u_int32_t, const DBT *, u_int32_t));
 */
int
__fop_write_log(env, txnp, ret_lsnp, flags,
    name, dirname, appname, pgsize, pageno, offset,
    page, flag)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *name;
	const DBT *dirname;
	u_int32_t appname;
	u_int32_t pgsize;
	db_pgno_t pageno;
	u_int32_t offset;
	const DBT *page;
	u_int32_t flag;
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
	rectype = DB___fop_write;
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
	    + sizeof(u_int32_t) + (dirname == NULL ? 0 : dirname->size)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (page == NULL ? 0 : page->size)
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

	if (dirname == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &dirname->size);
		bp += sizeof(dirname->size);
		memcpy(bp, dirname->data, dirname->size);
		bp += dirname->size;
	}

	LOGCOPY_32(env, bp, &appname);
	bp += sizeof(appname);

	LOGCOPY_32(env, bp, &pgsize);
	bp += sizeof(pgsize);

	uinttmp = (u_int32_t)pageno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &offset);
	bp += sizeof(offset);

	if (page == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &page->size);
		bp += sizeof(page->size);
		memcpy(bp, page->data, page->size);
		bp += page->size;
	}

	LOGCOPY_32(env, bp, &flag);
	bp += sizeof(flag);

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
		(void)__fop_write_print(env,
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
 * PUBLIC: int __fop_rename_42_read __P((ENV *, void *,
 * PUBLIC:     __fop_rename_42_args **));
 */
int
__fop_rename_42_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__fop_rename_42_args **argpp;
{
	__fop_rename_42_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__fop_rename_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	memset(&argp->fileid, 0, sizeof(argp->fileid));
	LOGCOPY_32(env,&argp->fileid.size, bp);
	bp += sizeof(u_int32_t);
	argp->fileid.data = bp;
	bp += argp->fileid.size;

	LOGCOPY_32(env, &argp->appname, bp);
	bp += sizeof(argp->appname);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __fop_rename_read __P((ENV *, void *, __fop_rename_args **));
 */
int
__fop_rename_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__fop_rename_args **argpp;
{
	__fop_rename_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__fop_rename_args) + sizeof(DB_TXN), &argp)) != 0)
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

	memset(&argp->dirname, 0, sizeof(argp->dirname));
	LOGCOPY_32(env,&argp->dirname.size, bp);
	bp += sizeof(u_int32_t);
	argp->dirname.data = bp;
	bp += argp->dirname.size;

	memset(&argp->fileid, 0, sizeof(argp->fileid));
	LOGCOPY_32(env,&argp->fileid.size, bp);
	bp += sizeof(u_int32_t);
	argp->fileid.data = bp;
	bp += argp->fileid.size;

	LOGCOPY_32(env, &argp->appname, bp);
	bp += sizeof(argp->appname);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __fop_rename_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, const DBT *, const DBT *, const DBT *,
 * PUBLIC:     u_int32_t));
 */
/*
 * PUBLIC: int __fop_rename_noundo_log __P((ENV *, DB_TXN *,
 * PUBLIC:     DB_LSN *, u_int32_t, const DBT *, const DBT *, const DBT *,
 * PUBLIC:     const DBT *, u_int32_t));
 */
/*
 * PUBLIC: int __fop_rename_int_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, const DBT *, const DBT *, const DBT *,
 * PUBLIC:     u_int32_t, u_int32_t));
 */
int
__fop_rename_log(env, txnp, ret_lsnp, flags,
    oldname, newname, dirname, fileid, appname)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *oldname;
	const DBT *newname;
	const DBT *dirname;
	const DBT *fileid;
	u_int32_t appname;

{
	return (__fop_rename_int_log(env, txnp, ret_lsnp, flags,
    oldname, newname, dirname, fileid, appname, DB___fop_rename));
}
int
__fop_rename_noundo_log(env, txnp, ret_lsnp, flags,
    oldname, newname, dirname, fileid, appname)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *oldname;
	const DBT *newname;
	const DBT *dirname;
	const DBT *fileid;
	u_int32_t appname;

{
	return (__fop_rename_int_log(env, txnp, ret_lsnp, flags,
    oldname, newname, dirname, fileid, appname, DB___fop_rename_noundo));
}
int
__fop_rename_int_log(env, txnp, ret_lsnp, flags,
    oldname, newname, dirname, fileid, appname, type)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *oldname;
	const DBT *newname;
	const DBT *dirname;
	const DBT *fileid;
	u_int32_t appname;
	u_int32_t type;
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
	rectype = type;
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
	    + sizeof(u_int32_t) + (dirname == NULL ? 0 : dirname->size)
	    + sizeof(u_int32_t) + (fileid == NULL ? 0 : fileid->size)
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

	if (dirname == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &dirname->size);
		bp += sizeof(dirname->size);
		memcpy(bp, dirname->data, dirname->size);
		bp += dirname->size;
	}

	if (fileid == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &fileid->size);
		bp += sizeof(fileid->size);
		memcpy(bp, fileid->data, fileid->size);
		bp += fileid->size;
	}

	LOGCOPY_32(env, bp, &appname);
	bp += sizeof(appname);

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
		(void)__fop_rename_print(env,
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
 * PUBLIC: int __fop_file_remove_read __P((ENV *, void *,
 * PUBLIC:     __fop_file_remove_args **));
 */
int
__fop_file_remove_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__fop_file_remove_args **argpp;
{
	__fop_file_remove_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__fop_file_remove_args) + sizeof(DB_TXN), &argp)) != 0)
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

	memset(&argp->real_fid, 0, sizeof(argp->real_fid));
	LOGCOPY_32(env,&argp->real_fid.size, bp);
	bp += sizeof(u_int32_t);
	argp->real_fid.data = bp;
	bp += argp->real_fid.size;

	memset(&argp->tmp_fid, 0, sizeof(argp->tmp_fid));
	LOGCOPY_32(env,&argp->tmp_fid.size, bp);
	bp += sizeof(u_int32_t);
	argp->tmp_fid.data = bp;
	bp += argp->tmp_fid.size;

	memset(&argp->name, 0, sizeof(argp->name));
	LOGCOPY_32(env,&argp->name.size, bp);
	bp += sizeof(u_int32_t);
	argp->name.data = bp;
	bp += argp->name.size;

	LOGCOPY_32(env, &argp->appname, bp);
	bp += sizeof(argp->appname);

	LOGCOPY_32(env, &argp->child, bp);
	bp += sizeof(argp->child);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __fop_file_remove_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, const DBT *, const DBT *, u_int32_t,
 * PUBLIC:     u_int32_t));
 */
int
__fop_file_remove_log(env, txnp, ret_lsnp, flags,
    real_fid, tmp_fid, name, appname, child)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *real_fid;
	const DBT *tmp_fid;
	const DBT *name;
	u_int32_t appname;
	u_int32_t child;
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
	rectype = DB___fop_file_remove;
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
	    + sizeof(u_int32_t) + (real_fid == NULL ? 0 : real_fid->size)
	    + sizeof(u_int32_t) + (tmp_fid == NULL ? 0 : tmp_fid->size)
	    + sizeof(u_int32_t) + (name == NULL ? 0 : name->size)
	    + sizeof(u_int32_t)
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

	if (real_fid == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &real_fid->size);
		bp += sizeof(real_fid->size);
		memcpy(bp, real_fid->data, real_fid->size);
		bp += real_fid->size;
	}

	if (tmp_fid == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &tmp_fid->size);
		bp += sizeof(tmp_fid->size);
		memcpy(bp, tmp_fid->data, tmp_fid->size);
		bp += tmp_fid->size;
	}

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

	LOGCOPY_32(env, bp, &appname);
	bp += sizeof(appname);

	LOGCOPY_32(env, bp, &child);
	bp += sizeof(child);

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
		(void)__fop_file_remove_print(env,
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
 * PUBLIC: int __fop_init_recover __P((ENV *, DB_DISTAB *));
 */
int
__fop_init_recover(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_create_recover, DB___fop_create)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_remove_recover, DB___fop_remove)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_write_recover, DB___fop_write)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_rename_recover, DB___fop_rename)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_rename_noundo_recover, DB___fop_rename_noundo)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_file_remove_recover, DB___fop_file_remove)) != 0)
		return (ret);
	return (0);
}
