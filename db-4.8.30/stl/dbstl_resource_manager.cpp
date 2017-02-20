/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <assert.h>
#include <utility>

#include "dbstl_resource_manager.h"
#include "dbstl_exception.h"
#include "dbstl_dbc.h"

START_NS(dbstl)

typedef struct {
    time_t  tv_sec;             /* seconds */
    long    tv_nsec;            /* nanoseconds */
} db_timespec;

extern "C"{
void __os_id (DB_ENV *, pid_t *, db_threadid_t*);
void __os_gettime(ENV *env, db_timespec *tp, int monotonic);
}

using std::pair;
using std::make_pair;

// Static data member definitions.
map<Db*, size_t> ResourceManager::open_dbs_;
map<DbEnv*, size_t> ResourceManager::open_envs_;
set<DbstlGlobalInnerObject *> ResourceManager::glob_objs_;
set<Db *> ResourceManager::deldbs; 
set<DbEnv *> ResourceManager::delenvs; 

DbEnv * ResourceManager::mtx_env_ = NULL;
db_mutex_t ResourceManager::mtx_handle_ = 0;
db_mutex_t ResourceManager::mtx_globj_ = 0;

#ifdef TLS_DEFN_MODIFIER
template <Typename T> TLS_DEFN_MODIFIER T *TlsWrapper<T>::tinst_ = NULL;
#elif defined(HAVE_PTHREAD_TLS)
static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
template <Typename T>
pthread_key_t TlsWrapper<T>::tls_key_;

template<Typename T>
void tls_init_once(void) {
    pthread_key_create(&TlsWrapper<T>::tls_key_, NULL);
}

template<Typename T>
TlsWrapper<T>::TlsWrapper()
{
    pthread_once(&once_control_, tls_init_once<T>);
}
#else
#error "No suitable thread-local storage model configured"
#endif

int ResourceManager::global_lock(db_mutex_t dbcontainer_mtx)
{
    int ret;

    ret = mtx_env_->mutex_lock(dbcontainer_mtx);
    dbstl_assert(ret == 0);
    return 0;
}

int ResourceManager::global_unlock(db_mutex_t dbcontainer_mtx)
{
    int ret;

    ret = mtx_env_->mutex_unlock(dbcontainer_mtx);
    dbstl_assert(ret == 0);
    return 0;
}

u_int32_t dbstl_strlen(const char *str)
{
    return (u_int32_t)strlen(str);
}

void dbstl_strcpy(char *dest, const char *src, size_t num)
{
    strncpy(dest, src, num);
}

int dbstl_strncmp(const char *s1, const char *s2, size_t num)
{
    return strncmp(s1, s2, num);
}

int dbstl_wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t num)
{
    return wcsncmp(s1, s2, num);
}

int dbstl_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

int dbstl_wcscmp(const wchar_t *s1, const wchar_t *s2)
{
    return wcscmp(s1, s2);
}

u_int32_t dbstl_wcslen(const wchar_t *str)
{
    return (u_int32_t)wcslen(str);
}

void dbstl_wcscpy(wchar_t *dest, const wchar_t *src, size_t num)
{
    wcsncpy(dest, src, num);
}

// This function should be called in a single thread inside a process, before
// any use of dbstl. We don't want to rely on platform dependent mutex API,
// so we defer the synchronization to users.
void ResourceManager::global_startup()
{
    int ret;
    db_timespec tnow;

    if (mtx_env_ == NULL) {
        mtx_env_ = new DbEnv(DB_CXX_NO_EXCEPTIONS);
        // Set cache size to 32k, to save space.
        BDBOP(mtx_env_->set_cachesize(0, 32 * 1024, 1), ret);
        BDBOP(mtx_env_->mutex_set_max(DBSTL_MAX_MTX_ENV_MUTEX), ret);
        BDBOP2(mtx_env_->open(NULL, DB_PRIVATE | DB_CREATE, 0777),
            ret, mtx_env_->close(0));
        BDBOP2(mtx_env_->mutex_alloc(DB_MUTEX_PROCESS_ONLY,
            &mtx_handle_), ret, mtx_env_->mutex_free(mtx_handle_));
        BDBOP2(mtx_env_->mutex_alloc(DB_MUTEX_PROCESS_ONLY,
            &mtx_globj_), ret, mtx_env_->mutex_free(mtx_globj_));
        __os_gettime(NULL, &tnow, 0);
        srand((unsigned int)tnow.tv_sec);
    }

}

