/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"
#ifdef CONFIG_TEST

#define	DB_DBM_HSEARCH	1
#include "db_int.h"
#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <tcl.h>
#endif
#include "dbinc/tcl_db.h"

/*
 * bdb_HCommand --
 *	Implements h* functions.
 *
 * PUBLIC: int bdb_HCommand __P((Tcl_Interp *, int, Tcl_Obj * CONST*));
 */
int
bdb_HCommand(interp, objc, objv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *hcmds[] = {
		"hcreate",
		"hdestroy",
		"hsearch",
		NULL
	};
	enum hcmds {
		HHCREATE,
		HHDESTROY,
		HHSEARCH
	};
	static const char *srchacts[] = {
		"enter",
		"find",
		NULL
	};
	enum srchacts {
		ACT_ENTER,
		ACT_FIND
	};
	ENTRY item, *hres;
	ACTION action;
	int actindex, cmdindex, nelem, result, ret;
	Tcl_Obj *res;

	result = TCL_OK;
	/*
	 * Get the command name index from the object based on the cmds
	 * defined above.  This SHOULD NOT fail because we already checked
	 * in the 'berkdb' command.
	 */
	if (Tcl_GetIndexFromObj(interp,
	    objv[1], hcmds, "command", TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));

	res = NULL;
	switch ((enum hcmds)cmdindex) {
	case HHCREATE:
		/*
		 * Must be 1 arg, nelem.  Error if not.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "nelem");
			return (TCL_ERROR);
		}
		result = Tcl_GetIntFromObj(interp, objv[2], &nelem);
		if (result == TCL_OK) {
			_debug_check();
			ret = hcreate((size_t)nelem) == 0 ? 1: 0;
			(void)_ReturnSetup(
			    interp, ret, DB_RETOK_STD(ret), "hcreate");
		}
		break;
	case HHSEARCH:
		/*
		 * 3 args for this.  Error if different.
		 */
		if (objc != 5) {
			Tcl_WrongNumArgs(interp, 2, objv, "key data action");
			return (TCL_ERROR);
		}
		item.key = Tcl_GetStringFromObj(objv[2], NULL);
		item.data = Tcl_GetStringFromObj(objv[3], NULL);
		if (Tcl_GetIndexFromObj(interp, objv[4], srchacts,
		    "action", TCL_EXACT, &actindex) != TCL_OK)
			return (IS_HELP(objv[4]));
		switch ((enum srchacts)actindex) {
		case ACT_ENTER:
			action = ENTER;
			break;
		default:
		case ACT_FIND:
			action = FIND;
			break;
		}
		_debug_check();
		hres = hsearch(item, action);
		if (hres == NULL)
			Tcl_SetResult(interp, "-1", TCL_STATIC);
		else if (action == FIND)
			Tcl_SetResult(interp, (char *)hres->data, TCL_STATIC);
		else
			/* action is ENTER */
			Tcl_SetResult(interp, "0", TCL_STATIC);

		break;
	case HHDESTROY:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		hdestroy();
		res = Tcl_NewIntObj(0);
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
 *
 * bdb_NdbmOpen --
 *	Opens an ndbm database.
 *
 * PUBLIC: #if DB_DBM_HSEARCH != 0
 * PUBLIC: int bdb_NdbmOpen __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DBM **));
 * PUBLIC: #endif
 */
