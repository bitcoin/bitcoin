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
 * PUBLIC: int __db_addrem_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __db_addrem_args **));
 */
int
__db_addrem_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_addrem_args **argpp;
{
	__db_addrem_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_addrem_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->indx, bp);
	bp += sizeof(argp->indx);

	LOGCOPY_32(env, &argp->nbytes, bp);
	bp += sizeof(argp->nbytes);

	memset(&argp->hdr, 0, sizeof(argp->hdr));
	LOGCOPY_32(env,&argp->hdr.size, bp);
	bp += sizeof(u_int32_t);
	argp->hdr.data = bp;
	bp += argp->hdr.size;

	memset(&argp->dbt, 0, sizeof(argp->dbt));
	LOGCOPY_32(env,&argp->dbt.size, bp);
	bp += sizeof(u_int32_t);
	argp->dbt.data = bp;
	bp += argp->dbt.size;

	LOGCOPY_TOLSN(env, &argp->pagelsn, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_addrem_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, db_pgno_t, u_int32_t, u_int32_t,
 * PUBLIC:     const DBT *, const DBT *, DB_LSN *));
 */
int
__db_addrem_log(dbp, txnp, ret_lsnp, flags,
    opcode, pgno, indx, nbytes, hdr,
    dbt, pagelsn)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t opcode;
	db_pgno_t pgno;
	u_int32_t indx;
	u_int32_t nbytes;
	const DBT *hdr;
	const DBT *dbt;
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
	rectype = DB___db_addrem;
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
	    + sizeof(u_int32_t) + (hdr == NULL ? 0 : hdr->size)
	    + sizeof(u_int32_t) + (dbt == NULL ? 0 : dbt->size)
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

	LOGCOPY_32(env, bp, &opcode);
	bp += sizeof(opcode);

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &indx);
	bp += sizeof(indx);

	LOGCOPY_32(env, bp, &nbytes);
	bp += sizeof(nbytes);

	if (hdr == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &hdr->size);
		bp += sizeof(hdr->size);
		memcpy(bp, hdr->data, hdr->size);
		bp += hdr->size;
	}

	if (dbt == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &dbt->size);
		bp += sizeof(dbt->size);
		memcpy(bp, dbt->data, dbt->size);
		bp += dbt->size;
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
		(void)__db_addrem_print(env,
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
 * PUBLIC: int __db_big_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __db_big_args **));
 */
int
__db_big_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_big_args **argpp;
{
	__db_big_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_big_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &uinttmp, bp);
	argp->prev_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->dbt, 0, sizeof(argp->dbt));
	LOGCOPY_32(env,&argp->dbt.size, bp);
	bp += sizeof(u_int32_t);
	argp->dbt.data = bp;
	bp += argp->dbt.size;

	LOGCOPY_TOLSN(env, &argp->pagelsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_TOLSN(env, &argp->prevlsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_TOLSN(env, &argp->nextlsn, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_big_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, db_pgno_t, db_pgno_t, db_pgno_t,
 * PUBLIC:     const DBT *, DB_LSN *, DB_LSN *, DB_LSN *));
 */
int
__db_big_log(dbp, txnp, ret_lsnp, flags,
    opcode, pgno, prev_pgno, next_pgno, dbt,
    pagelsn, prevlsn, nextlsn)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t opcode;
	db_pgno_t pgno;
	db_pgno_t prev_pgno;
	db_pgno_t next_pgno;
	const DBT *dbt;
	DB_LSN * pagelsn;
	DB_LSN * prevlsn;
	DB_LSN * nextlsn;
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
	rectype = DB___db_big;
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
	    + sizeof(u_int32_t) + (dbt == NULL ? 0 : dbt->size)
	    + sizeof(*pagelsn)
	    + sizeof(*prevlsn)
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

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)prev_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)next_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (dbt == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &dbt->size);
		bp += sizeof(dbt->size);
		memcpy(bp, dbt->data, dbt->size);
		bp += dbt->size;
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
		(void)__db_big_print(env,
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
 * PUBLIC: int __db_ovref_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __db_ovref_args **));
 */