ResourceManager::ResourceManager(void)
{

    // Initialize process wide dbstl settings. If there are multiple 
    // threads, the global_startup should be called in a single thread 
    // before any use of dbstl.
    global_startup();
}

void ResourceManager::close_db(Db *pdb)
{
    bool berase = false;

    if (pdb == NULL)
        return;
    db_csr_map_t::iterator itr = all_csrs_.find(pdb);
    if (itr == all_csrs_.end())
        return;

    this->close_db_cursors(pdb);

    delete all_csrs_[pdb];
    all_csrs_.erase(itr);
    pdb->close(0);
    set<Db *>::iterator itrdb = deldbs.find(pdb);
    // If new'ed by open_db, delete it.
    if (itrdb != deldbs.end()) {
        delete *itrdb;
        berase = true;
    }

    global_lock(mtx_handle_);
    open_dbs_.erase(pdb);
    if (berase)
        deldbs.erase(itrdb);

    global_unlock(mtx_handle_);
}

void ResourceManager::close_all_db_envs()
{
    u_int32_t oflags;
    int ret;
    size_t txnstk_sz;

    global_lock(mtx_handle_);
    for (map<DbEnv*, size_t>::iterator i = open_envs_.begin();
        i != open_envs_.end(); ++i) {
        BDBOP(i->first->get_open_flags(&oflags), ret);
        txnstk_sz = env_txns_[i->first].size();
        if (oflags & DB_INIT_CDB) {
            assert(txnstk_sz == 1);
            BDBOP(env_txns_[i->first].top()->commit(0), ret);
        } else
            assert(txnstk_sz == 0);

        i->first->close(0);
    }

    // Delete DbEnv objects new'ed by dbstl.
    set<DbEnv *>::iterator itr2 = delenvs.begin(); 
    for (; itr2 != delenvs.end(); ++itr2)
        delete *itr2;

    delenvs.clear();
    env_txns_.clear();
    open_envs_.clear();
    global_unlock(mtx_handle_);
}

void ResourceManager::close_db_env(DbEnv *penv)
{
    u_int32_t oflags;
    int ret;
    size_t txnstk_sz;
    bool berase = false;

    if (penv == NULL)
        return;
    map<DbEnv *, txnstk_t>::iterator itr = env_txns_.find(penv);
    if (itr == env_txns_.end())
        return;
    BDBOP(penv->get_open_flags(&oflags), ret);
    txnstk_sz = itr->second.size();
    if (oflags & DB_INIT_CDB) {
        assert(txnstk_sz == 1);
        BDBOP(itr->second.top()->commit(0), ret);
    } else
        assert(txnstk_sz == 0);
    env_txns_.erase(itr);
    penv->close(0);

    set<DbEnv *>::iterator itrdb = delenvs.find(penv);
    // If new'ed by open_db, delete it.
    if (itrdb != delenvs.end()) {
        delete penv;
        berase = true;
    }

    global_lock(mtx_handle_);
    open_envs_.erase(penv);
    if (berase)
        delenvs.erase(itrdb);
    global_unlock(mtx_handle_);
}

void ResourceManager::close_all_dbs()
{
    map<Db *, size_t>::iterator itr;
    set<Db *>::iterator itr2;
    Db *pdb;

    global_lock(mtx_handle_);
    for (itr = open_dbs_.begin(); itr != open_dbs_.end(); ++itr) {
        pdb = itr->first;
        this->close_db_cursors(pdb);

        delete all_csrs_[pdb];
        all_csrs_.erase(pdb);
        pdb->close(0);
    }

    // Delete Db objects new'ed by dbstl.
    for (itr2 = deldbs.begin(); itr2 != deldbs.end(); ++itr2)
        delete *itr2;

    deldbs.clear();
    open_dbs_.clear();

    global_unlock(mtx_handle_);
}

