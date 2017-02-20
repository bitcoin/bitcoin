/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_DB_BASE_ITERATOR_H
#define _DB_STL_DB_BASE_ITERATOR_H

#include "dbstl_common.h"

START_NS(dbstl)

template <Typename ddt>
class ElementRef;
template <Typename ddt>
class ElementHolder;

/** \defgroup dbstl_iterators dbstl iterator classes
Common information for all dbstl iterators:

1. Each instance of a dbstl iterator uniquely owns a Berkeley DB cursor,
so that the key/data pair it currently sits on is always valid before it moves
elsewhere. It also caches the current key/data pair values in order for member
functions like operator* /operator-> to work properly, but caching is not 
compatible with standard C++ Stl behavior --- the C++ standard requires the 
iterator refer to a shared piece of memory where the data is stored, 
thus two iterators of the same container sitting on the same element should 
point to the same memory location, which is false for dbstl iterators. 

2. There are some functions common to each child class of this class which have
identical behaviors, so we will document them here.

@{
This class is the base class for all dbstl iterators, there is no much to say
about this class itself, and users are not supposed to directly use this class
at all. So we will talk about some common functions of dbstl iterators in 
this section.

\sa db_vector_base_iterator db_vector_iterator db_map_base_iterator 
db_map_iterator db_set_base_iterator db_set_iterator
*/
template <Typename ddt>
class db_base_iterator
{
protected:
    typedef db_base_iterator<ddt> self;
    friend class ElementHolder<ddt>;
    friend class ElementRef<ddt>;

    // The container from which this iterator is created.
    mutable db_container *owner_;

    bool dead_; // Internally used to prevent recursive destruction calls.

    // Whether or not to always get current key/data pair directly from db;
    // If true, always poll db, slower but safe for concurrent use;
    // otherwise faster, but when multiple iterators point to the same
    // key/data pair (they definitely have same locker id) and use some
    // iterators to update current key/data pair, then other iterators'
    // key/data pairs are obsolete, need to call iterator::refresh().
    //
    bool directdb_get_;

    // Whether to do bulk retrieval for a read only iterator. If non-zero,
    // do bulk retrieval and use this member as the bulk buffer size,
    // otherwise, do not use bulk retrieval. While doing bulk retrieval,
    // no longer read from database even if directdb_get is true.
    u_int32_t bulk_retrieval_;

    // Whether to use DB_RMW flag in Dbc::get. Users can set this flag in
    // db_container::begin(). The flag may be ignored when there is no
    // locking subsystem initialized.
    bool rmw_csr_;

    // Whether this iterator is a db_set/db_multiset iterator.
    bool is_set_;

    // Whether this iterator is read only. Default value is false.
    // The const version of begin(), or passing an explicit "readonly"
    // parameter to non-const begin() will will create a read only
    // iterator;
    //
    bool read_only_;

    // Iteration status. If in valid range, it is 0, otherwise it is
    // INVALID_ITERATOR_POSITION.
    mutable int itr_status_;

    // Distinguish the invalid iterator of end() and rend(). If an
    // iterator was invalidated in operator++, inval_pos_type_ is
    // IPT_AFTER_LAST, else if in operator-- it is IPT_BEFORE_FIRST.
    // For random iterator, inval_pos_type_ is also updated in random
    // movement functions. This member is only valid when itr_status_ is
    // set to INVALID_ITERATOR_POSITION.
    mutable char inval_pos_type_;
    // Values to denote the invalid positions.
    const static char IPT_BEFORE_FIRST = -1;
    const static char IPT_AFTER_LAST = 1;
    const static char IPT_UNSET = 0;

    virtual void delete_me() const { delete this;}

    virtual db_base_iterator* dup_itr() const
    {
        THROW(InvalidFunctionCall, (
            "\ndb_base_iterator<>::dup_itr can't be called.\n"));
        
    }

    inline bool is_set_iterator() const
    {
        return is_set_;
    }

    // The following two functions can't be abstract virtual for
    // compiler errors.
    virtual int replace_current(const ddt&)
    {
        THROW(InvalidFunctionCall, (
            "\ndb_base_iterator<>::replace_current can't be called\n"));
    }

