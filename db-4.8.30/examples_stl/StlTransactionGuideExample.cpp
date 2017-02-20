/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

// File txn_guide_stl.cpp
#include <iostream>
#include <db_cxx.h>

#include "dbstl_map.h"

#ifdef _WIN32
#include <windows.h>
extern "C" {
    extern int _tgetopt(int nargc, TCHAR* const* nargv, const TCHAR * ostr);
    extern TCHAR *optarg;
}
#define PATHD '\\'

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
using namespace dbstl;
typedef db_multimap<const char *, int, ElementHolder<int> > strmap_t;
// Printing of thread_t is implementation-specific, so we
// create our own thread IDs for reporting purposes.
int global_thread_num;
mutex_t thread_num_lock;

// Forward declarations
int countRecords(strmap_t *);
int openDb(Db **, const char *, const char *, DbEnv *, u_int32_t);
int usage(void);
void *writerThread(void *);

// Usage function
int
usage()
{
    std::cerr << " [-h <database_home_directory>] [-m (in memory use)]" 
              << std::endl;
    return (EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
    // Initialize our handles
    Db *dbp = NULL;
    DbEnv *envp = NULL;

    thread_t writerThreads[NUMWRITERS];
    int i, inmem;
    u_int32_t envFlags;
    const char *dbHomeDir;

    inmem = 0;
    // Application name
    const char *progName = "TxnGuideStl";

    // Database file name
    const char *fileName = "mydb.db";

    // Parse the command line arguments
#ifdef _WIN32
    dbHomeDir = ".\\TESTDIR";
#else
    dbHomeDir = "./TESTDIR";
#endif

    // Env open flags
    envFlags =
      DB_CREATE     |  // Create the environment if it does not exist
      DB_RECOVER    |  // Run normal recovery.
      DB_INIT_LOCK  |  // Initialize the locking subsystem
      DB_INIT_LOG   |  // Initialize the logging subsystem
      DB_INIT_TXN   |  // Initialize the transactional subsystem. This
                       // also turns on logging.
      DB_INIT_MPOOL |  // Initialize the memory pool (in-memory cache)
      DB_THREAD;       // Cause the environment to be free-threaded

    try {
        // Create and open the environment
        envp = new DbEnv(DB_CXX_NO_EXCEPTIONS);

        // Indicate that we want db to internally perform deadlock
        // detection.  Also indicate that the transaction with
        // the fewest number of write locks will receive the
        // deadlock notification in the event of a deadlock.
        envp->set_lk_detect(DB_LOCK_MINWRITE);

        if (inmem) {
            envp->set_lg_bsize(64 * 1024 * 1024);
            envp->open(NULL, envFlags, 0644);
            fileName = NULL;
        } else
            envp->open(dbHomeDir, envFlags, 0644);

        // If we had utility threads (for running checkpoints or
        // deadlock detection, for example) we would spawn those
        // here. However, for a simple example such as this,
        // that is not required.

        // Open the database
        openDb(&dbp, progName, fileName,
            envp, DB_DUP);

        // Call this function before any use of dbstl in a single thread 
        // if multiple threads are using dbstl.
        dbstl::dbstl_startup(); 

        // We created the dbp and envp handles not via dbstl::open_db/open_env
        // functions, so we must register the handles in each thread using the
        // container.
        dbstl::register_db(dbp);
        dbstl::register_db_env(envp);

        strmap_t *strmap = new strmap_t(dbp, envp);
        // Initialize a mutex. Used to help provide thread ids.
        (void)mutex_init(&thread_num_lock, NULL);

        // Start the writer threads.
        for (i = 0; i < NUMWRITERS; i++)
            (void)thread_create(&writerThreads[i], NULL,
                writerThread, (void *)strmap);
        

        // Join the writers
        for (i = 0; i < NUMWRITERS; i++)
            (void)thread_join(writerThreads[i], NULL);

        delete strmap;

    } catch(DbException &e) {
        std::cerr << "Error opening database environment: "
                  << (inmem ? "NULL" : dbHomeDir) << std::endl;
        std::cerr << e.what() << std::endl;
        dbstl_exit();
        return (EXIT_FAILURE);
    }

    // Environment and database will be automatically closed by dbstl.

    // Final status message and return.

    std::cout << "I'm all done." << std::endl;
    
    dbstl_exit();
    delete envp;
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
    int max_retries = 1;   // Max retry on a deadlock
    const char *key_strings[] = {"key 1", "key 2", "key 3", "key 4",
                           "key 5", "key 6", "key 7", "key 8",
                           "key 9", "key 10"};

    strmap_t *strmap = (strmap_t *)args;
    DbEnv *envp = strmap->get_db_env_handle();

    // We created the dbp and envp handles not via dbstl::open_db/open_env
    // functions, so we must register the handles in each thread using the
    // container.
    dbstl::register_db(strmap->get_db_handle());
    dbstl::register_db_env(envp);

    // Get the thread number
    (void)mutex_lock(&thread_num_lock);
    global_thread_num++;
    thread_num = global_thread_num;
    (void)mutex_unlock(&thread_num_lock);

    // Initialize the random number generator
    srand(thread_num);

    // Perform 50 transactions
    for (int i = 0; i < 1; i++) {
        DbTxn *txn;
        int retry = 100;
        int retry_count = 0, payload;
        // while loop is used for deadlock retries
        while (retry--) {
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
                txn = dbstl::begin_txn(0, envp);

                // Perform the database write for this transaction.
                for (j = 0; j < 10; j++) {
                    payload = rand() + i;
                    strmap->insert(make_pair(key_strings[j], payload));
                }

                // countRecords runs a cursor over the entire database.
                // We do this to illustrate issues of deadlocking
                std::cout << thread_num <<  " : Found "
                          << countRecords(strmap)
                          << " records in the database." << std::endl;

                std::cout << thread_num <<  " : committing txn : " << i
                          << std::endl;

                // commit
                try {
                    dbstl::commit_txn(envp);
                } catch (DbException &e) {
                    std::cout << "Error on txn commit: "
                              << e.what() << std::endl;
                }
            } catch (DbDeadlockException &) {
                // First thing that we MUST do is abort the transaction.
                try {
                    dbstl::abort_txn(envp);
                } catch (DbException ex1) {
            std::cout<<ex1.what();
                }

                // Now we decide if we want to retry the operation.
                // If we have retried less than max_retries,
                // increment the retry count and goto retry.
                if (retry_count < max_retries) {
                    std::cout << "############### Writer " << thread_num
                              << ": Got DB_LOCK_DEADLOCK.\n"
                              << "Retrying write operation."
                              << std::endl;
                    retry_count++;
 
                 } else {
                    // Otherwise, just give up.
                    std::cerr << "Writer " << thread_num
                              << ": Got DeadLockException and out of "
                              << "retries. Giving up." << std::endl;
                    retry = 0;
                 }
           } catch (DbException &e) {
                std::cerr << "db_map<> storage failed" << std::endl;
                std::cerr << e.what() << std::endl;
                dbstl::abort_txn(envp);
                retry = 0;
           } catch (std::exception &ee) {
                std::cerr << "Unknown exception: " << ee.what() << std::endl;
                return (0);
          }
        }
    }
    return (0);
}