ResourceManager::~ResourceManager(void)
{
    u_int32_t oflags;
    int ret;

    global_lock(mtx_handle_);

    for (map<Db*, size_t>::iterator i = open_dbs_.begin();
        i != open_dbs_.end(); ++i) {
        this->close_db_cursors(i->first);
        (i->second)--;
        if (i->second == 0) {

            delete all_csrs_[i->first]; // Delete the cursor set.
            all_csrs_.erase(i->first);
            i->first->close(0);

            set<Db *>::iterator itrdb = deldbs.find(i->first);
            // If new'ed by open_db, delete it.
            if (itrdb != deldbs.end()) {
                delete *itrdb;
                deldbs.erase(itrdb);
            }
        }
    }

    for (map<DbEnv*, size_t>::iterator i = open_envs_.begin();
        i != open_envs_.end(); ++i) {
        BDBOP(i->first->get_open_flags(&oflags), ret);
        if (oflags & DB_INIT_CDB) {
            assert(env_txns_[i->first].size() == 1);
            BDBOP(env_txns_[i->first].top()->commit(0), ret);
            env_txns_[i->first].pop();
        }

        (i->second)--;
        if (i->second == 0) {
            assert(env_txns_[i->first].size() == 0);
            i->first->close(0);
            set<DbEnv *>::iterator itrdb = delenvs.find(i->first);
            // If new'ed by open_db, delete it.
            if (itrdb != delenvs.end()) {
                delete *itrdb;
                delenvs.erase(itrdb);
            }
        }
    }

    global_unlock(mtx_handle_);

    for (db_csr_map_t::iterator itr3 = all_csrs_.begin(); 
        itr3 != all_csrs_.end(); ++itr3)
    {
        // Delete the cursor set. Above code may not have a chance to
        // delete this set because the db(itr3->first) was already 
        // closed by another thread.
        delete itr3->second; 
    }
    // Don't bother to clear all_csrs_ since it is being destructed.

// Don't handle transactions, leave them alone, because autocommit
// transactions must have been committed/aborted, and outside transactions
// should be handled by user code, and if they are not handled yet,
// the DbEnv::close will fail.
// Only handle the transaction for CDS mode---an DB_TXN* handle is opened
// at environment registration/creation by cdsgroup_begin, so we need to commit
// that transaction.
//
}

Db* ResourceManager::open_db (
    DbEnv*penv, const char* filename, DBTYPE dbtype,
    u_int32_t oflags, u_int32_t set_flags1, int mode, DbTxn* txn,
    u_int32_t cflags, const char* dbname)
{
    int ret, ci = 0;
    u_int32_t envf = 0, envoflags = 0;
    DbTxn *ptxn = NULL;
    Db *pdb = new Db(penv, cflags | DB_CXX_NO_EXCEPTIONS);

    if (penv) {
        BDBOP(penv->get_open_flags(&envoflags), ret);
        BDBOP(penv->get_flags(&envf), ret);
    }
    if (set_flags1)
        BDBOP(pdb->set_flags(set_flags1), ret);
    // If no transaction is specified and we really need one, begin a
    // transaction and commit it before return, we don't commit
    // passed-in transaction.
    //
    if (penv && ((envf & DB_AUTO_COMMIT) ||
        (envoflags & DB_INIT_TXN)) && txn == 0){
        ptxn = current_txn(penv);
        BDBOP2(penv->txn_begin(ptxn, &txn, 0), ret, txn->abort());
        ci = 1;
    }
    if (txn == NULL)
        BDBOP2(pdb->open(txn, filename, dbname,
            dbtype, oflags, mode),
            ret, (pdb->close(0)));
    else
        BDBOP2(pdb->open(txn, filename, dbname,
            dbtype, oflags, mode),
            ret, (pdb->close(0), txn->abort()));
    if (ci && txn)
        BDBOP(txn->commit(0), ret);
    global_lock(mtx_handle_);
    open_dbs_.insert(make_pair(pdb, 1u));
    pair<set<Db *>::iterator, bool> delinsret = deldbs.insert(pdb);
    assert(delinsret.second);
    global_unlock(mtx_handle_);
    csrset_t *mycsrs = new csrset_t();
    all_csrs_.insert(make_pair(pdb, mycsrs));

    return pdb;
}

