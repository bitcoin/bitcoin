/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"
#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_dispatch.h"
#include "dbinc/db_am.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

/*
 * PUBLIC: int __txn_regop_42_read __P((ENV *, void *,
 * PUBLIC:     __txn_regop_42_args **));
 */
int
__txn_regop_42_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__txn_regop_42_args **argpp;
{
	__txn_regop_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__txn_regop_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->opcode, bp);
	bp += sizeof(argp->opcode);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->timestamp = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->locks, 0, sizeof(argp->locks));
	LOGCOPY_32(env,&argp->locks.size, bp);
	bp += sizeof(u_int32_t);
	argp->locks.data = bp;
	bp += argp->locks.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __txn_regop_read __P((ENV *, void *, __txn_regop_args **));
 */
int
__txn_regop_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__txn_regop_args **argpp;
{
	__txn_regop_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__txn_regop_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->opcode, bp);
	bp += sizeof(argp->opcode);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->timestamp = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->envid, bp);
	bp += sizeof(argp->envid);

	memset(&argp->locks, 0, sizeof(argp->locks));
	LOGCOPY_32(env,&argp->locks.size, bp);
	bp += sizeof(u_int32_t);
	argp->locks.data = bp;
	bp += argp->locks.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __txn_regop_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, int32_t, u_int32_t, const DBT *));
 */
int
__txn_regop_log(env, txnp, ret_lsnp, flags,
    opcode, timestamp, envid, locks)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t opcode;
	int32_t timestamp;
	u_int32_t envid;
	const DBT *locks;
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
	rectype = DB___txn_regop;
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
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (locks == NULL ? 0 : locks->size);
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

	uinttmp = (u_int32_t)timestamp;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &envid);
	bp += sizeof(envid);

	if (locks == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &locks->size);
		bp += sizeof(locks->size);
		memcpy(bp, locks->data, locks->size);
		bp += locks->size;
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
		(void)__txn_regop_print(env,
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
 * PUBLIC: int __txn_ckp_42_read __P((ENV *, void *, __txn_ckp_42_args **));
 */
int
__txn_ckp_42_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__txn_ckp_42_args **argpp;
{
	__txn_ckp_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__txn_ckp_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->ckp_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_TOLSN(env, &argp->last_ckp, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->timestamp = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->rep_gen, bp);
	bp += sizeof(argp->rep_gen);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __txn_ckp_read __P((ENV *, void *, __txn_ckp_args **));
 */
int
__txn_ckp_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__txn_ckp_args **argpp;
{
	__txn_ckp_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__txn_ckp_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->ckp_lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_TOLSN(env, &argp->last_ckp, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->timestamp = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->envid, bp);
	bp += sizeof(argp->envid);

	LOGCOPY_32(env, &argp->spare, bp);
	bp += sizeof(argp->spare);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __txn_ckp_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, DB_LSN *, DB_LSN *, int32_t, u_int32_t, u_int32_t));
 */
int
__txn_ckp_log(env, txnp, ret_lsnp, flags,
    ckp_lsn, last_ckp, timestamp, envid, spare)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	DB_LSN * ckp_lsn;
	DB_LSN * last_ckp;
	int32_t timestamp;
	u_int32_t envid;
	u_int32_t spare;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	DB_TXNLOGREC *lr;
	u_int32_t uinttmp, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int is_durable, ret;

	COMPQUIET(lr, NULL);

	rlsnp = ret_lsnp;
	rectype = DB___txn_ckp;
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
	    + sizeof(*ckp_lsn)
	    + sizeof(*last_ckp)
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

	if (ckp_lsn != NULL)
		LOGCOPY_FROMLSN(env, bp, ckp_lsn);
	else
		memset(bp, 0, sizeof(*ckp_lsn));
	bp += sizeof(*ckp_lsn);

	if (last_ckp != NULL)
		LOGCOPY_FROMLSN(env, bp, last_ckp);
	else
		memset(bp, 0, sizeof(*last_ckp));
	bp += sizeof(*last_ckp);

	uinttmp = (u_int32_t)timestamp;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &envid);
	bp += sizeof(envid);

	LOGCOPY_32(env, bp, &spare);
	bp += sizeof(spare);

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
		(void)__txn_ckp_print(env,
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
 * PUBLIC: int __txn_child_read __P((ENV *, void *, __txn_child_args **));
 */
int
__txn_child_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__txn_child_args **argpp;
{
	__txn_child_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__txn_child_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->child, bp);
	bp += sizeof(argp->child);

	LOGCOPY_TOLSN(env, &argp->c_lsn, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __txn_child_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, DB_LSN *));
 */
