/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_DB_VECTOR_H
#define _DB_STL_DB_VECTOR_H

#include "dbstl_common.h"
#include "dbstl_dbc.h"
#include "dbstl_element_ref.h"
#include "dbstl_resource_manager.h"
#include "dbstl_container.h"
#include "dbstl_base_iterator.h"
#include <list>
#include <algorithm>

START_NS(dbstl)

using std::list;
using std::istream;
using std::ostream;
using std::sort;

#define TRandDbCursor RandDbCursor<T>

// Forward Declarations
// The default parameter is needed for the following code to work.
template <class T, Typename value_type_sub = ElementRef<T> >
class db_vector;
template <class T, Typename value_type_sub = ElementRef<T> >
class db_vector_iterator;
template<class kdt, class ddt> class DbCursor;
template<class ddt> class RandDbCursor;
template<class ddt> class ElementRef;
template <typename T, typename T2>
class DbstlListSpecialOps;

/// \ingroup dbstl_iterators
/// @{
/// \defgroup db_vector_iterators Iterator classes for db_vector.
/// db_vector has two iterator classes --- db_vector_base_iterator and
/// db_vector_iterator. The differences 
/// between the two classes are that the db_vector_base_iterator
/// can only be used to read its referenced value, so it is intended as
/// db_vector's const iterator; While the other class allows both read and
/// write access. If your access pattern is readonly, it is strongly 
/// recommended that you use the const iterator because it is faster 
/// and more efficient.
/// The two classes have identical behaviors to std::vector::const_iterator and 
/// std::vector::iterator respectively. Note that the common public member 
/// function behaviors are described in the db_base_iterator section.
/// \sa db_base_iterator
//@{
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//
// db_vector_base_iterator class template definition
//
// db_vector_base_iterator is the const_iterator class for db_vector, and
// base class of db_vector_iterator -- iterator class for db_vector. Each
// db_vector_base_iterator object owns one RandDbCursor<> cursor (1:1 map). 
// On copy construction, the cursor is not duplicated, it is only created 
// when it is really used (ie: lazily).
// The value referred by this iterator is read only, can't be used to mutate
// its referenced data element.
//
/// This class is the const iterator class for db_vector, and it is 
/// inheirted by the db_vector_iterator class, which is the iterator
/// class for db_vector.
template <class T>
class _exported db_vector_base_iterator : public db_base_iterator<T> 
{
protected:
    // typedef's can't be put after where it is used.
    typedef db_vector_base_iterator<T> self;
    typedef db_recno_t index_type;
    using db_base_iterator<T>::replace_current_key;
public:
    ////////////////////////////////////////////////////////////////////
    // 
    // Begin public type definitions.
    //
    typedef T value_type;
    typedef ptrdiff_t difference_type;
    typedef difference_type distance_type;

    /// This is the return type for operator[].
    /// \sa value_type_wrap operator[](difference_type _Off) const
    typedef value_type value_type_wrap;
    typedef value_type& reference; 
    typedef value_type* pointer;
    // Use the STL tag, to ensure compatability with interal STL functions.
    //
    typedef std::random_access_iterator_tag iterator_category;
    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    // Begin public constructors and destructor.
    /// \name Constructors and destroctor
    /// Do not construct iterators explictily using these constructors,
    /// but call db_vector::begin() const to get an valid iterator.
    /// \sa db_vector::begin() const
    //@{
    db_vector_base_iterator(const db_vector_base_iterator<T>& vi)
        : base(vi), pcsr_(vi.pcsr_), curpair_base_(vi.curpair_base_)
    {
        
    }

    explicit db_vector_base_iterator(db_container*powner,
        u_int32_t b_bulk_retrieval = 0, bool rmw = false, 
        bool directdbget = true, bool readonly = false)
        : base(powner, directdbget, readonly, b_bulk_retrieval, rmw),
        pcsr_(new TRandDbCursor(b_bulk_retrieval, rmw, directdbget))
    {
        
    }

    db_vector_base_iterator() : pcsr_(NULL)
    {
        
    }

    virtual ~db_vector_base_iterator()
    {
        this->dead_ = true;
        if (pcsr_)
            pcsr_->close();
    }
    //@}

    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    //
    // Begin functions that compare iterator positions.
    //
    // Use itr_status_ to do comparison if it is non-zero because end
    // iterator does not have an underlying key/data pair, and call
    // underlying cursor comparison otherwise.
    /// \name Iterator comparison operators
    /// The way to compare two iterators is to compare the index values 
    /// of the two elements they point to. The iterator sitting on an 
    /// element with less index is regarded to be smaller. And the invalid
    /// iterator sitting after last element is greater than any other
    /// iterators, because it is assumed to have an index equal to last 
    /// element's index plus one; The invalid iterator sitting before first
    /// element is less than any other iterators because it is assumed to
    /// have an index -1.
    //@{
    /// \brief Equality comparison operator. 
    ///
    /// Invalid iterators are equal; Valid iterators
    /// sitting on the same key/data pair equal; Otherwise not equal.
    /// \param itr The iterator to compare against.
    /// \return True if this iterator equals to itr; False otherwise.
    inline bool operator==(const self&itr) const
    {
        COMPARE_CHECK(itr)
        if ((itr.itr_status_ == this->itr_status_) && 
            (this->itr_status_ == INVALID_ITERATOR_POSITION))
            return true;

        if (this->itr_status_ != INVALID_ITERATOR_POSITION &&
            itr.itr_status_ != INVALID_ITERATOR_POSITION) {
            return (pcsr_->compare((itr.pcsr_.base_ptr())) == 0);
        }
        return false;
    }

    /// \brief Unequal compare, identical to !operator(==itr)
    /// \param itr The iterator to compare against.
    /// \return False if this iterator equals to itr; True otherwise.
    inline bool operator!=(const self&itr) const
    {
        return !(*this == itr) ;
    }

    // end() iterator is largest. If both are end() iterator return false.
    /// \brief Less than comparison operator.
    /// \param itr The iterator to compare against.
    /// \return True if this iterator is less than itr.
    inline bool operator < (const self& itr) const
    {
        bool ret;

        if (this == &itr)
            return false;

        char after_last = base::IPT_AFTER_LAST,
            bef_fst = base::IPT_BEFORE_FIRST;

        if (this->itr_status_ == INVALID_ITERATOR_POSITION &&
            this->inval_pos_type_ == after_last)
            ret = false;
        else if (this->itr_status_ == INVALID_ITERATOR_POSITION &&
            this->inval_pos_type_ == bef_fst)
            ret = itr.inval_pos_type_ != bef_fst;
        else { // This iterator is on an ordinary position.
            if (itr.itr_status_ == INVALID_ITERATOR_POSITION &&
                itr.inval_pos_type_ == after_last)
                ret = true;
            else if (itr.itr_status_ == INVALID_ITERATOR_POSITION &&
                itr.inval_pos_type_ == bef_fst)
                ret = false;
            else { // Both iterators are on an ordinary position.
                // By this stage all valid cases using
                // INVALID_ITERATOR_POSITION have been dealt
                // with.
                assert((this->itr_status_ !=
                    INVALID_ITERATOR_POSITION) &&
                    (itr.itr_status_ != 
                    INVALID_ITERATOR_POSITION));
                ret = pcsr_->compare(
                    (itr.pcsr_.base_ptr())) < 0;
            }
        }
        return ret;
    }

    /// \brief Less equal comparison operator.
    /// \param itr The iterator to compare against.
    /// \return True if this iterator is less than or equal to itr.
    inline bool operator <= (const self& itr) const
    {
        return !(this->operator>(itr));
    }

    /// \brief Greater equal comparison operator.
    /// \param itr The iterator to compare against.
    /// \return True if this iterator is greater than or equal to itr.
    inline bool operator >= (const self& itr) const
    {
        return !(this->operator<(itr));
    }

    // end() iterator is largest. If both are end() iterator return false.
    /// \brief Greater comparison operator.
    /// \param itr The iterator to compare against.
    /// \return True if this iterator is greater than itr.
    inline bool operator > (const self& itr) const
    {
        bool ret;

        if (this == &itr)
            return false;

        char after_last = base::IPT_AFTER_LAST,
            bef_fst = base::IPT_BEFORE_FIRST;

        if (this->itr_status_ == INVALID_ITERATOR_POSITION &&
            this->inval_pos_type_ == after_last)
            ret = itr.inval_pos_type_ != after_last;
        else if (this->itr_status_ == INVALID_ITERATOR_POSITION &&
            this->inval_pos_type_ == bef_fst)
            ret = false;
        else { // This iterator is on an ordinary position.
            if (itr.itr_status_ == INVALID_ITERATOR_POSITION &&
                itr.inval_pos_type_ == after_last)
                ret = false;
            else if (itr.itr_status_ == INVALID_ITERATOR_POSITION &&
                itr.inval_pos_type_ == bef_fst)
                ret = true;
            else { // Both iterators are on an ordinary position.
                // By this stage all valid cases using
                // INVALID_ITERATOR_POSITION have been dealt
                // with.
                assert((this->itr_status_ !=
                    INVALID_ITERATOR_POSITION) &&
                    (itr.itr_status_ != 
                    INVALID_ITERATOR_POSITION));
                ret = pcsr_->compare(
                    (itr.pcsr_.base_ptr())) > 0;
            }
        }
        return ret;
    }
    //@} // vctitr_cmp
    ////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////
    //
    // Begin functions that shift the iterator position.
    //
    /// \name Iterator movement operators.
    /// When we talk about iterator movement, we think the 
    /// container is a uni-directional range, represented by [begin, end),
    /// and this is true no matter we are using iterators or reverse 
    /// iterators. When an iterator is moved closer to "begin", we say it
    /// is moved forward, otherwise we say it is moved backward.
    //@{
    /// \brief Pre-increment.
    ///
    /// Move the iterator one element backward, so that
    /// the element it sits on has a bigger index.
    /// Use ++iter rather than iter++ where possible to avoid two useless
    /// iterator copy constructions.
    /// \return This iterator after incremented.
    inline self& operator++()
    {
        move_by(*this, 1, false);
        return *this;
    }

    /// \brief Post-increment.
    /// Move the iterator one element backward, so that
    /// the element it sits on has a bigger index.
    /// Use ++iter rather than iter++ where possible to avoid two useless
    /// iterator copy constructions.
    /// \return A new iterator not incremented.
    inline self operator++(int)
    {
        self itr(*this);
        move_by(*this, 1, false);

        return itr;
    }

    /// \brief Pre-decrement.
    /// Move the iterator one element backward, so 
    /// that the element it sits on has a smaller index.
    /// Use --iter rather than iter-- where possible to avoid two useless
    /// iterator copy constructions.
    /// \return This iterator after decremented.
    inline self& operator--()
    {
        move_by(*this, 1, true);
        return *this;
    }

    /// \brief Post-decrement.
    ///
    /// Move the iterator one element backward, so 
    /// that the element it sits on has a smaller index.
    /// Use --iter rather than iter-- where possible to avoid two useless
    /// iterator copy constructions.
    /// \return A new iterator not decremented.
    inline self operator--(int)
    {
        self itr = *this;
        move_by(*this, 1, true);
        return itr;
    }

    /// \brief Assignment operator.
    ///
    /// This iterator will point to the same key/data
    /// pair as itr, and have the same configurations as itr.
    /// \sa db_base_iterator::operator=
    /// \param itr The right value of the assignment.
    /// \return This iterator's reference.
    inline const self& operator=(const self&itr)
    {
        ASSIGNMENT_PREDCOND(itr)
        base::operator=(itr);
        
        if (pcsr_)
            pcsr_->close();
        pcsr_ = itr.pcsr_;
        curpair_base_ = itr.curpair_base_;
        return itr;
    }

    // Always move both iterator and cursor synchronously, keep iterators
    // data synchronized.
    /// Iterator movement operator.
    /// Return another iterator by moving this iterator forward by n
    /// elements.
    /// \param n The amount and direction of movement. If negative, will
    /// move forward by |n| element.
    /// \return The new iterator at new position.
    inline self operator+(difference_type n) const
    {
        self itr(*this);
        move_by(itr, n, false);
        return itr;
    }

