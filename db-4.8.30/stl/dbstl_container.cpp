/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <string>

#include "dbstl_container.h"
#include "dbstl_resource_manager.h"
#include "dbstl_exception.h"
#include "dbstl_utility.h"
#include "dbstl_inner_utility.h"

typedef struct {
    time_t  tv_sec;             /* seconds */
    long    tv_nsec;            /* nanoseconds */
} db_timespec;

extern "C"{
void __os_id (DB_ENV *, pid_t *, db_threadid_t*);
void __os_gettime(ENV *env, db_timespec *tp, int monotonic);
}

START_NS(dbstl)

using std::string;
u_int32_t db_container::g_db_file_suffix_ = 0;

void set_global_dbfile_suffix_number(u_int32_t num)
{
    db_container::g_db_file_suffix_ = num;
}

// Internally used memory allocation functions, they will throw an exception
// of NotEnoughMemoryException if can't allocate memory.
void *DbstlReAlloc(void *ptr, size_t size)
{
    void *p;

    assert(size != 0);
    if ((p = realloc(ptr, size)) == NULL)
        THROW(NotEnoughMemoryException, 
            ("DbstlReAlloc failed to allocate memory", size));

    return p;
}

void *DbstlMalloc(size_t size)
{
    void *p;

    assert(size != 0);
    if ((p = malloc(size)) == NULL)
        THROW(NotEnoughMemoryException, 
            ("DbstlMalloc failed to allocate memory", size));
    return p;
}

void db_container::init_members()
{
    txn_begin_flags_ = 0; 
    commit_flags_ = 0; 
    cursor_oflags_ = 0; 
    pdb_ = NULL;
    is_set_ = false;
    auto_commit_ = false; 
    dbenv_ = NULL;
}

void db_container::init_members(Db *dbp, DbEnv *envp)
{
    txn_begin_flags_ = 0; 
    commit_flags_ = 0; 
    is_set_ = false;
    cursor_oflags_ = 0; 
    pdb_ = dbp;
    dbenv_ = envp;
    set_auto_commit(pdb_);
}

db_container::db_container()
{
    init_members();
}

db_container::db_container(const db_container& dbctnr)
{
    init_members(dbctnr);
}

db_container::db_container(Db *dbp, DbEnv *envp) 
{
    init_members(dbp, envp);
}

void db_container::init_members(const db_container&dbctnr)
{
    txn_begin_flags_ = dbctnr.txn_begin_flags_; 
    commit_flags_ = dbctnr.commit_flags_; 
    cursor_oflags_ = dbctnr.cursor_oflags_; 
    // We don't copy database handles because we will clone another 
    // database from dbctnr's db, and get its handle. We will 
    // copy the following database properties because they will be
    // definitely identical.
    //
    pdb_ = NULL;
    is_set_ = dbctnr.is_set_;
    auto_commit_ = dbctnr.auto_commit_; 
    dbenv_ = dbctnr.dbenv_;
}
void db_container::open_db_handles(Db *&pdb, DbEnv *&penv, DBTYPE dbtype, 
    u_int32_t oflags, u_int32_t sflags)
{
    if (pdb == NULL) {
        pdb = open_db(penv, NULL, dbtype, oflags, sflags);
        this->pdb_ = pdb;
    }

    if (penv == NULL) {
        penv = pdb->get_env();
        this->dbenv_ = penv;
        set_auto_commit(pdb_);
    }
}

Db* db_container::clone_db_config(Db *dbp)
{
    string str;

    return clone_db_config(dbp, str);
}

// Open a new db with identical configuration to dbp. The dbfname brings 
// back the generated db file name.
Db* db_container::clone_db_config(Db *dbp, string &dbfname)
{
    Db *tdb = NULL;
    int ret;
    DBTYPE dbtype;
    u_int32_t oflags, sflags;
    const char *dbfilename, *dbname, *tdbname;

    BDBOP2(dbp->get_type(&dbtype), ret, dbp->close(0));
    BDBOP2(dbp->get_open_flags(&oflags), ret, dbp->close(0));
    BDBOP2(dbp->get_flags(&sflags), ret, dbp->close(0));
    
    BDBOP (dbp->get_dbname(&dbfilename, &dbname), ret);
    if (dbfilename == NULL) {
        tdb = open_db(dbp->get_env(), 
            dbfilename, dbtype, oflags, sflags, 0420, NULL, 0, dbname);
        dbfname.assign("");

    } else {
        construct_db_file_name(dbfname);
        tdbname = dbfname.c_str();
        tdb = open_db(dbp->get_env(), tdbname, dbtype, oflags, sflags);
    }

    return tdb;
}

int db_container::construct_db_file_name(string &filename) const
{
    db_threadid_t tid;
    db_timespec ts;
    int len;
    char name[64];

    __os_gettime(NULL, &ts, 1);
    __os_id(NULL, NULL, &tid);

    // avoid name clash
    len = _snprintf(name, 64, "tmpdb_db_map_%lu_%d_%u.db", 
        (u_long)((uintptr_t)tid + ts.tv_nsec), rand(), g_db_file_suffix_++);
    filename = name;

    return 0;
}

