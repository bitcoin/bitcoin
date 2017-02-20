/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_DB_MAP_H_
#define _DB_STL_DB_MAP_H_

#include <string>

#include "dbstl_common.h"
#include "dbstl_dbc.h"
#include "dbstl_container.h"
#include "dbstl_resource_manager.h"
#include "dbstl_element_ref.h"
#include "dbstl_base_iterator.h"

START_NS(dbstl)

using std::pair;
using std::make_pair;
using std::string;
// Forward declarations, DO NOT delete the default argument values
// because class templates defintions need them. No need for _export here.
//
template <Typename T>
class _DB_STL_set_value;

template <Typename kdt, Typename ddt, Typename value_type_sub = 
    ElementRef<ddt> >
class db_map_iterator;

template <Typename kdt, Typename ddt, Typename value_type_sub = 
    ElementRef<ddt>, Typename iterator_t = 
    db_map_iterator<kdt, ddt, value_type_sub> >
class db_map;

template<Typename kdt, Typename ddt, Typename value_type_sub = 
    ElementRef<ddt>, Typename iterator_t = 
    db_map_iterator<kdt, ddt, value_type_sub> >
class db_multimap;

template <Typename kdt>
class db_set_base_iterator;

template <Typename kdt, Typename value_type_sub = ElementRef<kdt> >
class db_set_iterator;

template <Typename kdt, Typename value_type_sub = ElementRef<kdt> >
class db_set;

template <Typename kdt, Typename value_type_sub = ElementRef<kdt> >
class db_multiset;

/** \ingroup dbstl_iterators
@{
\defgroup db_map_iterators Iterator classes for db_map and db_multimap.
db_map has two iterator class templates -- db_map_base_iterator and
db_map_iterator. They are the const iterator class and iterator class for
db_map and db_multimap. db_map_iterator inherits from db_map_base_iterator.

The two classes have identical behaviors to std::map::const_iterator and
std::map::iterator respectively. Note that the common public member function
behaviors are described in the db_base_iterator section.

The differences between the two classes are that the db_map_base_iterator can 
only be used to read its referenced value, while db_map_iterator allows both 
read and write access. If your access pattern is readonly, it is strongly 
recommended that you use the const iterator because it is faster and more 
efficient.
@{
*/
//////////////////////////////////////////////////////////////////////
// db_map_base_iterator class definition
//
// This class is a const iterator class for db_map and db_multimap, it can
// be used only to read data under the iterator, can't be used to write.
//
// Iterator const-ness implementation:
//
// const iterators can not update key/data pairs, other than this, 
// they can do anything else like non-const iterators, so we define
// db_map_base_iterator to be the const iterator which can only be used
// to read its underlying key/data pair, but not updating them; We 
// derive the db_map_iterator from the base iterator to be the 
// read-write iterator. We also maintain a "readonly" property in all
// iterators so that users can specify a db_map_iterator to be
// read only. db_map_base_iterator is more efficient to read data then
// db_map_iterator, so read only accesses are strongly recommended to be 
// done using a const iterator.
//
template <Typename kdt, Typename ddt, Typename csrddt = ddt>
class _exported db_map_base_iterator : public 
    db_base_iterator<ddt>
{
protected:
    typedef db_map_base_iterator<kdt, ddt, csrddt> self;
    typedef db_base_iterator<ddt> base;
    using base::replace_current_key;
public:
    typedef kdt key_type;
    typedef ddt data_type;
    typedef pair<kdt, ddt> value_type;
    // Not used in this class, but required to satisfy 
    // db_reverse_iterator type extraction.
    typedef ptrdiff_t difference_type;
    typedef difference_type distance_type;
    typedef value_type& reference;
    typedef value_type* pointer;
    typedef value_type value_type_wrap;
    // We have to use standard iterator tags to match the parameter 
    // list of stl internal functions, we can't use our own tag 
    // classes, so we don't write tag classes in dbstl.
    //
    typedef std::bidirectional_iterator_tag iterator_category;

    ////////////////////////////////////////////////////////////////////
    //
    // Begin public constructors and destructor.
    /// @name Constructors and destructor
    /// Do not create iterators directly using these constructors, but
    /// call db_map::begin or db_multimap_begin to get instances of
    /// this class.
    /// \sa db_map::begin() db_multimap::begin()
    //@{
    /// Copy constructor.
    /// \param vi The other iterator of the same type to initialize this.
    db_map_base_iterator(const self& vi)
        : db_base_iterator<ddt>(vi)
    {
        // Lazy-dup another cursor, cursor to iterator mapping 
        // is 1 to 1.
        pcsr_ = vi.pcsr_;
        curpair_base_.first = vi.curpair_base_.first;
        curpair_base_.second = vi.curpair_base_.second;
    }

    /// Base copy constructor.
    /// \param vi Initialize from a base class iterator.
    db_map_base_iterator(const base& vi) : base(vi), 
        pcsr_(new cursor_type(vi.get_bulk_retrieval(), 
        vi.is_rmw(), vi.is_directdb_get()))
    {
        
    }
    
    /// Constructor.
    /// \param powner The container which creates this iterator.
    /// \param b_bulk_retrieval The bulk read buffer size. 0 means 
    /// bulk read disabled.
    /// \param rmw Whether set DB_RMW flag in underlying cursor.
    /// \param directdbget Whether do direct database get rather than
    /// using key/data values cached in the iterator whenever read.
    /// \param readonly Whether open a read only cursor. Only effective 
    /// when using Berkeley DB Concurrent Data Store.
    explicit db_map_base_iterator(db_container*powner, 
        u_int32_t b_bulk_retrieval = 0, bool rmw = false, 
        bool directdbget = true, bool readonly = false)
        : db_base_iterator<ddt>(
        powner, directdbget, readonly, b_bulk_retrieval, rmw),
        pcsr_(new cursor_type(b_bulk_retrieval, rmw, directdbget)) 
    {   
    }

    /// Default constructor, dose not create the cursor for now.
    db_map_base_iterator()
    {
    }

    // Use virtual because ElementRef<> uses a db_base_iterator* pointer
    // to refer to the iterator, and also use "dead_" flag to avoid 
    // multiple calls to the same destructor by ~ElementRef<>().
    /// Destructor.
    virtual ~db_map_base_iterator() 
    {
        this->dead_ = true;
        if (pcsr_)
            pcsr_->close(); 
    }
    //@}

    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    // Begin functions that shift iterator position.
    //
    // Do not throw exceptions here because it is likely and normal 
    // to iterate to the "end itrerator". 
    //
    /// @name Iterator increment movement functions.
    /// The two functions moves the iterator one element backward, so that
    /// the element it sits on has a bigger key. The btree/hash key
    /// comparison routine determines which key is greater.
    /// Use ++iter rather than iter++ where possible to avoid two useless
    /// iterator copy constructions.
    //@{
    /// Pre-increment.
    /// \return This iterator after incremented.
    inline self& operator++()
    {
        next();
        
        return *this;
    }

    /// Post-increment.
    /// \return Another iterator having the old value of this iterator.
    inline self operator++(int)
    {
        self itr = *this;

        next();
        
        return itr;
    }
    //@}

    /// @name Iterator decrement movement functions.
    /// The two functions moves the iterator one element forward, so that
    /// the element it sits on has a smaller key. The btree/hash key 
    /// comparison routine determines which key is greater.
    /// Use --iter rather than iter-- where possible to avoid two useless
    /// iterator copy constructions.
    //@{
    /// Pre-decrement.
    /// \return This iterator after decremented.
    inline  self& operator--() 
    {
        prev();
        
        return *this;
    }

    /// Post-decrement.
    /// \return Another iterator having the old value of this iterator.
    self operator--(int)
    {
        self itr = *this;
        prev();
        
        return itr;
    }
    //@}

    /// Assignment operator. This iterator will point to the same key/data
    /// pair as itr, and have the same configurations as itr.
    /// \param itr The right value of assignment.
    /// \return The reference of itr.
    /// \sa db_base_iterator::operator=(const self&)
    // We will duplicate the Dbc cursor here.
    inline const self& operator=(const self&itr) 
    {
        ASSIGNMENT_PREDCOND(itr)
        base::operator=(itr);

        curpair_base_.first = itr.curpair_base_.first;
        curpair_base_.second = itr.curpair_base_.second;
        if (pcsr_)
            pcsr_->close();
        pcsr_ = itr.pcsr_;
        
        return itr;
    }

    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    // Begin iterator comparison functions.
    //
    /// \name Compare operators.
    /// Only equal comparison is supported.
    //@{
    /// Compare two iterators.
    /// Two iterators compare equal when they are both invalid or 
    /// both valid and underlying cursor compare equal(i.e. sitting on the
    /// same key/data pair).
    //
    // Note that the iterator itr or this iterator may be an invalid
    // one, i.e. its this->itr_status_ is INVALID_ITERATOR_POSITION. 
    // We do not distinguish between end and rend iterators although
    // we are able to do so, because they are never compared together.
    /// Equal comparison operator.
    /// \param itr The iterator to compare against.
    /// \return Returns true if equal, false otherwise.
    inline bool operator==(const self&itr) const
    {
        COMPARE_CHECK(itr)  
        if (((itr.itr_status_ == this->itr_status_) && 
            (this->itr_status_ == INVALID_ITERATOR_POSITION)) ||
            ((itr.itr_status_ == this->itr_status_) && 
            (pcsr_->compare((itr.pcsr_.base_ptr())) == 0)))
            return true;
        return false;
        
    }

    /// Unequal comparison operator.
    /// \param itr The iterator to compare against.
    /// \return Returns false if equal, true otherwise.
    /// \sa bool operator==(const self&itr) const
    inline bool operator!=(const self&itr) const 
    {
        return !(*this == itr) ;
    }
    //@}
    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    // Begin functions that retrieve values from the iterator.
    //
    // curpair_base_ is always kept updated on iterator movement, but if 
    // directdb_get_ is true, curpair_base_ is also updated here before 
    // making use of the value it references.
    // Even if this iterator is invalid, this call is allowed, the
    // default value of type T is returned.
    //
    // Note that the returned reference can only be used to read data,
    // can't be used to update data.
    /// \name Functions that retrieve values from the iterator.
    //@{
    /// Dereference operator.
    /// Return the reference to the cached data element, which is an 
    /// pair<Key_type, T>. You can only read its referenced data via 
    /// this iterator but can not update it.
    /// \return Current data element reference object, i.e. ElementHolder
    /// or ElementRef object.
    inline reference operator*() const 
    {
        
        if (this->directdb_get_) { 
            csrddt d;
            pcsr_->get_current_key_data(curpair_base_.first, d);
            assign_second0(curpair_base_, d);
        }
        // Returning reference, no copy construction.
        return curpair_base_;
    }
     
    // curpair_base_ is always kept updated on iterator movement, but if 
    // directdb_get_ is true, curpair_base_ is also updated here before 
    // making use of the value it references.
    // Even if this iterator is invalid, this call is allowed, the
    // default value of type T is returned.
    //
    // Note that the returned reference can only be used to read data,
    // can't be used to update data.
    /// Arrow operator.
    /// Return the pointer to the cached data element, which is an 
    /// pair<Key_type, T>. You can only read its referenced data via 
    /// this iterator but can not update it.
    /// \return Current data element reference object's address, i.e. 
    /// address of ElementHolder or ElementRef object.
    inline pointer operator->() const
    {
        
        if (this->directdb_get_) {
            csrddt d;
            pcsr_->get_current_key_data(curpair_base_.first, d);
            assign_second0(curpair_base_, d);
        }

        return &curpair_base_;
    }
    //@}
    ////////////////////////////////////////////////////////////////////
        
    ////////////////////////////////////////////////////////////////////
    // Begin dbstl specific functions.
    //
    // Refresh the underlying cursor's current data and this object's 
    // curpair_base_. It need to be called only if directdb_get is 
    // disabled, and other iterators updated
    // the key/data pair this iterator points to and we are about to use 
    // this iterator to access that key/data pair. 
    // If direct db get is enabled, this method never needs to be called.
    /// @name dbstl specific functions
    //@{
    /// \brief Refresh iterator cached value.
    /// \param from_db If not doing direct database get and this parameter
    /// is true, we will retrieve data directly from db.
    /// \sa db_base_iterator::refresh(bool) 
    virtual int refresh(bool from_db = true) const
    {
        csrddt d;

        if (from_db && !this->directdb_get_)
            pcsr_->update_current_key_data_from_db(
                DbCursorBase::SKIP_NONE);
        pcsr_->get_current_key_data(curpair_base_.first, d);
        assign_second0(curpair_base_, d);

        return 0;
    }

    // By calling this function, users can choose to close the underlying 
    // cursor before iterator destruction to get better performance 
    // potentially.
    /// \brief Close underlying Berkeley DB cursor of this iterator.
    /// \sa db_base_iterator::close_cursor() const
    inline void close_cursor() const
    {
        if (pcsr_)
            pcsr_->close();
    }

    /// Iterator movement function.
    /// Move this iterator to the specified key k, by default moves
    /// exactly to k, and update cached data element, you can
    /// also specify DB_SET_RANGE, to move to the biggest key smaller 
    /// than k. The btree/hash key comparison routine determines which
    /// key is bigger. When the iterator is on a multiple container, 
    /// move_to will move itself to the first key/data pair of the 
    /// identical keys.
    /// \param k The target key value to move to.
    /// \param flag Flags available: DB_SET(default) or DB_SET_RANGE.
    /// DB_SET will move this iterator exactly at k; DB_SET_RANGE moves
    /// this iterator to k or the smallest key greater than k. If fail
    /// to find such a key, this iterator will become invalid.
    /// \return 0 if succeed; non-0 otherwise, and this iterator becomes
    /// invalid. Call db_strerror with the return value to get the error 
    /// message.
    inline int move_to(const kdt& k, int flag = DB_SET) const
    {
        int ret;
        // Use tmpk2 to avoid k being modified.
        kdt tmpk2 = k;

        this->itr_status_ = (ret = pcsr_->move_to(tmpk2, flag));
        if (ret != 0) {
            this->inval_pos_type_ = base::IPT_UNSET;
            return ret;
        }

        refresh();

        return ret;
    }

    /// Modify bulk buffer size. 
    /// Bulk read is enabled when creating an
    /// iterator, so users later can only modify the bulk buffer size
    /// to another value, but can't enable/disable bulk read while an
    /// iterator is already alive. 
    /// \param sz The new size of the bulk read buffer of this iterator.
    /// \return Returns true if succeeded, false otherwise.
    /// \sa db_base_iterator::set_bulk_buffer(u_int32_t )
    bool set_bulk_buffer(u_int32_t sz)
    {
        bool ret = this->pcsr_->set_bulk_buffer(sz);
        if (ret)
            this->bulk_retrieval_ = 
                this->pcsr_->get_bulk_bufsize();
        return ret;
    }

    /// \brief Get bulk retrieval buffer size in bytes.
    /// \return Return current bulk buffer size or 0 if bulk retrieval is
    /// not enabled.
    /// \sa db_base_iterator::get_bulk_bufsize()
    u_int32_t get_bulk_bufsize()
    {
        this->bulk_retrieval_ = pcsr_->get_bulk_bufsize();
        return this->bulk_retrieval_;
    }
    //@}
    ////////////////////////////////////////////////////////////////////

protected:
    
    // The cursor_type is used to directly return the pair object, 
    // rather than a reference to it
    typedef DbCursor<kdt, csrddt> cursor_type;

    friend class db_map_iterator<kdt, ddt, ElementRef<ddt> >;
    friend class db_map_iterator<kdt, ddt, ElementHolder<ddt> >;
    // Use friend classes to hide internal members from users.
    friend class db_map<kdt, ddt, ElementHolder<ddt> >;
    friend class db_map<kdt, ddt, ElementHolder<kdt>, 
        db_set_iterator<kdt, ElementHolder<kdt> > >;
    friend class db_map<kdt, _DB_STL_set_value<kdt>, ElementHolder<kdt>, 
        db_set_iterator<kdt, ElementHolder<kdt> > >;

    friend class db_set<kdt, ElementHolder<kdt> >;
    friend class db_set_iterator<kdt, ElementHolder<kdt> >;
    friend class db_multiset<kdt, ElementHolder<kdt> >;
    friend class db_multimap<kdt, ddt, ElementHolder<ddt> >;
    friend class db_multimap<kdt, _DB_STL_set_value<kdt>, 
        ElementHolder<kdt>, db_set_iterator<kdt, ElementHolder<kdt> > >;

    friend class db_map<kdt, ddt, ElementRef<ddt> >;
    friend class db_map<kdt, _DB_STL_set_value<kdt>, ElementRef<kdt>, 
        db_set_iterator<kdt, ElementRef<kdt> > >;

    friend class db_set<kdt, ElementRef<kdt> >;
    friend class db_set_iterator<kdt, ElementRef<kdt> >;
    friend class db_multiset<kdt, ElementRef<kdt> >;
    friend class db_multimap<kdt, ddt, ElementRef<ddt> >;
    friend class db_multimap<kdt, _DB_STL_set_value<kdt>, ElementRef<kdt>,
        db_set_iterator<kdt, ElementRef<kdt> > >;


    ////////////////////////////////////////////////////////////////////
    // Begin db_map_base_iterator data members.
    //
    // Cursor of this iterator, note that each db_map_base_iterator has a  
    // unique DbCursor, not shared with any other iterator, and when copy 
    // constructing or assigning, the cursor is duplicated
    // when it is actually used to access db.
    //
    mutable LazyDupCursor<DbCursor<kdt, csrddt> > pcsr_;

    // In order for std::map style itrerator to work, we need a pair 
    // here to store the key-value pair this iterator currently points
    // to in the db_map. 
    //
    // curpair_base_ is always kept updated on every cursor/iterator 
    // movement and initialized to point to the first key-value pair when
    // db_map<>::begin() is called.
    //
    mutable value_type curpair_base_;   
    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    // Begin internal helper functions.
    //
    // Open the iterator and its cursor.
    //
    void open() const
    {   
        u_int32_t oflags = 0, coflags = 0;
        int ret;
        Db *pdb = this->owner_->get_db_handle();
        DbEnv *penv = pdb->get_env();

        coflags = this->owner_->get_cursor_open_flags();
        assert(this->owner_ != NULL);
        if (!this->read_only_ && penv != NULL) {
            BDBOP((penv->get_open_flags(&oflags)), ret);
            if ((oflags & DB_INIT_CDB) != 0)
                this->owner_->set_cursor_open_flags(coflags |= 
                    DB_WRITECURSOR);
        }
        if (!pcsr_)
            pcsr_.set_cursor(new DbCursor<kdt, csrddt>(
                this->bulk_retrieval_, 
                this->rmw_csr_, this->directdb_get_));
        this->itr_status_ = pcsr_->open((db_container*)this->owner_, 
            coflags);
        
    }

    // Move this iterator as well as the underlying Dbc* cursor to 
    // first element and update cur_pair_.
    //
    int first() const
    {

        assert(this->owner_ != NULL);
        this->itr_status_ = pcsr_->first();
        if (this->itr_status_ == 0) 
            refresh();
        else
            this->inval_pos_type_ = base::IPT_UNSET;

        return this->itr_status_;
        
    }

    // Move this iterator as well as the underlying Dbc* cursor
    // to last effective(valid) element and update cur_pair_.
    //
    int last() const
    {

        assert(this->owner_ != NULL);
        this->itr_status_ = pcsr_->last();
        if (this->itr_status_ == 0)
            refresh();
        else
            this->inval_pos_type_ = base::IPT_UNSET;

        return this->itr_status_;
    }

    // Move this iterator as well as the underlying Dbc* cursor
    // to next element, then update its position flags and cur_pair_.
    //
    int next(int flags = DB_NEXT) const
    {

        assert(this->owner_ != NULL);
        
        if (this->itr_status_ == INVALID_ITERATOR_POSITION) {
            if (this->inval_pos_type_ == base::IPT_BEFORE_FIRST) {
                // This rend itr must have an non-NULL owner.
                open();
                // rend itr can go back to first element.
                this->itr_status_ = first();
            } else if (this->inval_pos_type_ == base::IPT_UNSET) {
                THROW0(InvalidIteratorException);
            }
            // Else, return itr_status_ in last line.
        } else {
            
            this->itr_status_ = pcsr_->next(flags);
            if (this->itr_status_ == 0)
                refresh();
            else 
                this->inval_pos_type_ = base::IPT_AFTER_LAST;
            
        }
        
        return this->itr_status_;
    }

    // Move this iterator as well as the underlying Dbc* cursor
    // to previous element.
    //
    int prev(int flags = DB_PREV) const
    {

        assert(this->owner_ != NULL);
        if (this->itr_status_ == INVALID_ITERATOR_POSITION) { 
            if (this->inval_pos_type_ == base::IPT_AFTER_LAST) {
                // This rend itr must have an non-NULL owner.
                open();
                // end itr can go back to last element.
                this->itr_status_ = last();
            } else if (this->inval_pos_type_ == base::IPT_UNSET) {
                THROW0(InvalidIteratorException);
            }
            // Else, return itr stat in last line.
        } else {
            this->itr_status_ = pcsr_->prev(flags);
            if (this->itr_status_ == 0)
                refresh();
            else 
                this->inval_pos_type_ = base::IPT_BEFORE_FIRST;
            
        }

        return this->itr_status_;
    }

    void set_curpair_base(const kdt& k, const csrddt &d) const
    {
        curpair_base_.first = k;
        assign_second0(curpair_base_, d);
    }

    ////////////////////////////////////////////////////////////////////

protected: // Do not remove this line, otherwise assign_second0 may be public.
#ifndef DOXYGEN_CANNOT_SEE_THIS
#if NO_MEMBER_FUNCTION_PARTIAL_SPECIALIZATION
};// end of db_map_base_iterator<>
template <Typename kdt, Typename datadt, Typename ddt>
void assign_second0(pair<kdt, ddt>& v, const datadt& d) 
{
    v.second = d;
}