    /// \brief Move this iterator backward by n elements.
    /// \param n The amount and direction of movement. If negative, will
    /// move forward by |n| element.
    /// \return Reference to this iterator at new position.
    inline const self& operator+=(difference_type n)
    {
        move_by(*this, n, false);
        return *this;
    }

    /// \brief Iterator movement operator.
    ///
    /// Return another iterator by moving this iterator backward by n
    /// elements.
    /// \param n The amount and direction of movement. If negative, will
    /// move backward by |n| element.
    /// \return The new iterator at new position.
    inline self operator-(difference_type n) const
    {
        self itr(*this);
        move_by(itr, n);

        return itr;
    }

    /// \brief Move this iterator forward by n elements.
    /// \param n The amount and direction of movement. If negative, will
    /// move backward by |n| element.
    /// \return Reference to this iterator at new position.
    inline const self& operator-=(difference_type n)
    {
        move_by(*this, n);
        return *this;
    }
    //@} //itr_movement

    /// \brief Iterator distance operator.
    ///
    /// Return the index difference of this iterator and itr, so if this
    /// iterator sits on an element with a smaller index, this call will
    /// return a negative number.
    /// \param itr The other iterator to substract. itr can be the invalid
    /// iterator after last element or before first element, their index
    /// will be regarded as last element's index + 1 and -1 respectively.
    /// \return The index difference.
    difference_type operator-(const self&itr) const
    {
        difference_type p1, p2;

        if (itr == end_itr_) {
            if (itr.inval_pos_type_ == base::IPT_BEFORE_FIRST)
                p2 = -1;
            else if (
                this->inval_pos_type_ == base::IPT_AFTER_LAST) {
                self pitr2(itr);
                pitr2.open();
                pitr2.last();
                p2 = pitr2.get_current_index() + 1;
            } else {
                THROW0(InvalidIteratorException);
            }
        } else
            p2 = itr.get_current_index();

        if (*this == end_itr_) {
            if (this->inval_pos_type_ == base::IPT_BEFORE_FIRST)
                p1 = -1;
            else if (
                this->inval_pos_type_ == base::IPT_AFTER_LAST) {
                self pitr1(*this);
                pitr1.open();
                pitr1.last();
                p1 = pitr1.get_current_index() + 1;
            } else {
                THROW0(InvalidIteratorException);
            }
        } else
            p1 = this->get_current_index();

        return (difference_type)(p1 - p2);
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Begin functions that retrieve values from the iterator.
    //
    // Each iterator has a dedicated cursor, and we keep the iterator
    // and cursor synchronized all the time. The returned value is read
    // only, can't be used to mutate its underlying data element.
    //
    /// \name Functions that retrieve values from the iterator.
    //@{
    /// \brief Dereference operator.
    ///
    /// Return the reference to the cached data element, which is an 
    /// ElementRef<T> object if T is a class type or an ElementHolder<T>
    /// object if T is a C++ primitive data type.
    /// The returned value can only be used to read its referenced 
    /// element.
    /// \return The reference to the element this iterator points to.
    inline reference operator*() const
    {
        if (this->directdb_get_)
            update_cur_pair();
        return curpair_base_; // Return reference, no copy construction.
    }

    /// \brief Arrow operator.
    ///
    /// Return the pointer to the cached data element, which is an 
    /// ElementRef<T> object if T is a class type or an ElementHolder<T>
    /// object if T is a C++ primitive data type.
    /// The returned value can only be used to read its referenced 
    /// element.
    /// \return The address of the referenced object.
    inline pointer operator->() const
    {
        if (this->directdb_get_)
            update_cur_pair();
        return &(curpair_base_);
    }

    /// \brief Iterator index operator.
    ///
    /// If _Off not in a valid range, the returned value will be invalid.
    /// Note that you should use a value_type_wrap type to hold the 
    /// returned value.
    /// \param _Off The valid index relative to this iterator.
    /// \return Return the element which is at position *this + _Off.
    /// The returned value can only be used to read its referenced 
    /// element. 
    inline value_type_wrap operator[](difference_type _Off) const
    {
        self itr(*this + _Off);
        return itr.curpair_base_;
    }
    //@}
    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    //
    // Begin functions that are dbstl specific.
    //
    /// \name Functions that are dbstl specific.
    //@{
    /// \brief Get current index of within the vector.
    ///
    /// Return the iterators current element's index (0 based). Requires
    /// this iterator to be a valid iterator, not end_itr_.
    /// \return current index of the iterator.
    inline index_type get_current_index() const
    {
        return pcsr_->get_current_index() - 1;
    }

    /// \brief Iterator movement function.
    ///
    /// Move this iterator to the index "n". If n is not in the valid 
    /// range, this iterator will be an invalid iterator equal to end() 
    /// iterator.
    /// \param n target element's index.
    /// \sa db_vector::end();
    inline void move_to(index_type n) const
    {
        T d;
        int ret;

        this->itr_status_ = pcsr_->move_to(n + 1);
        ret = pcsr_->get_current_data(d);
        dbstl_assert(ret == 0);
        if (this->itr_status_ == 0) 
            update_cur_pair();
    }

    /// \brief Refresh iterator cached value.
    /// \param from_db If not doing direct database get and this parameter
    /// is true, we will retrieve data directly from db.
    /// \sa db_base_iterator::refresh(bool).
    virtual int refresh(bool from_db = true)
    {
        
        if (from_db && !this->directdb_get_)
            this->pcsr_->update_current_key_data_from_db(
                DbCursorBase::SKIP_NONE);
        this->pcsr_->get_current_data(curpair_base_);
        
        return 0;
    }

    /// \brief Close underlying Berkeley DB cursor of this iterator.
    /// \sa db_base_iterator::close_cursor() const
    inline void close_cursor() const
    {
        this->pcsr_->close();
    }

    /// \brief Modify bulk buffer size. 
    ///
    /// Bulk read is enabled when creating an
    /// iterator, so you later can only modify the bulk buffer size
    /// to another value, but can't enable/disable bulk read while an
    /// iterator is already alive. 
    /// \param sz The new size of the bulk read buffer of this iterator.
    /// \return Returns true if succeeded, false otherwise.
    /// \sa db_base_iterator::set_bulk_buffer(u_int32_t sz)
    bool set_bulk_buffer(u_int32_t sz)
    {
        bool ret = this->pcsr_->set_bulk_buffer(sz);
        if (ret)
            this->bulk_retrieval_ = sz;
        return ret;
    }

    /// \brief Get bulk retrieval buffer size in bytes.
    /// \return Return current bulk buffer size, or 0 if bulk retrieval is
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
    typedef T value_type_base;
    typedef db_base_iterator<T> base;
    typedef index_type size_type;
    typedef index_type key_type;
    typedef T data_type;

    // end_itr_ is always the same, so share it across iterator instances.
    static self end_itr_;

    ////////////////// iterator data members //////////////////////////
    //
    mutable LazyDupCursor<TRandDbCursor> pcsr_;

    void open() const
    {
        u_int32_t oflags = 0, oflags2 = 0;
        int ret;
        DbEnv *penv = this->owner_->get_db_env_handle();

        oflags2 = this->owner_->get_cursor_open_flags();
        if (!this->read_only_ && penv != NULL) {
            BDBOP((penv->get_open_flags(&oflags)), ret);
            // Open a writeable cursor when in CDS mode and not
            // requesting a read only iterator.
            if ((oflags & DB_INIT_CDB) != 0)
                oflags2 |= DB_WRITECURSOR;
        }

        if (!this->pcsr_) {
            
            this->pcsr_.set_cursor(new TRandDbCursor(
                this->bulk_retrieval_, 
                this->rmw_csr_, this->directdb_get_));
        }
        this->itr_status_ = this->pcsr_->open(this->owner_, oflags2);
    }

    // Move the underlying Dbc* cursor to the first element.
    //
    inline int first() const
    {
        int ret = 0;

        if ((ret = pcsr_->first()) == 0)
            update_cur_pair();
        else
            this->itr_status_ = ret;

        return ret;
    }

    // Move the underlying Dbc* cursor to the last element.
    //
    inline index_type last() const
    {
        index_type pos_index, lsz;

        // last_index() will move the underlying cursor to last record.
        lsz = pcsr_->last_index();
        if (lsz > 0) {
            pos_index = lsz - 1;
            this->itr_status_ = 0;
            update_cur_pair();
        } else {
            this->itr_status_ = INVALID_ITERATOR_POSITION;
            pos_index = INVALID_INDEX;
        }

        return pos_index;
    }

    void set_curpair_base(const T& d)
    {
        curpair_base_ = d;
    }

    // Update curpair_base_'s data using current underlying key/data pair's
    // value. Called on every iterator movement.
    // Even if this iterator is invalid, this call is allowed, the
    // default value of type T is returned.
    //
    virtual void update_cur_pair() const
    {
        this->pcsr_->get_current_data(curpair_base_);
    }

    // The 'back' parameter indicates whether to decrease or
    // increase the index when moving. The default is to decrease.
    // 
    // Do not throw exceptions here because it is normal to iterate to
    // "end()".
    // Always move both iterator and cursor synchronously, keep iterators
    // data synchronized.
    void move_by(const self&itr, difference_type n, bool back = true) const
    {
        if (n == 0)
            return;
        if (!back)
            n = -n;
        if (itr == end_itr_) {
            if (n > 0) { // Go back from end()
                itr.open();
                itr.last();
                // Moving from end to the last position is
                // considered one place.
                if (n > 1) {
                    itr.itr_status_ = 
                        itr.pcsr_->advance(-n + 1);
                    if (itr.itr_status_ == 
                        INVALID_ITERATOR_POSITION)
                        itr.inval_pos_type_ = 
                            base::IPT_BEFORE_FIRST;
                }
            } else
                // Can't go further forward from the end.
                return;

        } else {
            if (n == 1)
                itr.itr_status_ = itr.pcsr_->prev();
            else if (n == -1)
                itr.itr_status_ = itr.pcsr_->next();
            else
                itr.itr_status_ = itr.pcsr_->advance(-n);
        }

        itr.update_cur_pair();
        if (itr.itr_status_ != 0) {
            if (n > 0)
                itr.inval_pos_type_ = base::IPT_BEFORE_FIRST;
            else
                itr.inval_pos_type_ = base::IPT_AFTER_LAST;
        }
    }

private:
    // Current data element cached here.
    mutable T curpair_base_;


    friend class db_vector<T, ElementHolder<T> >;
    friend class db_vector<T, ElementRef<T> >;
    friend class DbCursor<index_type, T>;
    friend class RandDbCursor<T>;
    friend class ElementRef<T>;
    friend class ElementHolder<T>;
}; // db_vector_base_iterator<>

template <Typename T>
    db_vector_base_iterator<T> db_vector_base_iterator<T>::end_itr_;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//
// db_vector_iterator class template definition
// This class is the iterator class for db_vector, its instances can
// be used to mutate their referenced data element.
//
template <class T, Typename value_type_sub>
class _exported db_vector_iterator : 
    public db_vector_base_iterator<T> 
{
protected:
    typedef db_vector_iterator<T, value_type_sub> self;
    typedef db_recno_t index_type;
    using db_base_iterator<T>::replace_current_key;
public:
    typedef T value_type;
    typedef ptrdiff_t difference_type;
    typedef difference_type distance_type;
    typedef value_type_sub& reference;

    /// This is the return type for operator[].
    /// \sa value_type_wrap operator[](difference_type _Off) const
    typedef value_type_sub value_type_wrap;
    typedef value_type_sub* pointer;
    // Use the STL tag, to ensure compatability with interal STL functions.
    //
    typedef std::random_access_iterator_tag iterator_category;

    ////////////////////////////////////////////////////////////////////
    // Begin public constructors and destructor.
    //
    /// \name Constructors and destructor
    /// Do not construct iterators explictily using these constructors,
    /// but call db_vector::begin to get an valid iterator.
    /// \sa db_vector::begin
    //@{
    db_vector_iterator(const db_vector_iterator<T, value_type_sub>& vi)
        : db_vector_base_iterator<T>(vi), 
        curpair_(vi.curpair_)
    {
        curpair_._DB_STL_SetIterator(this);
    }

    explicit db_vector_iterator(db_container*powner,
        u_int32_t b_bulk_retrieval = 0, bool brmw = false,
        bool directdbget = true, bool b_read_only = false)
        : db_vector_base_iterator<T>(powner, 
        b_bulk_retrieval, brmw, directdbget, b_read_only)
    {
        curpair_._DB_STL_SetIterator(this);
        this->read_only_ = b_read_only;
        this->rmw_csr_ = brmw;
    }

    db_vector_iterator() : db_vector_base_iterator<T>()
    {
        curpair_._DB_STL_SetIterator(this);
    }

    db_vector_iterator(const db_vector_base_iterator<T>&obj) 
        : db_vector_base_iterator<T>(obj)
    {
        curpair_._DB_STL_CopyData(*obj);
    }

    virtual ~db_vector_iterator()
    {
        this->dead_ = true;
    }
    //@}

    ////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////
    //
    // Begin functions that shift the iterator position.
    //
    // These functions are identical to those defined in 
    // db_vector_base_iterator, but we have to redefine them here because
    // the "self" have different definitions.
    //
    // Do not throw exceptions here because it is normal to iterate to
    // "end()".
    // Always move both iterator and cursor synchronously, keep iterators
    // data synchronized.
    //
    /// \name Iterator movement operators.
    /// These functions have identical behaviors and semantics as those of
    /// db_vector_base_iterator, so please refer to equivalent in that 
    /// class.
    //@{
    /// \brief Pre-increment.
    /// \return This iterator after incremented.
    /// \sa db_vector_base_iterator::operator++()
    inline self& operator++()
    {
        move_by(*this, 1, false);
        return *this;
    }

    /// \brief Post-increment.
    /// \return A new iterator not incremented.
    /// \sa db_vector_base_iterator::operator++(int)
    inline self operator++(int)
    {
        self itr(*this);
        move_by(*this, 1, false);

        return itr;
    }

    /// \brief Pre-decrement.
    /// \return This iterator after decremented.
    /// \sa db_vector_base_iterator::operator--()
    inline self& operator--()
    {
        move_by(*this, 1, true);
        return *this;
    }

    /// \brief Post-decrement.
    /// \return A new iterator not decremented.
    /// \sa db_vector_base_iterator::operator--(int)
    inline self operator--(int)
    {
        self itr = *this;
        move_by(*this, 1, true);
        return itr;
    }

    /// \brief Assignment operator.
    ///
    /// This iterator will point to the same key/data
    /// pair as itr, and have the same configurations as itr.
    /// \param itr The right value of the assignment.
    /// \return This iterator's reference.
    /// \sa db_base_iterator::operator=(const self&)
    inline const self& operator=(const self&itr)
    {
        ASSIGNMENT_PREDCOND(itr)
        base::operator=(itr);

        curpair_._DB_STL_CopyData(itr.curpair_);
        return itr;
    }

    // Always move both iterator and cursor synchronously, keep iterators
    // data synchronized.
    /// \brief Iterator movement operator.
    ///
    /// Return another iterator by moving this iterator backward by n
    /// elements.
    /// \param n The amount and direction of movement. If negative, will
    /// move forward by |n| element.
    /// \return The new iterator at new position.
    /// \sa db_vector_base_iterator::operator+(difference_type n) const
    inline self operator+(difference_type n) const
    {
        self itr(*this);
        move_by(itr, n, false);
        return itr;
    }

    /// \brief Move this iterator backward by n elements.
    /// \param n The amount and direction of movement. If negative, will
    /// move forward by |n| element.
    /// \return Reference to this iterator at new position.
    /// \sa db_vector_base_iterator::operator+=(difference_type n) 
    inline const self& operator+=(difference_type n)
    {
        move_by(*this, n, false);
        return *this;
    }

    /// \brief Iterator movement operator.
    ///
    /// Return another iterator by moving this iterator forward by n
    /// elements.
    /// \param n The amount and direction of movement. If negative, will
    /// move backward by |n| element.
    /// \return The new iterator at new position.
    /// \sa db_vector_base_iterator::operator-(difference_type n) const
    inline self operator-(difference_type n) const
    {
        self itr(*this);
        move_by(itr, n);

        return itr;
    }

    /// \brief Move this iterator forward by n elements.
    /// \param n The amount and direction of movement. If negative, will
    /// move backward by |n| element.
    /// \return Reference to this iterator at new position.
    /// \sa db_vector_base_iterator::operator-=(difference_type n) 
    inline const self& operator-=(difference_type n)
    {
        move_by(*this, n);
        return *this;
    }
    //@} // itr_movement

    /// \brief Iterator distance operator.
    ///
    /// Return the index difference of this iterator and itr, so if this
    /// iterator sits on an element with a smaller index, this call will
    /// return a negative number.
    /// \param itr The other iterator to substract. itr can be the invalid
    /// iterator after last element or before first element, their index
    /// will be regarded as last element's index + 1 and -1 respectively.
    /// \return The index difference.
    /// \sa db_vector_base_iterator::operator-(const self& itr) const
    difference_type operator-(const self&itr) const
    {
        return base::operator-(itr);
    }
    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    //
    // Begin functions that retrieve values from the iterator.
    //
    // Each iterator has a dedicated cursor, and we keep the iterator
    // and cursor synchronized all the time. The return value can be used
    // to mutate its referenced data element. If directdb_get_ is true(the
    // default value), users must call element._DB_STL_SaveElement() to 
    // store the changes they made to the data element before a next call 
    // of this function, otherwise the change is lost.
    //
    /// \name Functions that retrieve values from the iterator.
    //@{
    /// \brief Dereference operator.
    ///
    /// Return the reference to the cached data element, which is an 
    /// ElementRef<T> object if T is a class type or an ElementHolder<T>
    /// object if T is a C++ primitive data type.
    /// The returned value can be used to read or update its referenced 
    /// element.
    /// \return The reference to the element this iterator points to.
    inline reference operator*() const
    {
        if (this->directdb_get_)
            update_cur_pair();
        return curpair_; // Return reference, no copy construction.
    }

    /// \brief Arrow operator.
    ///
    /// Return the pointer to the cached data element, which is an 
    /// ElementRef<T> object if T is a class type or an ElementHolder<T>
    /// object if T is a C++ primitive data type.
    /// The returned value can be used to read or update its referenced 
    /// element.
    /// \return The address of the referenced object.
    inline pointer operator->() const
    {
        if (this->directdb_get_)
            update_cur_pair();
        return &curpair_;
    }

    // We can't return reference here otherwise we are returning an 
    // reference to an local object.
    /// \brief Iterator index operator.
    ///
    /// If _Off not in a valid range, the returned value will be invalid.
    /// Note that you should use a value_type_wrap type to hold the 
    /// returned value.
    /// \param _Off The valid index relative to this iterator.
    /// \return Return the element which is at position *this + _Off, 
    /// which is an ElementRef<T> object if T is a class type or
    /// an ElementHolder<T> object if T is a C++ primitive data type.
    /// The returned value can be used to read or update its referenced 
    /// element. 
    inline value_type_wrap operator[](difference_type _Off) const
    {
        self *itr = new self(*this + _Off);
        
        value_type_sub ref(itr->curpair_);
        ref._DB_STL_SetDelItr();
        return ref;
    }
    //@} // funcs_val
    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    //
    // Begin dbstl specific functions.
    //
    /// \brief Refresh iterator cached value.
    /// \param from_db If not doing direct database get and this parameter
    /// is true, we will retrieve data directly from db.
    /// \sa db_base_iterator::refresh(bool)
    virtual int refresh(bool from_db = true)
    {
        T d;

        if (from_db && !this->directdb_get_)
            this->pcsr_->update_current_key_data_from_db(
                DbCursorBase::SKIP_NONE);
        this->pcsr_->get_current_data(d);
        curpair_._DB_STL_SetData(d);
        this->set_curpair_base(d);
        return 0;
    }
    ////////////////////////////////////////////////////////////////////

protected:
    typedef T value_type_base;
    typedef db_vector_base_iterator<T> base;
    typedef index_type size_type;
    typedef index_type key_type;
    typedef T data_type;
    typedef db_vector<T, value_type_sub> OwnerType;

    friend class db_vector<T, value_type_sub>;
    friend class DbCursor<index_type, T>;
    friend class RandDbCursor<T>;
    friend class ElementRef<T>;
    friend class ElementHolder<T>;

    // The data item this iterator currently points to. It is updated
    // on every iterator movement. By default directdb_get_ member is true
    // (can be set to false via container's begin() method), so whenever 
    // operator */-> is called, it is retrieved from db, to support 
    // concurrency.
    mutable value_type_sub curpair_;

    virtual void delete_me() const
    {
        if (!this->dead_)
            delete this;
    }

    virtual self* dup_itr() const
    {
        self *itr = new self(*this);
        // The curpair_ of itr does not delete itr, the independent
        // one does.
        // itr->curpair_._DB_STL_SetDelItr(); 
        return itr;
    }

    // Replace the current key/data pair's data with d. Can only be called
    // by non-const iterator. Normally internal functions do not wrap
    // transactions, but replace_current is used in assignment by the user
    // so it needs to be wrapped.
    virtual int replace_current(const T& d)
    {
        int ret = 0;

        if (this->read_only_) {
            THROW(InvalidFunctionCall, (
"replace_current can't be called via a readonly iterator."));
        }
        ret = this->pcsr_->replace(d);

        return ret;
    }

    // This function is part of the db_base_iterator interface, but
    // is not valid for db_vector_iterator.
    virtual int replace_current_key(const T&) {
        THROW(InvalidFunctionCall, (
"replace_current_key not supported by db_vector_iterator<>"));
    }

    // Update curpair_'s data using current underlying key/data pair's
    // value. Called on every iterator movement.
    // Even if this iterator is invalid, this call is allowed, the
    // default value of type T is returned.
    //
    virtual void update_cur_pair() const
    {
        T t;

        this->pcsr_->get_current_data(t);
        curpair_._DB_STL_CopyData(t);
        base::update_cur_pair();
    }


}; // db_vector_iterator
//@} // db_vector_iterators
//@} // dbstl_iterators

// These operators make "n + itr" expressions valid. Without it, you can only
// use "itr + n"
template <Typename T>
db_vector_base_iterator<T> operator+(typename db_vector_base_iterator<T>::
    difference_type n, db_vector_base_iterator<T> itr)
{
    db_vector_base_iterator<T> i2 = itr;

    i2 += n;
    return i2;
}

template <Typename T, Typename value_type_sub>
db_vector_iterator<T, value_type_sub> operator+(
    typename db_vector_iterator<T, value_type_sub>::
    difference_type n, db_vector_iterator<T, value_type_sub> itr)
{
    db_vector_iterator<T, value_type_sub> i2 = itr;

    i2 += n;
    return i2;
}

template <Typename iterator, Typename iterator2>
db_reverse_iterator<iterator, iterator2> operator+(typename
    db_reverse_iterator<iterator, iterator2>::difference_type n,
    db_reverse_iterator<iterator, iterator2> itr)
{
    db_reverse_iterator<iterator, iterator2> i2 = itr;
    // The db_reverse_iterator<iterator>::operator+ will substract
    // n in it, we pass the + here.
    //
    i2 += n;
    return i2;
}

/// \ingroup dbstl_containers
//@{
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//
// db_vector class template definition
//
/// The db_vector class has the union set of public member functions as
/// std::vector, std::deque and std::list, and each method has identical
/// default semantics to that in the std equivalent containers.
/// The difference is that the data is maintained using a Berkeley DB database
/// as well as some Berkeley DB related extensions.
/// \param T The type of data to store.
/// \param value_type_sub If T is a class/struct type, do not specify anything
/// for this parameter; Otherwise, specify ElementHolder<T> to it.
/// Database(dbp) and environment(penv) handle requirement(applies for all 
/// constructors of this class template):
/// dbp must meet the following requirement:
/// 1. dbp must be a DB_RECNO type of database handle.
/// 2. DB_THREAD must be set to dbp's open flags.
/// 3. An optional flag DB_RENUMBER is required if the container object
/// is supposed to be a std::vector or std::deque equivalent; Not 
/// required if it is a std::list equivalent. But dbstl will not check
/// whether DB_RENUMBER is set to this database handle. 
/// Setting DB_RENUMBER will cause the index values of all elements in
/// the underlying databse to be maintained consecutive and in order, 
/// which involves potentially a lot of work because many indices may
/// be updated. 
/// See the db_container(Db*, DbEnv*) for more information about the two
/// parameters.
/// \sa db_container db_container(Db*, DbEnv*) db_container(const db_container&)
//
template <Typename T, Typename value_type_sub>
class _exported db_vector: public db_container
{
private:
    typedef db_vector<T, value_type_sub> self;
    typedef db_recno_t index_type;
public:
    typedef T value_type;
    typedef value_type_sub value_type_wrap;
    typedef value_type_sub* pointer;
    typedef db_vector_iterator<T, value_type_sub> iterator;
    typedef db_vector_base_iterator<T> const_iterator;
    typedef index_type size_type;
    typedef db_reverse_iterator<iterator, const_iterator> reverse_iterator;
    typedef const value_type_sub* const_pointer;
    typedef db_reverse_iterator<const_iterator, iterator> 
        const_reverse_iterator;

