/*-
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * http://www.apache.org/licenses/LICENSE-2.0.txt
 * 
 * authors: George Schlossnagle <george@omniti.com>
 */

extern "C"
{
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"          /* For REMOTE_NAME */
#include "http_log.h"

#include "sem_utils.h"
#include "skiplist.h"
#include "mm_hash.h"
}

#include "utils.h"
#include "db_cxx.h"

/* the semaphore set for the application */
static int semset;

/* process-local handle for global ref count management */
/* individual semaphores */
#define OK_TO_PROCEED 0
#define GLOBAL_LOCK   1
#define NUM_SEMS      2

/* mm helpers */
static MM *mm;
static MM_Hash *ref_counts;

/* locks */
int env_locks_init()
{
    char shmpath[32];
    unsigned short start[2] = { 1, 1 };

    snprintf(shmpath, 32, "/tmp/.mod_db4.%d", getpid());
    mm = mm_create(0, shmpath);
    if(NULL == mm) {
        return -1;
    }    
    mm_lock(mm, MM_LOCK_RW);
    ref_counts = mm_hash_new(mm, NULL);
    mm_unlock(mm);
    if (!ref_counts) {
        return -1;
    }
    if((semset = md4_sem_create(NUM_SEMS, start)) < 0) {
        return -1;
    }
    return 0;
}

void env_global_rw_lock()
{
    mm_lock(mm, MM_LOCK_RW);
}

void env_global_rd_lock()
{
    mm_lock(mm, MM_LOCK_RD);
}

void env_global_unlock()
{
    mm_unlock(mm);
}

void env_wait_for_child_crash()
{
    md4_sem_wait_for_zero(semset, OK_TO_PROCEED);
}

void env_child_crash()
{
    md4_sem_set(semset, OK_TO_PROCEED, 0);
}

void env_ok_to_proceed()
{
    md4_sem_set(semset, OK_TO_PROCEED, 1);
}

/* process resource globals */
static Skiplist open_transactions;
static Skiplist open_cursors;
static Skiplist open_log_cursors;
static Skiplist open_dbs;
static Skiplist open_dbenvs;

/* named pointers for db_env and bd skiplists */
struct named_resource {
  char *name;
  void *ptr;
};

/* skiplist comparitors for void pointers */

static int VP_COMPARE(void *a, void *b)
{
    return (a < b) ? (-1) : ((a == b) ? (0) : (1));
}

/* key for comparing DB *'s in skiplist */

struct db_key {
    const char *fname;
    const char *dname;
};

static int DB_COMPARE(void *a, void *b)
{
    int ret;
    DB *ae = (DB *) a;
    DB *be = (DB *) b;
    if(ae->fname == NULL) {
        if(be->fname == NULL) {
            return (ae < be) ? (-1) : ((ae == be) ? (0) : (1));
        }
        return 1;
    }
    else if(be->fname == NULL) {
        /* ae->fname != NULL, from above */
        return -1;
    }
    ret = strcmp(ae->fname, be->fname);
    if(ret == 0) {
        if(ae->dname == NULL) {
            if(be->dname == NULL) {
              return 0;
            }
            return 1;
        }
        else if(be->dname == NULL) {
            return -1;
        }
        ret = strcmp(ae->dname, be->dname);
    }
    return ret;
}

static int DB_COMPARE_K(void *a, void *b)
{
    struct db_key *akey = (struct db_key *) a;
    DB *be = (DB *) b;
    int ret;
    if(akey->fname == NULL) {
        if(be->fname == NULL) {
            /* should never match here */
            return (a < b) ? (-1) : ((a == b) ? (0) : (1));
        }
        return 1;
    }
    else if(be->fname == NULL) {
        /* akey->fname != NULL, from above */
        return -1;
    }
    ret = strcmp(akey->fname, be->fname);
    if(ret == 0) {
        if(akey->dname == NULL) {
            if(be->dname == NULL) {
              return 0;
            }
            return 1;
        }
        else if(be->dname == NULL) {
            return -1;
        }
        ret = strcmp(akey->dname, be->dname);
    }
    return ret;
}

static int DBENV_COMPARE(void *a, void *b)
{
    DB_ENV *ae = (DB_ENV *) a;
    DB_ENV *be = (DB_ENV *) b;
    return strcmp(ae->db_home, be->db_home);
}

static int DBENV_COMPARE_K(void *a, void *b)
{
    const char *aname = (const char *) a;
    DB_ENV *be = (DB_ENV *) b;
    return strcmp(aname, be->db_home);
}

