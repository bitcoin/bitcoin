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
#include "dbinc/tcl_db.h"

static int tcl_TxnCommit __P((Tcl_Interp *,
	       int, Tcl_Obj * CONST *, DB_TXN *, DBTCL_INFO *));
static int txn_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST *));

/*
 * _TxnInfoDelete --
 *	Removes nested txn info structures that are children
 *	of this txn.
 *	RECURSIVE:  Transactions can be arbitrarily nested, so we
 *	must recurse down until we get them all.
 *
 * PUBLIC: void _TxnInfoDelete __P((Tcl_Interp *, DBTCL_INFO *));
 */
void
_TxnInfoDelete(interp, txnip)
	Tcl_Interp *interp;		/* Interpreter */
	DBTCL_INFO *txnip;		/* Info for txn */
{
	DBTCL_INFO *nextp, *p;

	for (p = LIST_FIRST(&__db_infohead); p != NULL; p = nextp) {
		/*
		 * Check if this info structure "belongs" to this
		 * txn.  Remove its commands and info structure.
		 */
		nextp = LIST_NEXT(p, entries);
		if (p->i_parent == txnip && p->i_type == I_TXN) {
			_TxnInfoDelete(interp, p);
			(void)Tcl_DeleteCommand(interp, p->i_name);
			_DeleteInfo(p);
		}
	}
}

/*
 * tcl_TxnCheckpoint --
 *
 * PUBLIC: int tcl_TxnCheckpoint __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_TxnCheckpoint(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	static const char *txnckpopts[] = {
		"-force",
		"-kbyte",
		"-min",
		NULL
	};
	enum txnckpopts {
		TXNCKP_FORCE,
		TXNCKP_KB,
		TXNCKP_MIN
	};
	u_int32_t flags;
	int i, kb, min, optindex, result, ret;

	result = TCL_OK;
	flags = 0;
	kb = min = 0;

	/*
	 * Get the flag index from the object based on the options
	 * defined above.
	 */
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i],
		    txnckpopts, "option", TCL_EXACT, &optindex) != TCL_OK) {
			return (IS_HELP(objv[i]));
		}
		i++;
		switch ((enum txnckpopts)optindex) {
		case TXNCKP_FORCE:
			flags = DB_FORCE;
			break;
		case TXNCKP_KB:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-kbyte kb?");
				result = TCL_ERROR;
				break;
			}
			result = Tcl_GetIntFromObj(interp, objv[i++], &kb);
			break;
		case TXNCKP_MIN:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-min min?");
				result = TCL_ERROR;
				break;
			}
			result = Tcl_GetIntFromObj(interp, objv[i++], &min);
			break;
		}
	}
	_debug_check();
	ret = dbenv->txn_checkpoint(dbenv, (u_int32_t)kb, (u_int32_t)min,
	    flags);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "txn checkpoint");
	return (result);
}

/*
 * tcl_Txn --
 *
 * PUBLIC: int tcl_Txn __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *, DBTCL_INFO *));
 */
