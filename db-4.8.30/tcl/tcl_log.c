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
#include "dbinc/log.h"
#include "dbinc/tcl_db.h"

#ifdef CONFIG_TEST
static int tcl_LogcGet __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_LOGC *));

/*
 * tcl_LogArchive --
 *
 * PUBLIC: int tcl_LogArchive __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LogArchive(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	static const char *archopts[] = {
		"-arch_abs",	"-arch_data",	"-arch_log",	"-arch_remove",
		NULL
	};
	enum archopts {
		ARCH_ABS,	ARCH_DATA,	ARCH_LOG,	ARCH_REMOVE
	};
	Tcl_Obj *fileobj, *res;
	u_int32_t flag;
	int i, optindex, result, ret;
	char **file, **list;

	result = TCL_OK;
	flag = 0;
	/*
	 * Get the flag index from the object based on the options
	 * defined above.
	 */
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i],
		    archopts, "option", TCL_EXACT, &optindex) != TCL_OK)
			return (IS_HELP(objv[i]));
		i++;
		switch ((enum archopts)optindex) {
		case ARCH_ABS:
			flag |= DB_ARCH_ABS;
			break;
		case ARCH_DATA:
			flag |= DB_ARCH_DATA;
			break;
		case ARCH_LOG:
			flag |= DB_ARCH_LOG;
			break;
		case ARCH_REMOVE:
			flag |= DB_ARCH_REMOVE;
			break;
		}
	}
	_debug_check();
	list = NULL;
	ret = dbenv->log_archive(dbenv, &list, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "log archive");
	if (result == TCL_OK) {
		res = Tcl_NewListObj(0, NULL);
		for (file = list; file != NULL && *file != NULL; file++) {
			fileobj = NewStringObj(*file, strlen(*file));
			result = Tcl_ListObjAppendElement(interp, res, fileobj);
			if (result != TCL_OK)
				break;
		}
		Tcl_SetObjResult(interp, res);
	}
	if (list != NULL)
		__os_ufree(dbenv->env, list);
	return (result);
}

/*
 * tcl_LogCompare --
 *
 * PUBLIC: int tcl_LogCompare __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*));
 */
int
tcl_LogCompare(interp, objc, objv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	DB_LSN lsn0, lsn1;
	Tcl_Obj *res;
	int result, ret;

	result = TCL_OK;
	/*
	 * No flags, must be 4 args.
	 */
	if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "lsn1 lsn2");
		return (TCL_ERROR);
	}

	result = _GetLsn(interp, objv[2], &lsn0);
	if (result == TCL_ERROR)
		return (result);
	result = _GetLsn(interp, objv[3], &lsn1);
	if (result == TCL_ERROR)
		return (result);

	_debug_check();
	ret = log_compare(&lsn0, &lsn1);
	res = Tcl_NewIntObj(ret);
	Tcl_SetObjResult(interp, res);
	return (result);
}

/*
 * tcl_LogFile --
 *
 * PUBLIC: int tcl_LogFile __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LogFile(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	DB_LSN lsn;
	Tcl_Obj *res;
	size_t len;
	int result, ret;
	char *name;

	result = TCL_OK;
	/*
	 * No flags, must be 3 args.
	 */
	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "lsn");
		return (TCL_ERROR);
	}

	result = _GetLsn(interp, objv[2], &lsn);
	if (result == TCL_ERROR)
		return (result);

	len = MSG_SIZE;
	ret = ENOMEM;
	name = NULL;
	while (ret == ENOMEM) {
		if (name != NULL)
			__os_free(dbenv->env, name);
		ret = __os_malloc(dbenv->env, len, &name);
		if (ret != 0) {
			Tcl_SetResult(interp, db_strerror(ret), TCL_STATIC);
			break;
		}
		_debug_check();
		ret = dbenv->log_file(dbenv, &lsn, name, len);
		len *= 2;
	}
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "log_file");
	if (ret == 0) {
		res = NewStringObj(name, strlen(name));
		Tcl_SetObjResult(interp, res);
	}

	if (name != NULL)
		__os_free(dbenv->env, name);

	return (result);
}

/*
 * tcl_LogFlush --
 *
 * PUBLIC: int tcl_LogFlush __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LogFlush(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	DB_LSN lsn, *lsnp;
	int result, ret;

	result = TCL_OK;
	/*
	 * No flags, must be 2 or 3 args.
	 */
	if (objc > 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?lsn?");
		return (TCL_ERROR);
	}

	if (objc == 3) {
		lsnp = &lsn;
		result = _GetLsn(interp, objv[2], &lsn);
		if (result == TCL_ERROR)
			return (result);
	} else
		lsnp = NULL;

	_debug_check();
	ret = dbenv->log_flush(dbenv, lsnp);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "log_flush");
	return (result);
}

