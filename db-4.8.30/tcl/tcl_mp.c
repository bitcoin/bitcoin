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
static int      mp_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
static int      pg_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
static int      tcl_MpGet __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
    DB_MPOOLFILE *, DBTCL_INFO *));
static int      tcl_Pg __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
    void *, DB_MPOOLFILE *, DBTCL_INFO *));
static int      tcl_PgInit __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
    void *, DBTCL_INFO *));
static int      tcl_PgIsset __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
    void *, DBTCL_INFO *));
#endif

/*
 * _MpInfoDelete --
 *	Removes "sub" mp page info structures that are children
 *	of this mp.
 *
 * PUBLIC: void _MpInfoDelete __P((Tcl_Interp *, DBTCL_INFO *));
 */
void
_MpInfoDelete(interp, mpip)
	Tcl_Interp *interp;		/* Interpreter */
	DBTCL_INFO *mpip;		/* Info for mp */
{
	DBTCL_INFO *nextp, *p;

	for (p = LIST_FIRST(&__db_infohead); p != NULL; p = nextp) {
		/*
		 * Check if this info structure "belongs" to this
		 * mp.  Remove its commands and info structure.
		 */
		nextp = LIST_NEXT(p, entries);
		if (p->i_parent == mpip && p->i_type == I_PG) {
			(void)Tcl_DeleteCommand(interp, p->i_name);
			_DeleteInfo(p);
		}
	}
}

#ifdef CONFIG_TEST
/*
 * tcl_MpSync --
 *
 * PUBLIC: int tcl_MpSync __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_MpSync(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{

	DB_LSN lsn, *lsnp;
	int result, ret;

	result = TCL_OK;
	lsnp = NULL;
	/*
	 * No flags, must be 3 args.
	 */
	if (objc == 3) {
		result = _GetLsn(interp, objv[2], &lsn);
		if (result == TCL_ERROR)
			return (result);
		lsnp = &lsn;
	}
	else if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "lsn");
		return (TCL_ERROR);
	}

	_debug_check();
	ret = dbenv->memp_sync(dbenv, lsnp);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "memp sync"));
}

/*
 * tcl_MpTrickle --
 *
 * PUBLIC: int tcl_MpTrickle __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_MpTrickle(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{

	Tcl_Obj *res;
	int pages, percent, result, ret;

	result = TCL_OK;
	/*
	 * No flags, must be 3 args.
	 */
	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "percent");
		return (TCL_ERROR);
	}

	result = Tcl_GetIntFromObj(interp, objv[2], &percent);
	if (result == TCL_ERROR)
		return (result);

	_debug_check();
	ret = dbenv->memp_trickle(dbenv, percent, &pages);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "memp trickle");
	if (result == TCL_ERROR)
		return (result);

	res = Tcl_NewIntObj(pages);
	Tcl_SetObjResult(interp, res);
	return (result);

}

/*
 * tcl_Mp --
 *
 * PUBLIC: int tcl_Mp __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *, DBTCL_INFO *));
 */
