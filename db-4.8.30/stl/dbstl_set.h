/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_DB_SET_H_
#define _DB_STL_DB_SET_H_


#include "dbstl_common.h"
#include "dbstl_map.h"
#include "dbstl_dbc.h"
#include "dbstl_container.h"
#include "dbstl_resource_manager.h"
#include "dbstl_element_ref.h"
#include "dbstl_base_iterator.h"

START_NS(dbstl)

using std::pair;
using std::make_pair;

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// _DB_STL_set_value class template definition
// This class is used for db_set as value type because it inherits from
// db_map . It dose not need a byte for DB_HASH to work, hash db can store
// duplicated keys with empty data.
//
template <Typename T>
class _DB_STL_set_value 
{
public:
    
    inline _DB_STL_set_value(const ElementRef<T>&){ }
    inline _DB_STL_set_value(const T&){}
    inline _DB_STL_set_value(){}
};

/** \ingroup dbstl_iterators
@{
\defgroup dbset_iterators Iterator classes for db_set and db_multiset.
db_set_base_iterator and db_set_iterator are the const iterator and iterator 
class for db_set and db_multiset. They have identical behaviors to 
std::set::const_iterator and std::set::iterator respectively. 

The difference between the two classes is that the db_set_base_iterator 
can only be used to read its referenced value, while db_set_iterator allows
both read and write access. If the access pattern is readonly, it is strongly
recommended that you use the const iterator because it is faster and more 
efficient.

The two classes inherit several functions from db_map_base_iterator and 
db_map_iterator respectively.
\sa db_map_base_iterator db_map_iterator

@{
*/

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// db_set_base_iterator class template definition
//
// db_set_base_iterator class template is the const iterator class for db_set
// and db_multiset, it can only be used to read data.
// This class need to override operator* and operator-> because the 
// db_set<>::value_type is different from db_map<>::value_type. We also has
// to copy the iterator movement operators into this class because the "self"
// type is different in db_map_base_iterator and this class.
// In db_set_base_iterator's inherited curpair_base_ pair, we store key to
// its first and second member, to work with db_map and db_multimap.
// Besides this, there is no further difference, so we can still safely make
// use of its inherited methods.
//
template <Typename kdt>
class _exported db_set_base_iterator : public 
    db_map_base_iterator<kdt, kdt, _DB_STL_set_value<kdt> >
{
protected:
    typedef db_set_base_iterator<kdt> self;
    typedef db_map_base_iterator<kdt, kdt, _DB_STL_set_value<kdt> > base;
    using db_base_iterator<kdt>::replace_current_key;
public:
    
    typedef kdt key_type;
    typedef _DB_STL_set_value<kdt> ddt;
    typedef kdt value_type;
    typedef value_type& reference;
    typedef value_type* pointer;
    typedef value_type value_type_wrap;
    typedef ptrdiff_t difference_type;
    typedef difference_type distance_type;
    // We have to use std iterator tags to match the parameter list of
    // stl internal functions, so we don't need to write our own tag 
    // classes
    //
    typedef std::bidirectional_iterator_tag iterator_category;

    ////////////////////////////////////////////////////////////////
    // Begin constructors and destructor definitions.
    //
    /// \name Constructors and destructor
    /// Do not use these constructors to create iterators, but call
    /// db_set::begin() const or db_multiset::begin() const to create 
    /// valid iterators.
    //@{
    /// Destructor.
    virtual ~db_set_base_iterator()
    {

    }

    /// Constructor.
    /// \param powner The container which creates this iterator.
    /// \param b_bulk_retrieval The bulk read buffer size. 0 means 
    /// bulk read disabled.
    /// \param brmw Whether set DB_RMW flag in underlying cursor.
    /// \param directdbget Whether do direct database get rather than
    /// using key/data values cached in the iterator whenever read.
    /// \param b_read_only Whether open a read only cursor. Only effective
    /// when using Berkeley DB Concurrent Data Store.
    explicit db_set_base_iterator(db_container*powner, 
        u_int32_t b_bulk_retrieval = 0, bool brmw = false, 
        bool directdbget = true, bool b_read_only = false)
        : base(powner, b_bulk_retrieval, brmw, directdbget, b_read_only)
    {
        this->is_set_ = true;
    }

    /// Default constructor, dose not create the cursor for now.
    db_set_base_iterator() : base()
    {
        this->is_set_ = true;
    }

    /// Copy constructor.
    /// \param s The other iterator of the same type to initialize this.
    db_set_base_iterator(const db_set_base_iterator&s) : base(s)
    {
        this->is_set_ = true;
    }

    /// Base copy constructor.
    /// \param bo Initialize from a base class iterator.
    db_set_base_iterator(const base& bo) : base(bo) 
    {
        this->is_set_ = true;
    }
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that shift iterator positions.
    /// \name Iterator movement operators.
    /// These functions are identical to those of db_map_base_iterator
    /// and db_map_iterator and db_set_iterator. Actually the iterator
    /// movement functions in the four classes are the same.
    //@{
    // But we have to copy them into the four classes because the 
    // return type, namely "self" is different in each class.
    /// Post-increment. 
    /// \return This iterator after incremented.
    /// \sa db_map_base_iterator::operator++()
    inline self& operator++()
    {
        this->next();
        
        return *this;
    }

    /// Pre-increment. 
    /// \return Another iterator having the old value of this iterator.
    /// \sa db_map_base_iterator::operator++(int)
    inline self operator++(int)
    {
        self itr = *this;

        this->next();
        
        return itr;
    }

    /// Post-decrement. 
    /// \return This iterator after decremented.
    /// \sa db_map_base_iterator::operator--()
    inline self& operator--() 
    {
        this->prev();
        
        return *this;
    }

    /// Pre-decrement. 
    /// \return Another iterator having the old value of this iterator.
    /// \sa db_map_base_iterator::operator--(int)
    self operator--(int)
    {
        self itr = *this;
        this->prev();
        
        return itr;
    }
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that retrieve values from iterator.
    //
    // This function returns a read only reference, you can only read
    // its referenced data, not update it.
    // curpair_base_ is always kept updated on iterator movement, but it
    // is also updated here before making use of the value it points
    // to, which is stored in curpair_base_ .
    // Even if this iterator is invalid, this call is allowed, the
    // default value of type T is returned.
    //
    /// \name Functions that retrieve values from iterator.
    //@{
    /// Dereference operator.
    /// Return the reference to the cached data element, which is an 
    /// object of type T. You can only use the return value to read 
    /// its referenced data element, can not update it.
    /// \return Current data element reference object, i.e. ElementHolder
    /// or ElementRef object.
    inline reference operator*()  
    { 
        int ret;

        if (this->directdb_get_) {
            ret = this->pcsr_->get_current_key(
                this->curpair_base_.first);
            dbstl_assert(ret == 0);
        // curpair_base_.second is a _DB_STL_set_value<kdt> object, 
        // not used at all. Since const iterators can't be used to 
        // write, so this is not a problem.
        }
        // Returning reference, no copy construction.
        return this->curpair_base_.first;
    }
     
    // curpair_base_ is always kept updated on iterator movement, but it
    // is also updated here before making use of the value it points
    // to, which is stored in curpair_base_ .
    // Even if this iterator is invalid, this call is allowed, the
    // default value of type T is returned.
    /// Arrow operator.
    /// Return the pointer to the cached data element, which is an 
    /// object of type T. You can only use the return value to read 
    /// its referenced data element, can not update it.
    /// \return Current data element reference object's address, i.e. 
    /// address of ElementHolder or ElementRef object.
    inline pointer operator->() const
    {
        int ret;

        if (this->directdb_get_) {
            ret = this->pcsr_->get_current_key(
                this->curpair_base_.first);
            dbstl_assert(ret == 0);
        }

        return &(this->curpair_base_.first);
    }
    //@}
    ////////////////////////////////////////////////////////////////

    /// \brief Refresh iterator cached value.
    /// \param from_db If not doing direct database get and this parameter
    /// is true, we will retrieve data directly from db.
    /// \sa db_base_iterator::refresh(bool) 
    virtual int refresh(bool from_db = true) const
    {
        
        if (from_db && !this->directdb_get_)
            this->pcsr_->update_current_key_data_from_db(
                DbCursorBase::SKIP_NONE);
        this->pcsr_->get_current_key(this->curpair_base_.first);
        this->curpair_base_.second = this->curpair_base_.first;
        return 0;
    }

protected:

    // Declare friend classes to access protected/private members,
    // while expose to outside only methods in stl specifications.
    //
    friend class db_set<kdt>;
    
    friend class db_map<kdt, _DB_STL_set_value<kdt>, 
        ElementHolder<kdt>, self>;
    friend class db_map<kdt, _DB_STL_set_value<kdt>, 
        ElementRef<kdt>, self>;

    typedef pair<kdt, value_type> curpair_type;

}; // db_set_base_iterator


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// db_set_iterator class template definition
//
// db_set_iterator class template is the iterator class for db_set and 
// db_multiset, it can be used to update its referenced key/data pair.
// This class need to override operator* and operator-> because the 
// db_set<>::value_type is different from db_map<>::value_type. besides
// this, there is no further difference, so we can still safely make
// use of db_set inherited methods.
//
template <Typename kdt, Typename value_type_sub>
class _exported db_set_iterator : 
    public db_map_iterator<kdt, _DB_STL_set_value<kdt>, value_type_sub>
{
protected:
    typedef db_set_iterator<kdt, value_type_sub> self;
    typedef db_map_iterator<kdt, _DB_STL_set_value<kdt>, 
        value_type_sub> base;
    
public:
    
    typedef kdt key_type;
    typedef _DB_STL_set_value<kdt> ddt;
    typedef kdt value_type;
    typedef value_type_sub value_type_wrap;
    typedef value_type_sub& reference;
    typedef value_type_sub* pointer;
    typedef ptrdiff_t difference_type;
    typedef difference_type distance_type;
    // We have to use std iterator tags to match the parameter list of
    // stl internal functions, so we don't need to write our own tag 
    // classes
    //
    typedef std::bidirectional_iterator_tag iterator_category;
    
    ////////////////////////////////////////////////////////////////
    // Begin constructors and destructor definitions.
    /// \name Constructors and destructor
    /// Do not use these constructors to create iterators, but call
    /// db_set::begin() or db_multiset::begin() to create valid ones.
    //@{
    /// Destructor.
    virtual ~db_set_iterator()
    {

    }

    /// Constructor.
    /// \param powner The container which creates this iterator.
    /// \param b_bulk_retrieval The bulk read buffer size. 0 means 
    /// bulk read disabled.
    /// \param brmw Whether set DB_RMW flag in underlying cursor.
    /// \param directdbget Whether do direct database get rather than
    /// using key/data values cached in the iterator whenever read.
    /// \param b_read_only Whether open a read only cursor. Only effective
    /// when using Berkeley DB Concurrent Data Store.
    explicit db_set_iterator(db_container*powner, 
        u_int32_t b_bulk_retrieval = 0, bool brmw = false, 
        bool directdbget = true, bool b_read_only = false)
        : base(powner, b_bulk_retrieval, brmw, directdbget, b_read_only)
    {
        this->is_set_ = true;
    }

    /// Default constructor, dose not create the cursor for now.
    db_set_iterator() : base()
    {
        this->is_set_ = true;
    }

    /// Copy constructor.
    /// \param s The other iterator of the same type to initialize this.
    db_set_iterator(const db_set_iterator&s) : base(s)
    {
        this->is_set_ = true;
    }

    /// Base copy constructor.
    /// \param bo Initialize from a base class iterator.
    db_set_iterator(const base& bo) : base(bo) 
    {
        this->is_set_ = true;
    }

    /// Sibling copy constructor.
    /// Note that this class does not derive from db_set_base_iterator
    /// but from db_map_iterator.
    /// \param bs Initialize from a base class iterator.
    db_set_iterator(const db_set_base_iterator<kdt>&bs) : base(bs)
    {
        this->is_set_ = true;
    }
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that shift iterator positions.
    /// \name Iterator movement operators.
    //@{
    /// Pre-increment.
    /// Identical to those of db_map_iterator.
    /// \return This iterator after incremented.
    /// \sa db_map_iterator::operator++()
    inline self& operator++()
    {
        this->next();
        
        return *this;
    }

    /// Post-increment.
    /// \return Another iterator having the old value of this iterator.
    /// \sa db_map_iterator::operator++(int)
    inline self operator++(int)
    {
        self itr = *this;

        this->next();
        
        return itr;
    }

    /// Pre-decrement.
    /// \return This iterator after decremented.
    /// \sa db_map_iterator::operator--()
    inline  self& operator--() 
    {
        this->prev();
        
        return *this;
    }

    /// Post-decrement 
    /// \return Another iterator having the old value of this iterator.
    /// \sa db_map_iterator::operator--(int)
    self operator--(int)
    {
        self itr = *this;
        this->prev();
        
        return itr;
    }
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that retrieve values from iterator.
    //
    // This functions returns a read-write reference to its data element.
    // Even if this iterator is invalid, this call is allowed, the
    // default value of type T is returned.
    /// \name Functions that retrieve values from iterator.
    //@{
    /// Dereference operator.
    /// Return the reference to the cached data element, which is an 
    /// ElementRef<T> object if T is a class type or an ElementHolder<T>
    /// object if T is a C++ primitive data type.
    /// \return Current data element reference object, i.e. ElementHolder
    /// or ElementRef object.
    inline reference operator*()  
    { 

        if (this->directdb_get_) 
            refresh(true);
        // returning reference, no copy construction
        return this->curpair_.second;
    }
     
    // curpair_ is always kept updated on iterator movement, but it
    // is also updated here before making use of the value it points
    // to, which is stored in curpair_ .
    // Even if this iterator is invalid, this call is allowed, the
    // default value of type T is returned.
    /// Arrow operator.
    /// Return the pointer to the cached data element, which is an 
    /// ElementRef<T> object if T is a class type or an ElementHolder<T>
    /// object if T is a C++ primitive data type.
    /// \return Current data element reference object's address, i.e. 
    /// address of ElementHolder or ElementRef object.
    inline pointer operator->() const
    {

        if (this->directdb_get_) 
            refresh(true);

        return &(this->curpair_.second);
    }
    //@}
    ////////////////////////////////////////////////////////////////

    /// \brief Refresh iterator cached value.
    /// \param from_db If not doing direct database get and this parameter
    /// is true, we will retrieve data directly from db.
    /// \sa db_base_iterator::refresh(bool) 
    virtual int refresh(bool from_db = true) const
    {
        
        if (from_db && !this->directdb_get_)
            this->pcsr_->update_current_key_data_from_db(
                DbCursorBase::SKIP_NONE);
        this->pcsr_->get_current_key(this->curpair_.first);
        this->curpair_.second._DB_STL_CopyData(this->curpair_.first);
        this->set_curpair_base(this->curpair_.first, this->curpair_.first);
        return 0;
    }

protected:
    typedef pair<kdt, value_type_sub> curpair_type;
    typedef db_set_base_iterator<kdt> const_version;
    // Declare friend classes to access protected/private members,
    // while expose to outside only methods in stl specifications.
    //
    friend class db_set<kdt, ElementHolder<kdt> >;
    friend class db_set<kdt, ElementRef<kdt> >;
    friend class db_multiset<kdt, ElementHolder<kdt> >;
    friend class db_multiset<kdt, ElementRef<kdt> >;
    friend class db_map<kdt, _DB_STL_set_value<kdt>, 
        ElementHolder<kdt>, self>;
    friend class db_multimap<kdt, _DB_STL_set_value<kdt>, 
        ElementHolder<kdt>, self>;
    friend class db_map<kdt, _DB_STL_set_value<kdt>, 
        ElementRef<kdt>, self>;
    friend class db_multimap<kdt, _DB_STL_set_value<kdt>, 
        ElementRef<kdt>, self>;
    
    virtual void delete_me() const
    {
        delete this;
    }

}; // db_set_iterator
//@} // dbset_iterators
//@} // dbstl_iterators

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// db_set class template definition
//
// db_set<> inherits from db_map<>, storing nothing data, because
// we only need the key. It uses db_set_iterator<>/db_set_base_iterator
// as its iterator and const iterator respectively.
// The only difference between db_set<> and db_map<> classes
// is the value_type, so we redefine the insert function by directly copying
// the db_map<>::insert, in order to make use of the newly defined 
// value_type of db_set. If typedef could be dynamically bound, we won't
// have to make this duplicated effort.
/// \ingroup dbstl_containers
/// This class is the combination of std::set and hash_set. By setting 
/// database handles of DB_BTREE or DB_HASH type, you will be using the 
/// equivalent of std::set or hash_set. This container stores the key in the
/// key element of a key/data pair in the underlying database, 
/// but doesn't store anything in the data element.
/// Database and environment handle requirement:
/// The same as that of db_map.
/// \param kdt The key data type.
/// \param value_type_sub If kdt is a class/struct type, do not specify 
/// anything in this parameter; Otherwise specify ElementHolder<kdt>.
/// \sa db_map db_container
//
template <Typename kdt, Typename value_type_sub>
class _exported db_set : public db_map<kdt, _DB_STL_set_value<kdt>, 
    value_type_sub, db_set_iterator<kdt, value_type_sub> > 
{
protected:
    typedef db_set<kdt, value_type_sub> self;
public:
    typedef db_set_iterator<kdt, value_type_sub> iterator;
    typedef typename iterator::const_version const_iterator;
    typedef db_reverse_iterator<iterator, const_iterator> reverse_iterator;
    typedef db_reverse_iterator<const_iterator, iterator> 
        const_reverse_iterator;
    typedef kdt key_type;
    typedef ptrdiff_t difference_type;
    // the ElementRef should store key value because key is also data, 
    // and *itr is key and data value
    //
    typedef kdt value_type; 
    typedef value_type_sub value_type_wrap;
    typedef value_type_sub* pointer;
    typedef value_type_sub& reference;
    typedef value_type& const_reference;
    typedef size_t size_type;

    ////////////////////////////////////////////////////////////////
    // Begin constructors and destructor.
    /// \name Constructors and destructor
    //@{
    /// Create a std::set/hash_set equivalent associative container.
    /// See the handle requirement in class details to pass correct 
    /// database/environment handles.
    /// \param dbp The database handle.
    /// \param envp The database environment handle.
    /// \sa db_map(Db*, DbEnv*) db_container(Db*, DbEnv*)
    explicit db_set (Db *dbp = NULL, DbEnv* envp = NULL) : base(dbp, envp){
        // There are some special handling in db_map_iterator<> 
        // if owner is set.
        this->set_is_set(true);
    }

    /// Iteration constructor. Iterates between first and last, 
    /// copying each of the elements in the range into 
    /// this container. 
    /// Create a std::set/hash_set equivalent associative container.
    /// Insert a range of elements into the database. The range is
    /// [first, last), which contains elements that can
    /// be converted to type ddt automatically.
    /// This function supports auto-commit.
    /// See the handle requirement in class details to pass correct
    /// database/environment handles.
    /// \param dbp The database handle.
    /// \param envp The database environment handle.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    /// \sa db_map(Db*, DbEnv*, InputIterator, InputIterator)
    template <class InputIterator> 
    db_set (Db *dbp, DbEnv* envp, InputIterator first,
        InputIterator last) : base(*(new BulkRetrievalOption(
        BulkRetrievalOption::BulkRetrieval)))
    {

        const char *errmsg;
        
        this->init_members(dbp, envp);
        this->open_db_handles(dbp, envp, DB_BTREE, 
            DB_CREATE | DB_THREAD, 0);
        if ((errmsg = this->verify_config(dbp, envp)) != NULL) {
            THROW(InvalidArgumentException, ("Db*", errmsg));
        }
        this->set_db_handle_int(dbp, envp);
        this->set_auto_commit(dbp);
        this->begin_txn();
        try {
            insert(first, last);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
        // There are some special handling in db_map_iterator<> 
        // if owner is set.
        this->set_is_set(true);

    }

    /// Copy constructor.
    /// Create a database and insert all key/data pairs in x into this
    /// container. x's data members are not copied.
    /// This function supports auto-commit.
    /// \param x The source container to initialize this container.
    /// \sa db_map(const db_map&) db_container(const db_container&)
    db_set(const self& x) : base(*(new BulkRetrievalOption(
        BulkRetrievalOption::BulkRetrieval))) 
    {
        this->init_members(x);
        verify_db_handles(x);
        this->set_db_handle_int(this->clone_db_config(
            x.get_db_handle()), x.get_db_env_handle());
        assert(this->get_db_handle() != NULL);
        
        this->begin_txn();
        try {
            copy_db((self&)x);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
        // There are some special handling in db_map_iterator<> 
        // if owner is set.
        this->set_is_set(true);
    }

    virtual ~db_set(){}
    //@}
    ////////////////////////////////////////////////////////////////

    /// Container content assignment operator.
    /// This function supports auto-commit.
    /// \param x The source container whose key/data pairs will be 
    /// inserted into the target container. Old content in the target
    /// container is discarded.
    /// \return The container x's reference.
    /// \sa http://www.cplusplus.com/reference/stl/set/operator=/
    const self& operator=(const self& x)
    {
        ASSIGNMENT_PREDCOND(x)
        db_container::operator =(x);
        verify_db_handles(x);
        assert(this->get_db_handle() != NULL);
        this->begin_txn();
        try {
            this->copy_db((self &)x);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
        return x;
    }
    
    ////////////////////////////////////////////////////////////////
    // Begin key/value compare classes/functions.
    //
    // key_compare class definition, it is defined as an inner class,
    // using underlying btree/hash db's compare function. It is the same
    // as that of db_map<>, but because we have to redefine value_compare
    // in this class, gcc forces us to also define key_compare again.
    class key_compare 
    {
    private:
        Db*pdb;
    public:
        key_compare(Db*pdb1)
        {
            pdb = pdb1;
        }
        bool operator()(const kdt& k1, const kdt& k2) const
        {
            return compare_keys(pdb, k1, k2);
        }

    }; // key_compare class definition
    
    class value_compare 
    {
        key_compare kc;
    public:
        value_compare(Db*pdb) : kc(pdb)
        {
        
        }

        bool operator()(const value_type& v1, 
            const value_type& v2) const
        {
            
            return kc(v1, v2);
        }

    }; // value_compare class definition

    /// Get value comparison functor.
    /// \return The value comparison functor.
    /// \sa http://www.cplusplus.com/reference/stl/set/value_comp/
    inline value_compare value_comp() const
    {
        return value_compare(this->get_db_handle());
    }
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin insert functions and swap function.
    //
    // Note that when secondary index is enabled, each db_container
    // can create a db_multimap secondary container, but the insert 
    // function is not functional.
    //
    // Insert functions. Note that stl requires if the entry with x.key 
    // already exists, insert should not overwrite that entry and the 
    // insert should fail; but bdb Dbc::cursor(DB_KEYLAST) will replace
    // existing data with new one, so we will first find whether we
    // have this data, if have, return false;
    /// \name Insert Functions
    /// \sa http://www.cplusplus.com/reference/stl/set/insert/
    //@{
    /// Insert a single key/data pair if the key is not in the container.
    /// \param x The key/data pair to insert.
    /// \return A pair P, if insert OK, i.e. the inserted key wasn't in the
    /// container, P.first will be the iterator positioned on the inserted
    /// key/data pair, and P.second is true; otherwise P.first is an invalid 
    /// iterator equal to that returned by end() and P.second is false.
    pair<iterator,bool> insert (const value_type& x ) 
    {
        pair<iterator,bool> ib;

        _DB_STL_set_value<kdt> sv;
        iterator witr;

        this->init_itr(witr);
        this->open_itr(witr);

        if (witr.move_to(x) == 0) {// has it
            ib.first = witr;
            ib.second = false;

            return ib;
        }
        witr.itr_status_ = witr.pcsr_->insert(x, sv, DB_KEYLAST); 
        witr.refresh(false);
        ib.first = witr;
        ib.second = true;

        return ib;
    }

    // NOT_AUTOCOMMIT_TAG  
    // Note that if first and last are on same db as this container, 
    // then insert may deadlock if there is no common transaction context 
    // for first and witr (when they are on the same page).
    // The insert function in base class can not be directly used, 
    // got compile errors.
    /// Range insertion. Insert a range [first, last) of key/data pairs
    /// into this container.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    inline void insert (const_iterator& first, const_iterator& last) 
    {
        const_iterator ii;
        _DB_STL_set_value<kdt> v;
        iterator witr;

        this->init_itr(witr);
        
        this->open_itr(witr);
    
        for (ii = first; ii != last; ++ii) 
            witr.pcsr_->insert(*ii, v, DB_KEYLAST);
    }

    /// Range insertion. Insert a range [first, last) of key/data pairs
    /// into this container.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    void insert (iterator& first, iterator& last) 
    {
        iterator ii, witr;
        _DB_STL_set_value<kdt> d;

        init_itr(witr);
        open_itr(witr);
    
        for (ii = first; ii != last; ++ii)
            witr.pcsr_->insert(*ii, d, DB_KEYLAST);
        
    }

    // Ignore the position parameter because bdb knows better
    // where to insert.
    /// Insert with hint position. We ignore the hint position because 
    /// Berkeley DB knows better where to insert.
    /// \param position The hint position.
    /// \param x The key/data pair to insert.
    /// \return The iterator positioned on the inserted key/data pair, 
    /// or an invalid iterator if the key was already in the container.
    inline iterator insert ( iterator position, const value_type& x ) 
    {
        pair<iterator,bool> ib = insert(x);
        return ib.first;
    }

    /// Range insertion. Insert a range [first, last) of key/data pairs
    /// into this container.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    template <class InputIterator>
    void insert (InputIterator first, InputIterator last) 
    {
        InputIterator ii;
        _DB_STL_set_value<kdt> v;
        iterator witr;

        this->init_itr(witr);
    
        this->open_itr(witr);
    
        for (ii = first; ii != last; ++ii) 
            witr.pcsr_->insert(*ii, v, DB_KEYLAST);

    }
    //@}

    /// Swap content with another container.
    /// This function supports auto-commit.
    /// \param mp The container to swap content with.
    /// \param b_truncate See db_vector::swap's b_truncate parameter
    /// for details.
    /// \sa db_map::swap() db_vector::clear()
    void swap (db_set<kdt, value_type_sub>& mp, bool b_truncate = true)
    {   
        Db *swapdb = NULL;
        std::string dbfname(64, '\0');

        verify_db_handles(mp);
        this->begin_txn();
        try {
            swapdb = this->clone_db_config(this->get_db_handle(), 
                dbfname);
            
            db_set<kdt, value_type_sub> tmap(swapdb, 
                swapdb->get_env(), this->begin(), this->end());
            typename db_set<kdt, value_type_sub>::
                iterator itr1, itr2;

            this->clear(b_truncate);// Clear this db_map<> object.
            itr1 = mp.begin();
            itr2 = mp.end();
            this->insert(itr1, itr2);
            mp.clear(b_truncate);
            itr1 = tmap.begin();
            itr2 = tmap.end();
            mp.insert(itr1, itr2);
            // tmap has no opened cursor, so simply truncate.
            tmap.clear();
            
            swapdb->close(0);
            if (dbfname[0] != '\0') {
                swapdb = new Db(NULL, DB_CXX_NO_EXCEPTIONS);
                swapdb->remove(dbfname.c_str(), NULL, 0);
                swapdb->close(0);
                delete swapdb;
            }
            this->commit_txn();
        } catch (...) {
            this->abort_txn();
            throw;
        }   
        
    }
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin container comparison functions.
    //
    // Return true if contents in m1 and m2 are identical otherwise 
    // return false. 
    // 
    // Note that we don't require the keys' order be identical, we are
    // doing mathmatical set comparisons.
    /// Set content equality comparison operator.
    /// Return if the two containers have identical content. This function
    /// does not rely on key order.
    /// Two sets A and B are equal if and only if A contains B and B 
    /// contains A.
    /// \param m2 The container to compare against.
    /// \return Returns true if the two containers are equal, 
    /// false otherwise.
    //
    bool operator==(const db_set<kdt, value_type_sub>& m2) const
    {
        bool ret;
        
        COMPARE_CHECK(m2)
        verify_db_handles(m2);
        const db_set<kdt, value_type_sub>& m1 = *this;

        try {
            this->begin_txn();
            if (m1.size() != m2.size())
                ret = false;
            else {
                typename db_set<kdt, value_type_sub>::
                    const_iterator i1;

                for (i1 = m1.begin(); i1 != m1.end(); ++i1) {
                    if (m2.count(*i1) == 0) {
                        ret = false;
                        goto exit;
                    }
                        
                } // for
                ret = true;
            } // else
exit:
            this->commit_txn();
    // Now that m1 and m2 has the same number of unique elements and 
    // all elements of m1 are in m2, thus there can be no element of m2 
    // that dose not belong to m1, so we won't verify each element of 
    // m2 are in m1.
            return ret;
        } catch (...) {
            this->abort_txn();
            throw;
        }
    }// operator==

    /// Inequality comparison operator.
    bool operator!=(const db_set<kdt, value_type_sub>& m2) const
    {
        return !this->operator==(m2);
    }
    ////////////////////////////////////////////////////////////////
    
protected:
    typedef int (*db_compare_fcn_t)(Db *db, const Dbt *dbt1, 
        const Dbt *dbt2);
    
    
    typedef db_map<kdt, _DB_STL_set_value<kdt>, value_type_sub,
        db_set_iterator<kdt, value_type_sub> > base;
private:

    value_type_sub operator[] (const key_type& x)
    {
        THROW(NotSupportedException, ("db_set<>::operator[]"));
        
    }

    value_type_sub operator[] (const key_type& x) const
    {
        THROW(NotSupportedException, ("db_set<>::operator[]"));
        
    }

    inline void copy_db(db_set<kdt, value_type_sub> &x)
    {    

        // Make sure clear can succeed if there are cursors 
        // open in other threads.
        this->clear(false);
        insert(x.begin(), x.end());
        
    }

}; // db_set<>


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// db_multiset class template definition
//
// db_multiset<> inherits from db_multimap, storing nothing as data, because
// we only need the key. It uses db_set_iterator<> and db_set_base_iterator<>
// as its iterator and const iterator respectively, so that
// we can safely make use of the inherited methods. The only difference
// is the value_type, so we redefine the insert functions by directly copying
// the db_map<>::insert versions, in order to make use of the newly defined 
// value_type of db_multiset. If typedef could be dynamically bound, we won't
// have to make this duplicated effort.
/// \ingroup dbstl_containers
/// This class is the combination of std::multiset and hash_multiset. By 
/// setting database handles of DB_BTREE or DB_HASH type respectively, you 
/// will be using the equivalent of std::multiset or hash_multiset respectively.
/// This container stores the key in the key element of a key/data pair in 
/// the underlying database, but doesn't store anything in the data element.
/// Database and environment handle requirement:
/// The requirement to these handles is the same as that to db_multimap.
/// \param kdt The key data type.
/// \param value_type_sub If kdt is a class/struct type, do not specify 
/// anything in this parameter; Otherwise specify ElementHolder<kdt>.
/// \sa db_multimap db_map db_container db_set
//
template <Typename kdt, Typename value_type_sub>
class _exported db_multiset : public db_multimap<kdt, _DB_STL_set_value<kdt>, 
    value_type_sub, db_set_iterator<kdt, value_type_sub> >
{
protected:
    typedef db_multiset<kdt, value_type_sub> self;
public:
    typedef db_set_iterator<kdt, value_type_sub> iterator;
    typedef typename iterator::const_version const_iterator;
    typedef db_reverse_iterator<iterator, const_iterator> reverse_iterator;
    typedef db_reverse_iterator<const_iterator, iterator> 
        const_reverse_iterator;
    typedef kdt key_type;
    typedef ptrdiff_t difference_type;
    // The ElementRef should store key value because key is also data, 
    // and *itr is key and data value.
    //
    typedef kdt value_type; 
    typedef value_type_sub value_type_wrap;
    typedef value_type_sub& reference;
    typedef const value_type& const_reference;
    typedef value_type_sub* pointer;
    typedef size_t size_type;

public:
    ////////////////////////////////////////////////////////////////
    // Begin constructors and destructor.
    /// \name Constructors and destructor
    //@{
    /// Create a std::multiset/hash_multiset equivalent associative 
    /// container.
    /// See the handle requirement in class details to pass correct 
    /// database/environment handles.
    /// \param dbp The database handle.
    /// \param envp The database environment handle.
    /// \sa db_multimap(Db*, DbEnv*)
    explicit db_multiset (Db *dbp = NULL, DbEnv* envp = NULL) : 
        base(dbp, envp) {
        // There are special handling in db_map_iterator<> if owner 
        // is a set.
        this->set_is_set(true);
    }

    /// Iteration constructor. Iterates between first and last, 
    /// copying each of the elements in the range into 
    /// this container. 
    /// Create a std::multi/hash_multiset equivalent associative container.
    /// Insert a range of elements into the database. The range is
    /// [first, last), which contains elements that can
    /// be converted to type ddt automatically.
    /// This function supports auto-commit.
    /// See the handle requirement in class details to pass correct
    /// database/environment handles.
    /// \param dbp The database handle.
    /// \param envp The database environment handle.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    /// \sa db_multimap(Db*, DbEnv*, InputIterator, InputIterator)
    template <class InputIterator> 
    db_multiset (Db *dbp, DbEnv* envp, InputIterator first, 
        InputIterator last) : base(*(new BulkRetrievalOption(
        BulkRetrievalOption::BulkRetrieval)))
    {

        const char *errmsg;

        this->init_members(dbp, envp);
        this->open_db_handles(dbp, envp, DB_BTREE, DB_CREATE | 
            DB_THREAD, DB_DUP);
        // Note that we can't call base(dbp, envp) here because it 
        // will verify failed; And we can't call db_container 
        // directly because it is illegal to do so.
        if ((errmsg = verify_config(dbp, envp)) != NULL) {
            THROW(InvalidArgumentException, ("Db*", errmsg));
            
        }
        this->set_db_handle_int(dbp, envp);
        this->set_auto_commit(dbp);

        this->begin_txn();
        try {
            insert(first, last);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();

        // There are special handling in db_map_iterator<> if owner 
        // is a set.
        this->set_is_set(true);

    }

    /// Copy constructor.
    /// Create a database and insert all key/data pairs in x into this
    /// container. x's data members are not copied.
    /// This function supports auto-commit.
    /// \param x The source container to initialize this container.
    /// \sa db_multimap(const db_multimap&) db_container(const db_container&)
    db_multiset(const self& x) : base(*(new BulkRetrievalOption(
        BulkRetrievalOption::BulkRetrieval))) 
    {
        this->init_members(x);
        verify_db_handles(x);
        this->set_db_handle_int(this->clone_db_config(
            x.get_db_handle()), x.get_db_env_handle());
        assert(this->get_db_handle() != NULL);

        this->begin_txn();
        try {
            copy_db((self&)x);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();

        // There are special handling in db_map_iterator<> if owner 
        // is a set.
        this->set_is_set(true);
    }

    virtual ~db_multiset(){}
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that modify the container's content, i.e. insert,
    // erase, assignment and swap functions.
    //
    /// Container content assignment operator.
    /// This function supports auto-commit.
    /// \param x The source container whose key/data pairs will be 
    /// inserted into the target container. Old content in the target 
    /// container is discarded.
    /// \return The container x's reference.
    /// \sa http://www.cplusplus.com/reference/stl/multiset/operator=/
    inline const self& operator=(const self& x)
    {
        ASSIGNMENT_PREDCOND(x)
        db_container::operator =(x);
        verify_db_handles(x);
        assert(this->get_db_handle() != NULL);
        this->begin_txn();
        try {
            this->copy_db((self &)x);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
        return x;
    }

    // Note that when secondary index is enabled, each db_container
    // can create a db_multimap secondary container, but the insert 
    // function is not functional.
    /// \name Insert Functions
    /// \sa http://www.cplusplus.com/reference/stl/multiset/insert/
    //@{
    /// Insert a single key if the key is not in the container.
    /// \param x The key to insert.
    /// \return An iterator positioned on the newly inserted key. If the
    /// key x already exists, an invalid iterator equal to that returned 
    /// by end() function is returned.
    inline iterator insert(const value_type& x ) 
    {
        pair<iterator,bool> ib;
        _DB_STL_set_value<kdt> sv;
        iterator witr;

        this->init_itr(witr);
        this->open_itr(witr);

        witr.itr_status_ = witr.pcsr_->insert(x, sv, DB_KEYLAST);
        witr.refresh(false);
        return witr;
    }

    // Ignore the position parameter because bdb knows better
    // where to insert.
    /// Insert a single key with hint if the key is not in the container.
    /// The hint position is ignored because Berkeley DB controls where
    /// to insert the key.
    /// \param x The key to insert.
    /// \param position The hint insert position, ignored.
    /// \return An iterator positioned on the newly inserted key. If the
    /// key x already exists, an invalid iterator equal to that returned 
    /// by end() function is returned.
    inline iterator insert ( iterator position, const value_type& x ) 
    {
        return insert(x);
    }
    
    /// Range insertion. Insert a range [first, last) of key/data pairs
    /// into this container.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    template <class InputIterator>
    void insert (InputIterator first, InputIterator last) 
    {
        InputIterator ii;
        _DB_STL_set_value<kdt> v;

        iterator witr;

        this->init_itr(witr);   
        this->open_itr(witr);
        for (ii = first; ii != last; ++ii) 
            witr.pcsr_->insert(*ii, v, DB_KEYLAST);

    }

    // Member function template overload.
    // This function is a specialization for the member template version
    // dedicated to db_set<>.
    //
    /// Range insertion. Insert a range [first, last) of key/data pairs
    /// into this container.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    void insert (db_set_iterator<kdt, value_type_sub>& first, 
        db_set_iterator<kdt, value_type_sub>& last) 
    {
        db_set_iterator<kdt, value_type_sub> ii;
        iterator witr;
        _DB_STL_set_value<kdt> d;

        init_itr(witr);
        open_itr(witr);
    
        for (ii = first; ii != last; ++ii)
            witr.pcsr_->insert(*ii, d, DB_KEYLAST);
        

    }

    // Member function template overload.
    // This function is a specialization for the member template version
    // dedicated to db_set<>.
    //
    /// Range insertion. Insert a range [first, last) of key/data pairs
    /// into this container.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    void insert (db_set_base_iterator<kdt>& first, 
        db_set_base_iterator<kdt>& last) 
    {
        db_set_base_iterator<kdt> ii;
        iterator witr;
        _DB_STL_set_value<kdt> d;

        init_itr(witr);
        open_itr(witr);
    
        for (ii = first; ii != last; ++ii)
            witr.pcsr_->insert(*ii, d, DB_KEYLAST);
    }
    //@}

    // There is identical function in db_multimap<> and db_multiset
    // for this function, we MUST keep the code consistent on update!
    // Go to db_multimap<>::erase(const key_type&) to see why.
    /// \name Erase Functions
    /// \sa http://www.cplusplus.com/reference/stl/multiset/erase/
    //@{
    /// Erase elements by key.
    /// All key/data pairs with specified key x will be removed from 
    /// the underlying database.
    /// This function supports auto-commit.
    /// \param x The key to remove from the container.
    /// \return The number of key/data pairs removed.
    size_type erase (const key_type& x) 
    {
        size_type cnt;
        iterator itr;

        this->begin_txn();
        try {
            pair<iterator, iterator> rg = equal_range(x);
            for (itr = rg.first, cnt = 0; 
                itr != rg.second; ++itr) {
                cnt++;
                itr.pcsr_->del();
            }
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
        return cnt;
    }

    // Can not reopen external/outside iterator's cursor, pos must
    // already have been in a transactional context.
    // There is identical function in db_multimap<> and db_multiset
    // for this function, we MUST keep the code consistent on update!
    // Go to db_multimap<>::erase(const key_type&) to see why.
    //
    /// Erase a key/data pair at specified position.
    /// \param pos A valid iterator of this container to erase.
    inline void erase (iterator pos) 
    {
        if (pos == this->end())
            return;
        pos.pcsr_->del();
    }

    // Can not be auto commit because first and last are already open.
    // There is identical function in db_multimap<> and db_multiset
    // for this function, we MUST keep the code consistent on update!
    // Go to db_multimap<>::erase(const key_type&) to see why.
    //
    /// Range erase. Erase all key/data pairs within the valid range 
    /// [first, last).
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    inline void erase (iterator first, iterator last) 
    {
        iterator i;

        for (i = first; i != last; ++i)
            i.pcsr_->del();
    }
    //@}

    /// Swap content with another container.
    /// This function supports auto-commit.
    /// \param mp The container to swap content with.
    /// \param b_truncate See db_multimap::swap() for details.
    /// \sa db_map::swap() db_vector::clear()
    void swap (db_multiset<kdt, value_type_sub>& mp, bool b_truncate = true)
    {   
        Db *swapdb = NULL;
        std::string dbfname(64, '\0');

        verify_db_handles(mp);
        this->begin_txn();
        try {
            swapdb = this->clone_db_config(this->get_db_handle(), 
                dbfname);
            
            db_multiset<kdt, value_type_sub> tmap(swapdb, 
                swapdb->get_env(), this->begin(), this->end());
            this->clear(b_truncate);// Clear this db_map<> object.
            typename db_multiset<kdt, value_type_sub>::
                iterator itr1, itr2;
            itr1 = mp.begin();
            itr2 = mp.end();
            this->insert(itr1, itr2);
            mp.clear(b_truncate);
            itr1 = tmap.begin();
            itr2 = tmap.end();
            mp.insert(itr1, itr2);
            
            tmap.clear();
            
            swapdb->close(0);
            if (dbfname[0] != '\0') {
                swapdb = new Db(NULL, DB_CXX_NO_EXCEPTIONS);
                swapdb->remove(dbfname.c_str(), NULL, 0);
                swapdb->close(0);
                delete swapdb;
            }
            this->commit_txn();
            
        } catch (...) {
            swapdb->close(0);
            this->abort_txn();
            throw;
        }   
        
    }
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin container comparison functions.
    //
    // Compare two db_multiset containers for equality, containers
    // containing identical set of keys are considered equal, keys'
    // order are not presumed or relied upon by this comparison.
    /// Container content equality compare operator.
    /// This function does not rely on key order.
    /// Two sets A and B are equal if and only if
    /// for each and every key K having n occurrences in A, K has n 
    /// occurrences in B, and for each and every key K` having N
    /// occurrences in B, K` has n occurrences in A.
    /// \param m2 The container to compare against.
    /// \return Returns true if the two containers are equal, 
    /// false otherwise.
    bool operator==(const self& m2) const
    {
        COMPARE_CHECK(m2)
        verify_db_handles(m2);

        const db_multiset<kdt, value_type_sub> &m1 = *this;
        const_iterator i1, i11;
        pair<const_iterator, const_iterator> resrg1, resrg2;
        size_t n1, n2;
        bool ret = false;

        try {
            this->begin_txn();
            if (m1.size() != m2.size())
                ret = false;
            else {
                for (i1 = m1.begin(); i1 != m1.end(); ) {
                    resrg1 = m1.equal_range_N(*i1, n1);
                    resrg2 = m2.equal_range_N(*i1, n2);
                    if (n1 != n2) {
                        ret = false;
                        goto exit;
                    }
                        
            // If both is 1, resrg2 may contain no i1->first.
                    if (n2 == 1 && !(*(resrg2.first) == 
                        *(resrg1.first))) {
                        ret = false;
                        goto exit;
                    }
            // m1 and m2 contains identical set of i1->first key,
            // so move on, skip all equal keys in the range.
            //
                    i1 = resrg1.second; 
                        
                } // for
                ret = true;

            }// else
exit:
            this->commit_txn();
            return ret;
        } catch (...) {
            this->abort_txn();
            throw;
        }

    // Now that m1 and m2 has the same number of unique elements and all 
    // elements of m1 are in m2, thus there can be no element of m2 that 
    // dose not belong to m1, so we won't verify each element of m2 are 
    // in m1.

    } // operator==

    /// Inequality comparison operator. 
    bool operator!=(const self& m2) const
    {
        return !this->operator==(m2);
    }
    ////////////////////////////////////////////////////////////////

protected:
    
    typedef int (*db_compare_fcn_t)(Db *db, const Dbt *dbt1, 
        const Dbt *dbt2);
    typedef db_multimap<kdt, _DB_STL_set_value<kdt>, 
        value_type_sub, db_set_iterator<kdt, value_type_sub> > base;

    // Declare base our friend, we can't use 'friend class base' to do
    // this declaration on gcc.
    friend class db_multimap<kdt, _DB_STL_set_value<kdt>, 
        value_type_sub, db_set_iterator<kdt, value_type_sub> >; 
    // Declare iterator our friend, we can't use 'friend class iterator'
    // to do this declaration on gcc.
    friend class db_set_iterator<kdt, value_type_sub>; 
    friend class db_map_iterator<kdt, _DB_STL_set_value<kdt>, 
        value_type_sub>;
    friend class db_map_iterator<kdt, _DB_STL_set_value<kdt> >;

private:

    inline void copy_db(db_multiset<kdt, value_type_sub> &x)
    {    

        // Make sure clear can succeed if there are cursors 
        // open in other threads.
        this->clear(false);
        insert(x.begin(), x.end());
        
    }

    // Prevent user calling the inherited version.
    value_type operator[] (const key_type& x)
    {
        THROW(NotSupportedException, ("db_multiset<>::operator[]"));
        
    }

    value_type operator[] (const key_type& x) const
    {
        THROW(NotSupportedException, ("db_multiset<>::operator[]"));
        
    }

    virtual const char* verify_config(Db*dbp, DbEnv* envp) const
    {
        DBTYPE dbtype;
        u_int32_t oflags, sflags;
        int ret;
        const char *err = NULL;

        err = db_container::verify_config(dbp, envp);
        if (err)
            return err;

        BDBOP(dbp->get_type(&dbtype), ret);
        BDBOP(dbp->get_open_flags(&oflags), ret);
        BDBOP(dbp->get_flags(&sflags), ret);

        if (dbtype != DB_BTREE && dbtype != DB_HASH)
            err = 
"wrong database type, only DB_BTREE and DB_HASH allowed for db_map<> class";
        if (oflags & DB_TRUNCATE)
            err = 
"do not specify DB_TRUNCATE flag to create a db_map<> object";

        // Must set DB_DUP and NOT DB_DUPSORT.
        if (!(sflags & DB_DUP) || (sflags & DB_DUPSORT))
            err = 
"db_multiset<> requires a database with DB_DUP set and without DB_DUPSORT set.";
        
        if (sflags & DB_RECNUM)
            err = "no DB_RECNUM flag allowed in db_map<>";
        
        return err;
        
    }

}; // db_multiset<>


END_NS

#endif// !_DB_STL_DB_SET_H_