// Only called if the user does not supply an environment handle.
DbEnv* ResourceManager::open_env(const char* env_home, u_int32_t set_flags1,
    u_int32_t oflags, u_int32_t cachesize, int mode, u_int32_t cflags)
{
    int ret;

    DbEnv *penv = new DbEnv(cflags | DB_CXX_NO_EXCEPTIONS);
    if (set_flags1)
        BDBOP(penv->set_flags(set_flags1, 1), ret);
    BDBOP(penv->set_cachesize(0, cachesize, 1), ret);
    BDBOP(penv->set_lk_max_lockers(2000), ret);
    BDBOP(penv->set_lk_max_locks(2000), ret);
    BDBOP(penv->set_lk_max_objects(2000), ret);
    BDBOP2(penv->open(env_home, oflags, mode), ret, penv->close(0));

    stack<DbTxn*> stk;
    DbTxn *ptxn = NULL;
    if (oflags & DB_INIT_CDB) {
        BDBOP2(penv->cdsgroup_begin(&ptxn), ret, ptxn->commit(0));
        stk.push(ptxn);
    }

    env_txns_.insert(make_pair(penv, stk));
    global_lock(mtx_handle_);
    open_envs_.insert(make_pair(penv, 1u));
    delenvs.insert(penv);
    global_unlock(mtx_handle_);
    return penv;
}

DbTxn* ResourceManager::current_txn(DbEnv*env)
{
    if (env_txns_.count(env) <= 0)
        return NULL;

    stack<DbTxn*> &pstk = env_txns_[env];
    return pstk.size() != 0 ? pstk.top() : NULL;
}

void ResourceManager::set_global_callbacks()
{
    DbstlElemTraits<char> * cstarinst = 
        DbstlElemTraits<char>::instance();
    cstarinst->set_sequence_len_function(dbstl_strlen);
    cstarinst->set_sequence_copy_function(dbstl_strcpy);
    cstarinst->set_sequence_compare_function(dbstl_strcmp);
    cstarinst->set_sequence_n_compare_function(dbstl_strncmp);

    DbstlElemTraits<wchar_t> *wcstarinst = 
        DbstlElemTraits<wchar_t>::instance();
    wcstarinst->set_sequence_copy_function(dbstl_wcscpy);
    wcstarinst->set_sequence_len_function(dbstl_wcslen);
    wcstarinst->set_sequence_compare_function(dbstl_wcscmp);
    wcstarinst->set_sequence_n_compare_function(dbstl_wcsncmp);
}

ResourceManager* ResourceManager::instance()
{
    ResourceManager *pinst = NULL;
#ifdef HAVE_PTHREAD_TLS
    // Initialize the tls key.
    pthread_once(&once_control_, tls_init_once<ResourceManager>);
#endif

    if ((pinst = TlsWrapper<ResourceManager>::get_tls_obj()) == NULL){
        TlsWrapper<ResourceManager>::set_tls_obj(
            pinst = new ResourceManager());
        register_global_object(pinst);
        set_global_callbacks();
    }
    return pinst;
}

