/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"
#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_dispatch.h"
#include "dbinc/db_am.h"
#include "dbinc/hash.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

/*
 * PUBLIC: int __ham_insdel_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __ham_insdel_args **));
 */
int
__ham_insdel_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_insdel_args **argpp;
{
	__ham_insdel_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_insdel_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->opcode, bp);
	bp += sizeof(argp->opcode);

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

	LOGCOPY_32(env, &argp->ndx, bp);
	bp += sizeof(argp->ndx);

	LOGCOPY_TOLSN(env, &argp->pagelsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->key, 0, sizeof(argp->key));
	LOGCOPY_32(env,&argp->key.size, bp);
	bp += sizeof(u_int32_t);
	argp->key.data = bp;
	bp += argp->key.size;

	memset(&argp->data, 0, sizeof(argp->data));
	LOGCOPY_32(env,&argp->data.size, bp);
	bp += sizeof(u_int32_t);
	argp->data.data = bp;
	bp += argp->data.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_insdel_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, db_pgno_t, u_int32_t, DB_LSN *,
 * PUBLIC:     const DBT *, const DBT *));
 */
int
__ham_insdel_log(dbp, txnp, ret_lsnp, flags,
    opcode, pgno, ndx, pagelsn, key,
    data)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t opcode;
	db_pgno_t pgno;
	u_int32_t ndx;
	DB_LSN * pagelsn;
	const DBT *key;
	const DBT *data;
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
	rectype = DB___ham_insdel;
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
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(*pagelsn)
	    + sizeof(u_int32_t) + (key == NULL ? 0 : key->size)
	    + sizeof(u_int32_t) + (data == NULL ? 0 : data->size);
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

	LOGCOPY_32(env, bp, &opcode);
	bp += sizeof(opcode);

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &ndx);
	bp += sizeof(ndx);

	if (pagelsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(pagelsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, pagelsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, pagelsn);
	} else
		memset(bp, 0, sizeof(*pagelsn));
	bp += sizeof(*pagelsn);

	if (key == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &key->size);
		bp += sizeof(key->size);
		memcpy(bp, key->data, key->size);
		bp += key->size;
	}

	if (data == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &data->size);
		bp += sizeof(data->size);
		memcpy(bp, data->data, data->size);
		bp += data->size;
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
		(void)__ham_insdel_print(env,
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
 * PUBLIC: int __ham_newpage_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __ham_newpage_args **));
 */
int
__ham_newpage_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_newpage_args **argpp;
{
	__ham_newpage_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_newpage_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->opcode, bp);
	bp += sizeof(argp->opcode);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);
	if (dbpp != NULL) {
		*dbpp = NULL;
		ret = __dbreg_id_to_db(
		    env, argp->txnp, dbpp, argp->fileid, 1);
	}

	LOGCOPY_32(env, &uinttmp, bp);
	argp->prev_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->prevlsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->new_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->pagelsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->nextlsn, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_newpage_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, db_pgno_t, DB_LSN *, db_pgno_t, DB_LSN *,
 * PUBLIC:     db_pgno_t, DB_LSN *));
 */
int
__ham_newpage_log(dbp, txnp, ret_lsnp, flags,
    opcode, prev_pgno, prevlsn, new_pgno, pagelsn,
    next_pgno, nextlsn)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t opcode;
	db_pgno_t prev_pgno;
	DB_LSN * prevlsn;
	db_pgno_t new_pgno;
	DB_LSN * pagelsn;
	db_pgno_t next_pgno;
	DB_LSN * nextlsn;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	ENV *env;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	env = dbp->env;
	rlsnp = ret_lsnp;
	rectype = DB___ham_newpage;
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
	    + sizeof(u_int32_t)
	    + sizeof(*prevlsn)
	    + sizeof(u_int32_t)
	    + sizeof(*pagelsn)
	    + sizeof(u_int32_t)
	    + sizeof(*nextlsn);
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

	LOGCOPY_32(env, bp, &opcode);
	bp += sizeof(opcode);

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)prev_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (prevlsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(prevlsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, prevlsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, prevlsn);
	} else
		memset(bp, 0, sizeof(*prevlsn));
	bp += sizeof(*prevlsn);

	uinttmp = (u_int32_t)new_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (pagelsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(pagelsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, pagelsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, pagelsn);
	} else
		memset(bp, 0, sizeof(*pagelsn));
	bp += sizeof(*pagelsn);

	uinttmp = (u_int32_t)next_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (nextlsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(nextlsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, nextlsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, nextlsn);
	} else
		memset(bp, 0, sizeof(*nextlsn));
	bp += sizeof(*nextlsn);

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
		(void)__ham_newpage_print(env,
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
 * PUBLIC: int __ham_splitdata_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __ham_splitdata_args **));
 */
