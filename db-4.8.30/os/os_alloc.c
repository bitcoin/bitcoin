/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#ifdef DIAGNOSTIC
static void __os_guard __P((ENV *));

typedef union {
	size_t size;
	uintmax_t align;
} db_allocinfo_t;
#endif

/*
 * !!!
 * Correct for systems that return NULL when you allocate 0 bytes of memory.
 * There are several places in DB where we allocate the number of bytes held
 * by the key/data item, and it can be 0.  Correct here so that malloc never
 * returns a NULL for that reason (which behavior is permitted by ANSI).  We
 * could make these calls macros on non-Alpha architectures (that's where we
 * saw the problem), but it's probably not worth the autoconf complexity.
 *
 * !!!
 * Correct for systems that don't set errno when malloc and friends fail.
 *
 *	Out of memory.
 *	We wish to hold the whole sky,
 *	But we never will.
 */

/*
 * __os_umalloc --
 *	Allocate memory to be used by the application.
 *
 *	Use, in order of preference, the allocation function specified to the
 *	ENV handle, the allocation function specified as a replacement for
 *	the library malloc, or the library malloc().
 *
 * PUBLIC: int __os_umalloc __P((ENV *, size_t, void *));
 */
int
__os_umalloc(env, size, storep)
	ENV *env;
	size_t size;
	void *storep;
{
	DB_ENV *dbenv;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;

	/* Never allocate 0 bytes -- some C libraries don't like it. */
	if (size == 0)
		++size;

	if (dbenv == NULL || dbenv->db_malloc == NULL) {
		if (DB_GLOBAL(j_malloc) != NULL)
			*(void **)storep = DB_GLOBAL(j_malloc)(size);
		else
			*(void **)storep = malloc(size);
		if (*(void **)storep == NULL) {
			/*
			 *  Correct error return, see __os_malloc.
			 */
			if ((ret = __os_get_errno_ret_zero()) == 0) {
				ret = ENOMEM;
				__os_set_errno(ENOMEM);
			}
			__db_err(env, ret, "malloc: %lu", (u_long)size);
			return (ret);
		}
		return (0);
	}

	if ((*(void **)storep = dbenv->db_malloc(size)) == NULL) {
		__db_errx(env,
		    "user-specified malloc function returned NULL");
		return (ENOMEM);
	}

	return (0);
}

/*
 * __os_urealloc --
 *	Allocate memory to be used by the application.
 *
 *	A realloc(3) counterpart to __os_umalloc's malloc(3).
 *
 * PUBLIC: int __os_urealloc __P((ENV *, size_t, void *));
 */
int
__os_urealloc(env, size, storep)
	ENV *env;
	size_t size;
	void *storep;
{
	DB_ENV *dbenv;
	int ret;
	void *ptr;

	dbenv = env == NULL ? NULL : env->dbenv;
	ptr = *(void **)storep;

	/* Never allocate 0 bytes -- some C libraries don't like it. */
	if (size == 0)
		++size;

	if (dbenv == NULL || dbenv->db_realloc == NULL) {
		if (ptr == NULL)
			return (__os_umalloc(env, size, storep));

		if (DB_GLOBAL(j_realloc) != NULL)
			*(void **)storep = DB_GLOBAL(j_realloc)(ptr, size);
		else
			*(void **)storep = realloc(ptr, size);
		if (*(void **)storep == NULL) {
			/*
			 * Correct errno, see __os_realloc.
			 */
			if ((ret = __os_get_errno_ret_zero()) == 0) {
				ret = ENOMEM;
				__os_set_errno(ENOMEM);
			}
			__db_err(env, ret, "realloc: %lu", (u_long)size);
			return (ret);
		}
		return (0);
	}

	if ((*(void **)storep = dbenv->db_realloc(ptr, size)) == NULL) {
		__db_errx(env,
		    "User-specified realloc function returned NULL");
		return (ENOMEM);
	}

	return (0);
}

/*
 * __os_ufree --
 *	Free memory used by the application.
 *
 *	A free(3) counterpart to __os_umalloc's malloc(3).
 *
 * PUBLIC: void __os_ufree __P((ENV *, void *));
 */