int
tcl_Mp(interp, objc, objv, dbenv, envip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
	DBTCL_INFO *envip;		/* Info pointer */
{
	static const char *mpopts[] = {
		"-create",
		"-mode",
		"-multiversion",
		"-nommap",
		"-pagesize",
		"-rdonly",
		 NULL
	};
	enum mpopts {
		MPCREATE,
		MPMODE,
		MPMULTIVERSION,
		MPNOMMAP,
		MPPAGE,
		MPRDONLY
	};
	DBTCL_INFO *ip;
	DB_MPOOLFILE *mpf;
	Tcl_Obj *res;
	u_int32_t flag;
	int i, pgsize, mode, optindex, result, ret;
	char *file, newname[MSG_SIZE];

	result = TCL_OK;
	i = 2;
	flag = 0;
	mode = 0;
	pgsize = 0;
	memset(newname, 0, MSG_SIZE);
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i],
		    mpopts, "option", TCL_EXACT, &optindex) != TCL_OK) {
			/*
			 * Reset the result so we don't get an errant
			 * error message if there is another error.
			 * This arg is the file name.
			 */
			if (IS_HELP(objv[i]) == TCL_OK)
				return (TCL_OK);
			Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum mpopts)optindex) {
		case MPCREATE:
			flag |= DB_CREATE;
			break;
		case MPMODE:
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
		case MPMULTIVERSION:
			flag |= DB_MULTIVERSION;
			break;
		case MPNOMMAP:
			flag |= DB_NOMMAP;
			break;
		case MPPAGE:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-pagesize size?");
				result = TCL_ERROR;
				break;
			}
			/*
			 * Don't need to check result here because
			 * if TCL_ERROR, the error message is already
			 * set up, and we'll bail out below.  If ok,
			 * the mode is set and we go on.
			 */
			result = Tcl_GetIntFromObj(interp, objv[i++], &pgsize);
			break;
		case MPRDONLY:
			flag |= DB_RDONLY;
			break;
		}
		if (result != TCL_OK)
			goto error;
	}
	/*
	 * Any left over arg is a file name.  It better be the last arg.
	 */
	file = NULL;
	if (i != objc) {
		if (i != objc - 1) {
			Tcl_WrongNumArgs(interp, 2, objv, "?args? ?file?");
			result = TCL_ERROR;
			goto error;
		}
		file = Tcl_GetStringFromObj(objv[i++], NULL);
	}

	snprintf(newname, sizeof(newname), "%s.mp%d",
	    envip->i_name, envip->i_envmpid);
	ip = _NewInfo(interp, NULL, newname, I_MP);
	if (ip == NULL) {
		Tcl_SetResult(interp, "Could not set up info",
		    TCL_STATIC);
		return (TCL_ERROR);
	}

	_debug_check();
	if ((ret = dbenv->memp_fcreate(dbenv, &mpf, 0)) != 0) {
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "mpool");
		_DeleteInfo(ip);
		goto error;
	}

	/*
	 * XXX
	 * Interface doesn't currently support DB_MPOOLFILE configuration.
	 */
	if ((ret = mpf->open(mpf, file, flag, mode, (size_t)pgsize)) != 0) {
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "mpool");
		_DeleteInfo(ip);

		(void)mpf->close(mpf, 0);
		goto error;
	}

	/*
	 * Success.  Set up return.  Set up new info and command widget for
	 * this mpool.
	 */
	envip->i_envmpid++;
	ip->i_parent = envip;
	ip->i_pgsz = pgsize;
	_SetInfoData(ip, mpf);
	(void)Tcl_CreateObjCommand(interp, newname,
	    (Tcl_ObjCmdProc *)mp_Cmd, (ClientData)mpf, NULL);
	res = NewStringObj(newname, strlen(newname));
	Tcl_SetObjResult(interp, res);

error:
	return (result);
}