    virtual int replace_current_key(const ddt&)
    {
        THROW(InvalidFunctionCall, (
        "\ndb_base_iterator<>::replace_current_key can't be called\n"));
    }

public:
#if NEVER_DEFINE_THIS_MACRO_IT_IS_FOR_DOXYGEN_ONLY
    /// Read data from underlying database via its cursor, and update 
    /// its cached value.
    /// \param from_db Whether retrieve data from database rather than
    /// using the cached data in this iterator.
    /// \return 0 if succeeded. Otherwise an DbstlException exception 
    /// will be thrown.
    int refresh(bool from_db = true);

    /** Close its cursor. If you are sure the iterator is no longer
    used, call this function so that its underlying cursor is closed
    before this iterator is destructed, potentially increase performance
    and concurrency. Note that the cursor is definitely
    closed at iterator destruction if you don't close it explicitly.
    */
    void close_cursor()  const;

    /** Call this function to modify bulk buffer size. Bulk retrieval is
    enabled when creating an iterator, so users later can only modify
    the bulk buffer size to another value, but can't enable/disable
    bulk read while an iterator is already alive.
    \param sz The new buffer size in bytes.
    \return true if succeeded, false otherwise.
    */
    bool set_bulk_buffer(u_int32_t sz);
    
    /// Return current bulk buffer size. Returns 0 if bulk retrieval is
    /// not enabled.
    u_int32_t get_bulk_bufsize();
#endif // NEVER_DEFINE_THIS_MACRO_IT_IS_FOR_DOXYGEN_ONLY

    ////////////////////////////////////////////////////////////////////
    //
    // Begin public constructors and destructor.
    //
    /// Default constructor.
    db_base_iterator()
    {
        owner_ = NULL;
        directdb_get_ = true;
        dead_ = false;
        itr_status_ = INVALID_ITERATOR_POSITION;
        read_only_ = false;
        is_set_ = false;
        bulk_retrieval_ = 0;
        rmw_csr_ = false;
        inval_pos_type_ = IPT_UNSET;
    }

    /// Constructor.
    db_base_iterator(db_container *powner, bool directdbget,
        bool b_read_only, u_int32_t bulk, bool rmw)
    {
        owner_ = powner;
        directdb_get_ = directdbget;
        dead_ = false;
        itr_status_ = INVALID_ITERATOR_POSITION;
        read_only_ = b_read_only;
        is_set_ = false;
        bulk_retrieval_ = bulk;
        rmw_csr_ = rmw;
        inval_pos_type_ = IPT_UNSET;
    }

    /// Copy constructor. Copy all members of this class.
    db_base_iterator(const db_base_iterator &bi)
    {
        owner_ = bi.owner_;
        directdb_get_ = bi.directdb_get_;
        dead_ = false;
        itr_status_ = bi.itr_status_;
        read_only_ = bi.read_only_;
        is_set_ = bi.is_set_;
        bulk_retrieval_ = bi.bulk_retrieval_;
        rmw_csr_ = bi.rmw_csr_;
        inval_pos_type_ = bi.inval_pos_type_;
    }

    /**
    Iterator assignment operator.
    Iterator assignment will cause the underlying cursor of the right iterator
    to be duplicated to the left iterator after its previous cursor is closed,
    to make sure each iterator owns one unique cursor. The key/data cached 
    in the right iterator is copied to the left iterator. Consequently, 
    the left iterator points to the same key/data pair in the database 
    as the the right value after the assignment, and have identical cached
    key/data pair.
    \param bi The other iterator to assign with.
    \return The iterator bi's reference.
    */
    inline const self& operator=(const self& bi)
    {
        ASSIGNMENT_PREDCOND(bi)
        owner_ = bi.owner_;
        directdb_get_ = bi.directdb_get_;
        dead_ = false;
        itr_status_ = bi.itr_status_;
        read_only_ = bi.read_only_;
        is_set_ = bi.is_set_;
        bulk_retrieval_ = bi.bulk_retrieval_;
        rmw_csr_ = bi.rmw_csr_;
        inval_pos_type_ = bi.inval_pos_type_;
        return bi;
    }