int
__db_ovref_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_ovref_args **argpp;
{
	__db_ovref_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_ovref_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &uinttmp, bp);
	argp->adjust = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->lsn, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_ovref_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, int32_t, DB_LSN *));
 */
int
__db_ovref_log(dbp, txnp, ret_lsnp, flags, pgno, adjust, lsn)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	int32_t adjust;
	DB_LSN * lsn;
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
	rectype = DB___db_ovref;
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

	uinttmp = (u_int32_t)adjust;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

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
		(void)__db_ovref_print(env,
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
 * PUBLIC: int __db_relink_42_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __db_relink_42_args **));
 */
int
__db_relink_42_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_relink_42_args **argpp;
{
	__db_relink_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_relink_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->prev = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->lsn_prev, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->lsn_next, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_debug_read __P((ENV *, void *, __db_debug_args **));
 */
int
__db_debug_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__db_debug_args **argpp;
{
	__db_debug_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_debug_args) + sizeof(DB_TXN), &argp)) != 0)
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

	memset(&argp->op, 0, sizeof(argp->op));
	LOGCOPY_32(env,&argp->op.size, bp);
	bp += sizeof(u_int32_t);
	argp->op.data = bp;
	bp += argp->op.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

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

	LOGCOPY_32(env, &argp->arg_flags, bp);
	bp += sizeof(argp->arg_flags);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_debug_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *, int32_t, const DBT *, const DBT *,
 * PUBLIC:     u_int32_t));
 */
int
__db_debug_log(env, txnp, ret_lsnp, flags,
    op, fileid, key, data, arg_flags)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *op;
	int32_t fileid;
	const DBT *key;
	const DBT *data;
	u_int32_t arg_flags;
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
	rectype = DB___db_debug;
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
	    + sizeof(u_int32_t) + (op == NULL ? 0 : op->size)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (key == NULL ? 0 : key->size)
	    + sizeof(u_int32_t) + (data == NULL ? 0 : data->size)
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

	if (op == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &op->size);
		bp += sizeof(op->size);
		memcpy(bp, op->data, op->size);
		bp += op->size;
	}

	uinttmp = (u_int32_t)fileid;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

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

	LOGCOPY_32(env, bp, &arg_flags);
	bp += sizeof(arg_flags);

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
		(void)__db_debug_print(env,
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
 * PUBLIC: int __db_noop_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __db_noop_args **));
 */
int
__db_noop_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_noop_args **argpp;
{
	__db_noop_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_noop_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->prevlsn, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_noop_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *));
 */
int
__db_noop_log(dbp, txnp, ret_lsnp, flags, pgno, prevlsn)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * prevlsn;
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
	rectype = DB___db_noop;
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
	    + sizeof(*prevlsn);
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
		(void)__db_noop_print(env,
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
 * PUBLIC: int __db_pg_alloc_42_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __db_pg_alloc_42_args **));
 */
int
__db_pg_alloc_42_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_pg_alloc_42_args **argpp;
{
	__db_pg_alloc_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_pg_alloc_42_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->meta_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->page_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->ptype, bp);
	bp += sizeof(argp->ptype);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_pg_alloc_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __db_pg_alloc_args **));
 */
int
__db_pg_alloc_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_pg_alloc_args **argpp;
{
	__db_pg_alloc_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_pg_alloc_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->meta_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->page_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->ptype, bp);
	bp += sizeof(argp->ptype);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->last_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_pg_alloc_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, DB_LSN *, db_pgno_t, DB_LSN *, db_pgno_t, u_int32_t,
 * PUBLIC:     db_pgno_t, db_pgno_t));
 */