int
__txn_child_log(env, txnp, ret_lsnp, flags,
    child, c_lsn)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t child;
	DB_LSN * c_lsn;
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
	rectype = DB___txn_child;
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
	    + sizeof(*c_lsn);
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

	LOGCOPY_32(env, bp, &child);
	bp += sizeof(child);

	if (c_lsn != NULL)
		LOGCOPY_FROMLSN(env, bp, c_lsn);
	else
		memset(bp, 0, sizeof(*c_lsn));
	bp += sizeof(*c_lsn);

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
		(void)__txn_child_print(env,
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
 * PUBLIC: int __txn_xa_regop_42_read __P((ENV *, void *,
 * PUBLIC:     __txn_xa_regop_42_args **));
 */
int
__txn_xa_regop_42_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__txn_xa_regop_42_args **argpp;
{
	__txn_xa_regop_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__txn_xa_regop_42_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->opcode, bp);
	bp += sizeof(argp->opcode);

	memset(&argp->xid, 0, sizeof(argp->xid));
	LOGCOPY_32(env,&argp->xid.size, bp);
	bp += sizeof(u_int32_t);
	argp->xid.data = bp;
	bp += argp->xid.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->formatID = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->gtrid, bp);
	bp += sizeof(argp->gtrid);

	LOGCOPY_32(env, &argp->bqual, bp);
	bp += sizeof(argp->bqual);

	LOGCOPY_TOLSN(env, &argp->begin_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->locks, 0, sizeof(argp->locks));
	LOGCOPY_32(env,&argp->locks.size, bp);
	bp += sizeof(u_int32_t);
	argp->locks.data = bp;
	bp += argp->locks.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __txn_prepare_read __P((ENV *, void *, __txn_prepare_args **));
 */
int
__txn_prepare_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__txn_prepare_args **argpp;
{
	__txn_prepare_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__txn_prepare_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->opcode, bp);
	bp += sizeof(argp->opcode);

	memset(&argp->gid, 0, sizeof(argp->gid));
	LOGCOPY_32(env,&argp->gid.size, bp);
	bp += sizeof(u_int32_t);
	argp->gid.data = bp;
	bp += argp->gid.size;

	LOGCOPY_TOLSN(env, &argp->begin_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->locks, 0, sizeof(argp->locks));
	LOGCOPY_32(env,&argp->locks.size, bp);
	bp += sizeof(u_int32_t);
	argp->locks.data = bp;
	bp += argp->locks.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __txn_prepare_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, const DBT *, DB_LSN *, const DBT *));
 */
int
__txn_prepare_log(env, txnp, ret_lsnp, flags,
    opcode, gid, begin_lsn, locks)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t opcode;
	const DBT *gid;
	DB_LSN * begin_lsn;
	const DBT *locks;
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
	rectype = DB___txn_prepare;
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
	    + sizeof(u_int32_t) + (gid == NULL ? 0 : gid->size)
	    + sizeof(*begin_lsn)
	    + sizeof(u_int32_t) + (locks == NULL ? 0 : locks->size);
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

	if (gid == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &gid->size);
		bp += sizeof(gid->size);
		memcpy(bp, gid->data, gid->size);
		bp += gid->size;
	}

	if (begin_lsn != NULL)
		LOGCOPY_FROMLSN(env, bp, begin_lsn);
	else
		memset(bp, 0, sizeof(*begin_lsn));
	bp += sizeof(*begin_lsn);

	if (locks == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &locks->size);
		bp += sizeof(locks->size);
		memcpy(bp, locks->data, locks->size);
		bp += locks->size;
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
		(void)__txn_prepare_print(env,
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
 * PUBLIC: int __txn_recycle_read __P((ENV *, void *, __txn_recycle_args **));
 */
int
__txn_recycle_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__txn_recycle_args **argpp;
{
	__txn_recycle_args *argp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__txn_recycle_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_32(env, &argp->min, bp);
	bp += sizeof(argp->min);

	LOGCOPY_32(env, &argp->max, bp);
	bp += sizeof(argp->max);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __txn_recycle_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, u_int32_t));
 */
int
__txn_recycle_log(env, txnp, ret_lsnp, flags,
    min, max)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t min;
	u_int32_t max;
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
	rectype = DB___txn_recycle;
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

	LOGCOPY_32(env, bp, &min);
	bp += sizeof(min);

	LOGCOPY_32(env, bp, &max);
	bp += sizeof(max);

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
		(void)__txn_recycle_print(env,
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
 * PUBLIC: int __txn_init_recover __P((ENV *, DB_DISTAB *));
 */
int
__txn_init_recover(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_regop_recover, DB___txn_regop)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_ckp_recover, DB___txn_ckp)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_child_recover, DB___txn_child)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_prepare_recover, DB___txn_prepare)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_recycle_recover, DB___txn_recycle)) != 0)
		return (ret);
	return (0);
}