void
__os_ufree(env, ptr)
	ENV *env;
	void *ptr;
{
	DB_ENV *dbenv;

	dbenv = env == NULL ? NULL : env->dbenv;

	if (dbenv != NULL && dbenv->db_free != NULL)
		dbenv->db_free(ptr);
	else if (DB_GLOBAL(j_free) != NULL)
		DB_GLOBAL(j_free)(ptr);
	else
		free(ptr);
}

/*
 * __os_strdup --
 *	The strdup(3) function for DB.
 *
 * PUBLIC: int __os_strdup __P((ENV *, const char *, void *));
 */
int
__os_strdup(env, str, storep)
	ENV *env;
	const char *str;
	void *storep;
{
	size_t size;
	int ret;
	void *p;

	*(void **)storep = NULL;

	size = strlen(str) + 1;
	if ((ret = __os_malloc(env, size, &p)) != 0)
		return (ret);

	memcpy(p, str, size);

	*(void **)storep = p;
	return (0);
}

/*
 * __os_calloc --
 *	The calloc(3) function for DB.
 *
 * PUBLIC: int __os_calloc __P((ENV *, size_t, size_t, void *));
 */
int
__os_calloc(env, num, size, storep)
	ENV *env;
	size_t num, size;
	void *storep;
{
	int ret;

	size *= num;
	if ((ret = __os_malloc(env, size, storep)) != 0)
		return (ret);

	memset(*(void **)storep, 0, size);

	return (0);
}

/*
 * __os_malloc --
 *	The malloc(3) function for DB.
 *
 * PUBLIC: int __os_malloc __P((ENV *, size_t, void *));
 */
int
__os_malloc(env, size, storep)
	ENV *env;
	size_t size;
	void *storep;
{
	int ret;
	void *p;

	*(void **)storep = NULL;

	/* Never allocate 0 bytes -- some C libraries don't like it. */
	if (size == 0)
		++size;

#ifdef DIAGNOSTIC
	/* Add room for size and a guard byte. */
	size += sizeof(db_allocinfo_t) + 1;
#endif

	if (DB_GLOBAL(j_malloc) != NULL)
		p = DB_GLOBAL(j_malloc)(size);
	else
		p = malloc(size);
	if (p == NULL) {
		/*
		 * Some C libraries don't correctly set errno when malloc(3)
		 * fails.  We'd like to 0 out errno before calling malloc,
		 * but it turns out that setting errno is quite expensive on
		 * Windows/NT in an MT environment.
		 */
		if ((ret = __os_get_errno_ret_zero()) == 0) {
			ret = ENOMEM;
			__os_set_errno(ENOMEM);
		}
		__db_err(env, ret, "malloc: %lu", (u_long)size);
		return (ret);
	}

#ifdef DIAGNOSTIC
	/* Overwrite memory. */
	memset(p, CLEAR_BYTE, size);

	/*
	 * Guard bytes: if #DIAGNOSTIC is defined, we allocate an additional
	 * byte after the memory and set it to a special value that we check
	 * for when the memory is free'd.
	 */
	((u_int8_t *)p)[size - 1] = CLEAR_BYTE;

	((db_allocinfo_t *)p)->size = size;
	p = &((db_allocinfo_t *)p)[1];
#endif
	*(void **)storep = p;

	return (0);
}

/*
 * __os_realloc --
 *	The realloc(3) function for DB.
 *
 * PUBLIC: int __os_realloc __P((ENV *, size_t, void *));
 */
