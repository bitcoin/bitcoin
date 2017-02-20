/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"

#ifdef HAVE_QUEUE
#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_dispatch.h"
#include "dbinc/db_am.h"
#include "dbinc/log.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

/*
 * PUBLIC: int __qam_incfirst_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__qam_incfirst_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__qam_incfirst_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __qam_incfirst_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__qam_incfirst%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\trecno: %lu\n", (u_long)argp->recno);
	(void)printf("\tmeta_pgno: %lu\n", (u_long)argp->meta_pgno);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __qam_mvptr_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__qam_mvptr_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__qam_mvptr_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __qam_mvptr_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__qam_mvptr%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\topcode: %lu\n", (u_long)argp->opcode);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\told_first: %lu\n", (u_long)argp->old_first);
	(void)printf("\tnew_first: %lu\n", (u_long)argp->new_first);
	(void)printf("\told_cur: %lu\n", (u_long)argp->old_cur);
	(void)printf("\tnew_cur: %lu\n", (u_long)argp->new_cur);
	(void)printf("\tmetalsn: [%lu][%lu]\n",
	    (u_long)argp->metalsn.file, (u_long)argp->metalsn.offset);
	(void)printf("\tmeta_pgno: %lu\n", (u_long)argp->meta_pgno);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __qam_del_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__qam_del_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__qam_del_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __qam_del_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__qam_del%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tindx: %lu\n", (u_long)argp->indx);
	(void)printf("\trecno: %lu\n", (u_long)argp->recno);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __qam_add_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__qam_add_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__qam_add_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __qam_add_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__qam_add%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tindx: %lu\n", (u_long)argp->indx);
	(void)printf("\trecno: %lu\n", (u_long)argp->recno);
	(void)printf("\tdata: ");
	for (i = 0; i < argp->data.size; i++) {
		ch = ((u_int8_t *)argp->data.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tvflag: %lu\n", (u_long)argp->vflag);
	(void)printf("\tolddata: ");
	for (i = 0; i < argp->olddata.size; i++) {
		ch = ((u_int8_t *)argp->olddata.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __qam_delext_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__qam_delext_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__qam_delext_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __qam_delext_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__qam_delext%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tindx: %lu\n", (u_long)argp->indx);
	(void)printf("\trecno: %lu\n", (u_long)argp->recno);
	(void)printf("\tdata: ");
	for (i = 0; i < argp->data.size; i++) {
		ch = ((u_int8_t *)argp->data.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __qam_init_print __P((ENV *, DB_DISTAB *));
 */
int
__qam_init_print(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __qam_incfirst_print, DB___qam_incfirst)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __qam_mvptr_print, DB___qam_mvptr)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __qam_del_print, DB___qam_del)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __qam_add_print, DB___qam_add)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __qam_delext_print, DB___qam_delext)) != 0)
		return (ret);
	return (0);
}
#endif /* HAVE_QUEUE */