int
tcl_Txn(interp, objc, objv, dbenv, envip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
	DBTCL_INFO *envip;		/* Info pointer */
{
	static const char *txnopts[] = {
#ifdef CONFIG_TEST
		"-lock_timeout",
		"-read_committed",
		"-read_uncommitted",
		"-txn_timeout",
		"-txn_wait",
#endif
		"-nosync",
		"-nowait",
		"-parent",
		"-snapshot",
		"-sync",
		"-wrnosync",
		NULL
	};
	enum txnopts {
#ifdef CONFIG_TEST
		TXNLOCK_TIMEOUT,
		TXNREAD_COMMITTED,
		TXNREAD_UNCOMMITTED,
		TXNTIMEOUT,
		TXNWAIT,
#endif
		TXNNOSYNC,
		TXNNOWAIT,
		TXNPARENT,
		TXNSNAPSHOT,
		TXNSYNC,
		TXNWRNOSYNC
	};
	DBTCL_INFO *ip;
	DB_TXN *parent;
	DB_TXN *txn;
	Tcl_Obj *res;
	u_int32_t flag;
	int i, optindex, result, ret;
	char *arg, msg[MSG_SIZE], newname[MSG_SIZE];
#ifdef CONFIG_TEST
	db_timeout_t lk_time, tx_time;
	u_int32_t lk_timeflag, tx_timeflag;
#endif

	result = TCL_OK;
	memset(newname, 0, MSG_SIZE);

	parent = NULL;
	flag = 0;
#ifdef CONFIG_TEST
	COMPQUIET(tx_time, 0);
	COMPQUIET(lk_time, 0);
	lk_timeflag = tx_timeflag = 0;
#endif
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i],
		    txnopts, "option", TCL_EXACT, &optindex) != TCL_OK) {
			return (IS_HELP(objv[i]));
		}
		i++;
		switch ((enum txnopts)optindex) {
#ifdef CONFIG_TEST
		case TXNLOCK_TIMEOUT:
			lk_timeflag = DB_SET_LOCK_TIMEOUT;
			goto get_timeout;
		case TXNTIMEOUT:
			tx_timeflag = DB_SET_TXN_TIMEOUT;
get_timeout:		if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-txn_timestamp time?");
				return (TCL_ERROR);
			}
			result = Tcl_GetLongFromObj(interp, objv[i++], (long *)
			    ((enum txnopts)optindex == TXNLOCK_TIMEOUT ?
			    &lk_time : &tx_time));
			if (result != TCL_OK)
				return (TCL_ERROR);
			break;
		case TXNREAD_COMMITTED:
			flag |= DB_READ_COMMITTED;
			break;
		case TXNREAD_UNCOMMITTED:
			flag |= DB_READ_UNCOMMITTED;
			break;
		case TXNWAIT:
			flag |= DB_TXN_WAIT;
			break;
#endif
		case TXNNOSYNC:
			flag |= DB_TXN_NOSYNC;
			break;
		case TXNNOWAIT:
			flag |= DB_TXN_NOWAIT;
			break;
		case TXNPARENT:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-parent txn?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			parent = NAME_TO_TXN(arg);
			if (parent == NULL) {
				snprintf(msg, MSG_SIZE,
				    "Invalid parent txn: %s\n",
				    arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				return (TCL_ERROR);
			}
			break;
		case TXNSNAPSHOT:
			flag |= DB_TXN_SNAPSHOT;
			break;
		case TXNSYNC:
			flag |= DB_TXN_SYNC;
			break;
		case TXNWRNOSYNC:
			flag |= DB_TXN_WRITE_NOSYNC;
			break;
		}
	}
	snprintf(newname, sizeof(newname), "%s.txn%d",
	    envip->i_name, envip->i_envtxnid);
	ip = _NewInfo(interp, NULL, newname, I_TXN);
	if (ip == NULL) {
		Tcl_SetResult(interp, "Could not set up info",
		    TCL_STATIC);
		return (TCL_ERROR);
	}
	_debug_check();
	ret = dbenv->txn_begin(dbenv, parent, &txn, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "txn");
	if (result == TCL_ERROR)
		_DeleteInfo(ip);
	else {
		/*
		 * Success.  Set up return.  Set up new info
		 * and command widget for this txn.
		 */
		envip->i_envtxnid++;
		if (parent)
			ip->i_parent = _PtrToInfo(parent);
		else
			ip->i_parent = envip;
		_SetInfoData(ip, txn);
		(void)Tcl_CreateObjCommand(interp, newname,
		    (Tcl_ObjCmdProc *)txn_Cmd, (ClientData)txn, NULL);
		res = NewStringObj(newname, strlen(newname));
		Tcl_SetObjResult(interp, res);
#ifdef CONFIG_TEST
		if (tx_timeflag != 0) {
			ret = txn->set_timeout(txn, tx_time, tx_timeflag);
			if (ret != 0) {
				result =
				    _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
					"set_timeout");
				_DeleteInfo(ip);
			}
		}
		if (lk_timeflag != 0) {
			ret = txn->set_timeout(txn, lk_time, lk_timeflag);
			if (ret != 0) {
				result =
				    _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
					"set_timeout");
				_DeleteInfo(ip);
			}
		}
#endif
	}
	return (result);
}

/*
 * tcl_CDSGroup --
 *
 * PUBLIC: int tcl_CDSGroup __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *, DBTCL_INFO *));
 */