int ResourceManager::open_cursor(DbCursorBase *dcbcsr,
    Db *pdb, int flags)
{
    u_int32_t oflags = 0;
    int ret;

    if (!pdb || !dcbcsr)
        return 0;

    csrset_t::iterator csitr;
    Dbc* csr = NULL;
    dcbcsr->set_owner_db(pdb);

    DbTxn *ptxn = NULL;
    DbTxn *ptxn2 = this->current_txn(pdb->get_env());
    if (ptxn2) {
        ptxn = ptxn2;
        dcbcsr->set_owner_txn(ptxn);
    }

    if (pdb->get_env() != NULL){
        ret = pdb->get_env()->get_open_flags(&oflags);
        dbstl_assert(ret == 0);
    }

    // Call Dbc->cursor only if there is no active open cursor in the
    // current thread, otherwise duplicate one from the existing cursor
    // and use the locks already held in this thread.
    //
    csrset_t *pcsrset = NULL;
    db_csr_map_t::iterator itrpcsrset = all_csrs_.find(pdb);
    if (itrpcsrset == all_csrs_.end()) { // No such pair in current thread.
        pcsrset = new csrset_t;
        pair<db_csr_map_t::iterator, bool> insret0 = 
            all_csrs_.insert(make_pair(pdb, pcsrset));
        assert(insret0.second);
    } else
        pcsrset = itrpcsrset->second;

    assert(pcsrset != NULL);
    if (pcsrset->size() == 0) {
newcursor:
        BDBOP2(pdb->cursor(ptxn, &csr, flags), ret,
            ((csr != NULL ? csr->close() : 1),
            this->abort_txn(pdb->get_env())));
    } else {
        // We have some open cursors, so try to dup from one. If we are
        // in CDS mode, and trying to open a write cursor, we should
        // duplicate from a write cursor.
        csitr = pcsrset->begin();
        Dbc *csr22 = (*csitr)->get_cursor();
        assert(csr22 != NULL);
        assert(!((oflags & DB_INIT_TXN) && (flags & DB_WRITECURSOR)));
        // If opening a CDS write cursor, must find a write cursor
        // to duplicate from.
        if (((flags & DB_WRITECURSOR) != 0)) {
            for (;csitr != pcsrset->end(); ++csitr) {
                csr22 = (*csitr)->get_cursor();
                if (((DBC*)csr22)->flags & DBC_WRITECURSOR) {
                    // No need to abortTxn on fail in CDS.
                    BDBOP2(csr22->dup(&csr, DB_POSITION),
                         ret, csr->close());
                    goto done;
                }
            }
            goto newcursor; // No write cursor, create a new one.

        } else if (((oflags & DB_INIT_TXN) == 0) || 
            pdb->get_transactional() == 0) {
            // We are opening a DS or CDS read cursor, or 
            // opening a cursor in 
            // a transactional environment from a database not 
            // transactionally created.
            BDBOP2(csr22->dup(&csr, DB_POSITION), ret,
                (csr->close(), this->abort_txn(pdb->get_env())));
            goto done;
        } else  {
            // We are opening a transactional cursor, duplicate
            // from a transactional one.
            // We don't remove (close) the non-transactional ones,
            // they are in use.
            // Hold the locks already held in this thread,
            // so need DB_POSITION flag.
            //
            DbTxn *ptxn3 = NULL;
            DbCursorBase *dcbcursor = NULL;
            csrset_t::iterator itr3, itr4;
            int got_rg = 0;

            // Opening a cursor in a transactional environment
            // with no transaction specified. This should not
            // happen in the first place.
            if (ptxn == NULL)
                THROW(InvalidArgumentException, ("DbTxn*",
"Opening a cursor in a transactional environment but no transaction \
is started specified"));
            // When we check that there must be a valid transaction
            // handle ptxn when opening a cursor in a
            // transactional environment, the following code
            // to delete cursors with no transaction
            // is not required and never reached,
            // but we will leave it there.
            for (;csitr != pcsrset->end();) {
                dcbcursor = *csitr;
                ptxn3 = dcbcursor->get_owner_txn();
                if (ptxn3 == NULL) {
                    BDBOP(dcbcursor->close(), ret);
                    if (!got_rg){
                        got_rg++;
                        itr3 = csitr;
                    }
                } else if (got_rg) {
                    got_rg = 0;
                    itr4 = csitr;
                    pcsrset->erase(itr3, itr4);
                    csitr = pcsrset->begin();
                    continue;
                }

                if (ptxn3 == ptxn)  {
                    csr22 = dcbcursor->get_cursor();
                    BDBOP2(csr22->dup(&csr, DB_POSITION),
                        ret, (csr->close(), this->
                        abort_txn(pdb->get_env())));
                    goto done;
                }
                ++csitr;

            }
            if (got_rg) {
                pcsrset->erase(itr3, pcsrset->end());
                got_rg = 0;
            }

            goto newcursor;

        } // else oflags & DB_INIT_TXN
    } // else pcsrset->size()
done:
    // Insert into current thread's db-cursor map and txn_csrs_ map,
    // for later duplication.
    //
    dcbcsr->set_cursor(csr);
    this->add_cursor(pdb, dcbcsr);
    return 0;
}

void ResourceManager::add_cursor(Db* dbp, DbCursorBase* dcbcsr)
{
    if (!dbp || !dcbcsr)
        return;
    assert(dcbcsr->get_cursor() != NULL);

    (all_csrs_[dbp])->insert(dcbcsr);
    // Register to txncsrs_, we suppose current transaction is the context
    // of this operation.
    //
    this->add_txn_cursor(dcbcsr, dbp->get_env());
}

