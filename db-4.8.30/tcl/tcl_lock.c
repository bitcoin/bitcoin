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

/*
 * Prototypes for procedures defined later in this file:
 */
#ifdef CONFIG_TEST
static int      lock_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
static int	_LockMode __P((Tcl_Interp *, Tcl_Obj *, db_lockmode_t *));
static int	_GetThisLock __P((Tcl_Interp *, DB_ENV *, u_int32_t,
				     u_int32_t, DBT *, db_lockmode_t, char *));
static void	_LockPutInfo __P((Tcl_Interp *, db_lockop_t, DB_LOCK *,
				     u_int32_t, DBT *));

/*
 * tcl_LockDetect --
 *
 * PUBLIC: int tcl_LockDetect __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LockDetect(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	static const char *ldopts[] = {
		"default",
		"expire",
		"maxlocks",
		"maxwrites",
		"minlocks",
		"minwrites",
		"oldest",
		"random",
		"youngest",
		 NULL
	};
	enum ldopts {
		LD_DEFAULT,
		LD_EXPIRE,
		LD_MAXLOCKS,
		LD_MAXWRITES,
		LD_MINLOCKS,
		LD_MINWRITES,
		LD_OLDEST,
		LD_RANDOM,
		LD_YOUNGEST
	};
	u_int32_t flag, policy;
	int i, optindex, result, ret;

	result = TCL_OK;
	flag = policy = 0;
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i],
		    ldopts, "option", TCL_EXACT, &optindex) != TCL_OK)
			return (IS_HELP(objv[i]));
		i++;
		switch ((enum ldopts)optindex) {
		case LD_DEFAULT:
			FLAG_CHECK(policy);
			policy = DB_LOCK_DEFAULT;
			break;
		case LD_EXPIRE:
			FLAG_CHECK(policy);
			policy = DB_LOCK_EXPIRE;
			break;
		case LD_MAXLOCKS:
			FLAG_CHECK(policy);
			policy = DB_LOCK_MAXLOCKS;
			break;
		case LD_MAXWRITES:
			FLAG_CHECK(policy);
			policy = DB_LOCK_MAXWRITE;
			break;
		case LD_MINLOCKS:
			FLAG_CHECK(policy);
			policy = DB_LOCK_MINLOCKS;
			break;
		case LD_MINWRITES:
			FLAG_CHECK(policy);
			policy = DB_LOCK_MINWRITE;
			break;
		case LD_OLDEST:
			FLAG_CHECK(policy);
			policy = DB_LOCK_OLDEST;
			break;
		case LD_RANDOM:
			FLAG_CHECK(policy);
			policy = DB_LOCK_RANDOM;
			break;
		case LD_YOUNGEST:
			FLAG_CHECK(policy);
			policy = DB_LOCK_YOUNGEST;
			break;
		}
	}

	_debug_check();
	ret = dbenv->lock_detect(dbenv, flag, policy, NULL);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "lock detect");
	return (result);
}

/*
 * tcl_LockGet --
 *
 * PUBLIC: int tcl_LockGet __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LockGet(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	static const char *lgopts[] = {
		"-nowait",
		 NULL
	};
	enum lgopts {
		LGNOWAIT
	};
	DBT obj;
	Tcl_Obj *res;
	void *otmp;
	db_lockmode_t mode;
	u_int32_t flag, lockid;
	int freeobj, optindex, result, ret;
	char newname[MSG_SIZE];

	result = TCL_OK;
	freeobj = 0;
	memset(newname, 0, MSG_SIZE);
	if (objc != 5 && objc != 6) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-nowait? mode id obj");
		return (TCL_ERROR);
	}
	/*
	 * Work back from required args.
	 * Last arg is obj.
	 * Second last is lock id.
	 * Third last is lock mode.
	 */
	memset(&obj, 0, sizeof(obj));

	if ((result =
	    _GetUInt32(interp, objv[objc-2], &lockid)) != TCL_OK)
		return (result);

	ret = _CopyObjBytes(interp, objv[objc-1], &otmp,
	    &obj.size, &freeobj);
	if (ret != 0) {
		result = _ReturnSetup(interp, ret,
		    DB_RETOK_STD(ret), "lock get");
		return (result);
	}
	obj.data = otmp;
	if ((result = _LockMode(interp, objv[(objc - 3)], &mode)) != TCL_OK)
		goto out;

	/*
	 * Any left over arg is the flag.
	 */
	flag = 0;
	if (objc == 6) {
		if (Tcl_GetIndexFromObj(interp, objv[(objc - 4)],
		    lgopts, "option", TCL_EXACT, &optindex) != TCL_OK)
			return (IS_HELP(objv[(objc - 4)]));
		switch ((enum lgopts)optindex) {
		case LGNOWAIT:
			flag |= DB_LOCK_NOWAIT;
			break;
		}
	}

	result = _GetThisLock(interp, dbenv, lockid, flag, &obj, mode, newname);
	if (result == TCL_OK) {
		res = NewStringObj(newname, strlen(newname));
		Tcl_SetObjResult(interp, res);
	}
