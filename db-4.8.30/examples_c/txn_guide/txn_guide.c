/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

/* File: txn_guide.c */

/* We assume an ANSI-compatible compiler */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <db.h>

#ifdef _WIN32
#include <windows.h>
#define	PATHD '\\'
extern int getopt(int, char * const *, const char *);
extern char *optarg;

/* Wrap Windows thread API to make it look POSIXey. */
typedef HANDLE thread_t;
#define	thread_create(thrp, attr, func, arg)                               \
    (((*(thrp) = CreateThread(NULL, 0,                                     \
	(LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL)) == NULL) ? -1 : 0)
#define	thread_join(thr, statusp)                                          \
    ((WaitForSingleObject((thr), INFINITE) == WAIT_OBJECT_0) &&            \
    ((statusp == NULL) ? 0 :						   \
    (GetExitCodeThread((thr), (LPDWORD)(statusp)) ? 0 : -1)))

typedef HANDLE mutex_t;
#define	mutex_init(m, attr)                                                \
    (((*(m) = CreateMutex(NULL, FALSE, NULL)) != NULL) ? 0 : -1)
#define	mutex_lock(m)                                                      \
    ((WaitForSingleObject(*(m), INFINITE) == WAIT_OBJECT_0) ? 0 : -1)
#define	mutex_unlock(m)         (ReleaseMutex(*(m)) ? 0 : -1)
#else
#include <pthread.h>
#include <unistd.h>
#define	PATHD '/'

typedef pthread_t thread_t;
#define	thread_create(thrp, attr, func, arg)                               \
    pthread_create((thrp), (attr), (func), (arg))
#define	thread_join(thr, statusp) pthread_join((thr), (statusp))

typedef pthread_mutex_t mutex_t;
#define	mutex_init(m, attr)     pthread_mutex_init((m), (attr))
#define	mutex_lock(m)           pthread_mutex_lock(m)
#define	mutex_unlock(m)         pthread_mutex_unlock(m)
#endif

/* Run 5 writers threads at a time. */
#define	NUMWRITERS 5

/*
 * Printing of a thread_t is implementation-specific, so we
 * create our own thread IDs for reporting purposes.
 */
int global_thread_num;
mutex_t thread_num_lock;

/* Forward declarations */
int count_records(DB *, DB_TXN *);
int open_db(DB **, const char *, const char *, DB_ENV *, u_int32_t);
int usage(void);
void *writer_thread(void *);

/* Usage function */
int
usage()
{
    fprintf(stderr, " [-h <database_home_directory>]\n");
    return (EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
    /* Initialize our handles */
    DB *dbp = NULL;
    DB_ENV *envp = NULL;

    thread_t writer_threads[NUMWRITERS];
    int ch, i, ret, ret_t;
    u_int32_t env_flags;
    char *db_home_dir;
    /* Application name */
    const char *prog_name = "txn_guide";
    /* Database file name */
    const char *file_name = "mydb.db";

    /* Parse the command line arguments */
#ifdef _WIN32
    db_home_dir = ".\\";
#else
    db_home_dir = "./";
#endif
    while ((ch = getopt(argc, argv, "h:")) != EOF)
	switch (ch) {
	case 'h':
	    db_home_dir = optarg;
	    break;
	case '?':
	default:
	    return (usage());
	}

    /* Create the environment */
    ret = db_env_create(&envp, 0);
    if (ret != 0) {
	fprintf(stderr, "Error creating environment handle: %s\n",
	    db_strerror(ret));
	goto err;
    }

     /*
     * Indicate that we want db to perform lock detection internally.
     * Also indicate that the transaction with the fewest number of
     * write locks will receive the deadlock notification in
     * the event of a deadlock.
     */
    ret = envp->set_lk_detect(envp, DB_LOCK_MINWRITE);
    if (ret != 0) {
	fprintf(stderr, "Error setting lock detect: %s\n",
	    db_strerror(ret));
	goto err;
    }

    env_flags =
      DB_CREATE     |  /* Create the environment if it does not exist */
      DB_RECOVER    |  /* Run normal recovery. */
      DB_INIT_LOCK  |  /* Initialize the locking subsystem */
      DB_INIT_LOG   |  /* Initialize the logging subsystem */
      DB_INIT_TXN   |  /* Initialize the transactional subsystem. This
			* also turns on logging. */
      DB_INIT_MPOOL |  /* Initialize the memory pool (in-memory cache) */
      DB_THREAD;       /* Cause the environment to be free-threaded */

    /* Now actually open the environment */
    ret = envp->open(envp, db_home_dir, env_flags, 0);
    if (ret != 0) {
	fprintf(stderr, "Error opening environment: %s\n",
	    db_strerror(ret));
	goto err;
    }

    /*
     * If we had utility threads (for running checkpoints or
     * deadlock detection, for example) we would spawn those
     * here. However, for a simple example such as this,
     * that is not required.
     */

    /* Open the database */
    ret = open_db(&dbp, prog_name, file_name,
      envp, DB_DUPSORT);
    if (ret != 0)
	goto err;

    /* Initialize a mutex. Used to help provide thread ids. */
    (void)mutex_init(&thread_num_lock, NULL);

    /* Start the writer threads. */
    for (i = 0; i < NUMWRITERS; i++)
	(void)thread_create(
	   &writer_threads[i], NULL, writer_thread, (void *)dbp);

    /* Join the writers */
    for (i = 0; i < NUMWRITERS; i++)
	(void)thread_join(writer_threads[i], NULL);

err:
    /* Close our database handle, if it was opened. */
    if (dbp != NULL) {
	ret_t = dbp->close(dbp, 0);
	if (ret_t != 0) {
	    fprintf(stderr, "%s database close failed: %s\n",
		file_name, db_strerror(ret_t));
	    ret = ret_t;
	}
    }

    /* Close our environment, if it was opened. */
    if (envp != NULL) {
	ret_t = envp->close(envp, 0);
	if (ret_t != 0) {
	    fprintf(stderr, "environment close failed: %s\n",
		db_strerror(ret_t));
		ret = ret_t;
	}
    }

    /* Final status message and return. */
    printf("I'm all done.\n");
    return (ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
 * A function that performs a series of writes to a
 * Berkeley DB database. The information written
 * to the database is largely nonsensical, but the
 * mechanism of transactional commit/abort and
 * deadlock detection is illustrated here.
 */
void *
writer_thread(void *args)
{
    static char *key_strings[] = {
	"key 1", "key 2", "key 3", "key 4", "key 5",
	"key 6", "key 7", "key 8", "key 9", "key 10"
    };
    DB *dbp;
    DB_ENV *envp;
    DBT key, value;
    DB_TXN *txn;
    int i, j, payload, ret, thread_num;
    int retry_count, max_retries = 20;   /* Max retry on a deadlock */

    dbp = (DB *)args;
    envp = dbp->get_env(dbp);

    /* Get the thread number */
    (void)mutex_lock(&thread_num_lock);
    global_thread_num++;
    thread_num = global_thread_num;
    (void)mutex_unlock(&thread_num_lock);

    /* Initialize the random number generator */
    srand(thread_num);

    /* Write 50 times and then quit */
    for (i = 0; i < 50; i++) {
	retry_count = 0; /* Used for deadlock retries */

	/*
	 * Some think it is bad form to loop with a goto statement, but
	 * we do it anyway because it is the simplest and clearest way
	 * to achieve our abort/retry operation.
	 */
retry:
	/* Begin our transaction. We group multiple writes in
	 * this thread under a single transaction so as to
	 * (1) show that you can atomically perform multiple writes
	 * at a time, and (2) to increase the chances of a
	 * deadlock occurring so that we can observe our
	 * deadlock detection at work.
	 *
	 * Normally we would want to avoid the potential for deadlocks,
	 * so for this workload the correct thing would be to perform our
	 * puts with autocommit. But that would excessively simplify our
	 * example, so we do the "wrong" thing here instead.
	 */
	ret = envp->txn_begin(envp, NULL, &txn, 0);
	if (ret != 0) {
	    envp->err(envp, ret, "txn_begin failed");
	    return ((void *)EXIT_FAILURE);
	}
	for (j = 0; j < 10; j++) {
	    /* Set up our key and values DBTs */
	    memset(&key, 0, sizeof(DBT));
	    key.data = key_strings[j];
	    key.size = (u_int32_t)strlen(key_strings[j]) + 1;

	    memset(&value, 0, sizeof(DBT));
	    payload = rand() + i;
	    value.data = &payload;
	    value.size = sizeof(int);

	    /* Perform the database put. */
	    switch (ret = dbp->put(dbp, txn, &key, &value, 0)) {
		case 0:
		    break;
		/*
		 * Our database is configured for sorted duplicates,
		 * so there is a potential for a KEYEXIST error return.
		 * If we get one, simply ignore it and continue on.
		 *
		 * Note that you will see KEYEXIST errors only after you
		 * have run this program at least once.
		 */
		case DB_KEYEXIST:
		    printf("Got keyexists.\n");
		    break;
		/*
		 * Here's where we perform deadlock detection. If
		 * DB_LOCK_DEADLOCK is returned by the put operation,
		 * then this thread has been chosen to break a deadlock.
		 * It must abort its operation, and optionally retry the
		 * put.
		 */
		case DB_LOCK_DEADLOCK:
		    /*
		     * First thing that we MUST do is abort the
		     * transaction.
		     */
		    (void)txn->abort(txn);
		    /*
		     * Now we decide if we want to retry the operation.
		     * If we have retried less than max_retries,
		     * increment the retry count and goto retry.
		     */
		    if (retry_count < max_retries) {
			printf("Writer %i: Got DB_LOCK_DEADLOCK.\n",
			    thread_num);
			printf("Writer %i: Retrying write operation.\n",
			    thread_num);
			retry_count++;
			goto retry;
		    }
		    /*
		     * Otherwise, just give up.
		     */
		    printf("Writer %i: ", thread_num);
		    printf("Got DB_LOCK_DEADLOCK and out of retries.\n");
		    printf("Writer %i: Giving up.\n", thread_num);
		    return ((void *)EXIT_FAILURE);
		/*
		 * If a generic error occurs, we simply abort the
		 * transaction and exit the thread completely.
		 */
		default:
		    envp->err(envp, ret, "db put failed");
		    ret = txn->abort(txn);
		    if (ret != 0)
			envp->err(envp, ret,
			    "txn abort failed");
		    return ((void *)EXIT_FAILURE);
	     } /** End case statement **/

	}   /** End for loop **/

	/*
	 * print the number of records found in the database.
	 * See count_records() for usage information.
	 */
	printf("Thread %i. Record count: %i\n", thread_num,
	    count_records(dbp, NULL));

	/*
	 * If all goes well, we can commit the transaction and
	 * exit the thread.
	 */
	ret = txn->commit(txn, 0);
	if (ret != 0) {
	    envp->err(envp, ret, "txn commit failed");
	    return ((void *)EXIT_FAILURE);
	}
    }
    return ((void *)EXIT_SUCCESS);
}

/*
 * This simply counts the number of records contained in the
 * database and returns the result. You can use this function
 * in three ways:
 *
 * First call it with an active txn handle.
 * Secondly, configure the cursor for uncommitted reads (this
 *    is what the example currently does).
 * Third, call count_records AFTER the writer has committed
 *    its transaction.
 *
 * If you do none of these things, the writer thread will
 * self-deadlock.
 *
 * Note that this function exists only for illustrative purposes.
 * A more straight-forward way to count the number of records in
 * a database is to use DB->stat() or DB->stat_print().
 */

int
count_records(DB *dbp, DB_TXN *txn)
{

    DBT key, value;
    DBC *cursorp;
    int count, ret;

    cursorp = NULL;
    count = 0;

    /* Get the cursor */
    ret = dbp->cursor(dbp, txn, &cursorp,
	DB_READ_UNCOMMITTED);
    if (ret != 0) {
	dbp->err(dbp, ret,
	  "count_records: cursor open failed.");
	goto cursor_err;
    }

    /* Get the key DBT used for the database read */
    memset(&key, 0, sizeof(DBT));
    memset(&value, 0, sizeof(DBT));
    do {
	ret = cursorp->get(cursorp, &key, &value, DB_NEXT);
	switch (ret) {
	    case 0:
		count++;
		break;
	    case DB_NOTFOUND:
		break;
	    default:
		dbp->err(dbp, ret,
		    "Count records unspecified error");
		goto cursor_err;
	}
    } while (ret == 0);

cursor_err:
    if (cursorp != NULL) {
	ret = cursorp->close(cursorp);
	if (ret != 0) {
	    dbp->err(dbp, ret,
		"count_records: cursor close failed.");
	}
    }

    return (count);
}

/* Open a Berkeley DB database */
int
open_db(DB **dbpp, const char *progname, const char *file_name,
  DB_ENV *envp, u_int32_t extra_flags)
{
    int ret;
    u_int32_t open_flags;
    DB *dbp;

    /* Initialize the DB handle */
    ret = db_create(&dbp, envp, 0);
    if (ret != 0) {
	fprintf(stderr, "%s: %s\n", progname,
		db_strerror(ret));
	return (EXIT_FAILURE);
    }

    /* Point to the memory malloc'd by db_create() */
    *dbpp = dbp;

    if (extra_flags != 0) {
	ret = dbp->set_flags(dbp, extra_flags);
	if (ret != 0) {
	    dbp->err(dbp, ret,
		"open_db: Attempt to set extra flags failed.");
	    return (EXIT_FAILURE);
	}
    }

    /* Now open the database */
    open_flags = DB_CREATE              | /* Allow database creation */
		 DB_READ_UNCOMMITTED    | /* Allow dirty reads */
		 DB_AUTO_COMMIT;          /* Allow autocommit */

    ret = dbp->open(dbp,        /* Pointer to the database */
		    NULL,       /* Txn pointer */
		    file_name,  /* File name */
		    NULL,       /* Logical db name */
		    DB_BTREE,   /* Database type (using btree) */
		    open_flags, /* Open flags */
		    0);         /* File mode. Using defaults */
    if (ret != 0) {
	dbp->err(dbp, ret, "Database '%s' open failed",
	    file_name);
	return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}