// Close dbp's all open cursors opened in current thread, do not close
// those of dbp opened in other threads, Db::truncate requires dbp's
// all cursors of all threads should be closed, and it is user's duty
// to make sure other threads all close dbp's cursor, because if we
// close them here, it is also an error---multi-threaded access to the same
// Dbc* cursor should be serialized, we can't serialize with user code
// anyway.
//
size_t ResourceManager::close_db_cursors(Db* dbp1)
{
    int ret;
    Db* dbp;
    DbTxn *ptxn, *ptxn2;
    csrset_t *pcset_txn;

    if (dbp1 == NULL)
        return 0;

    dbp = dbp1;
    db_csr_map_t::iterator itr0;
    csrset_t::iterator itr;

    itr0 = all_csrs_.find(dbp1);
    if (itr0 == all_csrs_.end())
        return 0;

    csrset_t *pcset = itr0->second;

    pcset_txn = NULL;
    ptxn2 = ptxn = NULL;
    size_t txncsr_sz = txn_csrs_.size();

    for (itr = pcset->begin(), ret = 0; itr != pcset->end();
        ++itr, ret++) {

        BDBOP((*itr)->close(), ret);
        if (txncsr_sz > 0) {
            if (pcset_txn == NULL || ptxn !=
                (ptxn2 = (*itr)->get_owner_txn())) {
                ptxn = ptxn2 ? ptxn2 : (*itr)->get_owner_txn();
                if (ptxn != NULL)
                    pcset_txn = txn_csrs_[ptxn];
            }
            if (pcset_txn)
                pcset_txn->erase(*itr);
        }

    }

    // Don't delete the pcset or itr0 because this dbp1 may be used
    // by other containers in this thread.
    pcset->clear();
    // We don't delete the DbCursorBase object, it is still
    // referenced by others.
    return ret;
}

// Close the cursor of csr and remove the entry containing csr from
// txn_csrs_ and all_csrs_.
int ResourceManager::remove_cursor(DbCursorBase*csr,
    bool remove_from_txncsrs)
{
    int ret;

    if (csr == NULL)
        return 0;
    BDBOP(csr->close(), ret);

    if (remove_from_txncsrs) {
        DbTxn *ptxn = csr->get_owner_txn();
        if (ptxn != NULL) {
            txncsr_t::iterator itr = txn_csrs_.find(ptxn);
            if (itr != txn_csrs_.end())
                itr->second->erase(csr);
        }
    }

    Db *pdb = csr->get_owner_db();
    if (pdb != NULL)
        all_csrs_[pdb]->erase(csr);

    return ret;
}

/*
 * Remove cursors opened in transaction txn's context, should be called before
 * commiting/aborting a transaction.
 * Note that here we should remove the cursor from all_csrs_ too,
 * by calling remove_cursor() function.
 */
void ResourceManager::remove_txn_cursor(DbTxn* txn)
{
    int ret;

    if (!txn)
        return;

    txncsr_t::iterator itr0;
    csrset_t::iterator itr;
    itr0 = txn_csrs_.find(txn);
    if (itr0 == txn_csrs_.end())
        return; // No cursor opened in this txn.

    csrset_t *pcsrs = itr0->second;
    DbCursorBase *csr;

    // Remove(close and remove from csr registry) cursors
    // opened in the transaction txn's context.
    for (itr = pcsrs->begin(); itr != pcsrs->end(); ++itr) {
        // This cursor should be closed now and removed
        // from csr registry.
        csr = *itr;
        BDBOP(csr->close(), ret);
        all_csrs_[csr->get_owner_db()]->erase(csr);
    }

    delete pcsrs;
    // Erase csrs belonging to txn.
    txn_csrs_.erase(itr0);
}

