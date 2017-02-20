/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <tcl.h>
#endif
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/tcl_db.h"

/*
 * Prototypes for procedures defined later in this file:
 */
static int	tcl_DbAssociate __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB *));
static int	tcl_DbClose __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB *, DBTCL_INFO *));
static int	tcl_DbDelete __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB *));
static int	tcl_DbGet __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB *, int));
#ifdef CONFIG_TEST
static int	tcl_DbKeyRange __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB *));
#endif
static int	tcl_DbPut __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB *));
static int	tcl_DbStat __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB *));
static int	tcl_DbTruncate __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB *));
#ifdef CONFIG_TEST
static int	tcl_DbCompact __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB *));
static int	tcl_DbCompactStat __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB *));
#endif
static int	tcl_DbCursor __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB *, DBC **));
static int	tcl_DbJoin __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB *, DBC **));
static int	tcl_DbGetFlags __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB *));
static int	tcl_DbGetOpenFlags __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB *));
static int	tcl_DbGetjoin __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB *));
static int	tcl_DbCount __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB *));
static int	tcl_second_call __P((DB *, const DBT *, const DBT *, DBT *));

/*
 * _DbInfoDelete --
 *
 * PUBLIC: void _DbInfoDelete __P((Tcl_Interp *, DBTCL_INFO *));
 */
void
_DbInfoDelete(interp, dbip)
	Tcl_Interp *interp;
	DBTCL_INFO *dbip;
{
	DBTCL_INFO *nextp, *p;
	/*
	 * First we have to close any open cursors.  Then we close
	 * our db.
	 */
	for (p = LIST_FIRST(&__db_infohead); p != NULL; p = nextp) {
		nextp = LIST_NEXT(p, entries);
		/*
		 * Check if this is a cursor info structure and if
		 * it is, if it belongs to this DB.  If so, remove
		 * its commands and info structure.
		 */
		if (p->i_parent == dbip && p->i_type == I_DBC) {
			(void)Tcl_DeleteCommand(interp, p->i_name);
			_DeleteInfo(p);
		}
	}
	(void)Tcl_DeleteCommand(interp, dbip->i_name);
	_DeleteInfo(dbip);
}

/*
 *
 * PUBLIC: int db_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
 *
 * db_Cmd --
 *	Implements the "db" widget.
 */
