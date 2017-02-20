/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

/* User-specified role an environment should play in the replication group. */
typedef enum { MASTER, CLIENT, UNKNOWN } ENV_ROLE;

/* User-specified information about a replication site. */
typedef struct {
    char *host;     /* Host name. */
    u_int32_t port;     /* Port on which to connect to this site. */
    int peer;       /* Whether remote site is repmgr peer. */
} repsite_t;

/* Data used for common replication setup. */
typedef struct {
    const char *progname;
    char *home;
    int nsites;
    int remotesites;
    ENV_ROLE role;
    repsite_t self;
    repsite_t *site_list;
} SETUP_DATA;

/* Data shared by both repmgr and base versions of this program. */
typedef struct {
    int is_master;
    int app_finished;
    int in_client_sync;
} SHARED_DATA;

/* Arguments for support threads. */
typedef struct {
    DB_ENV *dbenv;
    SHARED_DATA *shared;
} supthr_args;

/* Portability macros for basic threading & timing */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#define snprintf        _snprintf
#define sleep(s)        Sleep(1000 * (s))

extern int getopt(int, char * const *, const char *);

typedef HANDLE thread_t;
#define thread_create(thrp, attr, func, arg)                   \
    (((*(thrp) = CreateThread(NULL, 0,                     \
    (LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL)) == NULL) ? -1 : 0)
#define thread_join(thr, statusp)                      \
    ((WaitForSingleObject((thr), INFINITE) == WAIT_OBJECT_0) &&        \
    GetExitCodeThread((thr), (LPDWORD)(statusp)) ? 0 : -1)

#else /* !_WIN32 */
#include <sys/time.h>
#include <pthread.h>

typedef pthread_t thread_t;
#define thread_create(thrp, attr, func, arg)                   \
    pthread_create((thrp), (attr), (func), (arg))
#define thread_join(thr, statusp) pthread_join((thr), (statusp))

#endif

void *checkpoint_thread __P((void *));
int common_rep_setup __P((DB_ENV *, int, char *[], SETUP_DATA *));
int create_env __P((const char *, DB_ENV **));
int doloop __P((DB_ENV *, SHARED_DATA *));
int env_init __P((DB_ENV *, const char *));
int finish_support_threads __P((thread_t *, thread_t *));
void *log_archive_thread __P((void *));
int start_support_threads __P((DB_ENV *, supthr_args *, thread_t *,
    thread_t *));
void usage __P((const int, const char *));