    /// Destructor.
    virtual ~db_base_iterator() {}

    ////////////////////////////////////////////////////////////////////

    /// \brief Get bulk buffer size.
    ///
    /// Return bulk buffer size. If the size is 0, bulk retrieval is not
    /// enabled.
    u_int32_t get_bulk_retrieval() const { return bulk_retrieval_; }

    /// \brief Get DB_RMW setting.
    ///
    /// Return true if the iterator's cursor has DB_RMW flag set, false
    /// otherwise. DB_RMW flag causes 
    /// a write lock to be acquired when reading a key/data pair, so
    /// that the transaction won't block later when writing back the 
    /// updated value in a read-modify-write operation cycle.
    bool is_rmw() const { return rmw_csr_; }

    /// \brief Get direct database get setting.
    ///
    /// Return true if every operation to retrieve the key/data pair the
    /// iterator points to will read from database rather than using
    /// the cached value, false otherwise.
    bool is_directdb_get() const {return directdb_get_; }
}; // db_base_iterator

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//
// db_reverse_iterator class template definition.
//
// db_reverse_iterator is the reverse iterator adapter for all iterator
// classes of dbstl. It makes an original iterator its reverse iterator.
// We don't want to use std::reverse_iterator<> adapter because it is more
// more expensive. Here we move the original iterator one position back to
// avoid unnecessary movement.
//
/// This class is the reverse class adaptor for all dbstl iterator classes. 
/// It inherits from real iterator classes like db_vector_iterator, 
/// db_map_iterator or db_set_iterator. When you call container::rbegin(), 
/// you will get an instance of this class.
/// \sa db_vector_base_iterator db_vector_iterator db_map_base_iterator 
/// db_map_iterator db_set_base_iterator db_set_iterator
template <Typename iterator, Typename twin_itr_t>
class _exported db_reverse_iterator : public iterator
{
public:
    // typedefs are not inherited, so re-define them here.
    //
    typedef db_reverse_iterator<iterator, twin_itr_t> self;
    typedef typename iterator::value_type T;
    typedef iterator iterator_type;
    typedef typename iterator::value_type value_type;
    typedef typename iterator::reference reference;
    typedef typename iterator::pointer pointer;
    typedef typename iterator::iterator_category iterator_category;
    typedef typename iterator::key_type key_type;
    typedef typename iterator::data_type data_type;
    typedef typename iterator::difference_type difference_type;
    typedef typename iterator::value_type_wrap value_type_wrap;
    // Construct a reverse iterator from iterator vi. We don't duplicate
    // itrerator vi here because vi is not supposed to be used again.
    // This function is supposed to be used by dbstl users, internally
    // self::set_iterator method is used.
    /// Constructor. Construct from an iterator of wrapped type.
    db_reverse_iterator(const iterator& vi) : iterator(vi)
    {
        iterator::operator--();
    }

    /// Copy constructor.
    db_reverse_iterator(const self& ritr)
        : iterator(ritr)
    {
    }

    /// Copy constructor.
    db_reverse_iterator(const 
        db_reverse_iterator<twin_itr_t, iterator>& ritr)
        : iterator(ritr)
    {
    }

    /// Default constructor.
    db_reverse_iterator() : iterator()
    {
    }

    // Do not throw exceptions here because it is normal to iterate to
    // "end()".
    //
    /// \name Reverse iterator movement functions
    /// When we talk about reverse iterator movement, we think the 
    /// container is a uni-directional range, represented by [begin, end),
    /// and this is true no matter we are using iterators or reverse 
    /// iterators. When an iterator is moved closer to "begin", we say it
    /// is moved forward, otherwise we say it is moved backward.
    //@{
    /// Move this iterator forward by one element. 
    /// \return The moved iterator at new position.
    self& operator++()
    {
        iterator::operator--();
        return *this;
    }

    /// Move this iterator forward by one element. 
    /// \return The original iterator at old position.
    self operator++(int)
    {
        // XXX!!! This conversion relies on this class or
        // db_set_iterator<> having no data members at all, which
        // is currently the case.
        return (self)iterator::operator--(1);
    }

