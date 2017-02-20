/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_RESOURCE_MANAGER_H__
#define _DB_STL_RESOURCE_MANAGER_H__

#include <map>
#include <vector>
#include <stack>
#include <set>

#include "dbstl_common.h"
#include "dbstl_inner_utility.h"

START_NS(dbstl)

class DbCursorBase;
using std::map;
using std::multimap;
using std::set;
using std::stack;

///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// ResourceManager class definition
//
// This class manages all the Berkeley DB handles and their mapping
// relationship. When it's only thread-specific instance is destructed,
// these handles are automatically closed in the correct order (the db and
// dbenv handles will be closed when the last thread using the handles in
// the process is destructed).
//
// The Db* and DbEnv* handles are process-wide global, they can be shared
// among multithreads, so they are stored into a static stl map, and access
// to the two map objects (open_dbs_ & open_envs_) is protected by a process
// wide mutex because few stl implementations support multithreaded access.
// We use reference counting in the two map objects to make sure each handle
// is closed when the last thread using it exits. Each thread sharing the
// handle should call ResourceManager::register_db/dbenv to tell DBSTL that
// it will use the handle, otherwise the handle may be closed prematurely.
//
// The transaction and cursor handles are thread specific. They are stored
// into stl containers and each instance of the ResourceManager is stored
// into thread local storage(TLS) of each thread. Thus the DB STL containers
// are thread safe.
//

// This map contains cursors of all open databases opened in the same thread.
// We can only duplicate a cursor of the same database; We don't allow sharing
// Berkeley DB cursor handles across multiple threads, each thread needs to
// open their own cursor handle;
//
typedef map<Db *, set<dbstl::DbCursorBase *> *> db_csr_map_t;

// The set of cursors that belong to a db handle or a transaction.
typedef set<dbstl::DbCursorBase *> csrset_t;
// The cursors opened in each transaction.
typedef map<DbTxn *, set<DbCursorBase *> *> txncsr_t;

// This stack contains the transactions started in current thread. Each
// transaction is the child transaction of the one under it in the stack.
//
// We support nested transactions for those created by the dbstl
// users, but still keep reference counting for dbstl internally
// created transactions so that the autocommit methods are really
// autocommit with least overhead (nested transactions are overheads). The
// way we keep both nested transaction and reference counting to internal
// transactions in the same stack is:
// 1. When creating an external transaction, look at the stack top, if there
// is a transaction, it must be an external one too, so use it as the parent
// transaction to create the external transaction.
// 2. When creating an internal transaction, look at the stack top, if there
// is one, call it T, look for its the reference counting, if there is a
// reference count for it, T is an internal one, so we increment its
// reference count; Otherwise, T is an external one, and according to the DB
// autocommit semantics, this function should be in T's context, so we
// simply use T and add it to the reference counting structure, and set its
// reference count to 2.
//
// We don't support expanding a transaction across multiple threads,
// because there are many restrictions to doing so, making it meaningless.
//
typedef stack<DbTxn *> txnstk_t;
typedef map<DbEnv *, txnstk_t> env_txns_t;

#ifdef WIN32
#pragma warning( push )
#pragma warning( disable:4251 )
#endif
// This class is used to wrap a ResourceManager instance pointer, so that
// each thread has its own ResourceManager singleton.

#ifdef TLS_DECL_MODIFIER
template <Typename T>
class TlsWrapper
{
public:
    static T *get_tls_obj()
    {
        return tinst_;
    }

    static void set_tls_obj(T *objp)
    {
        tinst_ = objp;
    }

private:
    TlsWrapper(){}

    // Thread local pointer to the instance of type T.
    static TLS_DECL_MODIFIER T *tinst_;
}; // TlsWrapper<>

#elif defined(HAVE_PTHREAD_TLS)
template <Typename T>
class TlsWrapper
{
public:
    static T *get_tls_obj()
    {
        return static_cast<T*>(pthread_getspecific(tls_key_));
    }

    static void set_tls_obj(T *objp)
    {
        pthread_setspecific(tls_key_, objp);
    }

