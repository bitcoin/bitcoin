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
 * PUBLIC: int __dbreg_register_read __P((ENV *, void *,
 * PUBLIC:     __dbreg_register_args **));
 */
int
__dbreg_register_read(env, recbuf, argpp)
	ENV *env;
	void *recbuf;
	__dbreg_register_args **argpp;
{
	__dbreg_register_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__dbreg_register_args) + sizeof(DB_TXN), &argp)) != 0)
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

	memset(&argp->name, 0, sizeof(argp->name));
	LOGCOPY_32(env,&argp->name.size, bp);
	bp += sizeof(u_int32_t);
	argp->name.data = bp;
	bp += argp->name.size;

	memset(&argp->uid, 0, sizeof(argp->uid));
	LOGCOPY_32(env,&argp->uid.size, bp);
	bp += sizeof(u_int32_t);
	argp->uid.data = bp;
	bp += argp->uid.size;

	LOGCOPY_32(env, &uinttmp, bp);
	argp->fileid = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->ftype = (DBTYPE)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->meta_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->id, bp);
	bp += sizeof(argp->id);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __dbreg_register_log __P((ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, u_int32_t, const DBT *, const DBT *, int32_t, DBTYPE,
 * PUBLIC:     db_pgno_t, u_int32_t));
 */
int
__dbreg_register_log(env, txnp, ret_lsnp, flags,
    opcode, name, uid, fileid, ftype, meta_pgno,
    id)
	ENV *env;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t opcode;
	const DBT *name;
	const DBT *uid;
	int32_t fileid;
	DBTYPE ftype;
	db_pgno_t meta_pgno;
	u_int32_t id;
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
	rectype = DB___dbreg_register;
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
	    + sizeof(u_int32_t) + (uid == NULL ? 0 : uid->size)
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

	LOGCOPY_32(env, bp, &opcode);
	bp += sizeof(opcode);

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

	if (uid == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &uid->size);
		bp += sizeof(uid->size);
		memcpy(bp, uid->data, uid->size);
		bp += uid->size;
	}

	uinttmp = (u_int32_t)fileid;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)ftype;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)meta_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &id);
	bp += sizeof(id);

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
		(void)__dbreg_register_print(env,
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
 * PUBLIC: int __dbreg_init_recover __P((ENV *, DB_DISTAB *));
 */
int
__dbreg_init_recover(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __dbreg_register_recover, DB___dbreg_register)) != 0)
		return (ret);
	return (0);
}
