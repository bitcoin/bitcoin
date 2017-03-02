/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/log.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

static int __absname __P((ENV *, char *, char *, char **));
static int __build_data __P((ENV *, char *, char ***));
static int __cmpfunc __P((const void *, const void *));
static int __log_archive __P((ENV *, char **[], u_int32_t));
static int __usermem __P((ENV *, char ***));

/*
 * __log_archive_pp --
 *	ENV->log_archive pre/post processing.
 *
 * PUBLIC: int __log_archive_pp __P((DB_ENV *, char **[], u_int32_t));
 */
int
__log_archive_pp(dbenv, listp, flags)
	DB_ENV *dbenv;
	char ***listp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lg_handle, "DB_ENV->log_archive", DB_INIT_LOG);

#define	OKFLAGS	(DB_ARCH_ABS | DB_ARCH_DATA | DB_ARCH_LOG | DB_ARCH_REMOVE)
	if (flags != 0) {
		if ((ret = __db_fchk(
		    env, "DB_ENV->log_archive", flags, OKFLAGS)) != 0)
			return (ret);
		if ((ret = __db_fcchk(env, "DB_ENV->log_archive",
		    flags, DB_ARCH_DATA, DB_ARCH_LOG)) != 0)
			return (ret);
		if ((ret = __db_fcchk(env, "DB_ENV->log_archive",
		    flags, DB_ARCH_REMOVE,
		    DB_ARCH_ABS | DB_ARCH_DATA | DB_ARCH_LOG)) != 0)
			return (ret);
	}

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_archive(env, listp, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __log_archive --
 *	ENV->log_archive.  Internal.
 */
static int
__log_archive(env, listp, flags)
	ENV *env;
	char ***listp;
	u_int32_t flags;
{
	DBT rec;
	DB_LOG *dblp;
	DB_LOGC *logc;
	DB_LSN stable_lsn;
	LOG *lp;
	u_int array_size, n;
	u_int32_t fnum;
	int ret, t_ret;
	char **array, **arrayp, *name, *p, *pref;
#ifdef HAVE_GETCWD
	char path[DB_MAXPATHLEN];
#endif

	dblp = env->lg_handle;
	lp = (LOG *)dblp->reginfo.primary;
	array = NULL;
	name = NULL;
	ret = 0;
	COMPQUIET(fnum, 0);

	if (flags != DB_ARCH_REMOVE)
		*listp = NULL;

	/* There are no log files if logs are in memory. */
	if (lp->db_log_inmemory) {
		LF_CLR(~DB_ARCH_DATA);
		if (flags == 0)
			return (0);
	}

	/*
	 * If the user wants the list of log files to remove and we're
	 * at a bad time in replication initialization, just return.
	 */
	if (!LF_ISSET(DB_ARCH_DATA) &&
	    !LF_ISSET(DB_ARCH_LOG) && __rep_noarchive(env))
		return (0);

	/*
	 * Prepend the original absolute pathname if the user wants an
	 * absolute path to the database environment directory.
	 */
#ifdef HAVE_GETCWD
	if (LF_ISSET(DB_ARCH_ABS)) {
		/*
		 * XXX
		 * Can't trust getcwd(3) to set a valid errno, so don't display
		 * one unless we know it's good.  It's likely a permissions
		 * problem: use something bland and useless in the default
		 * return value, so we don't send somebody off in the wrong
		 * direction.
		 */
		__os_set_errno(0);
		if (getcwd(path, sizeof(path)) == NULL) {
			ret = __os_get_errno();
			__db_err(env,
			    ret, "no absolute path for the current directory");
			return (ret);
		}
		pref = path;
	} else
#endif
		pref = NULL;

	LF_CLR(DB_ARCH_ABS);
	switch (flags) {
	case DB_ARCH_DATA:
		ret = __build_data(env, pref, listp);
		goto err;
	case DB_ARCH_LOG:
		memset(&rec, 0, sizeof(rec));
		if ((ret = __log_cursor(env, &logc)) != 0)
			goto err;
#ifdef UMRW
		ZERO_LSN(stable_lsn);
#endif
		ret = __logc_get(logc, &stable_lsn, &rec, DB_LAST);
		if ((t_ret = __logc_close(logc)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			goto err;
		fnum = stable_lsn.file;
		break;
	case DB_ARCH_REMOVE:
		__log_autoremove(env);
		goto err;
	case 0:

		ret = __log_get_stable_lsn(env, &stable_lsn);
		/*
		 * A return of DB_NOTFOUND means the checkpoint LSN
		 * is before the beginning of the log files we have.
		 * This is not an error; it just means we're done.
		 */
		if (ret != 0) {
			if (ret == DB_NOTFOUND)
				ret = 0;
			goto err;
		}
		/* Remove any log files before the last stable LSN. */
		fnum = stable_lsn.file - 1;
		break;
	default:
		ret = __db_unknown_path(env, "__log_archive");
		goto err;
	}

#define	LIST_INCREMENT	64
	/* Get some initial space. */
	array_size = 64;
	if ((ret = __os_malloc(env,
	    sizeof(char *) * array_size, &array)) != 0)
		goto err;
	array[0] = NULL;

	/* Build an array of the file names. */
	for (n = 0; fnum > 0; --fnum) {
		if ((ret = __log_name(dblp, fnum, &name, NULL, 0)) != 0) {
			__os_free(env, name);
			goto err;
		}
		if (__os_exists(env, name, NULL) != 0) {
			if (LF_ISSET(DB_ARCH_LOG) && fnum == stable_lsn.file)
				continue;
			__os_free(env, name);
			name = NULL;
			break;
		}

		if (n >= array_size - 2) {
			array_size += LIST_INCREMENT;
			if ((ret = __os_realloc(env,
			    sizeof(char *) * array_size, &array)) != 0)
				goto err;
		}

		if (pref != NULL) {
			if ((ret =
			    __absname(env, pref, name, &array[n])) != 0)
				goto err;
			__os_free(env, name);
		} else if ((p = __db_rpath(name)) != NULL) {
			if ((ret = __os_strdup(env, p + 1, &array[n])) != 0)
				goto err;
			__os_free(env, name);
		} else
			array[n] = name;

		name = NULL;
		array[++n] = NULL;
	}

	/* If there's nothing to return, we're done. */
	if (n == 0)
		goto err;

	/* Sort the list. */
	qsort(array, (size_t)n, sizeof(char *), __cmpfunc);

	/* Rework the memory. */
	if ((ret = __usermem(env, &array)) != 0)
		goto err;

	if (listp != NULL)
		*listp = array;

	if (0) {
err:		if (array != NULL) {
			for (arrayp = array; *arrayp != NULL; ++arrayp)
				__os_free(env, *arrayp);
			__os_free(env, array);
		}
		if (name != NULL)
			__os_free(env, name);
	}

	return (ret);
}

/*
 * __log_get_stable_lsn --
 *	Get the stable lsn based on where checkpoints are.
 *
 * PUBLIC: int __log_get_stable_lsn __P((ENV *, DB_LSN *));
 */
int
__log_get_stable_lsn(env, stable_lsn)
	ENV *env;
	DB_LSN *stable_lsn;
{
	DBT rec;
	DB_LOGC *logc;
	LOG *lp;
	__txn_ckp_args *ckp_args;
	int ret, t_ret;

	lp = env->lg_handle->reginfo.primary;

	ret = 0;
	memset(&rec, 0, sizeof(rec));
	if (!TXN_ON(env)) {
		if ((ret = __log_get_cached_ckp_lsn(env, stable_lsn)) != 0)
			goto err;
		/*
		 * No need to check for a return value of DB_NOTFOUND;
		 * __txn_findlastckp returns 0 if no checkpoint record
		 * is found.  Instead of checking the return value, we
		 * check to see if the return LSN has been filled in.
		 */
		if (IS_ZERO_LSN(*stable_lsn) && (ret =
		     __txn_findlastckp(env, stable_lsn, NULL)) != 0)
			goto err;
		/*
		 * If the LSN has not been filled in return DB_NOTFOUND
		 * so that the caller knows it may be done.
		 */
		if (IS_ZERO_LSN(*stable_lsn)) {
			ret = DB_NOTFOUND;
			goto err;
		}
	} else if ((ret = __txn_getckp(env, stable_lsn)) != 0)
		goto err;
	if ((ret = __log_cursor(env, &logc)) != 0)
		goto err;
	/*
	 * Read checkpoint records until we find one that is on disk,
	 * then copy the ckp_lsn to the stable_lsn;
	 */
	while ((ret = __logc_get(logc, stable_lsn, &rec, DB_SET)) == 0 &&
	    (ret = __txn_ckp_read(env, rec.data, &ckp_args)) == 0) {
		if (stable_lsn->file < lp->s_lsn.file ||
		    (stable_lsn->file == lp->s_lsn.file &&
		    stable_lsn->offset < lp->s_lsn.offset)) {
			*stable_lsn = ckp_args->ckp_lsn;
			__os_free(env, ckp_args);
			break;
		}
		*stable_lsn = ckp_args->last_ckp;
		__os_free(env, ckp_args);
	}
	if ((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
err:
	return (ret);
}

/*
 * __log_autoremove --
 *	Delete any non-essential log files.
 *
 * PUBLIC: void __log_autoremove __P((ENV *));
 */
void
__log_autoremove(env)
	ENV *env;
{
	int ret;
	char **begin, **list;

	/*
	 * Complain if there's an error, but don't return the error to our
	 * caller.  Auto-remove is done when writing a log record, and we
	 * don't want to fail a write, which could fail the corresponding
	 * committing transaction, for a permissions error.
	 */
	if ((ret = __log_archive(env, &list, DB_ARCH_ABS)) != 0) {
		if (ret != DB_NOTFOUND)
			__db_err(env, ret, "log file auto-remove");
		return;
	}

	/* Remove the files. */
	if (list != NULL) {
		for (begin = list; *list != NULL; ++list)
			(void)__os_unlink(env, *list, 0);
		__os_ufree(env, begin);
	}
}

/*
 * __build_data --
 *	Build a list of datafiles for return.
 */
static int
__build_data(env, pref, listp)
	ENV *env;
	char *pref, ***listp;
{
	DBT rec;
	DB_LOGC *logc;
	DB_LSN lsn;
	__dbreg_register_args *argp;
	u_int array_size, last, n, nxt;
	u_int32_t rectype;
	int ret, t_ret;
	char **array, **arrayp, **list, **lp, *p, *real_name;

	/* Get some initial space. */
	array_size = 64;
	if ((ret = __os_malloc(env,
	    sizeof(char *) * array_size, &array)) != 0)
		return (ret);
	array[0] = NULL;

	memset(&rec, 0, sizeof(rec));
	if ((ret = __log_cursor(env, &logc)) != 0)
		return (ret);
	for (n = 0; (ret = __logc_get(logc, &lsn, &rec, DB_PREV)) == 0;) {
		if (rec.size < sizeof(rectype)) {
			ret = EINVAL;
			__db_errx(env, "DB_ENV->log_archive: bad log record");
			break;
		}

		LOGCOPY_32(env, &rectype, rec.data);
		if (rectype != DB___dbreg_register)
			continue;
		if ((ret =
		    __dbreg_register_read(env, rec.data, &argp)) != 0) {
			ret = EINVAL;
			__db_errx(env,
			    "DB_ENV->log_archive: unable to read log record");
			break;
		}

		if (n >= array_size - 2) {
			array_size += LIST_INCREMENT;
			if ((ret = __os_realloc(env,
			    sizeof(char *) * array_size, &array)) != 0)
				goto free_continue;
		}

		if ((ret = __os_strdup(env,
		    argp->name.data, &array[n++])) != 0)
			goto free_continue;
		array[n] = NULL;

		if (argp->ftype == DB_QUEUE) {
			if ((ret = __qam_extent_names(env,
			    argp->name.data, &list)) != 0)
				goto q_err;
			for (lp = list;
			    lp != NULL && *lp != NULL; lp++) {
				if (n >= array_size - 2) {
					array_size += LIST_INCREMENT;
					if ((ret = __os_realloc(env,
					    sizeof(char *) *
					    array_size, &array)) != 0)
						goto q_err;
				}
				if ((ret =
				    __os_strdup(env, *lp, &array[n++])) != 0)
					goto q_err;
				array[n] = NULL;
			}
q_err:			if (list != NULL)
				__os_free(env, list);
		}
free_continue:	__os_free(env, argp);
		if (ret != 0)
			break;
	}
	if (ret == DB_NOTFOUND)
		ret = 0;
	if ((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	if (ret != 0)
		goto err1;

	/* If there's nothing to return, we're done. */
	if (n == 0) {
		ret = 0;
		*listp = NULL;
		goto err1;
	}

	/* Sort the list. */
	qsort(array, (size_t)n, sizeof(char *), __cmpfunc);

	/*
	 * Build the real pathnames, discarding nonexistent files and
	 * duplicates.
	 */
	for (last = nxt = 0; nxt < n;) {
		/*
		 * Discard duplicates.  Last is the next slot we're going
		 * to return to the user, nxt is the next slot that we're
		 * going to consider.
		 */
		if (last != nxt) {
			array[last] = array[nxt];
			array[nxt] = NULL;
		}
		for (++nxt; nxt < n &&
		    strcmp(array[last], array[nxt]) == 0; ++nxt) {
			__os_free(env, array[nxt]);
			array[nxt] = NULL;
		}

		/* Get the real name. */
		if ((ret = __db_appname(env,
		    DB_APP_DATA, array[last], NULL, &real_name)) != 0)
			goto err2;

		/* If the file doesn't exist, ignore it. */
		if (__os_exists(env, real_name, NULL) != 0) {
			__os_free(env, real_name);
			__os_free(env, array[last]);
			array[last] = NULL;
			continue;
		}

		/* Rework the name as requested by the user. */
		__os_free(env, array[last]);
		array[last] = NULL;
		if (pref != NULL) {
			ret = __absname(env, pref, real_name, &array[last]);
			__os_free(env, real_name);
			if (ret != 0)
				goto err2;
		} else if ((p = __db_rpath(real_name)) != NULL) {
			ret = __os_strdup(env, p + 1, &array[last]);
			__os_free(env, real_name);
			if (ret != 0)
				goto err2;
		} else
			array[last] = real_name;
		++last;
	}

	/* NULL-terminate the list. */
	array[last] = NULL;

	/* Rework the memory. */
	if ((ret = __usermem(env, &array)) != 0)
		goto err1;

	*listp = array;
	return (0);

err2:	/*
	 * XXX
	 * We've possibly inserted NULLs into the array list, so clean up a
	 * bit so that the other error processing works.
	 */
	if (array != NULL)
		for (; nxt < n; ++nxt)
			__os_free(env, array[nxt]);
	/* FALLTHROUGH */

err1:	if (array != NULL) {
		for (arrayp = array; *arrayp != NULL; ++arrayp)
			__os_free(env, *arrayp);
		__os_free(env, array);
	}
	return (ret);
}

/*
 * __absname --
 *	Return an absolute path name for the file.
 */
static int
__absname(env, pref, name, newnamep)
	ENV *env;
	char *pref, *name, **newnamep;
{
	size_t l_pref, l_name;
	int isabspath, ret;
	char *newname;

	l_name = strlen(name);
	isabspath = __os_abspath(name);
	l_pref = isabspath ? 0 : strlen(pref);

	/* Malloc space for concatenating the two. */
	if ((ret = __os_malloc(env,
	    l_pref + l_name + 2, &newname)) != 0)
		return (ret);
	*newnamep = newname;

	/* Build the name.  If `name' is an absolute path, ignore any prefix. */
	if (!isabspath) {
		memcpy(newname, pref, l_pref);
		if (strchr(PATH_SEPARATOR, newname[l_pref - 1]) == NULL)
			newname[l_pref++] = PATH_SEPARATOR[0];
	}
	memcpy(newname + l_pref, name, l_name + 1);

	return (0);
}

/*
 * __usermem --
 *	Create a single chunk of memory that holds the returned information.
 *	If the user has their own malloc routine, use it.
 */
static int
__usermem(env, listp)
	ENV *env;
	char ***listp;
{
	size_t len;
	int ret;
	char **array, **arrayp, **orig, *strp;

	/* Find out how much space we need. */
	for (len = 0, orig = *listp; *orig != NULL; ++orig)
		len += sizeof(char *) + strlen(*orig) + 1;
	len += sizeof(char *);

	/* Allocate it and set up the pointers. */
	if ((ret = __os_umalloc(env, len, &array)) != 0)
		return (ret);

	strp = (char *)(array + (orig - *listp) + 1);

	/* Copy the original information into the new memory. */
	for (orig = *listp, arrayp = array; *orig != NULL; ++orig, ++arrayp) {
		len = strlen(*orig);
		memcpy(strp, *orig, len + 1);
		*arrayp = strp;
		strp += len + 1;

		__os_free(env, *orig);
	}

	/* NULL-terminate the list. */
	*arrayp = NULL;

	__os_free(env, *listp);
	*listp = array;

	return (0);
}

static int
__cmpfunc(p1, p2)
	const void *p1, *p2;
{
	return (strcmp(*((char * const *)p1), *((char * const *)p2)));
}