int
db_Cmd(clientData, interp, objc, objv)
	ClientData clientData;		/* DB handle */
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *dbcmds[] = {
#ifdef CONFIG_TEST
		"keyrange",
		"pget",
		"rpcid",
		"test",
		"compact",
		"compact_stat",
#endif
		"associate",
		"close",
		"count",
		"cursor",
		"del",
		"get",
		"get_bt_minkey",
		"get_cachesize",
		"get_dbname",
		"get_encrypt_flags",
		"get_env",
		"get_errpfx",
		"get_flags",
		"get_h_ffactor",
		"get_h_nelem",
		"get_join",
		"get_lorder",
		"get_open_flags",
		"get_pagesize",
		"get_q_extentsize",
		"get_re_delim",
		"get_re_len",
		"get_re_pad",
		"get_re_source",
		"get_type",
		"is_byteswapped",
		"join",
		"put",
		"stat",
		"sync",
		"truncate",
		NULL
	};
	enum dbcmds {
#ifdef CONFIG_TEST
		DBKEYRANGE,
		DBPGET,
		DBRPCID,
		DBTEST,
		DBCOMPACT,
		DBCOMPACT_STAT,
#endif
		DBASSOCIATE,
		DBCLOSE,
		DBCOUNT,
		DBCURSOR,
		DBDELETE,
		DBGET,
		DBGETBTMINKEY,
		DBGETCACHESIZE,
		DBGETDBNAME,
		DBGETENCRYPTFLAGS,
		DBGETENV,
		DBGETERRPFX,
		DBGETFLAGS,
		DBGETHFFACTOR,
		DBGETHNELEM,
		DBGETJOIN,
		DBGETLORDER,
		DBGETOPENFLAGS,
		DBGETPAGESIZE,
		DBGETQEXTENTSIZE,
		DBGETREDELIM,
		DBGETRELEN,
		DBGETREPAD,
		DBGETRESOURCE,
		DBGETTYPE,
		DBSWAPPED,
		DBJOIN,
		DBPUT,
		DBSTAT,
		DBSYNC,
		DBTRUNCATE
	};
	DB *dbp;
	DB_ENV *dbenv;
	DBC *dbc;
	DBTCL_INFO *dbip, *ip;
	DBTYPE type;
	Tcl_Obj *res, *myobjv[3];
	int cmdindex, intval, ncache, result, ret;
	char newname[MSG_SIZE];
	u_int32_t bytes, gbytes, value;
	const char *strval, *filename, *dbname, *envid;

	Tcl_ResetResult(interp);
	dbp = (DB *)clientData;
	dbip = _PtrToInfo((void *)dbp);
	memset(newname, 0, MSG_SIZE);
	result = TCL_OK;
	if (objc <= 1) {
		Tcl_WrongNumArgs(interp, 1, objv, "command cmdargs");
		return (TCL_ERROR);
	}
	if (dbp == NULL) {
		Tcl_SetResult(interp, "NULL db pointer", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (dbip == NULL) {
		Tcl_SetResult(interp, "NULL db info pointer", TCL_STATIC);
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the dbcmds
	 * defined above.
	 */
	if (Tcl_GetIndexFromObj(interp,
	    objv[1], dbcmds, "command", TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));

	res = NULL;
	switch ((enum dbcmds)cmdindex) {
#ifdef CONFIG_TEST
	case DBKEYRANGE:
		result = tcl_DbKeyRange(interp, objc, objv, dbp);
		break;
	case DBPGET:
		result = tcl_DbGet(interp, objc, objv, dbp, 1);
		break;
	case DBRPCID:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		/*
		 * !!! Retrieve the client ID from the dbp handle directly.
		 * This is for testing purposes only.  It is dbp-private data.
		 */
		res = Tcl_NewLongObj((long)dbp->cl_id);
		break;
	case DBTEST:
		result = tcl_EnvTest(interp, objc, objv, dbp->dbenv);
		break;

	case DBCOMPACT:
		result = tcl_DbCompact(interp, objc, objv, dbp);
		break;

	case DBCOMPACT_STAT:
		result = tcl_DbCompactStat(interp, objc, objv, dbp);
		break;

#endif
	case DBASSOCIATE:
		result = tcl_DbAssociate(interp, objc, objv, dbp);
		break;
	case DBCLOSE:
		result = tcl_DbClose(interp, objc, objv, dbp, dbip);
		break;
	case DBDELETE:
		result = tcl_DbDelete(interp, objc, objv, dbp);
		break;
	case DBGET:
		result = tcl_DbGet(interp, objc, objv, dbp, 0);
		break;
	case DBPUT:
		result = tcl_DbPut(interp, objc, objv, dbp);
		break;
	case DBCOUNT:
		result = tcl_DbCount(interp, objc, objv, dbp);
		break;
	case DBSWAPPED:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbp->get_byteswapped(dbp, &intval);
		res = Tcl_NewIntObj(intval);
		break;
	case DBGETTYPE:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbp->get_type(dbp, &type);
		if (type == DB_BTREE)
			res = NewStringObj("btree", strlen("btree"));
		else if (type == DB_HASH)
			res = NewStringObj("hash", strlen("hash"));
		else if (type == DB_RECNO)
			res = NewStringObj("recno", strlen("recno"));
		else if (type == DB_QUEUE)
			res = NewStringObj("queue", strlen("queue"));
		else {
			Tcl_SetResult(interp,
			    "db gettype: Returned unknown type\n", TCL_STATIC);
			result = TCL_ERROR;
		}
		break;
	case DBSTAT:
		result = tcl_DbStat(interp, objc, objv, dbp);
		break;
	case DBSYNC:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbp->sync(dbp, 0);
		res = Tcl_NewIntObj(ret);
		if (ret != 0) {
			Tcl_SetObjResult(interp, res);
			result = TCL_ERROR;
		}
		break;
	case DBCURSOR:
		snprintf(newname, sizeof(newname),
		    "%s.c%d", dbip->i_name, dbip->i_dbdbcid);
		ip = _NewInfo(interp, NULL, newname, I_DBC);
		if (ip != NULL) {
			result = tcl_DbCursor(interp, objc, objv, dbp, &dbc);
			if (result == TCL_OK) {
				dbip->i_dbdbcid++;
				ip->i_parent = dbip;
				(void)Tcl_CreateObjCommand(interp, newname,
				    (Tcl_ObjCmdProc *)dbc_Cmd,
				    (ClientData)dbc, NULL);
				res = NewStringObj(newname, strlen(newname));
				_SetInfoData(ip, dbc);
			} else
				_DeleteInfo(ip);
		} else {
			Tcl_SetResult(interp,
			    "Could not set up info", TCL_STATIC);
			result = TCL_ERROR;
		}
		break;
	case DBJOIN:
		snprintf(newname, sizeof(newname),
		    "%s.c%d", dbip->i_name, dbip->i_dbdbcid);
		ip = _NewInfo(interp, NULL, newname, I_DBC);
		if (ip != NULL) {
			result = tcl_DbJoin(interp, objc, objv, dbp, &dbc);
			if (result == TCL_OK) {
				dbip->i_dbdbcid++;
				ip->i_parent = dbip;
				(void)Tcl_CreateObjCommand(interp, newname,
				    (Tcl_ObjCmdProc *)dbc_Cmd,
				    (ClientData)dbc, NULL);
				res = NewStringObj(newname, strlen(newname));
				_SetInfoData(ip, dbc);
			} else
				_DeleteInfo(ip);
		} else {
			Tcl_SetResult(interp,
			    "Could not set up info", TCL_STATIC);
			result = TCL_ERROR;
		}
		break;
	case DBGETBTMINKEY:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_bt_minkey(dbp, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_bt_minkey")) == TCL_OK)
			res = Tcl_NewIntObj((int)value);
		break;
	case DBGETCACHESIZE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_cachesize(dbp, &gbytes, &bytes, &ncache);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_cachesize")) == TCL_OK) {
			myobjv[0] = Tcl_NewIntObj((int)gbytes);
			myobjv[1] = Tcl_NewIntObj((int)bytes);
			myobjv[2] = Tcl_NewIntObj((int)ncache);
			res = Tcl_NewListObj(3, myobjv);
		}
		break;
	case DBGETDBNAME:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_dbname(dbp, &filename, &dbname);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_dbname")) == TCL_OK) {
			myobjv[0] = NewStringObj(filename, strlen(filename));
			myobjv[1] = NewStringObj(dbname, strlen(dbname));
			res = Tcl_NewListObj(2, myobjv);
		}
		break;
	case DBGETENCRYPTFLAGS:
		result = tcl_EnvGetEncryptFlags(interp, objc, objv, dbp->dbenv);
		break;
	case DBGETENV:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		dbenv = dbp->get_env(dbp);
		if (dbenv != NULL && (ip = _PtrToInfo(dbenv)) != NULL) {
			envid = ip->i_name;
			res = NewStringObj(envid, strlen(envid));
		} else
			Tcl_ResetResult(interp);
		break;
	case DBGETERRPFX:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		dbp->get_errpfx(dbp, &strval);
		res = NewStringObj(strval, strlen(strval));
		break;
	case DBGETFLAGS:
		result = tcl_DbGetFlags(interp, objc, objv, dbp);
		break;
	case DBGETHFFACTOR:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_h_ffactor(dbp, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_h_ffactor")) == TCL_OK)
			res = Tcl_NewIntObj((int)value);
		break;
	case DBGETHNELEM:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_h_nelem(dbp, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_h_nelem")) == TCL_OK)
			res = Tcl_NewIntObj((int)value);
		break;
	case DBGETJOIN:
		result = tcl_DbGetjoin(interp, objc, objv, dbp);
		break;
	case DBGETLORDER:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbp->get_lorder(dbp, &intval);
		res = Tcl_NewIntObj(intval);
		break;
	case DBGETOPENFLAGS:
		result = tcl_DbGetOpenFlags(interp, objc, objv, dbp);
		break;
	case DBGETPAGESIZE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_pagesize(dbp, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_pagesize")) == TCL_OK)
			res = Tcl_NewIntObj((int)value);
		break;
	case DBGETQEXTENTSIZE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_q_extentsize(dbp, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_q_extentsize")) == TCL_OK)
			res = Tcl_NewIntObj((int)value);
		break;
	case DBGETREDELIM:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_re_delim(dbp, &intval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_re_delim")) == TCL_OK)
			res = Tcl_NewIntObj(intval);
		break;
	case DBGETRELEN:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_re_len(dbp, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_re_len")) == TCL_OK)
			res = Tcl_NewIntObj((int)value);
		break;
	case DBGETREPAD:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_re_pad(dbp, &intval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_re_pad")) == TCL_OK)
			res = Tcl_NewIntObj((int)intval);
		break;
	case DBGETRESOURCE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbp->get_re_source(dbp, &strval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db get_re_source")) == TCL_OK)
			res = NewStringObj(strval, strlen(strval));
		break;
	case DBTRUNCATE:
		result = tcl_DbTruncate(interp, objc, objv, dbp);
		break;
	}
	/*
	 * Only set result if we have a res.  Otherwise, lower
	 * functions have already done so.
	 */
	if (result == TCL_OK && res)
		Tcl_SetObjResult(interp, res);
	return (result);
}

/*
 * tcl_db_stat --
 */
static int
tcl_DbStat(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	static const char *dbstatopts[] = {
#ifdef CONFIG_TEST
		"-read_committed",
		"-read_uncommitted",
#endif
		"-faststat",
		"-txn",
		NULL
	};
	enum dbstatopts {
#ifdef CONFIG_TEST
		DBCUR_READ_COMMITTED,
		DBCUR_READ_UNCOMMITTED,
#endif
		DBCUR_FASTSTAT,
		DBCUR_TXN
	};
	DBTYPE type;
	DB_BTREE_STAT *bsp;
	DB_HASH_STAT *hsp;
	DB_QUEUE_STAT *qsp;
	DB_TXN *txn;
	Tcl_Obj *res, *flaglist, *myobjv[2];
	u_int32_t flag;
	int i, optindex, result, ret;
	char *arg, msg[MSG_SIZE];
	void *sp;

	result = TCL_OK;
	flag = 0;
	txn = NULL;
	sp = NULL;
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbstatopts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			goto error;
		}
		i++;
		switch ((enum dbstatopts)optindex) {
#ifdef CONFIG_TEST
		case DBCUR_READ_COMMITTED:
			flag |= DB_READ_COMMITTED;
			break;
		case DBCUR_READ_UNCOMMITTED:
			flag |= DB_READ_UNCOMMITTED;
			break;
#endif
		case DBCUR_FASTSTAT:
			flag |= DB_FAST_STAT;
			break;
		case DBCUR_TXN:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "Stat: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		}
		if (result != TCL_OK)
			break;
	}
	if (result != TCL_OK)
		goto error;

	_debug_check();
	ret = dbp->stat(dbp, txn, &sp, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "db stat");
	if (result == TCL_ERROR)
		return (result);

	(void)dbp->get_type(dbp, &type);
	/*
	 * Have our stats, now construct the name value
	 * list pairs and free up the memory.
	 */
	res = Tcl_NewObj();

	/*
	 * MAKE_STAT_LIST assumes 'res' and 'error' label.
	 */
	if (type == DB_HASH) {
		hsp = (DB_HASH_STAT *)sp;
		MAKE_STAT_LIST("Magic", hsp->hash_magic);
		MAKE_STAT_LIST("Version", hsp->hash_version);
		MAKE_STAT_LIST("Page size", hsp->hash_pagesize);
		MAKE_STAT_LIST("Page count", hsp->hash_pagecnt);
		MAKE_STAT_LIST("Number of keys", hsp->hash_nkeys);
		MAKE_STAT_LIST("Number of records", hsp->hash_ndata);
		MAKE_STAT_LIST("Fill factor", hsp->hash_ffactor);
		MAKE_STAT_LIST("Buckets", hsp->hash_buckets);
		if (flag != DB_FAST_STAT) {
			MAKE_STAT_LIST("Free pages", hsp->hash_free);
			MAKE_WSTAT_LIST("Bytes free", hsp->hash_bfree);
			MAKE_STAT_LIST("Number of big pages",
			    hsp->hash_bigpages);
			MAKE_STAT_LIST("Big pages bytes free",
			    hsp->hash_big_bfree);
			MAKE_STAT_LIST("Overflow pages", hsp->hash_overflows);
			MAKE_STAT_LIST("Overflow bytes free",
			    hsp->hash_ovfl_free);
			MAKE_STAT_LIST("Duplicate pages", hsp->hash_dup);
			MAKE_STAT_LIST("Duplicate pages bytes free",
			    hsp->hash_dup_free);
		}
	} else if (type == DB_QUEUE) {
		qsp = (DB_QUEUE_STAT *)sp;
		MAKE_STAT_LIST("Magic", qsp->qs_magic);
		MAKE_STAT_LIST("Version", qsp->qs_version);
		MAKE_STAT_LIST("Page size", qsp->qs_pagesize);
		MAKE_STAT_LIST("Extent size", qsp->qs_extentsize);
		MAKE_STAT_LIST("Number of keys", qsp->qs_nkeys);
		MAKE_STAT_LIST("Number of records", qsp->qs_ndata);
		MAKE_STAT_LIST("Record length", qsp->qs_re_len);
		MAKE_STAT_LIST("Record pad", qsp->qs_re_pad);
		MAKE_STAT_LIST("First record number", qsp->qs_first_recno);
		MAKE_STAT_LIST("Last record number", qsp->qs_cur_recno);
		if (flag != DB_FAST_STAT) {
			MAKE_STAT_LIST("Number of pages", qsp->qs_pages);
			MAKE_WSTAT_LIST("Bytes free", qsp->qs_pgfree);
		}
	} else {	/* BTREE and RECNO are same stats */
		bsp = (DB_BTREE_STAT *)sp;
		MAKE_STAT_LIST("Magic", bsp->bt_magic);
		MAKE_STAT_LIST("Version", bsp->bt_version);
		MAKE_STAT_LIST("Number of keys", bsp->bt_nkeys);
		MAKE_STAT_LIST("Number of records", bsp->bt_ndata);
		MAKE_STAT_LIST("Minimum keys per page", bsp->bt_minkey);
		MAKE_STAT_LIST("Fixed record length", bsp->bt_re_len);
		MAKE_STAT_LIST("Record pad", bsp->bt_re_pad);
		MAKE_STAT_LIST("Page size", bsp->bt_pagesize);
		MAKE_STAT_LIST("Page count", bsp->bt_pagecnt);
		if (flag != DB_FAST_STAT) {
			MAKE_STAT_LIST("Levels", bsp->bt_levels);
			MAKE_STAT_LIST("Internal pages", bsp->bt_int_pg);
			MAKE_STAT_LIST("Leaf pages", bsp->bt_leaf_pg);
			MAKE_STAT_LIST("Duplicate pages", bsp->bt_dup_pg);
			MAKE_STAT_LIST("Overflow pages", bsp->bt_over_pg);
			MAKE_STAT_LIST("Empty pages", bsp->bt_empty_pg);
			MAKE_STAT_LIST("Pages on freelist", bsp->bt_free);
			MAKE_STAT_LIST("Internal pages bytes free",
			    bsp->bt_int_pgfree);
			MAKE_STAT_LIST("Leaf pages bytes free",
			    bsp->bt_leaf_pgfree);
			MAKE_STAT_LIST("Duplicate pages bytes free",
			    bsp->bt_dup_pgfree);
			MAKE_STAT_LIST("Bytes free in overflow pages",
			    bsp->bt_over_pgfree);
		}
	}

	/*
	 * Construct a {name {flag1 flag2 ... flagN}} list for the
	 * dbp flags.  These aren't access-method dependent, but they
	 * include all the interesting flags, and the integer value
	 * isn't useful from Tcl--return the strings instead.
	 */
	myobjv[0] = NewStringObj("Flags", strlen("Flags"));
	myobjv[1] = _GetFlagsList(interp, dbp->flags, __db_get_flags_fn());
	flaglist = Tcl_NewListObj(2, myobjv);
	if (flaglist == NULL) {
		result = TCL_ERROR;
		goto error;
	}
	if ((result =
	    Tcl_ListObjAppendElement(interp, res, flaglist)) != TCL_OK)
		goto error;

	Tcl_SetObjResult(interp, res);
error:
	if (sp != NULL)
		__os_ufree(dbp->env, sp);
	return (result);
}

/*
 * tcl_db_close --
 */
static int
tcl_DbClose(interp, objc, objv, dbp, dbip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
	DBTCL_INFO *dbip;		/* Info pointer */
{
	static const char *dbclose[] = {
		"-nosync", "--", NULL
	};
	enum dbclose {
		TCL_DBCLOSE_NOSYNC,
		TCL_DBCLOSE_ENDARG
	};
	u_int32_t flag;
	int endarg, i, optindex, result, ret;
	char *arg;

	result = TCL_OK;
	endarg = 0;
	flag = 0;
	if (objc > 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-nosync?");
		return (TCL_ERROR);
	}

	for (i = 2; i < objc; ++i) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbclose,
		    "option", TCL_EXACT, &optindex) != TCL_OK) {
			arg = Tcl_GetStringFromObj(objv[i], NULL);
			if (arg[0] == '-')
				return (IS_HELP(objv[i]));
			else
				Tcl_ResetResult(interp);
			break;
		}
		switch ((enum dbclose)optindex) {
		case TCL_DBCLOSE_NOSYNC:
			flag = DB_NOSYNC;
			break;
		case TCL_DBCLOSE_ENDARG:
			endarg = 1;
			break;
		}
		/*
		 * If, at any time, parsing the args we get an error,
		 * bail out and return.
		 */
		if (result != TCL_OK)
			return (result);
		if (endarg)
			break;
	}
	if (dbip->i_cdata != NULL)
		__os_free(dbp->env, dbip->i_cdata);
	_DbInfoDelete(interp, dbip);
	_debug_check();

	/* Paranoia. */
	dbp->api_internal = NULL;

	ret = (dbp)->close(dbp, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "db close");
	return (result);
}

/*
 * tcl_db_put --
 */
static int
tcl_DbPut(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	static const char *dbputopts[] = {
#ifdef CONFIG_TEST
		"-nodupdata",
#endif
		"-append",
		"-multiple",
		"-multiple_key",
		"-nooverwrite",
		"-overwritedup",
		"-partial",
		"-txn",
		NULL
	};
	enum dbputopts {
#ifdef CONFIG_TEST
		DBGET_NODUPDATA,
#endif
		DBPUT_APPEND,
		DBPUT_MULTIPLE,
		DBPUT_MULTIPLE_KEY,
		DBPUT_NOOVER,
		DBPUT_OVER,
		DBPUT_PART,
		DBPUT_TXN
	};
	static const char *dbputapp[] = {
		"-append",
		"-multiple_key",
		NULL
	};
	enum dbputapp { DBPUT_APPEND0, DBPUT_MULTIPLE_KEY0 };
	DBT key, data;
	DBTYPE type;
	DB_TXN *txn;
	Tcl_Obj **delemv, **elemv, *res;
	void *dtmp, *ktmp, *ptr;
	db_recno_t recno;
	u_int32_t flag, multiflag;
	int delemc, elemc, end, freekey, freedata;
	int dlen, klen, i, optindex, result, ret;
	char *arg, msg[MSG_SIZE];

	txn = NULL;
	result = TCL_OK;
	flag = multiflag = 0;
	if (objc <= 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-args? key data");
		return (TCL_ERROR);
	}

	dtmp = ktmp = NULL;
	freekey = freedata = 0;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	COMPQUIET(recno, 0);

	/*
	 * If it is a QUEUE or RECNO database, the key is a record number
	 * and must be setup up to contain a db_recno_t.  Otherwise the
	 * key is a "string".
	 */
	(void)dbp->get_type(dbp, &type);

	/*
	 * We need to determine where the end of required args are.  If we are
	 * using a QUEUE/RECNO db and -append, or -multiple_key is specified,
	 * then there is just one req arg (data).  Otherwise there are two
	 * (key data).
	 *
	 * We preparse the list to determine this since we need to know
	 * to properly check # of args for other options below.
	 */
	end = objc - 2;
	i = 2;
	while (i < objc - 1) {
		if (Tcl_GetIndexFromObj(interp, objv[i++], dbputapp,
		    "option", TCL_EXACT, &optindex) != TCL_OK)
			continue;
		switch ((enum dbputapp)optindex) {
		case DBPUT_APPEND0:
		case DBPUT_MULTIPLE_KEY0:
			end = objc - 1;
			break;
		}
	}
	Tcl_ResetResult(interp);

	/*
	 * Get the command name index from the object based on the options
	 * defined above.
	 */
	i = 2;
	while (i < end) {
		if (Tcl_GetIndexFromObj(interp, objv[i],
		    dbputopts, "option", TCL_EXACT, &optindex) != TCL_OK)
			return (IS_HELP(objv[i]));
		i++;
		switch ((enum dbputopts)optindex) {
#ifdef CONFIG_TEST
		case DBGET_NODUPDATA:
			FLAG_CHECK(flag);
			flag = DB_NODUPDATA;
			break;
#endif
		case DBPUT_TXN:
			if (i > (end - 1)) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "Put: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		case DBPUT_APPEND:
			FLAG_CHECK(flag);
			flag = DB_APPEND;
			break;
		case DBPUT_MULTIPLE:
			FLAG_CHECK(multiflag);
			multiflag = DB_MULTIPLE;
			break;
		case DBPUT_MULTIPLE_KEY:
			FLAG_CHECK(multiflag);
			multiflag = DB_MULTIPLE_KEY;
			break;
		case DBPUT_NOOVER:
			FLAG_CHECK(flag);
			flag = DB_NOOVERWRITE;
			break;
		case DBPUT_OVER:
			FLAG_CHECK(flag);
			flag = DB_OVERWRITE_DUP;
			break;
		case DBPUT_PART:
			if (i > (end - 1)) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-partial {offset length}?");
				result = TCL_ERROR;
				break;
			}
			/*
			 * Get sublist as {offset length}
			 */
			result = Tcl_ListObjGetElements(interp, objv[i++],
			    &elemc, &elemv);
			if (elemc != 2) {
				Tcl_SetResult(interp,
				    "List must be {offset length}", TCL_STATIC);
				result = TCL_ERROR;
				break;
			}
			data.flags = DB_DBT_PARTIAL;
			result = _GetUInt32(interp, elemv[0], &data.doff);
			if (result != TCL_OK)
				break;
			result = _GetUInt32(interp, elemv[1], &data.dlen);
			/*
			 * NOTE: We don't check result here because all we'd
			 * do is break anyway, and we are doing that.  If you
			 * add code here, you WILL need to add the check
			 * for result.  (See the check for save.doff, a few
			 * lines above and copy that.)
			 */
			break;
		}
		if (result != TCL_OK)
			break;
	}

	if (result == TCL_ERROR)
		return (result);

	if (multiflag == DB_MULTIPLE) {
		/*
		 * To work out how big a buffer is needed, we first need to
		 * find out the total length of the data and the number of data
		 * items (elemc).
		 */
		ktmp = Tcl_GetByteArrayFromObj(objv[objc - 2], &klen);
		result = Tcl_ListObjGetElements(interp, objv[objc - 2],
		    &elemc, &elemv);
		if (result != TCL_OK)
			return (result);

		dtmp = Tcl_GetByteArrayFromObj(objv[objc - 1], &dlen);
		result = Tcl_ListObjGetElements(interp, objv[objc - 1],
		    &delemc, &delemv);
		if (result != TCL_OK)
			return (result);

		if (elemc < delemc)
			delemc = elemc;
		else
			elemc = delemc;

		memset(&key, 0, sizeof(key));
		key.ulen = DB_ALIGN((u_int32_t)klen +
		    (u_int32_t)elemc * sizeof(u_int32_t) * 2, 1024UL);
		key.flags = DB_DBT_USERMEM | DB_DBT_BULK;
		if ((ret = __os_malloc(dbp->env, key.ulen, &key.data)) != 0)
			return (ret);
		freekey = 1;

		memset(&data, 0, sizeof(data));
		data.ulen = DB_ALIGN((u_int32_t)dlen +
		    (u_int32_t)delemc * sizeof(u_int32_t) * 2, 1024UL);
		data.flags = DB_DBT_USERMEM | DB_DBT_BULK;
		if ((ret = __os_malloc(dbp->env, data.ulen, &data.data)) != 0)
			return (ret);
		freedata = 1;

		if (type == DB_QUEUE || type == DB_RECNO) {
			DB_MULTIPLE_RECNO_WRITE_INIT(ptr, &key);
			for (i = 0; i < elemc; i++) {
				result = _GetUInt32(interp, elemv[i], &recno);
				DB_MULTIPLE_RECNO_WRITE_NEXT(ptr, &key, recno,
				    dtmp, 0);
				DB_ASSERT(dbp->env, ptr != NULL);
			}
		} else {
			DB_MULTIPLE_WRITE_INIT(ptr, &key);
			for (i = 0; i < elemc; i++) {
				ktmp = Tcl_GetByteArrayFromObj(elemv[i], &klen);
				DB_MULTIPLE_WRITE_NEXT(ptr,
				    &key, ktmp, (u_int32_t)klen);
				DB_ASSERT(dbp->env, ptr != NULL);
			}
		}
		DB_MULTIPLE_WRITE_INIT(ptr, &data);
		for (i = 0; i < elemc; i++) {
			dtmp = Tcl_GetByteArrayFromObj(delemv[i], &dlen);
			DB_MULTIPLE_WRITE_NEXT(ptr,
			    &data, dtmp, (u_int32_t)dlen);
			DB_ASSERT(dbp->env, ptr != NULL);
		}
	} else if (multiflag == DB_MULTIPLE_KEY) {
		/*
		 * To work out how big a buffer is needed, we first need to
		 * find out the total length of the data (len) and the number
		 * of data items (elemc).
		 */
		ktmp = Tcl_GetByteArrayFromObj(objv[objc - 1], &klen);
		result = Tcl_ListObjGetElements(interp, objv[objc - 1],
		    &elemc, &elemv);
		if (result != TCL_OK)
			return (result);

		memset(&key, 0, sizeof(key));
		key.ulen = DB_ALIGN((u_int32_t)klen +
		    (u_int32_t)elemc * sizeof(u_int32_t) * 2, 1024UL);
		key.flags = DB_DBT_USERMEM | DB_DBT_BULK;
		if ((ret = __os_malloc(dbp->env, key.ulen, &key.data)) != 0)
			return (ret);
		freekey = 1;

		if (type == DB_QUEUE || type == DB_RECNO) {
			DB_MULTIPLE_RECNO_WRITE_INIT(ptr, &key);
			for (i = 0; i + 1 < elemc; i += 2) {
				result = _GetUInt32(interp, elemv[i], &recno);
				dtmp = Tcl_GetByteArrayFromObj(elemv[i + 1],
				    &dlen);
				DB_MULTIPLE_RECNO_WRITE_NEXT(ptr, &key,
				    recno, dtmp, (u_int32_t)dlen);
				DB_ASSERT(dbp->env, ptr != NULL);
			}
		} else {
			DB_MULTIPLE_WRITE_INIT(ptr, &key);
			for (i = 0; i + 1 < elemc; i += 2) {
				ktmp = Tcl_GetByteArrayFromObj(elemv[i], &klen);
				dtmp = Tcl_GetByteArrayFromObj(elemv[i + 1],
				    &dlen);
				DB_MULTIPLE_KEY_WRITE_NEXT(ptr,
				    &key, ktmp, (u_int32_t)klen,
				    dtmp, (u_int32_t)dlen);
				DB_ASSERT(dbp->env, ptr != NULL);
			}
		}
	} else if (type == DB_QUEUE || type == DB_RECNO) {
		/*
		 * If we are a recno db and we are NOT using append, then the
		 * 2nd last arg is the key.
		 */
		key.data = &recno;
		key.ulen = key.size = sizeof(db_recno_t);
		key.flags = DB_DBT_USERMEM;
		if (flag == DB_APPEND)
			recno = 0;
		else {
			result = _GetUInt32(interp, objv[objc-2], &recno);
			if (result != TCL_OK)
				return (result);
		}
	} else {
		ret = _CopyObjBytes(interp, objv[objc-2], &ktmp,
		    &key.size, &freekey);
		if (ret != 0) {
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_DBPUT(ret), "db put");
			return (result);
		}
		key.data = ktmp;
	}

	if (multiflag == 0) {
		ret = _CopyObjBytes(interp,
		    objv[objc-1], &dtmp, &data.size, &freedata);
		if (ret != 0) {
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_DBPUT(ret), "db put");
			goto out;
		}
		data.data = dtmp;
	}
	_debug_check();
	ret = dbp->put(dbp, txn, &key, &data, flag | multiflag);
	result = _ReturnSetup(interp, ret, DB_RETOK_DBPUT(ret), "db put");

	/* We may have a returned record number. */
	if (ret == 0 &&
	    (type == DB_QUEUE || type == DB_RECNO) && flag == DB_APPEND) {
		res = Tcl_NewWideIntObj((Tcl_WideInt)recno);
		Tcl_SetObjResult(interp, res);
	}

out:	if (freedata && data.data != NULL)
		__os_free(dbp->env, data.data);
	if (freekey && key.data != NULL)
		__os_free(dbp->env, key.data);
	return (result);
}

/*
 * tcl_db_get --
 */
static int
tcl_DbGet(interp, objc, objv, dbp, ispget)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
	int ispget;			/* 1 for pget, 0 for get */
{
	static const char *dbgetopts[] = {
#ifdef CONFIG_TEST
		"-data_buf_size",
		"-multi",
		"-nolease",
		"-read_committed",
		"-read_uncommitted",
#endif
		"-consume",
		"-consume_wait",
		"-get_both",
		"-glob",
		"-partial",
		"-recno",
		"-rmw",
		"-txn",
		"--",
		NULL
	};
	enum dbgetopts {
#ifdef CONFIG_TEST
		DBGET_DATA_BUF_SIZE,
		DBGET_MULTI,
		DBGET_NOLEASE,
		DBGET_READ_COMMITTED,
		DBGET_READ_UNCOMMITTED,
#endif
		DBGET_CONSUME,
		DBGET_CONSUME_WAIT,
		DBGET_BOTH,
		DBGET_GLOB,
		DBGET_PART,
		DBGET_RECNO,
		DBGET_RMW,
		DBGET_TXN,
		DBGET_ENDARG
	};
	DBC *dbc;
	DBT key, pkey, data, save;
	DBTYPE ptype, type;
	DB_TXN *txn;
	Tcl_Obj **elemv, *retlist;
	db_recno_t precno, recno;
	u_int32_t flag, cflag, isdup, mflag, rmw;
	int elemc, end, endarg, freekey, freedata, i;
	int optindex, result, ret, useglob, useprecno, userecno;
	char *arg, *pattern, *prefix, msg[MSG_SIZE];
	void *dtmp, *ktmp;
#ifdef CONFIG_TEST
	int bufsize, data_buf_size;
#endif

	result = TCL_OK;
	freekey = freedata = 0;
	cflag = endarg = flag = mflag = rmw = 0;
	useglob = userecno = 0;
	txn = NULL;
	pattern = prefix = NULL;
	dtmp = ktmp = NULL;
#ifdef CONFIG_TEST
	COMPQUIET(bufsize, 0);
	data_buf_size = 0;
#endif

	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-args? key");
		return (TCL_ERROR);
	}

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	memset(&save, 0, sizeof(save));

	/* For the primary key in a pget call. */
	memset(&pkey, 0, sizeof(pkey));

	/*
	 * Get the command name index from the object based on the options
	 * defined above.
	 */
	i = 2;
	(void)dbp->get_type(dbp, &type);
	end = objc;
	while (i < end) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbgetopts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			arg = Tcl_GetStringFromObj(objv[i], NULL);
			if (arg[0] == '-') {
				result = IS_HELP(objv[i]);
				goto out;
			} else
				Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum dbgetopts)optindex) {
#ifdef CONFIG_TEST
		case DBGET_DATA_BUF_SIZE:
			result =
			    Tcl_GetIntFromObj(interp, objv[i], &data_buf_size);
			if (result != TCL_OK)
				goto out;
			i++;
			break;
		case DBGET_MULTI:
			mflag |= DB_MULTIPLE;
			result =
			    Tcl_GetIntFromObj(interp, objv[i], &bufsize);
			if (result != TCL_OK)
				goto out;
			i++;
			break;
		case DBGET_NOLEASE:
			rmw |= DB_IGNORE_LEASE;
			break;
		case DBGET_READ_COMMITTED:
			rmw |= DB_READ_COMMITTED;
			break;
		case DBGET_READ_UNCOMMITTED:
			rmw |= DB_READ_UNCOMMITTED;
			break;
#endif
		case DBGET_BOTH:
			/*
			 * Change 'end' and make sure we aren't already past
			 * the new end.
			 */
			if (i > objc - 2) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-get_both key data?");
				result = TCL_ERROR;
				break;
			}
			end = objc - 2;
			FLAG_CHECK(flag);
			flag = DB_GET_BOTH;
			break;
		case DBGET_TXN:
			if (i >= end) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "Get: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		case DBGET_GLOB:
			useglob = 1;
			end = objc - 1;
			break;
		case DBGET_CONSUME:
			FLAG_CHECK(flag);
			flag = DB_CONSUME;
			break;
		case DBGET_CONSUME_WAIT:
			FLAG_CHECK(flag);
			flag = DB_CONSUME_WAIT;
			break;
		case DBGET_RECNO:
			end = objc - 1;
			userecno = 1;
			if (type != DB_RECNO && type != DB_QUEUE) {
				FLAG_CHECK(flag);
				flag = DB_SET_RECNO;
				key.flags |= DB_DBT_MALLOC;
			}
			break;
		case DBGET_RMW:
			rmw |= DB_RMW;
			break;
		case DBGET_PART:
			end = objc - 1;
			if (i == end) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-partial {offset length}?");
				result = TCL_ERROR;
				break;
			}
			/*
			 * Get sublist as {offset length}
			 */
			result = Tcl_ListObjGetElements(interp, objv[i++],
			    &elemc, &elemv);
			if (elemc != 2) {
				Tcl_SetResult(interp,
				    "List must be {offset length}", TCL_STATIC);
				result = TCL_ERROR;
				break;
			}
			save.flags = DB_DBT_PARTIAL;
			result = _GetUInt32(interp, elemv[0], &save.doff);
			if (result != TCL_OK)
				break;
			result = _GetUInt32(interp, elemv[1], &save.dlen);
			/*
			 * NOTE: We don't check result here because all we'd
			 * do is break anyway, and we are doing that.  If you
			 * add code here, you WILL need to add the check
			 * for result.  (See the check for save.doff, a few
			 * lines above and copy that.)
			 */
			break;
		case DBGET_ENDARG:
			endarg = 1;
			break;
		}
		if (result != TCL_OK)
			break;
		if (endarg)
			break;
	}
	if (result != TCL_OK)
		goto out;

	if (type == DB_RECNO || type == DB_QUEUE)
		userecno = 1;

	/*
	 * Check args we have left versus the flags we were given.
	 * We might have 0, 1 or 2 left.  If we have 0, it must
	 * be DB_CONSUME*, if 2, then DB_GET_BOTH, all others should
	 * be 1.
	 */
	if (((flag == DB_CONSUME || flag == DB_CONSUME_WAIT) && i != objc) ||
	    (flag == DB_GET_BOTH && i != objc - 2)) {
		Tcl_SetResult(interp,
		    "Wrong number of key/data given based on flags specified\n",
		    TCL_STATIC);
		result = TCL_ERROR;
		goto out;
	} else if (flag == 0 && i != objc - 1) {
		Tcl_SetResult(interp,
		    "Wrong number of key/data given\n", TCL_STATIC);
		result = TCL_ERROR;
		goto out;
	}

	/*
	 * Find out whether the primary key should also be a recno.
	 */
	if (ispget && dbp->s_primary != NULL) {
		(void)dbp->s_primary->get_type(dbp->s_primary, &ptype);
		useprecno = ptype == DB_RECNO || ptype == DB_QUEUE;
	} else
		useprecno = 0;

	/*
	 * Check for illegal combos of options.
	 */
	if (useglob && (userecno || flag == DB_SET_RECNO ||
	    type == DB_RECNO || type == DB_QUEUE)) {
		Tcl_SetResult(interp,
		    "Cannot use -glob and record numbers.\n",
		    TCL_STATIC);
		result = TCL_ERROR;
		goto out;
	}