void env_rsrc_list_init()
{
    skiplist_init(&open_transactions);
    skiplist_set_compare(&open_transactions, VP_COMPARE, VP_COMPARE);

    skiplist_init(&open_cursors);
    skiplist_set_compare(&open_cursors, VP_COMPARE, VP_COMPARE);
    
    skiplist_init(&open_log_cursors);
    skiplist_set_compare(&open_log_cursors, VP_COMPARE, VP_COMPARE);

    skiplist_init(&open_dbs);
    skiplist_set_compare(&open_dbs, DB_COMPARE, DB_COMPARE_K);
    
    skiplist_init(&open_dbenvs);
    skiplist_set_compare(&open_dbenvs, DBENV_COMPARE, DBENV_COMPARE_K);
}

static void register_cursor(DBC *dbc)
{
    skiplist_insert(&open_cursors, dbc);
}

static void unregister_cursor(DBC *dbc)
{
    skiplist_remove(&open_cursors, dbc, NULL);
}

static void register_log_cursor(DB_LOGC *cursor)
{
    skiplist_insert(&open_log_cursors, cursor);
}

static void unregister_log_cursor(DB_LOGC *cursor)
{
    skiplist_remove(&open_log_cursors, cursor, NULL);
}

static void register_transaction(DB_TXN *txn)
{
    skiplist_insert(&open_transactions, txn);
}

static void unregister_transaction(DB_TXN *txn)
{
    skiplist_remove(&open_transactions, txn, NULL);
}

static DB *retrieve_db(const char *fname, const char *dname)
{
    DB *rv;
    struct db_key key;
    if(fname == NULL) {
        return NULL;
    }
    key.fname = fname;
    key.dname = dname;
    rv = (DB *) skiplist_find(&open_dbs, (void *) &key, NULL);
    return rv;
}

static void register_db(DB *db)
{
    skiplist_insert(&open_dbs, db);
}

static void unregister_db(DB *db)
{
    struct db_key key;
    key.fname = db->fname;
    key.dname = db->dname;
    skiplist_remove(&open_dbs, &key, NULL);
}

static DB_ENV *retrieve_db_env(const char *db_home)
{
  return (DB_ENV *) skiplist_find(&open_dbenvs, (void *) db_home, NULL);
}

static void register_db_env(DB_ENV *dbenv)
{
    global_ref_count_increase(dbenv->db_home);
    skiplist_insert(&open_dbenvs, dbenv);
}

static void unregister_db_env(DB_ENV *dbenv)
{
    global_ref_count_decrease(dbenv->db_home);
    skiplist_remove(&open_dbenvs, dbenv->db_home, NULL);
}

int global_ref_count_increase(char *path)
{
    int refcount = 0;
    int pathlen = 0;
    pathlen = strlen(path);

    env_global_rw_lock();
    refcount = (int) mm_hash_find(ref_counts, path, pathlen);
    refcount++;
    mm_hash_update(ref_counts, path, pathlen, (void *)refcount);
    env_global_unlock();
    return refcount;
}

int global_ref_count_decrease(char *path)
{
    int refcount = 0;
    int pathlen = 0;
    pathlen = strlen(path);

    env_global_rw_lock();
    refcount = (int) mm_hash_find(ref_counts, path, pathlen);
    if(refcount > 0) refcount--;
    mm_hash_update(ref_counts, path, pathlen, (void *)refcount);
    env_global_unlock();
    return refcount;
}

int global_ref_count_get(const char *path)
{
    int refcount = 0;
    int pathlen = 0;
    pathlen = strlen(path);

    env_global_rd_lock();
    refcount = (int) mm_hash_find(ref_counts, path, pathlen);
    env_global_unlock();
    return refcount;
}

void global_ref_count_clean()
{
    env_global_rd_lock();
    mm_hash_free(ref_counts);
    ref_counts = mm_hash_new(mm, NULL);
    env_global_unlock();
}

/* wrapper methods  {{{ */

static int (*old_log_cursor_close)(DB_LOGC *, u_int32_t) = NULL;
static int new_log_cursor_close(DB_LOGC *cursor, u_int32_t flags)
{
    unregister_log_cursor(cursor);
    return old_log_cursor_close(cursor, flags);
}

static int (*old_db_txn_abort)(DB_TXN *) = NULL;
static int new_db_txn_abort(DB_TXN *tid)
{
    unregister_transaction(tid);
    return old_db_txn_abort(tid);
}