void db_container::set_auto_commit(Db *db) 
{
    u_int32_t envof, envf, dbf;

    if (db == NULL || dbenv_ == NULL) {
        auto_commit_ = false;
        return;
    }

    dbenv_->get_open_flags(&envof);
    if ((envof & DB_INIT_TXN) == 0) {
        this->auto_commit_ = false;
    } else {
        dbenv_->get_flags(&envf);
        db->get_flags(&dbf);
        if (((envf & DB_AUTO_COMMIT) != 0) || 
            ((dbf & DB_AUTO_COMMIT) != 0))
            this->auto_commit_ = true;
        else
            this->auto_commit_ = false;
    }
}

void db_container::set_db_handle(Db *dbp, DbEnv *newenv)
{
    const char *errmsg;

    if ((errmsg = verify_config(dbp, newenv)) != NULL) {
        THROW(InvalidArgumentException, ("Db*", errmsg));
        
    }

    pdb_ = dbp;
    if (newenv)
        dbenv_ = newenv;
    
}

void db_container::verify_db_handles(const db_container &cntnr) const
{
    Db *pdb2 = cntnr.get_db_handle();
    const char *home = NULL, *home2 = NULL, *dbf = NULL, *dbn = NULL,
        *dbf2 = NULL, *dbn2 = NULL;
    int ret = 0;
    u_int32_t flags = 0, flags2 = 0;
    bool same_dbfile, same_dbname, anonymous_inmemdbs;
    // Check the two database handles do not refer to the same database.
    // If they don't point to two anonymous databases at the same time,
    // then two identical file names and two identical database names 
    // mean the two databases are the same.
    assert(this->pdb_ != pdb2);
    if (pdb_ == NULL)
        return;

    BDBOP(pdb_->get_dbname(&dbf, &dbn), ret);
    BDBOP(pdb2->get_dbname(&dbf2, &dbn2), ret);

    anonymous_inmemdbs = (dbf == NULL && dbf2 == NULL && 
        dbn == NULL && dbn2 == NULL);

    same_dbfile = (dbf != NULL && dbf2 != NULL && (strcmp(dbf, dbf2) == 0))
        || (dbf == NULL && dbf2 == NULL);

    same_dbname = (dbn == NULL && dbn2 == NULL) ||
        (dbn != NULL && dbn2 != NULL && strcmp(dbn, dbn2) == 0);

    assert((!(anonymous_inmemdbs) && same_dbfile && same_dbname) == false);

    // If any one of the two environments are transactional, both of them
    // should be opened in the same transactional environment.
    DbEnv *penv2 = cntnr.get_db_env_handle();
    if (dbenv_ != penv2 ){
        BDBOP(this->dbenv_->get_open_flags(&flags), ret);
        BDBOP(penv2->get_open_flags(&flags2), ret);

        if ((flags & DB_INIT_TXN) || (flags2 & DB_INIT_TXN)) {
            BDBOP(dbenv_->get_home(&home), ret);
            BDBOP(penv2->get_home(&home), ret);
            assert(home != NULL && home2 != NULL && 
                strcmp(home, home2) == 0);
        }
    }
}

bool operator==(const Dbt&d1, const Dbt&d2)  
{
    if (d1.get_size() != d2.get_size())
        return false;

    return memcmp(d1.get_data(), d2.get_data(), 
        d2.get_size()) == 0;
}

bool operator==(const DBT&d1, const DBT&d2)  
{
    if (d1.size != d2.size)
        return false;
    return memcmp(d1.data, d2.data, d2.size) == 0;
}

void close_all_dbs()
{
    ResourceManager::instance()->close_all_dbs();
}

void close_db(Db *pdb) 
{
    ResourceManager::instance()->close_db(pdb);
}

DbTxn* begin_txn(u_int32_t flags, DbEnv*env) 
{
    return ResourceManager::instance()->begin_txn(flags, env, 1);
}

void commit_txn(DbEnv *env, u_int32_t flags) 
{
    ResourceManager::instance()->commit_txn(env, flags);
}

void commit_txn(DbEnv *env, DbTxn *txn, u_int32_t flags) 
{
    ResourceManager::instance()->commit_txn(env, txn, flags);
}

void abort_txn(DbEnv *env)
{
    ResourceManager::instance()->abort_txn(env);
}

void abort_txn(DbEnv *env, DbTxn *txn)
{
    ResourceManager::instance()->abort_txn(env, txn);
}

DbTxn* current_txn(DbEnv *env)
{
    return ResourceManager::instance()->current_txn(env);
}

DbTxn* set_current_txn_handle(DbEnv *env, DbTxn *newtxn)
{
    return ResourceManager::instance()->
        set_current_txn_handle(env, newtxn);
}