template<Typename kdt, Typename ddt>
void assign_second0(pair<kdt, ddt> &v, 
    const _DB_STL_set_value<kdt>&
    /* d unused, use v.first to assign v.second */) 
{
    v.second = v.first;
}
#else

template <Typename datadt>
inline void assign_second0(value_type& v, const datadt& d) const
{
    v.second = d;
}

template<>
inline void 
assign_second0(value_type &v, const _DB_STL_set_value<kdt>& 
    /* d unused, use v.first to assign v.second */) const
{
    v.second = v.first;
}

};// end of db_map_base_iterator<>

#endif

#else
};
#endif // DOXYGEN_CANNOT_SEE_THIS
//@} // db_map_iterators
//@} // dbstl_iterators


#if NO_MEMBER_FUNCTION_PARTIAL_SPECIALIZATION
template <Typename kdt, Typename datadt, Typename value_type_sub>
void assign_second0(pair<kdt, value_type_sub>& v, const datadt& d) ;
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// db_map_iterator class template definition
//
// db_map_iterator is the iterator class template for db_map and 
// db_multimap, it is also the base class for db_set_iterator. It can be
// used to both read and write the database.
//
// Template parameters info:
// kdt is "key data type", ddt is "data data type", value_type_sub is
// either ElementRef<ddt> (by default, in this case ElementRef inherits
// from ddt, so ddt must not be a primitive type) or ElementHolder<ddt>,
// in this case ElementHolder has a data member of type ddt, so suitable for
// primitive types, but don't apply it to classes otherwise you can't access
// members like this : *iterator.member = value.  
//
/// \ingroup dbstl_iterators 
//@{
/// \ingroup db_map_iterators
//@{
template <Typename kdt, Typename ddt, Typename value_type_sub>
class _exported db_map_iterator : public 
    db_map_base_iterator<kdt, typename value_type_sub::content_type, ddt>
{
protected:
    typedef db_map_iterator<kdt, ddt, value_type_sub> self;
    typedef typename value_type_sub::content_type realddt;
    using db_base_iterator<typename value_type_sub::content_type>::
        replace_current_key;
public:
    typedef kdt key_type;
    typedef ddt data_type;
    typedef pair<kdt, ddt> value_type;
    typedef pair<kdt, value_type_sub> value_type_wrap;
    // Not used in this class, but required to satisfy 
    // db_reverse_iterator type extraction.
    typedef ptrdiff_t difference_type;
    typedef difference_type distance_type;
    typedef value_type_wrap& reference;
    typedef value_type_wrap* pointer;

    // We have to use standard iterator tags to match the parameter 
    // list of stl internal functions, we can't use our own tag 
    // classes, so we don't write tag classes in dbstl.
    //
    typedef std::bidirectional_iterator_tag iterator_category;
    
    // Refresh the underlying cursor's current data and this object's 
    // curpair_. It need to be called only if other iterators updated the 
    // key/data pair this iterator points to and we are about to use 
    // this iterator to access that key/data pair. If direct db get is
    // enabled, this method never needs to be called.
    /// \brief Refresh iterator cached value.
    /// \param from_db If not doing direct database get and this parameter
    /// is true, we will retrieve data directly from db.
    /// \sa db_base_iterator::refresh(bool )
    virtual int refresh(bool from_db = true) const
    {
        kdt k;
        ddt d;

        if (from_db && !this->directdb_get_)
            this->pcsr_->update_current_key_data_from_db(
                DbCursorBase::SKIP_NONE);
        this->pcsr_->get_current_key_data(k, d);
        curpair_.first = k;
        assign_second(curpair_, d);
        this->set_curpair_base(k, d);

        return 0;
    }

    ////////////////////////////////////////////////////////////////
    // Begin constructors and destructor definitions.
    /// \name Constructors and destructor
    /// Do not create iterators directly using these constructors, but
    /// call db_map::begin or db_multimap_begin to get instances of
    /// this class.
    /// \sa db_map::begin() db_multimap::begin()
    //@{
    /// Copy constructor.
    /// \param vi The other iterator of the same type to initialize this.
    db_map_iterator(const db_map_iterator<kdt, ddt, value_type_sub>& vi)
        : db_map_base_iterator<kdt, realddt, ddt>(vi)
    {
        // Lazy-dup another cursor, cursor to iterator mapping 
        // is 1 to 1.
        curpair_.first = vi.curpair_.first;
        curpair_.second._DB_STL_CopyData(vi.curpair_.second);
        curpair_.second._DB_STL_SetIterator(this);
    }

    /// Base copy constructor.
    /// \param vi Initialize from a base class iterator.
    db_map_iterator(const db_map_base_iterator<kdt, realddt, ddt>& vi) :
        db_map_base_iterator<kdt, realddt, ddt>(vi)

    {
        curpair_.second._DB_STL_SetIterator(this);
        curpair_.first = vi->first;
        curpair_.second._DB_STL_CopyData(vi->second);
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
    explicit db_map_iterator(db_container*powner, 
        u_int32_t b_bulk_retrieval = 0, bool brmw = false, 
        bool directdbget = true, bool b_read_only = false)
        : db_map_base_iterator<kdt, realddt, ddt>
        (powner, b_bulk_retrieval, brmw, directdbget, b_read_only) 
    { 
        curpair_.second._DB_STL_SetIterator(this);
    }

    /// Default constructor, dose not create the cursor for now.
    db_map_iterator() : db_map_base_iterator<kdt, realddt, ddt>()
    {
        curpair_.second._DB_STL_SetIterator(this);
    }

    // Use virtual because ElementRef<> uses a db_base_iterator* pointer 
    // to refer to the iterator, and also use "dead_" flag to avoid 
    // multiple call to the same destructor by ~ElementRef<>().
    /// Destructor.
    virtual ~db_map_iterator() 
    {
        // Required here though set in base destructor too.
        this->dead_ = true;
    }
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    // Begin functions that shift iterator position.
    //
    // Do not throw exceptions here because it is likely and normal 
    // to iterate to the "end itrerator". 
    /// \name Iterator movement operators.
    //@{
    /// Pre-increment \sa db_map_base_iterator::operator++()
    /// \return This iterator after incremented.
    inline self& operator++()
    {
        this->next();
        
        return *this;
    }

    /// Post-increment \sa db_map_base_iterator::operator++(int)
    /// \return Another iterator having the old value of this iterator.
    inline self operator++(int)
    {
        self itr = *this;

        this->next();
        
        return itr;
    }

    /// Pre-decrement \sa db_map_base_iterator::operator--()
    /// \return This iterator after decremented.
    inline  self& operator--() 
    {
        this->prev();
        
        return *this;
    }

    /// Post-decrement \sa db_map_base_iterator::operator--(int)
    /// \return Another iterator having the old value of this iterator.
    self operator--(int)
    {
        self itr = *this;
        this->prev();
        
        return itr;
    }
    //@}
    // Assignment operator, we will duplicate the Dbc cursor here.
    /// Assignment operator. This iterator will point to the same key/data
    /// pair as itr, and have the same configurations as itr.
    /// \param itr The right value of assignment.
    /// \return The reference of itr.
    /// \sa db_base_iterator::operator=(const self&)
    inline const self& operator=(const self&itr) 
    {
        ASSIGNMENT_PREDCOND(itr)
        base::operator=(itr);

        curpair_.first = itr.curpair_.first;

        // Only copy data from itr.curpair_ into curpair_, 
        // don't store into db. Note that we can not assign 
        // itr.curpair_ to curpair_ simply by curpair_ = itr.curpair_,
        // otherwise, ElementRef<>::operator= is called, which will
        // update the data element referenced by this iterator.
        //
        curpair_.second._DB_STL_CopyData(itr.curpair_.second);
        
        return itr;
    }
    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    // Begin functions that retrieve values from the iterator.
    //
    // curpair_base_ is always kept updated on iterator movement, but if 
    // directdb_get_ is true, curpair_base_ is also updated here before 
    // making use of the value it references.
    // Even if this iterator is invalid, this call is allowed, the
    // default value of type T is returned.
    //
    /// \name Functions that retrieve values from the iterator.
    //@{
    /// Dereference operator.
    /// Return the reference to the cached data element, which is an
    /// pair<Key_type, ElementRef<T> > object if T is a class type or an
    /// pair<Key_type, ElementHolder<T> > object if T is a C++ primitive
    /// data type. 
    /// \return Current data element reference object, i.e. ElementHolder
    /// or ElementRef object.
    inline reference operator*() const 
    {
        
        if (this->directdb_get_) {
            ddt d;
            this->pcsr_->get_current_key_data(curpair_.first, d);
            assign_second(curpair_, d);
        }

        return curpair_;// returning reference, no copy construction
    }
     
    // curpair_base_ is always kept updated on iterator movement, but if 
    // directdb_get_ is true, curpair_base_ is also updated here before 
    // making use of the value it references.
    // Even if this iterator is invalid, this call is allowed, the
    // default value of type T is returned.
    /// Arrow operator.
    /// Return the pointer to the cached data element, which is an
    /// pair<Key_type, ElementRef<T> > object if T is a class type or an
    /// pair<Key_type, ElementHolder<T> > object if T is a C++ primitive
    /// data type. 
    /// \return Current data element reference object's address, i.e. 
    /// address of ElementHolder or ElementRef object.
    inline pointer operator->() const
    {
    
        if (this->directdb_get_) {
            ddt d;
            this->pcsr_->get_current_key_data(curpair_.first, d);
            assign_second(curpair_, d);
        }
        return &curpair_;
    }
    //@}
    ////////////////////////////////////////////////////////////////////

//@} // db_map_iterators
//@} // dbstl_iterators
protected:
    // The cursor_type is used to directly return the pair object, 
    // rather than a reference to it.
    typedef DbCursor<kdt, ddt> cursor_type;
    typedef db_map_base_iterator<kdt, realddt, ddt> base;
    typedef db_map_base_iterator<kdt, realddt> const_version;

    // Use friend classes to hide internal members from users.
    friend class db_map<kdt, ddt, value_type_sub>;
    friend class db_map<kdt, ddt, value_type_sub, 
        db_set_iterator<kdt, value_type_sub> >;
    friend class db_set<kdt, value_type_sub>;
    friend class db_set_iterator<kdt, value_type_sub>;
    friend class db_multiset<kdt, value_type_sub>;
    friend class db_multimap<kdt, ddt, value_type_sub>;
    friend class db_multimap<kdt, _DB_STL_set_value<kdt>, value_type_sub,
        db_set_iterator<kdt, value_type_sub > >;

    ////////////////////////////////////////////////////////////////
    // Begin db_map_iterator data members.
    //
    // In order for std::map style itrerator to work, we need a pair 
    // here to store the key-value pair this iterator currently points
    // to in the db_map. 
    //
    // curpair_ is always kept updated on every cursor/iterator movement
    // and initialized to point to the first key-value pair when
    // db_map<>::begin() is called.
    //
    mutable value_type_wrap curpair_;   
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin internal helper functions.
    //
    // Called by ElementRef<> object when this iterator belongs to the 
    // object---The only situation is in db_container::operator[] which
    // has to return an ElementRef/Holder object A, and its iterator has
    // to survive until A is destructed.
    virtual void delete_me() const
    {
        if (!this->dead_)
            delete this;
    }

    // Duplicate this iterator.
    virtual self* dup_itr() const
    {
        self *itr = new self(*this);
        // The curpair_ of itr does not delete itr, the independent 
        // one does.
        //itr->curpair_.second._DB_STL_SetDelItr();
        return itr;
    }

    // Replace the current key/data pair's data pointed to by this 
    // iterator's underlying Dbc* cursor with the parameter d.
    //
    virtual int replace_current(
        const typename value_type_sub::content_type& d)
    {
        int ret;

        if (this->read_only_) {
            THROW(InvalidFunctionCall, (
"db_map_iterator<>::replace_current can't be called via a read only iterator"));
        }
        ret = this->pcsr_->replace(d);
        
        return ret;
    }

    // Used by set iterator to store another different key : 
    // remove the previous one then insert the new one.
    // It has to be defined in this class because db_set_iterator
    // inherits from db_map_iterator but we have no polymorphism when
    // using stl because the object are always used rather the 
    // pointer/reference.
    //
    virtual int replace_current_key(const kdt& k)
    {
        int ret;

        if (this->read_only_) {
            THROW(InvalidFunctionCall, (
"db_map_iterator<>::replace_current_key can't be called via a read only iterator"));
        }
        ret = this->pcsr_->replace_key(k);

        return ret;
    }

    ////////////////////////////////////////////////////////////////


protected: // Do not remove this line, otherwise assign_second may be public.
#ifndef DOXYGEN_CANNOT_SEE_THIS
#if NO_MEMBER_FUNCTION_PARTIAL_SPECIALIZATION
};// end of db_map_iterator<>

template <Typename kdt, Typename datadt, Typename value_type_sub>
void assign_second(pair<kdt, value_type_sub>& v, const datadt& d) 
{
    v.second._DB_STL_CopyData(d);
}

template<Typename kdt, Typename value_type_sub>
void assign_second(pair<kdt, value_type_sub> &v, 
    const _DB_STL_set_value<kdt>&
    /* d unused, use v.first to assign v.second */) 
{
    v.second._DB_STL_CopyData(v.first);
}
#else

template <Typename datadt>
inline void assign_second(value_type_wrap& v, const datadt& d) const
{
    v.second._DB_STL_CopyData(d);
}

template<>
inline void 
assign_second(value_type_wrap &v, const _DB_STL_set_value<kdt>& 
    /* d unused, use v.first to assign v.second */) const
{
    v.second._DB_STL_CopyData(v.first);
}

};// end of db_map_iterator<>