static int (*old_db_txn_commit)(DB_TXN *, u_int32_t) = NULL;
static int new_db_txn_commit(DB_TXN *tid, u_int32_t flags)
{
    unregister_transaction(tid);
    return old_db_txn_commit(tid, flags);
}

static int (*old_db_txn_discard)(DB_TXN *, u_int32_t) = NULL;
static int new_db_txn_discard(DB_TXN *tid, u_int32_t flags)
{
    unregister_transaction(tid);
    return old_db_txn_discard(tid, flags);
}

static int (*old_db_env_txn_begin)(DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t);
static int new_db_env_txn_begin(DB_ENV *env, DB_TXN *parent, DB_TXN **tid, u_int32_t flags)
{
    int ret;
    if((ret = old_db_env_txn_begin(env, parent, tid, flags)) == 0) {
        register_transaction(*tid);
        /* overload DB_TXN->abort */
        if(old_db_txn_abort == NULL) {
            old_db_txn_abort = (*tid)->abort;
        }
        (*tid)->abort = new_db_txn_abort;

        /* overload DB_TXN->commit */
        if(old_db_txn_commit == NULL) {
            old_db_txn_commit = (*tid)->commit;
        }
        (*tid)->commit = new_db_txn_commit;

        /* overload DB_TXN->discard */
        if(old_db_txn_discard == NULL) {
            old_db_txn_discard = (*tid)->discard;
        }
        (*tid)->discard = new_db_txn_discard;
    }
    return ret;
}

static int (*old_db_env_open)(DB_ENV *, const char *, u_int32_t, int) = NULL;
static int new_db_env_open(DB_ENV *dbenv, const char *db_home, u_int32_t flags, int mode)
{
    int ret =666;
    DB_ENV *cached_dbenv;
    flags |= DB_INIT_MPOOL;
    /* if global ref count is 0, open for recovery */
    if(global_ref_count_get(db_home) == 0) {
        flags |= DB_RECOVER;
        flags |= DB_INIT_TXN;
        flags |= DB_CREATE;
    }
    if((cached_dbenv = retrieve_db_env(db_home)) != NULL) {
        memcpy(dbenv, cached_dbenv, sizeof(DB_ENV));
        ret = 0;
    }
    else if((ret = old_db_env_open(dbenv, db_home, flags, mode)) == 0) {
        register_db_env(dbenv);
    }
    return ret;
}

static int(*old_db_env_close)(DB_ENV *, u_int32_t) = NULL;
static int new_db_env_close(DB_ENV *dbenv, u_int32_t flags)
{
    int ret;
    /* we're already locked */
    unregister_db_env(dbenv);
    ret = old_db_env_close(dbenv, flags);
}

static int (*old_db_env_log_cursor)(DB_ENV *, DB_LOGC **, u_int32_t) = NULL;
static int new_db_env_log_cursor(DB_ENV *dbenv, DB_LOGC **cursop, u_int32_t flags)
{
    int ret;
    if((ret = old_db_env_log_cursor(dbenv, cursop, flags)) == 0) {
        register_log_cursor(*cursop);
        if(old_log_cursor_close == NULL) {
            old_log_cursor_close = (*cursop)->close;
        }
        (*cursop)->close = new_log_cursor_close;
    }
    return ret;
}

static int (*old_db_open)(DB *, DB_TXN *, const char *, const char *, DBTYPE, u_int32_t, int) = NULL;
static int new_db_open(DB *db, DB_TXN *txnid, const char *file,
                const char *database, DBTYPE type, u_int32_t flags, int mode)
{
    int ret;
    DB *cached_db;

    cached_db = retrieve_db(file, database);
    if(cached_db) {
        memcpy(db, cached_db, sizeof(DB));
        ret = 0;
    }
    else if((ret = old_db_open(db, txnid, file, database, type, flags, mode)) == 0) {
        register_db(db);
    } 
    return ret;
}

static int (*old_db_close)(DB *, u_int32_t) = NULL;
static int new_db_close(DB *db, u_int32_t flags)
{
    unregister_db(db);
    return old_db_close(db, flags);
}


static int (*old_dbc_close)(DBC *);
static int new_dbc_close(DBC *cursor)
{
    unregister_cursor(cursor);
    return old_dbc_close(cursor);
}

static int (*old_dbc_dup)(DBC *, DBC **, u_int32_t) = NULL;
static int new_dbc_dup(DBC *oldcursor, DBC **newcursor, u_int32_t flags)
{
    int ret;
    if((ret = old_dbc_dup(oldcursor, newcursor, flags)) == 0) {
        register_cursor(*newcursor);
        
        /* overload DBC->close */
        (*newcursor)->close = oldcursor->close;
        
        /* overload DBC->dup */
        (*newcursor)->dup = oldcursor->dup;
    }
    return ret;
}

