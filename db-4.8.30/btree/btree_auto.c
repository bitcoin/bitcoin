/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"
#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/btree.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

/*
 * PUBLIC: int __bam_split_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_split_args **));
 */
int
__bam_split_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_split_args **argpp;
{
	__bam_split_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_split_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->left = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->llsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->right = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->rlsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &argp->indx, bp);
	bp += sizeof(argp->indx);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->npgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->nlsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->ppgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->plsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &argp->pindx, bp);
	bp += sizeof(argp->pindx);

	memset(&argp->pg, 0, sizeof(argp->pg));
	LOGCOPY_32(env,&argp->pg.size, bp);
	bp += sizeof(u_int32_t);
	argp->pg.data = bp;
	bp += argp->pg.size;
	if (LOG_SWAPPED(env) && dbpp != NULL && *dbpp != NULL) {
		int t_ret;
		if ((t_ret = __db_pageswap(*dbpp, (PAGE *)argp->pg.data,
		    (size_t)argp->pg.size, NULL, 1)) != 0)
			return (t_ret);
	}

	memset(&argp->pentry, 0, sizeof(argp->pentry));
	LOGCOPY_32(env,&argp->pentry.size, bp);
	bp += sizeof(u_int32_t);
	argp->pentry.data = bp;
	bp += argp->pentry.size;

	memset(&argp->rentry, 0, sizeof(argp->rentry));
	LOGCOPY_32(env,&argp->rentry.size, bp);
	bp += sizeof(u_int32_t);
	argp->rentry.data = bp;
	bp += argp->rentry.size;

	LOGCOPY_32(env, &argp->opflags, bp);
	bp += sizeof(argp->opflags);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_split_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, db_pgno_t, DB_LSN *, u_int32_t,
 * PUBLIC:     db_pgno_t, DB_LSN *, db_pgno_t, DB_LSN *, u_int32_t, const DBT *,
 * PUBLIC:     const DBT *, const DBT *, u_int32_t));
 */
int
__bam_split_log(dbp, txnp, ret_lsnp, flags, left, llsn, right, rlsn, indx,
    npgno, nlsn, ppgno, plsn, pindx, pg,
    pentry, rentry, opflags)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t left;
	DB_LSN * llsn;
	db_pgno_t right;
	DB_LSN * rlsn;
	u_int32_t indx;
	db_pgno_t npgno;
	DB_LSN * nlsn;
	db_pgno_t ppgno;
	DB_LSN * plsn;
	u_int32_t pindx;
	const DBT *pg;
	const DBT *pentry;
	const DBT *rentry;
	u_int32_t opflags;
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
	rectype = DB___bam_split;
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
	    + sizeof(*llsn)
	    + sizeof(u_int32_t)
	    + sizeof(*rlsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(*nlsn)
	    + sizeof(u_int32_t)
	    + sizeof(*plsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (pg == NULL ? 0 : pg->size)
	    + sizeof(u_int32_t) + (pentry == NULL ? 0 : pentry->size)
	    + sizeof(u_int32_t) + (rentry == NULL ? 0 : rentry->size)
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

	uinttmp = (u_int32_t)left;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (llsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(llsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, llsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, llsn);
	} else
		memset(bp, 0, sizeof(*llsn));
	bp += sizeof(*llsn);

	uinttmp = (u_int32_t)right;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (rlsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(rlsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, rlsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, rlsn);
	} else
		memset(bp, 0, sizeof(*rlsn));
	bp += sizeof(*rlsn);

	LOGCOPY_32(env, bp, &indx);
	bp += sizeof(indx);

	uinttmp = (u_int32_t)npgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (nlsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(nlsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, nlsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, nlsn);
	} else
		memset(bp, 0, sizeof(*nlsn));
	bp += sizeof(*nlsn);

	uinttmp = (u_int32_t)ppgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (plsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(plsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, plsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, plsn);
	} else
		memset(bp, 0, sizeof(*plsn));
	bp += sizeof(*plsn);

	LOGCOPY_32(env, bp, &pindx);
	bp += sizeof(pindx);

	if (pg == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &pg->size);
		bp += sizeof(pg->size);
		memcpy(bp, pg->data, pg->size);
		if (LOG_SWAPPED(env))
			if ((ret = __db_pageswap(dbp,
			    (PAGE *)bp, (size_t)pg->size, (DBT *)NULL, 0)) != 0)
				return (ret);
		bp += pg->size;
	}

	if (pentry == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &pentry->size);
		bp += sizeof(pentry->size);
		memcpy(bp, pentry->data, pentry->size);
		bp += pentry->size;
	}

	if (rentry == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &rentry->size);
		bp += sizeof(rentry->size);
		memcpy(bp, rentry->data, rentry->size);
		bp += rentry->size;
	}

	LOGCOPY_32(env, bp, &opflags);
	bp += sizeof(opflags);

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
		(void)__bam_split_print(env,
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
 * PUBLIC: int __bam_split_42_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __bam_split_42_args **));
 */