void register_db(Db *pdb1)
{
    ResourceManager::instance()->register_db(pdb1);
}

void register_db_env(DbEnv *env1) 
{
    ResourceManager::instance()->register_db_env(env1);
}

Db* open_db (DbEnv *penv, const char *filename, DBTYPE dbtype, 
    u_int32_t oflags, u_int32_t set_flags, int mode, 
    DbTxn *txn, u_int32_t cflags, const char *dbname) 
{
    return ResourceManager::instance()->open_db(
        penv, filename, dbtype, oflags, set_flags, mode, txn, 
        cflags, dbname);
}

DbEnv* open_env(const char *env_home, u_int32_t set_flags,
    u_int32_t oflags, u_int32_t cachesize, int mode, u_int32_t cflags)
{
    return ResourceManager::instance()->open_env(
        env_home, set_flags, oflags, cachesize, mode, cflags);
}

void close_db_env(DbEnv *pdbenv)
{

    ResourceManager::instance()->close_db_env(pdbenv);
}

void close_all_db_envs()
{
    ResourceManager::instance()->close_all_db_envs();
}

size_t close_db_cursors(Db *dbp1)
{
    return ResourceManager::instance()->close_db_cursors(dbp1);
}

db_mutex_t alloc_mutex() 
{
    int ret;
    db_mutex_t mtx;

    BDBOP2(ResourceManager::instance()->get_mutex_env()->mutex_alloc(
        DB_MUTEX_PROCESS_ONLY, &mtx), ret, ResourceManager::
        instance()->get_mutex_env()->mutex_free(mtx));
    return mtx;
}

int lock_mutex(db_mutex_t mtx)
{
    int ret;

    BDBOP2(ResourceManager::instance()->global_lock(mtx), ret, 
        ResourceManager::
        instance()->get_mutex_env()->mutex_free(mtx));
    return 0;
}

int unlock_mutex(db_mutex_t mtx)
{
    int ret;

    BDBOP2(ResourceManager::instance()->global_unlock(mtx), ret, 
        ResourceManager::
        instance()->get_mutex_env()->mutex_free(mtx));
    return 0;
}

void free_mutex(db_mutex_t mtx)
{
    ResourceManager::instance()->get_mutex_env()->mutex_free(mtx);
}

void dbstl_startup()
{
    ResourceManager::instance()->global_startup();
}

void dbstl_exit()
{
    ResourceManager::instance()->global_exit();
}

// Internally used only.
void throw_bdb_exception(const char *caller, int error)
{
     switch (error) {
     case DB_LOCK_DEADLOCK:
         {
             DbDeadlockException dl_except(caller);
             throw dl_except;
         }
     case DB_LOCK_NOTGRANTED:
         {
             DbLockNotGrantedException lng_except(caller);
             throw lng_except;
         }
     case DB_REP_HANDLE_DEAD:
         {
             DbRepHandleDeadException hd_except(caller);
             throw hd_except;
         }
     case DB_RUNRECOVERY:
         {
             DbRunRecoveryException rr_except(caller);
             throw rr_except;
         }
     default:
         {
             DbException except(caller, error);
             throw except;
         }
     }
}

void register_global_object(DbstlGlobalInnerObject *gio)
{
    ResourceManager::instance()->register_global_object(gio);
}

u_int32_t hash_default(Db * /* dbp */, const void *key, u_int32_t len)
{
    const u_int8_t *k, *e;
    u_int32_t h;

    k = (const u_int8_t *)key;
    e = k + len;
    for (h = 0; k < e; ++k) {
        h *= 16777619;
        h ^= *k;
    }
    return (h);
}

bool DbstlMultipleDataIterator::next(Dbt &data)
{
    if (*p_ == (u_int32_t)-1) {
        data.set_data(0);
        data.set_size(0);
        p_ = 0;
    } else {
        data.set_data(data_ + *p_--);
        data.set_size(*p_--);
        if (data.get_size() == 0 && data.get_data() == data_)
            data.set_data(0);
    }
    return (p_ != 0);
}

bool DbstlMultipleKeyDataIterator::next(Dbt &key, Dbt &data)
{
    if (*p_ == (u_int32_t)-1) {
        key.set_data(0);
        key.set_size(0);
        data.set_data(0);
        data.set_size(0);
        p_ = 0;
    } else {
        key.set_data(data_ + *p_);
        p_--;
        key.set_size(*p_);
        p_--;
        data.set_data(data_ + *p_);
        p_--;
        data.set_size(*p_);
        p_--;
    }
    return (p_ != 0);
}

bool DbstlMultipleRecnoDataIterator::next(db_recno_t &recno, Dbt &data)
{
    if (*p_ == (u_int32_t)0) {
        recno = 0;
        data.set_data(0);
        data.set_size(0);
        p_ = 0;
    } else {
        recno = *p_--;
        data.set_data(data_ + *p_--);
        data.set_size(*p_--);
    }
    return (p_ != 0);
}

END_NS