int
__db_pg_alloc_log(dbp, txnp, ret_lsnp, flags, meta_lsn, meta_pgno, page_lsn, pgno, ptype,
    next, last_pgno)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	DB_LSN * meta_lsn;
	db_pgno_t meta_pgno;
	DB_LSN * page_lsn;
	db_pgno_t pgno;
	u_int32_t ptype;
	db_pgno_t next;
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
	rectype = DB___db_pg_alloc;
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
	    + sizeof(*page_lsn)
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

	uinttmp = (u_int32_t)meta_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (page_lsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(page_lsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, page_lsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, page_lsn);
	} else
		memset(bp, 0, sizeof(*page_lsn));
	bp += sizeof(*page_lsn);

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &ptype);
	bp += sizeof(ptype);

	uinttmp = (u_int32_t)next;
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
		(void)__db_pg_alloc_print(env,
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
 * PUBLIC: int __db_pg_free_42_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __db_pg_free_42_args **));
 */
int
__db_pg_free_42_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_pg_free_42_args **argpp;
{
	__db_pg_free_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_pg_free_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->meta_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->meta_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->header, 0, sizeof(argp->header));
	LOGCOPY_32(env,&argp->header.size, bp);
	bp += sizeof(u_int32_t);
	argp->header.data = bp;
	bp += argp->header.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_pg_free_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __db_pg_free_args **));
 */
int
__db_pg_free_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_pg_free_args **argpp;
{
	__db_pg_free_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_pg_free_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->meta_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->meta_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->header, 0, sizeof(argp->header));
	LOGCOPY_32(env,&argp->header.size, bp);
	bp += sizeof(u_int32_t);
	argp->header.data = bp;
	bp += argp->header.size;
	if (LOG_SWAPPED(env) && dbpp != NULL && *dbpp != NULL) {
		int t_ret;
		if ((t_ret = __db_pageswap(*dbpp, (PAGE *)argp->header.data,
		    (size_t)argp->header.size, NULL, 1)) != 0)
			return (t_ret);
	}

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->last_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_pg_free_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, db_pgno_t, const DBT *,
 * PUBLIC:     db_pgno_t, db_pgno_t));
 */
int
__db_pg_free_log(dbp, txnp, ret_lsnp, flags, pgno, meta_lsn, meta_pgno, header, next,
    last_pgno)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * meta_lsn;
	db_pgno_t meta_pgno;
	const DBT *header;
	db_pgno_t next;
	db_pgno_t last_pgno;
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
	rectype = DB___db_pg_free;
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
	    + sizeof(*meta_lsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (header == NULL ? 0 : header->size)
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

	uinttmp = (u_int32_t)meta_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (header == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &header->size);
		bp += sizeof(header->size);
		memcpy(bp, header->data, header->size);
		if (LOG_SWAPPED(env))
			if ((ret = __db_pageswap(dbp,
			    (PAGE *)bp, (size_t)header->size, (DBT *)NULL, 0)) != 0)
				return (ret);
		bp += header->size;
	}

	uinttmp = (u_int32_t)next;
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
		(void)__db_pg_free_print(env,
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
 * PUBLIC: int __db_cksum_read __P((ENV *, void *, __db_cksum_args **));
 */
int
__db_cksum_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__db_cksum_args **argpp;
{
	__db_cksum_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_cksum_args) + sizeof(DB_TXN), &argp)) != 0)
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

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_cksum_log __P((ENV *, DB_TXN *, DB_LSN *, u_int32_t));
 */
int
__db_cksum_log(env, txnp, ret_lsnp, flags)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	u_int32_t rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rlsnp = ret_lsnp;
	rectype = DB___db_cksum;
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

	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN);
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
		(void)__db_cksum_print(env,
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
 * PUBLIC: int __db_pg_freedata_42_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __db_pg_freedata_42_args **));
 */