/*
 * tcl_LogGet --
 *
 * PUBLIC: int tcl_LogGet __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LogGet(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{

	COMPQUIET(objv, NULL);
	COMPQUIET(objc, 0);
	COMPQUIET(dbenv, NULL);

	Tcl_SetResult(interp, "FAIL: log_get deprecated\n", TCL_STATIC);
	return (TCL_ERROR);
}

/*
 * tcl_LogPut --
 *
 * PUBLIC: int tcl_LogPut __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LogPut(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	static const char *logputopts[] = {
		"-flush",
		NULL
	};
	enum logputopts {
		LOGPUT_FLUSH
	};
	DB_LSN lsn;
	DBT data;
	Tcl_Obj *intobj, *res;
	void *dtmp;
	u_int32_t flag;
	int freedata, optindex, result, ret;

	result = TCL_OK;
	flag = 0;
	freedata = 0;
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-args? record");
		return (TCL_ERROR);
	}

	/*
	 * Data/record must be the last arg.
	 */
	memset(&data, 0, sizeof(data));
	ret = _CopyObjBytes(interp, objv[objc-1], &dtmp,
	    &data.size, &freedata);
	if (ret != 0) {
		result = _ReturnSetup(interp, ret,
		    DB_RETOK_STD(ret), "log put");
		return (result);
	}
	data.data = dtmp;

	/*
	 * Get the command name index from the object based on the options
	 * defined above.
	 */
	if (objc == 4) {
		if (Tcl_GetIndexFromObj(interp, objv[2],
		    logputopts, "option", TCL_EXACT, &optindex) != TCL_OK) {
			return (IS_HELP(objv[2]));
		}
		switch ((enum logputopts)optindex) {
		case LOGPUT_FLUSH:
			flag = DB_FLUSH;
			break;
		}
	}

	if (result == TCL_ERROR)
		return (result);

	_debug_check();
	ret = dbenv->log_put(dbenv, &lsn, &data, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "log_put");
	if (result == TCL_ERROR)
		return (result);
	res = Tcl_NewListObj(0, NULL);
	intobj = Tcl_NewWideIntObj((Tcl_WideInt)lsn.file);
	result = Tcl_ListObjAppendElement(interp, res, intobj);
	intobj = Tcl_NewWideIntObj((Tcl_WideInt)lsn.offset);
	result = Tcl_ListObjAppendElement(interp, res, intobj);
	Tcl_SetObjResult(interp, res);
	if (freedata)
		__os_free(NULL, dtmp);
	return (result);
}
/*
 * tcl_LogStat --
 *
 * PUBLIC: int tcl_LogStat __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LogStat(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	DB_LOG_STAT *sp;
	Tcl_Obj *res;
	int result, ret;

	result = TCL_OK;
	/*
	 * No args for this.  Error if there are some.
	 */
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return (TCL_ERROR);
	}
	_debug_check();
	ret = dbenv->log_stat(dbenv, &sp, 0);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "log stat");
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
	MAKE_STAT_LIST("Magic", sp->st_magic);
	MAKE_STAT_LIST("Log file Version", sp->st_version);
	MAKE_STAT_LIST("Region size", sp->st_regsize);
	MAKE_STAT_LIST("Log file mode", sp->st_mode);
	MAKE_STAT_LIST("Log record cache size", sp->st_lg_bsize);
	MAKE_STAT_LIST("Current log file size", sp->st_lg_size);
	MAKE_WSTAT_LIST("Log file records written", sp->st_record);
	MAKE_STAT_LIST("Mbytes written", sp->st_w_mbytes);
	MAKE_STAT_LIST("Bytes written (over Mb)", sp->st_w_bytes);
	MAKE_STAT_LIST("Mbytes written since checkpoint", sp->st_wc_mbytes);
	MAKE_STAT_LIST("Bytes written (over Mb) since checkpoint",
	    sp->st_wc_bytes);
	MAKE_WSTAT_LIST("Times log written", sp->st_wcount);
	MAKE_STAT_LIST("Times log written because cache filled up",
	    sp->st_wcount_fill);
	MAKE_WSTAT_LIST("Times log read from disk", sp->st_rcount);
	MAKE_WSTAT_LIST("Times log flushed to disk", sp->st_scount);
	MAKE_STAT_LIST("Current log file number", sp->st_cur_file);
	MAKE_STAT_LIST("Current log file offset", sp->st_cur_offset);
	MAKE_STAT_LIST("On-disk log file number", sp->st_disk_file);
	MAKE_STAT_LIST("On-disk log file offset", sp->st_disk_offset);
	MAKE_STAT_LIST("Max commits in a log flush", sp->st_maxcommitperflush);
	MAKE_STAT_LIST("Min commits in a log flush", sp->st_mincommitperflush);
	MAKE_WSTAT_LIST("Number of region lock waits", sp->st_region_wait);
	MAKE_WSTAT_LIST("Number of region lock nowaits", sp->st_region_nowait);