    // Friend declarations don't work portably, so we have to declare 
    // tls_key_ public.
    static pthread_key_t tls_key_;
private:
    TlsWrapper();

}; // TlsWrapper<>

#else
#error "A multi-threaded build of STL for Berkeley DB requires thread local storage.  None is configured."
#endif

class _exported ResourceManager : public DbstlGlobalInnerObject
{
private:

    ResourceManager(void);
    // open_dbs_ & open_envs_ are shared among threads, protected by
    // ghdl_mtx;
    static map<Db *, size_t> open_dbs_;
    static map<DbEnv *, size_t>open_envs_;

    // Transaction stack of all environments. Use a stack to allow nested
    // transactions. The transaction at the top of the stack is the
    // current active transaction.
    //
    env_txns_t env_txns_;

    // Cursors opened in a corresponding transaction context. When
    // committing or aborting a transaction, first close all open cursors.
    //
    txncsr_t txn_csrs_;

    // If in container X, its methods X::A and X::B both call begin and
    // commit transaction. X::A calls X::B after it's begin transaction
    // call, then X::B will commit the transaction prematurally. To avoid
    // will commit the only transaction prematurally, to avoid this, we use
    // this, we use this map to record each transaction's reference count.
    // Each begin/commit_txn() will increment/decrement the reference
    // count, when reference count goes to 0, the transaction is committed.
    // Abort_txn will unconditionally abort the transaction.
    //
    map<DbTxn *, size_t> txn_count_;

    // Contains the cursors opened in the current thread for each database,
    // So that we can close them in the right way, freeing any Berkeley DB
    // handles before exiting.
    //
    db_csr_map_t all_csrs_;

    // Remove cursors opened in the transaction txn's context, should be
    // called before commiting or aborting a transaction.
    //
    void remove_txn_cursor(DbTxn *txn);

    // Add a cursor to the current transaction's set of open cursors.
    void add_txn_cursor(DbCursorBase *dcbcsr, DbEnv *env);

    // The environment mtx_env_ and mtx_handle_ are used for synchronizing
    // multiple threads' access to open_dbs_ and open_envs_ and glob_objs_.
    // They are discarded when program exits, no deallocation/release
    // is done.
    static DbEnv *mtx_env_;
    static db_mutex_t mtx_handle_;
    static db_mutex_t mtx_globj_;
    static set<DbstlGlobalInnerObject *> glob_objs_;

    // This set stores db handles that are new'ed by open_db, and thus 
    // should be deleted after this db is closed automatically by dbstl.
    // If a db is new'ed and created by user without using open_db, users
    // should delete it.
    static set<Db *> deldbs; 
    static set<DbEnv *> delenvs; // Similar to deldbs, works with open_envs.
    static void set_global_callbacks();
public:
    
    // This function should be called in a single thread inside a process, 
    // before any use of dbstl.
    static void global_startup();
    // Delete registered DbstlGlobalInnerObject objects.
    static void global_exit();
    static void register_global_object(DbstlGlobalInnerObject *gio);
    static DbEnv *get_mutex_env() { return mtx_env_; }
    // Lock mtx_handle_, if it is 0, allocate it first.
    static int global_lock(db_mutex_t dbcontainer_mtx);
    static int global_unlock(db_mutex_t dbcontainer_mtx);
    // Close pdb regardless of its reference count, users must make sure
    // pdb is not used by others before calling this method. We can't
    // close by reference count in this method, otherwise when the thread
    // exits pdb's reference count is decremented twice.
    void close_db(Db *pdb);

    // Close all db handles regardless of reference count, used to clean up
    // the calling thread's ResourceManager singleton.
    void close_all_dbs();

    // Close specified db env handle and remove it from resource manager.
    void close_db_env(DbEnv *penv);

    // Close and remove all db env registered in the resource manager.
    // Used to clean up the calling thread's ResourceManager singleton.
    void close_all_db_envs();