#ifdef	CONFIG_TEST
	if (data_buf_size != 0 && flag == DB_GET_BOTH) {
		Tcl_SetResult(interp,
    "Only one of -data_buf_size or -get_both can be specified.\n",
		    TCL_STATIC);
		result = TCL_ERROR;
		goto out;
	}
	if (data_buf_size != 0 && mflag != 0) {
		Tcl_SetResult(interp,
    "Only one of -data_buf_size or -multi can be specified.\n",
		    TCL_STATIC);
		result = TCL_ERROR;
		goto out;
	}
#endif
	if (useglob && flag == DB_GET_BOTH) {
		Tcl_SetResult(interp,
		    "Only one of -glob or -get_both can be specified.\n",
		    TCL_STATIC);
		result = TCL_ERROR;
		goto out;
	}

	if (useglob)
		pattern = Tcl_GetStringFromObj(objv[objc - 1], NULL);

	/*
	 * This is the list we return
	 */
	retlist = Tcl_NewListObj(0, NULL);
	save.flags |= DB_DBT_MALLOC;

	/*
	 * isdup is used to know if we support duplicates.  If not, we
	 * can just do a db->get call and avoid using cursors.
	 */
	if ((ret = dbp->get_flags(dbp, &isdup)) != 0) {
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "db get");
		goto out;
	}
	isdup &= DB_DUP;

	/*
	 * If the database doesn't support duplicates or we're performing
	 * ops that don't require returning multiple items, use DB->get
	 * instead of a cursor operation.
	 */
	if (pattern == NULL && (isdup == 0 || mflag != 0 ||
#ifdef	CONFIG_TEST
	    data_buf_size != 0 ||
#endif
	    flag == DB_SET_RECNO || flag == DB_GET_BOTH ||
	    flag == DB_CONSUME || flag == DB_CONSUME_WAIT)) {
#ifdef	CONFIG_TEST
		if (data_buf_size == 0) {
			F_CLR(&save, DB_DBT_USERMEM);
			F_SET(&save, DB_DBT_MALLOC);
		} else {
			(void)__os_malloc(
			    NULL, (size_t)data_buf_size, &save.data);
			save.ulen = (u_int32_t)data_buf_size;
			F_CLR(&save, DB_DBT_MALLOC);
			F_SET(&save, DB_DBT_USERMEM);
		}
#endif
		if (flag == DB_GET_BOTH) {
			if (userecno) {
				result = _GetUInt32(interp,
				    objv[(objc - 2)], &recno);
				if (result == TCL_OK) {
					key.data = &recno;
					key.size = sizeof(db_recno_t);
				} else
					goto out;
			} else {
				/*
				 * Some get calls (SET_*) can change the
				 * key pointers.  So, we need to store
				 * the allocated key space in a tmp.
				 */
				ret = _CopyObjBytes(interp, objv[objc-2],
				    &key.data, &key.size, &freekey);
				if (ret != 0) {
					result = _ReturnSetup(interp, ret,
					    DB_RETOK_DBGET(ret), "db get");
					goto out;
				}
			}
			ktmp = key.data;
			/*
			 * Already checked args above.  Fill in key and save.
			 * Save is used in the dbp->get call below to fill in
			 * data.
			 *
			 * If the "data" here is really a primary key--that
			 * is, if we're in a pget--and that primary key
			 * is a recno, treat it appropriately as an int.
			 */
			if (useprecno) {
				result = _GetUInt32(interp,
				    objv[objc - 1], &precno);
				if (result == TCL_OK) {
					save.data = &precno;
					save.size = sizeof(db_recno_t);
				} else
					goto out;
			} else {
				ret = _CopyObjBytes(interp, objv[objc-1],
				    &dtmp, &save.size, &freedata);
				if (ret != 0) {
					result = _ReturnSetup(interp, ret,
					    DB_RETOK_DBGET(ret), "db get");
					goto out;
				}
				save.data = dtmp;
			}
		} else if (flag != DB_CONSUME && flag != DB_CONSUME_WAIT) {
			if (userecno) {
				result = _GetUInt32(
				    interp, objv[(objc - 1)], &recno);
				if (result == TCL_OK) {
					key.data = &recno;
					key.size = sizeof(db_recno_t);
				} else
					goto out;
			} else {
				/*
				 * Some get calls (SET_*) can change the
				 * key pointers.  So, we need to store
				 * the allocated key space in a tmp.
				 */
				ret = _CopyObjBytes(interp, objv[objc-1],
				    &key.data, &key.size, &freekey);
				if (ret != 0) {
					result = _ReturnSetup(interp, ret,
					    DB_RETOK_DBGET(ret), "db get");
					goto out;
				}
			}
			ktmp = key.data;
#ifdef CONFIG_TEST
			if (mflag & DB_MULTIPLE) {
				if ((ret = __os_malloc(dbp->env,
				    (size_t)bufsize, &save.data)) != 0) {
					Tcl_SetResult(interp,
					    db_strerror(ret), TCL_STATIC);
					goto out;
				}
				save.ulen = (u_int32_t)bufsize;
				F_CLR(&save, DB_DBT_MALLOC);
				F_SET(&save, DB_DBT_USERMEM);
			}
#endif
		}

		data = save;

		if (ispget) {
			if (flag == DB_GET_BOTH) {
				pkey.data = save.data;
				pkey.size = save.size;
				data.data = NULL;
				data.size = 0;
			}
			F_SET(&pkey, DB_DBT_MALLOC);
			_debug_check();
			ret = dbp->pget(dbp,
			    txn, &key, &pkey, &data, flag | rmw);
		} else {
			_debug_check();
			ret = dbp->get(dbp,
			    txn, &key, &data, flag | rmw | mflag);
		}
		result = _ReturnSetup(interp, ret, DB_RETOK_DBGET(ret),
		    "db get");
		if (ret == 0) {
			/*
			 * Success.  Return a list of the form {name value}
			 * If it was a recno in key.data, we need to convert
			 * into a string/object representation of that recno.
			 */
			if (mflag & DB_MULTIPLE)
				result = _SetMultiList(interp,
				    retlist, &key, &data, type, flag);
			else if (type == DB_RECNO || type == DB_QUEUE)
				if (ispget)
					result = _Set3DBTList(interp,
					    retlist, &key, 1, &pkey,
					    useprecno, &data);
				else
					result = _SetListRecnoElem(interp,
					    retlist, *(db_recno_t *)key.data,
					    data.data, data.size);
			else {
				if (ispget)
					result = _Set3DBTList(interp,
					    retlist, &key, 0, &pkey,
					    useprecno, &data);
				else
					result = _SetListElem(interp, retlist,
					    key.data, key.size,
					    data.data, data.size);
			}
		}
		/*
		 * Free space from DBT.
		 *
		 * If we set DB_DBT_MALLOC, we need to free the space if and
		 * only if we succeeded and if DB allocated anything (the
		 * pointer has changed from what we passed in).  If
		 * DB_DBT_MALLOC is not set, this is a bulk get buffer, and
		 * needs to be freed no matter what.
		 */
		if (F_ISSET(&key, DB_DBT_MALLOC) && ret == 0 &&
		    key.data != ktmp)
			__os_ufree(dbp->env, key.data);
		if (F_ISSET(&data, DB_DBT_MALLOC) && ret == 0 &&
		    data.data != dtmp)
			__os_ufree(dbp->env, data.data);
		else if (!F_ISSET(&data, DB_DBT_MALLOC))
			__os_free(dbp->env, data.data);
		if (ispget && ret == 0 && pkey.data != save.data)
			__os_ufree(dbp->env, pkey.data);
		if (result == TCL_OK)
			Tcl_SetObjResult(interp, retlist);
		goto out;
	}

	if (userecno) {
		result = _GetUInt32(interp, objv[(objc - 1)], &recno);
		if (result == TCL_OK) {
			key.data = &recno;
			key.size = sizeof(db_recno_t);
		} else
			goto out;
	} else {
		/*
		 * Some get calls (SET_*) can change the
		 * key pointers.  So, we need to store
		 * the allocated key space in a tmp.
		 */
		ret = _CopyObjBytes(interp, objv[objc-1], &key.data,
		    &key.size, &freekey);
		if (ret != 0) {
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_DBGET(ret), "db get");
			return (result);
		}
	}
	ktmp = key.data;
	ret = dbp->cursor(dbp, txn, &dbc, 0);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "db cursor");
	if (result == TCL_ERROR)
		goto out;

	/*
	 * At this point, we have a cursor, if we have a pattern,
	 * we go to the nearest one and step forward until we don't
	 * have any more that match the pattern prefix.  If we have
	 * an exact key, we go to that key position, and step through
	 * all the duplicates.  In either case we build up a list of
	 * the form {{key data} {key data}...} along the way.
	 */
	memset(&data, 0, sizeof(data));
	/*
	 * Restore any "partial" info we have saved.
	 */
	data = save;
	if (pattern) {
		/*
		 * Note, prefix is returned in new space.  Must free it.
		 */
		ret = _GetGlobPrefix(pattern, &prefix);
		if (ret) {
			result = TCL_ERROR;
			Tcl_SetResult(interp,
			    "Unable to allocate pattern space", TCL_STATIC);
			goto out1;
		}
		key.data = prefix;
		key.size = (u_int32_t)strlen(prefix);
		/*
		 * If they give us an empty pattern string
		 * (i.e. -glob *), go through entire DB.
		 */
		if (strlen(prefix) == 0)
			cflag = DB_FIRST;
		else
			cflag = DB_SET_RANGE;
	} else
		cflag = DB_SET;
	if (ispget) {
		_debug_check();
		F_SET(&pkey, DB_DBT_MALLOC);
		ret = dbc->pget(dbc, &key, &pkey, &data, cflag | rmw);
	} else {
		_debug_check();
		ret = dbc->get(dbc, &key, &data, cflag | rmw);
	}
	result = _ReturnSetup(interp, ret, DB_RETOK_DBCGET(ret),
	    "db get (cursor)");
	if (result == TCL_ERROR)
		goto out1;
	if (pattern) {
		if (ret == 0 && prefix != NULL &&
		    memcmp(key.data, prefix, strlen(prefix)) != 0) {
			/*
			 * Free space from DB_DBT_MALLOC
			 */
			__os_ufree(dbp->env, data.data);
			goto out1;
		}
		cflag = DB_NEXT;
	} else
		cflag = DB_NEXT_DUP;

	while (ret == 0 && result == TCL_OK) {
		/*
		 * Build up our {name value} sublist
		 */
		if (ispget)
			result = _Set3DBTList(interp, retlist, &key, 0,
			    &pkey, useprecno, &data);
		else
			result = _SetListElem(interp, retlist,
			    key.data, key.size, data.data, data.size);
		/*
		 * Free space from DB_DBT_MALLOC
		 */
		if (ispget)
			__os_ufree(dbp->env, pkey.data);
		__os_ufree(dbp->env, data.data);
		if (result != TCL_OK)
			break;
		/*
		 * Append {name value} to return list
		 */
		memset(&key, 0, sizeof(key));
		memset(&pkey, 0, sizeof(pkey));
		memset(&data, 0, sizeof(data));
		/*
		 * Restore any "partial" info we have saved.
		 */
		data = save;
		if (ispget) {
			F_SET(&pkey, DB_DBT_MALLOC);
			ret = dbc->pget(dbc, &key, &pkey, &data, cflag | rmw);
		} else
			ret = dbc->get(dbc, &key, &data, cflag | rmw);
		if (ret == 0 && prefix != NULL &&
		    memcmp(key.data, prefix, strlen(prefix)) != 0) {
			/*
			 * Free space from DB_DBT_MALLOC
			 */
			__os_ufree(dbp->env, data.data);
			break;
		}
	}