// Begin a new transaction from the specified environment env.
// When outtxn is non-zero, it supports nested txn,
// so the new transaction is started as a child transaction of the
// current one, and we push it into env1's transaction stack;
// Otherwise, we are starting an internal transaction for autocommit,
// no new transaction will be started, but current transaction's reference
// count will be incremented.
DbTxn* ResourceManager::begin_txn(u_int32_t flags, DbEnv*env1, int outtxn)
{
    DbEnv *env = env1;
    DbTxn *ptxn, *txn = NULL;
    int ret;

    if (!env1)
        return NULL;

    assert(env_txns_.count(env1) > 0);

    stack<DbTxn*>&stk = env_txns_[env1];

    // Not an outside transaction, so if there is transaction in stack,
    // use it and increment its reference count.
    if (outtxn == 0) {
        // We have a transaction in stack, increment its reference
        // count.
        if (stk.size() > 0) {
            txn = stk.top();
            // The txn was created externally, now we internally
            // use it, so the reference count is 2.
            map<DbTxn *, size_t>::iterator itr12;
            if ((itr12 = txn_count_.find(txn)) == txn_count_.end())
                txn_count_.insert(make_pair(txn, 2u));
            else
                txn_count_[txn]++;
        } else {
    // Empty stack, create a transaction and set reference count to 1.
            BDBOP(env->txn_begin(NULL, &txn, flags), ret);
            stk.push(txn);
            txn_count_[txn] = 1;// the first to use it
            txn_csrs_.insert(make_pair(txn, new csrset_t()));
        }

    } else { // Creating a transaction by user, used outside of dbstl.
        ptxn = stk.size() > 0 ? stk.top() : NULL;

        BDBOP(env->txn_begin(ptxn, &txn, flags), ret);

        // txn now is the current txn
        stk.push(txn);
        txn_csrs_.insert(make_pair(txn, new csrset_t()));
    }

    return txn;
}

void ResourceManager::commit_txn(DbEnv *env, u_int32_t flags)
{
    int ret;
    DbTxn *ptxn;

    if (!env)
        return;

    assert(env_txns_.count(env) > 0);
    stack<DbTxn*> &stk = env_txns_[env];
    ptxn = stk.top();
    assert(ptxn != NULL);
    size_t txncnt = txn_count_[ptxn];

    if (txncnt > 1) // used internally
        txn_count_[ptxn]--;
    else {
        txn_count_.erase(ptxn);
        this->remove_txn_cursor(ptxn);
        stk.pop();
        BDBOP(ptxn->commit(flags), ret);

    }

}

void ResourceManager::commit_txn(DbEnv *env, DbTxn *txn, u_int32_t flags)
{
    DbTxn *ptxn = NULL;
    int ret;

    if (env == NULL || txn == NULL)
        return;

    stack<DbTxn*> &stk = env_txns_[env];
    while (stk.size() > 0 && (ptxn = stk.top()) != txn) {
        stk.pop();
        txn_count_.erase(ptxn);// may be in the txn_count_ map
        this->remove_txn_cursor(ptxn);
        // Child txns could be committed by parent txn, but c++ API 
        // can't delete the new'ed child txns when committing the 
        // parent txn, so have to commit them explicitly.
        ptxn->commit(flags);
    }
    if (stk.size() == 0)
        THROW(InvalidArgumentException, (
            "No such transaction created by dbstl"));
    else {
        stk.pop();
        txn_count_.erase(txn);// may be in the txn_count_ map
        this->remove_txn_cursor(txn);
        if (ptxn){
            BDBOP(ptxn->commit(flags), ret);
        } else // could never happen
            THROW(InvalidArgumentException, (
                "No such transaction created by dbstl"));

    }
}

void ResourceManager::abort_txn(DbEnv *env, DbTxn *txn)
{
    int ret;
    DbTxn *ptxn = NULL;
    u_int32_t oflags;

    if (env == NULL || txn == NULL)
        return;

    BDBOP (env->get_open_flags(&oflags), ret);
    stack<DbTxn*> &stk = env_txns_[env];
    while (stk.size() > 0 && (ptxn = stk.top()) != txn) {
        txn_count_.erase(ptxn);// may be in the txn_count_ map
        this->remove_txn_cursor(ptxn);
        stk.pop();
        // Child txns could be aborted by parent txn, but c++ API 
        // can't delete the new'ed child txns when aborting the 
        // parent txn, so have to abort them explicitly.
        ptxn->abort();
    }
    if (stk.size() == 0)
        THROW(InvalidArgumentException, (
            "No such transaction created by dbstl"));
    else {
        stk.pop();
        txn_count_.erase(txn);// may be in the txn_count_ map
        this->remove_txn_cursor(txn);
        if (ptxn){
            if ((oflags & DB_INIT_CDB) == 0)
                BDBOP(ptxn->abort(), ret);
        } else // could never happen
            THROW(InvalidArgumentException, (
                "No such transaction created by dbstl"));

    }
}

