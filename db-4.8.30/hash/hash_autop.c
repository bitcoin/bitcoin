/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"

#ifdef HAVE_HASH
#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_dispatch.h"
#include "dbinc/db_am.h"
#include "dbinc/hash.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

/*
 * PUBLIC: int __ham_insdel_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_insdel_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_insdel_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_insdel_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_insdel%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\topcode: %lu\n", (u_long)argp->opcode);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tndx: %lu\n", (u_long)argp->ndx);
	(void)printf("\tpagelsn: [%lu][%lu]\n",
	    (u_long)argp->pagelsn.file, (u_long)argp->pagelsn.offset);
	(void)printf("\tkey: ");
	for (i = 0; i < argp->key.size; i++) {
		ch = ((u_int8_t *)argp->key.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
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
 * PUBLIC: int __ham_newpage_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_newpage_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_newpage_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_newpage_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_newpage%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\topcode: %lu\n", (u_long)argp->opcode);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tprev_pgno: %lu\n", (u_long)argp->prev_pgno);
	(void)printf("\tprevlsn: [%lu][%lu]\n",
	    (u_long)argp->prevlsn.file, (u_long)argp->prevlsn.offset);
	(void)printf("\tnew_pgno: %lu\n", (u_long)argp->new_pgno);
	(void)printf("\tpagelsn: [%lu][%lu]\n",
	    (u_long)argp->pagelsn.file, (u_long)argp->pagelsn.offset);
	(void)printf("\tnext_pgno: %lu\n", (u_long)argp->next_pgno);
	(void)printf("\tnextlsn: [%lu][%lu]\n",
	    (u_long)argp->nextlsn.file, (u_long)argp->nextlsn.offset);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __ham_splitdata_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_splitdata_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_splitdata_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_splitdata_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_splitdata%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\topcode: %lu\n", (u_long)argp->opcode);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tpageimage: ");
	for (i = 0; i < argp->pageimage.size; i++) {
		ch = ((u_int8_t *)argp->pageimage.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tpagelsn: [%lu][%lu]\n",
	    (u_long)argp->pagelsn.file, (u_long)argp->pagelsn.offset);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __ham_replace_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_replace_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_replace_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_replace_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_replace%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tndx: %lu\n", (u_long)argp->ndx);
	(void)printf("\tpagelsn: [%lu][%lu]\n",
	    (u_long)argp->pagelsn.file, (u_long)argp->pagelsn.offset);
	(void)printf("\toff: %ld\n", (long)argp->off);
	(void)printf("\tolditem: ");
	for (i = 0; i < argp->olditem.size; i++) {
		ch = ((u_int8_t *)argp->olditem.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tnewitem: ");
	for (i = 0; i < argp->newitem.size; i++) {
		ch = ((u_int8_t *)argp->newitem.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\tmakedup: %lu\n", (u_long)argp->makedup);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __ham_copypage_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_copypage_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_copypage_args *argp;
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_copypage_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_copypage%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tpagelsn: [%lu][%lu]\n",
	    (u_long)argp->pagelsn.file, (u_long)argp->pagelsn.offset);
	(void)printf("\tnext_pgno: %lu\n", (u_long)argp->next_pgno);
	(void)printf("\tnextlsn: [%lu][%lu]\n",
	    (u_long)argp->nextlsn.file, (u_long)argp->nextlsn.offset);
	(void)printf("\tnnext_pgno: %lu\n", (u_long)argp->nnext_pgno);
	(void)printf("\tnnextlsn: [%lu][%lu]\n",
	    (u_long)argp->nnextlsn.file, (u_long)argp->nnextlsn.offset);
	(void)printf("\tpage: ");
	for (i = 0; i < argp->page.size; i++) {
		ch = ((u_int8_t *)argp->page.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __ham_metagroup_42_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_metagroup_42_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_metagroup_42_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_metagroup_42_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_metagroup_42%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tbucket: %lu\n", (u_long)argp->bucket);
	(void)printf("\tmmpgno: %lu\n", (u_long)argp->mmpgno);
	(void)printf("\tmmetalsn: [%lu][%lu]\n",
	    (u_long)argp->mmetalsn.file, (u_long)argp->mmetalsn.offset);
	(void)printf("\tmpgno: %lu\n", (u_long)argp->mpgno);
	(void)printf("\tmetalsn: [%lu][%lu]\n",
	    (u_long)argp->metalsn.file, (u_long)argp->metalsn.offset);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tpagelsn: [%lu][%lu]\n",
	    (u_long)argp->pagelsn.file, (u_long)argp->pagelsn.offset);
	(void)printf("\tnewalloc: %lu\n", (u_long)argp->newalloc);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __ham_metagroup_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_metagroup_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_metagroup_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_metagroup_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_metagroup%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tbucket: %lu\n", (u_long)argp->bucket);
	(void)printf("\tmmpgno: %lu\n", (u_long)argp->mmpgno);
	(void)printf("\tmmetalsn: [%lu][%lu]\n",
	    (u_long)argp->mmetalsn.file, (u_long)argp->mmetalsn.offset);
	(void)printf("\tmpgno: %lu\n", (u_long)argp->mpgno);
	(void)printf("\tmetalsn: [%lu][%lu]\n",
	    (u_long)argp->metalsn.file, (u_long)argp->metalsn.offset);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tpagelsn: [%lu][%lu]\n",
	    (u_long)argp->pagelsn.file, (u_long)argp->pagelsn.offset);
	(void)printf("\tnewalloc: %lu\n", (u_long)argp->newalloc);
	(void)printf("\tlast_pgno: %lu\n", (u_long)argp->last_pgno);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __ham_groupalloc_42_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_groupalloc_42_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_groupalloc_42_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_groupalloc_42_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_groupalloc_42%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tmeta_lsn: [%lu][%lu]\n",
	    (u_long)argp->meta_lsn.file, (u_long)argp->meta_lsn.offset);
	(void)printf("\tstart_pgno: %lu\n", (u_long)argp->start_pgno);
	(void)printf("\tnum: %lu\n", (u_long)argp->num);
	(void)printf("\tfree: %lu\n", (u_long)argp->free);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __ham_groupalloc_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_groupalloc_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_groupalloc_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_groupalloc_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_groupalloc%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tmeta_lsn: [%lu][%lu]\n",
	    (u_long)argp->meta_lsn.file, (u_long)argp->meta_lsn.offset);
	(void)printf("\tstart_pgno: %lu\n", (u_long)argp->start_pgno);
	(void)printf("\tnum: %lu\n", (u_long)argp->num);
	(void)printf("\tunused: %lu\n", (u_long)argp->unused);
	(void)printf("\tlast_pgno: %lu\n", (u_long)argp->last_pgno);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __ham_curadj_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_curadj_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_curadj_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_curadj_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_curadj%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tpgno: %lu\n", (u_long)argp->pgno);
	(void)printf("\tindx: %lu\n", (u_long)argp->indx);
	(void)printf("\tlen: %lu\n", (u_long)argp->len);
	(void)printf("\tdup_off: %lu\n", (u_long)argp->dup_off);
	(void)printf("\tadd: %ld\n", (long)argp->add);
	(void)printf("\tis_dup: %ld\n", (long)argp->is_dup);
	(void)printf("\torder: %lu\n", (u_long)argp->order);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __ham_chgpg_print __P((ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops, void *));
 */
