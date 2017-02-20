/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_GLOBALS_H_
#define _DB_GLOBALS_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************
 * Global variables.
 *
 * Held in a single structure to minimize the name-space pollution.
 *******************************************************/
#ifdef HAVE_VXWORKS
#include "semLib.h"
#endif

typedef struct __db_globals {
#ifdef HAVE_BREW
    struct tm ltm;          /* BREW localtime structure */
#endif
#ifdef HAVE_VXWORKS
    u_int32_t db_global_init;   /* VxWorks: inited */
    SEM_ID db_global_lock;      /* VxWorks: global semaphore */
#endif

    char *db_line;          /* DB display string. */

    char error_buf[40];     /* Error string buffer. */

    int uid_init;           /* srand set in UID generator */

    u_long rand_next;       /* rand/srand value */

    u_int32_t fid_serial;       /* file id counter */

    int db_errno;           /* Errno value if not available */

    int (*j_close) __P((int));  /* Underlying OS interface jump table.*/
    void    (*j_dirfree) __P((char **, int));
    int (*j_dirlist) __P((const char *, char ***, int *));
    int (*j_exists) __P((const char *, int *));
    void    (*j_free) __P((void *));
    int (*j_fsync) __P((int));
    int (*j_ftruncate) __P((int, off_t));
    int (*j_ioinfo) __P((const char *,
            int, u_int32_t *, u_int32_t *, u_int32_t *));
    void   *(*j_malloc) __P((size_t));
    int (*j_file_map) __P((DB_ENV *, char *, size_t, int, void **));
    int (*j_file_unmap) __P((DB_ENV *, void *));
    int (*j_open) __P((const char *, int, ...));
    ssize_t (*j_pread) __P((int, void *, size_t, off_t));
    ssize_t (*j_pwrite) __P((int, const void *, size_t, off_t));
    ssize_t (*j_read) __P((int, void *, size_t));
    void   *(*j_realloc) __P((void *, size_t));
    int (*j_region_map) __P((DB_ENV *, char *, size_t, int *, void **));
    int (*j_region_unmap) __P((DB_ENV *, void *));
    int (*j_rename) __P((const char *, const char *));
    int (*j_seek) __P((int, off_t, int));
    int (*j_unlink) __P((const char *));
    ssize_t (*j_write) __P((int, const void *, size_t));
    int (*j_yield) __P((u_long, u_long));
} DB_GLOBALS;

#ifdef HAVE_BREW
#define DB_GLOBAL(v)                            \
    ((DB_GLOBALS *)(((BDBApp *)GETAPPINSTANCE())->db_global_values))->v
#else
#ifdef DB_INITIALIZE_DB_GLOBALS
DB_GLOBALS __db_global_values = {
#ifdef HAVE_VXWORKS
    0,              /* VxWorks: initialized */
    NULL,               /* VxWorks: global semaphore */
#endif

    "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=",
    { 0 },
    0,
    0,
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
#else
extern  DB_GLOBALS  __db_global_values;
#endif

#define DB_GLOBAL(v)    __db_global_values.v
#endif /* HAVE_BREW */

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_GLOBALS_H_ */