out:
	if (freeobj)
		__os_free(dbenv->env, otmp);
	return (result);
}

/*
 * tcl_LockStat --
 *
 * PUBLIC: int tcl_LockStat __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LockStat(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	DB_LOCK_STAT *sp;
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
	ret = dbenv->lock_stat(dbenv, &sp, 0);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "lock stat");
	if (result == TCL_ERROR)
		return (result);
	/*
	 * Have our stats, now construct the name value
	 * list pairs and free up the memory.
	 */
	res = Tcl_NewObj();
#ifdef HAVE_STATISTICS
	/*
	 * MAKE_STAT_LIST assumes 'res' and 'error' label.
	 */
	MAKE_STAT_LIST("Region size", sp->st_regsize);
	MAKE_STAT_LIST("Last allocated locker ID", sp->st_id);
	MAKE_STAT_LIST("Current maximum unused locker ID", sp->st_cur_maxid);
	MAKE_STAT_LIST("Maximum locks", sp->st_maxlocks);
	MAKE_STAT_LIST("Maximum lockers", sp->st_maxlockers);
	MAKE_STAT_LIST("Maximum objects", sp->st_maxobjects);
	MAKE_STAT_LIST("Lock modes", sp->st_nmodes);
	MAKE_STAT_LIST("Number of lock table partitions", sp->st_partitions);
	MAKE_STAT_LIST("Current number of locks", sp->st_nlocks);
	MAKE_STAT_LIST("Maximum number of locks so far", sp->st_maxnlocks);
	MAKE_STAT_LIST("Maximum number of locks in any hash bucket",
	    sp->st_maxhlocks);
	MAKE_WSTAT_LIST("Maximum number of lock steals for an empty partition",
	    sp->st_locksteals);
	MAKE_WSTAT_LIST("Maximum number lock steals in any partition",
	    sp->st_maxlsteals);
	MAKE_STAT_LIST("Current number of lockers", sp->st_nlockers);
	MAKE_STAT_LIST("Maximum number of lockers so far", sp->st_maxnlockers);
	MAKE_STAT_LIST("Current number of objects", sp->st_nobjects);
	MAKE_STAT_LIST("Maximum number of objects so far", sp->st_maxnobjects);
	MAKE_STAT_LIST("Maximum number of objects in any hash bucket",
	    sp->st_maxhobjects);
	MAKE_WSTAT_LIST("Maximum number of object steals for an empty partition",
	    sp->st_objectsteals);
	MAKE_WSTAT_LIST("Maximum number object steals in any partition",
	    sp->st_maxosteals);
	MAKE_WSTAT_LIST("Lock requests", sp->st_nrequests);
	MAKE_WSTAT_LIST("Lock releases", sp->st_nreleases);
	MAKE_WSTAT_LIST("Lock upgrades", sp->st_nupgrade);
	MAKE_WSTAT_LIST("Lock downgrades", sp->st_ndowngrade);
	MAKE_STAT_LIST("Number of conflicted locks for which we waited",
	    sp->st_lock_wait);
	MAKE_STAT_LIST("Number of conflicted locks for which we did not wait",
	    sp->st_lock_nowait);
	MAKE_WSTAT_LIST("Deadlocks detected", sp->st_ndeadlocks);
	MAKE_WSTAT_LIST("Number of region lock waits", sp->st_region_wait);
	MAKE_WSTAT_LIST("Number of region lock nowaits", sp->st_region_nowait);
	MAKE_WSTAT_LIST("Number of object allocation waits", sp->st_objs_wait);
	MAKE_STAT_LIST("Number of object allocation nowaits",
	    sp->st_objs_nowait);
	MAKE_STAT_LIST("Number of locker allocation waits",
	    sp->st_lockers_wait);
	MAKE_STAT_LIST("Number of locker allocation nowaits",
	    sp->st_lockers_nowait);
	MAKE_WSTAT_LIST("Maximum hash bucket length", sp->st_hash_len);
	MAKE_STAT_LIST("Lock timeout value", sp->st_locktimeout);
	MAKE_WSTAT_LIST("Number of lock timeouts", sp->st_nlocktimeouts);
	MAKE_STAT_LIST("Transaction timeout value", sp->st_txntimeout);
	MAKE_WSTAT_LIST("Number of transaction timeouts", sp->st_ntxntimeouts);
	MAKE_WSTAT_LIST("Number lock partition mutex waits", sp->st_part_wait);
	MAKE_STAT_LIST("Number lock partition mutex nowaits",
	    sp->st_part_nowait);
	MAKE_STAT_LIST("Maximum number waits on any lock partition mutex",
	    sp->st_part_max_wait);
	MAKE_STAT_LIST("Maximum number nowaits on any lock partition mutex",
	    sp->st_part_max_nowait);