out1:
	(void)dbc->close(dbc);
	if (result == TCL_OK)
		Tcl_SetObjResult(interp, retlist);
out:
	/*
	 * _GetGlobPrefix(), the function which allocates prefix, works
	 * by copying and condensing another string.  Thus prefix may
	 * have multiple nuls at the end, so we free using __os_free().
	 */
	if (prefix != NULL)
		__os_free(dbp->env, prefix);
	if (dtmp != NULL && freedata)
		__os_free(dbp->env, dtmp);
	if (ktmp != NULL && freekey)
		__os_free(dbp->env, ktmp);
	return (result);
}

/*
 * tcl_db_delete --
 */
static int
tcl_DbDelete(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	static const char *dbdelopts[] = {
		"-consume",
		"-glob",
		"-multiple",
		"-multiple_key",
		"-txn",
		NULL
	};
	enum dbdelopts {
		DBDEL_CONSUME,
		DBDEL_GLOB,
		DBDEL_MULTIPLE,
		DBDEL_MULTIPLE_KEY,
		DBDEL_TXN
	};
	DBC *dbc;
	DBT key, data;
	DBTYPE type;
	DB_TXN *txn;
	Tcl_Obj **elemv;
	void *dtmp, *ktmp, *ptr;
	db_recno_t recno;
	int dlen, elemc, freekey, i, j, klen, optindex, result, ret;
	u_int32_t dflag, flag, multiflag;
	char *arg, *pattern, *prefix, msg[MSG_SIZE];

	result = TCL_OK;
	freekey = 0;
	dflag = 0;
	multiflag = 0;
	pattern = prefix = NULL;
	txn = NULL;
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-args? key");
		return (TCL_ERROR);
	}

	dtmp = ktmp = NULL;
	memset(&key, 0, sizeof(key));
	/*
	 * The first arg must be -glob, -txn or a list of keys.
	 */
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbdelopts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			/*
			 * If we don't have a -glob or -txn, then the remaining
			 * args must be exact keys.  Reset the result so we
			 * don't get an errant error message if there is another
			 * error.
			 */
			if (IS_HELP(objv[i]) == TCL_OK)
				return (TCL_OK);
			Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum dbdelopts)optindex) {
		case DBDEL_TXN:
			if (i == objc) {
				/*
				 * Someone could conceivably have a key of
				 * the same name.  So just break and use it.
				 */
				i--;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "Delete: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		case DBDEL_GLOB:
			/*
			 * Get the pattern.  Get the prefix and use cursors to
			 * get all the data items.
			 */
			if (i == objc) {
				/*
				 * Someone could conceivably have a key of
				 * the same name.  So just break and use it.
				 */
				i--;
				break;
			}
			pattern = Tcl_GetStringFromObj(objv[i++], NULL);
			break;
		case DBDEL_CONSUME:
			FLAG_CHECK(dflag);
			dflag = DB_CONSUME;
			break;
		case DBDEL_MULTIPLE:
			FLAG_CHECK(multiflag);
			multiflag |= DB_MULTIPLE;
			break;
		case DBDEL_MULTIPLE_KEY:
			FLAG_CHECK(multiflag);
			multiflag |= DB_MULTIPLE_KEY;
			break;
		}
		if (result != TCL_OK)
			break;
	}

	if (result != TCL_OK)
		goto out;
	/*
	 * XXX
	 * For consistency with get, we have decided for the moment, to
	 * allow -glob, or one key, not many.  The code was originally
	 * written to take many keys and we'll leave it that way, because
	 * tcl_DbGet may one day accept many disjoint keys to get, rather
	 * than one, and at that time we'd make delete be consistent.  In
	 * any case, the code is already here and there is no need to remove,
	 * just check that we only have one arg left.
	 *
	 * If we have a pattern AND more keys to process, there is an error.
	 * Either we have some number of exact keys, or we have a pattern.
	 */
	if (pattern == NULL) {
		if (i != (objc - 1)) {
			Tcl_WrongNumArgs(
			    interp, 2, objv, "?args? -glob pattern | key");
			result = TCL_ERROR;
			goto out;
		}
	} else {
		if (i != objc) {
			Tcl_WrongNumArgs(
			    interp, 2, objv, "?args? -glob pattern | key");
			result = TCL_ERROR;
			goto out;
		}
	}

	/*
	 * If we have remaining args, they are all exact keys.  Call
	 * DB->del on each of those keys.
	 *
	 * If it is a RECNO database, the key is a record number and must be
	 * setup up to contain a db_recno_t.  Otherwise the key is a "string".
	 */
	(void)dbp->get_type(dbp, &type);
	ret = 0;
	while (i < objc && ret == 0) {
		memset(&key, 0, sizeof(key));
		if (multiflag == DB_MULTIPLE) {
			/*
			 * To work out how big a buffer is needed, we first
			 * need to find out the total length of the data and
			 * the number of data items (elemc).
			 */
			ktmp = Tcl_GetByteArrayFromObj(objv[i], &klen);
			result = Tcl_ListObjGetElements(interp, objv[i++],
			    &elemc, &elemv);
			if (result != TCL_OK)
				return (result);

			memset(&key, 0, sizeof(key));
			key.ulen = DB_ALIGN((u_int32_t)klen + (u_int32_t)elemc
			    * sizeof(u_int32_t) * 2, 1024UL);
			key.flags = DB_DBT_USERMEM | DB_DBT_BULK;
			if ((ret =
			    __os_malloc(dbp->env, key.ulen, &key.data)) != 0)
				return (ret);
			freekey = 1;

			if (type == DB_RECNO || type == DB_QUEUE) {
				DB_MULTIPLE_RECNO_WRITE_INIT(ptr, &key);
				for (j = 0; j < elemc; j++) {
					result =
					    _GetUInt32(interp,
					    elemv[j], &recno);
					if (result != TCL_OK)
						return (result);
					DB_MULTIPLE_RECNO_WRITE_NEXT(ptr,
					    &key, recno, dtmp, 0);
					DB_ASSERT(dbp->env, ptr != NULL);
				}
			} else {
				DB_MULTIPLE_WRITE_INIT(ptr, &key);
				for (j = 0; j < elemc; j++) {
					ktmp = Tcl_GetByteArrayFromObj(elemv[j],
					    &klen);
					DB_MULTIPLE_WRITE_NEXT(ptr,
					    &key, ktmp, (u_int32_t)klen);
					DB_ASSERT(dbp->env, ptr != NULL);
				}
			}
		} else if (multiflag == DB_MULTIPLE_KEY) {
			/*
			 * To work out how big a buffer is needed, we first
			 * need to find out the total length of the data (len)
			 * and the number of data items (elemc).
			 */
			ktmp = Tcl_GetByteArrayFromObj(objv[i], &klen);
			result = Tcl_ListObjGetElements(interp, objv[i++],
			    &elemc, &elemv);
			if (result != TCL_OK)
				return (result);

			memset(&key, 0, sizeof(key));
			key.ulen = DB_ALIGN((u_int32_t)klen +
			    (u_int32_t)elemc * sizeof(u_int32_t) * 2, 1024UL);
			key.flags = DB_DBT_USERMEM | DB_DBT_BULK;
			if ((ret =
			    __os_malloc(dbp->env, key.ulen, &key.data)) != 0)
				return (ret);
			freekey = 1;

			if (type == DB_RECNO || type == DB_QUEUE) {
				DB_MULTIPLE_RECNO_WRITE_INIT(ptr, &key);
				for (j = 0; j + 1 < elemc; j += 2) {
					result =
					    _GetUInt32(interp,
					    elemv[j], &recno);
					if (result != TCL_OK)
						return (result);
					dtmp = Tcl_GetByteArrayFromObj(
					    elemv[j + 1], &dlen);
					DB_MULTIPLE_RECNO_WRITE_NEXT(ptr,
					    &key, recno, dtmp, (u_int32_t)dlen);
					DB_ASSERT(dbp->env, ptr != NULL);
				}
			} else {
				DB_MULTIPLE_WRITE_INIT(ptr, &key);
				for (j = 0; j + 1 < elemc; j += 2) {
					ktmp = Tcl_GetByteArrayFromObj(
					    elemv[j], &klen);
					dtmp = Tcl_GetByteArrayFromObj(
					    elemv[j + 1], &dlen);
					DB_MULTIPLE_KEY_WRITE_NEXT(ptr,
					    &key, ktmp, (u_int32_t)klen,
					    dtmp, (u_int32_t)dlen);
					DB_ASSERT(dbp->env, ptr != NULL);
				}
			}
		} else if (type == DB_RECNO || type == DB_QUEUE) {
			result = _GetUInt32(interp, objv[i++], &recno);
			if (result == TCL_OK) {
				key.data = &recno;
				key.size = sizeof(db_recno_t);
			} else
				return (result);
		} else {
			ret = _CopyObjBytes(interp, objv[i++], &ktmp,
			    &key.size, &freekey);
			if (ret != 0) {
				result = _ReturnSetup(interp, ret,
				    DB_RETOK_DBDEL(ret), "db del");
				return (result);
			}
			key.data = ktmp;
		}
		_debug_check();
		ret = dbp->del(dbp, txn, &key, dflag | multiflag);
		/*
		 * If we have any error, set up return result and stop
		 * processing keys.
		 */
		if (freekey && key.data != NULL)
			__os_free(dbp->env, key.data);
		if (ret != 0)
			break;
	}
	result = _ReturnSetup(interp, ret, DB_RETOK_DBDEL(ret), "db del");

	/*
	 * At this point we've either finished or, if we have a pattern,
	 * we go to the nearest one and step forward until we don't
	 * have any more that match the pattern prefix.
	 */
	if (pattern) {
		ret = dbp->cursor(dbp, txn, &dbc, 0);
		if (ret != 0) {
			result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "db cursor");
			goto out;
		}
		/*
		 * Note, prefix is returned in new space.  Must free it.
		 */
		memset(&key, 0, sizeof(key));
		memset(&data, 0, sizeof(data));
		ret = _GetGlobPrefix(pattern, &prefix);
		if (ret) {
			result = TCL_ERROR;
			Tcl_SetResult(interp,
			    "Unable to allocate pattern space", TCL_STATIC);
			goto out;
		}
		key.data = prefix;
		key.size = (u_int32_t)strlen(prefix);
		if (strlen(prefix) == 0)
			flag = DB_FIRST;
		else
			flag = DB_SET_RANGE;
		ret = dbc->get(dbc, &key, &data, flag);
		while (ret == 0 &&
		    memcmp(key.data, prefix, strlen(prefix)) == 0) {
			/*
			 * Each time through here the cursor is pointing
			 * at the current valid item.  Delete it and
			 * move ahead.
			 */
			_debug_check();
			ret = dbc->del(dbc, dflag);
			if (ret != 0) {
				result = _ReturnSetup(interp, ret,
				    DB_RETOK_DBCDEL(ret), "db c_del");
				break;
			}
			/*
			 * Deleted the current, now move to the next item
			 * in the list, check if it matches the prefix pattern.
			 */
			memset(&key, 0, sizeof(key));
			memset(&data, 0, sizeof(data));
			ret = dbc->get(dbc, &key, &data, DB_NEXT);
		}
		if (ret == DB_NOTFOUND)
			ret = 0;
		/*
		 * _GetGlobPrefix(), the function which allocates prefix, works
		 * by copying and condensing another string.  Thus prefix may
		 * have multiple nuls at the end, so we free using __os_free().
		 */
		__os_free(dbp->env, prefix);
		(void)dbc->close(dbc);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "db del");
	}