#endif
	Tcl_SetObjResult(interp, res);
error:
	__os_ufree(dbenv->env, sp);
	return (result);
}

/*
 * logc_Cmd --
 *	Implements the log cursor command.
 *
 * PUBLIC: int logc_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
 */
int
logc_Cmd(clientData, interp, objc, objv)
	ClientData clientData;		/* Cursor handle */
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *logccmds[] = {
		"close",
		"get",
		"version",
		NULL
	};
	enum logccmds {
		LOGCCLOSE,
		LOGCGET,
		LOGCVERSION
	};
	DB_LOGC *logc;
	DBTCL_INFO *logcip;
	Tcl_Obj *res;
	u_int32_t version;
	int cmdindex, result, ret;

	Tcl_ResetResult(interp);
	logc = (DB_LOGC *)clientData;
	logcip = _PtrToInfo((void *)logc);
	result = TCL_OK;

	if (objc <= 1) {
		Tcl_WrongNumArgs(interp, 1, objv, "command cmdargs");
		return (TCL_ERROR);
	}
	if (logc == NULL) {
		Tcl_SetResult(interp, "NULL logc pointer", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (logcip == NULL) {
		Tcl_SetResult(interp, "NULL logc info pointer", TCL_STATIC);
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the berkdbcmds
	 * defined above.
	 */
	if (Tcl_GetIndexFromObj(interp, objv[1], logccmds, "command",
	    TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));
	switch ((enum logccmds)cmdindex) {
	case LOGCCLOSE:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = logc->close(logc, 0);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "logc close");
		if (result == TCL_OK) {
			(void)Tcl_DeleteCommand(interp, logcip->i_name);
			_DeleteInfo(logcip);
		}
		break;
	case LOGCGET:
		result = tcl_LogcGet(interp, objc, objv, logc);
		break;
	case LOGCVERSION:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = logc->version(logc, &version, 0);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "logc version")) == TCL_OK) {
			res = Tcl_NewIntObj((int)version);
			Tcl_SetObjResult(interp, res);
		}
		break;
	}

	return (result);
}

static int
tcl_LogcGet(interp, objc, objv, logc)
	Tcl_Interp *interp;
	int objc;
	Tcl_Obj * CONST *objv;
	DB_LOGC *logc;
{
	static const char *logcgetopts[] = {
		"-current",
		"-first",
		"-last",
		"-next",
		"-prev",
		"-set",
		NULL
	};
	enum logcgetopts {
		LOGCGET_CURRENT,
		LOGCGET_FIRST,
		LOGCGET_LAST,
		LOGCGET_NEXT,
		LOGCGET_PREV,
		LOGCGET_SET
	};
	DB_LSN lsn;
	DBT data;
	Tcl_Obj *dataobj, *lsnlist, *myobjv[2], *res;
	u_int32_t flag;
	int i, myobjc, optindex, result, ret;

	result = TCL_OK;
	res = NULL;
	flag = 0;

	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-args? lsn");
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the options
	 * defined above.
	 */
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i],
		    logcgetopts, "option", TCL_EXACT, &optindex) != TCL_OK)
			return (IS_HELP(objv[i]));
		i++;
		switch ((enum logcgetopts)optindex) {
		case LOGCGET_CURRENT:
			FLAG_CHECK(flag);
			flag |= DB_CURRENT;
			break;
		case LOGCGET_FIRST:
			FLAG_CHECK(flag);
			flag |= DB_FIRST;
			break;
		case LOGCGET_LAST:
			FLAG_CHECK(flag);
			flag |= DB_LAST;
			break;
		case LOGCGET_NEXT:
			FLAG_CHECK(flag);
			flag |= DB_NEXT;
			break;
		case LOGCGET_PREV:
			FLAG_CHECK(flag);
			flag |= DB_PREV;
			break;
		case LOGCGET_SET:
			FLAG_CHECK(flag);
			flag |= DB_SET;
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-set lsn?");
				result = TCL_ERROR;
				break;
			}
			result = _GetLsn(interp, objv[i++], &lsn);
			break;
		}
	}

	if (result == TCL_ERROR)
		return (result);

	memset(&data, 0, sizeof(data));

	_debug_check();
	ret = logc->get(logc, &lsn, &data, flag);

	res = Tcl_NewListObj(0, NULL);
	if (res == NULL)
		goto memerr;

	if (ret == 0) {
		/*
		 * Success.  Set up return list as {LSN data} where LSN
		 * is a sublist {file offset}.
		 */
		myobjc = 2;
		myobjv[0] = Tcl_NewWideIntObj((Tcl_WideInt)lsn.file);
		myobjv[1] = Tcl_NewWideIntObj((Tcl_WideInt)lsn.offset);
		lsnlist = Tcl_NewListObj(myobjc, myobjv);
		if (lsnlist == NULL)
			goto memerr;

		result = Tcl_ListObjAppendElement(interp, res, lsnlist);
		dataobj = NewStringObj(data.data, data.size);
		if (dataobj == NULL) {
			goto memerr;
		}
		result = Tcl_ListObjAppendElement(interp, res, dataobj);
	} else
		result = _ReturnSetup(interp, ret, DB_RETOK_LGGET(ret),
		    "DB_LOGC->get");

	Tcl_SetObjResult(interp, res);

	if (0) {
memerr:		if (res != NULL) {
			Tcl_DecrRefCount(res);
		}
		Tcl_SetResult(interp, "allocation failed", TCL_STATIC);
	}

	return (result);
}