int
__bam_split_42_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_split_42_args **argpp;
{
	__bam_split_42_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_split_42_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->left = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->llsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->right = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->rlsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &argp->indx, bp);
	bp += sizeof(argp->indx);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->npgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->nlsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->root_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->pg, 0, sizeof(argp->pg));
	LOGCOPY_32(env,&argp->pg.size, bp);
	bp += sizeof(u_int32_t);
	argp->pg.data = bp;
	bp += argp->pg.size;

	LOGCOPY_32(env, &argp->opflags, bp);
	bp += sizeof(argp->opflags);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_rsplit_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_rsplit_args **));
 */
int
__bam_rsplit_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_rsplit_args **argpp;
{
	__bam_rsplit_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_rsplit_args) + sizeof(DB_TXN), &argp)) != 0)
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

	memset(&argp->pgdbt, 0, sizeof(argp->pgdbt));
	LOGCOPY_32(env,&argp->pgdbt.size, bp);
	bp += sizeof(u_int32_t);
	argp->pgdbt.data = bp;
	bp += argp->pgdbt.size;
	if (LOG_SWAPPED(env) && dbpp != NULL && *dbpp != NULL) {
		int t_ret;
		if ((t_ret = __db_pageswap(*dbpp, (PAGE *)argp->pgdbt.data,
		    (size_t)argp->pgdbt.size, NULL, 1)) != 0)
			return (t_ret);
	}

	LOGCOPY_32(env, &uinttmp, bp);
	argp->root_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->nrec = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	memset(&argp->rootent, 0, sizeof(argp->rootent));
	LOGCOPY_32(env,&argp->rootent.size, bp);
	bp += sizeof(u_int32_t);
	argp->rootent.data = bp;
	bp += argp->rootent.size;

	LOGCOPY_TOLSN(env, &argp->rootlsn, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_rsplit_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, const DBT *, db_pgno_t, db_pgno_t,
 * PUBLIC:     const DBT *, DB_LSN *));
 */
int
__bam_rsplit_log(dbp, txnp, ret_lsnp, flags, pgno, pgdbt, root_pgno, nrec, rootent,
    rootlsn)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	const DBT *pgdbt;
	db_pgno_t root_pgno;
	db_pgno_t nrec;
	const DBT *rootent;
	DB_LSN * rootlsn;
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
	rectype = DB___bam_rsplit;
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
	    + sizeof(u_int32_t) + (pgdbt == NULL ? 0 : pgdbt->size)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (rootent == NULL ? 0 : rootent->size)
	    + sizeof(*rootlsn);
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

	if (pgdbt == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &pgdbt->size);
		bp += sizeof(pgdbt->size);
		memcpy(bp, pgdbt->data, pgdbt->size);
		if (LOG_SWAPPED(env))
			if ((ret = __db_pageswap(dbp,
			    (PAGE *)bp, (size_t)pgdbt->size, (DBT *)NULL, 0)) != 0)
				return (ret);
		bp += pgdbt->size;
	}

	uinttmp = (u_int32_t)root_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)nrec;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (rootent == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &rootent->size);
		bp += sizeof(rootent->size);
		memcpy(bp, rootent->data, rootent->size);
		bp += rootent->size;
	}

	if (rootlsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(rootlsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, rootlsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, rootlsn);
	} else
		memset(bp, 0, sizeof(*rootlsn));
	bp += sizeof(*rootlsn);

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
		(void)__bam_rsplit_print(env,
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
 * PUBLIC: int __bam_adj_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_adj_args **));
 */
