/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_DBC_H
#define _DB_STL_DBC_H

#include <set>
#include <errno.h>

#include "dbstl_common.h"
#include "dbstl_dbt.h"
#include "dbstl_exception.h"
#include "dbstl_container.h"
#include "dbstl_resource_manager.h"

START_NS(dbstl)

// Forward declarations.
class db_container;
class DbCursorBase;
template<Typename data_dt>
class RandDbCursor;
class DbstlMultipleKeyDataIterator;
class DbstlMultipleRecnoDataIterator;
using std::set;

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//
// LazyDupCursor class template definition.
//
// This class allows us to make a shallow copy on construction. When the
// cursor pointer is first dereferenced a deep copy is made.
//
// The allowed type for BaseType is DbCursor<> and RandDbCursor<>
// The expected usage of this class is:
// 1. Create an iterator in container::begin(), the iterator::pcsr.csr_ptr_
// points to an object, thus no need to duplicate.
// 2. The iterator is created with default argument, thus the
// iterator::pcsr.csr_ptr_ and dup_src_ is NULL, and this iterator is
// copied using copy constructor for may be many times, but until the
// cursor is really used, no cursor is duplicated.
//
// There is an informing mechanism between an instance of this class and
// its dup_src_ cursor: when that cursor is about to change state, it will
// inform all registered LazyDupCursor "listeners" of the change, so that
// they will duplicate from the cursor before the change, because that
// is the expected cursor state for the listeners.

template <Typename BaseType>
class LazyDupCursor
{
    // dup_src_ is used by this class internally to duplicate another
    // cursor and set to csr_ptr_, and it is assigned in the copy
    // constructor from another LazyDupCursor object's csr_ptr_; csr_ptr_
    // is the acutual pointer that is used to perform cursor operations.
    //
    BaseType *csr_ptr_, *dup_src_;
    typedef LazyDupCursor<BaseType> self;

public:
    ////////////////////////////////////////////////////////////////////
    //
    // Begin public constructors and destructor.
    //
    inline LazyDupCursor()
    {
        csr_ptr_ = NULL;
        dup_src_ = NULL;
    }

    // Used in all iterator types' constructors, dbcptr is created
    // solely for this object, and the cursor is not yet opened, so we
    // simply assign it to csr_ptr_.
    explicit inline LazyDupCursor(BaseType *dbcptr)
    {
        csr_ptr_ = dbcptr;
        // Already have pointer, do not need to duplicate.
        dup_src_ = NULL;
    }

    // Do not copy to csr_ptr_, shallow copy from dp2.csr_ptr_.
    LazyDupCursor(const self& dp2)
    {
        csr_ptr_ = NULL;
        if (dp2.csr_ptr_)
            dup_src_ = dp2.csr_ptr_;
        else
            dup_src_ = dp2.dup_src_;
        if (dup_src_)
            dup_src_->add_dupper(this);
    }

    ~LazyDupCursor()
    {
        // Not duplicated yet, remove from dup_src_.
        if (csr_ptr_ == NULL && dup_src_ != NULL)
            dup_src_->erase_dupper(this);
        if (csr_ptr_)
            delete csr_ptr_;// Delete the cursor.
    }

    ////////////////////////////////////////////////////////////////////

    // Deep copy.
    inline const self& operator=(const self &dp2)
    {
        BaseType *dcb;

        dcb = dp2.csr_ptr_ ? dp2.csr_ptr_ : dp2.dup_src_;
        this->operator=(dcb);

        return dp2;
    }

    // Deep copy.
    inline BaseType *operator=(BaseType *dcb)
    {

        if (csr_ptr_) {
            // Only dup_src_ will inform this, not csr_ptr_.
            delete csr_ptr_;
            csr_ptr_ = NULL;
        }

        if (dcb)
            csr_ptr_ = new BaseType(*dcb);
        if (dup_src_ != NULL) {
            dup_src_->erase_dupper(this);
            dup_src_ = NULL;
        }

        return dcb;
    }

    void set_cursor(BaseType *dbc)
    {
        assert(dbc != NULL);
        if (csr_ptr_) {
            // Only dup_src_ will inform this, not csr_ptr_.
            delete csr_ptr_;
            csr_ptr_ = NULL;
        }

        csr_ptr_ = dbc;
        if (dup_src_ != NULL) {
            dup_src_->erase_dupper(this);
            dup_src_ = NULL;
        }
    }

    // If dup_src_ is informing this object, pass false parameter.
    inline BaseType* duplicate(bool erase_dupper = true)
    {
        assert(dup_src_ != NULL);
        if (csr_ptr_) {
            // Only dup_src_ will inform this, not csr_ptr_.
            delete csr_ptr_;
            csr_ptr_ = NULL;
        }
        csr_ptr_ = new BaseType(*dup_src_);
        if (erase_dupper)
            dup_src_->erase_dupper(this);
        dup_src_ = NULL;
        return csr_ptr_;
    }

    inline BaseType* operator->()
    {
        if (csr_ptr_)
            return csr_ptr_;

        return duplicate();
    }

    inline operator bool()
    {
        return csr_ptr_ != NULL;
    }

    inline bool operator!()
    {
        return !csr_ptr_;
    }

    inline bool operator==(void *p)
    {
        return csr_ptr_ == p;
    }

