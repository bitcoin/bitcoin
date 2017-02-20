/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1998-2009 Oracle.  All rights reserved.
 *
 * This code is derived from software contributed to Sleepycat Software by
 * Frederick G.M. Roeber of Netscape Communications Corp.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * DB uses memory-mapped files for two things:
 *	faster access of read-only databases, and
 *	shared memory for process synchronization and locking.
 * The code carefully does not mix the two uses.  The first-case uses are
 * actually written such that memory-mapping isn't really required -- it's
 * merely a convenience -- so we don't have to worry much about it.  In the
 * second case, it's solely used as a shared memory mechanism, so that's
 * all we have to replace.
 *
 * All memory in VxWorks is shared, and a task can allocate memory and keep
 * notes.  So I merely have to allocate memory, remember the "filename" for
 * that memory, and issue small-integer segment IDs which index the list of
 * these shared-memory segments. Subsequent opens are checked against the
 * list of already open segments.
 */
typedef struct {
	void *segment;			/* Segment address. */
	u_int32_t size;			/* Segment size. */
	char *name;			/* Segment name. */
	long segid;			/* Segment ID. */
} os_segdata_t;

static os_segdata_t *__os_segdata;	/* Segment table. */
static int __os_segdata_size;		/* Segment table size. */

#define	OS_SEGDATA_STARTING_SIZE 16
#define	OS_SEGDATA_INCREMENT	 16

static int __os_segdata_allocate
	       __P((ENV *, const char *, REGINFO *, REGION *));
static int __os_segdata_find_byname
	       __P((ENV *, const char *, REGINFO *, REGION *));
static int __os_segdata_init __P((ENV *));
static int __os_segdata_new __P((ENV *, int *));
static int __os_segdata_release __P((ENV *, REGION *, int));

/*
 * __os_attach --
 *	Create/join a shared memory region.
 */
int
__os_attach(env, infop, rp)
	ENV *env;
	REGINFO *infop;
	REGION *rp;
{
	DB_ENV *dbenv;
	int ret;

	dbenv = env->dbenv;

	if (__os_segdata == NULL)
		__os_segdata_init(env);

	DB_BEGIN_SINGLE_THREAD;

	/* Try to find an already existing segment. */
	ret = __os_segdata_find_byname(env, infop->name, infop, rp);

	/*
	 * If we are trying to join a region, it is easy, either we
	 * found it and we return, or we didn't find it and we return
	 * an error that it doesn't exist.
	 */
	if (!F_ISSET(infop, REGION_CREATE)) {
		if (ret != 0) {
			__db_errx(env, "segment %s does not exist",
			    infop->name);
			ret = EAGAIN;
		}
		goto out;
	}

	/*
	 * If we get here, we are trying to create the region.
	 * There are several things to consider:
	 * - if we have an error (not a found or not-found value), return.
	 * - they better have shm_key set.
	 * - if the region is already there (ret == 0 from above),
	 * assume the application crashed and we're restarting.
	 * Delete the old region.
	 * - try to create the region.
	 */
	if (ret != 0 && ret != ENOENT)
		goto out;

	if (dbenv->shm_key == INVALID_REGION_SEGID) {
		__db_errx(env, "no base shared memory ID specified");
		ret = EAGAIN;
		goto out;
	}
	if (ret == 0 && __os_segdata_release(env, rp, 1) != 0) {
		__db_errx(env,
		    "key: %ld: shared memory region already exists",
		    dbenv->shm_key + (infop->id - 1));
		ret = EAGAIN;
		goto out;
	}

	ret = __os_segdata_allocate(env, infop->name, infop, rp);
out:
	DB_END_SINGLE_THREAD;
	return (ret);
}

/*
 * __os_detach --
 *	Detach from a shared region.
 */
