/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"
#ifdef HAVE_64BIT_TYPES

#include "db_int.h"
#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <tcl.h>
#endif
#include "dbinc/tcl_db.h"
#include "dbinc_auto/sequence_ext.h"

/*
 * Prototypes for procedures defined later in this file:
 */
static int	tcl_SeqClose __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB_SEQUENCE *, DBTCL_INFO *));
static int	tcl_SeqGet __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB_SEQUENCE *));
static int	tcl_SeqRemove __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB_SEQUENCE *, DBTCL_INFO *));
static int	tcl_SeqStat __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB_SEQUENCE *));
static int	tcl_SeqGetFlags __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB_SEQUENCE *));

/*
 *
 * PUBLIC: int seq_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
 *
 * seq_Cmd --
 *	Implements the "seq" widget.
 */
int
seq_Cmd(clientData, interp, objc, objv)
	ClientData clientData;		/* SEQ handle */
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *seqcmds[] = {
		"close",
		"get",
		"get_cachesize",
		"get_db",
		"get_flags",
		"get_key",
		"get_range",
		"remove",
		"stat",
		NULL
	};
	enum seqcmds {
		SEQCLOSE,
		SEQGET,
		SEQGETCACHESIZE,
		SEQGETDB,
		SEQGETFLAGS,
		SEQGETKEY,
		SEQGETRANGE,
		SEQREMOVE,
		SEQSTAT
	};
	DB *dbp;
	DBT key;
	DBTCL_INFO *dbip, *ip;
	DB_SEQUENCE *seq;
	Tcl_Obj *myobjv[2], *res;
	db_seq_t min, max;
	int cmdindex, ncache, result, ret;

	Tcl_ResetResult(interp);
	seq = (DB_SEQUENCE *)clientData;
	result = TCL_OK;
	dbip = NULL;
	if (objc <= 1) {
		Tcl_WrongNumArgs(interp, 1, objv, "command cmdargs");
		return (TCL_ERROR);
	}
	if (seq == NULL) {
		Tcl_SetResult(interp, "NULL sequence pointer", TCL_STATIC);
		return (TCL_ERROR);
	}

	ip = _PtrToInfo((void *)seq);
	if (ip == NULL) {
		Tcl_SetResult(interp, "NULL info pointer", TCL_STATIC);
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the dbcmds
	 * defined above.
	 */
	if (Tcl_GetIndexFromObj(interp,
	    objv[1], seqcmds, "command", TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));

	res = NULL;
	switch ((enum seqcmds)cmdindex) {
	case SEQGETRANGE:
		ret = seq->get_range(seq, &min, &max);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "sequence get_range")) == TCL_OK) {
			myobjv[0] = Tcl_NewWideIntObj(min);
			myobjv[1] = Tcl_NewWideIntObj(max);
			res = Tcl_NewListObj(2, myobjv);
		}
		break;
	case SEQCLOSE:
		result = tcl_SeqClose(interp, objc, objv, seq, ip);
		break;
	case SEQREMOVE:
		result = tcl_SeqRemove(interp, objc, objv, seq, ip);
		break;
	case SEQGET:
		result = tcl_SeqGet(interp, objc, objv, seq);
		break;
	case SEQSTAT:
		result = tcl_SeqStat(interp, objc, objv, seq);
		break;
	case SEQGETCACHESIZE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = seq->get_cachesize(seq, &ncache);
		if ((result = _ReturnSetup(interp, ret,
		    DB_RETOK_STD(ret), "sequence get_cachesize")) == TCL_OK)
			res = Tcl_NewIntObj(ncache);
		break;
	case SEQGETDB:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = seq->get_db(seq, &dbp);
		if (ret == 0 && (dbip = _PtrToInfo((void *)dbp)) == NULL) {
			Tcl_SetResult(interp,
			     "NULL db info pointer", TCL_STATIC);
			return (TCL_ERROR);
		}

		if ((result = _ReturnSetup(interp, ret,
		    DB_RETOK_STD(ret), "sequence get_db")) == TCL_OK)
			res = NewStringObj(dbip->i_name, strlen(dbip->i_name));
		break;
	case SEQGETKEY:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = seq->get_key(seq, &key);
		if ((result = _ReturnSetup(interp, ret,
		    DB_RETOK_STD(ret), "sequence get_key")) == TCL_OK)
			res = Tcl_NewByteArrayObj(
			    (u_char *)key.data, (int)key.size);
		break;
	case SEQGETFLAGS:
		result = tcl_SeqGetFlags(interp, objc, objv, seq);
		break;
	}

	/*
	 * Only set result if we have a res.  Otherwise, lower functions have
	 * already done so.
	 */
	if (result == TCL_OK && res)
		Tcl_SetObjResult(interp, res);
	return (result);
}