int
bdb_NdbmOpen(interp, objc, objv, dbpp)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DBM **dbpp;			/* Dbm pointer */
{
	static const char *ndbopen[] = {
		"-create",
		"-mode",
		"-rdonly",
		"-truncate",
		"--",
		NULL
	};
	enum ndbopen {
		NDB_CREATE,
		NDB_MODE,
		NDB_RDONLY,
		NDB_TRUNC,
		NDB_ENDARG
	};

	int endarg, i, mode, open_flags, optindex, read_only, result, ret;
	char *arg, *db;

	result = TCL_OK;
	endarg = mode = open_flags = read_only = 0;

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "?args?");
		return (TCL_ERROR);
	}

	/*
	 * Get the option name index from the object based on the args
	 * defined above.
	 */
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], ndbopen, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			arg = Tcl_GetStringFromObj(objv[i], NULL);
			if (arg[0] == '-') {
				result = IS_HELP(objv[i]);
				goto error;
			} else
				Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum ndbopen)optindex) {
		case NDB_CREATE:
			open_flags |= O_CREAT;
			break;
		case NDB_RDONLY:
			read_only = 1;
			break;
		case NDB_TRUNC:
			open_flags |= O_TRUNC;
			break;
		case NDB_MODE:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-mode mode?");
				result = TCL_ERROR;
				break;
			}
			/*
			 * Don't need to check result here because
			 * if TCL_ERROR, the error message is already
			 * set up, and we'll bail out below.  If ok,
			 * the mode is set and we go on.
			 */
			result = Tcl_GetIntFromObj(interp, objv[i++], &mode);
			break;
		case NDB_ENDARG:
			endarg = 1;
			break;
		}

		/*
		 * If, at any time, parsing the args we get an error,
		 * bail out and return.
		 */
		if (result != TCL_OK)
			goto error;
		if (endarg)
			break;
	}
	if (result != TCL_OK)
		goto error;

	/*
	 * Any args we have left, (better be 0, or 1 left) is a
	 * file name.  If we have 0, then an in-memory db.  If
	 * there is 1, a db name.
	 */
	db = NULL;
	if (i != objc && i != objc - 1) {
		Tcl_WrongNumArgs(interp, 2, objv, "?args? ?file?");
		result = TCL_ERROR;
		goto error;
	}
	if (i != objc)
		db = Tcl_GetStringFromObj(objv[objc - 1], NULL);

	/*
	 * When we get here, we have already parsed all of our args
	 * and made all our calls to set up the database.  Everything
	 * is okay so far, no errors, if we get here.
	 *
	 * Now open the database.
	 */
	if (read_only)
		open_flags |= O_RDONLY;
	else
		open_flags |= O_RDWR;
	_debug_check();
	if ((*dbpp = dbm_open(db, open_flags, mode)) == NULL) {
		ret = Tcl_GetErrno();
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db open");
		goto error;
	}
	return (TCL_OK);

error:
	*dbpp = NULL;
	return (result);
}

/*
 * bdb_DbmCommand --
 *	Implements "dbm" commands.
 *
 * PUBLIC: #if DB_DBM_HSEARCH != 0
 * PUBLIC: int bdb_DbmCommand
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST*, int, DBM *));
 * PUBLIC: #endif
 */