int
__os_detach(env, infop, destroy)
	ENV *env;
	REGINFO *infop;
	int destroy;
{
	/*
	 * If just detaching, there is no mapping to discard.
	 * If destroying, remove the region.
	 */
	if (destroy)
		return (__os_segdata_release(env, infop->rp, 0));
	return (0);
}

/*
 * __os_mapfile --
 *	Map in a shared memory file.
 */
int
__os_mapfile(env, path, fhp, len, is_rdonly, addrp)
	ENV *env;
	char *path;
	DB_FH *fhp;
	int is_rdonly;
	size_t len;
	void **addrp;
{
	/* We cannot map in regular files in VxWorks. */
	COMPQUIET(env, NULL);
	COMPQUIET(path, NULL);
	COMPQUIET(fhp, NULL);
	COMPQUIET(is_rdonly, 0);
	COMPQUIET(len, 0);
	COMPQUIET(addrp, NULL);
	return (DB_OPNOTSUP);
}

/*
 * __os_unmapfile --
 *	Unmap the shared memory file.
 */
int
__os_unmapfile(env, addr, len)
	ENV *env;
	void *addr;
	size_t len;
{
	/* We cannot map in regular files in VxWorks. */
	COMPQUIET(env, NULL);
	COMPQUIET(addr, NULL);
	COMPQUIET(len, 0);
	return (DB_OPNOTSUP);
}

/*
 * __os_segdata_init --
 *	Initializes the library's table of shared memory segments.
 *	Called once on the first time through __os_segdata_new().
 */
static int
__os_segdata_init(env)
	ENV *env;
{
	int ret;

	if (__os_segdata != NULL) {
		__db_errx(env, "shared memory segment already exists");
		return (EEXIST);
	}

	/*
	 * The lock init call returns a locked lock.
	 */
	DB_BEGIN_SINGLE_THREAD;
	__os_segdata_size = OS_SEGDATA_STARTING_SIZE;
	ret = __os_calloc(env,
	    __os_segdata_size, sizeof(os_segdata_t), &__os_segdata);
	DB_END_SINGLE_THREAD;
	return (ret);
}

/*
 * __os_segdata_destroy --
 *	Destroys the library's table of shared memory segments.  It also
 *	frees all linked data: the segments themselves, and their names.
 *	Currently not called.  This function should be called if the
 *	user creates a function to unload or shutdown.
 */
int
__os_segdata_destroy(env)
	ENV *env;
{
	os_segdata_t *p;
	int i;

	if (__os_segdata == NULL)
		return (0);

	DB_BEGIN_SINGLE_THREAD;
	for (i = 0; i < __os_segdata_size; i++) {
		p = &__os_segdata[i];
		if (p->name != NULL) {
			__os_free(env, p->name);
			p->name = NULL;
		}
		if (p->segment != NULL) {
			__os_free(env, p->segment);
			p->segment = NULL;
		}
		p->size = 0;
	}

	__os_free(env, __os_segdata);
	__os_segdata = NULL;
	__os_segdata_size = 0;
	DB_END_SINGLE_THREAD;

	return (0);
}

/*
 * __os_segdata_allocate --
 *	Creates a new segment of the specified size, optionally with the
 *	specified name.
 *
 * Assumes it is called with the SEGDATA lock taken.
 */
static int
__os_segdata_allocate(env, name, infop, rp)
	ENV *env;
	const char *name;
	REGINFO *infop;
	REGION *rp;
{
	DB_ENV *dbenv;
	os_segdata_t *p;
	int id, ret;

	dbenv = env->dbenv;

	if ((ret = __os_segdata_new(env, &id)) != 0)
		return (ret);

	p = &__os_segdata[id];
	if ((ret = __os_calloc(env, 1, rp->size, &p->segment)) != 0)
		return (ret);
	if ((ret = __os_strdup(env, name, &p->name)) != 0) {
		__os_free(env, p->segment);
		p->segment = NULL;
		return (ret);
	}
	p->size = rp->size;
	p->segid = dbenv->shm_key + infop->id - 1;

	infop->addr = p->segment;
	rp->segid = id;

	return (0);
}