/*
 * tcl_MpStat --
 *
 * PUBLIC: int tcl_MpStat __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_MpStat(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	DB_MPOOL_FSTAT **fsp, **savefsp;
	DB_MPOOL_STAT *sp;
	int result;
	int ret;
	Tcl_Obj *res;
	Tcl_Obj *res1;

	result = TCL_OK;
	savefsp = NULL;
	/*
	 * No args for this.  Error if there are some.
	 */
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return (TCL_ERROR);
	}
	_debug_check();
	ret = dbenv->memp_stat(dbenv, &sp, &fsp, 0);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "memp stat");
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
	MAKE_STAT_LIST("Cache size (gbytes)", sp->st_gbytes);
	MAKE_STAT_LIST("Cache size (bytes)", sp->st_bytes);
	MAKE_STAT_LIST("Number of caches", sp->st_ncache);
	MAKE_STAT_LIST("Maximum number of caches", sp->st_max_ncache);
	MAKE_STAT_LIST("Region size", sp->st_regsize);
	MAKE_STAT_LIST("Maximum memory-mapped file size", sp->st_mmapsize);
	MAKE_STAT_LIST("Maximum open file descriptors", sp->st_maxopenfd);
	MAKE_STAT_LIST("Maximum sequential buffer writes", sp->st_maxwrite);
	MAKE_STAT_LIST(
	    "Sleep after writing maximum buffers", sp->st_maxwrite_sleep);
	MAKE_STAT_LIST("Pages mapped into address space", sp->st_map);
	MAKE_WSTAT_LIST("Cache hits", sp->st_cache_hit);
	MAKE_WSTAT_LIST("Cache misses", sp->st_cache_miss);
	MAKE_WSTAT_LIST("Pages created", sp->st_page_create);
	MAKE_WSTAT_LIST("Pages read in", sp->st_page_in);
	MAKE_WSTAT_LIST("Pages written", sp->st_page_out);
	MAKE_WSTAT_LIST("Clean page evictions", sp->st_ro_evict);
	MAKE_WSTAT_LIST("Dirty page evictions", sp->st_rw_evict);
	MAKE_WSTAT_LIST("Dirty pages trickled", sp->st_page_trickle);
	MAKE_STAT_LIST("Cached pages", sp->st_pages);
	MAKE_WSTAT_LIST("Cached clean pages", sp->st_page_clean);
	MAKE_WSTAT_LIST("Cached dirty pages", sp->st_page_dirty);
	MAKE_WSTAT_LIST("Hash buckets", sp->st_hash_buckets);
	MAKE_WSTAT_LIST("Default pagesize", sp->st_pagesize);
	MAKE_WSTAT_LIST("Hash lookups", sp->st_hash_searches);
	MAKE_WSTAT_LIST("Longest hash chain found", sp->st_hash_longest);
	MAKE_WSTAT_LIST("Hash elements examined", sp->st_hash_examined);
	MAKE_WSTAT_LIST("Number of hash bucket nowaits", sp->st_hash_nowait);
	MAKE_WSTAT_LIST("Number of hash bucket waits", sp->st_hash_wait);
	MAKE_STAT_LIST("Maximum number of hash bucket nowaits",
	    sp->st_hash_max_nowait);
	MAKE_STAT_LIST("Maximum number of hash bucket waits",
	    sp->st_hash_max_wait);
	MAKE_WSTAT_LIST("Number of region lock nowaits", sp->st_region_nowait);
	MAKE_WSTAT_LIST("Number of region lock waits", sp->st_region_wait);
	MAKE_WSTAT_LIST("Buffers frozen", sp->st_mvcc_frozen);
	MAKE_WSTAT_LIST("Buffers thawed", sp->st_mvcc_thawed);
	MAKE_WSTAT_LIST("Frozen buffers freed", sp->st_mvcc_freed);
	MAKE_WSTAT_LIST("Page allocations", sp->st_alloc);
	MAKE_STAT_LIST("Buckets examined during allocation",
	    sp->st_alloc_buckets);
	MAKE_STAT_LIST("Maximum buckets examined during allocation",
	    sp->st_alloc_max_buckets);
	MAKE_WSTAT_LIST("Pages examined during allocation", sp->st_alloc_pages);
	MAKE_STAT_LIST("Maximum pages examined during allocation",
	    sp->st_alloc_max_pages);
	MAKE_WSTAT_LIST("Threads waiting on buffer I/O", sp->st_io_wait);
	MAKE_WSTAT_LIST("Number of syncs interrupted", sp->st_sync_interrupted);

	/*
	 * Save global stat list as res1.  The MAKE_STAT_LIST
	 * macro assumes 'res' so we'll use that to build up
	 * our per-file sublist.
	 */
	res1 = res;
	for (savefsp = fsp; fsp != NULL && *fsp != NULL; fsp++) {
		res = Tcl_NewObj();
		MAKE_STAT_STRLIST("File Name", (*fsp)->file_name);
		MAKE_STAT_LIST("Page size", (*fsp)->st_pagesize);
		MAKE_STAT_LIST("Pages mapped into address space",
		    (*fsp)->st_map);
		MAKE_WSTAT_LIST("Cache hits", (*fsp)->st_cache_hit);
		MAKE_WSTAT_LIST("Cache misses", (*fsp)->st_cache_miss);
		MAKE_WSTAT_LIST("Pages created", (*fsp)->st_page_create);
		MAKE_WSTAT_LIST("Pages read in", (*fsp)->st_page_in);
		MAKE_WSTAT_LIST("Pages written", (*fsp)->st_page_out);
		/*
		 * Now that we have a complete "per-file" stat list, append
		 * that to the other list.
		 */
		result = Tcl_ListObjAppendElement(interp, res1, res);
		if (result != TCL_OK)
			goto error;
	}
#endif
	Tcl_SetObjResult(interp, res1);
error:
	__os_ufree(dbenv->env, sp);
	if (savefsp != NULL)
		__os_ufree(dbenv->env, savefsp);
	return (result);
}