/*
 * tcl_db_stat --
 */
static int
tcl_SeqStat(interp, objc, objv, seq)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_SEQUENCE *seq;		/* Database pointer */
{
	DB_SEQUENCE_STAT *sp;
	u_int32_t flag;
	Tcl_Obj *res, *flaglist, *myobjv[2];
	int result, ret;
	char *arg;

	result = TCL_OK;
	flag = 0;

	if (objc > 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-clear?");
		return (TCL_ERROR);
	}

	if (objc == 3) {
		arg = Tcl_GetStringFromObj(objv[2], NULL);
		if (strcmp(arg, "-clear") == 0)
			flag = DB_STAT_CLEAR;
		else {
			Tcl_SetResult(interp,
			    "db stat: unknown arg", TCL_STATIC);
			return (TCL_ERROR);
		}
	}

	_debug_check();
	ret = seq->stat(seq, &sp, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "db stat");
	if (result == TCL_ERROR)
		return (result);

	res = Tcl_NewObj();
	MAKE_WSTAT_LIST("Wait", sp->st_wait);
	MAKE_WSTAT_LIST("No wait", sp->st_nowait);
	MAKE_WSTAT_LIST("Current", sp->st_current);
	MAKE_WSTAT_LIST("Cached", sp->st_value);
	MAKE_WSTAT_LIST("Max Cached", sp->st_last_value);
	MAKE_WSTAT_LIST("Min", sp->st_min);
	MAKE_WSTAT_LIST("Max", sp->st_max);
	MAKE_STAT_LIST("Cache size", sp->st_cache_size);
	/*
	 * Construct a {name {flag1 flag2 ... flagN}} list for the
	 * seq flags.
	 */
	myobjv[0] = NewStringObj("Flags", strlen("Flags"));
	myobjv[1] =
	    _GetFlagsList(interp, sp->st_flags, __db_get_seq_flags_fn());
	flaglist = Tcl_NewListObj(2, myobjv);
	if (flaglist == NULL) {
		result = TCL_ERROR;
		goto error;
	}
	if ((result =
	    Tcl_ListObjAppendElement(interp, res, flaglist)) != TCL_OK)
		goto error;

	Tcl_SetObjResult(interp, res);

error:	__os_ufree(seq->seq_dbp->env, sp);
	return (result);
}

/*
 * tcl_db_close --
 */
static int
tcl_SeqClose(interp, objc, objv, seq, ip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_SEQUENCE *seq;		/* Database pointer */
	DBTCL_INFO *ip;			/* Info pointer */
{
	int result, ret;

	result = TCL_OK;
	if (objc > 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "");
		return (TCL_ERROR);
	}

	_DeleteInfo(ip);
	_debug_check();

	ret = seq->close(seq, 0);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "sequence close");
	return (result);
}

/*
 * tcl_SeqGet --
 */