    typedef typename value_type_sub::content_type const_value_type;

    // We don't use value_type_sub& as a reference type here because 
    // we created a local iterator object in operator[], and we must keep
    // an active cursor on the key/data pair. Thus operator[] needs to 
    // return an object rather than a reference to a local object.
    typedef value_type_sub reference;

    // We can't return reference here because we are using a local iterator.
    typedef const_value_type const_reference;
    typedef ptrdiff_t difference_type;

    /////////////////////////////////////////////////////////////////////
    // Begin functions that create iterators.
    /// \name Iterator functions.
    //@{
    /// \brief Create a read-write or read-only iterator. 
    ///
    /// We allow users to create a readonly 
    /// iterator here so that they don't have to use a const container 
    /// to create a const_iterator. But using const_iterator
    /// is faster. The flags set via db_container::set_cursor_oflags() is
    /// used as the cursor open flags.
    /// \param rmw Whether this iterator will open a Berkeley DB
    /// cursor with DB_RMW flag set. If the iterator is used to read a 
    /// key/data pair, then update it and store back to db, it is good 
    /// to set the DB_RMW flag, by specifying RMWItrOpt::read_modify_write()
    /// If you don't want to set the DB_RMW flag, specify 
    /// RMWItrOpt::no_read_modify_write(), which is the default behavior.
    /// \param readonly Whether the iterator is created as a readonly 
    /// iterator. Read only iterators can not update its underlying 
    /// key/data pair.
    /// \param bulk_read Whether read database key/data pairs in bulk, by
    /// specifying DB_MULTIPLE_KEY flag to underlying cursor's Dbc::get 
    /// function. Only readonly iterators can do bulk retrieval, if
    /// iterator is not read only, this parameter is ignored. Bulk 
    /// retrieval can accelerate reading speed because each database read
    /// operation will read many key/data pairs, thus saved many database
    /// read operations. The default bulk buffer size is 32KB, you can 
    /// set your desired bulk buffer size by specifying 
    /// BulkRetrievalOpt::bulk_retrieval(your_bulk_buffer_size); 
    /// If you don't want bulk retrieval, set 
    /// BulkRetrievalItrOpt::no_bulk_retrieval() as the real parameter.
    /// \param directdb_get Whether always read key/data pair from backing
    /// db rather than using the value cached in the iterator. The current
    /// key/data pair is cached in the iterator and always kept updated on
    /// iterator movement, but in some extreme conditions, errors can 
    /// happen if you use cached key/data pairs without always refreshing
    /// them from database. By default we are always reading from database
    /// when we are accessing the data the iterator sits on, except when 
    /// we are doing bulk retrievals. But your application can gain extra
    /// performance promotion if you can set this flag to false.
    /// \return The created iterator.
    /// \sa db_container::set_cursor_oflags();
    iterator begin(ReadModifyWriteOption rmw = 
        ReadModifyWriteOption::no_read_modify_write(),
        bool readonly = false, BulkRetrievalOption bulk_read =
        BulkRetrievalOption::no_bulk_retrieval(),
        bool directdb_get = true)
    {
        bool b_rmw;
        u_int32_t bulk_retrieval;

        bulk_retrieval = 0;
        b_rmw = (rmw == ReadModifyWriteOption::read_modify_write());
        if (readonly && b_rmw)
            b_rmw = false;
        // Bulk only available to readonly iterators.
        if (readonly && bulk_read ==
            BulkRetrievalOption::BulkRetrieval)
            bulk_retrieval = bulk_read.bulk_buf_size();

        iterator itr((db_container*)this, bulk_retrieval, b_rmw,
            directdb_get, readonly);

        open_itr(itr, readonly);
        itr.first();

        return itr;
    }