int
tcl_CDSGroup(interp, objc, objv, dbenv, envip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
	DBTCL_INFO *envip;		/* Info pointer */
{
	DBTCL_INFO *ip;
	DB_TXN *txn;
	Tcl_Obj *res;
	int result, ret;
	char newname[MSG_SIZE];

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "env cdsgroup");
		return (TCL_ERROR);
	}

	result = TCL_OK;
	memset(newname, 0, MSG_SIZE);

	snprintf(newname, sizeof(newname), "%s.txn%d",
	    envip->i_name, envip->i_envtxnid);
	ip = _NewInfo(interp, NULL, newname, I_TXN);
	if (ip == NULL) {
		Tcl_SetResult(interp, "Could not set up info",
		    TCL_STATIC);
		return (TCL_ERROR);
	}
	_debug_check();
	ret = dbenv->cdsgroup_begin(dbenv, &txn);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "cdsgroup");
	if (result == TCL_ERROR)
		_DeleteInfo(ip);
	else {
		/*
		 * Success.  Set up return.  Set up new info
		 * and command widget for this txn.
		 */
		envip->i_envtxnid++;
		ip->i_parent = envip;
		_SetInfoData(ip, txn);
		(void)Tcl_CreateObjCommand(interp, newname,
		    (Tcl_ObjCmdProc *)txn_Cmd, (ClientData)txn, NULL);
		res = NewStringObj(newname, strlen(newname));
		Tcl_SetObjResult(interp, res);
	}
	return (result);
}

/*
 * tcl_TxnStat --
 *
 * PUBLIC: int tcl_TxnStat __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_TxnStat(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	DBTCL_INFO *ip;
	DB_TXN_ACTIVE *p;
	DB_TXN_STAT *sp;
	Tcl_Obj *myobjv[2], *res, *thislist, *lsnlist;
	u_int32_t i;
	int myobjc, result, ret;

	result = TCL_OK;
	/*
	 * No args for this.  Error if there are some.
	 */
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return (TCL_ERROR);
	}
	_debug_check();
	ret = dbenv->txn_stat(dbenv, &sp, 0);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "txn stat");
	if (result == TCL_ERROR)
		return (result);

	/*
	 * Have our stats, now construct the name value
	 * list pairs and free up the memory.
	 */
	res = Tcl_NewObj();
	/*
	 * MAKE_STAT_LIST assumes 'res' and 'error' label.
	 */
#ifdef HAVE_STATISTICS
	MAKE_STAT_LIST("Region size", sp->st_regsize);
	MAKE_STAT_LSN("LSN of last checkpoint", &sp->st_last_ckp);
	MAKE_STAT_LIST("Time of last checkpoint", sp->st_time_ckp);
	MAKE_STAT_LIST("Last txn ID allocated", sp->st_last_txnid);
	MAKE_STAT_LIST("Maximum txns", sp->st_maxtxns);
	MAKE_WSTAT_LIST("Number aborted txns", sp->st_naborts);
	MAKE_WSTAT_LIST("Number txns begun", sp->st_nbegins);
	MAKE_WSTAT_LIST("Number committed txns", sp->st_ncommits);
	MAKE_STAT_LIST("Number active txns", sp->st_nactive);
	MAKE_STAT_LIST("Number of snapshot txns", sp->st_nsnapshot);
	MAKE_STAT_LIST("Number restored txns", sp->st_nrestores);
	MAKE_STAT_LIST("Maximum active txns", sp->st_maxnactive);
	MAKE_STAT_LIST("Maximum snapshot txns", sp->st_maxnsnapshot);
	MAKE_WSTAT_LIST("Number of region lock waits", sp->st_region_wait);
	MAKE_WSTAT_LIST("Number of region lock nowaits", sp->st_region_nowait);
	for (i = 0, p = sp->st_txnarray; i < sp->st_nactive; i++, p++)
		LIST_FOREACH(ip, &__db_infohead, entries) {
			if (ip->i_type != I_TXN)
				continue;
			if (ip->i_type == I_TXN &&
			    (ip->i_txnp->id(ip->i_txnp) == p->txnid)) {
				MAKE_STAT_LSN(ip->i_name, &p->lsn);
				if (p->parentid != 0)
					MAKE_STAT_STRLIST("Parent",
					    ip->i_parent->i_name);
				else
					MAKE_STAT_LIST("Parent", 0);
				break;
			}
		}