int
__ham_splitdata_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_splitdata_args **argpp;
{
	__ham_splitdata_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_splitdata_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->opcode, bp);
	bp += sizeof(argp->opcode);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->pageimage, 0, sizeof(argp->pageimage));
	LOGCOPY_32(env,&argp->pageimage.size, bp);
	bp += sizeof(u_int32_t);
	argp->pageimage.data = bp;
	bp += argp->pageimage.size;
	if (LOG_SWAPPED(env) && dbpp != NULL && *dbpp != NULL) {
		int t_ret;
		if ((t_ret = __db_pageswap(*dbpp, (PAGE *)argp->pageimage.data,
		    (size_t)argp->pageimage.size, NULL, 1)) != 0)
			return (t_ret);
	}

	LOGCOPY_TOLSN(env, &argp->pagelsn, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_splitdata_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, db_pgno_t, const DBT *, DB_LSN *));
 */
int
__ham_splitdata_log(dbp, txnp, ret_lsnp, flags, opcode, pgno, pageimage, pagelsn)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t opcode;
	db_pgno_t pgno;
	const DBT *pageimage;
	DB_LSN * pagelsn;
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
	rectype = DB___ham_splitdata;
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
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (pageimage == NULL ? 0 : pageimage->size)
	    + sizeof(*pagelsn);
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

	LOGCOPY_32(env, bp, &opcode);
	bp += sizeof(opcode);

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (pageimage == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &pageimage->size);
		bp += sizeof(pageimage->size);
		memcpy(bp, pageimage->data, pageimage->size);
		if (LOG_SWAPPED(env))
			if ((ret = __db_pageswap(dbp,
			    (PAGE *)bp, (size_t)pageimage->size, (DBT *)NULL, 0)) != 0)
				return (ret);
		bp += pageimage->size;
	}

	if (pagelsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(pagelsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, pagelsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, pagelsn);
	} else
		memset(bp, 0, sizeof(*pagelsn));
	bp += sizeof(*pagelsn);

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
		(void)__ham_splitdata_print(env,
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
 * PUBLIC: int __ham_replace_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __ham_replace_args **));
 */
int
__ham_replace_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_replace_args **argpp;
{
	__ham_replace_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_replace_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->ndx, bp);
	bp += sizeof(argp->ndx);

	LOGCOPY_TOLSN(env, &argp->pagelsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->off = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->olditem, 0, sizeof(argp->olditem));
	LOGCOPY_32(env,&argp->olditem.size, bp);
	bp += sizeof(u_int32_t);
	argp->olditem.data = bp;
	bp += argp->olditem.size;

	memset(&argp->newitem, 0, sizeof(argp->newitem));
	LOGCOPY_32(env,&argp->newitem.size, bp);
	bp += sizeof(u_int32_t);
	argp->newitem.data = bp;
	bp += argp->newitem.size;

	LOGCOPY_32(env, &argp->makedup, bp);
	bp += sizeof(argp->makedup);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_replace_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, u_int32_t, DB_LSN *, int32_t, const DBT *,
 * PUBLIC:     const DBT *, u_int32_t));
 */
int
__ham_replace_log(dbp, txnp, ret_lsnp, flags, pgno, ndx, pagelsn, off, olditem,
    newitem, makedup)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	u_int32_t ndx;
	DB_LSN * pagelsn;
	int32_t off;
	const DBT *olditem;
	const DBT *newitem;
	u_int32_t makedup;
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
	rectype = DB___ham_replace;
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
	    + sizeof(u_int32_t)
	    + sizeof(*pagelsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (olditem == NULL ? 0 : olditem->size)
	    + sizeof(u_int32_t) + (newitem == NULL ? 0 : newitem->size)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &ndx);
	bp += sizeof(ndx);

	if (pagelsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(pagelsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, pagelsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, pagelsn);
	} else
		memset(bp, 0, sizeof(*pagelsn));
	bp += sizeof(*pagelsn);

	uinttmp = (u_int32_t)off;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (olditem == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &olditem->size);
		bp += sizeof(olditem->size);
		memcpy(bp, olditem->data, olditem->size);
		bp += olditem->size;
	}

	if (newitem == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &newitem->size);
		bp += sizeof(newitem->size);
		memcpy(bp, newitem->data, newitem->size);
		bp += newitem->size;
	}

	LOGCOPY_32(env, bp, &makedup);
	bp += sizeof(makedup);

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
		(void)__ham_replace_print(env,
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
 * PUBLIC: int __ham_copypage_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __ham_copypage_args **));
 */