#endif
	Tcl_SetObjResult(interp, res);
error:
	__os_ufree(dbenv->env, sp);
	return (result);
}

/*
 * tcl_LockTimeout --
 *
 * PUBLIC: int tcl_LockTimeout __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LockTimeout(interp, objc, objv, dbenv)
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
	ret = dbenv->set_timeout(dbenv, (u_int32_t)timeout,
	    DB_SET_LOCK_TIMEOUT);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "lock timeout");
	return (result);
}

/*
 * lock_Cmd --
 *	Implements the "lock" widget.
 */
static int
lock_Cmd(clientData, interp, objc, objv)
	ClientData clientData;		/* Lock handle */
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *lkcmds[] = {
		"put",
		NULL
	};
	enum lkcmds {
		LKPUT
	};
	DB_ENV *dbenv;
	DB_LOCK *lock;
	DBTCL_INFO *lkip;
	int cmdindex, result, ret;

	Tcl_ResetResult(interp);
	lock = (DB_LOCK *)clientData;
	lkip = _PtrToInfo((void *)lock);
	result = TCL_OK;

	if (lock == NULL) {
		Tcl_SetResult(interp, "NULL lock", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (lkip == NULL) {
		Tcl_SetResult(interp, "NULL lock info pointer", TCL_STATIC);
		return (TCL_ERROR);
	}

	dbenv = NAME_TO_ENV(lkip->i_parent->i_name);
	/*
	 * No args for this.  Error if there are some.
	 */
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return (TCL_ERROR);
	}
	/*
	 * Get the command name index from the object based on the dbcmds
	 * defined above.
	 */
	if (Tcl_GetIndexFromObj(interp,
	    objv[1], lkcmds, "command", TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));

	switch ((enum lkcmds)cmdindex) {
	case LKPUT:
		_debug_check();
		ret = dbenv->lock_put(dbenv, lock);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "lock put");
		(void)Tcl_DeleteCommand(interp, lkip->i_name);
		_DeleteInfo(lkip);
		__os_free(dbenv->env, lock);
		break;
	}
	return (result);
}

