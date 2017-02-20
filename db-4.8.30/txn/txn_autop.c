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
 * PUBLIC: int __txn_regop_42_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_regop_42_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_regop_42_args *argp;
	struct tm *lt;
	time_t timeval;
	char time_buf[CTIME_BUFLEN];
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret = __txn_regop_42_read(env, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__txn_regop_42%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\topcode: %lu\n", (u_long)argp->opcode);
	timeval = (time_t)argp->timestamp;
	lt = localtime(&timeval);
	(void)printf(
	    "\ttimestamp: %ld (%.24s, 20%02lu%02lu%02lu%02lu%02lu.%02lu)\n",
	    (long)argp->timestamp, __os_ctime(&timeval, time_buf),
	    (u_long)lt->tm_year - 100, (u_long)lt->tm_mon+1,
	    (u_long)lt->tm_mday, (u_long)lt->tm_hour,
	    (u_long)lt->tm_min, (u_long)lt->tm_sec);
	(void)printf("\tlocks: \n");
	__lock_list_print(env, &argp->locks);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_regop_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_regop_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_regop_args *argp;
	struct tm *lt;
	time_t timeval;
	char time_buf[CTIME_BUFLEN];
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret = __txn_regop_read(env, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__txn_regop%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\topcode: %lu\n", (u_long)argp->opcode);
	timeval = (time_t)argp->timestamp;
	lt = localtime(&timeval);
	(void)printf(
	    "\ttimestamp: %ld (%.24s, 20%02lu%02lu%02lu%02lu%02lu.%02lu)\n",
	    (long)argp->timestamp, __os_ctime(&timeval, time_buf),
	    (u_long)lt->tm_year - 100, (u_long)lt->tm_mon+1,
	    (u_long)lt->tm_mday, (u_long)lt->tm_hour,
	    (u_long)lt->tm_min, (u_long)lt->tm_sec);
	(void)printf("\tenvid: %lu\n", (u_long)argp->envid);
	(void)printf("\tlocks: \n");
	__lock_list_print(env, &argp->locks);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_ckp_42_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_ckp_42_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_ckp_42_args *argp;
	struct tm *lt;
	time_t timeval;
	char time_buf[CTIME_BUFLEN];
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret = __txn_ckp_42_read(env, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__txn_ckp_42%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tckp_lsn: [%lu][%lu]\n",
	    (u_long)argp->ckp_lsn.file, (u_long)argp->ckp_lsn.offset);
	(void)printf("\tlast_ckp: [%lu][%lu]\n",
	    (u_long)argp->last_ckp.file, (u_long)argp->last_ckp.offset);
	timeval = (time_t)argp->timestamp;
	lt = localtime(&timeval);
	(void)printf(
	    "\ttimestamp: %ld (%.24s, 20%02lu%02lu%02lu%02lu%02lu.%02lu)\n",
	    (long)argp->timestamp, __os_ctime(&timeval, time_buf),
	    (u_long)lt->tm_year - 100, (u_long)lt->tm_mon+1,
	    (u_long)lt->tm_mday, (u_long)lt->tm_hour,
	    (u_long)lt->tm_min, (u_long)lt->tm_sec);
	(void)printf("\trep_gen: %lu\n", (u_long)argp->rep_gen);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_ckp_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_ckp_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_ckp_args *argp;
	struct tm *lt;
	time_t timeval;
	char time_buf[CTIME_BUFLEN];
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret = __txn_ckp_read(env, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__txn_ckp%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tckp_lsn: [%lu][%lu]\n",
	    (u_long)argp->ckp_lsn.file, (u_long)argp->ckp_lsn.offset);
	(void)printf("\tlast_ckp: [%lu][%lu]\n",
	    (u_long)argp->last_ckp.file, (u_long)argp->last_ckp.offset);
	timeval = (time_t)argp->timestamp;
	lt = localtime(&timeval);
	(void)printf(
	    "\ttimestamp: %ld (%.24s, 20%02lu%02lu%02lu%02lu%02lu.%02lu)\n",
	    (long)argp->timestamp, __os_ctime(&timeval, time_buf),
	    (u_long)lt->tm_year - 100, (u_long)lt->tm_mon+1,
	    (u_long)lt->tm_mday, (u_long)lt->tm_hour,
	    (u_long)lt->tm_min, (u_long)lt->tm_sec);
	(void)printf("\tenvid: %lu\n", (u_long)argp->envid);
	(void)printf("\tspare: %lu\n", (u_long)argp->spare);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_child_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_child_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_child_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret = __txn_child_read(env, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__txn_child%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tchild: 0x%lx\n", (u_long)argp->child);
	(void)printf("\tc_lsn: [%lu][%lu]\n",
	    (u_long)argp->c_lsn.file, (u_long)argp->c_lsn.offset);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_xa_regop_42_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_xa_regop_42_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_xa_regop_42_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret = __txn_xa_regop_42_read(env, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__txn_xa_regop_42%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\topcode: %lu\n", (u_long)argp->opcode);
	(void)printf("\txid: ");
	for (i = 0; i < argp->xid.size; i++) {
		ch = ((u_int8_t *)argp->xid.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tformatID: %ld\n", (long)argp->formatID);
	(void)printf("\tgtrid: %lu\n", (u_long)argp->gtrid);
	(void)printf("\tbqual: %lu\n", (u_long)argp->bqual);
	(void)printf("\tbegin_lsn: [%lu][%lu]\n",
	    (u_long)argp->begin_lsn.file, (u_long)argp->begin_lsn.offset);
	(void)printf("\tlocks: \n");
	__lock_list_print(env, &argp->locks);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_prepare_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_prepare_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_prepare_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret = __txn_prepare_read(env, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__txn_prepare%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\topcode: %lu\n", (u_long)argp->opcode);
	(void)printf("\tgid: ");
	for (i = 0; i < argp->gid.size; i++) {
		ch = ((u_int8_t *)argp->gid.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tbegin_lsn: [%lu][%lu]\n",
	    (u_long)argp->begin_lsn.file, (u_long)argp->begin_lsn.offset);
	(void)printf("\tlocks: \n");
	__lock_list_print(env, &argp->locks);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_recycle_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__txn_recycle_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__txn_recycle_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret = __txn_recycle_read(env, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__txn_recycle%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tmin: %lu\n", (u_long)argp->min);
	(void)printf("\tmax: %lu\n", (u_long)argp->max);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __txn_init_print __P((ENV *, DB_DISTAB *));
 */
int
__txn_init_print(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_regop_print, DB___txn_regop)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_ckp_print, DB___txn_ckp)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_child_print, DB___txn_child)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_prepare_print, DB___txn_prepare)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_recycle_print, DB___txn_recycle)) != 0)
		return (ret);
	return (0);
}