// Abort current txn, close/remove its cursors and reference count.
void ResourceManager::abort_txn(DbEnv*env)
{
    int ret;
    DbTxn *ptxn;
    u_int32_t oflags;

    if (!env)
        return;
    env_txns_t::iterator itr(env_txns_.find(env));
    if (itr == env_txns_.end())
        return;

    stack<DbTxn*> &stk = itr->second;
    if (stk.size() == 0)
        return;
    ptxn = stk.top();
    if (ptxn == NULL)
        return;
    this->remove_txn_cursor(ptxn);
    BDBOP (env->get_open_flags(&oflags), ret);

    // Transactions handles created via cdsgroup_begin can not be aborted
    // because they are not really transactions, they just borrow the
    // DB_TXN structure to store a locker id.
    if ((oflags & DB_INIT_CDB) == 0) 
        BDBOP(ptxn->abort(), ret);
    txn_count_.erase(ptxn);
    stk.pop();
}

DbTxn* ResourceManager::set_current_txn_handle(DbEnv *env, DbTxn *newtxn)
{
    assert(env_txns_.count(env) > 0);
    stack<DbTxn*> &stk = env_txns_[env];
    DbTxn *ptxn = stk.top();
    stk.pop();
    stk.push(newtxn);
    return ptxn;
}

void ResourceManager::add_txn_cursor(DbCursorBase *dcbcsr, DbEnv *env)
{
    if (!env || !dcbcsr)
        return;

    DbTxn *ptxn = this->current_txn(env);
    if (ptxn == NULL)
        return;

    u_int32_t oflags;
    int ret;

    BDBOP(env->get_open_flags(&oflags), ret);
    if ((oflags & DB_INIT_TXN) == 0)
        return;

    txncsr_t::iterator itr;
    csrset_t *pset;

    itr = txn_csrs_.find(ptxn);
    pair<txncsr_t::iterator, bool> insret;

    if (itr == txn_csrs_.end()) {
        insret = txn_csrs_.insert(make_pair(ptxn, new csrset_t()));
        assert(insret.second);
        itr = insret.first;
    }
    pset = itr->second;
    pset->insert(dcbcsr);
}

void ResourceManager::register_db(Db*pdb1)
{
    if (!pdb1)
        return;
    global_lock(mtx_handle_);
    if (open_dbs_.count(pdb1) == 0)
        open_dbs_.insert(make_pair(pdb1, 1u));
    else
        open_dbs_[pdb1]++;
    global_unlock(mtx_handle_);
    csrset_t *pcsrset = new csrset_t();
    pair<db_csr_map_t::iterator, bool> insret = all_csrs_.insert(
        make_pair(pdb1, pcsrset));
    if (!insret.second)
        delete pcsrset;

}

void ResourceManager::register_db_env(DbEnv*env1)
{
    u_int32_t oflags = 0;
    DbTxn *ptxn = NULL;
    int ret;

    if (!env1)
        return;

    stack<DbTxn*> stk;
    BDBOP(env1->get_open_flags(&oflags), ret);

    if (oflags & DB_INIT_CDB) {
        env1->cdsgroup_begin(&ptxn);
        stk.push(ptxn);
    }

    env_txns_.insert(make_pair(env1, stk));

    global_lock(mtx_handle_);
    if (open_envs_.count(env1) == 0)
        open_envs_.insert(make_pair(env1, 1u));
    else
        open_envs_[env1]++;
    global_unlock(mtx_handle_);
}

// Delete registered DbstlGlobalInnerObject objects.
void ResourceManager::global_exit()
{
    set<DbstlGlobalInnerObject *>::iterator itr;
    global_lock(mtx_globj_);
    for (itr = glob_objs_.begin(); itr != glob_objs_.end(); ++itr)
        delete *itr;
    global_unlock(mtx_globj_);

    mtx_env_->mutex_free(mtx_globj_);
    mtx_env_->mutex_free(mtx_handle_);
    delete mtx_env_;
}

void ResourceManager::register_global_object(DbstlGlobalInnerObject *gio)
{
    global_lock(mtx_globj_);
    glob_objs_.insert(gio);
    global_unlock(mtx_globj_);
}

END_NS