static int (*old_db_cursor)(DB *, DB_TXN *, DBC **, u_int32_t) = NULL;
static int new_db_cursor(DB *db, DB_TXN *txnid, DBC **cursop, u_int32_t flags)
{
    int ret;
    if((ret = old_db_cursor(db, txnid, cursop, flags)) == 0) {
        register_cursor(*cursop);
        
        /* overload DBC->close */
        if(old_dbc_close == NULL) { 
            old_dbc_close = (*cursop)->close;
        }
        (*cursop)->close = new_dbc_close;
        
        /* overload DBC->dup */
        if(old_dbc_dup == NULL) { 
            old_dbc_dup = (*cursop)->dup;
        }
        (*cursop)->dup = new_dbc_dup;
    }
    return ret;
}

/* }}} */

/* {{{ new DB_ENV constructor
 */

int mod_db4_db_env_create(DB_ENV **dbenvp, u_int32_t flags)
{
    int cachesize = 0;
    int ret;
    DB_ENV *dbenv;

    if ((ret = db_env_create(dbenvp, 0)) != 0) {
        /* FIXME report error */

        return ret;
    }
    dbenv = *dbenvp;
    DbEnv::wrap_DB_ENV(dbenv);
    /* Here we set defaults settings for the db_env */
    /* grab context info from httpd.conf for error file */
    /* grab context info for cachesize */
    if (0 && cachesize) {
        if ((ret = dbenv->set_cachesize(dbenv, 0, cachesize, 0)) != 0) {
            dbenv->err(dbenv, ret, "set_cachesize");
            dbenv->close(dbenv, 0);
        }
    }
    /* overload DB_ENV->open */
    if(old_db_env_open == NULL) {
      old_db_env_open = dbenv->open;
    }
    dbenv->open = new_db_env_open;

    /* overload DB_ENV->close */
    if(old_db_env_close == NULL) {
      old_db_env_close = dbenv->close;
    }
    dbenv->close = new_db_env_close;

    /* overload DB_ENV->log_cursor */
    if(old_db_env_log_cursor == NULL) {
      old_db_env_log_cursor = dbenv->log_cursor;
    }
    dbenv->log_cursor = new_db_env_log_cursor;

    /* overload DB_ENV->txn_begin */
    if(old_db_env_txn_begin == NULL) {
      old_db_env_txn_begin = dbenv->txn_begin;
    }
    dbenv->txn_begin = new_db_env_txn_begin;
    return 0;
}
/* }}} */

/* {{{ new DB constructor
 */
int mod_db4_db_create(DB **dbp, DB_ENV *dbenv, u_int32_t flags)
{
    int ret;

flags = 0;

    if((ret = db_create(dbp, dbenv, flags)) == 0) {
        /* FIXME this should be removed I think register_db(*dbp); */
        /* overload DB->open */
        if(old_db_open == NULL) {
            old_db_open = (*dbp)->open;
        }
        (*dbp)->open = new_db_open;

        /* overload DB->close */
        if(old_db_close == NULL) {
            old_db_close = (*dbp)->close;
        }
        (*dbp)->close = new_db_close;

        /* overload DB->cursor */
        if(old_db_cursor == NULL) {
            old_db_cursor = (*dbp)->cursor;
        }
        (*dbp)->cursor = new_db_cursor;
    }
    return ret;
}

/* }}} */

void mod_db4_child_clean_request_shutdown()
{
    DBC *cursor;
    DB_TXN *transaction;
    while(cursor = (DBC *)skiplist_pop(&open_cursors, NULL)) {
        cursor->close(cursor);
    }
    while(transaction = (DB_TXN *)skiplist_pop(&open_transactions, NULL)) {
        transaction->abort(transaction);
    }
}

void mod_db4_child_clean_process_shutdown()
{
    DB *db;
    DB_ENV *dbenv;
    mod_db4_child_clean_request_shutdown();
    while(db = (DB *)skiplist_pop(&open_dbs, NULL)) {
        db->close(db, 0);
    }
    while(dbenv = (DB_ENV *)skiplist_pop(&open_dbenvs, NULL)) {
        DbEnv *dbe = DbEnv::get_DbEnv(dbenv);
        global_ref_count_decrease(dbenv->db_home);
        dbe->close(0);
        delete dbe;
    }
}
/* vim: set ts=4 sts=4 expandtab bs=2 ai fdm=marker: */