int
__ham_copypage_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_copypage_args **argpp;
{
	__ham_copypage_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_copypage_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->pagelsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->nextlsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->nnext_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->nnextlsn, bp);
	bp += sizeof(DB_LSN);

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

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_copypage_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, db_pgno_t, DB_LSN *, db_pgno_t,
 * PUBLIC:     DB_LSN *, const DBT *));
 */
int
__ham_copypage_log(dbp, txnp, ret_lsnp, flags, pgno, pagelsn, next_pgno, nextlsn, nnext_pgno,
    nnextlsn, page)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * pagelsn;
	db_pgno_t next_pgno;
	DB_LSN * nextlsn;
	db_pgno_t nnext_pgno;
	DB_LSN * nnextlsn;
	const DBT *page;
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
	rectype = DB___ham_copypage;
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
	    + sizeof(*pagelsn)
	    + sizeof(u_int32_t)
	    + sizeof(*nextlsn)
	    + sizeof(u_int32_t)
	    + sizeof(*nnextlsn)
	    + sizeof(u_int32_t) + (page == NULL ? 0 : page->size);
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

	if (pagelsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(pagelsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, pagelsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, pagelsn);
	} else
		memset(bp, 0, sizeof(*pagelsn));
	bp += sizeof(*pagelsn);

	uinttmp = (u_int32_t)next_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (nextlsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(nextlsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, nextlsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, nextlsn);
	} else
		memset(bp, 0, sizeof(*nextlsn));
	bp += sizeof(*nextlsn);

	uinttmp = (u_int32_t)nnext_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (nnextlsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(nnextlsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, nnextlsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, nnextlsn);
	} else
		memset(bp, 0, sizeof(*nnextlsn));
	bp += sizeof(*nnextlsn);

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
		(void)__ham_copypage_print(env,
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
 * PUBLIC: int __ham_metagroup_42_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __ham_metagroup_42_args **));
 */
int
__ham_metagroup_42_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_metagroup_42_args **argpp;
{
	__ham_metagroup_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_metagroup_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->bucket, bp);
	bp += sizeof(argp->bucket);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mmpgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->mmetalsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mpgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->metalsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->pagelsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &argp->newalloc, bp);
	bp += sizeof(argp->newalloc);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_metagroup_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __ham_metagroup_args **));
 */
int
__ham_metagroup_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_metagroup_args **argpp;
{
	__ham_metagroup_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_metagroup_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->bucket, bp);
	bp += sizeof(argp->bucket);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mmpgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->mmetalsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->mpgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->metalsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->pagelsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &argp->newalloc, bp);
	bp += sizeof(argp->newalloc);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->last_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_metagroup_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, db_pgno_t, DB_LSN *, db_pgno_t, DB_LSN *,
 * PUBLIC:     db_pgno_t, DB_LSN *, u_int32_t, db_pgno_t));
 */
int
__ham_metagroup_log(dbp, txnp, ret_lsnp, flags, bucket, mmpgno, mmetalsn, mpgno, metalsn,
    pgno, pagelsn, newalloc, last_pgno)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t bucket;
	db_pgno_t mmpgno;
	DB_LSN * mmetalsn;
	db_pgno_t mpgno;
	DB_LSN * metalsn;
	db_pgno_t pgno;
	DB_LSN * pagelsn;
	u_int32_t newalloc;
	db_pgno_t last_pgno;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	ENV *env;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	env = dbp->env;
	rlsnp = ret_lsnp;
	rectype = DB___ham_metagroup;
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
	    + sizeof(u_int32_t)
	    + sizeof(*mmetalsn)
	    + sizeof(u_int32_t)
	    + sizeof(*metalsn)
	    + sizeof(u_int32_t)
	    + sizeof(*pagelsn)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &bucket);
	bp += sizeof(bucket);

	uinttmp = (u_int32_t)mmpgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (mmetalsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(mmetalsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, mmetalsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, mmetalsn);
	} else
		memset(bp, 0, sizeof(*mmetalsn));
	bp += sizeof(*mmetalsn);

	uinttmp = (u_int32_t)mpgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (metalsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(metalsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, metalsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, metalsn);
	} else
		memset(bp, 0, sizeof(*metalsn));
	bp += sizeof(*metalsn);

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (pagelsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(pagelsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, pagelsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, pagelsn);
	} else
		memset(bp, 0, sizeof(*pagelsn));
	bp += sizeof(*pagelsn);

	LOGCOPY_32(env, bp, &newalloc);
	bp += sizeof(newalloc);

	uinttmp = (u_int32_t)last_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

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
		(void)__ham_metagroup_print(env,
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
 * PUBLIC: int __ham_groupalloc_42_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __ham_groupalloc_42_args **));
 */
int
__ham_groupalloc_42_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_groupalloc_42_args **argpp;
{
	__ham_groupalloc_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_groupalloc_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->meta_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->start_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->num, bp);
	bp += sizeof(argp->num);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->free = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_groupalloc_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __ham_groupalloc_args **));
 */