int
__os_realloc(env, size, storep)
	ENV *env;
	size_t size;
	void *storep;
{
	int ret;
	void *p, *ptr;

	ptr = *(void **)storep;

	/* Never allocate 0 bytes -- some C libraries don't like it. */
	if (size == 0)
		++size;

	/* If we haven't yet allocated anything yet, simply call malloc. */
	if (ptr == NULL)
		return (__os_malloc(env, size, storep));

#ifdef DIAGNOSTIC
	/* Add room for size and a guard byte. */
	size += sizeof(db_allocinfo_t) + 1;

	/* Back up to the real beginning */
	ptr = &((db_allocinfo_t *)ptr)[-1];

	{
		size_t s;

		s = ((db_allocinfo_t *)ptr)->size;
		if (((u_int8_t *)ptr)[s - 1] != CLEAR_BYTE)
			 __os_guard(env);
	}
#endif

	/*
	 * Don't overwrite the original pointer, there are places in DB we
	 * try to continue after realloc fails.
	 */
	if (DB_GLOBAL(j_realloc) != NULL)
		p = DB_GLOBAL(j_realloc)(ptr, size);
	else
		p = realloc(ptr, size);
	if (p == NULL) {
		/*
		 * Some C libraries don't correctly set errno when malloc(3)
		 * fails.  We'd like to 0 out errno before calling malloc,
		 * but it turns out that setting errno is quite expensive on
		 * Windows/NT in an MT environment.
		 */
		if ((ret = __os_get_errno_ret_zero()) == 0) {
			ret = ENOMEM;
			__os_set_errno(ENOMEM);
		}
		__db_err(env, ret, "realloc: %lu", (u_long)size);
		return (ret);
	}
#ifdef DIAGNOSTIC
	((u_int8_t *)p)[size - 1] = CLEAR_BYTE;	/* Initialize guard byte. */

	((db_allocinfo_t *)p)->size = size;
	p = &((db_allocinfo_t *)p)[1];
#endif

	*(void **)storep = p;

	return (0);
}

/*
 * __os_free --
 *	The free(3) function for DB.
 *
 * PUBLIC: void __os_free __P((ENV *, void *));
 */
void
__os_free(env, ptr)
	ENV *env;
	void *ptr;
{
#ifdef DIAGNOSTIC
	size_t size;
#endif

	/*
	 * ANSI C requires free(NULL) work.  Don't depend on the underlying
	 * library.
	 */
	if (ptr == NULL)
		return;

#ifdef DIAGNOSTIC
	/*
	 * Check that the guard byte (one past the end of the memory) is
	 * still CLEAR_BYTE.
	 */
	ptr = &((db_allocinfo_t *)ptr)[-1];
	size = ((db_allocinfo_t *)ptr)->size;
	if (((u_int8_t *)ptr)[size - 1] != CLEAR_BYTE)
		 __os_guard(env);

	/* Overwrite memory. */
	if (size != 0)
		memset(ptr, CLEAR_BYTE, size);
#else
	COMPQUIET(env, NULL);
#endif

	if (DB_GLOBAL(j_free) != NULL)
		DB_GLOBAL(j_free)(ptr);
	else
		free(ptr);
}

#ifdef DIAGNOSTIC
/*
 * __os_guard --
 *	Complain and abort.
 */
static void
__os_guard(env)
	ENV *env;
{
	__db_errx(env, "Guard byte incorrect during free");
	__os_abort(env);
	/* NOTREACHED */
}
#endif

/*
 * __ua_memcpy --
 *	Copy memory to memory without relying on any kind of alignment.
 *
 *	There are places in DB that we have unaligned data, for example,
 *	when we've stored a structure in a log record as a DBT, and now
 *	we want to look at it.  Unfortunately, if you have code like:
 *
 *		struct a {
 *			int x;
 *		} *p;
 *
 *		void *func_argument;
 *		int local;
 *
 *		p = (struct a *)func_argument;
 *		memcpy(&local, p->x, sizeof(local));
 *
 *	compilers optimize to use inline instructions requiring alignment,
 *	and records in the log don't have any particular alignment.  (This
 *	isn't a compiler bug, because it's a structure they're allowed to
 *	assume alignment.)
 *
 *	Casting the memcpy arguments to (u_int8_t *) appears to work most
 *	of the time, but we've seen examples where it wasn't sufficient
 *	and there's nothing in ANSI C that requires that work.
 *
 * PUBLIC: void *__ua_memcpy __P((void *, const void *, size_t));
 */
void *
__ua_memcpy(dst, src, len)
	void *dst;
	const void *src;
	size_t len;
{
	return ((void *)memcpy(dst, src, len));
}