int
__bam_adj_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_adj_args **argpp;
{
	__bam_adj_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_adj_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &argp->indx, bp);
	bp += sizeof(argp->indx);

	LOGCOPY_32(env, &argp->indx_copy, bp);
	bp += sizeof(argp->indx_copy);

	LOGCOPY_32(env, &argp->is_insert, bp);
	bp += sizeof(argp->is_insert);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_adj_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, u_int32_t, u_int32_t,
 * PUBLIC:     u_int32_t));
 */
int
__bam_adj_log(dbp, txnp, ret_lsnp, flags, pgno, lsn, indx, indx_copy, is_insert)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
	u_int32_t indx_copy;
	u_int32_t is_insert;
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
	rectype = DB___bam_adj;
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
	    + sizeof(*lsn)
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

	LOGCOPY_32(env, bp, &indx);
	bp += sizeof(indx);

	LOGCOPY_32(env, bp, &indx_copy);
	bp += sizeof(indx_copy);

	LOGCOPY_32(env, bp, &is_insert);
	bp += sizeof(is_insert);

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
		(void)__bam_adj_print(env,
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
 * PUBLIC: int __bam_cadjust_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_cadjust_args **));
 */
int
__bam_cadjust_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_cadjust_args **argpp;
{
	__bam_cadjust_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_cadjust_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &argp->indx, bp);
	bp += sizeof(argp->indx);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->adjust = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->opflags, bp);
	bp += sizeof(argp->opflags);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_cadjust_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, u_int32_t, int32_t, u_int32_t));
 */
int
__bam_cadjust_log(dbp, txnp, ret_lsnp, flags, pgno, lsn, indx, adjust, opflags)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
	int32_t adjust;
	u_int32_t opflags;
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
	rectype = DB___bam_cadjust;
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
	    + sizeof(*lsn)
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

	LOGCOPY_32(env, bp, &indx);
	bp += sizeof(indx);

	uinttmp = (u_int32_t)adjust;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &opflags);
	bp += sizeof(opflags);

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
		(void)__bam_cadjust_print(env,
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
 * PUBLIC: int __bam_cdel_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_cdel_args **));
 */
int
__bam_cdel_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_cdel_args **argpp;
{
	__bam_cdel_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_cdel_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &argp->indx, bp);
	bp += sizeof(argp->indx);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_cdel_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, u_int32_t));
 */
int
__bam_cdel_log(dbp, txnp, ret_lsnp, flags, pgno, lsn, indx)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
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
	rectype = DB___bam_cdel;
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
	    + sizeof(*lsn)
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

	LOGCOPY_32(env, bp, &indx);
	bp += sizeof(indx);

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
		(void)__bam_cdel_print(env,
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
 * PUBLIC: int __bam_repl_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_repl_args **));
 */
int
__bam_repl_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_repl_args **argpp;
{
	__bam_repl_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_repl_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &argp->indx, bp);
	bp += sizeof(argp->indx);

	LOGCOPY_32(env, &argp->isdeleted, bp);
	bp += sizeof(argp->isdeleted);

	memset(&argp->orig, 0, sizeof(argp->orig));
	LOGCOPY_32(env,&argp->orig.size, bp);
	bp += sizeof(u_int32_t);
	argp->orig.data = bp;
	bp += argp->orig.size;

	memset(&argp->repl, 0, sizeof(argp->repl));
	LOGCOPY_32(env,&argp->repl.size, bp);
	bp += sizeof(u_int32_t);
	argp->repl.data = bp;
	bp += argp->repl.size;

	LOGCOPY_32(env, &argp->prefix, bp);
	bp += sizeof(argp->prefix);

	LOGCOPY_32(env, &argp->suffix, bp);
	bp += sizeof(argp->suffix);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_repl_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, u_int32_t, u_int32_t,
 * PUBLIC:     const DBT *, const DBT *, u_int32_t, u_int32_t));
 */