#endif
#else
};
#endif // DOXYGEN_CANNOT_SEE_THIS
u_int32_t hash_default(Db *dbp, const void *key, u_int32_t len);

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// db_map container class definition
/// \ingroup dbstl_containers
//@{
/// db_map has identical methods to std::map and the semantics for each
/// method is identical to its std::map counterpart, except that it stores data
/// into underlying Berkeley DB btree or hash database. Passing a database
/// handle of btree or hash type creates a db_map equivalent to std::map and
/// std::hashmap respectively.
/// Database(dbp) and environment(penv) handle requirement(applies to all 
/// constructors in this class template):
/// 0. The dbp is opened inside the penv environment. Either one of the two 
/// handles can be NULL. If dbp is NULL, an anonymous database is created 
/// by dbstl. 
/// 1. Database type of dbp should be DB_BTREE or DB_HASH.
/// 2. No DB_DUP or DB_DUPSORT flag set in dbp.
/// 3. No DB_RECNUM flag set in dbp.
/// 4. No DB_TRUNCATE specified in dbp's database open flags.
/// 5. DB_THREAD must be set if you are sharing the dbp across 
/// multiple threads directly, or indirectly by sharing the container object 
/// across multiple threads.
/// \param kdt The key data type.
/// \param ddt The data data type. db_map stores key/data pairs.
/// \param value_type_sub Do not specify anything if ddt type is a 
/// class/struct type; Otherwise, specify ElementHolder<ddt> to it.
/// \param iterator_t Never specify anything to this type parameter. It is
/// only used internally.
/// \sa db_container db_container(Db*, DbEnv*) db_container(const db_container&)
template <Typename kdt, Typename ddt, Typename value_type_sub, 
    Typename iterator_t>
class _exported db_map : public db_container 
{

public:
    // iterator_t is default argument, see forward declaration at the
    // head of this file
    typedef iterator_t iterator;
    typedef typename iterator::const_version const_iterator;
    typedef db_reverse_iterator<iterator, const_iterator> reverse_iterator;
    typedef db_reverse_iterator<const_iterator, iterator>
        const_reverse_iterator;
    typedef kdt key_type;
    typedef ddt data_type;
    typedef value_type_sub data_type_wrap;
    typedef pair<kdt, ddt> value_type;
    typedef pair<kdt, value_type_sub> value_type_wrap;
    typedef const value_type const_value_type;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    // The following three types are not used in db_map, but we define
    // them to conform to stl specifications.
    typedef value_type_wrap& reference;
    typedef const value_type& const_reference;
    typedef value_type_wrap* pointer;
protected:
    typedef db_map<kdt, ddt, value_type_sub, iterator> self;
    typedef typename value_type_sub::content_type realddt;