// This simply counts the number of records contained in the
// database and returns the result. 
//
// Note that this method exists only for illustrative purposes.
// A more straight-forward way to count the number of records in
// a database is to use the db_map<>::size() method.
int
countRecords(strmap_t *strmap)
{

    int count = 0;
    strmap_t::iterator itr;
    try {
        // Set the flag used by Db::cursor.        
        for (itr = strmap->begin(); itr != strmap->end(); ++itr)
            count++;
    } catch (DbDeadlockException &de) {
        std::cerr << "countRecords: got deadlock" << std::endl;
        // itr's cursor will be automatically closed when it is destructed.
        throw de;
    } catch (DbException &e) {
        std::cerr << "countRecords error:" << std::endl;
        std::cerr << e.what() << std::endl;
    }

    // itr's cursor will be automatically closed when it is destructed.

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
        Db *dbp = new Db(envp, DB_CXX_NO_EXCEPTIONS);

        // Point to the new'd Db.
        *dbpp = dbp;

        if (extraFlags != 0)
            ret = dbp->set_flags(extraFlags);

        // Now open the database.
        openFlags = DB_CREATE              | // Allow database creation
                    DB_READ_UNCOMMITTED    | // Allow uncommitted reads
                    DB_AUTO_COMMIT;          // Allow autocommit

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