int
__ham_groupalloc_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_groupalloc_args **argpp;
{
	__ham_groupalloc_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_groupalloc_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->meta_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->start_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->num, bp);
	bp += sizeof(argp->num);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->unused = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->last_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_groupalloc_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, DB_LSN *, db_pgno_t, u_int32_t, db_pgno_t,
 * PUBLIC:     db_pgno_t));
 */
int
__ham_groupalloc_log(dbp, txnp, ret_lsnp, flags, meta_lsn, start_pgno, num, unused, last_pgno)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	DB_LSN * meta_lsn;
	db_pgno_t start_pgno;
	u_int32_t num;
	db_pgno_t unused;
	db_pgno_t last_pgno;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	ENV *env;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	env = dbp->env;
	rlsnp = ret_lsnp;
	rectype = DB___ham_groupalloc;
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
	    + sizeof(*meta_lsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (meta_lsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(meta_lsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, meta_lsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, meta_lsn);
	} else
		memset(bp, 0, sizeof(*meta_lsn));
	bp += sizeof(*meta_lsn);

	uinttmp = (u_int32_t)start_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &num);
	bp += sizeof(num);

	uinttmp = (u_int32_t)unused;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)last_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

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
		(void)__ham_groupalloc_print(env,
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
 * PUBLIC: int __ham_curadj_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __ham_curadj_args **));
 */
int
__ham_curadj_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_curadj_args **argpp;
{
	__ham_curadj_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_curadj_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->indx, bp);
	bp += sizeof(argp->indx);

	LOGCOPY_32(env, &argp->len, bp);
	bp += sizeof(argp->len);

	LOGCOPY_32(env, &argp->dup_off, bp);
	bp += sizeof(argp->dup_off);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->add = (int)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->is_dup = (int)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->order, bp);
	bp += sizeof(argp->order);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_curadj_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, u_int32_t, u_int32_t, u_int32_t, int, int,
 * PUBLIC:     u_int32_t));
 */
int
__ham_curadj_log(dbp, txnp, ret_lsnp, flags, pgno, indx, len, dup_off, add,
    is_dup, order)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	u_int32_t indx;
	u_int32_t len;
	u_int32_t dup_off;
	int add;
	int is_dup;
	u_int32_t order;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	ENV *env;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	env = dbp->env;
	rlsnp = ret_lsnp;
	rectype = DB___ham_curadj;
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
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &indx);
	bp += sizeof(indx);

	LOGCOPY_32(env, bp, &len);
	bp += sizeof(len);

	LOGCOPY_32(env, bp, &dup_off);
	bp += sizeof(dup_off);

	uinttmp = (u_int32_t)add;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)is_dup;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &order);
	bp += sizeof(order);

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
		(void)__ham_curadj_print(env,
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
 * PUBLIC: int __ham_chgpg_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __ham_chgpg_args **));
 */
int
__ham_chgpg_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__ham_chgpg_args **argpp;
{
	__ham_chgpg_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__ham_chgpg_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->mode = (db_ham_mode)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->old_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->new_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->old_indx, bp);
	bp += sizeof(argp->old_indx);

	LOGCOPY_32(env, &argp->new_indx, bp);
	bp += sizeof(argp->new_indx);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __ham_chgpg_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_ham_mode, db_pgno_t, db_pgno_t, u_int32_t,
 * PUBLIC:     u_int32_t));
 */
int
__ham_chgpg_log(dbp, txnp, ret_lsnp, flags, mode, old_pgno, new_pgno, old_indx, new_indx)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_ham_mode mode;
	db_pgno_t old_pgno;
	db_pgno_t new_pgno;
	u_int32_t old_indx;
	u_int32_t new_indx;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	ENV *env;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	env = dbp->env;
	rlsnp = ret_lsnp;
	rectype = DB___ham_chgpg;
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
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)mode;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)old_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)new_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &old_indx);
	bp += sizeof(old_indx);

	LOGCOPY_32(env, bp, &new_indx);
	bp += sizeof(new_indx);

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
		(void)__ham_chgpg_print(env,
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
 * PUBLIC: int __ham_init_recover __P((ENV *, DB_DISTAB *));
 */
int
__ham_init_recover(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_insdel_recover, DB___ham_insdel)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_newpage_recover, DB___ham_newpage)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_splitdata_recover, DB___ham_splitdata)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_replace_recover, DB___ham_replace)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_copypage_recover, DB___ham_copypage)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_metagroup_recover, DB___ham_metagroup)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_groupalloc_recover, DB___ham_groupalloc)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_curadj_recover, DB___ham_curadj)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_chgpg_recover, DB___ham_chgpg)) != 0)
		return (ret);
	return (0);
}