int
bdb_DbmCommand(interp, objc, objv, flag, dbm)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	int flag;			/* Which db interface */
	DBM *dbm;			/* DBM pointer */
{
	static const char *dbmcmds[] = {
		"dbmclose",
		"dbminit",
		"delete",
		"fetch",
		"firstkey",
		"nextkey",
		"store",
		NULL
	};
	enum dbmcmds {
		DBMCLOSE,
		DBMINIT,
		DBMDELETE,
		DBMFETCH,
		DBMFIRST,
		DBMNEXT,
		DBMSTORE
	};
	static const char *stflag[] = {
		"insert",	"replace",
		NULL
	};
	enum stflag {
		STINSERT,	STREPLACE
	};
	datum key, data;
	void *dtmp, *ktmp;
	u_int32_t size;
	int cmdindex, freedata, freekey, stindex, result, ret;
	char *name, *t;

	result = TCL_OK;
	freekey = freedata = 0;
	dtmp = ktmp = NULL;

	/*
	 * Get the command name index from the object based on the cmds
	 * defined above.  This SHOULD NOT fail because we already checked
	 * in the 'berkdb' command.
	 */
	if (Tcl_GetIndexFromObj(interp,
	    objv[1], dbmcmds, "command", TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));

	switch ((enum dbmcmds)cmdindex) {
	case DBMCLOSE:
		/*
		 * No arg for this.  Error if different.
		 */
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		if (flag == DBTCL_DBM)
			ret = dbmclose();
		else {
			Tcl_SetResult(interp,
			    "Bad interface flag for command", TCL_STATIC);
			return (TCL_ERROR);
		}
		(void)_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "dbmclose");
		break;
	case DBMINIT:
		/*
		 * Must be 1 arg - file.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "file");
			return (TCL_ERROR);
		}
		name = Tcl_GetStringFromObj(objv[2], NULL);
		if (flag == DBTCL_DBM)
			ret = dbminit(name);
		else {
			Tcl_SetResult(interp, "Bad interface flag for command",
			    TCL_STATIC);
			return (TCL_ERROR);
		}
		(void)_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "dbminit");
		break;
	case DBMFETCH:
		/*
		 * 1 arg for this.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "key");
			return (TCL_ERROR);
		}
		if ((ret = _CopyObjBytes(
		    interp, objv[2], &ktmp, &size, &freekey)) != 0) {
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_STD(ret), "dbm fetch");
			goto out;
		}
		key.dsize = (int)size;
		key.dptr = (char *)ktmp;
		_debug_check();
		if (flag == DBTCL_DBM)
			data = fetch(key);
		else if (flag == DBTCL_NDBM)
			data = dbm_fetch(dbm, key);
		else {
			Tcl_SetResult(interp,
			    "Bad interface flag for command", TCL_STATIC);
			result = TCL_ERROR;
			goto out;
		}
		if (data.dptr == NULL ||
		    (ret = __os_malloc(NULL, (size_t)data.dsize + 1, &t)) != 0)
			Tcl_SetResult(interp, "-1", TCL_STATIC);
		else {
			memcpy(t, data.dptr, (size_t)data.dsize);
			t[data.dsize] = '\0';
			Tcl_SetResult(interp, t, TCL_VOLATILE);
			__os_free(NULL, t);
		}
		break;
	case DBMSTORE:
		/*
		 * 2 args for this.  Error if different.
		 */
		if (objc != 4 && flag == DBTCL_DBM) {
			Tcl_WrongNumArgs(interp, 2, objv, "key data");
			return (TCL_ERROR);
		}
		if (objc != 5 && flag == DBTCL_NDBM) {
			Tcl_WrongNumArgs(interp, 2, objv, "key data action");
			return (TCL_ERROR);
		}
		if ((ret = _CopyObjBytes(
		    interp, objv[2], &ktmp, &size, &freekey)) != 0) {
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_STD(ret), "dbm fetch");
			goto out;
		}
		key.dsize = (int)size;
		key.dptr = (char *)ktmp;
		if ((ret = _CopyObjBytes(
		    interp, objv[3], &dtmp, &size, &freedata)) != 0) {
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_STD(ret), "dbm fetch");
			goto out;
		}
		data.dsize = (int)size;
		data.dptr = (char *)dtmp;
		_debug_check();
		if (flag == DBTCL_DBM)
			ret = store(key, data);
		else if (flag == DBTCL_NDBM) {
			if (Tcl_GetIndexFromObj(interp, objv[4], stflag,
			    "flag", TCL_EXACT, &stindex) != TCL_OK)
				return (IS_HELP(objv[4]));
			switch ((enum stflag)stindex) {
			case STINSERT:
				flag = DBM_INSERT;
				break;
			case STREPLACE:
				flag = DBM_REPLACE;
				break;
			}
			ret = dbm_store(dbm, key, data, flag);
		} else {
			Tcl_SetResult(interp,
			    "Bad interface flag for command", TCL_STATIC);
			return (TCL_ERROR);
		}
		(void)_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "store");
		break;
	case DBMDELETE:
		/*
		 * 1 arg for this.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "key");
			return (TCL_ERROR);
		}
		if ((ret = _CopyObjBytes(
		    interp, objv[2], &ktmp, &size, &freekey)) != 0) {
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_STD(ret), "dbm fetch");
			goto out;
		}
		key.dsize = (int)size;
		key.dptr = (char *)ktmp;
		_debug_check();
		if (flag == DBTCL_DBM)
			ret = delete(key);
		else if (flag == DBTCL_NDBM)
			ret = dbm_delete(dbm, key);
		else {
			Tcl_SetResult(interp,
			    "Bad interface flag for command", TCL_STATIC);
			return (TCL_ERROR);
		}
		(void)_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "delete");
		break;
	case DBMFIRST:
		/*
		 * No arg for this.  Error if different.
		 */
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		if (flag == DBTCL_DBM)
			key = firstkey();
		else if (flag == DBTCL_NDBM)
			key = dbm_firstkey(dbm);
		else {
			Tcl_SetResult(interp,
			    "Bad interface flag for command", TCL_STATIC);
			return (TCL_ERROR);
		}
		if (key.dptr == NULL ||
		    (ret = __os_malloc(NULL, (size_t)key.dsize + 1, &t)) != 0)
			Tcl_SetResult(interp, "-1", TCL_STATIC);
		else {
			memcpy(t, key.dptr, (size_t)key.dsize);
			t[key.dsize] = '\0';
			Tcl_SetResult(interp, t, TCL_VOLATILE);
			__os_free(NULL, t);
		}
		break;
	case DBMNEXT:
		/*
		 * 0 or 1 arg for this.  Error if different.
		 */
		_debug_check();
		if (flag == DBTCL_DBM) {
			if (objc != 3) {
				Tcl_WrongNumArgs(interp, 2, objv, NULL);
				return (TCL_ERROR);
			}
			if ((ret = _CopyObjBytes(
			    interp, objv[2], &ktmp, &size, &freekey)) != 0) {
				result = _ReturnSetup(interp, ret,
				    DB_RETOK_STD(ret), "dbm fetch");
				goto out;
			}
			key.dsize = (int)size;
			key.dptr = (char *)ktmp;
			data = nextkey(key);
		} else if (flag == DBTCL_NDBM) {
			if (objc != 2) {
				Tcl_WrongNumArgs(interp, 2, objv, NULL);
				return (TCL_ERROR);
			}
			data = dbm_nextkey(dbm);
		} else {
			Tcl_SetResult(interp,
			    "Bad interface flag for command", TCL_STATIC);
			return (TCL_ERROR);
		}
		if (data.dptr == NULL ||
		    (ret = __os_malloc(NULL, (size_t)data.dsize + 1, &t)) != 0)
			Tcl_SetResult(interp, "-1", TCL_STATIC);
		else {
			memcpy(t, data.dptr, (size_t)data.dsize);
			t[data.dsize] = '\0';
			Tcl_SetResult(interp, t, TCL_VOLATILE);
			__os_free(NULL, t);
		}
		break;
	}