out:
	return (result);
}

/*
 * tcl_db_cursor --
 */
static int
tcl_DbCursor(interp, objc, objv, dbp, dbcp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
	DBC **dbcp;			/* Return cursor pointer */
{
	static const char *dbcuropts[] = {
#ifdef CONFIG_TEST
		"-read_committed",
		"-read_uncommitted",
		"-update",
#endif
		"-bulk",
		"-txn",
		NULL
	};
	enum dbcuropts {
#ifdef CONFIG_TEST
		DBCUR_READ_COMMITTED,
		DBCUR_READ_UNCOMMITTED,
		DBCUR_UPDATE,
#endif
		DBCUR_BULK,
		DBCUR_TXN
	};
	DB_TXN *txn;
	u_int32_t flag;
	int i, optindex, result, ret;
	char *arg, msg[MSG_SIZE];

	result = TCL_OK;
	flag = 0;
	txn = NULL;
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbcuropts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			goto out;
		}
		i++;
		switch ((enum dbcuropts)optindex) {
#ifdef CONFIG_TEST
		case DBCUR_READ_COMMITTED:
			flag |= DB_READ_COMMITTED;
			break;
		case DBCUR_READ_UNCOMMITTED:
			flag |= DB_READ_UNCOMMITTED;
			break;
		case DBCUR_UPDATE:
			flag |= DB_WRITECURSOR;
			break;
#endif
		case DBCUR_BULK:
			flag |= DB_CURSOR_BULK;
			break;
		case DBCUR_TXN:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "Cursor: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		}
		if (result != TCL_OK)
			break;
	}
	if (result != TCL_OK)
		goto out;

	_debug_check();
	ret = dbp->cursor(dbp, txn, dbcp, flag);
	if (ret != 0)
		result = _ErrorSetup(interp, ret, "db cursor");
out:
	return (result);
}