int
__ham_chgpg_print(env, dbtp, lsnp, notused2, notused3)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
	void *notused3;
{
	__ham_chgpg_args *argp;
	int ret;

	notused2 = DB_TXN_PRINT;
	notused3 = NULL;

	if ((ret =
	    __ham_chgpg_read(env, NULL, NULL, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]__ham_chgpg%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tfileid: %ld\n", (long)argp->fileid);
	(void)printf("\tmode: %ld\n", (long)argp->mode);
	(void)printf("\told_pgno: %lu\n", (u_long)argp->old_pgno);
	(void)printf("\tnew_pgno: %lu\n", (u_long)argp->new_pgno);
	(void)printf("\told_indx: %lu\n", (u_long)argp->old_indx);
	(void)printf("\tnew_indx: %lu\n", (u_long)argp->new_indx);
	(void)printf("\n");
	__os_free(env, argp);
	return (0);
}

/*
 * PUBLIC: int __ham_init_print __P((ENV *, DB_DISTAB *));
 */
int
__ham_init_print(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_insdel_print, DB___ham_insdel)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_newpage_print, DB___ham_newpage)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_splitdata_print, DB___ham_splitdata)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_replace_print, DB___ham_replace)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_copypage_print, DB___ham_copypage)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_metagroup_print, DB___ham_metagroup)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_groupalloc_print, DB___ham_groupalloc)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_curadj_print, DB___ham_curadj)) != 0)
		return (ret);
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_chgpg_print, DB___ham_chgpg)) != 0)
		return (ret);
	return (0);
}
#endif /* HAVE_HASH */