    // Begin a new transaction in the specified environment. When outtxn
    // is non-zero support nested transactions - the new transaction will
    // be started as a child of the current transaction. If outtxn is 0
    // we are starting an internal transaction for autocommit, no new
    // transaction will be started, but the current transaction's
    // reference count will be incremented if it already has a reference
    // count; otherwise, it was created by user, and we simply use this
    // transaction, set its reference count to 2.
    //
    // This function is called by both containers to begin an internal
    // transaction for autocommit, and by db stl users to begin an
    // external transaction.
    //
    DbTxn *begin_txn(u_int32_t flags/* flags for DbEnv::txn_begin */,
        DbEnv *env, int outtxn);

    // Decrement reference count if it exists and if it goes to 0, commit
    // current transaction T opened in env;
    // If T has no reference count(outside transaction), simply find
    // it by popping the stack and commit it.
    //
    // This function is called by db_container to commit a transaction
    // for auto commit, also can be called by db stl user to commit
    // an external explicit transaction.
    //
    void commit_txn(DbEnv *env, u_int32_t flags = 0);

    // Commit specified transaction txn: find it by popping the stack,
    // discard all its child transactions and commit it.
    //
    // This function is called by dbstl user to commit an external
    // explicit transaction.
    //
    void commit_txn(DbEnv *env, DbTxn *txn, u_int32_t flags = 0);

    // Abort current transaction of the environment.
    //
    void abort_txn(DbEnv *env);

    // Abort specified transaction: find it by popping the stack, discard
    // all its child transactions and abort it.
    //
    // This function is called by dbstl user to abort an external
    // explicit transaction.
    //
    void abort_txn(DbEnv *env, DbTxn *txn);

    // Set env's current transaction handle. The original transaction
    // handle is returned without aborting or commiting. This API can be
    // used to share a transaction handle among multiple threads.
    DbTxn* set_current_txn_handle(DbEnv *env, DbTxn *newtxn);

    // Register a Db handle. This handle and handles opened in it
    // will be closed by ResourceManager, so application code must not
    // try to close or delete it. Users can configure the handle before
    // opening the Db and then register it via this function.
    //
    void register_db(Db *pdb1);

    // Register a DbEnv handle. This handle and handles opened in it
    // will be closed by ResourceManager, so application code must not try
    // to close or delete it. Users can configure the handle before
    // opening the Environment and then register it via this function.
    //
    void register_db_env(DbEnv *env1);

    // Helper function to open a database and register it into
    // ResourceManager.
    // Users can set the create flags, open flags, db type, and flags
    // needing to be set before open.
    //
    Db* open_db (DbEnv *penv, const char *filename, DBTYPE dbtype,
        u_int32_t oflags, u_int32_t set_flags, int mode = 0644,
        DbTxn* txn = NULL, u_int32_t cflags = 0,
        const char *dbname = NULL);

    // Helper function to open a dbenv and register it into
    // ResourceManager.
    // Users can set the create flags, open flags, db type, and flags
    // needing to be set before open.
    //
    DbEnv *open_env(const char *env_home, u_int32_t set_flags,
        u_int32_t oflags = DB_CREATE | DB_INIT_MPOOL,
        u_int32_t cachesize = 4 * 1024 * 1024, int mode = 0644,
        u_int32_t cflags = 0/* Flags for DbEnv constructor. */);

    static ResourceManager *instance();

    // Release all registered resource in the right order.
    virtual ~ResourceManager(void);

    // Return current transaction of environment env, that is, the one on
    // the transaction stack top, the active one.
    //
    DbTxn *current_txn(DbEnv *env);

    // Open a Berkeley DB cursor.
    //
    int open_cursor(DbCursorBase *dcbcsr, Db *pdb, int flags = 0);

    // Add a db-cursor mapping.
    void add_cursor(Db *dbp, DbCursorBase *csr);

    // Close all cursors opened in the database.
    size_t close_db_cursors(Db *dbp1);

    // Close and remove a cursor from ResourceManager.
    //
    int remove_cursor(DbCursorBase *csr, bool remove_from_txncsrs = true);

}; // ResourceManager

END_NS
#ifdef WIN32
#pragma warning( pop )
#endif
#endif // !_DB_STL_RESOURCE_MANAGER_H__