/*
 * tcl_DbAssociate --
 *	Call DB->associate().
 */
static int
tcl_DbAssociate(interp, objc, objv, dbp)
	Tcl_Interp *interp;
	int objc;
	Tcl_Obj *CONST objv[];
	DB *dbp;
{
	static const char *dbaopts[] = {
		"-create",
		"-immutable_key",
		"-txn",
		NULL
	};
	enum dbaopts {
		DBA_CREATE,
		DBA_IMMUTABLE_KEY,
		DBA_TXN
	};
	DB *sdbp;
	DB_TXN *txn;
	DBTCL_INFO *sdbip;
	int i, optindex, result, ret;
	char *arg, msg[MSG_SIZE];
	u_int32_t flag;
#ifdef CONFIG_TEST
	/*
	 * When calling DB->associate over RPC, the Tcl API uses
	 * special flags that the RPC server interprets to set the
	 * callback correctly.
	 */
	const char *cbname;
	struct {
		const char *name;
		u_int32_t flag;
	} *cb, callbacks[] = {
		{ "", 0 }, /* A NULL callback in Tcl. */
		{ "_s_reversedata", DB_RPC2ND_REVERSEDATA },
		{ "_s_noop", DB_RPC2ND_NOOP },
		{ "_s_concatkeydata", DB_RPC2ND_CONCATKEYDATA },
		{ "_s_concatdatakey", DB_RPC2ND_CONCATDATAKEY },
		{ "_s_reverseconcat", DB_RPC2ND_REVERSECONCAT },
		{ "_s_truncdata", DB_RPC2ND_TRUNCDATA },
		{ "_s_reversedata", DB_RPC2ND_REVERSEDATA },
		{ "_s_constant", DB_RPC2ND_CONSTANT },
		{ "sj_getzip", DB_RPC2ND_GETZIP },
		{ "sj_getname", DB_RPC2ND_GETNAME },
		{ NULL, 0 }
	};
#endif

	txn = NULL;
	result = TCL_OK;
	flag = 0;
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "[callback] secondary");
		return (TCL_ERROR);
	}

	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbaopts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			if (result == TCL_OK)
				return (result);
			result = TCL_OK;
			Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum dbaopts)optindex) {
		case DBA_CREATE:
			flag |= DB_CREATE;
			break;
		case DBA_IMMUTABLE_KEY:
			flag |= DB_IMMUTABLE_KEY;
			break;
		case DBA_TXN:
			if (i > (objc - 1)) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "Associate: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		}
	}
	if (result != TCL_OK)
		return (result);

	/*
	 * Better be 1 or 2 args left.  The last arg must be the sdb
	 * handle.  If 2 args then objc-2 is the callback proc, else
	 * we have a NULL callback.
	 */
	/* Get the secondary DB handle. */
	arg = Tcl_GetStringFromObj(objv[objc - 1], NULL);
	sdbp = NAME_TO_DB(arg);
	if (sdbp == NULL) {
		snprintf(msg, MSG_SIZE,
		    "Associate: Invalid database handle: %s\n", arg);
		Tcl_SetResult(interp, msg, TCL_VOLATILE);
		return (TCL_ERROR);
	}

	/*
	 * The callback is simply a Tcl object containing the name
	 * of the callback proc, which is the second-to-last argument.
	 *
	 * Note that the callback needs to go in the *secondary* DB handle's
	 * info struct;  we may have multiple secondaries with different
	 * callbacks.
	 */
	sdbip = (DBTCL_INFO *)sdbp->api_internal;

#ifdef CONFIG_TEST
	if (i != objc - 1 && RPC_ON(dbp->dbenv)) {
		/*
		 * The flag values allowed to DB->associate may have changed to
		 * overlap with the range we've chosen.  If this happens, we
		 * need to reset all of the RPC_2ND_* flags to a new range.
		 */
		if ((flag & DB_RPC2ND_MASK) != 0) {
			snprintf(msg, MSG_SIZE,
			    "RPC secondary flags overlap -- recalculate!\n");
			Tcl_SetResult(interp, msg, TCL_VOLATILE);
			return (TCL_ERROR);
		}

		cbname = Tcl_GetStringFromObj(objv[objc - 2], NULL);
		for (cb = callbacks; cb->name != NULL; cb++)
			if (strcmp(cb->name, cbname) == 0) {
				flag |= cb->flag;
				break;
			}

		if (cb->name == NULL) {
			snprintf(msg, MSG_SIZE,
			    "Associate: unknown callback: %s\n", cbname);
			Tcl_SetResult(interp, msg, TCL_VOLATILE);
			return (TCL_ERROR);
		}

		ret = dbp->associate(dbp, txn, sdbp, NULL, flag);

		/*
		 * The primary reference isn't set when calling through
		 * the RPC server, but the Tcl API peeks at it in other
		 * places (see tcl_DbGet).
		 */
		if (ret == 0)
			sdbp->s_primary = dbp;
	} else if (i != objc - 1) {
#else
	if (i != objc - 1) {
#endif
		/*
		 * We have 2 args, get the callback.
		 */
		sdbip->i_second_call = objv[objc - 2];
		Tcl_IncrRefCount(sdbip->i_second_call);

		/* Now call associate. */
		_debug_check();
		ret = dbp->associate(dbp, txn, sdbp, tcl_second_call, flag);
	} else {
		/*
		 * We have a NULL callback.
		 */
		sdbip->i_second_call = NULL;
		ret = dbp->associate(dbp, txn, sdbp, NULL, flag);
	}
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "associate");

	return (result);
}

/*
 * tcl_second_call --
 *	Callback function for secondary indices.  Get the callback
 *	out of ip->i_second_call and call it.
 */