    /// Move this iterator backward by one element. 
    /// \return The moved iterator at new position.
    self& operator--()
    {
        iterator::operator++();
        return *this;
    }

    /// Move this iterator backward by one element. 
    /// \return The original iterator at old position.
    self operator--(int)
    {
        // XXX!!! This conversion relies on this class or
        // db_set_iterator<> having no data members at all, which
        // is currently the case.
        return (self)iterator::operator++(1);
    }
    //@} // Movement operators.

    /// Assignment operator.
    /// \param ri The iterator to assign with.
    /// \return The iterator ri.
    /// \sa db_base_iterator::operator=(const self&)
    const self& operator=(const self& ri)
    {
        ASSIGNMENT_PREDCOND(ri)
        iterator::operator=(ri);
        return ri;
    }

    ////////////// Methods below only applies to random iterators. /////
    /// \name Operators for random reverse iterators
    /// Return a new iterator by moving this iterator backward or forward
    /// by n elements.
    //@{
    /// Iterator shuffle operator.
    /// Return a new iterator by moving this iterator forward 
    /// by n elements.
    /// \param n The amount and direction of movement. If negative,
    /// will move towards reverse direction.
    /// \return A new iterator at new position.
    self operator+(difference_type n) const
    {
        self ritr(*this);
        ritr.iterator::operator-=(n);
        return ritr;
    }

    /// Iterator shuffle operator.
    /// Return a new iterator by moving this iterator backward 
    /// by n elements.
    /// \param n The amount and direction of movement. If negative,
    /// will move towards reverse direction.
    /// \return A new iterator at new position.
    self operator-(difference_type n) const
    {
        self ritr(*this);
        ritr.iterator::operator+=(n);
        return ritr;
    }
    //@}
    /// \name Operators for random reverse iterators
    /// Move this iterator backward or forward by n elements and then 
    /// return it.
    //@{
    /// Iterator shuffle operator.
    /// Move this iterator forward by n elements and then 
    /// return it.
    /// \param n The amount and direction of movement. If negative,
    /// will move towards reverse direction.
    /// \return This iterator at new position.
    const self& operator+=(difference_type n)
    {
        iterator::operator-=(n);
        return *this;
    }

    /// Iterator shuffle operator.
    /// Move this iterator backward by n elements and then 
    /// return it.
    /// \param n The amount and direction of movement. If negative,
    /// will move towards reverse direction.
    /// \return This iterator at new position.
    const self& operator-=(difference_type n)
    {
        iterator::operator+=(n);
        return *this;
    }
    //@}
    /// \name Operators for random reverse iterators
    /// Reverse iterator comparison against reverse iterator itr, the one
    /// sitting on elements with less index is returned to be greater.
    //@{
    /// Less compare operator.
    bool operator<(const self& itr) const
    {
        return (iterator::operator>(itr));
    }

    /// Greater compare operator.
    bool operator>(const self& itr) const
    {
        return (iterator::operator<(itr));
    }

    /// Less equal compare operator.
    bool operator<=(const self& itr) const
    {
        return (iterator::operator>=(itr));
    }

    /// Greater equal compare operator.
    bool operator>=(const self& itr) const
    {
        return (iterator::operator<=(itr));
    }
    //@}
    // Because v.rend() - v.rbegin() < 0, we should negate base version.
    /// Return the negative value of the difference of indices of elements 
    /// this iterator and itr are sitting on.
    /// \return itr.index - this->index.
    /// \param itr The other reverse iterator.
    difference_type operator-(const self&itr) const
    {
        difference_type d = iterator::operator-(itr);
        return -d;
    }

    /// Return the reference of the element which can be reached by moving
    /// this reverse iterator by Off times backward. If Off is negative,
    /// the movement will be forward.
    inline value_type_wrap operator[](difference_type Off) const
    {
        self ritr(*this);
        value_type_wrap res = ritr.iterator::operator[](-Off);
        return res;
    }

}; // reverse_iterator
//@} // dbstl_iterators
END_NS

#endif // !_DB_STL_DB_BASE_ITERATOR_H