    inline BaseType* base_ptr(){
        if (csr_ptr_)
            return csr_ptr_;
        return duplicate();
    }
};


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// DbCursorBase class definition.
//
// DbCursorBase is the base class for DbCursor<> class template, this class
// wraps the Berkeley DB cursor, in order for the ResourceManager to close
// the Berkeley DB cursor and set the pointer to null. 
// If we don't set the cursor to NULL, the handle could become valid again,
// since Berkeley DB recycles handles. DB STL would then try to use the same
// handle across different instances, which is not supported.
//
// In ResourceManager, whenver a cursor is opened, it stores the
// DbCursorBase* pointer, so that when need to close the cursor, it calls
// DbCursorBase::close() function.
//
class DbCursorBase
{
protected:
    Dbc *csr_;
    DbTxn *owner_txn_;
    Db *owner_db_;
    int csr_status_;

public:
    enum DbcGetSkipOptions{SKIP_KEY, SKIP_DATA, SKIP_NONE};
    inline DbTxn *get_owner_txn() const { return owner_txn_;}
    inline void set_owner_txn(DbTxn *otxn) { owner_txn_ = otxn;}

    inline Db *get_owner_db() const { return owner_db_;}
    inline void set_owner_db(Db *odb) { owner_db_ = odb;}

    inline Dbc *get_cursor()  const { return csr_;}
    inline Dbc *&get_cursor_reference() { return csr_;}
    inline void set_cursor(Dbc*csr1)
    {
        if (csr_)
            ResourceManager::instance()->remove_cursor(this);
        csr_ = csr1;
    }

    inline int close()
    {
        int ret = 0;

        if (csr_ != NULL && (((DBC *)csr_)->flags & DBC_ACTIVE) != 0) {
            ret = csr_->close();
            csr_ = NULL;
        }
        return ret;
    }

    DbCursorBase(){
        owner_txn_ = NULL;
        owner_db_ = NULL;
        csr_ = NULL;
        csr_status_ = 0;
    }

    DbCursorBase(const DbCursorBase &csrbase)
    {
        this->operator=(csrbase);
    }

    const DbCursorBase &operator=(const DbCursorBase &csrbase)
    {
        owner_txn_ = csrbase.owner_txn_;
        owner_db_ = csrbase.owner_db_;
        csr_ = NULL; // Need to call DbCursor<>::dup to duplicate.
        csr_status_ = 0;
        return csrbase;
    }

    virtual ~DbCursorBase()
    {
        close();
    }
}; // DbCursorBase

////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//
// DbCursor class template definition
//
// DbCursor is the connection between Berkeley DB and dbstl container classes
// it is the wrapper class for Dbc* cursor of Berkeley Db, to be used for
// iterator classes of Berkeley DB backed STL container classes.
// Requirement:
// 1. Deep copy using Dbc->dup.
// 2. Dbc*cursor management via ResourceManager class.
// 3. Provide methods to do increment, decrement and advance operations,
//    advance is only available for random access iterator from DB_RECNO
//    containers.
//

template<typename key_dt, typename data_dt>
class DbCursor : public DbCursorBase{
protected:
    // Lazy duplication support: store the LazyDupCursor objects which
    // will duplicate from this cursor.
    typedef LazyDupCursor<DbCursor<key_dt, data_dt> > dupper_t;
    typedef LazyDupCursor<RandDbCursor<data_dt> > dupperr_t;
    typedef set<LazyDupCursor<DbCursor<key_dt, data_dt> >* > dupset_t;
    typedef set<LazyDupCursor<RandDbCursor<data_dt> >* > dupsetr_t;

    set<LazyDupCursor<DbCursor<key_dt, data_dt> >* > sduppers1_;
    set<LazyDupCursor<RandDbCursor<data_dt> >* > sduppers2_;

    // We must use DB_DBT_USERMEM for Dbc::get and Db::get if they are
    // used in multi-threaded application, so we use key_buf_ and
    // data_buf_ data members for get operations, and initialize them
    // to use user memory.
    Dbt key_buf_, data_buf_;

    // Similar to Berkeley DB C++ API's classes, used to iterate through
    // bulk retrieved key/data pairs.
    DbstlMultipleKeyDataIterator *multi_itr_;
    DbstlMultipleRecnoDataIterator *recno_itr_;

    // Whether to use bulk retrieval. If non-zero, do bulk retrieval,
    // bulk buffer size is this member, otherwise not bulk read.
    // By default this member is 0.
    u_int32_t bulk_retrieval_;
    // Whether to use DB_RMW flag in Dbc::get, by default false.
    bool rmw_get_;

    // Whether to poll data from cursor's current position on every
    // get_current_key/data call.
    // Note that curr_key_/curr_data_ members are always maintained
    // to contain current k/d value of the pair pointed to by csr_.
    // If doing bulk retrieval, this flag is ignored, we will always
    // read data from bulk buffer.
    bool directdb_get_;