    /// \brief Create a const iterator.
    ///
    /// The created iterator can only be used to read its referenced
    /// data element. Can only be called when using a const reference to
    /// the contaienr object. The parameters have identical meanings and 
    /// usage to those of the other non-const begin function.
    /// \param bulkretrieval Same as that of 
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param directdb_get Same as that of
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \return The created const iterator.
    /// \sa begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    const_iterator begin(BulkRetrievalOption bulkretrieval =
        (BulkRetrievalOption::no_bulk_retrieval()),
        bool directdb_get = true) const
    {
        u_int32_t b_bulk_retrieval = (bulkretrieval ==
            BulkRetrievalOption::BulkRetrieval) ?
            bulkretrieval.bulk_buf_size() : 0;

        const_iterator itr((db_container*)this, b_bulk_retrieval, 
            false, directdb_get, true);

        open_itr(itr, true);
        itr.first();

        return itr;
    }
    
    /// \brief Create an open boundary iterator.
    /// \return Returns an invalid iterator denoting the position after 
    /// the last valid element of the container.
    inline iterator end()
    {
        end_itr_.owner_ = (db_container*)this;
        end_itr_.inval_pos_type_ = db_base_iterator<T>::IPT_AFTER_LAST;
        return end_itr_;
    }

    /// \brief Create an open boundary iterator.
    /// \return Returns an invalid const iterator denoting the position 
    /// after the last valid element of the container.
    inline const_iterator end() const
    {
        end_itr_.owner_ = (db_container*)this;
        end_itr_.inval_pos_type_ = db_base_iterator<T>::IPT_AFTER_LAST;
        return end_itr_;
    }