/*
 * tcl_LockVec --
 *
 * PUBLIC: int tcl_LockVec __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_LockVec(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* environment pointer */
{
	static const char *lvopts[] = {
		"-nowait",
		 NULL
	};
	enum lvopts {
		LVNOWAIT
	};
	static const char *lkops[] = {
		"get",
		"put",
		"put_all",
		"put_obj",
		"timeout",
		 NULL
	};
	enum lkops {
		LKGET,
		LKPUT,
		LKPUTALL,
		LKPUTOBJ,
		LKTIMEOUT
	};

	DB_LOCK *lock;
	DB_LOCKREQ list;
	DBT obj;
	Tcl_Obj **myobjv, *res, *thisop;
	void *otmp;
	u_int32_t flag, lockid;
	int freeobj, i, myobjc, optindex, result, ret;
	char *lockname, msg[MSG_SIZE], newname[MSG_SIZE];

	result = TCL_OK;
	memset(newname, 0, MSG_SIZE);
	memset(&list, 0, sizeof(DB_LOCKREQ));
	flag = 0;
	freeobj = 0;
	otmp = NULL;

	/*
	 * If -nowait is given, it MUST be first arg.
	 */
	if (Tcl_GetIndexFromObj(interp, objv[2],
	    lvopts, "option", TCL_EXACT, &optindex) == TCL_OK) {
		switch ((enum lvopts)optindex) {
		case LVNOWAIT:
			flag |= DB_LOCK_NOWAIT;
			break;
		}
		i = 3;
	} else {
		if (IS_HELP(objv[2]) == TCL_OK)
			return (TCL_OK);
		Tcl_ResetResult(interp);
		i = 2;
	}

	/*
	 * Our next arg MUST be the locker ID.
	 */
	result = _GetUInt32(interp, objv[i++], &lockid);
	if (result != TCL_OK)
		return (result);

	/*
	 * All other remaining args are operation tuples.
	 * Go through sequentially to decode, execute and build
	 * up list of return values.
	 */
	res = Tcl_NewListObj(0, NULL);
	while (i < objc) {
		/*
		 * Get the list of the tuple.
		 */
		lock = NULL;
		result = Tcl_ListObjGetElements(interp, objv[i],
		    &myobjc, &myobjv);
		if (result == TCL_OK)
			i++;
		else
			break;
		/*
		 * First we will set up the list of requests.
		 * We will make a "second pass" after we get back
		 * the results from the lock_vec call to create
		 * the return list.
		 */
		if (Tcl_GetIndexFromObj(interp, myobjv[0],
		    lkops, "option", TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(myobjv[0]);
			goto error;
		}
		switch ((enum lkops)optindex) {
		case LKGET:
			if (myobjc != 3) {
				Tcl_WrongNumArgs(interp, 1, myobjv,
				    "{get obj mode}");
				result = TCL_ERROR;
				goto error;
			}
			result = _LockMode(interp, myobjv[2], &list.mode);
			if (result != TCL_OK)
				goto error;
			ret = _CopyObjBytes(interp, myobjv[1], &otmp,
			    &obj.size, &freeobj);
			if (ret != 0) {
				result = _ReturnSetup(interp, ret,
				    DB_RETOK_STD(ret), "lock vec");
				return (result);
			}
			obj.data = otmp;
			ret = _GetThisLock(interp, dbenv, lockid, flag,
			    &obj, list.mode, newname);
			if (ret != 0) {
				result = _ReturnSetup(interp, ret,
				    DB_RETOK_STD(ret), "lock vec");
				thisop = Tcl_NewIntObj(ret);
				(void)Tcl_ListObjAppendElement(interp, res,
				    thisop);
				goto error;
			}
			thisop = NewStringObj(newname, strlen(newname));
			(void)Tcl_ListObjAppendElement(interp, res, thisop);
			if (freeobj && otmp != NULL) {
				__os_free(dbenv->env, otmp);
				freeobj = 0;
			}
			continue;
		case LKPUT:
			if (myobjc != 2) {
				Tcl_WrongNumArgs(interp, 1, myobjv,
				    "{put lock}");
				result = TCL_ERROR;
				goto error;
			}
			list.op = DB_LOCK_PUT;
			lockname = Tcl_GetStringFromObj(myobjv[1], NULL);
			lock = NAME_TO_LOCK(lockname);
			if (lock == NULL) {
				snprintf(msg, MSG_SIZE, "Invalid lock: %s\n",
				    lockname);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
				goto error;
			}
			list.lock = *lock;
			break;
		case LKPUTALL:
			if (myobjc != 1) {
				Tcl_WrongNumArgs(interp, 1, myobjv,
				    "{put_all}");
				result = TCL_ERROR;
				goto error;
			}
			list.op = DB_LOCK_PUT_ALL;
			break;
		case LKPUTOBJ:
			if (myobjc != 2) {
				Tcl_WrongNumArgs(interp, 1, myobjv,
				    "{put_obj obj}");
				result = TCL_ERROR;
				goto error;
			}
			list.op = DB_LOCK_PUT_OBJ;
			ret = _CopyObjBytes(interp, myobjv[1], &otmp,
			    &obj.size, &freeobj);
			if (ret != 0) {
				result = _ReturnSetup(interp, ret,
				    DB_RETOK_STD(ret), "lock vec");
				return (result);
			}
			obj.data = otmp;
			list.obj = &obj;
			break;
		case LKTIMEOUT:
			list.op = DB_LOCK_TIMEOUT;
			break;

		}
		/*
		 * We get here, we have set up our request, now call
		 * lock_vec.
		 */
		_debug_check();
		ret = dbenv->lock_vec(dbenv, lockid, flag, &list, 1, NULL);
		/*
		 * Now deal with whether or not the operation succeeded.
		 * Get's were done above, all these are only puts.
		 */
		thisop = Tcl_NewIntObj(ret);
		result = Tcl_ListObjAppendElement(interp, res, thisop);
		if (ret != 0 && result == TCL_OK)
			result = _ReturnSetup(interp, ret,
			    DB_RETOK_STD(ret), "lock put");
		if (freeobj && otmp != NULL) {
			__os_free(dbenv->env, otmp);
			freeobj = 0;
		}
		/*
		 * We did a put of some kind.  Since we did that,
		 * we have to delete the commands associated with
		 * any of the locks we just put.
		 */
		_LockPutInfo(interp, list.op, lock, lockid, &obj);
	}

	if (result == TCL_OK && res)
		Tcl_SetObjResult(interp, res);
error:
	return (result);
}

static int
_LockMode(interp, obj, mode)
	Tcl_Interp *interp;
	Tcl_Obj *obj;
	db_lockmode_t *mode;
{
	static const char *lkmode[] = {
		"ng",
		"read",
		"write",
		"iwrite",
		"iread",
		"iwr",
		 NULL
	};
	enum lkmode {
		LK_NG,
		LK_READ,
		LK_WRITE,
		LK_IWRITE,
		LK_IREAD,
		LK_IWR
	};
	int optindex;

	if (Tcl_GetIndexFromObj(interp, obj, lkmode, "option",
	    TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(obj));
	switch ((enum lkmode)optindex) {
	case LK_NG:
		*mode = DB_LOCK_NG;
		break;
	case LK_READ:
		*mode = DB_LOCK_READ;
		break;
	case LK_WRITE:
		*mode = DB_LOCK_WRITE;
		break;
	case LK_IREAD:
		*mode = DB_LOCK_IREAD;
		break;
	case LK_IWRITE:
		*mode = DB_LOCK_IWRITE;
		break;
	case LK_IWR:
		*mode = DB_LOCK_IWR;
		break;
	}
	return (TCL_OK);
}

static void
_LockPutInfo(interp, op, lock, lockid, objp)
	Tcl_Interp *interp;
	db_lockop_t op;
	DB_LOCK *lock;
	u_int32_t lockid;
	DBT *objp;
{
	DBTCL_INFO *p, *nextp;
	int found;

	for (p = LIST_FIRST(&__db_infohead); p != NULL; p = nextp) {
		found = 0;
		nextp = LIST_NEXT(p, entries);
		if ((op == DB_LOCK_PUT && (p->i_lock == lock)) ||
		    (op == DB_LOCK_PUT_ALL && p->i_locker == lockid) ||
		    (op == DB_LOCK_PUT_OBJ && p->i_lockobj.data &&
			memcmp(p->i_lockobj.data, objp->data, objp->size) == 0))
			found = 1;
		if (found) {
			(void)Tcl_DeleteCommand(interp, p->i_name);
			__os_free(NULL, p->i_lock);
			_DeleteInfo(p);
		}
	}
}

static int
_GetThisLock(interp, dbenv, lockid, flag, objp, mode, newname)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Env handle */
	u_int32_t lockid;		/* Locker ID */
	u_int32_t flag;			/* Lock flag */
	DBT *objp;			/* Object to lock */
	db_lockmode_t mode;		/* Lock mode */
	char *newname;			/* New command name */
{
	DBTCL_INFO *envip, *ip;
	DB_LOCK *lock;
	int result, ret;

	result = TCL_OK;
	envip = _PtrToInfo((void *)dbenv);
	if (envip == NULL) {
		Tcl_SetResult(interp, "Could not find env info\n", TCL_STATIC);
		return (TCL_ERROR);
	}
	snprintf(newname, MSG_SIZE, "%s.lock%d",
	    envip->i_name, envip->i_envlockid);
	ip = _NewInfo(interp, NULL, newname, I_LOCK);
	if (ip == NULL) {
		Tcl_SetResult(interp, "Could not set up info",
		    TCL_STATIC);
		return (TCL_ERROR);
	}
	ret = __os_malloc(dbenv->env, sizeof(DB_LOCK), &lock);
	if (ret != 0) {
		Tcl_SetResult(interp, db_strerror(ret), TCL_STATIC);
		return (TCL_ERROR);
	}
	_debug_check();
	ret = dbenv->lock_get(dbenv, lockid, flag, objp, mode, lock);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "lock get");
	if (result == TCL_ERROR) {
		__os_free(dbenv->env, lock);
		_DeleteInfo(ip);
		return (result);
	}
	/*
	 * Success.  Set up return.  Set up new info
	 * and command widget for this lock.
	 */
	ret = __os_malloc(dbenv->env, objp->size, &ip->i_lockobj.data);
	if (ret != 0) {
		Tcl_SetResult(interp, "Could not duplicate obj",
		    TCL_STATIC);
		(void)dbenv->lock_put(dbenv, lock);
		__os_free(dbenv->env, lock);
		_DeleteInfo(ip);
		result = TCL_ERROR;
		goto error;
	}
	memcpy(ip->i_lockobj.data, objp->data, objp->size);
	ip->i_lockobj.size = objp->size;
	envip->i_envlockid++;
	ip->i_parent = envip;
	ip->i_locker = lockid;
	_SetInfoData(ip, lock);
	(void)Tcl_CreateObjCommand(interp, newname,
	    (Tcl_ObjCmdProc *)lock_Cmd, (ClientData)lock, NULL);
error:
	return (result);
}
#endif
