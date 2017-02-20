/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

// File TxnGuideInMemory.cpp

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <db_cxx.h>

#ifdef _WIN32
#include <windows.h>
#define PATHD '\\'
extern "C" {
    extern int getopt(int, char * const *, const char *);
    extern char *optarg;
}

typedef HANDLE thread_t;
#define thread_create(thrp, attr, func, arg)                               \
    (((*(thrp) = CreateThread(NULL, 0,                                     \
        (LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL)) == NULL) ? -1 : 0)
#define thread_join(thr, statusp)                                          \
    ((WaitForSingleObject((thr), INFINITE) == WAIT_OBJECT_0) &&            \
    ((statusp == NULL) ? 0 :                           \
    (GetExitCodeThread((thr), (LPDWORD)(statusp)) ? 0 : -1)))

typedef HANDLE mutex_t;
#define mutex_init(m, attr)                                                \
    (((*(m) = CreateMutex(NULL, FALSE, NULL)) != NULL) ? 0 : -1)
#define mutex_lock(m)                                                      \
    ((WaitForSingleObject(*(m), INFINITE) == WAIT_OBJECT_0) ? 0 : -1)
#define mutex_unlock(m)         (ReleaseMutex(*(m)) ? 0 : -1)
#else
#include <pthread.h>
#include <unistd.h>
#define PATHD '/'

typedef pthread_t thread_t;
#define thread_create(thrp, attr, func, arg)                               \
    pthread_create((thrp), (attr), (func), (arg))
#define thread_join(thr, statusp) pthread_join((thr), (statusp))

typedef pthread_mutex_t mutex_t;
#define mutex_init(m, attr)     pthread_mutex_init((m), (attr))
#define mutex_lock(m)           pthread_mutex_lock(m)
#define mutex_unlock(m)         pthread_mutex_unlock(m)
#endif

// Run 5 writers threads at a time.
#define NUMWRITERS 5

// Printing of pthread_t is implementation-specific, so we
// create our own thread IDs for reporting purposes.
int global_thread_num;
mutex_t thread_num_lock;

// Forward declarations
int countRecords(Db *, DbTxn *);
int openDb(Db **, const char *, const char *, DbEnv *, u_int32_t);
int usage(void);
void *writerThread(void *);

int
main(void)
{
    // Initialize our handles
    Db *dbp = NULL;
    DbEnv *envp = NULL;

    thread_t writerThreads[NUMWRITERS];
    int i;
    u_int32_t envFlags;

    // Application name
    const char *progName = "TxnGuideInMemory";

    // Env open flags
    envFlags =
      DB_CREATE     |  // Create the environment if it does not exist
      DB_RECOVER    |  // Run normal recovery.
      DB_INIT_LOCK  |  // Initialize the locking subsystem
      DB_INIT_LOG   |  // Initialize the logging subsystem
      DB_INIT_TXN   |  // Initialize the transactional subsystem. This
                       // also turns on logging.
      DB_INIT_MPOOL |  // Initialize the memory pool (in-memory cache)
      DB_PRIVATE    |  // Region files are not backed by the filesystem.
                       // Instead, they are backed by heap memory.
      DB_THREAD;       // Cause the environment to be free-threaded

    try {
        // Create the environment
        envp = new DbEnv(0);

        // Specify in-memory logging
        envp->log_set_config(DB_LOG_IN_MEMORY, 1);

        // Specify the size of the in-memory log buffer.
        envp->set_lg_bsize(10 * 1024 * 1024);

        // Specify the size of the in-memory cache
        envp->set_cachesize(0, 10 * 1024 * 1024, 1);

        // Indicate that we want db to internally perform deadlock
        // detection.  Also indicate that the transaction with
        // the fewest number of write locks will receive the
        // deadlock notification in the event of a deadlock.
        envp->set_lk_detect(DB_LOCK_MINWRITE);

        // Open the environment
        envp->open(NULL, envFlags, 0);

        // If we had utility threads (for running checkpoints or
        // deadlock detection, for example) we would spawn those
        // here. However, for a simple example such as this,
        // that is not required.

        // Open the database
        openDb(&dbp, progName, NULL,
            envp, DB_DUPSORT);

        // Initialize a mutex. Used to help provide thread ids.
        (void)mutex_init(&thread_num_lock, NULL);

        // Start the writer threads.
        for (i = 0; i < NUMWRITERS; i++)
            (void)thread_create(
                &writerThreads[i], NULL,
                writerThread,
                (void *)dbp);

        // Join the writers
        for (i = 0; i < NUMWRITERS; i++)
            (void)thread_join(writerThreads[i], NULL);

    } catch(DbException &e) {
        std::cerr << "Error opening database environment: "
                  << std::endl;
        std::cerr << e.what() << std::endl;
        return (EXIT_FAILURE);
    }

    try {
        // Close our database handle if it was opened.
        if (dbp != NULL)
            dbp->close(0);

        // Close our environment if it was opened.
        if (envp != NULL)
            envp->close(0);
    } catch(DbException &e) {
        std::cerr << "Error closing database and environment."
                  << std::endl;
        std::cerr << e.what() << std::endl;
        return (EXIT_FAILURE);
    }

    // Final status message and return.

    std::cout << "I'm all done." << std::endl;
    return (EXIT_SUCCESS);
}

// A function that performs a series of writes to a
// Berkeley DB database. The information written
// to the database is largely nonsensical, but the
// mechanism of transactional commit/abort and
// deadlock detection is illustrated here.
void *
writerThread(void *args)
{
    int j, thread_num;
    int max_retries = 20;   // Max retry on a deadlock
    const char *key_strings[] = {"key 1", "key 2", "key 3", "key 4",
                           "key 5", "key 6", "key 7", "key 8",
                           "key 9", "key 10"};

    Db *dbp = (Db *)args;
    DbEnv *envp = dbp->get_env();

    // Get the thread number
    (void)mutex_lock(&thread_num_lock);
    global_thread_num++;
    thread_num = global_thread_num;
    (void)mutex_unlock(&thread_num_lock);

    // Initialize the random number generator
    srand(thread_num);

    // Perform 50 transactions
    for (int i=0; i<50; i++) {
        DbTxn *txn;
        bool retry = true;
        int retry_count = 0;
        // while loop is used for deadlock retries
        while (retry) {
            // try block used for deadlock detection and
            // general db exception handling
            try {

                // Begin our transaction. We group multiple writes in
                // this thread under a single transaction so as to
                // (1) show that you can atomically perform multiple
                // writes at a time, and (2) to increase the chances
                // of a deadlock occurring so that we can observe our
                // deadlock detection at work.

                // Normally we would want to avoid the potential for
                // deadlocks, so for this workload the correct thing
                // would be to perform our puts with autocommit. But
                // that would excessively simplify our example, so we
                // do the "wrong" thing here instead.
                txn = NULL;
                envp->txn_begin(NULL, &txn, 0);

                // Perform the database write for this transaction.
                for (j = 0; j < 10; j++) {
                    Dbt key, value;
                    key.set_data((void *)key_strings[j]);
                    key.set_size((u_int32_t)strlen(key_strings[j]) + 1);

                    int payload = rand() + i;
                    value.set_data(&payload);
                    value.set_size(sizeof(int));

                    // Perform the database put
                    dbp->put(txn, &key, &value, 0);
                }

                // countRecords runs a cursor over the entire database.
                // We do this to illustrate issues of deadlocking
                std::cout << thread_num <<  " : Found "
                          <<  countRecords(dbp, txn)
                          << " records in the database." << std::endl;

                std::cout << thread_num <<  " : committing txn : " << i
                          << std::endl;

                // commit
                try {
                    txn->commit(0);
                    retry = false;
                    txn = NULL;
                } catch (DbException &e) {
                    std::cout << "Error on txn commit: "
                              << e.what() << std::endl;
                }
            } catch (DbDeadlockException &) {
                // First thing that we MUST do is abort the transaction.
                if (txn != NULL)
                    (void)txn->abort();

                // Now we decide if we want to retry the operation.
                // If we have retried less than max_retries,
                // increment the retry count and goto retry.
                if (retry_count < max_retries) {
                    std::cerr << "############### Writer " << thread_num
                              << ": Got DB_LOCK_DEADLOCK.\n"
                              << "Retrying write operation." << std::endl;
                    retry_count++;
                    retry = true;
                 } else {
                    // Otherwise, just give up.
                    std::cerr << "Writer " << thread_num
                              << ": Got DeadLockException and out of "
                              << "retries. Giving up." << std::endl;
                    retry = false;
                 }
           } catch (DbException &e) {
                std::cerr << "db put failed" << std::endl;
                std::cerr << e.what() << std::endl;
                if (txn != NULL)
                    txn->abort();
                retry = false;
           } catch (std::exception &ee) {
            std::cerr << "Unknown exception: " << ee.what() << std::endl;
            return (0);
          }
        }
    }
    return (0);
}


// This simply counts the number of records contained in the
// database and returns the result. You can use this method
// in three ways:
//
// First call it with an active txn handle.
//
// Secondly, configure the cursor for uncommitted reads
//
// Third, call countRecords AFTER the writer has committed
//    its transaction.
//
// If you do none of these things, the writer thread will
// self-deadlock.
//
// Note that this method exists only for illustrative purposes.
// A more straight-forward way to count the number of records in
// a database is to use the Database.getStats() method.
int
countRecords(Db *dbp, DbTxn *txn)
{

    Dbc *cursorp = NULL;
    int count = 0;

    try {
        // Get the cursor
        dbp->cursor(txn, &cursorp, 0);

        Dbt key, value;
        while (cursorp->get(&key, &value, DB_NEXT) == 0) {
            count++;
        }
    } catch (DbDeadlockException &de) {
        std::cerr << "countRecords: got deadlock" << std::endl;
        cursorp->close();
        throw de;
    } catch (DbException &e) {
        std::cerr << "countRecords error:" << std::endl;
        std::cerr << e.what() << std::endl;
    }

    if (cursorp != NULL) {
        try {
            cursorp->close();
        } catch (DbException &e) {
            std::cerr << "countRecords: cursor close failed:" << std::endl;
            std::cerr << e.what() << std::endl;
        }
    }

    return (count);
}


// Open a Berkeley DB database
int
openDb(Db **dbpp, const char *progname, const char *fileName,
  DbEnv *envp, u_int32_t extraFlags)
{
    int ret;
    u_int32_t openFlags;

    try {
        Db *dbp = new Db(envp, 0);

        // Point to the new'd Db
        *dbpp = dbp;

        if (extraFlags != 0)
            ret = dbp->set_flags(extraFlags);

        // Now open the database */
        openFlags = DB_CREATE        | // Allow database creation
                    DB_THREAD        |
                    DB_AUTO_COMMIT;    // Allow autocommit

        dbp->open(NULL,       // Txn pointer
                  fileName,   // File name
                  NULL,       // Logical db name
                  DB_BTREE,   // Database type (using btree)
                  openFlags,  // Open flags
                  0);         // File mode. Using defaults
    } catch (DbException &e) {
        std::cerr << progname << ": openDb: db open failed:" << std::endl;
        std::cerr << e.what() << std::endl;
        return (EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}