    /// \brief Create a reverse iterator.
    ///
    /// This function creates a reverse iterator initialized to sit on the
    /// last element in the underlying database, and can be used to 
    /// read/write. The meaning and usage of
    /// its parameters are identical to the above begin function.
    /// \param rmw Same as that of 
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param bulk_read Same as that of 
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param directdb_get Same as that of
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param readonly Same as that of 
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \return The created iterator.
    /// \sa begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    reverse_iterator rbegin(
        ReadModifyWriteOption rmw = 
        ReadModifyWriteOption::no_read_modify_write(),
        bool readonly = false, BulkRetrievalOption bulk_read =
        BulkRetrievalOption::no_bulk_retrieval(),
        bool directdb_get = true)
    {
        u_int32_t bulk_retrieval = 0;

        reverse_iterator itr(end());
        itr.rmw_csr_ = 
            (rmw == (ReadModifyWriteOption::read_modify_write()));
        itr.directdb_get_ = directdb_get;
        itr.read_only_ = readonly;
        itr.owner_ = (db_container*)this;

        // Bulk only available to readonly iterators.
        if (readonly && 
            bulk_read == BulkRetrievalOption::BulkRetrieval)
            bulk_retrieval = bulk_read.bulk_buf_size();
        itr.bulk_retrieval_ = bulk_retrieval;

        return itr;
    }

    /// \brief Create a const reverse iterator.
    ///
    /// This function creates a const reverse iterator initialized to sit
    /// on the last element in the backing database, and can only read the
    /// element, it is only available to const db_vector containers. 
    /// The meaning and usage of its parameters are identical as above.
    /// \param bulkretrieval Same as that of 
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \param directdb_get Same as that of
    /// begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    /// \return The created iterator.
    /// \sa begin(ReadModifyWrite, bool, BulkRetrievalOption, bool);
    const_reverse_iterator rbegin(BulkRetrievalOption bulkretrieval =
        BulkRetrievalOption(BulkRetrievalOption::no_bulk_retrieval()),
        bool directdb_get = true) const
    {
        const_reverse_iterator itr(end());
        itr.bulk_retrieval_ = 
            (bulkretrieval == (BulkRetrievalOption::BulkRetrieval) ?
            bulkretrieval.bulk_buf_size() : 0);

        itr.directdb_get_ = directdb_get;
        itr.read_only_ = true;
        itr.owner_ = (db_container*)this;

        return itr;
    }

    /// \brief Create an open boundary iterator.
    /// \return Returns an invalid iterator denoting the position 
    /// before the first valid element of the container.
    inline reverse_iterator rend()  
    {
        reverse_iterator itr; // No cursor created.

        itr.itr_status_ = INVALID_ITERATOR_POSITION;
        itr.owner_ = (db_container*)this;
        itr.inval_pos_type_ = db_base_iterator<T>::IPT_BEFORE_FIRST;
        return itr;
    }

    /// \brief Create an open boundary iterator.
    /// \return Returns an invalid const iterator denoting the position 
    /// before the first valid element of the container. 
    inline const_reverse_iterator rend() const
    {
        const_reverse_iterator itr; // No cursor created.

        itr.itr_status_ = INVALID_ITERATOR_POSITION;
        itr.owner_ = (db_container*)this;
        itr.inval_pos_type_ = db_base_iterator<T>::IPT_BEFORE_FIRST;
        return itr;
    }
    //@} // iterator_funcs
    /////////////////////////////////////////////////////////////////////

    /// \brief Get container size.
    /// This function supports auto-commit.
    // All container methods using internal working iterators can be used
    // to implement autocommit if no DB operations can be directly used,
    // because the work iterator is not supposed to point to a specific
    // record before reopening it.
    /// \return Return the number of elements in this container.
    /// \sa http://www.cplusplus.com/reference/stl/vector/size.html
    size_type size() const
    {
        index_type sz;

        try {
            this->begin_txn();
            const_iterator derefitr;
            init_itr(derefitr);
            open_itr(derefitr, true);
            sz = derefitr.last() + 1;
            this->commit_txn();
            return (size_type)sz; // Largest index is the size.
        } catch (...) {
            this->abort_txn();
            throw;
        }
    }

    /// \name Huge return
    /// These two functions return 2^30, denoting a huge number that does
    /// not overflow, because dbstl does not have to manage memory space.
    /// But the return value is not the real limit, see the Berkeley DB 
    /// database limits for the limits.
    //@{
    /// Get max size.
    /// The returned size is not the actual limit of database. See the
    /// Berkeley DB limits to get real max size.
    /// \return A meaningless huge number.
    inline size_type max_size() const
    {
        return SIZE_T_MAX;
    }

    /// Get capacity.
    inline size_type capacity() const
    {
        return SIZE_T_MAX;
    }
    //@}

    /// Returns whether this container is empty.
    /// \return True if empty, false otherwise.
    inline bool empty() const
    {
        const_iterator itr = begin();
        return itr.itr_status_ == INVALID_ITERATOR_POSITION;
    }

    /////////////////////////////////////////////////////////////////////
    // Begin element access functions.
    //
    /// \name Element access functions.
    /// The operator[] and at() only come from std::vector and std::deque, 
    /// If you are using db_vector as std::list, you don't have
    /// to set DB_RENUMBER flag to the backing database handle, and you get
    /// better performance, but at the same time you can't use these 
    /// functions. Otherwise if you have set the DB_RENUMBER flag to the 
    /// backing database handle, you can use this function though it is an
    /// std::list equivalent.
    //@{
    /// Index operator, can act as both a left value and a right value.
    /// \param n The valid index of the vector.
    /// \return The reference to the element at specified position.
    inline reference operator[](index_type n)
    {
        iterator derefitr, *pitr;

        init_itr(derefitr);
        open_itr(derefitr);
        if (n == INVALID_INDEX)
            n = derefitr.last();
        derefitr.move_to(n);
        
        pitr = new iterator(derefitr);
        reference ref(pitr->curpair_);
        ref._DB_STL_SetDelItr();
        return ref;
    }

    /// \brief Read only index operator.
    ///
    /// Only used as a right value, no need for assignment capability.
    /// The return value can't be used to update the element.
    /// \param n The valid index of the vector.
    /// \return The const reference to the element at specified position.
    inline const_reference operator[](index_type n) const
    {
        const_iterator derefitr;

        init_itr(derefitr);
        open_itr(derefitr);
        if (n == INVALID_INDEX)
            n = derefitr.last();
        derefitr.move_to(n);

        // _DB_STL_value returns a reference
        return (*derefitr);
    }

    /// \brief Index function.
    /// \param n The valid index of the vector.
    /// \return The reference to the element at specified position, can 
    /// act as both a left value and a right value.
    /// \sa http://www.cplusplus.com/reference/stl/vector/at.html
    inline reference at(index_type n)
    {
        return (*this)[n];
    }

    /// \brief Read only index function.
    ///
    /// Only used as a right value, no need for assignment capability.
    /// The return value can't be used to update the element.
    /// \param n The valid index of the vector.
    /// \return The const reference to the element at specified position.
    /// \sa http://www.cplusplus.com/reference/stl/vector/at.html
    inline const_reference at(index_type n) const
    {
           return (*this)[n];
    }

    /// Return a reference to the first element.
    /// \return Return a reference to the first element.
    /// \sa http://www.cplusplus.com/reference/stl/vector/front.html 
    inline reference front()
    {
        iterator itr, *pitr;

        init_itr(itr);
        open_itr(itr);
        itr.first();
        
        pitr = new iterator(itr);
        reference ref(pitr->curpair_);
        ref._DB_STL_SetDelItr();
        return ref;
    }

    /// \brief Return a const reference to the first element. 
    ///
    /// The return value can't be used to update the element.
    /// \return Return a const reference to the first element. 
    /// \sa http://www.cplusplus.com/reference/stl/vector/front.html 
    inline const_reference front() const
    {
        const_iterator itr;

        init_itr(itr);
        open_itr(itr);
        itr.first();
        return (*itr);
    }

    /// Return a reference to the last element.
    /// \return Return a reference to the last element.
    /// \sa http://www.cplusplus.com/reference/stl/vector/back.html
    inline reference back()
    {
        iterator itr, *pitr;

        init_itr(itr);
        open_itr(itr);
        itr.last();
        
        pitr = new iterator(itr);
        reference ref(pitr->curpair_);
        ref._DB_STL_SetDelItr();
        return ref;
    }

    /// \brief Return a reference to the last element. 
    ///
    /// The return value can't be used to update the element.
    /// \return Return a reference to the last element. 
    /// \sa http://www.cplusplus.com/reference/stl/vector/back.html
    inline const_reference back() const
    {
        const_iterator itr;

        init_itr(itr);
        open_itr(itr);
        itr.last();
        return (*itr);
    }
    //@}

    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    //
    // Begin db_vector constructors and destructor. 
    /// \brief Constructor.
    ///
    /// Note that we do not need an allocator in db-stl containser, but
    /// we need backing up Db* and DbEnv*, and we have to verify that the
    /// passed in bdb handles are valid for use by the container class.
    /// See class detail for handle requirement.
    /// \param dbp The same as that of db_container(Db*, DbEnv*);
    /// \param penv The same as that of db_container(Db*, DbEnv*);
    /// \sa db_container(Db*, DbEnv*);
    //
    explicit db_vector(Db* dbp = NULL, DbEnv *penv = NULL)
        : base(dbp, penv)
    {
        const char *errmsg;

        this->open_db_handles(dbp, penv, DB_RECNO, DB_CREATE |
            DB_THREAD, DB_RENUMBER);
        if ((errmsg = verify_config(dbp, penv)) != NULL) {
            THROW(InvalidArgumentException, ("Db*", errmsg));
        }
        this->set_db_handle_int(dbp, penv);
    }

    /// \brief Constructor.
    ///
    /// This function supports auto-commit. 
    /// Insert n elements of T type into the database, the value of the 
    /// elements is the default value or user set value.
    /// See class detail for handle requirement.
    /// \param n The number of elements to insert.
    /// \param val The value of elements to insert.
    /// \param dbp The same as that of db_container(Db*, DbEnv*);
    /// \param penv The same as that of db_container(Db*, DbEnv*);
    /// \sa db_vector(Db*, DbEnv*); db_container(Db*, DbEnv*);
    explicit db_vector(size_type n, const T& val = T(), Db* dbp = NULL,
        DbEnv *penv = NULL) : base(dbp, penv)
    {
        size_type i;
        const char *errmsg;

        this->open_db_handles(dbp, penv, DB_RECNO, DB_CREATE |
            DB_THREAD, DB_RENUMBER);
        if ((errmsg = verify_config(dbp, penv)) != NULL) {
            THROW(InvalidArgumentException, ("Db*", errmsg));
        }
        this->set_db_handle_int(dbp, penv);

        this->begin_txn();
        // This transaction will prevent push_back to autocommit,
        // as expected, because a single push_back should not be
        // automatic in this function
        //
        try {
            for (i = 0; i < n; i++)
                push_back(val);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
    }

    /// \brief Copy constructor.
    ///
    /// This function supports auto-commit. 
    /// Insert all elements in x into this container.
    /// \sa db_container(const db_container&)
    db_vector(const self& x) : db_container(x)
    {
        // This objects underlying db should have been opened already,
        // only copy db contents.
        //
        verify_db_handles(x);
        this->set_db_handle_int(this->clone_db_config(
            x.get_db_handle()), x.get_db_env_handle());

        this->begin_txn();

        try {
            copydb(x);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
    }

    /// Range constructor.
    /// This function supports auto-commit. 
    // The order of parameters has to be altered in order to avoid
    // clashing with the other constuctor above (the one with size_type
    // as first parameter).
    /// Insert a range of elements into this container. The range is 
    /// [first, last), which contains elements that can be converted to 
    /// type T automatically.
    /// See class detail for handle requirement.
    /// \param dbp The same as that of db_container(Db*, DbEnv*);
    /// \param penv The same as that of db_container(Db*, DbEnv*);
    /// \param first Range closed boundary.
    /// \param last Range open boundary.
    /// \sa db_vector(Db*, DbEnv*);
    template <class InputIterator>
    db_vector(Db*dbp, DbEnv *penv,
        InputIterator first, InputIterator last) : base(dbp, penv)
    {
        const char *errmsg;

        this->open_db_handles(dbp, penv, DB_RECNO, DB_CREATE |
            DB_THREAD, DB_RENUMBER);
        if ((errmsg = verify_config(dbp, penv)) != NULL)
            THROW(InvalidArgumentException, ("Db*", errmsg));

        this->set_db_handle_int(dbp, penv);

        this->begin_txn();
        try {
            push_range(first, last);
        } catch (...) {
            this->abort_txn();
            throw;
        }

        this->commit_txn();
    }

    /// Range constructor.
    /// This function supports auto-commit. 
    /// Insert the range of elements in [first, last) into this container.
    /// See class detail for handle requirement.
    /// \param dbp The same as that of db_container(Db*, DbEnv*);
    /// \param penv The same as that of db_container(Db*, DbEnv*);
    /// \param first Range closed boundary.
    /// \param last Range open boundary.
    /// \sa db_vector(Db*, DbEnv*);
    db_vector(const_iterator first, const_iterator last,
        Db*dbp = NULL, DbEnv *penv = NULL) : base(dbp, penv)
    {
        const char *errmsg;

        this->open_db_handles(dbp, penv, DB_RECNO, DB_CREATE |
            DB_THREAD, DB_RENUMBER);
        if ((errmsg = verify_config(dbp, penv)) != NULL)
            THROW(InvalidArgumentException, ("Db*", errmsg));
        this->set_db_handle_int(dbp, penv);

        this->begin_txn();
        try {
            push_range(first, last);
        } catch (...) {
            this->abort_txn();
            throw;
        }

        this->commit_txn();
    }

    // We don't have to close Berkeley DB database or environment handles
    // because all related handles are managed by ResourceManager and 
    // closed in the right order when the program exits.
    //
    virtual ~db_vector() {
    }

    ////////////////////////////////////////////////////////////////////

    /// \brief Container assignment operator.
    ///
    /// This function supports auto-commit. 
    /// This db_vector is assumed to be valid for use, only copy
    /// content of x into this container.
    /// \param x The right value container.
    /// \return The container x's reference.
    const self& operator=(const self& x)
    {
        ASSIGNMENT_PREDCOND(x)
        // TODO: rename verify_db_handles to validate_db_handle
        db_container::operator =(x);
        verify_db_handles(x);
        this->begin_txn();
        try {
            copydb(x);
        } catch (...) {
            this->abort_txn();
            throw;
        }

        this->commit_txn();
        return x;
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Begin db_vector comparison functions.
    /// \name Compare functions.
    /// \sa http://www.sgi.com/tech/stl/Vector.html
    //@{ 
    /// \brief Container equality comparison operator.
    ///
    /// This function supports auto-commit. 
    /// \return Compare two vectors, return true if they have identical
    /// sequences of elements, otherwise return false.
    /// \param v2 The vector to compare against.
    template <Typename T2, Typename T3>
    bool operator==(const db_vector<T2, T3>& v2) const
    {
        bool ret;
        size_t sz;

        verify_db_handles(v2);
        typename self::iterator i1;
        typename db_vector<T2, T3>::iterator i2;

        try {
            this->begin_txn();
            if ((sz = this->size()) != v2.size()) {
                ret = false;
                this->commit_txn();
                return ret;
            }
            if (sz == 0) {
                ret = true;
                this->commit_txn();
                return ret;
            }

            // Identical size, compare each element.
            for (i1 = begin(), i2 = v2.begin(); i1 != end() &&
                i2 != v2.end(); ++i1, ++i2)
                if (!((T)(*i1) == (T2)(*i2))) {
                    ret = false;
                    this->commit_txn();
                    return ret;
                }

            // All elements equal, the two vectors are equal.
            ret = true;
            this->commit_txn();
            return ret;
        } catch (...) {
            this->abort_txn();
            throw;
        }
    }

    /// \brief Container in-equality comparison operator.
    ///
    /// This function supports auto-commit. 
    /// \param v2 The vector to compare against.
    /// \return Returns false if elements in each slot of both 
    /// containers equal; Returns true otherwise.
    template <Typename T2, Typename T3>
    bool operator!=(const db_vector<T2, T3>& v2) const
    {
        return !this->operator==(v2);
    }

    /// \brief Container equality comparison operator.
    ///
    /// This function supports auto-commit. 
    /// \return Compare two vectors, return true if they have identical 
    /// elements, otherwise return false.
    bool operator==(const self& v2) const
    {
        bool ret;

        COMPARE_CHECK(v2)
        verify_db_handles(v2);

        try {
            this->begin_txn();
            if (this->size() != v2.size()) {
                ret = false;
                this->commit_txn();
                return ret;
            }
            typename self::const_iterator i1;
            typename self::const_iterator i2;
            // Identical size, compare each element.
            for (i1 = begin(), i2 = v2.begin(); i1 != end() &&
                i2 != v2.end(); ++i1, ++i2)
                if (!(*i1 == *i2)) {
                    ret = false;
                    this->commit_txn();
                    return ret;
                }

            // All elements are equal, the two vectors are equal.
            ret = true;
            this->commit_txn();
            return ret;
        } catch (...) {
            this->abort_txn();
            throw;
        }
    }

    /// \brief Container in-equality comparison operator.
    ///
    /// This function supports auto-commit. 
    /// \param v2 The vector to compare against.
    /// \return Returns false if elements in each slot of both 
    /// containers equal; Returns true otherwise.
    bool operator!=(const self& v2) const
    {
        return !this->operator==(v2);
    }

    /// \brief Container less than comparison operator.
    ///
    /// This function supports auto-commit. 
    /// \param v2 The container to compare against.
    /// \return Compare two vectors, return true if this is less 
    /// than v2, otherwise return false.
    bool operator<(const self& v2) const
    {
        bool ret;

        if (this == &v2)
            return false;

        verify_db_handles(v2);
        typename self::const_iterator i1;
        typename self::const_iterator i2;
        size_t s1, s2, sz, i;

        try {
            this->begin_txn();
            s1 = this->size();
            s2 = v2.size();
            sz = s1 < s2 ? s1 : s2;

            // Compare each element.
            for (i1 = begin(), i = 0, i2 = v2.begin();
                i < sz;
                ++i1, ++i2, ++i) {
                if (*i1 == *i2)
                    continue;
                else {
                    if (*i1 < *i2)
                        ret = true;
                    else
                        ret = false;
                    this->commit_txn();
                    return ret;
                }
            }

            ret = s1 < s2;
            this->commit_txn();
            return ret;
        } catch (...) {
            this->abort_txn();
            throw;
        }
    }
    //@} // cmp_funcs
    ////////////////////////////////////////////////////////////////////

    /// \brief Resize this container to specified size n, insert values t
    /// if need to enlarge the container.
    ///
    /// This function supports auto-commit. 
    /// \param n The number of elements in this container after the call.
    /// \param t The value to insert when enlarging the container. 
    /// \sa http://www.cplusplus.com/reference/stl/vector/resize.html
    inline void resize(size_type n, T t = T())
    {
        size_t i, sz;

        try {
            begin_txn();
            if (n == (sz = size())) {
                commit_txn();
                return;
            }

            if (n < sz) // Remove sz - n elements at tail.
                erase(begin() + n, end());
            else
                for (i = sz; i < n; i++)
                    push_back(t);
            commit_txn();
        } catch (...) {
            abort_txn();
            throw;
        }
    }

    /// \brief Reserve space.
    ///
    /// The vector is backed by Berkeley DB, we always have enough space.
    /// This function does nothing, because dbstl does not have to manage
    /// memory space.
    inline void reserve(size_type /* n */)
    {
    }

    /** \name Assign functions
    See the function documentation for the correct usage of b_truncate 
    parameter.
    @{
    The following four member functions have default parameter b_truncate,
    because they require all key/data pairs in the database be deleted 
    before the real operation, and by default we use Db::truncate to 
    truncate the database rather than delete the key/data pairs one by
    one, but Db::truncate requirs no open cursors on the database handle,
    and the four member functions will close any open cursors of backing 
    database handle in current thread, but can do nothing to cursors of 
    other threads opened from the same database handle. 
    So you must make sure there are no open cursors of the database handle
    in any other threads. On the other hand, users can specify "false" to
    the b_truncate parameter and thus the key/data pairs will be deleted
    one by one. Other than that, they have identical behaviors as their 
    counterparts in std::vector. 
    \sa http://www.cplusplus.com/reference/stl/vector/assign.html
    */
    /// Assign a range [first, last) to this container.
    /// \param first The range closed boundary.
    /// \param last The range open boundary.
    /// \param b_truncate See its member group doc for details.
    template <class InputIterator>
    void assign ( InputIterator first, InputIterator last, 
        bool b_truncate = true)
    {
        if (this->get_db_handle() == NULL)
            return;

        this->begin_txn();
        try {
            clear(b_truncate);
            push_range(first, last);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
    }

    /// Assign a range [first, last) to this container.
    /// \param first The range closed boundary.
    /// \param last The range open boundary.
    /// \param b_truncate See its member group doc for details.
    void assign(const_iterator first, const_iterator last, 
        bool b_truncate = true)
    {
        if (this->get_db_handle() == NULL)
            return;

        this->begin_txn();
        try {
            clear(b_truncate);
            push_range(first, last);

        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
    }

    /// Assign n number of elements of value u into this container.
    /// \param n The number of elements in this container after the call.
    /// \param u The value of elements to insert.
    /// \param b_truncate See its member group doc for details.
    /// This function supports auto-commit. 
    void assign ( size_type n, const T& u, bool b_truncate = true)
    {
        if (this->get_db_handle() == NULL)
            return;

        this->begin_txn();
        try {
            clear(b_truncate);
            size_t i;
            for (i = 0; i < n; i++)
                push_back(u);
        } catch (...) {
            this->abort_txn();
            throw;
        }

        this->commit_txn();
    }
    //@} // assign_funcs

    // Directly use DB->put, so that when there is no explicit transaction,
    // it is autocommit. This
    // method is often called by other db_vector methods, in that case
    // those methods will begin/commit_txn internally, causing push_back
    // to not autocommit, as expected.
    /// \brief Push back an element into the vector.
    ///
    /// This function supports auto-commit. 
    /// \param x The value of element to push into this vector.
    /// \sa http://www.cplusplus.com/reference/stl/vector/push_back.html
    inline void push_back ( const T& x )
    {
        index_type k0 = 0; // This value is ignored.
        int ret;

        // x may be an temporary object, so must copy it.
        DataItem dt(x, false), k(k0, true);
        // In CDS mode, the current transaction is the DB_TXN created
        // by cds_group_begin.
        BDBOP(this->get_db_handle()->put(ResourceManager::instance()->
            current_txn(this->get_db_env_handle()),
            &(k.get_dbt()), &(dt.get_dbt()), DB_APPEND), ret);
    }

    /// \brief Pop out last element from the vector.
    ///
    /// This function supports auto-commit. 
    /// \sa http://www.cplusplus.com/reference/stl/vector/pop_back.html
    void pop_back ()
    {
        try {
            iterator derefitr;

            this->begin_txn();
            init_itr(derefitr);
            open_itr(derefitr);
            derefitr.last();
            derefitr.pcsr_->del();
            this->commit_txn();
        } catch(...) {
            this->abort_txn();
            throw;
        }
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Begin std::deque and std::list specific public functions.
    //
    // These methods are not in std::vector, but are in std::list and
    // std::deque. They are defined here so that db_vector can be used to
    // replace std::list and std::deque.
    /// \name Functions specific to deque and list
    /// These functions come from std::list and std::deque, and have
    /// identical behaviors to their counterparts in std::list/std::deque.
    /// \sa http://www.cplusplus.com/reference/stl/deque/pop_front.html 
    /// http://www.cplusplus.com/reference/stl/deque/push_front.html
    //@{
    /// \brief Push an element x into the vector from front.
    /// \param x The element to push into this vector.
    /// This function supports auto-commit. 
    void push_front (const T& x)
    {
        int flag, ret;

        try {
            this->begin_txn();
            iterator derefitr;
            init_itr(derefitr);
            open_itr(derefitr);
            // MOVE iterator and cursor to 1st element.
            ret = derefitr.first();
            if (ret < 0)
                flag = DB_KEYLAST; // empty
            else
                flag = DB_BEFORE;
            derefitr.pcsr_->insert(x, flag);
            this->commit_txn();
        } catch(...) {
            this->abort_txn();
            throw;
        }
    }

    /// \brief Pop out the front element from the vector.
    ///
    /// This function supports auto-commit. 
    void pop_front ()
    {
        try {
            this->begin_txn();
            iterator derefitr;
            init_itr(derefitr);
            open_itr(derefitr);
            derefitr.first();
            derefitr.pcsr_->del();
            this->commit_txn();
        } catch(...) {
            this->abort_txn();
            throw;
        }

    }
    //@}
    ////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    //
    // Begin insert and erase functions.
    //
    // This method can not be autocommit because pos can not be reopened
    // while it already points to a dest position. In order to gain
    // transaction it should have already been opened in a transactional
    // context, so it is meaningless to wrap the insert operation with
    // begin/commit transaction in this method.
    /// \name Insert functions
    /// The iterator pos in the functions must be a read-write iterator,
    /// can't be read only.
    /// \sa http://www.cplusplus.com/reference/stl/vector/insert.html
    //@{
    /// \brief Insert x before position pos.
    /// \param x The element to insert.
    /// \param pos The position before which to insert. 
    iterator insert (iterator pos, const T& x)
    {
        u_int32_t flag;
        bool isempty;

        make_insert_args(pos, flag, isempty);

        pos.pcsr_->insert(x, flag);
        pos.update_cur_pair(); // Sync with underlying cursor.

        return pos; // Returns the new position's iterator.
    }

    /// \brief Insert n number of elements x before position pos.
    /// \param x The element to insert.
    /// \param pos The position before which to insert. 
    /// \param n The number of elements to insert.
    void insert (iterator pos, size_type n, const T& x)
    {
        u_int32_t flag;
        size_t i;
        bool isempty;

        make_insert_args(pos, flag, isempty);

        for (i = 0; i < n; i++) {
            pos.pcsr_->insert(x, flag);
            pos.update_cur_pair();
            // Move the underlying Dbc*cursor to next record
            // (i.e. the orig record it pointed to before
            // the insertion). So it will point to
            // the new record after insertion.
            //
            if (flag == DB_BEFORE)
                ++pos;

            // If using DB_AFTER flag, no need to move because
            // cursor already points to the newly inserted record
            // after the orig record it pointed to.
            //

            // There is already data in the underlying database
            // so use DB_BEFORE unless pos is begin() and the
            // vector was empty before this insert call.
            //
            if (flag == DB_KEYLAST) {
                if(isempty)
                    flag = DB_AFTER;
                else
             // This branch can never be reached because any
             // iterator of a empty container can only have its
             // cursor at the begin() position.
             //
                    flag = DB_BEFORE;
            }
        }
    }

    /// \brief Range insertion.
    ///
    /// Insert elements in range [first, last) into this vector before
    /// position pos.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    /// \param pos The position before which to insert. 
    template <class InputIterator>
    void insert (iterator pos, InputIterator first, InputIterator last)
    {
        u_int32_t flag;
        InputIterator itr;
        bool isempty;

        make_insert_args(pos, flag, isempty);
        // !!!XXX
        // The cursor will point to the newly inserted record, so we
        // need to move the cursor to the original one.
        //

        for (itr = first; itr != last; ++itr) {
            pos.pcsr_->insert(*itr, flag);
            pos.update_cur_pair();
            // Move the underlying Dbc*cursor to next record
            // (i.e. the orig record it pointed to before
            // the insertion). So it will point to
            // the new record after insertion.
            //
            if (flag == DB_BEFORE)
                ++pos;
            // There is already data in the underlying database
            // so use DB_BEFORE unless pos is begin() and the
            // vector was empty before this insert call.
            //
            if (flag == DB_KEYLAST) {
                // pos == begin() && this was empty
                if(isempty)
                    flag = DB_AFTER;
                else
             // This branch can never be reached because any
             // iterator of a empty container can only have its
             // cursor at the begin() position.
             //
                    flag = DB_BEFORE;
            }
        }
    }

    // this method can not be autocommitted, reason as above
    /// \brief Range insertion.
    ///
    /// Insert elements in range [first, last) into this vector before
    /// position pos.
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    /// \param pos The position before which to insert. 
    void insert (iterator pos, const_iterator first, const_iterator last)
    {
        u_int32_t flag;
        iterator itr;
        bool isempty;

        make_insert_args(pos, flag, isempty);

        for (itr = first; itr != last; ++itr) {
            pos.pcsr_->insert(*itr, flag);
            pos.update_cur_pair();
            // Move the underlying Dbc*cursor to next record
            // (i.e. the orig record it pointed to before
            // the insertion). So it will point to
            // the new record after insertion.
            //
            if (flag == DB_BEFORE)
                ++pos;
            // There is already data in the underlying database
            // so use DB_BEFORE unless pos is begin() and the
            // vector was empty before this insert call.
            //
            if (flag == DB_KEYLAST) {
                // pos == begin() && this was empty.
                if(isempty)
                    flag = DB_AFTER;
                else
                    flag = DB_BEFORE;
            }
        }
    }
    //@}

    /// \name Erase functions
    /// The iterator pos in the functions must be a read-write iterator,
    /// can't be read only.
    /// \sa http://www.cplusplus.com/reference/stl/vector/erase.html
    //@{
    /// \brief Erase element at position pos.
    /// \param pos The valid position in the container's range to erase.
    /// \return The next position after the erased element.
    inline iterator erase (iterator pos)
    {
        if (pos == end_itr_)
            return pos;
        pos.pcsr_->del();
        ++pos;
        // Synchronized with underlying cursor.
        return pos;
    }

    /// \brief Erase elements in range [first, last)
    /// \param first The closed boundary of the range.
    /// \param last The open boundary of the range.
    /// \return The next position after the erased elements.
    iterator erase (iterator first, iterator last)
    {
        iterator itr;
        int ret = 0;
        Dbt k, d;

        // If ret is non-zero, it is because there is no element in
        // this container any more.
        //
        for (itr = first; itr != last && ret == 0; ++itr) {
            if (itr == end_itr_)
                return itr;
            ret = itr.pcsr_->del();
        }
        return itr;
    }
    //@}
    ////////////////////////////////////////////////////////////////////

    /// \brief Swap content with another vector vec.
    /// \param vec The other vector to swap content with.
    /// This function supports auto-commit. 
    /// \sa http://www.cplusplus.com/reference/stl/vector/swap.html
    void swap (self& vec)
    {
        T tmp;
        size_t sz, vsz, i, j, m;
        self& me = *this;
        self *longer, *shorter;

        verify_db_handles(vec);
        this->begin_txn();
        try {
            sz = this->size();
            vsz = vec.size();
            // do swap
            for (i = 0; (i < sz) && (i < vsz); i++) {
                tmp = me[(index_type)i];
                me[(index_type)i] = vec[(index_type)i];
                vec[(index_type)i] = tmp;
            }

            // Move the longer vector's remaining part to the
            // shorter one.
            //
            if (sz == vsz)
                return;
            else if (sz < vsz) {
                longer = &vec;
                shorter = &me;
                j = vsz;
            } else {
                longer = &me;
                shorter = &vec;
                j = sz;
            }

            self &lv = *longer;
            self &sv = *shorter;
            m = i;
            for (; i < j; i++)
                sv.push_back(lv[(index_type)i]);

            typename self::iterator itr1 = 
                lv.begin() + (int)m, itr2 = lv.end();

            lv.erase(itr1, itr2);
        } catch (...) {
            this->abort_txn();
            throw;
        }
        this->commit_txn();
    }

    // When DB_AUTO_COMMIT is set, no transaction needs to be begun to
    // support autocommit because DB->truncate internally supports it.
    /// Remove all elements of the vector, make it an empty vector.
    /// This function supports auto-commit. 
    /// \param b_truncate Same as that of db_vector::assign().
    /// \sa http://www.cplusplus.com/reference/stl/vector/clear.html
    void clear(bool b_truncate = true)
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

            // DB_RMW flag requires locking subsystem.
            if ((flag & DB_INIT_LOCK) || (flag & DB_INIT_CDB) ||
                (flag & DB_INIT_TXN))
                brmw = 
                    ReadModifyWriteOption::read_modify_write();

            try { // Truncate is capable of autocommit internally.
                this->begin_txn();
                erase(begin(brmw, false), end());
                this->commit_txn();
            } catch (...) {
                this->abort_txn();
                throw;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Begin methods only defined in std::list class.
    /// \name std::list specific functions
    /// \sa http://www.cplusplus.com/reference/stl/list/
    //@{
    /// \brief Remove all elements whose values are "value" from the list.
    ///
    /// This function supports auto-commit. 
    /// \param value The target value to remove.
    /// \sa http://www.cplusplus.com/reference/stl/list/remove/
    void remove(const T& value)
    {
        iterator i;

        try {
            begin_txn();
            for (i = begin(); i != end(); ++i)
                if (*i == value)
                    erase(i);
            commit_txn();
        } catch (...) {
            abort_txn();
            throw;
        }
    }

    /// \brief Remove all elements making "pred" return true.
    ///
    /// This function supports auto-commit. 
    /// \param pred The binary predicate judging elements in this list.
    /// \sa http://www.cplusplus.com/reference/stl/list/remove_if/
    template <class Predicate>
    void remove_if(Predicate pred)
    {
        iterator i;

        try {
            begin_txn();
            for (i = begin(); i != end(); ++i)
                if (pred(*i))
                    erase(i);
            commit_txn();
        } catch (...) {
            abort_txn();
            throw;
        }
    }

    /// \brief Merge content with another container.
    ///
    /// This function supports auto-commit. 
    /// \param x The other list to merge with.
    /// \sa http://www.cplusplus.com/reference/stl/list/merge/
    void merge(self& x)
    {
        DbstlListSpecialOps<T, value_type_sub> obj(this);
        obj.merge(x);
    }

    
    /// \brief Merge content with another container.
    ///
    /// This function supports auto-commit. 
    /// \param x The other list to merge with.
    /// \param comp The compare function to determine insertion position.
    /// \sa http://www.cplusplus.com/reference/stl/list/merge/
    template <class Compare>
    void merge(self& x, Compare comp)
    {
        verify_db_handles(x);
        iterator itr, itrx;

        try {
            begin_txn();
            for (itr = begin(), itrx = x.begin();
                itr != end_itr_ && itrx != x.end();) {
                if (!comp(*itr, *itrx)) {
                    insert(itr, *itrx);
                    ++itrx;
                } else
                    ++itr;
            }
            if (itr == end_itr_ && itrx != x.end())
                insert(itr, itrx, x.end());
            x.clear();
            commit_txn();
        } catch (...) {
            abort_txn();
            throw;
        }
    }

    /// \brief Remove consecutive duplicate values from this list.
    ///
    /// This function supports auto-commit. 
    /// \sa http://www.cplusplus.com/reference/stl/list/unique/
    void unique()
    {
        DbstlListSpecialOps<T, value_type_sub> obj(this);
        obj.unique();
    }

    /// \brief Remove consecutive duplicate values from this list.
    ///
    /// This function supports auto-commit. 
    /// \param binary_pred The compare predicate to dertermine uniqueness.
    /// \sa http://www.cplusplus.com/reference/stl/list/unique/
    template <class BinaryPredicate>
    void unique(BinaryPredicate binary_pred)
    {
        DbstlListSpecialOps<T, value_type_sub> obj(this);
        obj.unique(binary_pred);
    }

    /// \brief Sort this list.
    ///
    /// This function supports auto-commit. 
    /// \sa http://www.cplusplus.com/reference/stl/list/sort/
    void sort()
    {
        DbstlListSpecialOps<T, value_type_sub> obj(this);
        obj.sort();
    }

    /// \brief Sort this list.
    ///
    /// This function supports auto-commit. 
    /// \param comp The compare operator to determine element order.
    /// \sa http://www.cplusplus.com/reference/stl/list/sort/
    template <class Compare>
    void sort(Compare comp)
    {
        DbstlListSpecialOps<T, value_type_sub> obj(this);
        obj.sort(comp);
    }

    /// \brief Reverse this list.
    ///
    /// This function supports auto-commit. 
    /// \sa http://www.cplusplus.com/reference/stl/list/reverse/
    void reverse()
    {
        try {
            self tmp;
            const self &cthis = *this;
            const_reverse_iterator ri;

            begin_txn();
            for (ri = cthis.rbegin(BulkRetrievalOption::
                bulk_retrieval()); ri != rend(); ++ri)
                tmp.push_back(*ri);
            assign(tmp.begin(), tmp.end());
            commit_txn();
        } catch (...) {
            abort_txn();
            throw;
        }
    }

    /// \brief Moves elements from list x into this list.
    ///
    /// Moves all elements in list x into this list 
    /// container at the 
    /// specified position, effectively inserting the specified 
    /// elements into the container and removing them from x.
    /// This function supports auto-commit. 
    /// \param position Position within the container where the elements 
    /// of x are inserted.
    /// \param x The other list container to splice from.
    /// \sa http://www.cplusplus.com/reference/stl/list/splice/
    void splice (iterator position, self& x)
    {
        verify_db_handles(x);
        try {
            begin_txn();
            insert(position, x.begin(), x.end());
            x.clear();
            commit_txn();
        } catch (...) {
            abort_txn();
            throw;
        }
    }

    /// \brief Moves elements from list x into this list.
    ///
    /// Moves elements at position i of list x into this list 
    /// container at the 
    /// specified position, effectively inserting the specified 
    /// elements into the container and removing them from x.
    /// This function supports auto-commit. 
    /// \param position Position within the container where the elements 
    /// of x are inserted.
    /// \param x The other list container to splice from.
    /// \param i The position of element in x to move into this list.
    /// \sa http://www.cplusplus.com/reference/stl/list/splice/
    void splice (iterator position, self& x, iterator i)
    {
        verify_db_handles(x);
        try {
            begin_txn();
            assert(!(i == x.end()));
            insert(position, *i);
            x.erase(i);
            commit_txn();
        } catch (...) {
            abort_txn();
            throw;
        }
    }

    /// \brief Moves elements from list x into this list.
    ///
    /// Moves elements in range [first, last) of list x into this list 
    /// container at the 
    /// specified position, effectively inserting the specified 
    /// elements into the container and removing them from x.
    /// This function supports auto-commit. 
    /// \param position Position within the container where the elements 
    /// of x are inserted.
    /// \param x The other list container to splice from.
    /// \param first The range's closed boundary.
    /// \param last The range's open boundary.
    /// \sa http://www.cplusplus.com/reference/stl/list/splice/
    void splice (iterator position, self& x, iterator first, iterator last)
    {
        verify_db_handles(x);
        try {
            begin_txn();
            insert(position, first, last);
            x.erase(first, last);
            commit_txn();
        } catch (...) {
            abort_txn();
            throw;
        }
    }
    //@}
    ////////////////////////////////////////////////////////////////////

private:
    typedef db_vector_iterator<T, value_type_sub> iterator_type;
    typedef db_container base;
    friend class db_vector_iterator<T, value_type_sub>;
    friend class db_vector_base_iterator<T>;
    friend class db_reverse_iterator<iterator, const_iterator>;
    friend class db_reverse_iterator<const_iterator, iterator>;
    friend class DbstlListSpecialOps<T, value_type_sub>;

    // Replace current contents with those in 'x'.
    inline void copydb(const self&x)
    {
        const_iterator itr;

        // TODO: Make sure clear can succeed, it fails if there are
        // cursors open in other threads.
        clear(false);
        for (itr = x.begin(); itr != x.end(); ++itr)
            push_back(*itr);
    }

    static iterator end_itr_;

    template <Typename InputIterator>
    inline void push_range(InputIterator& first, InputIterator& last)
    {
        InputIterator itr;

        for (itr = first; itr != last; ++itr)
            push_back(*itr);
    }

    inline void push_range(const_iterator& first, const_iterator& last)
    {
        const_iterator itr;

        for (itr = first; itr != last; ++itr)
            push_back(*itr);

    }

    // Move pos to last, pos must initially be the end() iterator.
    inline void end_to_last(const const_iterator& pos) const
    {
        if (pos != end_itr_)
            return;
        pos.pcsr_.set_cursor(new TRandDbCursor());
        open_itr(pos);
        pos.last();
    }

    // This function generate appropriate flags for cursor insert calls.
    void make_insert_args(iterator& pos, u_int32_t& flag, bool &isempty)
    {
        isempty = false;                    
        if (pos.itr_status_ == INVALID_ITERATOR_POSITION) { 
            ((self*)pos.owner_)->end_to_last(pos);      
            /* Empty db, iterator at "begin()". */      
            if (((self*)pos.owner_)->empty()) {     
                flag = DB_KEYLAST; /* Empty */      
                isempty = true;             
            } else  
                /* Move pos to last element. */
                flag = DB_AFTER;
        } else                          
            flag = DB_BEFORE; 
    }

    // Open iterator and move it to point the 1st key/data pair.
    //
    void open_itr(const const_iterator&itr, bool readonly = false) const
    {
        u_int32_t oflags = 0;
        int ret;
        DbEnv *penv = this->get_db_handle()->get_env();

        itr.owner_ = (db_container*)this;
        if (!readonly && penv != NULL) {
            BDBOP((penv->get_open_flags(&oflags)), ret);
            // Open a writeable cursor when in CDS mode and not
            // requesting a read only iterator.
            if ((oflags & DB_INIT_CDB) != 0)
                ((self *)this)->set_cursor_open_flags(
                    this->get_cursor_open_flags() |
                    DB_WRITECURSOR);
        }
        if (!itr.pcsr_)
            itr.pcsr_.set_cursor(new TRandDbCursor(
                itr.bulk_retrieval_, 
                itr.rmw_csr_, itr.directdb_get_));
        itr.itr_status_ = itr.pcsr_->open((db_container*)this,
            this->get_cursor_open_flags());
    }

    void open_itr(const reverse_iterator &itr, bool readonly = false) const
    {
        u_int32_t oflags = 0;
        int ret;
        DbEnv *penv = this->get_db_handle()->get_env();

        itr.owner_ = (db_container*)this;
        if (!readonly && penv != NULL) {
            BDBOP((penv->get_open_flags(&oflags)) , ret);
            // Open a writeable cursor when in CDS mode and not
            // requesting a read only iterator.
            if ((oflags & DB_INIT_CDB) != 0)
                ((self *)this)->set_cursor_open_flags(
                    this->get_cursor_open_flags() |
                    DB_WRITECURSOR);
        }
        if (!itr.pcsr_)
            itr.pcsr_.set_cursor(new TRandDbCursor(
                itr.bulk_retrieval_,
                itr.rmw_csr_, itr.directdb_get_));
        itr.itr_status_ = itr.pcsr_->open((db_container*)this,
            this->get_cursor_open_flags());
        itr.update_cur_pair();
    }

    inline void init_itr(const_iterator &itr) const
    {
        itr.owner_ = (db_container*)this;
    }

    // Certain flags and parameters need to be set to the database and 
    // environment handle for them to back-up a certain type of container.
    // This function verifies that db and env handles are well configured 
    // to be suitable for this type of containers.
    virtual const char* verify_config(Db*db, DbEnv*env) const
    {
        u_int32_t oflags, sflags, oflags2;
        const char *errmsg = NULL;
        int ret;
        DBTYPE dbtype;

        errmsg = db_container::verify_config(db, env);
        if (errmsg)
            return errmsg;

        oflags = sflags = oflags2 = 0;
        BDBOP((db->get_type(&dbtype)) || (db->get_open_flags(&oflags))
             || (db->get_flags(&sflags)) ||
             (env->get_open_flags(&oflags2)), ret);

        if (dbtype != DB_RECNO)
            errmsg = "Must use DB_RECNO type of database.";
        // DB_THREAD is not always required, only required if the db 
        // handle is shared among multiple threads, which is not a 
        // case we can detect here.

        return errmsg;
    }


}; // db_vector

template <Typename T, Typename value_type_sub>
typename db_vector<T, value_type_sub>::iterator
    db_vector<T, value_type_sub>::end_itr_;

// Partial spececialization version of std::swap for db_vector.
template <Typename T, Typename value_type_sub>
void swap(db_vector<T, value_type_sub>&v1, db_vector<T, value_type_sub>&v2)
{
    v1.swap(v2);
}



template <typename T, typename T2>
class _exported DbstlListSpecialOps
{
    typedef db_vector<T, T2> partner;
    typedef typename partner::iterator iterator;
    partner *that;
public:
    DbstlListSpecialOps(partner *that1)
    {
        that = that1;
    }

    template <class BinaryPredicate>
    void unique(BinaryPredicate binary_pred)
    {
        T t, t2;

        try {
            that->begin_txn();
            iterator i = that->begin();
            t2 = *i;
            ++i;
            for (; i != that->end_itr_; ++i) {
                if (binary_pred((t = *i), t2))
                    that->erase(i);
                else
                    t2 = t;
            }
            that->commit_txn();
        } catch (...) {
            that->abort_txn();
            throw;
        }
    }

    void unique()
    {
        T t, t2;

        try {
            that->begin_txn();
            iterator i = that->begin();
            t2 = *i;
            ++i;
            for (; i != that->end_itr_; ++i) {
                if ((t = *i) == t2)
                    that->erase(i);
                else
                    t2 = t;
            }
            that->commit_txn();
        } catch (...) {
            that->abort_txn();
            throw;
        }
    }

    /// This function supports auto-commit. 
    void merge(partner& x)
    {
        that->verify_db_handles(x);
        T b;
        iterator itr, itrx;

        try {
            that->begin_txn();
            for (itr = that->begin(), itrx = x.begin();
                itr != that->end_itr_ && itrx != x.end();) {
                if (*itr > (b = *itrx)) {
                    that->insert(itr, b);
                    ++itrx;
                } else
                    ++itr;
            }
            if (itr == that->end_itr_ && itrx != x.end())
                that->insert(itr, itrx, x.end());
            x.clear();
            that->commit_txn();
        } catch (...) {
            that->abort_txn();
            throw;
        }
    }

    /// This function supports auto-commit. 
    void sort()
    {
        try {
            that->begin_txn();
            std::sort(that->begin(), that->end());
            that->commit_txn();
        } catch (...) {
            that->abort_txn();
            throw;
        }
    }

    /// This function supports auto-commit. 
    template <class Compare>
    void sort(Compare comp)
    {
        try {
            that->begin_txn();
            std::sort(that->begin(), that->end(), comp);
            that->commit_txn();
        } catch (...) {
            that->abort_txn();
            throw;
        }
    }



};


template <typename T, typename T2>
class _exported DbstlListSpecialOps<T*, T2>
{
    typedef db_vector<T*, T2> partner;
    typedef typename partner::iterator iterator;
    typedef typename partner::const_iterator const_iterator;
    partner *that;
    DbstlElemTraits<T> *inst;
    typename DbstlElemTraits<T>::ElemSizeFunct sizef;
    typename DbstlElemTraits<T>::SequenceLenFunct seqlenf;
    typename DbstlElemTraits<T>::SequenceCopyFunct seqcopyf;
    typename DbstlElemTraits<T>::SequenceCompareFunct seqcmpf;

    void seq_assign(T *&dest, const T*src)
    {
        size_t sz = 0;
        size_t seql = seqlenf(src);

        
        if (sizef == NULL)
            sz = sizeof(T) * (seql + 1);
        else {
            
            for (size_t i = 0; i < seql; i++)
                sz += sizef(src[i]);
            // Add space for terminating object, like '\0' 
            // for char *string.
            T tmp;
            sz += sizef(tmp);
        }
        dest = (T *)DbstlReAlloc(dest, sz);
        memset(dest, 0, sz);
        seqcopyf(dest, src, seql);
    }

    template <typename T4>
    class CompareInt
    {
        typename DbstlElemTraits<T4>::SequenceCompareFunct cmpf;
    public:
        CompareInt(typename DbstlElemTraits<T4>::
            SequenceCompareFunct cmpf1)
        {
            cmpf = cmpf1;
        }

        bool operator()(const std::basic_string<T4, 
            DbstlElemTraits<T4> >
            &a, const std::basic_string<T, DbstlElemTraits<T4> > &b)
        {
            return cmpf(a.c_str(), b.c_str());
        }
        
    };

    template <typename T4, typename Compare>
    class CompareInt2
    {
    public:
        Compare comp_;

        CompareInt2(Compare comp)
        {
            comp_ = comp;
        }

        bool operator()(const std::basic_string<T4, 
            DbstlElemTraits<T4> >
            &s1, const std::basic_string<T4, DbstlElemTraits<T4> >& s2)
        {
            return comp_(s1.c_str(), s2.c_str());
        }

    };
public:
    DbstlListSpecialOps(partner *that1)
    {
        that = that1;

        // Though he following settings are called in ResourceManager 
        // singleton initialization, we still have to call them here 
        // because the global variable in the dll is not the same one
        // as the one in this application.
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

        inst = DbstlElemTraits<T>::instance();
        sizef = inst->get_size_function();
        seqlenf = inst->get_sequence_len_function();
        seqcopyf = inst->get_sequence_copy_function();
        seqcmpf = inst->get_sequence_compare_function();
    }

    template <class BinaryPredicate>
    void unique(BinaryPredicate binary_pred)
    {
        T *t2 = NULL;

        try {
            that->begin_txn();
            iterator i = that->begin();
            
            seq_assign(t2, *i);
            ++i;
            for (; i != that->end(); ++i) {
                if (binary_pred(*i, t2))
                    that->erase(i);
                else
                    seq_assign(t2, *i);
            }
            that->commit_txn();
            free(t2);
        } catch (...) {
            that->abort_txn();
            free(t2);
            throw;
        }
    }

    void unique()
    {
        T *t2 = NULL;

        try {
            that->begin_txn();
            iterator i = that->begin();
            seq_assign(t2, *i);
            ++i;
            for (; i != that->end(); ++i) {
                if (seqcmpf(*i, t2) == 0)
                    that->erase(i);
                else
                    seq_assign(t2, *i);
            }
            that->commit_txn();
            free(t2);
        } catch (...) {
            that->abort_txn();
            free(t2);
            throw;
        }
    }

    /// This function supports auto-commit. 
    void merge(partner& x)
    {
        that->verify_db_handles(x);
        iterator itr, itrx;

        try {
            that->begin_txn();
            for (itr = that->begin(), itrx = x.begin();
                itr != that->end() && itrx != x.end();) {
                if (seqcmpf(*itr, *itrx) > 0) {
                    that->insert(itr, *itrx);
                    ++itrx;
                } else
                    ++itr;
            }
            if (itr == that->end() && itrx != x.end())
                that->insert(itr, itrx, x.end());
            x.clear();
            that->commit_txn();
        } catch (...) {
            that->abort_txn();
            throw;
        }
    }

    void sort()
    {
        try {
            typedef std::basic_string<T, DbstlElemTraits<T> > 
                string_t;
            CompareInt<T> comp(DbstlElemTraits<T>::instance()->
                get_sequence_compare_function());
            std::list<string_t> tmplist(that->size());

            that->begin_txn();
            const_iterator itr;
            const partner&cthat = *that;
            typename std::list<string_t>::iterator itr1;

            for (itr = cthat.begin(BulkRetrievalOption::
                bulk_retrieval()), itr1 = tmplist.begin();
                itr1 != tmplist.end(); ++itr, ++itr1)
                *itr1 = string_t(*itr);

            tmplist.sort();
            that->clear(false);
            for (typename std::list<string_t>::iterator 
                it = tmplist.begin(); 
                it != tmplist.end(); ++it)
                that->push_back((T*)(it->c_str()));

            that->commit_txn();
        } catch (...) {
            that->abort_txn();
            throw;
        }
    }

    /// This function supports auto-commit. 
    template <class Compare>
    void sort(Compare comp)
    {
        try {
            typedef std::basic_string<T, DbstlElemTraits<T> > 
                string_t;
            CompareInt2<T, Compare> comp2(comp);

            std::list<string_t> tmplist(that->size());
            that->begin_txn();
            const_iterator itr;
            const partner&cthat = *that;
            typename std::list<string_t>::iterator itr1;

            for (itr = cthat.begin(BulkRetrievalOption::
                bulk_retrieval()), itr1 = tmplist.begin();
                itr1 != tmplist.end(); ++itr, ++itr1)
                *itr1 = string_t(*itr);

            tmplist.sort(comp2);
            that->clear(false);
            for (typename std::list<string_t>::iterator it = 
                tmplist.begin(); 
                it != tmplist.end(); ++it)
                that->push_back((T*)(it->c_str()));
            
            that->commit_txn();
        } catch (...) {
            that->abort_txn();
            throw;
        }
    }


};
//@} //dbstl_containers
END_NS
#endif //_DB_STL_DB_VECTOR_H