    // This constructor is for db_multimap's constructors to call, 
    // because other constructors of this class will verify db handles 
    // and create one if needed. We need a special one that don't do 
    // anything. The BulkRetrievalOption is randomly picked, no special
    // implications at all.
    db_map(BulkRetrievalOption& arg){ delete &arg; }
public:
    ////////////////////////////////////////////////////////////////
    // Begin inner class definitions.
    //
    // key_compare class definition, it is defined as an inner class,
    // using underlying btree/hash db's compare function
    //
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

    // value_compare class definition, it is defined as an inner class,
    // using key_compare class to do comparison. 
    //
    // The difference between key_compare and value_compare is the 
    // parameter its operator() function accepts, see the function
    // signature.
    //
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
            
            return kc(v1.first, v2.first);
        }

    }; // value_compare class definition

    class hasher
    {
    private:
        Db*pdb;
    public:
        hasher(Db*db){pdb = db;}
        size_t operator()(const kdt&k) const
        {
            DBTYPE dbtype;
            int ret;

            assert(pdb != NULL);
            ret = pdb->get_type(&dbtype);
            assert(ret == 0);
            if (dbtype != DB_HASH) {
                THROW(InvalidFunctionCall, (
                    "db_map<>::hasher"));
            }
            h_hash_fcn_t hash = NULL;
            BDBOP(pdb->get_h_hash(&hash), ret);
            if (hash == NULL)
                hash = hash_default;
            return hash(pdb, &k, sizeof(k));
        }
    }; // hasher

    class key_equal
    {
    private:
        Db*pdb;
    public:
        key_equal(Db*db){pdb = db;}
        bool operator()(const kdt& kk1, const kdt&kk2) const
        {
            DBTYPE dbtype;
            kdt k1 = kk1, k2 = kk2;
            int ret;

            dbstl_assert(pdb != NULL);
            ret = pdb->get_type(&dbtype);
            dbstl_assert(ret == 0);
            if (dbtype != DB_HASH) {
                THROW(InvalidFunctionCall, (
                    "db_map<>::key_equal"));
            }

            db_compare_fcn_t comp = NULL;
            BDBOP(pdb->get_h_compare(&comp), ret);
            if (comp == NULL)
                return memcmp(&kk1, &kk2, sizeof(kdt)) == 0;
            Dbt kd1(&k1, sizeof(k1)), kd2(&k2, sizeof(k2));
            
            return comp(pdb, &kd1, &kd2) == 0;


        }

    };// key_equal
    ////////////////////////////////////////////////////////////////

    /// Function to get key compare functor.
    /// Used when this container is a hash_map, hash_multimap,
    /// hash_set or hash_multiset equivalent.
    /// \return key_equal type of compare functor.
    /// \sa http://www.sgi.com/tech/stl/hash_map.html
    inline key_equal key_eq() const
    {
        key_equal ke(this->get_db_handle());
        return ke;
    }

    /// Function to get hash key generating functor.
    /// Used when this container is a hash_map, hash_multimap,
    /// hash_set or hash_multiset equivalent.
    /// \return The hash key generating functor.
    /// \sa http://www.sgi.com/tech/stl/hash_map.html
    inline hasher hash_funct() const
    {
        hasher h(this->get_db_handle());
        return h;

    }

    /// Function to get value compare functor. Used when this container
    /// is a std::map, std::multimap, std::set or std::multiset equivalent.
    /// \return The value compare functor.
    /// \sa http://www.cplusplus.com/reference/stl/map/value_comp/
    inline value_compare value_comp() const
    {
        value_compare vc(this->get_db_handle());
        return vc;
    }
    
    /// Function to get key compare functor. Used when this container
    /// is a std::map, std::multimap, std::set or std::multiset equivalent.
    /// \return The key compare functor.
    /// \sa http://www.cplusplus.com/reference/stl/map/key_comp/
    inline key_compare key_comp() const 
    {
        key_compare kc(this->get_db_handle());
        return kc;
    }

    ////////////////////////////////////////////////////////////////
    // Begin constructors and destructor definitions.
    /// \name Constructors and destructor
    //@{
    // We don't need the equal compare or allocator here, user need to 
    // call Db::set_bt_compare or Db::set_h_compare to set comparison
    // function.
    /// Create a std::map/hash_map equivalent associative container.
    /// See the handle requirement in class details to pass correct 
    /// database/environment handles.
    /// \param dbp The database handle.
    /// \param envp The database environment handle.
    /// \sa db_container(Db*, DbEnv*)
    explicit db_map(Db *dbp = NULL, DbEnv* envp = NULL)  : 
        db_container(dbp, envp)
    {
        const char *errmsg;

        this->open_db_handles(dbp, envp, DB_BTREE, 
            DB_CREATE | DB_THREAD, 0);
        
        if ((errmsg = verify_config(dbp, envp)) != NULL) {
            THROW(InvalidArgumentException, ("Db*", errmsg));
        }
        this->set_db_handle_int(dbp, envp);
    }

    /// Iteration constructor. Iterates between first and last, 
    /// setting a copy of each of the sequence of elements as the
    /// content of the container object. 
    /// Create a std::map/hash_map equivalent associative container.
    /// Insert a range of elements into the database. The range is
    /// [first, last), which contains elements that can
    /// be converted to type ddt automatically.
    /// See the handle requirement in class details to pass correct
    /// database/environment handles.
    /// This function supports auto-commit.
    /// \param dbp The database handle.
    /// \param envp The database environment handle.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    /// \sa db_container(Db*, DbEnv*)
    template <class InputIterator> 
    db_map(Db *dbp, DbEnv* envp, InputIterator first, 
        InputIterator last) : db_container(dbp, envp)
    {
        const char *errmsg;

        this->open_db_handles(dbp, envp, DB_BTREE, 
            DB_CREATE | DB_THREAD, 0);
        if ((errmsg = verify_config(dbp, envp)) != NULL) {
            THROW(InvalidArgumentException, ("Db*", errmsg));
        }
        this->set_db_handle_int(dbp, envp);

        this->begin_txn();
        try {
            insert(first, last);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
    }

    // Copy constructor. The object is initialized to have the same 
    // contents as the x map object, do not copy properties because
    // if we copy things like pdb, we are storing to the same db, so we
    // create a new database, use it as the backing db, and store data 
    // into it.
    /// Copy constructor.
    /// Create an database and insert all key/data pairs in x into this
    /// container. x's data members are not copied.
    /// This function supports auto-commit.
    /// \param x The other container to initialize this container.
    /// \sa db_container(const db_container&)
    db_map(const db_map<kdt, ddt, value_type_sub, iterator>& x) : 
        db_container(x)
    {
        verify_db_handles(x);
        this->set_db_handle_int(this->clone_db_config(
            x.get_db_handle()), x.get_db_env_handle());
        assert(this->get_db_handle() != NULL);
        
        this->begin_txn();
        try {
            copy_db((db_map<kdt, ddt, value_type_sub, iterator>&)x);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
    }

    virtual ~db_map(){}
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin insert function definitions.
    /// Container content assignment operator.
    /// This function supports auto-commit.
    /// \param x The other container whose key/data pairs will be inserted
    /// into this container. Old content in this containers are discarded.
    /// \sa http://www.cplusplus.com/reference/stl/map/operator=/
    inline const self& operator=(const self& x)
    {
        ASSIGNMENT_PREDCOND(x)
        db_container::operator =(x);
        verify_db_handles(x);
        assert(this->get_db_handle() != NULL);
        this->begin_txn();
        try {
            copy_db((self &)x);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
        return x;
    }

    /// \name Insert Functions
    /// They have similiar usage as their C++ STL equivalents.
    /// Note that when secondary index is enabled, each 
    /// db_container can create a db_multimap secondary container, 
    /// but the insert function is not functional for secondary containers.
    /// \sa http://www.cplusplus.com/reference/stl/map/insert/
    //@{
    //
    // Insert functions. Note that stl requires if the entry with x.key 
    // already exists, insert should not overwrite that entry and the 
    // insert should fail; but bdb Dbc::cursor(DB_KEYLAST) will replace
    // existing data with new one, so we will first find whether we
    // have this data, if have, return false;
    //
    // Can not internally use begin/commit_txn to wrap this call because 
    // it returns an iterator, which is closed after commit_txn(), and
    // reopening it is wrong in multithreaded access.
    /// Insert a single key/data pair if the key is not in the container.
    /// \param x The key/data pair to insert.
    /// \return A pair P, if insert OK, i.e. the inserted key wasn't in the
    /// container, P.first will be the iterator sitting on the inserted
    /// key/data pair, and P.second is true; otherwise P.first is an 
    /// invalid iterator and P.second is false.
    pair<iterator,bool> insert (const value_type& x ) 
    {
        pair<iterator,bool> ib;
        iterator witr;

        init_itr(witr);
        open_itr(witr);

        if (witr.move_to(x.first) == 0) {// has it
            ib.first = witr;
            ib.second = false;
            // Cursor movements are not logged, no need to 
            // use transaction here.
            return ib;
        } 
    
        witr.itr_status_ = witr.pcsr_->insert(x.first, x.second, 
            DB_KEYLAST);
        assert(witr.itr_status_ == 0);
        witr.refresh(false);
        ib.first = witr;
        ib.second = true;

        return ib;
    }

    /// Insert with hint position. We ignore the hint position because 
    /// Berkeley DB knows better where to insert.
    /// \param position The hint position.
    /// \param x The key/data pair to insert.
    /// \return The iterator sitting on the inserted key/data pair, or an
    /// invalid iterator if the key was already in the container.
    inline iterator insert (iterator position, const value_type& x ) 
    {
        pair<iterator,bool> ib = insert(x);
        return ib.first;
    }

    // Member function template overload.
    /// Range insertion. Insert a range [first, last) of key/data pairs
    /// into this container.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    void insert (const db_map_base_iterator<kdt, realddt, ddt>& first, 
        const db_map_base_iterator<kdt, realddt, ddt>& last) 
    {
        db_map_base_iterator<kdt, realddt, ddt> ii;
        iterator witr;

        init_itr(witr);
        open_itr(witr);

        for (ii = first; ii != last; ++ii)
            witr.pcsr_->insert(ii->first, ii->second, 
                DB_KEYLAST);    
    }

    /// Range insertion. Insert a range [first, last) of key/data pairs
    /// into this container.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    template<typename InputIterator>
    void insert (InputIterator first, InputIterator last) 
    {
        InputIterator ii;
        iterator witr;

        init_itr(witr);
        open_itr(witr);

        for (ii = first; ii != last; ++ii)
            witr.pcsr_->insert(ii->first, ii->second, 
                DB_KEYLAST);    
    }
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that create iterators.
    /// \name Iterator Functions
    /// The parameters in begin functions of this group have identical
    /// meaning to thoes in db_vector::begin, refer to those functions
    /// for details.
    /// \sa db_vector::begin()
    //@{
    /// Begin a read-write or readonly iterator which sits on the first
    /// key/data pair of the database. 
    /// \param rmw Same as that of 
    /// db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param bulkretrieval Same as that of 
    /// db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param directdb_get Same as that of
    /// db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param readonly Same as that of 
    /// db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \return The created iterator.
    /// \sa db_vector::begin(ReadModifyWriteOption, bool, 
    /// BulkRetrievalOption, bool)
    //
    iterator begin(ReadModifyWriteOption rmw = 
        ReadModifyWriteOption::no_read_modify_write(),
        bool readonly = false, BulkRetrievalOption bulkretrieval = 
        BulkRetrievalOption::no_bulk_retrieval(), 
        bool directdb_get = true) 
    {
        bool b_rmw;
        u_int32_t bulk_retrieval = 0;

        b_rmw = (rmw == ReadModifyWriteOption::read_modify_write());
        // Read only cursor don't need acquire write lock.
        if (readonly && b_rmw)
            b_rmw = false;
        if (readonly && bulkretrieval == BulkRetrievalOption::
            BulkRetrieval)
            bulk_retrieval = bulkretrieval.bulk_buf_size();

        iterator itr(dynamic_cast<db_container*>(this), 
            bulk_retrieval, b_rmw, directdb_get, readonly);

        open_itr(itr, readonly);
        itr.first();
        return itr;
    }

    /// Begin a read-only iterator.
    /// \param bulkretrieval Same as that of 
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param directdb_get Same as that of
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \return The created const iterator.
    /// \sa db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption,
    /// bool);
    const_iterator begin(BulkRetrievalOption bulkretrieval = 
        BulkRetrievalOption::no_bulk_retrieval(), 
        bool directdb_get = true) const 
    {
        u_int32_t b_bulk_retrieval = (bulkretrieval == 
            BulkRetrievalOption::BulkRetrieval ? 
            bulkretrieval.bulk_buf_size() : 0);

        const_iterator itr((db_container*)this,
             b_bulk_retrieval, false, directdb_get, true);

        open_itr(itr, true);
        itr.first();
        return itr;
    }

    /// \brief Create an open boundary iterator.
    /// \return Returns an invalid iterator denoting the position after 
    /// the last valid element of the container.
    /// \sa db_vector::end()
    inline iterator end()
    {
        iterator itr;

        // end() is at an invalid position. We don't know what key it 
        // refers, so itr_status_ and inval_pos_type are the only 
        // data members to identify an iterator's position.
        //
        itr.itr_status_ = INVALID_ITERATOR_POSITION;
        itr.inval_pos_type_ = iterator::IPT_AFTER_LAST;
        itr.owner_ = (db_container*)this;
        return itr;
    }

    /// \brief Create an open boundary iterator.
    /// \return Returns an invalid const iterator denoting the position 
    /// after the last valid element of the container.
    /// \sa db_vector::end() const
    inline const_iterator end() const
    {
        const_iterator itr;

        // end() is at an invalid position. We don't know what key it 
        // refers, so itr_status_ and inval_pos_type are the only 
        // data members to identify an iterator's position.
        //
        itr.itr_status_ = INVALID_ITERATOR_POSITION;
        itr.inval_pos_type_ = iterator::IPT_AFTER_LAST;
        itr.owner_ = (db_container*)this;
        return itr;
    }

    /// Begin a read-write or readonly reverse iterator which sits on the 
    /// first key/data pair of the database. 
    /// \param rmw Same as that of 
    /// db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param bulkretrieval Same as that of 
    /// db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param directdb_get Same as that of
    /// db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param read_only Same as that of 
    /// db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \return The created iterator.
    /// \sa db_vector::begin(ReadModifyWriteOption, bool, 
    /// BulkRetrievalOption, bool)
    /// \sa db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption,
    /// bool);
    reverse_iterator rbegin(ReadModifyWriteOption rmw = 
        ReadModifyWriteOption::no_read_modify_write(),
        bool read_only = false, BulkRetrievalOption bulkretrieval = 
        BulkRetrievalOption::no_bulk_retrieval(), 
        bool directdb_get = true) 
    {
        u_int32_t bulk_retrieval = 0;

        iterator itr = end();
        itr.rmw_csr_ = (rmw == (
            ReadModifyWriteOption::read_modify_write())) && !read_only;
        itr.directdb_get_ = directdb_get;
        itr.read_only_ = read_only;
        if (read_only && bulkretrieval == BulkRetrievalOption::
            BulkRetrieval)
            bulk_retrieval = bulkretrieval.bulk_buf_size();
        itr.bulk_retrieval_ = bulk_retrieval;
        reverse_iterator ritr(itr);
        
        return ritr;
    }

    /// Begin a read-only reverse iterator.
    /// \param bulkretrieval Same as that of 
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param directdb_get Same as that of
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \return The created const iterator.
    /// \sa db_vector::begin(ReadModifyWrite, bool, BulkRetrievalOption, 
    /// bool);
    const_reverse_iterator rbegin(BulkRetrievalOption bulkretrieval = 
        BulkRetrievalOption::no_bulk_retrieval(), 
        bool directdb_get = true) const
    {
        const_iterator itr = end();
        itr.bulk_retrieval_ = (bulkretrieval == 
            BulkRetrievalOption::BulkRetrieval ? 
            bulkretrieval.bulk_buf_size() : 0);
        itr.directdb_get_ = directdb_get;
        itr.read_only_ = true;
        const_reverse_iterator ritr(itr);
        
        return ritr;
    }

    /// \brief Create an open boundary iterator.
    /// \return Returns an invalid iterator denoting the position 
    /// before the first valid element of the container.
    /// \sa db_vector::rend()
    inline reverse_iterator rend()
    {
        reverse_iterator ritr;
        ritr.inval_pos_type_ = iterator::IPT_BEFORE_FIRST;
        return ritr;
    }

    /// \brief Create an open boundary iterator.
    /// \return Returns an invalid const iterator denoting the position 
    /// before the first valid element of the container. 
    /// \sa db_vector::rend() const
    inline const_reverse_iterator rend() const
    {
        const_reverse_iterator ritr;
        ritr.inval_pos_type_ = iterator::IPT_BEFORE_FIRST;
        return ritr;
    }
    //@} // iterator functions
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    //
    // Begin functions that return container metadata.
    /// \name Metadata Functions
    /// These functions return metadata about the container.
    //@{
    /// Get container category.
    /// Determines whether this container object is a std::map<> 
    /// equivalent(when returns false) or that of hash_map<> 
    /// class(when returns true). This method is not in stl, but it
    /// may be called by users because some operations are not supported
    /// by both type(map/hash_map) of containers, you need to call this
    /// function to distinguish the two types. dbstl will not stop you
    /// from calling the wrong methods of this class.
    /// \return Returns true if this container is a hash container based
    /// on a Berkeley DB hash database; returns false if it is based on a
    /// Berkeley DB btree database.
    //
    inline bool is_hash() const
    {
        DBTYPE dbtype = DB_UNKNOWN;
        int ret;

        assert(this->get_db_handle() != NULL);
        ret = this->get_db_handle()->get_type(&dbtype);
        assert(ret == 0);
        return dbtype == DB_HASH;
    }

    /// Only for std::hash_map, return number of hash bucket in use.
    /// This function supports auto-commit.
    /// \return The number of hash buckets of the database.
    size_type bucket_count() const
    {
        DBTYPE dbtype;
        u_int32_t flags;
        void *sp;
        size_type sz;
        int ret;
        DbTxn*txn;
        
        assert(this->get_db_handle() != NULL);
        ret = this->get_db_handle()->get_type(&dbtype);
        assert(ret == 0);
        if (dbtype != DB_HASH) {
            THROW(InvalidFunctionCall, ("db_map<>::bucket_count"));
            
        }
        flags = DB_FAST_STAT;
        
        // Here we use current_txn(), so we will get a valid 
        // transaction handle if we are using explicit transactions; 
        // and NULL if we are using autocommit, in which case bdb 
        // internal auto commit will be enabled automatically.
        //
        txn = ResourceManager::instance()->
            current_txn(this->get_db_handle()->get_env());
        BDBOP(this->get_db_handle()->stat(txn, &sp, flags), ret);
        
        sz = (size_type)(((DB_HASH_STAT*)sp)->hash_buckets);
        free(sp);
        return sz;
    }
    
    /// Get container size.
    // Return size of the map, can control whether compute
    // accurately(slower if db is huge) or not.
    /// This function supports auto-commit.
    /// \return Return the number of key/data pairs in the container.
    /// \param accurate This function uses database's statistics to get
    /// the number of key/data pairs. The statistics mechanism will either
    /// scan the whole database to find the accurate number or use the
    /// number of last accurate scanning, and thus much faster. If there 
    /// are millions of key/data pairs, the scanning can take some while,
    /// so in that case you may want to set the "accurate" parameter to 
    /// false.
    size_type size(bool accurate = true) const
    {
        u_int32_t flags;
        void *sp;
        DBTYPE dbtype;
        size_t sz;
        int ret;
        DbTxn*txn;

        flags = accurate ? 0 : DB_FAST_STAT;
        BDBOP(this->get_db_handle()->get_type(&dbtype), ret);

        // Here we use current_txn(), so we will get a valid 
        // transaction handle if we are using explicit transactions; 
        // and NULL if we are using autocommit, in which case bdb 
        // internal auto commit will be enabled automatically.
        //
        txn = ResourceManager::instance()->
            current_txn(this->get_db_handle()->get_env());
        BDBOP(this->get_db_handle()->stat(txn, &sp, flags), ret);

        assert((dbtype == DB_BTREE) || (dbtype == DB_HASH));
        // dbtype is BTREE OR HASH, no others.
        sz = dbtype == DB_BTREE ? ((DB_BTREE_STAT*)sp)->
            bt_ndata : ((DB_HASH_STAT*)sp)->hash_ndata;
        free(sp);
        return sz;
    }

    /// Get max size.
    /// The returned size is not the actual limit of database. See the
    /// Berkeley DB limits to get real max size.
    /// \return A meaningless huge number.
    /// \sa db_vector::max_size()
    inline size_type max_size() const
    {
        return SIZE_T_MAX;
    }

    /// Returns whether this container is empty.
    /// This function supports auto-commit.
    /// \return True if empty, false otherwise.
    bool empty() const
    {
        // If we fail to move to the first record, the db is 
        // supposed to be empty.
        const_iterator witr;
        bool ret;

        try {
            this->begin_txn();
            init_itr(witr);
            open_itr(witr, true);
            ret = witr.first() != 0;
            this->commit_txn();
            return ret;
        } catch (...) {
            this->abort_txn();
            throw;
        }
    }
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin element accessors.
    //
    // Don't use transaction wrapper(begin/commit_txn) here because 
    // even insert(the only logged operation) is only part of the 
    // whole expression---the use case is dmmap[key] = value; 
    // So insert and another put call should
    // be atomic, so there must be an outside transaction.
    //
    // As stated in STL specification, this method can't have "const" 
    // modifier because it is likely to insert a new record.
    /// Retrieve data element by key.
    /// This function returns an reference to the underlying data element
    /// of the specified key x. The returned object can be used to read or
    /// write the data element of the key/data pair. 
    /// Do use a data_type_wrap of db_map or value_type::second_type(they 
    /// are the same) type of variable to hold the return value of this 
    /// function.
    /// \param x The target key to get value from.
    /// \return Data element reference.
    //
    data_type_wrap operator[] (const key_type& x)
    {
        iterator witr, *pitr;
        int ret;

        init_itr(witr);
        open_itr(witr, false);
        
        if (witr.move_to(x) != 0) {
            ddt d;//default value
            DbstlInitializeDefault<ddt> initdef(d);
            // Insert (x, d) as place holder.
            witr.pcsr_->insert(x, d, DB_KEYLAST);
            // Should be OK this time.
            ret = witr.move_to(x);
            assert(ret == 0);
            // Return the reference to the data item of x.
        }
        
        //witr->curpair_.second._DB_STL_SetDelItr();
        pitr = new iterator(witr);
        data_type_wrap ref(pitr->curpair_.second);
        ref._DB_STL_SetDelItr();
        return ref;
    }

    // Only returns a right-value, no left value for assignment, so 
    // directly return the value rather than the ElementRef/ElementHolder 
    // wrapper. Must use a const reference to this container to call this
    // const function.
    //
    /// Retrieve data element by key.
    /// This function returns the value of the underlying data element of
    /// specified key x. You can only read the element, but unable to 
    /// update the element via the return value of this function. And you
    /// need to use the container's const reference to call this method.
    /// \param x The target key to get value from.
    /// \return Data element, read only, can't be used to modify it.
    const ddt operator[] (const key_type& x) const
    {
        iterator witr;

        init_itr(witr);
        open_itr(witr);
        
        // x is supposed to be in this map.
        if (witr.move_to(x) != 0) {
            THROW0(NoSuchKeyException);
            
        }
        return witr.curpair_.second._DB_STL_value();
    }
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that erase elements from the container.
    //
    // Can not reopen external/outside iterator's cursor, pos must
    // already be in a transactional context.
    // There is identical function in db_multimap<> and db_multiset
    // for this function, we MUST keep the code consistent on update!
    // Go to db_multimap<>::erase(const key_type&) to see why.
    /// \name Erase Functions
    /// \sa http://www.cplusplus.com/reference/stl/map/erase/
    //@{
    /// Erase a key/data pair at specified position.
    /// \param pos An valid iterator of this container to erase.
    inline void erase (iterator pos) 
    {
        if (pos == end())
            return;
        pos.pcsr_->del();
    }

    /// Erase elements by key.
    /// All key/data pairs with specified key x will be removed from 
    /// underlying database.
    /// This function supports auto-commit.
    /// \param x The key to remove from the container.
    /// \return The number of key/data pairs removed.
    // There is identical function in db_multimap<> and db_multiset
    // for this function, we MUST keep the code consistent on update!
    // Go to db_multimap<>::erase(const key_type&) to see why.
    //
    size_type erase (const key_type& x) 
    {
        size_type cnt;
        iterator itr;

        this->begin_txn();
        try {
            pair<iterator, iterator> rg = equal_range(x);
            for (itr = rg.first, cnt = 0; itr != rg.second; ++itr) {
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

    // Can not be auto commit because first and last are already open.
    // There is identical function in db_multimap<> and db_multiset
    // for this function, we MUST keep the code consistent on update!
    // Go to db_multimap<>::erase(const key_type&) to see why.
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
    
    /// Swap content with container mp.
    /// This function supports auto-commit.
    /// \param mp The container to swap content with.
    /// \param b_truncate: See db_vector::swap() for details.
    /// \sa http://www.cplusplus.com/reference/stl/map/swap/ 
    /// db_vector::clear()
    void swap (db_map<kdt, ddt, value_type_sub>& mp, bool b_truncate = true)
    {
        Db *swapdb = NULL;
        std::string dbfname(64, '\0');
        
        verify_db_handles(mp);
        this->begin_txn();
        try {
            swapdb = this->clone_db_config(this->get_db_handle(), 
                dbfname);
            db_map<kdt, ddt, value_type_sub> tmap(swapdb, 
                swapdb->get_env(), begin(), end());
            clear(b_truncate);// Clear this db_map<> object.
            typename db_map<kdt, ddt, value_type_sub>::
                iterator itr1, itr2;
            itr1 = mp.begin();
            itr2 = mp.end();
            insert(itr1, itr2);
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
        } catch (...) {
            this->abort_txn();
            throw;
        }   
        this->commit_txn();
    }

    /// Clear contents in this container.
    /// This function supports auto-commit.
    /// \param b_truncate See db_vector::clear(bool) for details.
    /// \sa db_vector::clear(bool)
    void clear (bool b_truncate = true) 
    {
        int ret;
        u_int32_t flag;
        DbEnv *penv = this->get_db_handle()->get_env();

        if (b_truncate) {
            ResourceManager::instance()->close_db_cursors(
                this->get_db_handle());
            BDBOP2(this->get_db_handle()->truncate(
                ResourceManager::instance()->current_txn(penv), 
                NULL, 0), ret, this->abort_txn());
        } else {
            ReadModifyWriteOption brmw(
                ReadModifyWriteOption::no_read_modify_write());

            BDBOP(penv->get_open_flags(&flag), ret);

            // DB_RMW flag requires locking subsystem started.
            if ((flag & DB_INIT_LOCK) || (flag & DB_INIT_CDB) || 
                (flag & DB_INIT_TXN))
                brmw = 
                    ReadModifyWriteOption::read_modify_write();
            try {
                // In if branch, truncate is capable of 
                // autocommit internally.
                this->begin_txn();
                erase(begin(brmw, false), end());
                this->commit_txn();
            } catch (...) {
                this->abort_txn();
                throw;
            }
        }
    }
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that searches a key in the map.
    /// \name Searching Functions
    /// The following functions are returning iterators, and they by 
    /// default return read-write iterators. If you intend to use the 
    /// returned iterator only to read, you should call the const version
    /// of each function using a const reference to this container.
    /// Using const iterators can potentially promote concurrency a lot.
    /// You can also set the readonly parameter to each non-const version
    /// of the functions to true if you don't use the returned iterator
    /// to write, which also promotes concurrency and overall performance.
    //@{
    /// Find the key/data pair with specified key x. 
    /// \param x The target key to find.
    /// \return The valid const iterator sitting on the key x, or an 
    /// invalid one.
    /// \sa http://www.cplusplus.com/reference/stl/map/find/
    const_iterator find (const key_type& x) const
    {
        const_iterator witr;

        init_itr(witr);
        open_itr(witr, true);
        if (witr.move_to(x)) 
            return ((self *)this)->end();

        return witr;
    }

    /// Find the greatest key less than or equal to x.
    /// \param x The target key to find.
    /// \return The valid const iterator sitting on the key, or an 
    /// invalid one.
    /// \sa http://www.cplusplus.com/reference/stl/map/lower_bound/
    const_iterator lower_bound (const key_type& x) const 
    {
        const_iterator witr;

        init_itr(witr);
        open_itr(witr, true);
        if (witr.move_to(x, DB_SET_RANGE)) 
            return ((self *)this)->end();
        
        return witr;
    }

    /// Find the range within which all keys equal to specified key x.
    /// \param x The target key to find.
    /// \return The range [first, last).
    /// \sa http://www.cplusplus.com/reference/stl/map/equal_range/
    pair<const_iterator, const_iterator>
    equal_range (const key_type& x) const 
    {
        pair<const_iterator,const_iterator> pr;
        const_iterator witr;
        kdt k;

        init_itr(witr);
        open_itr(witr, true);
        if (witr.move_to(x, DB_SET_RANGE)) {
            pr.first = ((self *)this)->end();
            pr.second = ((self *)this)->end();
        } else {
            pr.first = witr;
            // If no duplicate keys, move one next is sufficient.
            if (witr.pcsr_->get_current_key(k) == 0 && k == x)
                ++witr;
            pr.second = witr;
        }
        

        return pr;
    }

    /// Find the key/data pair with specified key x. 
    /// \param x The target key to find.
    /// \param readonly Whether the returned iterator is readonly.
    /// \return The valid iterator sitting on the key x, or an 
    /// invalid one.
    /// \sa http://www.cplusplus.com/reference/stl/map/find/
    iterator find (const key_type& x, bool readonly = false) 
    {
        iterator witr;

        init_itr(witr);
        open_itr(witr, readonly);
        if (witr.move_to(x)) 
            return ((self *)this)->end();

        return witr;
    }

    /// Find the greatest key less than or equal to x.
    /// \param x The target key to find.
    /// \param readonly Whether the returned iterator is readonly.
    /// \return The valid iterator sitting on the key, or an 
    /// invalid one.
    /// \sa http://www.cplusplus.com/reference/stl/map/lower_bound/
    iterator lower_bound (const key_type& x, bool readonly = false) 
    {
        iterator witr;

        init_itr(witr);
        open_itr(witr, readonly);
        if (witr.move_to(x, DB_SET_RANGE)) 
            return ((self *)this)->end();
        
        return witr;
    }

    /// Find the range within which all keys equal to specified key x.
    /// \param x The target key to find.
    /// \param readonly Whether the returned iterator is readonly.
    /// \return The range [first, last).
    /// \sa http://www.cplusplus.com/reference/stl/map/equal_range/
    pair<iterator, iterator>
    equal_range (const key_type& x, bool readonly = false) 
    {
        pair<iterator,iterator> pr;
        iterator witr;
        kdt k;

        init_itr(witr);
        open_itr(witr, readonly);
        if (witr.move_to(x, DB_SET_RANGE)) {
            pr.first = ((self *)this)->end();
            pr.second = ((self *)this)->end();
        } else {
            pr.first = witr;
            // If no dup, move one next is sufficient.
            if (witr.pcsr_->get_current_key(k) == 0 && k == x)
                ++witr;
            pr.second = witr;
        }
        

        return pr;
    }

    /// Count the number of key/data pairs having specified key x.
    /// \param x The key to count.
    /// \return The number of key/data pairs having x as key within the
    /// container.
    /// \sa http://www.cplusplus.com/reference/stl/map/count/
    size_type count (const key_type& x) const
    {
        int ret;
        const_iterator witr;
        try {
            this->begin_txn();
            init_itr(witr);
            open_itr(witr, true);
            ret = witr.move_to(x);
            this->commit_txn();
            if (ret != 0) 
                return 0;// No such key/data pair.
            // No duplicates, so it must be one, we don't call
            // Dbc::count because we don't have to.
            //
            else
                return 1;
        } catch (...) {
            this->abort_txn();
            throw;
        }
        
    }

    /// Find the least key greater than x.
    /// \param x The target key to find.
    /// \return The valid iterator sitting on the key, or an 
    /// invalid one.
    /// \sa http://www.cplusplus.com/reference/stl/map/upper_bound/
    const_iterator upper_bound (const key_type& x) const
    {
        const_iterator witr;

        init_itr(witr);
        open_itr(witr, true);

        if (witr.move_to(x, DB_SET_RANGE)) 
            return ((self *)this)->end();

        kdt k;

        // x exists in db, and witr.pcsr_ points to x in db.
        if (witr.pcsr_->get_current_key(k) == 0 && k == x)
            ++witr;// No dup, so move one next is sufficient.

        return witr;
    }

    /// Find the least key greater than x.
    /// \param x The target key to find.
    /// \param readonly Whether the returned iterator is readonly.
    /// \return The valid iterator sitting on the key, or an 
    /// invalid one.
    /// \sa http://www.cplusplus.com/reference/stl/map/upper_bound/
    iterator upper_bound (const key_type& x, bool readonly = false)
    {
        iterator witr;

        init_itr(witr);
        open_itr(witr, readonly);

        if (witr.move_to(x, DB_SET_RANGE)) 
            return ((self *)this)->end();

        kdt k;

        // x exists in db, and witr.pcsr_ points to x in db.
        if (witr.pcsr_->get_current_key(k) == 0 && k == x)
            ++witr;// No dup, so move one next is sufficient.

        return witr;
    }
    //@}
    ////////////////////////////////////////////////////////////////

    // Compare function, return true if contents in m1 and m2 are 
    // identical otherwise return false. 
    // Note that we don't require the key-data pairs' order be identical
    // Put into db_map<> rather than global to utilize transactional
    // support.
    /// Map content equality comparison operator.
    /// This function does not rely on key order. For a set of keys S1 in
    /// this container and another set of keys S2 of container m2, if 
    /// set S1 contains S2 and S2 contains S1 (S1 equals to S2) and each
    /// data element of a key K in S1 from this container equals the data
    /// element of K in m2, the two db_map<> containers equal. Otherwise
    /// they are not equal.
    /// \param m2 The other container to compare against.
    /// \return Returns true if they have equal content, false otherwise.
    bool operator==(const db_map<kdt, ddt, value_type_sub>& m2) const
    {
        bool ret;
        const db_map<kdt, ddt, value_type_sub>& m1 = *this;
        COMPARE_CHECK(m2)
        verify_db_handles(m2);
        try {
            this->begin_txn();
            if (m1.size() != m2.size())
                ret = false;
            else {
                typename db_map<kdt, ddt, value_type_sub>::
                    const_iterator i1, i2;
                
                for (i1 = m1.begin(); i1 != m1.end(); ++i1) {
                    if (m2.count(i1->first) == 0) {
                        ret = false;
                        goto exit;
                    }
                    i2 = m2.find(i1->first);
                    if ((i2->second == i1->second) == 
                        false) {
                        ret = false;
                        goto exit;
                    }
                } // for

                ret = true;
            }
exit:
            this->commit_txn();
            return ret;

        } catch (...) {
            this->abort_txn();
            throw;
        }
    // Now that m1 and m2 has the same number of unique elements and all
    // elements of m1 are in m2, thus there can be no element of m2 
    // that dose not belong to m1, so we won't verify each element of 
    // m2 are in m1.
    //
    }

    /// Container unequality comparison operator.
    /// \param m2 The container to compare against.
    /// \return Returns false if equal, true otherwise.
    bool operator!=(const db_map<kdt, ddt, value_type_sub>& m2) const
    {
        return !this->operator ==(m2);
    }

    
protected:
    
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
        if ((sflags & DB_DUP) || (sflags & DB_DUPSORT))
            err = 
"db_map<> can not be backed by database permitting duplicate keys";
        if (sflags & DB_RECNUM)
            err = "no DB_RECNUM flag allowed in db_map<>";
        
        
        return err;

    }


    typedef ddt mapped_type;
    typedef int (*db_compare_fcn_t)(Db *db, const Dbt *dbt1, 
        const Dbt *dbt2);
    typedef u_int32_t (*h_hash_fcn_t)
    (Db *, const void *bytes, u_int32_t length);
    typedef db_set_iterator<kdt> db_multiset_iterator_t;

    static bool compare_keys(Db *pdb, const kdt& k1, const kdt& k2) 
    {
        DBTYPE dbtype;
        int ret;
        bool bret;
        u_int32_t sz1, sz2;

        assert(pdb != NULL);
        ret = pdb->get_type(&dbtype);
        assert(ret == 0);
        db_compare_fcn_t comp = NULL;

        if (dbtype == DB_BTREE)
            BDBOP(pdb->get_bt_compare(&comp), ret);
        else // hash
            BDBOP(pdb->get_h_compare(&comp), ret);

        DataItem key1(k1, true), key2(k2, true);
        Dbt &kdbt1 = key1.get_dbt();
        Dbt &kdbt2 = key2.get_dbt();
        sz1 = kdbt1.get_size();
        sz2 = kdbt2.get_size();
        
        if (comp == NULL) {
            ret = memcmp(&k1, &k2, sz1 > sz2 ? sz2 : sz1);
            return (ret == 0) ? (sz1 < sz2) : (ret < 0);
        }
        // Return strict weak ordering.
        bret = (comp(pdb, &kdbt1, &kdbt2) < 0);
        return bret;
    }
    
    void open_itr(db_map_base_iterator<kdt, realddt, ddt>&itr, 
        bool readonly = false) const
    {   
        u_int32_t oflags = 0;
        int ret;
        DbEnv *penv = this->get_db_handle()->get_env();

        if (!readonly && penv != NULL) {
            BDBOP((penv->get_open_flags(&oflags)) , ret);
             if ((oflags & DB_INIT_CDB) != 0)
                ((self *)this)->set_cursor_open_flags(
                    this->get_cursor_open_flags() | 
                    DB_WRITECURSOR);
        }

        itr.itr_status_ = itr.pcsr_->open((db_container*)this, 
            this->get_cursor_open_flags());
        itr.owner_ = (db_container*)this;
    }

    void open_itr(const_reverse_iterator
        &itr, bool readonly = false) const
    {   
        u_int32_t oflags = 0;
        int ret;
        DbEnv *penv = this->get_db_handle()->get_env();

        if (!readonly && penv != NULL) {
            BDBOP((penv->get_open_flags(&oflags)) , ret);
             if ((oflags & DB_INIT_CDB) != 0)
                ((self *)this)->set_cursor_open_flags(
                    this->get_cursor_open_flags() | 
                    DB_WRITECURSOR);
        }
        itr.itr_status_ = itr.pcsr_->open((db_container*)this, 
            this->get_cursor_open_flags());
        itr.owner_ = (db_container*)this;
    }

    inline void init_itr(db_map_base_iterator<kdt, realddt, ddt> &
        witr) const {
        typedef DbCursor<kdt, ddt> cursor_type;
        witr.pcsr_.set_cursor(new cursor_type());
        witr.owner_ = (db_container*)this;
    }

    // Do not use begin_txn/commit_txn in non-public(internal) methods, 
    // only wrap in public methods.
    //
    inline void copy_db(db_map<kdt, ddt, value_type_sub, iterator> &x)
    {    

        // Make sure clear can succeed if there are cursors 
        // open in other threads.
        clear(false);
        insert(x.begin(), x.end());
        
    }

};//db_map
//@}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// db_multimap class template definition
//
// This class derives from db_map<>, using many of its methods,
// it also hides some functions that should not be used in
// this class, such as operator[].
//
// The underlying db must allow duplicate.
// iterator_t is default argument, see forward declaration at the
// head of this file.
// iterator_t is default argument, see forward declaration at the
// head of this file
//
/// \ingroup dbstl_containers
//@{
/// This class is the combination of std::multimap and hash_multimap. By 
/// setting database handles as DB_BTREE or DB_HASH type respectively, you
/// will be using an equivalent of std::multimap or hash_multimap respectively.
/// Database(dbp) and environment(penv) handle requirement:
/// The dbp handle must meet the following requirement:
/// 1. Database type should be DB_BTREE or DB_HASH.
/// 2. Either DB_DUP or DB_DUPSORT flag must be set. Note that so far 
/// Berkeley DB does not allow DB_DUPSORT be set and the database is storing
/// identical key/data pairs, i.e. we can't store two (1, 2), (1, 2) pairs 
/// into a database D with DB_DUPSORT flag set, but only can do so with DB_DUP
/// flag set; But we can store a (1, 2) pair and a (1, 3) pair into D with
/// DB_DUPSORT flag set. So if your data set allows DB_DUPSORT flag, you
/// should set it to gain a lot of performance promotion.
/// 3. No DB_RECNUM flag set.
/// 4. No DB_TRUNCATE specified in database open flags.
/// 5. DB_THREAD must be set if you are sharing the database handle across 
/// multiple threads directly, or indirectly by sharing the container object
/// across multiple threads.
/// \param kdt The key data type.
/// \param ddt The data data type. db_multimap stores key/data pairs.
/// \param value_type_sub Do not specify anything if ddt type is a 
/// class/struct type; Otherwise, specify ElementHolder<ddt> to it.
/// \param iterator_t Never specify anything to this type parameter. It is
/// only used internally.
/// \sa db_container db_map
template<Typename kdt, Typename ddt, Typename value_type_sub, 
    Typename iterator_t>
class _exported db_multimap : public db_map<kdt, ddt, 
    value_type_sub, iterator_t> 
{
protected:
    typedef db_multimap<kdt, ddt, value_type_sub, iterator_t> self;
    typedef db_map<kdt, ddt, value_type_sub, iterator_t> base;
public:
    typedef iterator_t iterator;
    typedef typename iterator::const_version const_iterator;
    typedef db_reverse_iterator<iterator, const_iterator> reverse_iterator;
    typedef db_reverse_iterator<const_iterator, iterator> 
        const_reverse_iterator;
    typedef kdt key_type;
    typedef ddt data_type;
    typedef value_type_sub data_type_wrap;
    typedef pair<kdt, value_type_sub> value_type_wrap;
    typedef pair<kdt, ddt> value_type;
    typedef value_type_wrap* pointer;
    typedef value_type_wrap& reference; 
    typedef const value_type& const_reference; 
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    ////////////////////////////////////////////////////////////////
    // Begin constructors and destructor
    /// Constructor.
    /// See class detail for handle requirement.
    /// \param dbp The database handle.
    /// \param envp The database environment handle.
    /// \sa db_map::db_map(Db*, DbEnv*) db_vector::db_vector(Db*, DbEnv*)
    explicit db_multimap (Db *dbp = NULL, DbEnv* envp = NULL) : 
        base(*(new BulkRetrievalOption(
        BulkRetrievalOption::BulkRetrieval)))
    {
        const char *errmsg;

        this->init_members(dbp, envp);
        this->open_db_handles(dbp, envp, DB_BTREE, DB_CREATE | 
            DB_THREAD, DB_DUP);
        // We can't call base(dbp, envp) here because it will verify 
        // failed and we can't call db_container directly, it is 
        // illegal to do so.
        if ((errmsg = verify_config(dbp, envp)) != NULL) {
            THROW(InvalidArgumentException, ("Db*", errmsg));
            
        }
        this->set_db_handle_int(dbp, envp);
        this->set_auto_commit(dbp);
    }

    /// Iteration constructor.
    /// Iterates between first and last, setting
    /// a copy of each of the sequence of elements as the content of 
    /// the container object. 
    /// This function supports auto-commit.
    /// See class detail for handle requirement.
    /// \param dbp The database handle.
    /// \param envp The database environment handle.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    /// \sa db_map::db_map(Db*, DbEnv*, InputIterator, InputIterator)
    /// db_vector::db_vector(Db*, DbEnv*)
    //
    template <class InputIterator> 
    db_multimap (Db *dbp, DbEnv* envp, InputIterator first, 
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
    }

    /// Copy constructor.
    /// Create an database and insert all key/data pairs in x into this
    /// container. x's data members are not copied.
    /// This function supports auto-commit.
    /// \param x The other container to initialize this container.
    /// \sa db_container(const db_container&) db_map(const db_map&)
    db_multimap (const self& x) : base(*(new BulkRetrievalOption(
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
    }

    virtual ~db_multimap(){}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that modify multimap content, e.g. insert, 
    // erase, assignment and swap.
    //
    /// Container content assignment operator.
    /// This function supports auto-commit.
    /// \param x The other container whose key/data pairs will be inserted
    /// into this container. Old content in this containers are discarded.
    /// \sa http://www.cplusplus.com/reference/stl/multimap/operator=/
    inline const self& operator=(const self&x)
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

    /// \name Insert Functions
    /// \sa http://www.cplusplus.com/reference/stl/multimap/insert/
    //@{
    /// Range insertion. Insert a range [first, last) of key/data pairs
    /// into this container.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    template<typename InputIterator>
    void insert (InputIterator first, InputIterator last) 
    {
        InputIterator ii;
        iterator witr;

        init_itr(witr);
        open_itr(witr);

        for (ii = first; ii != last; ++ii)
            witr.pcsr_->insert(ii->first, ii->second, 
                DB_KEYLAST);    
    }

    // Compiler can't see the inherited version, unknown why.
    /// Range insertion. Insert a range [first, last) of key/data pairs
    /// into this container.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    inline void insert (const_iterator& first, const_iterator& last) {
        base::insert(first, last);
    }

    // Insert x into this container, the other two versions are 
    // inherited from db_map<> class.
    // Methods returning an iterator or using an iterator as parameter 
    // can not be internally wrapped by 
    // begin/commit_txn because a cursor is inside its transaction, it 
    // must have been closed after transaction commit, and reopen is 
    // unsafe in multithreaded access.
    //
    /// Insert a single key/data pair if the key is not in the container.
    /// \param x The key/data pair to insert.
    /// \return A pair P, if insert OK, i.e. the inserted key wasn't in the
    /// container, P.first will be the iterator sitting on the inserted
    /// key/data pair, and P.second is true; otherwise P.first is an 
    /// invalid iterator and P.second is false.
    inline iterator insert (const value_type& x) 
    {
        iterator witr;

        this->init_itr(witr);
        this->open_itr(witr);
        witr.itr_status_ = witr.pcsr_->insert(x.first, x.second, 
            DB_KEYLAST);
        witr.refresh(false);
        return witr;
    }
    //@}

    /// Swap content with another multimap container.
    /// This function supports auto-commit.
    /// \param mp The other container to swap content with.
    /// \param b_truncate See db_map::swap() for details.
    /// \sa db_vector::clear()
    void swap (db_multimap<kdt, ddt, value_type_sub>& mp, 
        bool b_truncate = true)
    {
        Db *swapdb = NULL;
        std::string dbfname(64, '\0');

        verify_db_handles(mp);
        this->begin_txn();
        try {
            swapdb = this->clone_db_config(this->get_db_handle(), 
                dbfname);
            
            db_multimap<kdt, ddt, value_type_sub> tmap(
                swapdb, swapdb->get_env(),
                this->begin(), this->end());
            // Clear this db_multimap<> object.
            this->clear(b_truncate);
            typename db_multimap<kdt, ddt, value_type_sub>::
                iterator mpbitr, mpeitr;

            mpbitr = mp.begin();
            mpeitr = mp.end();
            insert(mpbitr, mpeitr);
            mp.clear(b_truncate);
            mpbitr = tmap.begin();
            mpeitr = tmap.end();
            mp.insert(mpbitr, mpeitr);
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

    // This method has identical code to db_map<>::erase(const key_type&), 
    // but we can NOT simply inherit and use 
    // that version because:
    // 1. The db_map<>::erase called equal_range which is overloaded in 
    //  db_multimap, so if we want the inherited erase to call the right
    //  version of equal_range, we have to make equal_range virtual
    // 2.  Making equal_range virtual will make the code not build--- The 
    //  default template parameter can't be replaced by real parameter, 
    //  unknow why.
    // So we have to copy the code from db_map<> to here, and keep the 
    // code consistent on each update.
    // Also, when I copy only this function, I found other erase overloaded 
    // functions also have to be copied from db_map<> to db_multimap and 
    // db_multiset, otherwise the code don't build, so I 
    // finally have to copy all versions of erase functions into db_multiset
    // and db_multimap. When updating an erase function, do update all 
    // three versions.
    /// \name Erase Functions
    /// \sa http://www.cplusplus.com/reference/stl/multimap/erase/
    //@{
    /// Erase elements by key.
    /// All key/data pairs with specified key x will be removed from 
    /// underlying database.
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
            for (itr = rg.first, cnt = 0; itr != rg.second; ++itr) {
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
    // already be in a transactional context.
    // There is identical function in db_multimap<> and db_multiset
    // for this function, we MUST keep the code consistent on update!
    // Go to db_multimap<>::erase(const key_type&) to see why.
    //
    /// Erase a key/data pair at specified position.
    /// \param pos An valid iterator of this container to erase.
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

        for (iterator i = first; i != last; ++i)
            i.pcsr_->del();
    }
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that searches a key in the multimap.
    /// \name Searching Functions
    /// See of db_map's searching functions group for details about 
    /// iterator, function version and parameters.
    /// \sa db_map
    //@{
    /// Find the range within which all keys equal to specified key x.
    /// \param x The target key to find.
    /// \return The range [first, last).
    /// \sa http://www.cplusplus.com/reference/stl/multimap/equal_range/
    pair<const_iterator, const_iterator>
    equal_range (const key_type& x) const
    {
        pair<const_iterator,const_iterator> pr;
        const_iterator witr;
        kdt k;

        this->init_itr(witr);
        this->open_itr(witr, true);
        // Move witr to x if this contains x and return the itr, or if 
        // no x, position witr to the least key greater than x.
        //
        if (witr.move_to(x, DB_SET_RANGE)) {
            pr.first =  ((self *)this)->end();
            pr.second = ((self *)this)->end();
        } else {
            pr.first = witr;
            
            // No dup, so move one next is sufficient.
            if (witr.pcsr_->get_current_key(k) == 0 && k == x)
                witr.next(DB_NEXT_NODUP);
            pr.second = witr;
        }
            
        return pr;
    }

    /// Find the range within which all keys equal to specified key x.
    /// \param x The target key to find.
    /// \param readonly Whether the returned iterator is readonly.
    /// \return The range [first, last).
    /// \sa http://www.cplusplus.com/reference/stl/multimap/equal_range/
    pair<iterator,iterator>
    equal_range (const key_type& x, bool readonly = false) 
    {
        pair<iterator,iterator> pr;
        iterator witr;
        kdt k;

        this->init_itr(witr);
        this->open_itr(witr, readonly);
        // Move witr to x if this contains x and return the itr, or if 
        // no x, position witr to the least key greater than x.
        //
        if (witr.move_to(x, DB_SET_RANGE)) {
            pr.first =  ((self *)this)->end();
            pr.second = ((self *)this)->end();
        } else {
            pr.first = witr;
            
            // No dup, so move one next is sufficient.
            if (witr.pcsr_->get_current_key(k) == 0 && k == x)
                witr.next(DB_NEXT_NODUP);
            pr.second = witr;
        }
            
        return pr;
    }

    /// Find equal range and number of key/data pairs in the range.
    /// This function also returns the number of elements within the 
    /// returned range via the out parameter nelem.
    /// \param x The target key to find.
    /// \param nelem The output parameter to take back the number of 
    /// key/data pair in the returned range.
    /// \sa http://www.cplusplus.com/reference/stl/multimap/equal_range/
    pair<const_iterator, const_iterator>
    equal_range_N (const key_type& x, size_t& nelem) const
    {
        int ret;
        pair<const_iterator,const_iterator> pr;
        size_t stepped;
        const_iterator witr;
        kdt k;

        this->init_itr(witr);
        this->open_itr(witr, true);
        // Move witr to x if this contains x and return the itr, or if 
        // no x, position witr to the least key greater than x.
        //
        if (witr.move_to(x, DB_SET_RANGE)) {
            pr.first = ((self *)this)->end();
            pr.second = ((self *)this)->end();
            nelem = 0;
        } else {
            pr.first = witr;
            if (witr.pcsr_->get_current_key(k) == 0 && k == x) {
                for (stepped = 1, ret = 
                    witr.pcsr_->next(DB_NEXT_DUP); ret == 0;
                    ret = witr.pcsr_->next(DB_NEXT_DUP),
                    stepped += 1)
                    ;
                pr.second = ++witr;
                nelem = stepped;
            } else {
                pr.second = witr;
                nelem = 0;
            }
        }
        return pr;
    }

    /// Find equal range and number of key/data pairs in the range.
    /// This function also returns the number of elements within the 
    /// returned range via the out parameter nelem.
    /// \param x The target key to find.
    /// \param nelem The output parameter to take back the number of 
    /// key/data pair in the returned range.
    /// \param readonly Whether the returned iterator is readonly.
    /// \sa http://www.cplusplus.com/reference/stl/multimap/equal_range/
    //
    pair<iterator,iterator>
    equal_range_N (const key_type& x, size_t& nelem, 
        bool readonly = false)
    {
        int ret;
        pair<iterator,iterator> pr;
        size_t stepped;
        iterator witr;
        kdt k;

        this->init_itr(witr);
        this->open_itr(witr, readonly);
        // Move witr to x if this contains x and return the itr, or if 
        // no x, position witr to the least key greater than x.
        //
        if (witr.move_to(x, DB_SET_RANGE)) {
            pr.first = ((self *)this)->end();
            pr.second = ((self *)this)->end();
            nelem = 0;
        } else {
            pr.first = witr;
            if (witr.pcsr_->get_current_key(k) == 0 && k == x) {
                for (stepped = 1, ret = 
                    witr.pcsr_->next(DB_NEXT_DUP); ret == 0;
                    ret = witr.pcsr_->next(DB_NEXT_DUP),
                    stepped += 1)
                    ;
                pr.second = ++witr;
                nelem = stepped;
            } else {
                pr.second = witr;
                nelem = 0;
            }
        }
        return pr;
    }

    /// Count the number of key/data pairs having specified key x.
    /// \param x The key to count.
    /// \return The number of key/data pairs having x as key within the
    /// container.
    /// \sa http://www.cplusplus.com/reference/stl/multimap/count/
    size_type count (const key_type& x) const
    {
        int ret;
        size_type cnt;
        iterator witr;

        try {
            this->begin_txn();
            this->init_itr(witr);
            this->open_itr(witr, true);
            ret = witr.move_to(x);
            if (ret) 
                cnt = 0;
            else
                cnt = witr.pcsr_->count();
            this->commit_txn();
        } catch (...) {
            this->abort_txn();
            throw;
        }

        return cnt;
    }

    /// Find the least key greater than x.
    /// \param x The target key to find.
    /// \return The valid iterator sitting on the key, or an 
    /// invalid one.
    /// \sa http://www.cplusplus.com/reference/stl/multimap/upper_bound/
    const_iterator upper_bound (
        const key_type& x) const
    {
        int ret;
        const_iterator witr;

        this->init_itr(witr);
        this->open_itr(witr, true);

        // No key equal to or greater than x.
        if (witr.move_to(x, DB_SET_RANGE)) 
            return ((self *)this)->end();

        kdt k;
        // x exists in db, and witr.pcsr_ points to x in db, 
        // need to move cursor to next different key.
        //
        if (witr.pcsr_->get_current_key(k) == 0 && k == x) 
            ret = witr.next(DB_NEXT_NODUP); 

        return witr;
    }

    /// Find the least key greater than x.
    /// \param x The target key to find.
    /// \param readonly Whether the returned iterator is readonly.
    /// \return The valid iterator sitting on the key, or an 
    /// invalid one.
    /// \sa http://www.cplusplus.com/reference/stl/multimap/upper_bound/
    iterator upper_bound (const key_type& x, bool readonly = false) 
    {
        int ret;
        iterator witr;

        this->init_itr(witr);
        this->open_itr(witr, readonly);

        // No key equal to or greater than x.
        if (witr.move_to(x, DB_SET_RANGE)) 
            return ((self *)this)->end();

        kdt k;
        // x exists in db, and witr.pcsr_ points to x in db, 
        // need to move cursor to next different key.
        //
        if (witr.pcsr_->get_current_key(k) == 0 && k == x) 
            ret = witr.next(DB_NEXT_NODUP); 

        return witr;
    }
    //@}
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    // Begin functions that compare container content.
    //
    // In hash_multimap this function is a global compare function, 
    // return true if contents in m1 and m2 are identical
    // otherwise return false. But we have multiple reasons to make
    // it a member of db_multimap<>:
    // 1. There need to be a temporary set to store values of a range, and
    //   db_multimap<>  is quite likely to store huge amount of data, 
    //   not suitable to store into std::set, let alone std::set is not 
    //   completely multithread-safe, thus we store them into db_set<>, 
    //   thus we need a temporary db handle, and call 
    //   db_container::clone_db_handle() function to open the db handle.
    // 2. We need the transactional support. Making this function 
    //   autocommit is good to eliminate phantom issues.
    // Note that we don't require the key-data pairs' order be identical,
    // but we assume identical records of keys are adjacent, so that 
    // iteration will go through them all one by one; Also, the records'
    // order of identical keys are unpridictable and irrelivent, so we 
    // should treat values of a equal range a set, and compare two value 
    // sets for equality when comparing a equal range of key X.
    //
    /**
    Returns whether the two containers have identical content.
    This function does not rely on key order. For a set of keys S1 in this
    container and another set of keys S2 of container m2, if set S1 
    contains S2 and S2 contains S1 (S1 equals to S2) and each set of data
    elements of any key K in S1 from this container equals the set of data
    elements of K in m2, the two db_multimap<> containers equal. Otherwise
    they are not equal. Data element set comparison does not rely on order
    either.
    \param m2 The other container to compare against.
    \return Returns true if they are equal, false otherwise.
    */
    bool operator==(const db_multimap<kdt, ddt, value_type_sub>& m2) const
    {
        
        typedef typename self::const_iterator mm_itr_t;
        
        COMPARE_CHECK(m2)
        
        bool ret = false, retset = false;
        size_t n1, n2;
        int ret2;
        const self &m1 = *this;
        DbTxn *ptxn = NULL;
        DbEnv *penv;
        Db *pdb;
        const char *dbfilename, *dbname;
        const char *pname1, *pname2;
        string name1, name2;
        u_int32_t oflags;

        verify_db_handles(m2);
        pdb = this->get_db_handle();
        penv = pdb->get_env();
        try {
            this->begin_txn();
            if (m1.size() != m2.size()) {
                ret = false;
                this->commit_txn();
                return ret;
            }
            BDBOP(pdb->get_dbname(&dbfilename, &dbname), ret2);
            if (dbfilename == NULL)
                pname1 = pname2 = NULL;
            else {

                this->construct_db_file_name(name1);
                this->construct_db_file_name(name2);
                // Make name2 different from name1.
                name2.push_back('2');
                pname1 = name1.c_str();
                pname2 = name2.c_str();
            }

            Db *value_set_db = open_db(penv, 
                pname1, DB_BTREE, DB_CREATE, 0);

            Db *value_set_db2 = open_db(penv, 
                pname2, DB_BTREE, DB_CREATE, 0);

            db_set<ddt, value_type_sub> s1(value_set_db, penv), 
                s2(value_set_db2, penv);

            mm_itr_t i1, i11;
            pair<mm_itr_t, mm_itr_t> resrg1, resrg2;
            for (i1 = m1.begin(); 
                i1 != m1.end();
                i1 = resrg1.second) {
                
                resrg1 = m1.equal_range_N(i1->first, n1);
                resrg2 = m2.equal_range_N(i1->first, n2);
                if (n1 != n2) {
                    ret = false;
                    retset = true;
                    break;
                }
                    
                if (n2 == 1 && !(resrg2.first->second == 
                    resrg1.first->second)) {
                    ret = false;
                    retset = true;
                    break;
                }
                
                for (i11 = resrg1.first; i11 != resrg1.second;
                    ++i11) 
                    s1.insert(i11->second);

                for (i11 = resrg2.first; i11 != resrg2.second;
                    ++i11)
                    s2.insert(i11->second);
                if (!(s1 == s2)) {
                    ret = false;
                    retset = true;
                    break;
                }
                s1.clear();
                s2.clear();

                // Skip all equal keys in the range.
                
                
            } // for

            if (!retset) // Care: there are breaks in the for loop.
                ret = true;

            close_db(value_set_db);
            close_db(value_set_db2);

            ptxn = this->current_txn();
            BDBOP(penv->get_open_flags(&oflags), ret2);
            // The transaction handle in CDS is not a real 
            // transaction.
            if (oflags & DB_INIT_CDB)
                ptxn = NULL;
            if (name1.length() > 0)
                BDBOP2(penv->dbremove(ptxn, name1.c_str(), 
                    NULL, 0), ret2, this->abort_txn());
            if (name2.length() > 0)
                BDBOP2(penv->dbremove(ptxn, name2.c_str(), 
                    NULL, 0), ret2, this->abort_txn());

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
    //
        
    } // operator==

    /// Container unequality comparison operator.
    /// \param m2 The container to compare against.
    /// \return Returns false if equal, true otherwise.
    bool operator!=(const db_multimap<kdt, ddt, value_type_sub>& m2) const
    {
        return !this->operator==(m2);
    }
    ////////////////////////////////////////////////////////////////
    
protected:
    typedef ddt mapped_type;
    typedef value_type_sub tkpair;
    typedef int (*bt_compare_fcn_t)(Db *db, const Dbt *dbt1, 
        const Dbt *dbt2);

    friend class db_map_iterator<kdt, _DB_STL_set_value<kdt>, 
        value_type_sub>;
    friend class db_map_iterator<kdt, ddt, value_type_sub>;

    db_multimap(BulkRetrievalOption &opt) : base(opt){}
private:
    value_type_sub operator[] (const key_type& x)
    {
        THROW(NotSupportedException, ("db_multimap<>::operator[]"));
    }

    value_type_sub operator[] (const key_type& x) const
    {
        THROW(NotSupportedException, ("db_multimap<>::operator[]"));
        
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

        // Can't go without no dup or dupsort flag set.
        if (!((sflags & DB_DUP) || (sflags & DB_DUPSORT)))
            err = 
"db_multimap<> can not be backed by database not permitting duplicate keys";
        
        if (sflags & DB_RECNUM)
            err = "no DB_RECNUM flag allowed in db_map<>";
        
        return err;
        
    }

    inline void copy_db(db_multimap<kdt, ddt, value_type_sub> &x)
    {    

        // Make sure clear can succeed if there are cursors 
        // open in other threads.
        this->clear(false);
        insert(x.begin(), x.end());
        
    }

};// db_multimap<>
//@} //dbstl_containers
END_NS

#endif // !_DB_STL_DB_MAP_H_