static int
tcl_second_call(dbp, pkey, data, skey)
	DB *dbp;
	const DBT *pkey, *data;
	DBT *skey;
{
	DBT *tskey;
	DBTCL_INFO *ip;
	Tcl_Interp *interp;
	Tcl_Obj *pobj, *dobj, *objv[3], *robj, **skeylist;
	size_t len;
	int ilen, result, ret;
	u_int32_t i, nskeys;
	void *retbuf, *databuf;

	ip = (DBTCL_INFO *)dbp->api_internal;
	interp = ip->i_interp;
	objv[0] = ip->i_second_call;

	/*
	 * Create two ByteArray objects, with the contents of the pkey
	 * and data DBTs that are our inputs.
	 */
	pobj = Tcl_NewByteArrayObj(pkey->data, (int)pkey->size);
	Tcl_IncrRefCount(pobj);
	dobj = Tcl_NewByteArrayObj(data->data, (int)data->size);
	Tcl_IncrRefCount(dobj);

	objv[1] = pobj;
	objv[2] = dobj;

	result = Tcl_EvalObjv(interp, 3, objv, 0);

	Tcl_DecrRefCount(pobj);
	Tcl_DecrRefCount(dobj);

	if (result != TCL_OK) {
		__db_errx(dbp->env,
		    "Tcl callback function failed with code %d", result);
		return (EINVAL);
	}

	robj = Tcl_GetObjResult(interp);
	if (robj->typePtr == NULL || strcmp(robj->typePtr->name, "list") != 0) {
		nskeys = 1;
		skeylist = &robj;
		tskey = skey;
	} else {
		if ((result = Tcl_ListObjGetElements(interp,
		    robj, &ilen, &skeylist)) != TCL_OK) {
			__db_errx(dbp->env,
			    "Could not get list elements from Tcl callback");
			return (EINVAL);
		}
		nskeys = (u_int32_t)ilen;

		/*
		 * It would be nice to check for nskeys == 0 and return
		 * DB_DONOTINDEX, but Tcl does not distinguish between an empty
		 * string and an empty list, so that would disallow empty
		 * secondary keys.
		 */
		if (nskeys == 0) {
			nskeys = 1;
			skeylist = &robj;
		}
		if (nskeys == 1)
			tskey = skey;
		else {
			memset(skey, 0, sizeof(DBT));
			if ((ret = __os_umalloc(dbp->env,
			    nskeys * sizeof(DBT), &skey->data)) != 0)
				return (ret);
			skey->size = nskeys;
			F_SET(skey, DB_DBT_MULTIPLE | DB_DBT_APPMALLOC);
			tskey = (DBT *)skey->data;
		}
	}

	for (i = 0; i < nskeys; i++, tskey++) {
		retbuf = Tcl_GetByteArrayFromObj(skeylist[i], &ilen);
		len = (size_t)ilen;

		/*
		 * retbuf is owned by Tcl; copy it into malloc'ed memory.
		 * We need to use __os_umalloc rather than ufree because this
		 * will be freed by DB using __os_ufree--the DB_DBT_APPMALLOC
		 * flag tells DB to free application-allocated memory.
		 */
		if ((ret = __os_umalloc(dbp->env, len, &databuf)) != 0)
			return (ret);
		memcpy(databuf, retbuf, len);

		memset(tskey, 0, sizeof(DBT));
		tskey->data = databuf;
		tskey->size = (u_int32_t)len;
		F_SET(tskey, DB_DBT_APPMALLOC);
	}

	return (0);
}

/*
 * tcl_db_join --
 */
static int
tcl_DbJoin(interp, objc, objv, dbp, dbcp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
	DBC **dbcp;			/* Cursor pointer */
{
	static const char *dbjopts[] = {
		"-nosort",
		NULL
	};
	enum dbjopts {
		DBJ_NOSORT
	};
	DBC **listp;
	size_t size;
	u_int32_t flag;
	int adj, i, j, optindex, result, ret;
	char *arg, msg[MSG_SIZE];

	result = TCL_OK;
	flag = 0;
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "curs1 curs2 ...");
		return (TCL_ERROR);
	}

	for (adj = i = 2; i < objc; i++) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbjopts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			if (result == TCL_OK)
				return (result);
			result = TCL_OK;
			Tcl_ResetResult(interp);
			break;
		}
		switch ((enum dbjopts)optindex) {
		case DBJ_NOSORT:
			flag |= DB_JOIN_NOSORT;
			adj++;
			break;
		}
	}
	if (result != TCL_OK)
		return (result);
	/*
	 * Allocate one more for NULL ptr at end of list.
	 */
	size = sizeof(DBC *) * (size_t)((objc - adj) + 1);
	ret = __os_malloc(dbp->env, size, &listp);
	if (ret != 0) {
		Tcl_SetResult(interp, db_strerror(ret), TCL_STATIC);
		return (TCL_ERROR);
	}

	memset(listp, 0, size);
	for (j = 0, i = adj; i < objc; i++, j++) {
		arg = Tcl_GetStringFromObj(objv[i], NULL);
		listp[j] = NAME_TO_DBC(arg);
		if (listp[j] == NULL) {
			snprintf(msg, MSG_SIZE,
			    "Join: Invalid cursor: %s\n", arg);
			Tcl_SetResult(interp, msg, TCL_VOLATILE);
			result = TCL_ERROR;
			goto out;
		}
	}
	listp[j] = NULL;
	_debug_check();
	ret = dbp->join(dbp, listp, dbcp, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "db join");

out:
	__os_free(dbp->env, listp);
	return (result);
}

/*
 * tcl_db_getjoin --
 */
static int
tcl_DbGetjoin(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	static const char *dbgetjopts[] = {
#ifdef CONFIG_TEST
		"-nosort",
#endif
		"-txn",
		NULL
	};
	enum dbgetjopts {
#ifdef CONFIG_TEST
		DBGETJ_NOSORT,
#endif
		DBGETJ_TXN
	};
	DB_TXN *txn;
	DB *elemdbp;
	DBC **listp;
	DBC *dbc;
	DBT key, data;
	Tcl_Obj **elemv, *retlist;
	void *ktmp;
	size_t size;
	u_int32_t flag;
	int adj, elemc, freekey, i, j, optindex, result, ret;
	char *arg, msg[MSG_SIZE];

	result = TCL_OK;
	flag = 0;
	ktmp = NULL;
	freekey = 0;
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "{db1 key1} {db2 key2} ...");
		return (TCL_ERROR);
	}

	txn = NULL;
	i = 2;
	adj = i;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbgetjopts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			if (result == TCL_OK)
				return (result);
			result = TCL_OK;
			Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum dbgetjopts)optindex) {
#ifdef CONFIG_TEST
		case DBGETJ_NOSORT:
			flag |= DB_JOIN_NOSORT;
			adj++;
			break;
#endif
		case DBGETJ_TXN:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			adj += 2;
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "GetJoin: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		}
	}
	if (result != TCL_OK)
		return (result);
	size = sizeof(DBC *) * (size_t)((objc - adj) + 1);
	ret = __os_malloc(NULL, size, &listp);
	if (ret != 0) {
		Tcl_SetResult(interp, db_strerror(ret), TCL_STATIC);
		return (TCL_ERROR);
	}

	memset(listp, 0, size);
	for (j = 0, i = adj; i < objc; i++, j++) {
		/*
		 * Get each sublist as {db key}
		 */
		result = Tcl_ListObjGetElements(interp, objv[i],
		    &elemc, &elemv);
		if (elemc != 2) {
			Tcl_SetResult(interp, "Lists must be {db key}",
			    TCL_STATIC);
			result = TCL_ERROR;
			goto out;
		}
		/*
		 * Get a pointer to that open db.  Then, open a cursor in
		 * that db, and go to the "key" place.
		 */
		elemdbp = NAME_TO_DB(Tcl_GetStringFromObj(elemv[0], NULL));
		if (elemdbp == NULL) {
			snprintf(msg, MSG_SIZE, "Get_join: Invalid db: %s\n",
			    Tcl_GetStringFromObj(elemv[0], NULL));
			Tcl_SetResult(interp, msg, TCL_VOLATILE);
			result = TCL_ERROR;
			goto out;
		}
		ret = elemdbp->cursor(elemdbp, txn, &listp[j], 0);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db cursor")) == TCL_ERROR)
			goto out;
		memset(&key, 0, sizeof(key));
		memset(&data, 0, sizeof(data));
		ret = _CopyObjBytes(interp, elemv[elemc-1], &ktmp,
		    &key.size, &freekey);
		if (ret != 0) {
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_STD(ret), "db join");
			goto out;
		}
		key.data = ktmp;
		ret = (listp[j])->get(listp[j], &key, &data, DB_SET);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_DBCGET(ret),
		    "db cget")) == TCL_ERROR)
			goto out;
	}
	listp[j] = NULL;
	_debug_check();
	ret = dbp->join(dbp, listp, &dbc, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "db join");
	if (result == TCL_ERROR)
		goto out;

	retlist = Tcl_NewListObj(0, NULL);
	while (ret == 0 && result == TCL_OK) {
		memset(&key, 0, sizeof(key));
		memset(&data, 0, sizeof(data));
		key.flags |= DB_DBT_MALLOC;
		data.flags |= DB_DBT_MALLOC;
		ret = dbc->get(dbc, &key, &data, 0);
		/*
		 * Build up our {name value} sublist
		 */
		if (ret == 0) {
			result = _SetListElem(interp, retlist,
			    key.data, key.size,
			    data.data, data.size);
			__os_ufree(dbp->env, key.data);
			__os_ufree(dbp->env, data.data);
		}
	}
	(void)dbc->close(dbc);
	if (result == TCL_OK)
		Tcl_SetObjResult(interp, retlist);
out:
	if (ktmp != NULL && freekey)
		__os_free(dbp->env, ktmp);
	while (j) {
		if (listp[j])
			(void)(listp[j])->close(listp[j]);
		j--;
	}
	__os_free(dbp->env, listp);
	return (result);
}

/*
 * tcl_DbGetFlags --
 */
static int
tcl_DbGetFlags(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	int i, ret, result;
	u_int32_t flags;
	char buf[512];
	Tcl_Obj *res;

	static const struct {
		u_int32_t flag;
		char *arg;
	} db_flags[] = {
		{ DB_CHKSUM, "-chksum" },
		{ DB_DUP, "-dup" },
		{ DB_DUPSORT, "-dupsort" },
		{ DB_ENCRYPT, "-encrypt" },
		{ DB_INORDER, "-inorder" },
		{ DB_TXN_NOT_DURABLE, "-notdurable" },
		{ DB_RECNUM, "-recnum" },
		{ DB_RENUMBER, "-renumber" },
		{ DB_REVSPLITOFF, "-revsplitoff" },
		{ DB_SNAPSHOT, "-snapshot" },
		{ 0, NULL }
	};

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return (TCL_ERROR);
	}

	ret = dbp->get_flags(dbp, &flags);
	if ((result = _ReturnSetup(
	    interp, ret, DB_RETOK_STD(ret), "db get_flags")) == TCL_OK) {
		buf[0] = '\0';

		for (i = 0; db_flags[i].flag != 0; i++)
			if (LF_ISSET(db_flags[i].flag)) {
				if (strlen(buf) > 0)
					(void)strncat(buf, " ", sizeof(buf));
				(void)strncat(
				    buf, db_flags[i].arg, sizeof(buf));
			}

		res = NewStringObj(buf, strlen(buf));
		Tcl_SetObjResult(interp, res);
	}

	return (result);
}

/*
 * tcl_DbGetOpenFlags --
 */
static int
tcl_DbGetOpenFlags(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	int i, ret, result;
	u_int32_t flags;
	char buf[512];
	Tcl_Obj *res;

	static const struct {
		u_int32_t flag;
		char *arg;
	} open_flags[] = {
		{ DB_AUTO_COMMIT,	"-auto_commit" },
		{ DB_CREATE,		"-create" },
		{ DB_EXCL,		"-excl" },
		{ DB_MULTIVERSION,	"-multiversion" },
		{ DB_NOMMAP,		"-nommap" },
		{ DB_RDONLY,		"-rdonly" },
		{ DB_READ_UNCOMMITTED,	"-read_uncommitted" },
		{ DB_THREAD,		"-thread" },
		{ DB_TRUNCATE,		"-truncate" },
		{ 0, NULL }
	};

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return (TCL_ERROR);
	}

	ret = dbp->get_open_flags(dbp, &flags);
	if ((result = _ReturnSetup(
	    interp, ret, DB_RETOK_STD(ret), "db get_open_flags")) == TCL_OK) {
		buf[0] = '\0';

		for (i = 0; open_flags[i].flag != 0; i++)
			if (LF_ISSET(open_flags[i].flag)) {
				if (strlen(buf) > 0)
					(void)strncat(buf, " ", sizeof(buf));
				(void)strncat(
				    buf, open_flags[i].arg, sizeof(buf));
			}

		res = NewStringObj(buf, strlen(buf));
		Tcl_SetObjResult(interp, res);
	}

	return (result);
}

/*
 * tcl_DbCount --
 */