int
__bam_repl_log(dbp, txnp, ret_lsnp, flags, pgno, lsn, indx, isdeleted, orig,
    repl, prefix, suffix)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
	u_int32_t isdeleted;
	const DBT *orig;
	const DBT *repl;
	u_int32_t prefix;
	u_int32_t suffix;
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
	rectype = DB___bam_repl;
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
	    + sizeof(*lsn)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t)
	    + sizeof(u_int32_t) + (orig == NULL ? 0 : orig->size)
	    + sizeof(u_int32_t) + (repl == NULL ? 0 : repl->size)
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

	LOGCOPY_32(env, bp, &indx);
	bp += sizeof(indx);

	LOGCOPY_32(env, bp, &isdeleted);
	bp += sizeof(isdeleted);

	if (orig == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &orig->size);
		bp += sizeof(orig->size);
		memcpy(bp, orig->data, orig->size);
		bp += orig->size;
	}

	if (repl == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &repl->size);
		bp += sizeof(repl->size);
		memcpy(bp, repl->data, repl->size);
		bp += repl->size;
	}

	LOGCOPY_32(env, bp, &prefix);
	bp += sizeof(prefix);

	LOGCOPY_32(env, bp, &suffix);
	bp += sizeof(suffix);

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
		(void)__bam_repl_print(env,
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
 * PUBLIC: int __bam_root_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_root_args **));
 */
int
__bam_root_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_root_args **argpp;
{
	__bam_root_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_root_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->meta_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->root_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->meta_lsn, bp);
	bp += sizeof(DB_LSN);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_root_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, db_pgno_t, DB_LSN *));
 */
int
__bam_root_log(dbp, txnp, ret_lsnp, flags, meta_pgno, root_pgno, meta_lsn)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t meta_pgno;
	db_pgno_t root_pgno;
	DB_LSN * meta_lsn;
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
	rectype = DB___bam_root;
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
	    + sizeof(*meta_lsn);
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

	uinttmp = (u_int32_t)meta_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)root_pgno;
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
		(void)__bam_root_print(env,
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
 * PUBLIC: int __bam_curadj_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_curadj_args **));
 */
int
__bam_curadj_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_curadj_args **argpp;
{
	__bam_curadj_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_curadj_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->mode = (db_ca_mode)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->from_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->to_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->left_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->first_indx, bp);
	bp += sizeof(argp->first_indx);

	LOGCOPY_32(env, &argp->from_indx, bp);
	bp += sizeof(argp->from_indx);

	LOGCOPY_32(env, &argp->to_indx, bp);
	bp += sizeof(argp->to_indx);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_curadj_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_ca_mode, db_pgno_t, db_pgno_t, db_pgno_t,
 * PUBLIC:     u_int32_t, u_int32_t, u_int32_t));
 */
int
__bam_curadj_log(dbp, txnp, ret_lsnp, flags, mode, from_pgno, to_pgno, left_pgno, first_indx,
    from_indx, to_indx)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_ca_mode mode;
	db_pgno_t from_pgno;
	db_pgno_t to_pgno;
	db_pgno_t left_pgno;
	u_int32_t first_indx;
	u_int32_t from_indx;
	u_int32_t to_indx;
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
	rectype = DB___bam_curadj;
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

	uinttmp = (u_int32_t)mode;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)from_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)to_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)left_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, bp, &first_indx);
	bp += sizeof(first_indx);

	LOGCOPY_32(env, bp, &from_indx);
	bp += sizeof(from_indx);

	LOGCOPY_32(env, bp, &to_indx);
	bp += sizeof(to_indx);

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
		(void)__bam_curadj_print(env,
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
 * PUBLIC: int __bam_rcuradj_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_rcuradj_args **));
 */
int
__bam_rcuradj_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_rcuradj_args **argpp;
{
	__bam_rcuradj_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_rcuradj_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->mode = (ca_recno_arg)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->root = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->recno = (db_recno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &argp->order, bp);
	bp += sizeof(argp->order);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_rcuradj_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, ca_recno_arg, db_pgno_t, db_recno_t, u_int32_t));
 */
int
__bam_rcuradj_log(dbp, txnp, ret_lsnp, flags, mode, root, recno, order)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	ca_recno_arg mode;
	db_pgno_t root;
	db_recno_t recno;
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
	rectype = DB___bam_rcuradj;
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

	uinttmp = (u_int32_t)root;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)recno;
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
		(void)__bam_rcuradj_print(env,
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
 * PUBLIC: int __bam_relink_43_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __bam_relink_43_args **));
 */