/*
 * mp_Cmd --
 *	Implements the "mp" widget.
 */
static int
mp_Cmd(clientData, interp, objc, objv)
	ClientData clientData;		/* Mp handle */
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *mpcmds[] = {
		"close",
		"fsync",
		"get",
		"get_clear_len",
		"get_fileid",
		"get_ftype",
		"get_lsn_offset",
		"get_pgcookie",
		NULL
	};
	enum mpcmds {
		MPCLOSE,
		MPFSYNC,
		MPGET,
		MPGETCLEARLEN,
		MPGETFILEID,
		MPGETFTYPE,
		MPGETLSNOFFSET,
		MPGETPGCOOKIE
	};
	DB_MPOOLFILE *mp;
	int cmdindex, ftype, length, result, ret;
	DBTCL_INFO *mpip;
	Tcl_Obj *res;
	char *obj_name;
	u_int32_t value;
	int32_t intval;
	u_int8_t fileid[DB_FILE_ID_LEN];
	DBT cookie;

	Tcl_ResetResult(interp);
	mp = (DB_MPOOLFILE *)clientData;
	obj_name = Tcl_GetStringFromObj(objv[0], &length);
	mpip = _NameToInfo(obj_name);
	result = TCL_OK;

	if (mp == NULL) {
		Tcl_SetResult(interp, "NULL mp pointer", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (mpip == NULL) {
		Tcl_SetResult(interp, "NULL mp info pointer", TCL_STATIC);
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the dbcmds
	 * defined above.
	 */
	if (Tcl_GetIndexFromObj(interp,
	    objv[1], mpcmds, "command", TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));

	res = NULL;
	switch ((enum mpcmds)cmdindex) {
	case MPCLOSE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = mp->close(mp, 0);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "mp close");
		_MpInfoDelete(interp, mpip);
		(void)Tcl_DeleteCommand(interp, mpip->i_name);
		_DeleteInfo(mpip);
		break;
	case MPFSYNC:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = mp->sync(mp);
		res = Tcl_NewIntObj(ret);
		break;
	case MPGET:
		result = tcl_MpGet(interp, objc, objv, mp, mpip);
		break;
	case MPGETCLEARLEN:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = mp->get_clear_len(mp, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "mp get_clear_len")) == TCL_OK)
			res = Tcl_NewIntObj((int)value);
		break;
	case MPGETFILEID:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = mp->get_fileid(mp, fileid);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "mp get_fileid")) == TCL_OK)
			res = NewStringObj((char *)fileid, DB_FILE_ID_LEN);
		break;
	case MPGETFTYPE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = mp->get_ftype(mp, &ftype);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "mp get_ftype")) == TCL_OK)
			res = Tcl_NewIntObj(ftype);
		break;
	case MPGETLSNOFFSET:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = mp->get_lsn_offset(mp, &intval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "mp get_lsn_offset")) == TCL_OK)
			res = Tcl_NewIntObj(intval);
		break;
	case MPGETPGCOOKIE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		memset(&cookie, 0, sizeof(DBT));
		ret = mp->get_pgcookie(mp, &cookie);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "mp get_pgcookie")) == TCL_OK)
			res = Tcl_NewByteArrayObj((u_char *)cookie.data,
			    (int)cookie.size);
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
 * tcl_MpGet --
 */
