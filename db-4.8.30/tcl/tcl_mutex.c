/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <tcl.h>
#endif
#include "dbinc/tcl_db.h"

#ifdef CONFIG_TEST
/*
 * PUBLIC: int tcl_Mutex __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:    DB_ENV *));
 *
 * tcl_Mutex --
 *      Implements dbenv->mutex_alloc method.
 */
int
tcl_Mutex(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment */
{
	static const char *which[] = {
		"-process_only",
		"-self_block",
		NULL
	};
	enum which {
		PROCONLY,
		SELFBLOCK
	};
	int arg, i, result, ret;
	u_int32_t flags;
	db_mutex_t indx;
	Tcl_Obj *res;

	result = TCL_OK;
	flags = 0;
	Tcl_ResetResult(interp);
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 2, objv,
		    "-proccess_only | -self_block");
		return (TCL_ERROR);
	}

	i = 2;
	while (i < objc) {
		/*
		 * If there is an arg, make sure it is the right one.
		 */
		if (Tcl_GetIndexFromObj(interp, objv[i], which, "option",
		    TCL_EXACT, &arg) != TCL_OK)
			return (IS_HELP(objv[i]));
		i++;
		switch ((enum which)arg) {
		case PROCONLY:
			flags |= DB_MUTEX_PROCESS_ONLY;
			break;
		case SELFBLOCK:
			flags |= DB_MUTEX_SELF_BLOCK;
			break;
		}
	}
	res = NULL;
	ret = dbenv->mutex_alloc(dbenv, flags, &indx);
	if (ret != 0) {
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "mutex_alloc");
		Tcl_SetResult(interp, "allocation failed", TCL_STATIC);
	} else {
		res = Tcl_NewWideIntObj((Tcl_WideInt)indx);
		Tcl_SetObjResult(interp, res);
	}
	return (result);
}

/*
 * PUBLIC: int tcl_MutFree __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:    DB_ENV *));
 *
 * tcl_MutFree --
 *      Implements dbenv->mutex_free method.
 */
int
tcl_MutFree(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment */
{
	int result, ret;
	db_mutex_t indx;

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 3, objv, "mutexid");
		return (TCL_ERROR);
	}
	if ((result = _GetUInt32(interp, objv[2], &indx)) != TCL_OK)
		return (result);
	ret = dbenv->mutex_free(dbenv, indx);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "env mutex_free"));
}

/*
 * PUBLIC: int tcl_MutGet __P((Tcl_Interp *, DB_ENV *, int));
 *
 * tcl_MutGet --
 *      Implements dbenv->mutex_get_* methods.
 */
int
tcl_MutGet(interp, dbenv, op)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Environment */
	int op;				/* Which item to get */
{
	Tcl_Obj *res;
	u_int32_t val;
	int result, ret;

	res = NULL;
	val = 0;
	ret = 0;

	switch (op) {
	case DBTCL_MUT_ALIGN:
		ret = dbenv->mutex_get_align(dbenv, &val);
		break;
	case DBTCL_MUT_INCR:
		ret = dbenv->mutex_get_increment(dbenv, &val);
		break;
	case DBTCL_MUT_MAX:
		ret = dbenv->mutex_get_max(dbenv, &val);
		break;
	case DBTCL_MUT_TAS:
		ret = dbenv->mutex_get_tas_spins(dbenv, &val);
		break;
	default:
		return (TCL_ERROR);
	}
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "mutex_get")) == TCL_OK) {
		res = Tcl_NewLongObj((long)val);
		Tcl_SetObjResult(interp, res);
	}
	return (result);
}

/*
 * PUBLIC: int tcl_MutLock __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:    DB_ENV *));
 *
 * tcl_MutLock --
 *      Implements dbenv->mutex_lock method.
 */
int
tcl_MutLock(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment */
{
	int result, ret;
	db_mutex_t indx;

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 3, objv, "mutexid");
		return (TCL_ERROR);
	}
	if ((result = _GetUInt32(interp, objv[2], &indx)) != TCL_OK)
		return (result);
	ret = dbenv->mutex_lock(dbenv, indx);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "env mutex_lock"));
}

/*
 * PUBLIC: int tcl_MutSet __P((Tcl_Interp *, Tcl_Obj *,
 * PUBLIC:    DB_ENV *, int));
 *
 * tcl_MutSet --
 *      Implements dbenv->mutex_set methods.
 */
int
tcl_MutSet(interp, obj, dbenv, op)
	Tcl_Interp *interp;		/* Interpreter */
	Tcl_Obj *obj;			/* The argument object */
	DB_ENV *dbenv;			/* Environment */
	int op;				/* Which item to set */
{
	int result, ret;
	u_int32_t val;

	if ((result = _GetUInt32(interp, obj, &val)) != TCL_OK)
		return (result);
	switch (op) {
	case DBTCL_MUT_ALIGN:
		ret = dbenv->mutex_set_align(dbenv, val);
		break;
	case DBTCL_MUT_INCR:
		ret = dbenv->mutex_set_increment(dbenv, val);
		break;
	case DBTCL_MUT_MAX:
		ret = dbenv->mutex_set_max(dbenv, val);
		break;
	case DBTCL_MUT_TAS:
		ret = dbenv->mutex_set_tas_spins(dbenv, val);
		break;
	default:
		return (TCL_ERROR);
	}
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "env mutex_set"));
}

/*
 * PUBLIC: int tcl_MutStat __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:    DB_ENV *));
 *
 * tcl_MutStat --
 *      Implements dbenv->mutex_stat method.
 */
int
tcl_MutStat(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment */
{
	DB_MUTEX_STAT *sp;
	Tcl_Obj *res;
	u_int32_t flag;
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
	ret = dbenv->mutex_stat(dbenv, &sp, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "mutex stat");
	if (result == TCL_ERROR)
		return (result);

	res = Tcl_NewObj();
	MAKE_STAT_LIST("Mutex align", sp->st_mutex_align);
	MAKE_STAT_LIST("Mutex TAS spins", sp->st_mutex_tas_spins);
	MAKE_STAT_LIST("Mutex count", sp->st_mutex_cnt);
	MAKE_STAT_LIST("Free mutexes", sp->st_mutex_free);
	MAKE_STAT_LIST("Mutexes in use", sp->st_mutex_inuse);
	MAKE_STAT_LIST("Max in use", sp->st_mutex_inuse_max);
	MAKE_STAT_LIST("Mutex region size", sp->st_regsize);
	MAKE_WSTAT_LIST("Number of region waits", sp->st_region_wait);
	MAKE_WSTAT_LIST("Number of region no waits", sp->st_region_nowait);
	Tcl_SetObjResult(interp, res);

	/*
	 * The 'error' label is used by the MAKE_STAT_LIST macro.
	 * Therefore we cannot remove it, and also we know that
	 * sp is allocated at that time.
	 */
error:	__os_ufree(dbenv->env, sp);
	return (result);
}

/*
 * PUBLIC: int tcl_MutUnlock __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:    DB_ENV *));
 *
 * tcl_MutUnlock --
 *      Implements dbenv->mutex_unlock method.
 */
int
tcl_MutUnlock(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment */
{
	int result, ret;
	db_mutex_t indx;

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 3, objv, "mutexid");
		return (TCL_ERROR);
	}
	if ((result = _GetUInt32(interp, objv[2], &indx)) != TCL_OK)
		return (result);
	ret = dbenv->mutex_unlock(dbenv, indx);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env mutex_unlock"));
}
#endif