int
__bam_relink_43_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_relink_43_args **argpp;
{
	__bam_relink_43_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_relink_43_args) + sizeof(DB_TXN), &argp)) != 0)
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
 * PUBLIC: int __bam_relink_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_relink_args **));
 */
int
__bam_relink_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_relink_args **argpp;
{
	__bam_relink_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_relink_args) + sizeof(DB_TXN), &argp)) != 0)
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
	argp->new_pgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

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
 * PUBLIC: int __bam_relink_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, db_pgno_t, db_pgno_t, DB_LSN *, db_pgno_t,
 * PUBLIC:     DB_LSN *));
 */
int
__bam_relink_log(dbp, txnp, ret_lsnp, flags, pgno, new_pgno, prev, lsn_prev, next,
    lsn_next)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	db_pgno_t new_pgno;
	db_pgno_t prev;
	DB_LSN * lsn_prev;
	db_pgno_t next;
	DB_LSN * lsn_next;
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
	rectype = DB___bam_relink;
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
	    + sizeof(*lsn_prev)
	    + sizeof(u_int32_t)
	    + sizeof(*lsn_next);
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

	uinttmp = (u_int32_t)new_pgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)prev;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (lsn_prev != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(lsn_prev, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, lsn_prev)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, lsn_prev);
	} else
		memset(bp, 0, sizeof(*lsn_prev));
	bp += sizeof(*lsn_prev);

	uinttmp = (u_int32_t)next;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (lsn_next != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(lsn_next, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, lsn_next)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, lsn_next);
	} else
		memset(bp, 0, sizeof(*lsn_next));
	bp += sizeof(*lsn_next);

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
		(void)__bam_relink_print(env,
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
 * PUBLIC: int __bam_merge_44_read __P((ENV *, DB **, void *,
 * PUBLIC:     void *, __bam_merge_44_args **));
 */
int
__bam_merge_44_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_merge_44_args **argpp;
{
	__bam_merge_44_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_merge_44_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->npgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->nlsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->hdr, 0, sizeof(argp->hdr));
	LOGCOPY_32(env,&argp->hdr.size, bp);
	bp += sizeof(u_int32_t);
	argp->hdr.data = bp;
	bp += argp->hdr.size;

	memset(&argp->data, 0, sizeof(argp->data));
	LOGCOPY_32(env,&argp->data.size, bp);
	bp += sizeof(u_int32_t);
	argp->data.data = bp;
	bp += argp->data.size;

	memset(&argp->ind, 0, sizeof(argp->ind));
	LOGCOPY_32(env,&argp->ind.size, bp);
	bp += sizeof(u_int32_t);
	argp->ind.data = bp;
	bp += argp->ind.size;

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_merge_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_merge_args **));
 */
int
__bam_merge_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_merge_args **argpp;
{
	__bam_merge_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_merge_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->npgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_TOLSN(env, &argp->nlsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->hdr, 0, sizeof(argp->hdr));
	LOGCOPY_32(env,&argp->hdr.size, bp);
	bp += sizeof(u_int32_t);
	argp->hdr.data = bp;
	bp += argp->hdr.size;

	memset(&argp->data, 0, sizeof(argp->data));
	LOGCOPY_32(env,&argp->data.size, bp);
	bp += sizeof(u_int32_t);
	argp->data.data = bp;
	bp += argp->data.size;
	if (LOG_SWAPPED(env) && dbpp != NULL && *dbpp != NULL) {
		int t_ret;
		if ((t_ret = __db_pageswap(*dbpp,
		    (PAGE *)argp->hdr.data, (size_t)argp->hdr.size,
		    &argp->data, 1)) != 0)
			return (t_ret);
	}

	LOGCOPY_32(env, &uinttmp, bp);
	argp->pg_copy = (int32_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_merge_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, db_pgno_t, DB_LSN *, const DBT *,
 * PUBLIC:     const DBT *, int32_t));
 */