static int
tcl_MpGet(interp, objc, objv, mp, mpip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_MPOOLFILE *mp;		/* mp pointer */
	DBTCL_INFO *mpip;		/* mp info pointer */
{
	static const char *mpget[] = {
		"-create",
		"-dirty",
		"-last",
		"-new",
		"-txn",
		NULL
	};
	enum mpget {
		MPGET_CREATE,
		MPGET_DIRTY,
		MPGET_LAST,
		MPGET_NEW,
		MPGET_TXN
	};

	DBTCL_INFO *ip;
	Tcl_Obj *res;
	DB_TXN *txn;
	db_pgno_t pgno;
	u_int32_t flag;
	int i, ipgno, optindex, result, ret;
	char *arg, msg[MSG_SIZE], newname[MSG_SIZE];
	void *page;

	txn = NULL;
	result = TCL_OK;
	memset(newname, 0, MSG_SIZE);
	i = 2;
	flag = 0;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i],
		    mpget, "option", TCL_EXACT, &optindex) != TCL_OK) {
			/*
			 * Reset the result so we don't get an errant
			 * error message if there is another error.
			 * This arg is the page number.
			 */
			if (IS_HELP(objv[i]) == TCL_OK)
				return (TCL_OK);
			Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum mpget)optindex) {
		case MPGET_CREATE:
			flag |= DB_MPOOL_CREATE;
			break;
		case MPGET_DIRTY:
			flag |= DB_MPOOL_DIRTY;
			break;
		case MPGET_LAST:
			flag |= DB_MPOOL_LAST;
			break;
		case MPGET_NEW:
			flag |= DB_MPOOL_NEW;
			break;
		case MPGET_TXN:
			if (i == objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "mpool get: Invalid txn: %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				result = TCL_ERROR;
			}
			break;
		}
		if (result != TCL_OK)
			goto error;
	}
	/*
	 * Any left over arg is a page number.  It better be the last arg.
	 */
	ipgno = 0;
	if (i != objc) {
		if (i != objc - 1) {
			Tcl_WrongNumArgs(interp, 2, objv, "?args? ?pgno?");
			result = TCL_ERROR;
			goto error;
		}
		result = Tcl_GetIntFromObj(interp, objv[i++], &ipgno);
		if (result != TCL_OK)
			goto error;
	}

	snprintf(newname, sizeof(newname), "%s.pg%d",
	    mpip->i_name, mpip->i_mppgid);
	ip = _NewInfo(interp, NULL, newname, I_PG);
	if (ip == NULL) {
		Tcl_SetResult(interp, "Could not set up info",
		    TCL_STATIC);
		return (TCL_ERROR);
	}
	_debug_check();
	pgno = (db_pgno_t)ipgno;
	ret = mp->get(mp, &pgno, NULL, flag, &page);
	result = _ReturnSetup(interp, ret, DB_RETOK_MPGET(ret), "mpool get");
	if (result == TCL_ERROR)
		_DeleteInfo(ip);
	else {
		/*
		 * Success.  Set up return.  Set up new info
		 * and command widget for this mpool.
		 */
		mpip->i_mppgid++;
		ip->i_parent = mpip;
		ip->i_pgno = pgno;
		ip->i_pgsz = mpip->i_pgsz;
		_SetInfoData(ip, page);
		(void)Tcl_CreateObjCommand(interp, newname,
		    (Tcl_ObjCmdProc *)pg_Cmd, (ClientData)page, NULL);
		res = NewStringObj(newname, strlen(newname));
		Tcl_SetObjResult(interp, res);
	}
error:
	return (result);
}

/*
 * pg_Cmd --
 *	Implements the "pg" widget.
 */
