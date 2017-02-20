/* Do not edit: automatically built by gen_rec.awk. */

#include "db_config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "db.h"
#include "ex_apprec.h"
/*
 * PUBLIC: int ex_apprec_mkdir_print __P((DB_ENV *, DBT *, DB_LSN *,
 * PUBLIC:     db_recops));
 */
int
ex_apprec_mkdir_print(dbenv, dbtp, lsnp, notused2)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops notused2;
{
	ex_apprec_mkdir_args *argp;
	int ex_apprec_mkdir_read __P((DB_ENV *, void *, ex_apprec_mkdir_args **));
	u_int32_t i;
	int ch;
	int ret;

	notused2 = DB_TXN_PRINT;

	if ((ret = ex_apprec_mkdir_read(dbenv, dbtp->data, &argp)) != 0)
		return (ret);
	(void)printf(
    "[%lu][%lu]ex_apprec_mkdir%s: rec: %lu txnp %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset,
	    (argp->type & DB_debug_FLAG) ? "_debug" : "",
	    (u_long)argp->type,
	    (u_long)argp->txnp->txnid,
	    (u_long)argp->prev_lsn.file, (u_long)argp->prev_lsn.offset);
	(void)printf("\tdirname: ");
	for (i = 0; i < argp->dirname.size; i++) {
		ch = ((u_int8_t *)argp->dirname.data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	(void)printf("\n");
	(void)printf("\n");
	free(argp);
	return (0);
}

/*
 * PUBLIC: int ex_apprec_init_print __P((DB_ENV *, DB_DISTAB *));
 */
int
ex_apprec_init_print(dbenv, dtabp)
	DB_ENV *dbenv;
	DB_DISTAB *dtabp;
{
	int __db_add_recovery __P((DB_ENV *, DB_DISTAB *,
	    int (*)(DB_ENV *, DBT *, DB_LSN *, db_recops), u_int32_t));
	int ret;

	if ((ret = __db_add_recovery(dbenv, dtabp,
	    ex_apprec_mkdir_print, DB_ex_apprec_mkdir)) != 0)
		return (ret);
	return (0);
}