static const char *confwhich[] = {
	"autoremove",
	"direct",
	"dsync",
	"inmemory",
	"zero",
	NULL
};
enum logwhich {
	LOGCONF_AUTO,
	LOGCONF_DIRECT,
	LOGCONF_DSYNC,
	LOGCONF_INMEMORY,
	LOGCONF_ZERO
};

/*
 * tcl_LogConfig --
 *	Call DB_ENV->rep_set_config().
 *
 * PUBLIC: int tcl_LogConfig
 * PUBLIC:     __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *));
 */
int
tcl_LogConfig(interp, dbenv, list)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Environment pointer */
	Tcl_Obj *list;			/* {which on|off} */
{
	static const char *confonoff[] = {
		"off",
		"on",
		NULL
	};
	enum confonoff {
		LOGCONF_OFF,
		LOGCONF_ON
	};
	Tcl_Obj **myobjv, *onoff, *which;
	int myobjc, on, optindex, result, ret;
	u_int32_t wh;

	result = Tcl_ListObjGetElements(interp, list, &myobjc, &myobjv);
	if (myobjc != 2)
		Tcl_WrongNumArgs(interp, 2, myobjv, "?{which onoff}?");
	which = myobjv[0];
	onoff = myobjv[1];
	if (result != TCL_OK)
		return (result);
	if (Tcl_GetIndexFromObj(interp, which, confwhich, "option",
	    TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(which));

	switch ((enum logwhich)optindex) {
	case LOGCONF_AUTO:
		wh = DB_LOG_AUTO_REMOVE;
		break;
	case LOGCONF_DIRECT:
		wh = DB_LOG_DIRECT;
		break;
	case LOGCONF_DSYNC:
		wh = DB_LOG_DSYNC;
		break;
	case LOGCONF_INMEMORY:
		wh = DB_LOG_IN_MEMORY;
		break;
	case LOGCONF_ZERO:
		wh = DB_LOG_ZERO;
		break;
	default:
		return (TCL_ERROR);
	}
	if (Tcl_GetIndexFromObj(interp, onoff, confonoff, "option",
	    TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(onoff));
	switch ((enum confonoff)optindex) {
	case LOGCONF_OFF:
		on = 0;
		break;
	case LOGCONF_ON:
		on = 1;
		break;
	default:
		return (TCL_ERROR);
	}
	ret = dbenv->log_set_config(dbenv, wh, on);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env rep_config"));
}

/*
 * tcl_LogGetConfig --
 *	Call DB_ENV->rep_get_config().
 *
 * PUBLIC: int tcl_LogGetConfig
 * PUBLIC:     __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *));
 */
int
tcl_LogGetConfig(interp, dbenv, which)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Environment pointer */
	Tcl_Obj *which;			/* which flag */
{
	Tcl_Obj *res;
	int on, optindex, result, ret;
	u_int32_t wh;

	if (Tcl_GetIndexFromObj(interp, which, confwhich, "option",
	    TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(which));

	res = NULL;
	switch ((enum logwhich)optindex) {
	case LOGCONF_AUTO:
		wh = DB_LOG_AUTO_REMOVE;
		break;
	case LOGCONF_DIRECT:
		wh = DB_LOG_DIRECT;
		break;
	case LOGCONF_DSYNC:
		wh = DB_LOG_DSYNC;
		break;
	case LOGCONF_INMEMORY:
		wh = DB_LOG_IN_MEMORY;
		break;
	case LOGCONF_ZERO:
		wh = DB_LOG_ZERO;
		break;
	default:
		return (TCL_ERROR);
	}
	ret = dbenv->log_get_config(dbenv, wh, &on);
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env log_config")) == TCL_OK) {
		res = Tcl_NewIntObj(on);
		Tcl_SetObjResult(interp, res);
	}
	return (result);
}
#endif