#endif
	Tcl_SetObjResult(interp, res);
error:
	__os_ufree(dbenv->env, sp);
	return (result);
}

/*
 * tcl_TxnTimeout --
 *
 * PUBLIC: int tcl_TxnTimeout __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_TxnTimeout(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	long timeout;
	int result, ret;

	/*
	 * One arg, the timeout.
	 */
	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?timeout?");
		return (TCL_ERROR);
	}
	result = Tcl_GetLongFromObj(interp, objv[2], &timeout);
	if (result != TCL_OK)
		return (result);
	_debug_check();
	ret = dbenv->set_timeout(dbenv, (u_int32_t)timeout, DB_SET_TXN_TIMEOUT);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "lock timeout");
	return (result);
}

/*
 * txn_Cmd --
 *	Implements the "txn" widget.
 */
static int
txn_Cmd(clientData, interp, objc, objv)
	ClientData clientData;		/* Txn handle */
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *txncmds[] = {
#ifdef CONFIG_TEST
		"discard",
		"getname",
		"id",
		"prepare",
		"setname",
#endif
		"abort",
		"commit",
		"getname",
		"setname",
		NULL
	};
	enum txncmds {
#ifdef CONFIG_TEST
		TXNDISCARD,
		TXNGETNAME,
		TXNID,
		TXNPREPARE,
		TXNSETNAME,
#endif
		TXNABORT,
		TXNCOMMIT
	};
	DBTCL_INFO *txnip;
	DB_TXN *txnp;
	Tcl_Obj *res;
	int cmdindex, result, ret;
#ifdef CONFIG_TEST
	u_int8_t *gid, garray[DB_GID_SIZE];
	int length;
	const char *name;
#endif

	Tcl_ResetResult(interp);
	txnp = (DB_TXN *)clientData;
	txnip = _PtrToInfo((void *)txnp);
	result = TCL_OK;
	if (txnp == NULL) {
		Tcl_SetResult(interp, "NULL txn pointer", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (txnip == NULL) {
		Tcl_SetResult(interp, "NULL txn info pointer", TCL_STATIC);
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the dbcmds
	 * defined above.
	 */
	if (Tcl_GetIndexFromObj(interp,
	    objv[1], txncmds, "command", TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));

	res = NULL;
	switch ((enum txncmds)cmdindex) {
#ifdef CONFIG_TEST
	case TXNDISCARD:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = txnp->discard(txnp, 0);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "txn discard");
		_TxnInfoDelete(interp, txnip);
		(void)Tcl_DeleteCommand(interp, txnip->i_name);
		_DeleteInfo(txnip);
		break;
	case TXNID:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		res = Tcl_NewIntObj((int)txnp->id(txnp));
		break;
	case TXNPREPARE:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		gid = (u_int8_t *)Tcl_GetByteArrayFromObj(objv[2], &length);
		memcpy(garray, gid, (size_t)length);
		ret = txnp->prepare(txnp, garray);
		/*
		 * !!!
		 * DB_TXN->prepare commits all outstanding children.  But it
		 * does NOT destroy the current txn handle.  So, we must call
		 * _TxnInfoDelete to recursively remove all nested txn handles,
		 * we do not call _DeleteInfo on ourselves.
		 */
		_TxnInfoDelete(interp, txnip);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "txn prepare");
		break;
	case TXNGETNAME:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = txnp->get_name(txnp, &name);
		if ((result = _ReturnSetup(
		    interp, ret, DB_RETOK_STD(ret), "txn getname")) == TCL_OK)
			res = NewStringObj(name, strlen(name));
		break;
	case TXNSETNAME:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "name");
			return (TCL_ERROR);
		}
		_debug_check();
		ret = txnp->set_name(txnp, Tcl_GetStringFromObj(objv[2], NULL));
		result =
		    _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "setname");
		break;