out:	if (dtmp != NULL && freedata)
		__os_free(NULL, dtmp);
	if (ktmp != NULL && freekey)
		__os_free(NULL, ktmp);
	return (result);
}

/*
 * ndbm_Cmd --
 *	Implements the "ndbm" widget.
 *
 * PUBLIC: int ndbm_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
 */
int
ndbm_Cmd(clientData, interp, objc, objv)
	ClientData clientData;		/* DB handle */
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *ndbcmds[] = {
		"clearerr",
		"close",
		"delete",
		"dirfno",
		"error",
		"fetch",
		"firstkey",
		"nextkey",
		"pagfno",
		"rdonly",
		"store",
		NULL
	};
	enum ndbcmds {
		NDBCLRERR,
		NDBCLOSE,
		NDBDELETE,
		NDBDIRFNO,
		NDBERR,
		NDBFETCH,
		NDBFIRST,
		NDBNEXT,
		NDBPAGFNO,
		NDBRDONLY,
		NDBSTORE
	};
	DBM *dbp;
	DBTCL_INFO *dbip;
	Tcl_Obj *res;
	int cmdindex, result, ret;

	Tcl_ResetResult(interp);
	dbp = (DBM *)clientData;
	dbip = _PtrToInfo((void *)dbp);
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
	    objv[1], ndbcmds, "command", TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));

	res = NULL;
	switch ((enum ndbcmds)cmdindex) {
	case NDBCLOSE:
		_debug_check();
		dbm_close(dbp);
		(void)Tcl_DeleteCommand(interp, dbip->i_name);
		_DeleteInfo(dbip);
		res = Tcl_NewIntObj(0);
		break;
	case NDBDELETE:
	case NDBFETCH:
	case NDBFIRST:
	case NDBNEXT:
	case NDBSTORE:
		result = bdb_DbmCommand(interp, objc, objv, DBTCL_NDBM, dbp);
		break;
	case NDBCLRERR:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbm_clearerr(dbp);
		if (ret)
			(void)_ReturnSetup(
			    interp, ret, DB_RETOK_STD(ret), "clearerr");
		else
			res = Tcl_NewIntObj(ret);
		break;
	case NDBDIRFNO:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbm_dirfno(dbp);
		res = Tcl_NewIntObj(ret);
		break;
	case NDBPAGFNO:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbm_pagfno(dbp);
		res = Tcl_NewIntObj(ret);
		break;
	case NDBERR:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbm_error(dbp);
		Tcl_SetErrno(ret);
		Tcl_SetResult(interp,
		    (char *)Tcl_PosixError(interp), TCL_STATIC);
		break;
	case NDBRDONLY:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbm_rdonly(dbp);
		if (ret)
			(void)_ReturnSetup(
			    interp, ret, DB_RETOK_STD(ret), "rdonly");
		else
			res = Tcl_NewIntObj(ret);
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
#endif /* CONFIG_TEST */