static int
tcl_DbCount(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	DBC *dbc;
	DBT key, data;
	Tcl_Obj *res;
	void *ktmp;
	db_recno_t count, recno;
	int freekey, result, ret;

	res = NULL;
	count = 0;
	freekey = ret = 0;
	ktmp = NULL;
	result = TCL_OK;

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "key");
		return (TCL_ERROR);
	}

	/*
	 * Get the count for our key.
	 * We do this by getting a cursor for this DB.  Moving the cursor
	 * to the set location, and getting a count on that cursor.
	 */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	/*
	 * If it's a queue or recno database, we must make sure to
	 * treat the key as a recno rather than as a byte string.
	 */
	if (dbp->type == DB_RECNO || dbp->type == DB_QUEUE) {
		result = _GetUInt32(interp, objv[2], &recno);
		if (result == TCL_OK) {
			key.data = &recno;
			key.size = sizeof(db_recno_t);
		} else
			return (result);
	} else {
		ret = _CopyObjBytes(interp, objv[2], &ktmp,
		    &key.size, &freekey);
		if (ret != 0) {
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_STD(ret), "db count");
			return (result);
		}
		key.data = ktmp;
	}
	_debug_check();
	ret = dbp->cursor(dbp, NULL, &dbc, 0);
	if (ret != 0) {
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db cursor");
		goto out;
	}
	/*
	 * Move our cursor to the key.
	 */
	ret = dbc->get(dbc, &key, &data, DB_SET);
	if (ret == DB_KEYEMPTY || ret == DB_NOTFOUND)
		count = 0;
	else {
		ret = dbc->count(dbc, &count, 0);
		if (ret != 0) {
			result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "db c count");
			goto out;
		}
	}
	res = Tcl_NewWideIntObj((Tcl_WideInt)count);
	Tcl_SetObjResult(interp, res);

out:	if (ktmp != NULL && freekey)
		__os_free(dbp->env, ktmp);
	(void)dbc->close(dbc);
	return (result);
}

#ifdef CONFIG_TEST
/*
 * tcl_DbKeyRange --
 */
static int
tcl_DbKeyRange(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	static const char *dbkeyropts[] = {
		"-txn",
		NULL
	};
	enum dbkeyropts {
		DBKEYR_TXN
	};
	DB_TXN *txn;
	DB_KEY_RANGE range;
	DBT key;
	DBTYPE type;
	Tcl_Obj *myobjv[3], *retlist;
	void *ktmp;
	db_recno_t recno;
	u_int32_t flag;
	int freekey, i, myobjc, optindex, result, ret;
	char *arg, msg[MSG_SIZE];

	ktmp = NULL;
	flag = 0;
	freekey = 0;
	result = TCL_OK;
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-txn id? key");
		return (TCL_ERROR);
	}

	txn = NULL;
	for (i = 2; i < objc;) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbkeyropts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			if (result == TCL_OK)
				return (result);
			result = TCL_OK;
			Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum dbkeyropts)optindex) {
		case DBKEYR_TXN:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "KeyRange: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		}
	}
	if (result != TCL_OK)
		return (result);
	(void)dbp->get_type(dbp, &type);
	ret = 0;
	/*
	 * Make sure we have a key.
	 */
	if (i != (objc - 1)) {
		Tcl_WrongNumArgs(interp, 2, objv, "?args? key");
		result = TCL_ERROR;
		goto out;
	}
	memset(&key, 0, sizeof(key));
	if (type == DB_RECNO || type == DB_QUEUE) {
		result = _GetUInt32(interp, objv[i], &recno);
		if (result == TCL_OK) {
			key.data = &recno;
			key.size = sizeof(db_recno_t);
		} else
			return (result);
	} else {
		ret = _CopyObjBytes(interp, objv[i++], &ktmp,
		    &key.size, &freekey);
		if (ret != 0) {
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_STD(ret), "db keyrange");
			return (result);
		}
		key.data = ktmp;
	}
	_debug_check();
	ret = dbp->key_range(dbp, txn, &key, &range, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "db keyrange");
	if (result == TCL_ERROR)
		goto out;

	/*
	 * If we succeeded, set up return list.
	 */
	myobjc = 3;
	myobjv[0] = Tcl_NewDoubleObj(range.less);
	myobjv[1] = Tcl_NewDoubleObj(range.equal);
	myobjv[2] = Tcl_NewDoubleObj(range.greater);
	retlist = Tcl_NewListObj(myobjc, myobjv);
	if (result == TCL_OK)
		Tcl_SetObjResult(interp, retlist);

out:	if (ktmp != NULL && freekey)
		__os_free(dbp->env, ktmp);
	return (result);
}
#endif

/*
 * tcl_DbTruncate --
 */
static int
tcl_DbTruncate(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	static const char *dbcuropts[] = {
		"-txn",
		NULL
	};
	enum dbcuropts {
		DBTRUNC_TXN
	};
	DB_TXN *txn;
	Tcl_Obj *res;
	u_int32_t count;
	int i, optindex, result, ret;
	char *arg, msg[MSG_SIZE];

	txn = NULL;
	result = TCL_OK;

	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbcuropts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			goto out;
		}
		i++;
		switch ((enum dbcuropts)optindex) {
		case DBTRUNC_TXN:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "Truncate: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		}
		if (result != TCL_OK)
			break;
	}
	if (result != TCL_OK)
		goto out;

	_debug_check();
	ret = dbp->truncate(dbp, txn, &count, 0);
	if (ret != 0)
		result = _ErrorSetup(interp, ret, "db truncate");

	else {
		res = Tcl_NewWideIntObj((Tcl_WideInt)count);
		Tcl_SetObjResult(interp, res);
	}
out:
	return (result);
}

#ifdef CONFIG_TEST
/*
 * tcl_DbCompact --
 */
static int
tcl_DbCompact(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	static const char *dbcuropts[] = {
		"-fillpercent",
		"-freespace",
		"-freeonly",
		"-pages",
		"-start",
		"-stop",
		"-timeout",
		"-txn",
		NULL
	};
	enum dbcuropts {
		DBREORG_FILLFACTOR,
		DBREORG_FREESPACE,
		DBREORG_FREEONLY,
		DBREORG_PAGES,
		DBREORG_START,
		DBREORG_STOP,
		DBREORG_TIMEOUT,
		DBREORG_TXN
	};
	DBTCL_INFO *ip;
	DBT *key, end, start, stop;
	DBTYPE type;
	DB_TXN *txn;
	Tcl_Obj *myobj, *retlist;
	db_recno_t recno, srecno;
	u_int32_t arg, fillfactor, flags, pages, timeout;
	char *carg, msg[MSG_SIZE];
	int freekey, i, optindex, result, ret;
	void *kp;

	flags = 0;
	result = TCL_OK;
	txn = NULL;
	(void)dbp->get_type(dbp, &type);
	memset(&start, 0, sizeof(start));
	memset(&stop, 0, sizeof(stop));
	memset(&end, 0, sizeof(end));
	ip = (DBTCL_INFO *)dbp->api_internal;
	fillfactor = pages = timeout = 0;

	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbcuropts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			goto out;
		}
		i++;
		switch ((enum dbcuropts)optindex) {
		case DBREORG_FILLFACTOR:
			if (i == objc) {
				Tcl_WrongNumArgs(interp,
				    2, objv, "?-fillfactor number?");
				result = TCL_ERROR;
				break;
			}
			result = _GetUInt32(interp, objv[i++], &arg);
			if (result != TCL_OK)
				goto out;
			i++;
			fillfactor = arg;
			break;
		case DBREORG_FREESPACE:
			LF_SET(DB_FREE_SPACE);
			break;

		case DBREORG_FREEONLY:
			LF_SET(DB_FREELIST_ONLY);
			break;

		case DBREORG_PAGES:
			if (i == objc) {
				Tcl_WrongNumArgs(interp,
				    2, objv, "?-pages number?");
				result = TCL_ERROR;
				break;
			}
			result = _GetUInt32(interp, objv[i++], &arg);
			if (result != TCL_OK)
				goto out;
			i++;
			pages = arg;
			break;
		case DBREORG_TIMEOUT:
			if (i == objc) {
				Tcl_WrongNumArgs(interp,
				    2, objv, "?-timeout number?");
				result = TCL_ERROR;
				break;
			}
			result = _GetUInt32(interp, objv[i++], &arg);
			if (result != TCL_OK)
				goto out;
			i++;
			timeout = arg;
			break;

		case DBREORG_START:
		case DBREORG_STOP:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 1, objv,
				    "?-args? -start/stop key");
				result = TCL_ERROR;
				goto out;
			}
			if ((enum dbcuropts)optindex == DBREORG_START) {
				key = &start;
				key->data = &recno;
			} else {
				key = &stop;
				key->data = &srecno;
			}
			if (type == DB_RECNO || type == DB_QUEUE) {
				result = _GetUInt32(
				    interp, objv[i], key->data);
				if (result == TCL_OK) {
					key->size = sizeof(db_recno_t);
				} else
					goto out;
			} else {
				ret = _CopyObjBytes(interp, objv[i],
				    &key->data, &key->size, &freekey);
				if (ret != 0)
					goto err;
				if (freekey == 0) {
					if ((ret = __os_malloc(NULL,
					     key->size, &kp)) != 0)
						goto err;

					memcpy(kp, key->data, key->size);
					key->data = kp;
					key->ulen = key->size;
				}
			}
			i++;
			break;
		case DBREORG_TXN:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			carg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(carg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "Compact: Invalid txn: %s\n", carg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
		}
		if (result != TCL_OK)
			break;
	}
	if (result != TCL_OK)
		goto out;

	if (ip->i_cdata == NULL)
		if ((ret = __os_calloc(dbp->env,
		    1, sizeof(DB_COMPACT), &ip->i_cdata)) != 0) {
			Tcl_SetResult(interp,
			    db_strerror(ret), TCL_STATIC);
			goto out;
		}

	ip->i_cdata->compact_fillpercent = fillfactor;
	ip->i_cdata->compact_timeout = timeout;
	ip->i_cdata->compact_pages = pages;

	_debug_check();
	ret = dbp->compact(dbp, txn, &start, &stop, ip->i_cdata, flags, &end);
	result = _ReturnSetup(interp, ret, DB_RETOK_DBCGET(ret), "dbp compact");
	if (result == TCL_ERROR)
		goto out;

	retlist = Tcl_NewListObj(0, NULL);
	if (ret != 0)
		goto out;
	if (type == DB_RECNO || type == DB_QUEUE) {
		if (end.size == 0)
			recno  = 0;
		else
			recno = *((db_recno_t *)end.data);
		myobj = Tcl_NewWideIntObj((Tcl_WideInt)recno);
	} else
		myobj = Tcl_NewByteArrayObj(end.data, (int)end.size);
	result = Tcl_ListObjAppendElement(interp, retlist, myobj);
	if (result == TCL_OK)
		Tcl_SetObjResult(interp, retlist);

	if (0) {
err:		result = _ReturnSetup(interp,
		     ret, DB_RETOK_DBCGET(ret), "dbc compact");
	}
out:
	if (start.data != NULL && start.data != &recno)
		__os_free(NULL, start.data);
	if (stop.data != NULL && stop.data != &srecno)
		__os_free(NULL, stop.data);
	if (end.data != NULL)
		__os_free(NULL, end.data);

	return (result);
}

/*
 * tcl_DbCompactStat
 */
static int
tcl_DbCompactStat(interp, objc, objv, dbp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB *dbp;			/* Database pointer */
{
	DBTCL_INFO *ip;

	COMPQUIET(objc, 0);
	COMPQUIET(objv, NULL);

	ip = (DBTCL_INFO *)dbp->api_internal;

	return (tcl_CompactStat(interp, ip));
}

/*
 * PUBLIC: int tcl_CompactStat __P((Tcl_Interp *, DBTCL_INFO *));
 */
int
tcl_CompactStat(interp, ip)
	Tcl_Interp *interp;		/* Interpreter */
	DBTCL_INFO *ip;
{
	DB_COMPACT *rp;
	Tcl_Obj *res;
	int result;
	char msg[MSG_SIZE];

	result = TCL_OK;
	rp = NULL;

	_debug_check();
	if ((rp = ip->i_cdata) == NULL) {
		snprintf(msg, MSG_SIZE,
		    "Compact stat: No stats available\n");
		Tcl_SetResult(interp, msg, TCL_VOLATILE);
		result = TCL_ERROR;
		goto error;
	}

	res = Tcl_NewObj();

	MAKE_STAT_LIST("Pages freed", rp->compact_pages_free);
	MAKE_STAT_LIST("Pages truncated", rp->compact_pages_truncated);
	MAKE_STAT_LIST("Pages examined", rp->compact_pages_examine);
	MAKE_STAT_LIST("Levels removed", rp->compact_levels);
	MAKE_STAT_LIST("Deadlocks encountered", rp->compact_deadlock);

	Tcl_SetObjResult(interp, res);
error:
	return (result);
}
#endif