int
__db_pg_freedata_42_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_pg_freedata_42_args **argpp;
{
	__db_pg_freedata_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_pg_freedata_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->meta_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->meta_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->header, 0, sizeof(argp->header));
	LOGCOPY_32(env,&argp->header.size, bp);
	bp += sizeof(u_int32_t);
	argp->header.data = bp;
	bp += argp->header.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->data, 0, sizeof(argp->data));
	LOGCOPY_32(env,&argp->data.size, bp);
	bp += sizeof(u_int32_t);
	argp->data.data = bp;
	bp += argp->data.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_pg_freedata_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __db_pg_freedata_args **));
 */
int
__db_pg_freedata_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_pg_freedata_args **argpp;
{
	__db_pg_freedata_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_pg_freedata_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->meta_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->meta_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->header, 0, sizeof(argp->header));
	LOGCOPY_32(env,&argp->header.size, bp);
	bp += sizeof(u_int32_t);
	argp->header.data = bp;
	bp += argp->header.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->last_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->data, 0, sizeof(argp->data));
	LOGCOPY_32(env,&argp->data.size, bp);
	bp += sizeof(u_int32_t);
	argp->data.data = bp;
	bp += argp->data.size;
	if (LOG_SWAPPED(env) && dbpp != NULL && *dbpp != NULL) {
		int t_ret;
		if ((t_ret = __db_pageswap(*dbpp,
		    (PAGE *)argp->header.data, (size_t)argp->header.size,
		    &argp->data, 1)) != 0)
			return (t_ret);
	}

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_pg_freedata_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, db_pgno_t, const DBT *,
 * PUBLIC:     db_pgno_t, db_pgno_t, const DBT *));
 */
int
__db_pg_freedata_log(dbp, txnp, ret_lsnp, flags, pgno, meta_lsn, meta_pgno, header, next,
    last_pgno, data)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * meta_lsn;
	db_pgno_t meta_pgno;
	const DBT *header;
	db_pgno_t next;
	db_pgno_t last_pgno;
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
	rectype = DB___db_pg_freedata;
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
	    + sizeof(*meta_lsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (header == NULL ? 0 : header->size)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
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

	uinttmp = (u_int32_t)meta_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (header == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &header->size);
		bp += sizeof(header->size);
		memcpy(bp, header->data, header->size);
		if (LOG_SWAPPED(env))
			if ((ret = __db_pageswap(dbp,
			    (PAGE *)bp, (size_t)header->size, (DBT *)data, 0)) != 0)
				return (ret);
		bp += header->size;
	}

	uinttmp = (u_int32_t)next;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)last_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (data == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &data->size);
		bp += sizeof(data->size);
		memcpy(bp, data->data, data->size);
		if (LOG_SWAPPED(env) && F_ISSET(data, DB_DBT_APPMALLOC))
			__os_free(env, data->data);
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
		(void)__db_pg_freedata_print(env,
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
 * PUBLIC: int __db_pg_init_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __db_pg_init_args **));
 */
int
__db_pg_init_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_pg_init_args **argpp;
{
	__db_pg_init_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_pg_init_args) + sizeof(DB_TXN), &argp)) != 0)
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

	memset(&argp->header, 0, sizeof(argp->header));
	LOGCOPY_32(env,&argp->header.size, bp);
	bp += sizeof(u_int32_t);
	argp->header.data = bp;
	bp += argp->header.size;

	memset(&argp->data, 0, sizeof(argp->data));
	LOGCOPY_32(env,&argp->data.size, bp);
	bp += sizeof(u_int32_t);
	argp->data.data = bp;
	bp += argp->data.size;
	if (LOG_SWAPPED(env) && dbpp != NULL && *dbpp != NULL) {
		int t_ret;
		if ((t_ret = __db_pageswap(*dbpp,
		    (PAGE *)argp->header.data, (size_t)argp->header.size,
		    &argp->data, 1)) != 0)
			return (t_ret);
	}

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_pg_init_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, const DBT *, const DBT *));
 */