static int
pg_Cmd(clientData, interp, objc, objv)
	ClientData clientData;		/* Page handle */
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *pgcmds[] = {
		"init",
		"is_setto",
		"pgnum",
		"pgsize",
		"put",
		NULL
	};
	enum pgcmds {
		PGINIT,
		PGISSET,
		PGNUM,
		PGSIZE,
		PGPUT
	};
	DB_MPOOLFILE *mp;
	int cmdindex, length, result;
	char *obj_name;
	void *page;
	DBTCL_INFO *pgip;
	Tcl_Obj *res;

	Tcl_ResetResult(interp);
	page = (void *)clientData;
	obj_name = Tcl_GetStringFromObj(objv[0], &length);
	pgip = _NameToInfo(obj_name);
	mp = NAME_TO_MP(pgip->i_parent->i_name);
	result = TCL_OK;

	if (page == NULL) {
		Tcl_SetResult(interp, "NULL page pointer", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (mp == NULL) {
		Tcl_SetResult(interp, "NULL mp pointer", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (pgip == NULL) {
		Tcl_SetResult(interp, "NULL page info pointer", TCL_STATIC);
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the dbcmds
	 * defined above.
	 */
	if (Tcl_GetIndexFromObj(interp,
	    objv[1], pgcmds, "command", TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));

	res = NULL;
	switch ((enum pgcmds)cmdindex) {
	case PGNUM:
		res = Tcl_NewWideIntObj((Tcl_WideInt)pgip->i_pgno);
		break;
	case PGSIZE:
		res = Tcl_NewWideIntObj((Tcl_WideInt)pgip->i_pgsz);
		break;
	case PGPUT:
		result = tcl_Pg(interp, objc, objv, page, mp, pgip);
		break;
	case PGINIT:
		result = tcl_PgInit(interp, objc, objv, page, pgip);
		break;
	case PGISSET:
		result = tcl_PgIsset(interp, objc, objv, page, pgip);
		break;
	}

	/*
	 * Only set result if we have a res.  Otherwise, lower
	 * functions have already done so.
	 */
	if (result == TCL_OK && res != NULL)
		Tcl_SetObjResult(interp, res);
	return (result);
}

static int
tcl_Pg(interp, objc, objv, page, mp, pgip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	void *page;			/* Page pointer */
	DB_MPOOLFILE *mp;		/* Mpool pointer */
	DBTCL_INFO *pgip;		/* Info pointer */
{
	static const char *pgopt[] = {
		"-discard",
		NULL
	};
	enum pgopt {
		PGDISCARD
	};
	u_int32_t flag;
	int i, optindex, result, ret;

	result = TCL_OK;
	i = 2;
	flag = 0;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i],
		    pgopt, "option", TCL_EXACT, &optindex) != TCL_OK)
			return (IS_HELP(objv[i]));
		i++;
		switch ((enum pgopt)optindex) {
		case PGDISCARD:
			flag |= DB_MPOOL_DISCARD;
			break;
		}
	}

	_debug_check();
	ret = mp->put(mp, page, DB_PRIORITY_UNCHANGED, flag);

	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "page");

	(void)Tcl_DeleteCommand(interp, pgip->i_name);
	_DeleteInfo(pgip);
	return (result);
}

static int
tcl_PgInit(interp, objc, objv, page, pgip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	void *page;			/* Page pointer */
	DBTCL_INFO *pgip;		/* Info pointer */
{
	Tcl_Obj *res;
	long *p, *endp, newval;
	int length, pgsz, result;
	u_char *s;

	result = TCL_OK;
	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "val");
		return (TCL_ERROR);
	}

	pgsz = pgip->i_pgsz;
	result = Tcl_GetLongFromObj(interp, objv[2], &newval);
	if (result != TCL_OK) {
		s = Tcl_GetByteArrayFromObj(objv[2], &length);
		if (s == NULL)
			return (TCL_ERROR);
		memcpy(page, s, (size_t)((length < pgsz) ? length : pgsz));
		result = TCL_OK;
	} else {
		p = (long *)page;
		for (endp = p + ((u_int)pgsz / sizeof(long)); p < endp; p++)
			*p = newval;
	}
	res = Tcl_NewIntObj(0);
	Tcl_SetObjResult(interp, res);
	return (result);
}

static int
tcl_PgIsset(interp, objc, objv, page, pgip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	void *page;			/* Page pointer */
	DBTCL_INFO *pgip;		/* Info pointer */
{
	Tcl_Obj *res;
	long *p, *endp, newval;
	int length, pgsz, result;
	u_char *s;

	result = TCL_OK;
	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "val");
		return (TCL_ERROR);
	}

	pgsz = pgip->i_pgsz;
	result = Tcl_GetLongFromObj(interp, objv[2], &newval);
	if (result != TCL_OK) {
		if ((s = Tcl_GetByteArrayFromObj(objv[2], &length)) == NULL)
			return (TCL_ERROR);
		result = TCL_OK;

		if (memcmp(page, s,
		    (size_t)((length < pgsz) ? length : pgsz)) != 0) {
			res = Tcl_NewIntObj(0);
			Tcl_SetObjResult(interp, res);
			return (result);
		}
	} else {
		p = (long *)page;
		/*
		 * If any value is not the same, return 0 (is not set to
		 * this value).  Otherwise, if we finish the loop, we return 1
		 * (is set to this value).
		 */
		for (endp = p + ((u_int)pgsz / sizeof(long)); p < endp; p++)
			if (*p != newval) {
				res = Tcl_NewIntObj(0);
				Tcl_SetObjResult(interp, res);
				return (result);
			}
	}

	res = Tcl_NewIntObj(1);
	Tcl_SetObjResult(interp, res);
	return (result);
}
#endif