int
__bam_merge_log(dbp, txnp, ret_lsnp, flags, pgno, lsn, npgno, nlsn, hdr,
    data, pg_copy)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	db_pgno_t npgno;
	DB_LSN * nlsn;
	const DBT *hdr;
	const DBT *data;
	int32_t pg_copy;
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
	rectype = DB___bam_merge;
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
	    + sizeof(*lsn)
	    + sizeof(u_int32_t)
	    + sizeof(*nlsn)
	    + sizeof(u_int32_t) + (hdr == NULL ? 0 : hdr->size)
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

	uinttmp = (u_int32_t)dbp->log_filename->id;
	LOGCOPY_32(env, bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)pgno;
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

	uinttmp = (u_int32_t)npgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	if (nlsn != NULL) {
		if (txnp != NULL) {
			LOG *lp = env->lg_handle->reginfo.primary;
			if (LOG_COMPARE(nlsn, &lp->lsn) >= 0 && (ret =
			    __log_check_page_lsn(env, dbp, nlsn)) != 0)
				return (ret);
		}
		LOGCOPY_FROMLSN(env, bp, nlsn);
	} else
		memset(bp, 0, sizeof(*nlsn));
	bp += sizeof(*nlsn);

	if (hdr == NULL) {
		zero = 0;
		LOGCOPY_32(env, bp, &zero);
		bp += sizeof(u_int32_t);
	} else {
		LOGCOPY_32(env, bp, &hdr->size);
		bp += sizeof(hdr->size);
		memcpy(bp, hdr->data, hdr->size);
		if (LOG_SWAPPED(env))
			if ((ret = __db_pageswap(dbp,
			    (PAGE *)bp, (size_t)hdr->size, (DBT *)data, 0)) != 0)
				return (ret);
		bp += hdr->size;
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

	uinttmp = (u_int32_t)pg_copy;
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
		(void)__bam_merge_print(env,
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
 * PUBLIC: int __bam_pgno_read __P((ENV *, DB **, void *, void *,
 * PUBLIC:     __bam_pgno_args **));
 */
int
__bam_pgno_read(env, dbpp, td, recbuf, argpp)
	ENV *env;
	DB **dbpp;
	void *td;
	void *recbuf;
	__bam_pgno_args **argpp;
{
	__bam_pgno_args *argp;
	u_int32_t uinttmp;
	u_int8_t *bp;
	int ret;

	if ((ret = __os_malloc(env,
	    sizeof(__bam_pgno_args) + sizeof(DB_TXN), &argp)) != 0)
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

	LOGCOPY_TOLSN(env, &argp->lsn, bp);
	bp += sizeof(DB_LSN);

	LOGCOPY_32(env, &argp->indx, bp);
	bp += sizeof(argp->indx);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->opgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	LOGCOPY_32(env, &uinttmp, bp);
	argp->npgno = (db_pgno_t)uinttmp;
	bp += sizeof(uinttmp);

	*argpp = argp;
	return (ret);
}

/*
 * PUBLIC: int __bam_pgno_log __P((DB *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, db_pgno_t, DB_LSN *, u_int32_t, db_pgno_t,
 * PUBLIC:     db_pgno_t));
 */
int
__bam_pgno_log(dbp, txnp, ret_lsnp, flags, pgno, lsn, indx, opgno, npgno)
	DB *dbp;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
	db_pgno_t opgno;
	db_pgno_t npgno;
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
	rectype = DB___bam_pgno;
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
	    + sizeof(*lsn)
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

	LOGCOPY_32(env, bp, &indx);
	bp += sizeof(indx);

	uinttmp = (u_int32_t)opgno;
	LOGCOPY_32(env,bp, &uinttmp);
	bp += sizeof(uinttmp);

	uinttmp = (u_int32_t)npgno;
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
		(void)__bam_pgno_print(env,
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
 * PUBLIC: int __bam_init_recover __P((ENV *, DB_DISTAB *));
 */
int
__bam_init_recover(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_split_recover, DB___bam_split)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_rsplit_recover, DB___bam_rsplit)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_adj_recover, DB___bam_adj)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_cadjust_recover, DB___bam_cadjust)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_cdel_recover, DB___bam_cdel)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_repl_recover, DB___bam_repl)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_root_recover, DB___bam_root)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_curadj_recover, DB___bam_curadj)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_rcuradj_recover, DB___bam_rcuradj)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_relink_recover, DB___bam_relink)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_merge_recover, DB___bam_merge)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_pgno_recover, DB___bam_pgno)) != 0)
		return (ret);
	return (0);
}