int
__db_pg_init_log(dbp, txnp, ret_lsnp, flags, pgno, header, data)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	const DBT *header;
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
	rectype = DB___db_pg_init;
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
	    + sizeof(u_int32_t) + (header == NULL ? 0 : header->size)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (header == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &header->size);
		bp += sizeof(header->size);
		memcpy(bp, header->data, header->size);
		if (LOG_SWAPPED(env))
			if ((ret = __db_pageswap(dbp,
			    (PAGE *)bp, (size_t)header->size, (DBT *)data, 0)) != 0)
				return (ret);
		bp += header->size;
	}

	if (data == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &data->size);
		bp += sizeof(data->size);
		memcpy(bp, data->data, data->size);
		if (LOG_SWAPPED(env) && F_ISSET(data, DB_DBT_APPMALLOC))
			__os_free(env, data->data);
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
		(void)__db_pg_init_print(env,
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
 * PUBLIC: int __db_pg_sort_44_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __db_pg_sort_44_args **));
 */
int
__db_pg_sort_44_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_pg_sort_44_args **argpp;
{
	__db_pg_sort_44_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_pg_sort_44_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->meta = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->meta_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->last_free = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->last_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->last_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->list, 0, sizeof(argp->list));
	LOGCOPY_32(env,&argp->list.size, bp);
	bp += sizeof(u_int32_t);
	argp->list.data = bp;
	bp += argp->list.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_pg_trunc_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __db_pg_trunc_args **));
 */
int
__db_pg_trunc_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__db_pg_trunc_args **argpp;
{
	__db_pg_trunc_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__db_pg_trunc_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->meta = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->meta_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->last_free = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->last_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->next_free = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->last_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->list, 0, sizeof(argp->list));
	LOGCOPY_32(env,&argp->list.size, bp);
	bp += sizeof(u_int32_t);
	argp->list.data = bp;
	bp += argp->list.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __db_pg_trunc_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, db_pgno_t, DB_LSN *, db_pgno_t,
 * PUBLIC:     db_pgno_t, const DBT *));
 */
int
__db_pg_trunc_log(dbp, txnp, ret_lsnp, flags, meta, meta_lsn, last_free, last_lsn, next_free,
    last_pgno, list)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t meta;
	DB_LSN * meta_lsn;
	db_pgno_t last_free;
	DB_LSN * last_lsn;
	db_pgno_t next_free;
	db_pgno_t last_pgno;
	const DBT *list;
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
	rectype = DB___db_pg_trunc;
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
	    + sizeof(*meta_lsn)
	    + sizeof(u_int32_t)
	    + sizeof(*last_lsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (list == NULL ? 0 : list->size);
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

	uinttmp = (u_int32_t)meta;
	LOGCOPY_32(env,bp, &uinttmp);
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

	uinttmp = (u_int32_t)last_free;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (last_lsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(last_lsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, last_lsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, last_lsn);
	} else
		memset(bp, 0, sizeof(*last_lsn));
	bp += sizeof(*last_lsn);

	uinttmp = (u_int32_t)next_free;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)last_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (list == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &list->size);
		bp += sizeof(list->size);
		memcpy(bp, list->data, list->size);
		bp += list->size;
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
		(void)__db_pg_trunc_print(env,
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
 * PUBLIC: int __db_init_recover __P((ENV *, DB_DISTAB *));
 */
int
__db_init_recover(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_addrem_recover, DB___db_addrem)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_big_recover, DB___db_big)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_ovref_recover, DB___db_ovref)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_debug_recover, DB___db_debug)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_noop_recover, DB___db_noop)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_alloc_recover, DB___db_pg_alloc)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_free_recover, DB___db_pg_free)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_cksum_recover, DB___db_cksum)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_freedata_recover, DB___db_pg_freedata)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_init_recover, DB___db_pg_init)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_trunc_recover, DB___db_pg_trunc)) != 0)
		return (ret);
	return (0);
}
