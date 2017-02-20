/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "db_int.h"
#include "dbinc/db_swap.h"
#include "ex_apprec.h"
/*
 * PUBLIC: int ex_apprec_mkdir_read __P((DB_ENV *, void *,
 * PUBLIC:     ex_apprec_mkdir_args **));
 */
int
ex_apprec_mkdir_read(dbenv, recbuf, argpp)
	DB_ENV *dbenv;
	void *recbuf;
	ex_apprec_mkdir_args **argpp;
{
	ex_apprec_mkdir_args *argp;
	u_int8_t *bp;
	ENV *env;

	env = dbenv->env;

	if ((argp = malloc(sizeof(ex_apprec_mkdir_args) + sizeof(DB_TXN))) == NULL)
		return (ENOMEM);
	bp = recbuf;
	argp->txnp = (DB_TXN *)&argp[1];
	memset(argp->txnp, 0, sizeof(DB_TXN));

	LOGCOPY_32(env, &argp->type, bp);
	bp += sizeof(argp->type);

	LOGCOPY_32(env, &argp->txnp->txnid, bp);
	bp += sizeof(argp->txnp->txnid);

	LOGCOPY_TOLSN(env, &argp->prev_lsn, bp);
	bp += sizeof(DB_LSN);

	memset(&argp->dirname, 0, sizeof(argp->dirname));
	LOGCOPY_32(env,&argp->dirname.size, bp);
	bp += sizeof(u_int32_t);
	argp->dirname.data = bp;
	bp += argp->dirname.size;

	*argpp = argp;
	return (0);
}

/*
 * PUBLIC: int ex_apprec_mkdir_log __P((DB_ENV *, DB_TXN *, DB_LSN *,
 * PUBLIC:     u_int32_t, const DBT *));
 */
int
ex_apprec_mkdir_log(dbenv, txnp, ret_lsnp, flags,
    dirname)
	DB_ENV *dbenv;
	DB_TXN *txnp;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	const DBT *dirname;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn, *rlsnp;
	ENV *env;
	u_int32_t zero, rectype, txn_num;
	u_int npad;
	u_int8_t *bp;
	int ret;

	env = dbenv->env;
	rlsnp = ret_lsnp;
	rectype = DB_ex_apprec_mkdir;
	npad = 0;
	ret = 0;

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
	    + sizeof(u_int32_t) + (dirname == NULL ? 0 : dirname->size);
	if ((logrec.data = malloc(logrec.size)) == NULL)
		return (ENOMEM);
	bp = logrec.data;

	if (npad > 0)
		memset((u_int8_t *)logrec.data + logrec.size - npad, 0, npad);

	bp = logrec.data;

	LOGCOPY_32(env, bp, &rectype);
	bp += sizeof(rectype);

	LOGCOPY_32(env, bp, &txn_num);
	bp += sizeof(txn_num);

	LOGCOPY_FROMLSN(env, bp, lsnp);
	bp += sizeof(DB_LSN);

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

	if ((ret = dbenv->log_put(dbenv, rlsnp, (DBT *)&logrec,
	    flags | DB_LOG_NOCOPY)) == 0 && txnp != NULL) {
		*lsnp = *rlsnp;
		if (rlsnp != ret_lsnp)
			 *ret_lsnp = *rlsnp;
	}
#ifdef LOG_DIAGNOSTIC
	if (ret != 0)
		(void)ex_apprec_mkdir_print(dbenv,
		    (DBT *)&logrec, ret_lsnp, DB_TXN_PRINT);
#endif

	free(logrec.data);
	return (ret);
}