    // Inform LazyDupCursor objects registered in this object to do
    // duplication because this cursor is to be changed.
    // This function should be called in any function of
    // DbCursor and RandDbCursor whenever the cursor is about to change
    // state(move/close, etc).
    inline void inform_duppers()
    {
        typename dupset_t::iterator i1;
        typename dupsetr_t::iterator i2;
        for (i1 = sduppers1_.begin(); i1 != sduppers1_.end(); i1++)
            (*i1)->duplicate(false);
        for (i2 = sduppers2_.begin(); i2 != sduppers2_.end(); i2++)
            (*i2)->duplicate(false);
        sduppers1_.clear();
        sduppers2_.clear();
    }

public:
    friend class DataItem;

    // Current key/data pair pointed by "csr_" Dbc*cursor. They are both
    // maintained on cursor movement. If directdb_get_ is true,
    // they are both refreshed on every get_current{[_key][_data]} call and 
    // the retrieved key/data pair is returned to user.
    DataItem curr_key_;
    DataItem curr_data_;

    typedef DbCursor<key_dt, data_dt> self;

    // This function is used by all iterators to do equals comparison.
    // Random iterators will also use it to do less than/greater than
    // comparisons.
    // Internally, the page number difference or index difference is
    // returned, so for btree and hash databases, if two cursors point to
    // the same key/data pair, we will get 0 returned, meaning they are
    // equal; if return value is not 0, it means no more than that they
    // they are not equal. We can't assume any order information between
    // the two cursors. For recno databases, we use the recno to do less
    // than and greater than comparison. So we can get a reliable knowledge
    // of the relative position of two iterators from the return value.
    int compare(const self *csr2) const{
        int res, ret;

        BDBOP(((DBC *)csr_)->cmp((DBC *)csr_, (DBC *)csr2->csr_,
            &res, 0), ret);
        return res;
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Add and remove cursor change event listeners.
    //
    inline void add_dupper(dupper_t *dupper)
    {
        sduppers1_.insert(dupper);
    }

    inline void add_dupper(dupperr_t *dupper)
    {
        sduppers2_.insert(dupper);
    }

    inline void erase_dupper(dupper_t *dup1)
    {
        sduppers1_.erase(dup1);
    }

    inline void erase_dupper(dupperr_t *dup1)
    {
        sduppers2_.erase(dup1);
    }

    ////////////////////////////////////////////////////////////////////

public:

    inline bool get_rmw()
    {
        return rmw_get_;
    }

    bool set_rmw(bool rmw, DB_ENV *env = NULL )
    {
        u_int32_t flag = 0;
        DB_ENV *dbenv = NULL;
        int ret;

        if (env)
            dbenv = env;
        else
            dbenv = ((DBC*)csr_)->dbenv;
        BDBOP(dbenv->get_open_flags(dbenv, &flag), ret);

        // DB_RMW flag requires locking subsystem started.
        if (rmw && ((flag & DB_INIT_LOCK) || (flag & DB_INIT_CDB) ||
            (flag & DB_INIT_TXN)))
            rmw_get_ = true;
        else
            rmw_get_ = false;
        return rmw_get_;
    }

    // Modify bulk buffer size. Bulk read is enabled when creating an
    // iterator, so users later can only modify the bulk buffer size
    // to another value, but can't enable/disable bulk read while an
    // iterator is already alive. 
    // Returns true if succeeded, false otherwise.
    inline bool set_bulk_buffer(u_int32_t sz)
    {
        if (bulk_retrieval_ && sz) {
            normalize_bulk_bufsize(sz);
            bulk_retrieval_ = sz;
            return true;
        }

        return false;

    }

    inline u_int32_t get_bulk_bufsize()
    {
        return bulk_retrieval_;
    }

    inline void enlarge_dbt(Dbt &d, u_int32_t sz)
    {
        void *p;

        p = DbstlReAlloc(d.get_data(), sz);
        dbstl_assert(p != NULL);
        d.set_ulen(sz);
        d.set_data(p);
        d.set_size(sz);
    }
    // Move forward or backward, often by 1 key/data pair, we can use
    // different flags for Dbc::get function. Then update the key/data
    // pair and csr_status_ members.
    //
    int increment(int flag)
    {
        int ret = 0;
        Dbt &k = key_buf_, &d = data_buf_;
        u_int32_t sz, getflags = 0, bulk_bufsz;

        if (csr_ == NULL)
            return INVALID_ITERATOR_CURSOR;
        curr_key_.reset();
        curr_data_.reset();
        inform_duppers();

        // Berkeley DB cursor flags are not bitwise set, so we can't
        // use bit operations here.
        //
        if (this->bulk_retrieval_ != 0)
            switch (flag) {
            case DB_PREV:
            case DB_PREV_DUP:
            case DB_PREV_NODUP:
            case DB_LAST:
            case DB_JOIN_ITEM:
            case DB_GET_RECNO:
            case DB_SET_RECNO: 
                break;
            default:
                getflags |= DB_MULTIPLE_KEY;
                if (data_buf_.get_ulen() != bulk_retrieval_)
                    enlarge_dbt(data_buf_, bulk_retrieval_);
                break;
            }

        if (this->rmw_get_)
            getflags |= DB_RMW;

        // Do not use BDBOP or BDBOP2 here because it is likely
        // that an iteration will step onto end() position.
retry:      ret = csr_->get(&k, &d, flag | getflags);
        if (ret == 0) {
            if (bulk_retrieval_ && (getflags & DB_MULTIPLE_KEY)) {
                // A new retrieval, so both multi_itr_ and
                // recno_itr_ must be NULL.
                if (((DBC*)csr_)->dbtype == DB_RECNO) {
                    if (recno_itr_) {
                        delete recno_itr_;
                        recno_itr_ = NULL;
                    }
                    recno_itr_ =
                    new DbstlMultipleRecnoDataIterator(d);
                } else {
                    if (multi_itr_) {
                        delete multi_itr_;
                        multi_itr_ = NULL;
                    }
                    multi_itr_ = new
                        DbstlMultipleKeyDataIterator(d);
                }
            } else {
                // Non bulk retrieval succeeded.
                curr_key_.set_dbt(k, false);
                curr_data_.set_dbt(d, false);
                limit_buf_size_after_use();
            }
        } else if (ret == DB_BUFFER_SMALL) {
            // Either the key or data DBTs might trigger a
            // DB_KEYSMALL return. Only enlarge the DBT if it 
            // is actually too small. 
            if (((sz = d.get_size()) > 0) && (sz > d.get_ulen()))
                enlarge_dbt(d, sz);

            if (((sz = k.get_size()) > 0) && (sz > k.get_ulen()))
                enlarge_dbt(k, sz);

            goto retry;
        } else {
            if (ret == DB_NOTFOUND) {
                ret = INVALID_ITERATOR_POSITION;
                this->curr_key_.reset();
                this->curr_data_.reset();
            } else if (bulk_retrieval_ &&
                (getflags & DB_MULTIPLE_KEY)){
                BDBOP(((DBC*)csr_)->dbp->
                    get_pagesize(((DBC*)csr_)->
                    dbp, &bulk_bufsz), ret);
                if (bulk_bufsz > d.get_ulen()) {// buf size error
                    normalize_bulk_bufsize(bulk_bufsz);
                    bulk_retrieval_ = bulk_bufsz;
                    enlarge_dbt(d, bulk_bufsz);
                    goto retry;
                } else
                    throw_bdb_exception(
                        "DbCursor<>::increment", ret);
            } else
                throw_bdb_exception(
                    "DbCursor<>::increment", ret);
        }

        csr_status_ = ret;
        return ret;
    }

    // After each use of key_buf_ and data_buf_, limit their buffer size to
    // a reasonable size so that they don't waste a big memory space.
    inline void limit_buf_size_after_use() 
    {
        if (bulk_retrieval_)
            // Bulk buffer has to be huge, so don't check it.
            return;

        if (key_buf_.get_ulen() > DBSTL_MAX_KEY_BUF_LEN) {
            key_buf_.set_data(DbstlReAlloc(key_buf_.get_data(),
                DBSTL_MAX_KEY_BUF_LEN));
            key_buf_.set_ulen(DBSTL_MAX_KEY_BUF_LEN);
        }
        if (data_buf_.get_ulen() > DBSTL_MAX_DATA_BUF_LEN) {
            data_buf_.set_data(DbstlReAlloc(data_buf_.get_data(),
                DBSTL_MAX_DATA_BUF_LEN));
            data_buf_.set_ulen(DBSTL_MAX_DATA_BUF_LEN);
        }
    }

    // Duplicate this object's cursor and set it to dbc1.
    //
    inline int dup(DbCursor<key_dt, data_dt>& dbc1) const
    {
        Dbc* pcsr = 0;
        int ret;

        if (csr_ != 0 && csr_->dup(&pcsr, DB_POSITION) == 0) {
            dbc1.set_cursor(pcsr);
            dbc1.set_owner_db(this->get_owner_db());
            dbc1.set_owner_txn(this->get_owner_txn());
            ResourceManager::instance()->add_cursor(
                this->get_owner_db(), &dbc1);
            ret = 0;
        } else
            ret = ITERATOR_DUP_ERROR;

        return ret;
    }

public:
    // Open a cursor, do not move it, it is at an invalid position.
    // All cursors should be opened using this method.
    //
    inline int open(db_container *pdbc, int flags)
    {
        int ret;

        Db *pdb = pdbc->get_db_handle();
        if (pdb == NULL)
            return 0;
        if (csr_) // Close before open.
            return 0;
        ret = ResourceManager::instance()->
            open_cursor(this, pdb, flags);
        set_rmw(rmw_get_);
        this->csr_status_ = ret;
        return ret;
    }

    // Move Berkeley DB cursor to specified key k, by default use DB_SET,
    // but DB_SET_RANGE can and may also be used.
    //
    int move_to(const key_dt&k, u_int32_t flag = DB_SET)
    {
        Dbt &d1 = data_buf_;
        int ret;
        u_int32_t sz;
        DataItem k1(k, true);

        if (csr_ == NULL)
            return INVALID_ITERATOR_CURSOR;

        curr_key_.reset();
        curr_data_.reset();
        inform_duppers();

        // It is likely that k is not in db, causing get(DB_SET) to
        // fail, we should not throw an exception because of this.
        //
        if (rmw_get_)
            flag |= DB_RMW;
retry:      ret = csr_->get(&k1.get_dbt(), &d1, flag);
        if (ret == 0) {
            curr_key_ = k1;
            curr_data_.set_dbt(d1, false);
            limit_buf_size_after_use();
        } else if (ret == DB_BUFFER_SMALL) {
            sz = d1.get_size();
            assert(sz > 0);
            enlarge_dbt(d1, sz);
            goto retry;
        } else {
            if (ret == DB_NOTFOUND) {
                ret = INVALID_ITERATOR_POSITION;
                // Invalidate current values because it is
                // at an invalid position.
                this->curr_key_.reset();
                this->curr_data_.reset();
            } else
                throw_bdb_exception("DbCursor<>::move_to", ret);
        }

        csr_status_ = ret;
        return ret;
    }

    // Returns the number of keys equal to the current one.
    inline size_t count()
    {
        int ret;
        db_recno_t cnt;

        BDBOP2(csr_->count(&cnt, 0), ret, close());
        return (size_t)cnt;
    }

    int insert(const key_dt&k, const data_dt& d, int pos = DB_BEFORE)
    {
        // !!!XXX:
        //         We do a deep copy of the input data into a local
        //         variable. Apparently not doing so causes issues
        //         when using gcc. Even though the put completes prior
        //         to returning from this function call.
        //         It would be best to avoid this additional copy.
        int ret;
        // (k, d) pair may be a temporary pair, so we must copy them.
        DataItem k1(k, false), d1(d, false);

        inform_duppers();
        if (pos == DB_AFTER) {
            ret = this->csr_->put(&k1.get_dbt(), &d1.get_dbt(),
                pos);
            // May be using this flag for an empty database,
            // because begin() an iterator of an empty db_vector
            // equals its end() iterator, so use DB_KEYLAST to
            // retry.
            //
            if (ret == EINVAL || ret == 0)
                return ret;
            else if (ret)
                throw_bdb_exception("DbCursor<>::insert", ret);
        }
        if (pos == DB_NODUPDATA)
            BDBOP3(this->csr_->put(&k1.get_dbt(), &d1.get_dbt(),
                pos), ret, DB_KEYEXIST, close());
        else
            BDBOP2(this->csr_->put(&k1.get_dbt(), &d1.get_dbt(),
                pos), ret, close());
        this->csr_status_ = ret;
        if (ret == 0) {
            curr_key_ = k1;
            curr_data_ = d1;
        }
        // This cursor points to the new key/data pair now.
        return ret;
    }

    // Replace current cursor-pointed data item with d.
    inline int replace(const data_dt& d)
    {
        Dbt k1;
        int ret;
        // !!!XXX:
        //         We do a deep copy of the input data into a local
        //         variable. Apparently not doing so causes issues
        //         when using gcc. Even though the put completes prior
        //         to returning from this function call.
        //         It would be best to avoid this additional copy.
        // d may be a temporary object, so we must copy it.
        DataItem d1(d, false);

        
        BDBOP2(this->csr_->put(&k1, &d1.get_dbt(), DB_CURRENT),
            ret, close());
        curr_data_ = d1; // Update current data.
        
        this->csr_status_ = ret;
        return ret;
    }

    // Remove old key and insert new key-psuodo_data. First insert then
    // move to old key and remove it so that the cursor remains at the
    // old key's position, according to DB documentation.
    // But from practice I can see
    // the cursor after delete seems not at old position because a for
    // loop iteration exits prematurelly, not all elements are passed.
    //
    inline int replace_key(const key_dt&k)
    {
        data_dt d;
        key_dt k0;
        int ret;

        this->get_current_key_data(k0, d);
        if (k0 == k)
            return 0;

        DbCursor<key_dt, data_dt> csr2;
        this->dup(csr2);
        // Delete current, then insert new key/data pair.
        ret = csr2.del(); 
        ret = csr2.insert(k, d, DB_KEYLAST);
        this->csr_status_ = ret;
        
        // Now this->csr_ is sitting on an invalid position, its 
        // iterator is invalidated. Must first move it to the next
        // position before using it.
        return ret;
    }

    inline int del()
    {
        int ret;

        inform_duppers();
        BDBOP2(csr_->del(0), ret, close());

        // By default pos.csr_ will stay at where it was after delete,
        // which now is an invalid position. So we need to move to
        // next to conform to stl specifications, but we don't move it
        // here, iterator::erase should move the iterator itself 
        // forward.
        //
        this->csr_status_ = ret;
        return ret;
    }

    // Make sure the bulk buffer is large enough, and a multiple of 1KB. 
    // This function may be called prior to cursor initialization, it is 
    // not possible to verify that the buffer size is a multiple of the 
    // page size here.
    u_int32_t normalize_bulk_bufsize(u_int32_t &bulksz)
    {
        if (bulksz == 0)
            return 0;

        while (bulksz < 16 * sizeof(data_dt))
            bulksz *= 2;

        bulksz = bulksz + 1024 - bulksz % 1024;

        return bulksz;
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Begin public constructors and destructor.
    //
    explicit DbCursor(u_int32_t b_bulk_retrieval = 0, bool brmw1 = false,
        bool directdbget = true) : DbCursorBase(),
        curr_key_(sizeof(key_dt)), curr_data_(sizeof(data_dt))
    {
        u_int32_t bulksz = sizeof(data_dt); // non-bulk
        rmw_get_ = brmw1;
        this->bulk_retrieval_ = 
            normalize_bulk_bufsize(b_bulk_retrieval);
        recno_itr_ = NULL;
        multi_itr_ = NULL;

        if (bulk_retrieval_) {
            if (bulksz <= bulk_retrieval_)
                bulksz = bulk_retrieval_;
            else {
                normalize_bulk_bufsize(bulksz);
                bulk_retrieval_ = bulksz;
            }
        }
        key_buf_.set_data(DbstlMalloc(sizeof(key_dt)));
        key_buf_.set_ulen(sizeof(key_dt));
        key_buf_.set_flags(DB_DBT_USERMEM);
        data_buf_.set_data(DbstlMalloc(bulksz));
        data_buf_.set_ulen(bulksz);
        data_buf_.set_flags(DB_DBT_USERMEM);
        directdb_get_ = directdbget;
    }

    // Copy constructor, duplicate cursor here.
    DbCursor(const DbCursor<key_dt, data_dt>& dbc) :
        DbCursorBase(dbc),
        curr_key_(dbc.curr_key_), curr_data_(dbc.curr_data_)
    {
        void *pk, *pd;

        dbc.dup(*this);
        csr_status_ = dbc.csr_status_;
        if (csr_ || dbc.csr_)
            this->rmw_get_ = set_rmw(dbc.rmw_get_,
                ((DBC*)dbc.csr_)->dbenv);
        else
            rmw_get_ = dbc.rmw_get_;

        bulk_retrieval_ = dbc.bulk_retrieval_;

        // Now we have to copy key_buf_ and data_buf_ to support
        // multiple retrieval.
        key_buf_.set_data(pk = DbstlMalloc(dbc.key_buf_.get_ulen()));
        key_buf_.set_ulen(dbc.key_buf_.get_ulen());
        key_buf_.set_size(dbc.key_buf_.get_size());
        key_buf_.set_flags(DB_DBT_USERMEM);
        memcpy(pk, dbc.key_buf_.get_data(), key_buf_.get_ulen());

        data_buf_.set_data(pd = DbstlMalloc(dbc.data_buf_.get_ulen()));
        data_buf_.set_ulen(dbc.data_buf_.get_ulen());
        data_buf_.set_size(dbc.data_buf_.get_size());
        data_buf_.set_flags(DB_DBT_USERMEM);
        memcpy(pd, dbc.data_buf_.get_data(), data_buf_.get_ulen());
        if (dbc.recno_itr_) {
            recno_itr_ = new DbstlMultipleRecnoDataIterator(
                data_buf_);
            recno_itr_->set_pointer(dbc.recno_itr_->get_pointer());
        } else
            recno_itr_ = NULL;
        if (dbc.multi_itr_) {
            multi_itr_ = new DbstlMultipleKeyDataIterator(
                data_buf_);
            multi_itr_->set_pointer(dbc.multi_itr_->get_pointer());

        } else
            multi_itr_ = NULL;

        directdb_get_ = dbc.directdb_get_;

        // Do not copy sduppers, they are private to each DbCursor<>
        // object.
    }

    virtual ~DbCursor()
    {
        close(); // Call close() ahead of freeing following buffers.
        free(key_buf_.get_data());
        free(data_buf_.get_data());
        if (multi_itr_)
            delete multi_itr_;
        if (recno_itr_)
            delete recno_itr_;
    }

    ////////////////////////////////////////////////////////////////////

    const DbCursor<key_dt, data_dt>& operator=
        (const DbCursor<key_dt, data_dt>& dbc)
    {
        void *pk;
        u_int32_t ulen;

        DbCursorBase::operator =(dbc);
        dbc.dup(*this);
        curr_key_ = dbc.curr_key_;
        curr_data_ = dbc.curr_data_;
        rmw_get_ = dbc.rmw_get_;
        this->bulk_retrieval_ = dbc.bulk_retrieval_;
        this->directdb_get_ = dbc.directdb_get_;
        // Now we have to copy key_buf_ and data_buf_ to support
        // bulk retrieval.
        key_buf_.set_data(pk = DbstlReAlloc(key_buf_.get_data(),
            ulen = dbc.key_buf_.get_ulen()));
        key_buf_.set_ulen(ulen);
        key_buf_.set_size(dbc.key_buf_.get_size());
        key_buf_.set_flags(DB_DBT_USERMEM);
        memcpy(pk, dbc.key_buf_.get_data(), ulen);

        data_buf_.set_data(pk = DbstlReAlloc(key_buf_.get_data(),
            ulen = dbc.key_buf_.get_ulen()));
        data_buf_.set_ulen(ulen);
        data_buf_.set_size(dbc.data_buf_.get_size());
        data_buf_.set_flags(DB_DBT_USERMEM);
        memcpy(pk, dbc.key_buf_.get_data(), ulen);

        if (dbc.recno_itr_) {
            if (recno_itr_) {
                delete recno_itr_;
                recno_itr_ = NULL;
            }
            recno_itr_ = new DbstlMultipleRecnoDataIterator(
                data_buf_);
            recno_itr_->set_pointer(dbc.recno_itr_->get_pointer());
        } else if (recno_itr_) {
            delete recno_itr_;
            recno_itr_ = NULL;
        }

        if (dbc.multi_itr_) {
            if (multi_itr_) {
                delete multi_itr_;
                multi_itr_ = NULL;
            }
            multi_itr_ = new DbstlMultipleKeyDataIterator(
                data_buf_);
            multi_itr_->set_pointer(dbc.multi_itr_->get_pointer());

        } else if (multi_itr_) {
            delete multi_itr_;
            multi_itr_ = NULL;
        }

        return dbc;
        // Do not copy sduppers, they are private to each DbCursor<>
        // object.

    }

    // Move Dbc*cursor to next position. If doing bulk read, read from
    // the bulk buffer. If bulk buffer exhausted, do another bulk read
    // from database, and then read from the bulk buffer. Quit if no
    // more data in database.
    //
    int next(int flag = DB_NEXT)
    {
        Dbt k, d;
        db_recno_t recno;
        int ret;

retry:      if (bulk_retrieval_) {
            if (multi_itr_) {
                if (multi_itr_->next(k, d)) {
                    curr_key_.set_dbt(k, false);
                    curr_data_.set_dbt(d, false);
                    return 0;
                } else {
                    delete multi_itr_;
                    multi_itr_ = NULL;
                }
            }
            if (recno_itr_) {
                if (recno_itr_->next(recno, d)) {
                    curr_key_.set_dbt(k, false);
                    curr_data_.set_dbt(d, false);
                    return 0;
                } else {
                    delete recno_itr_;
                    recno_itr_ = NULL;
                }
            }
        }
        ret = increment(flag);
        if (bulk_retrieval_ && ret == 0)
            goto retry;
        return ret;
    }

    inline int prev(int flag = DB_PREV)
    {
        return increment(flag);
    }

    // Move Dbc*cursor to first element. If doing bulk read, read data
    // from bulk buffer.
    int first()
    {
        Dbt k, d;
        db_recno_t recno;
        int ret;

        ret = increment(DB_FIRST);
        if (bulk_retrieval_) {
            if (multi_itr_) {
                if (multi_itr_->next(k, d)) {
                    curr_key_.set_dbt(k, false);
                    curr_data_.set_dbt(d, false);
                    return 0;
                } else {
                    delete multi_itr_;
                    multi_itr_ = NULL;
                }
            }
            if (recno_itr_) {
                if (recno_itr_->next(recno, d)) {
                    curr_key_.set_dbt(k, false);
                    curr_data_.set_dbt(d, false);
                    return 0;
                } else {
                    delete recno_itr_;
                    recno_itr_ = NULL;
                }
            }
        }

        return ret;
    }

    inline int last()
    {
        return increment(DB_LAST);
    }

    // Get current key/data pair, shallow copy. Return 0 on success,
    // -1 if no data.
    inline int get_current_key_data(key_dt&k, data_dt&d)
    {
        if (directdb_get_)
            update_current_key_data_from_db(
                DbCursorBase::SKIP_NONE);
        if (curr_key_.get_data(k) == 0 && curr_data_.get_data(d) == 0)
            return 0;
        else 
            return INVALID_KEY_DATA;
    }

    // Get current data, shallow copy. Return 0 on success, -1 if no data.
    inline int get_current_data(data_dt&d)
    {
        if (directdb_get_)
            update_current_key_data_from_db(DbCursorBase::SKIP_KEY);
        if (curr_data_.get_data(d) == 0)
            return 0;
        else 
            return INVALID_KEY_DATA;
    }

    // Get current key, shallow copy. Return 0 on success, -1 if no data.
    inline int get_current_key(key_dt&k)
    {
        if (directdb_get_)
            update_current_key_data_from_db(
                DbCursorBase::SKIP_DATA);
        if (curr_key_.get_data(k) == 0)
            return 0;
        else 
            return INVALID_KEY_DATA;
    }

    inline void close()
    {
        if (csr_) {
            inform_duppers();
            ResourceManager::instance()->remove_cursor(this);
        }
        csr_ = NULL;
    }

    // Parameter skipkd specifies skip retrieving key or data:
    // If 0, don't skip, retrieve both;
    // If 1, skip retrieving key;
    // If 2, skip retrieving data.
    // Do not poll from db again if doing bulk retrieval.
    void update_current_key_data_from_db(DbcGetSkipOptions skipkd) {
        int ret;
        u_int32_t sz, sz1, kflags = DB_DBT_USERMEM,
            dflags = DB_DBT_USERMEM;
        // Do not poll from db again if doing bulk retrieval.
        if (this->bulk_retrieval_)
            return;
        if (this->csr_status_ != 0) {
            curr_key_.reset();
            curr_data_.reset();
            return;
        }
        
        // We will modify flags if skip key or data, so cache old
        // value and set it after get calls.
        if (skipkd != DbCursorBase::SKIP_NONE) {
            kflags = key_buf_.get_flags();
            dflags = data_buf_.get_flags();
        }
        if (skipkd == DbCursorBase::SKIP_KEY) {
            key_buf_.set_dlen(0);
            key_buf_.set_flags(DB_DBT_PARTIAL | DB_DBT_USERMEM);
        }

        if (skipkd == DbCursorBase::SKIP_DATA) {
            data_buf_.set_dlen(0);
            data_buf_.set_flags(DB_DBT_PARTIAL | DB_DBT_USERMEM);
        }
retry:      ret = csr_->get(&key_buf_, &data_buf_, DB_CURRENT);
        if (ret == 0) {
            if (skipkd != DbCursorBase::SKIP_KEY)
                curr_key_ = key_buf_;
            if (skipkd != DbCursorBase::SKIP_DATA)
                curr_data_ = data_buf_;
            limit_buf_size_after_use();
        } else if (ret == DB_BUFFER_SMALL) {
            if ((sz = key_buf_.get_size()) > 0)
                enlarge_dbt(key_buf_, sz);
            if ((sz1 = data_buf_.get_size()) > 0) 
                enlarge_dbt(data_buf_, sz1);
            if (sz == 0 && sz1 == 0)
                THROW0(InvalidDbtException);
            goto retry;
        } else {
            if (skipkd != DbCursorBase::SKIP_NONE) {
                key_buf_.set_flags(kflags);
                data_buf_.set_flags(dflags);
            }
            throw_bdb_exception(
            "DbCursor<>::update_current_key_data_from_db", ret);
        }

        if (skipkd != DbCursorBase::SKIP_NONE) {
            key_buf_.set_flags(kflags);
            data_buf_.set_flags(dflags);
        }
    }
}; // DbCursor<>

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//
// RandDbCursor class template definition
//
// RandDbCursor is a random accessible cursor wrapper for use by
// db_vector_iterator, it derives from DbCursor<> class. It has a fixed key
// data type, which is index_type.
//
typedef db_recno_t index_type;
template<Typename data_dt>
class RandDbCursor : public DbCursor<index_type, data_dt>
{
protected:
    friend class DataItem;
    typedef ssize_t difference_type;
public:
    typedef RandDbCursor<data_dt> self;
    typedef DbCursor<index_type, data_dt> base;

    // Return current csr_ pointed element's index in recno database
    // (i.e. the index starting from 1). csr_ must be open and
    // point to an existing key/data pair.
    //
    inline index_type get_current_index() const
    {
        index_type ndx;

        if (this->directdb_get_)
            ((self *)this)->update_current_key_data_from_db(
                DbCursorBase::SKIP_DATA);
        this->curr_key_.get_data(ndx);
        return ndx;
    }

    inline int compare(const self *csr2) const{
        index_type i1, i2;

        i1 = this->get_current_index();
        i2 = csr2->get_current_index();
        return i1 - i2;
    }

    // Insert data d before/after current position.
    int insert(const data_dt& d, int pos = DB_BEFORE){
        int k = 1, ret;
        //data_dt dta;

        // Inserting into empty db, must set key to 1.
        if (pos == DB_KEYLAST)
            k = 1;

        ret = base::insert(k, d, pos);

        // Inserting into a empty db using begin() itr, so flag is
        // DB_AFTER and surely failed, so change to use DB_KEYLAST
        // and try again.
        if (ret == EINVAL) {
            k = 1;
            pos = DB_KEYLAST;
            ret = base::insert(k, d, pos);
        }
        this->csr_status_ = ret;
        return ret;
    }

    /*
     * Move the cursor n positions, if reaches the beginning or end,
     * returns DB_NOTFOUND.
     */
    int advance(difference_type n)
    {
        int ret = 0;
        index_type indx;
        u_int32_t sz, flags = 0;

        indx = this->get_current_index();
        if (n == 0)
            return 0;

        index_type i = (index_type)n;
        indx += i;

        if (n < 0 && indx < 1) { // Index in recno db starts from 1.

            ret = INVALID_ITERATOR_POSITION;
            return ret;
        }
        this->inform_duppers();

        // Do a search to determine whether new position is valid.
        Dbt k, &d = this->data_buf_;

        
        k.set_data(&indx);
        k.set_size(sizeof(indx));
        if (this->rmw_get_)
            flags |= DB_RMW;

retry:      if (this->csr_ && 
            ((ret = this->csr_->get(&k, &d, DB_SET)) == DB_NOTFOUND)) {
            this->csr_status_ = ret = INVALID_ITERATOR_POSITION;
            this->curr_key_.reset();
            this->curr_data_.reset();
        } else if (ret == DB_BUFFER_SMALL) {
            sz = d.get_size();
            assert(sz > 0);
            this->enlarge_dbt(d, sz);
            goto retry;
        } else if (ret == 0) {
            this->curr_key_.set_dbt(k, false);
            this->curr_data_.set_dbt(d, false);
            this->limit_buf_size_after_use();
        } else
            throw_bdb_exception("RandDbCursor<>::advance", ret);
        this->csr_status_ = ret;
        return ret;
    }

    // Return the last index of recno db (index starting from 1),
    // it will also move the underlying cursor to last key/data pair.
    //
    inline index_type last_index()
    {
        int ret;

        ret = this->last();
        if (ret)
            return 0;// Invalid position.
        else
            return get_current_index();
    }

    explicit RandDbCursor(u_int32_t b_bulk_retrieval = 0,
        bool b_rmw1 = false, bool directdbget = true)
        : base(b_bulk_retrieval, b_rmw1, directdbget)
    {
    }

    RandDbCursor(const RandDbCursor<data_dt>& rdbc) : base(rdbc)
    {
    }

    explicit RandDbCursor(Dbc* csr1, int posidx = 0) : base(csr1)
    {
    }

    virtual ~RandDbCursor()
    {
    }

}; // RandDbCursor<>

END_NS //ns dbstl

#endif // !_DB_STL_DBC_H
