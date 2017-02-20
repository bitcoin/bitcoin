/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_OS_H_
#define _DB_OS_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* Number of times to retry system calls that return EINTR or EBUSY. */
#define DB_RETRY    100

#ifdef __TANDEM
/*
 * OSS Tandem problem: fsync can return a Guardian file system error of 70,
 * which has no symbolic name in OSS.  HP says to retry the fsync. [#12957]
 */
#define RETRY_CHK(op, ret) do {                     \
    int __retries, __t_ret;                     \
    for ((ret) = 0, __retries = DB_RETRY;;) {           \
        if ((op) == 0)                      \
            break;                      \
        (ret) = __os_get_syserr();              \
        if (((__t_ret = __os_posix_err(ret)) == EAGAIN ||   \
            __t_ret == EBUSY || __t_ret == EINTR ||     \
            __t_ret == EIO || __t_ret == 70) && --__retries > 0)\
            continue;                   \
        break;                          \
    }                               \
} while (0)
#else
#define RETRY_CHK(op, ret) do {                     \
    int __retries, __t_ret;                     \
    for ((ret) = 0, __retries = DB_RETRY;;) {           \
        if ((op) == 0)                      \
            break;                      \
        (ret) = __os_get_syserr();              \
        if (((__t_ret = __os_posix_err(ret)) == EAGAIN ||   \
            __t_ret == EBUSY || __t_ret == EINTR ||     \
            __t_ret == EIO) && --__retries > 0)         \
            continue;                   \
        break;                          \
    }                               \
} while (0)
#endif

#define RETRY_CHK_EINTR_ONLY(op, ret) do {              \
    int __retries;                          \
    for ((ret) = 0, __retries = DB_RETRY;;) {           \
        if ((op) == 0)                      \
            break;                      \
        (ret) = __os_get_syserr();              \
        if (__os_posix_err(ret) == EINTR && --__retries > 0)    \
            continue;                   \
        break;                          \
    }                               \
} while (0)

/*
 * Flags understood by __os_open.
 */
#define DB_OSO_ABSMODE  0x0001      /* Absolute mode specified. */
#define DB_OSO_CREATE   0x0002      /* POSIX: O_CREAT */
#define DB_OSO_DIRECT   0x0004      /* Don't buffer the file in the OS. */
#define DB_OSO_DSYNC    0x0008      /* POSIX: O_DSYNC. */
#define DB_OSO_EXCL 0x0010      /* POSIX: O_EXCL */
#define DB_OSO_RDONLY   0x0020      /* POSIX: O_RDONLY */
#define DB_OSO_REGION   0x0040      /* Opening a region file. */
#define DB_OSO_SEQ  0x0080      /* Expected sequential access. */
#define DB_OSO_TEMP 0x0100      /* Remove after last close. */
#define DB_OSO_TRUNC    0x0200      /* POSIX: O_TRUNC */

/*
 * File modes.
 */
#define DB_MODE_400 (S_IRUSR)
#define DB_MODE_600 (S_IRUSR|S_IWUSR)
#define DB_MODE_660 (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)
#define DB_MODE_666 (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#define DB_MODE_700 (S_IRUSR|S_IWUSR|S_IXUSR)

/*
 * We group certain seek/write calls into a single function so that we
 * can use pread(2)/pwrite(2) where they're available.
 */
#define DB_IO_READ  1
#define DB_IO_WRITE 2

/*
 * Make a last "panic" check.  Imagine a thread of control running in Berkeley
 * DB, going to sleep.  Another thread of control decides to run recovery
 * because the environment is broken.  The first thing recovery does is panic
 * the existing environment, but we only check the panic flag when crossing the
 * public API.  If the sleeping thread wakes up and writes something, we could
 * have two threads of control writing the log files at the same time.  So,
 * before reading or writing, make a last panic check.  Obviously, there's still
 * a window, but it's very, very small.
 */
#define LAST_PANIC_CHECK_BEFORE_IO(env)                 \
    PANIC_CHECK(env);

/* DB filehandle. */
struct __fh_t {
    /*
     * Linked list of DB_FH's, linked from the DB_ENV, used to keep track
     * of all open file handles for resource cleanup.
     */
     TAILQ_ENTRY(__fh_t) q;

    /*
     * The file-handle mutex is only used to protect the handle/fd
     * across seek and read/write pairs, it does not protect the
     * the reference count, or any other fields in the structure.
     */
    db_mutex_t mtx_fh;      /* Mutex to lock. */

    int ref;            /* Reference count. */

#ifdef HAVE_BREW
    IFile   *ifp;           /* IFile pointer */
#endif
#if defined(DB_WIN32)
    HANDLE  handle;         /* Windows/32 file handle. */
    HANDLE  trunc_handle;       /* Handle for truncate calls. */
#endif
    int fd;         /* POSIX file descriptor. */

    char    *name;          /* File name at open. */

    /*
     * Last seek statistics, used for zero-filling on filesystems
     * that don't support it directly.
     */
    db_pgno_t pgno;
    u_int32_t pgsize;
    u_int32_t offset;

#ifdef HAVE_STATISTICS
    u_int32_t seek_count;       /* I/O statistics */
    u_int32_t read_count;
    u_int32_t write_count;
#endif

#define DB_FH_ENVLINK   0x01        /* We're linked on the DB_ENV. */
#define DB_FH_NOSYNC    0x02        /* Handle doesn't need to be sync'd. */
#define DB_FH_OPENED    0x04        /* Handle is valid. */
#define DB_FH_UNLINK    0x08        /* Unlink on close */
#define DB_FH_REGION    0x10        /* Opened to contain a region */
    u_int8_t flags;
};

/* Standard buffer size for ctime/ctime_r function calls. */
#define CTIME_BUFLEN    26

/*
 * VxWorks requires we cast (const char *) variables to (char *) in order to
 * pass them to system calls like stat, read and write.
 */
#ifdef HAVE_VXWORKS
#define CHAR_STAR_CAST  (char *)
#else
#define CHAR_STAR_CAST
#endif

#if defined(__cplusplus)
}
#endif

#include "dbinc_auto/os_ext.h"
#endif /* !_DB_OS_H_ */