static int
tcl_SeqGet(interp, objc, objv, seq)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_SEQUENCE *seq;		/* Sequence pointer */
{
	static const char *seqgetopts[] = {
		"-nosync",
		"-txn",
		NULL
	};
	enum seqgetopts {
		SEQGET_NOSYNC,
		SEQGET_TXN
	};
	DB_TXN *txn;
	Tcl_Obj *res;
	db_seq_t value;
	u_int32_t aflag, delta;
	int i, end, optindex, result, ret;
	char *arg, msg[MSG_SIZE];

	result = TCL_OK;
	txn = NULL;
	aflag = 0;

	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-args? delta");
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the options
	 * defined above.
	 */
	i = 2;
	end = objc;
	while (i < end) {
		if (Tcl_GetIndexFromObj(interp, objv[i], seqgetopts, "option",
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
		switch ((enum seqgetopts)optindex) {
		case SEQGET_NOSYNC:
			aflag |= DB_TXN_NOSYNC;
			break;
		case SEQGET_TXN:
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
		} /* switch */
		if (result != TCL_OK)
			break;
	}
	if (result != TCL_OK)
		goto out;

	if (i != objc - 1) {
		Tcl_SetResult(interp,
		    "Wrong number of key/data given\n", TCL_STATIC);
		result = TCL_ERROR;
		goto out;
	}

	if ((result = _GetUInt32(interp, objv[objc - 1], &delta)) != TCL_OK)
		goto out;

	ret = seq->get(seq, txn, (int32_t)delta, &value, aflag);
	result = _ReturnSetup(interp, ret, DB_RETOK_DBGET(ret), "sequence get");
	if (ret == 0) {
		res = Tcl_NewWideIntObj((Tcl_WideInt)value);
		Tcl_SetObjResult(interp, res);
	}
out:
	return (result);
}
/*
 */
static int
tcl_SeqRemove(interp, objc, objv, seq, ip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_SEQUENCE *seq;		/* Sequence pointer */
	DBTCL_INFO *ip;			/* Info pointer */
{
	static const char *seqgetopts[] = {
		"-nosync",
		"-txn",
		NULL
	};
	enum seqgetopts {
		SEQGET_NOSYNC,
		SEQGET_TXN
	};
	DB_TXN *txn;
	u_int32_t aflag;
	int i, end, optindex, result, ret;
	char *arg, msg[MSG_SIZE];

	result = TCL_OK;
	txn = NULL;
	aflag = 0;

	_DeleteInfo(ip);

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-args?");
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the options
	 * defined above.
	 */
	i = 2;
	end = objc;
	while (i < end) {
		if (Tcl_GetIndexFromObj(interp, objv[i], seqgetopts, "option",
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
		switch ((enum seqgetopts)optindex) {
		case SEQGET_NOSYNC:
			aflag |= DB_TXN_NOSYNC;
			break;
		case SEQGET_TXN:
			if (i >= end) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "Remove: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		} /* switch */
		if (result != TCL_OK)
			break;
	}
	if (result != TCL_OK)
		goto out;

	ret = seq->remove(seq, txn, aflag);
	result = _ReturnSetup(interp,
	    ret, DB_RETOK_DBGET(ret), "sequence remove");
out:
	return (result);
}

/*
 * tcl_SeqGetFlags --
 */
static int
tcl_SeqGetFlags(interp, objc, objv, seq)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_SEQUENCE *seq;		/* Sequence pointer */
{
	int i, ret, result;
	u_int32_t flags;
	char buf[512];
	Tcl_Obj *res;

	static const struct {
		u_int32_t flag;
		char *arg;
	} seq_flags[] = {
		{ DB_SEQ_INC, "-inc" },
		{ DB_SEQ_DEC, "-dec" },
		{ DB_SEQ_WRAP, "-wrap" },
		{ 0, NULL }
	};

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return (TCL_ERROR);
	}

	ret = seq->get_flags(seq, &flags);
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "db get_flags")) == TCL_OK) {
		buf[0] = '\0';

		for (i = 0; seq_flags[i].flag != 0; i++)
			if (LF_ISSET(seq_flags[i].flag)) {
				if (strlen(buf) > 0)
					(void)strncat(buf, " ", sizeof(buf));
				(void)strncat(
				    buf, seq_flags[i].arg, sizeof(buf));
			}

		res = NewStringObj(buf, strlen(buf));
		Tcl_SetObjResult(interp, res);
	}

	return (result);
}
#endif /* HAVE_64BIT_TYPES */