/*
 * __os_segdata_new --
 *	Finds a new segdata slot.  Does not initialise it, so the fd returned
 *	is only valid until you call this again.
 *
 * Assumes it is called with the SEGDATA lock taken.
 */
static int
__os_segdata_new(env, segidp)
	ENV *env;
	int *segidp;
{
	os_segdata_t *p;
	int i, newsize, ret;

	if (__os_segdata == NULL) {
		__db_errx(env, "shared memory segment not initialized");
		return (EAGAIN);
	}

	for (i = 0; i < __os_segdata_size; i++) {
		p = &__os_segdata[i];
		if (p->segment == NULL) {
			*segidp = i;
			return (0);
		}
	}

	/*
	 * No more free slots, expand.
	 */
	newsize = __os_segdata_size + OS_SEGDATA_INCREMENT;
	if ((ret = __os_realloc(env, newsize * sizeof(os_segdata_t),
	    &__os_segdata)) != 0)
		return (ret);
	memset(&__os_segdata[__os_segdata_size],
	    0, OS_SEGDATA_INCREMENT * sizeof(os_segdata_t));

	*segidp = __os_segdata_size;
	__os_segdata_size = newsize;

	return (0);
}

/*
 * __os_segdata_find_byname --
 *	Finds a segment by its name and shm_key.
 *
 * Assumes it is called with the SEGDATA lock taken.
 */
static int
__os_segdata_find_byname(env, name, infop, rp)
	ENV *env;
	const char *name;
	REGINFO *infop;
	REGION *rp;
{
	DB_ENV *dbenv;
	os_segdata_t *p;
	long segid;
	int i;

	dbenv = env->dbenv;

	if (__os_segdata == NULL) {
		__db_errx(env, "shared memory segment not initialized");
		return (EAGAIN);
	}

	if (name == NULL) {
		__db_errx(env, "no segment name given");
		return (EAGAIN);
	}

	/*
	 * If we are creating the region, compute the segid.
	 * If we are joining the region, we use the segid in the
	 * index we are given.
	 */
	if (F_ISSET(infop, REGION_CREATE))
		segid = dbenv->shm_key + (infop->id - 1);
	else {
		if (rp->segid >= __os_segdata_size ||
		    rp->segid == INVALID_REGION_SEGID) {
			__db_errx(env, "Invalid segment id given");
			return (EAGAIN);
		}
		segid = __os_segdata[rp->segid].segid;
	}
	for (i = 0; i < __os_segdata_size; i++) {
		p = &__os_segdata[i];
		if (p->name != NULL && strcmp(name, p->name) == 0 &&
		    p->segid == segid) {
			infop->addr = p->segment;
			rp->segid = i;
			return (0);
		}
	}
	return (ENOENT);
}

/*
 * __os_segdata_release --
 *	Free a segdata entry.
 */
static int
__os_segdata_release(env, rp, is_locked)
	ENV *env;
	REGION *rp;
	int is_locked;
{
	os_segdata_t *p;

	if (__os_segdata == NULL) {
		__db_errx(env, "shared memory segment not initialized");
		return (EAGAIN);
	}

	if (rp->segid < 0 || rp->segid >= __os_segdata_size) {
		__db_errx(env, "segment id %ld out of range", rp->segid);
		return (EINVAL);
	}

	if (is_locked == 0)
		DB_BEGIN_SINGLE_THREAD;
	p = &__os_segdata[rp->segid];
	if (p->name != NULL) {
		__os_free(env, p->name);
		p->name = NULL;
	}
	if (p->segment != NULL) {
		__os_free(env, p->segment);
		p->segment = NULL;
	}
	p->size = 0;
	if (is_locked == 0)
		DB_END_SINGLE_THREAD;

	/* Any shrink-table logic could go here */

	return (0);
}