#endif
	case TXNABORT:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = txnp->abort(txnp);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "txn abort");
		_TxnInfoDelete(interp, txnip);
		(void)Tcl_DeleteCommand(interp, txnip->i_name);
		_DeleteInfo(txnip);
		break;
	case TXNCOMMIT:
		result = tcl_TxnCommit(interp, objc, objv, txnp, txnip);
		_TxnInfoDelete(interp, txnip);
		(void)Tcl_DeleteCommand(interp, txnip->i_name);
		_DeleteInfo(txnip);
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

static int
tcl_TxnCommit(interp, objc, objv, txnp, txnip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_TXN *txnp;			/* Transaction pointer */
	DBTCL_INFO *txnip;		/* Info pointer */
{
	static const char *commitopt[] = {
		"-nosync",
		"-sync",
		"-wrnosync",
		NULL
	};
	enum commitopt {
		COMNOSYNC,
		COMSYNC,
		COMWRNOSYNC
	};
	u_int32_t flag;
	int optindex, result, ret;

	COMPQUIET(txnip, NULL);

	result = TCL_OK;
	flag = 0;
	if (objc != 2 && objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return (TCL_ERROR);
	}
	if (objc == 3) {
		if (Tcl_GetIndexFromObj(interp, objv[2], commitopt,
		    "option", TCL_EXACT, &optindex) != TCL_OK)
			return (IS_HELP(objv[2]));
		switch ((enum commitopt)optindex) {
		case COMSYNC:
			flag = DB_TXN_SYNC;
			break;
		case COMNOSYNC:
			flag = DB_TXN_NOSYNC;
			break;
		case COMWRNOSYNC:
			flag = DB_TXN_WRITE_NOSYNC;
			break;
		}
	}

	_debug_check();
	ret = txnp->commit(txnp, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "txn commit");
	return (result);
}

#ifdef CONFIG_TEST
/*
 * tcl_TxnRecover --
 *
 * PUBLIC: int tcl_TxnRecover __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *, DBTCL_INFO *));
 */
int
tcl_TxnRecover(interp, objc, objv, dbenv, envip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
	DBTCL_INFO *envip;		/* Info pointer */
{
#define	DO_PREPLIST(count)						\
for (i = 0; i < count; i++) {						\
	snprintf(newname, sizeof(newname), "%s.txn%d",			\
	    envip->i_name, envip->i_envtxnid);				\
	ip = _NewInfo(interp, NULL, newname, I_TXN);			\
	if (ip == NULL) {						\
		Tcl_SetResult(interp, "Could not set up info",		\
		    TCL_STATIC);					\
		return (TCL_ERROR);					\
	}								\
	envip->i_envtxnid++;						\
	ip->i_parent = envip;						\
	p = &prep[i];							\
	_SetInfoData(ip, p->txn);					\
	(void)Tcl_CreateObjCommand(interp, newname,			\
	    (Tcl_ObjCmdProc *)txn_Cmd, (ClientData)p->txn, NULL);	\
	result = _SetListElem(interp, res, newname,			\
	    (u_int32_t)strlen(newname), p->gid, DB_GID_SIZE);		\
	if (result != TCL_OK)						\
		goto error;						\
}

	DBTCL_INFO *ip;
	DB_PREPLIST prep[DBTCL_PREP], *p;
	Tcl_Obj *res;
	u_int32_t count, i;
	int result, ret;
	char newname[MSG_SIZE];

	result = TCL_OK;
	/*
	 * No args for this.  Error if there are some.
	 */
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return (TCL_ERROR);
	}
	_debug_check();
	ret = dbenv->txn_recover(dbenv, prep, DBTCL_PREP, &count, DB_FIRST);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "txn recover");
	if (result == TCL_ERROR)
		return (result);
	res = Tcl_NewObj();
	DO_PREPLIST(count);

	/*
	 * If count returned is the maximum size we have, then there
	 * might be more.  Keep going until we get them all.
	 */
	while (count == DBTCL_PREP) {
		ret = dbenv->txn_recover(
		    dbenv, prep, DBTCL_PREP, &count, DB_NEXT);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "txn recover");
		if (result == TCL_ERROR)
			return (result);
		DO_PREPLIST(count);
	}
	Tcl_SetObjResult(interp, res);
error:
	return (result);
}
#endif
