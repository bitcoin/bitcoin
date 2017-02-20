/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_KDPAIR_H
#define _DB_STL_KDPAIR_H

#include <iostream>

#include "dbstl_common.h"
#include "dbstl_dbt.h"
#include "dbstl_exception.h"
#include "dbstl_base_iterator.h"
#include "dbstl_utility.h"

START_NS(dbstl)

using std::istream;
using std::ostream;
using std::basic_ostream;
using std::basic_istream;

template <Typename ddt>
class db_base_iterator;
template <Typename ddt>
class ElementHolder;

/** \ingroup dbstl_helper_classes
\defgroup Element_wrappers ElementRef and ElementHolder wrappers.
An ElementRef and ElementHolder object represents the reference to the 
data element referenced by an iterator. Each iterator 
object has an ElementRef or ElementHolder object that
stores the data element that the iterator points to.

The ElementHolder class is used to store primitive types into STL containers.

The ElementRef class is used to store other types into STL containers.

The ElementRef and ElementHolder classes have identical interfaces, and are
treated the same by other STL classes. Since the ElementRef class inherits
from the template data class, all methods have a _DB_STL_ prefix to avoid name
clashes.

An ElementRef or ElementHolder class corresponds to a single iterator instance.
An Element object is generally owned by an iterator object. The ownership
relationship is swapped in some specific situations, specifically for the 
dereference and array index operator.
@{
*/
/// ElementRef element wrapper for classes and structures.
/// \sa ElementHolder
template <Typename ddt>
class _exported ElementRef : public ddt
{
public:
    typedef ElementRef<ddt> self;
    typedef ddt base;
    typedef db_base_iterator<ddt> iterator_type;
    typedef ddt content_type; // Used by assoc classes.

private:
    // The iterator pointing the data element stored in this object.
    iterator_type *_DB_STL_itr_;

    // Whether or not to delete itr on destruction, by default it is
    // false because this object is supposed to live in the lifetime of
    // its _DB_STL_itr_ owner. But there is one exception: in
    // db_vector<>::operator[]/front/back and db_map<>::operator[]
    // functions, an ElementRef<T> object has to be
    // returned instead of its reference, thus the
    // returned ElementRef<> has to live longer than its _DB_STL_itr_,
    // thus we new an iterator, and call _DB_STL_SetDelItr() method, 
    // setting this member to true,
    // to tell this object that it should delete the
    // _DB_STL_itr_ iterator on destruction, and duplicate the _DB_STL_itr_
    // iterator on copy construction. Although
    // std::vector<> returns reference rather than value, this is not a
    // problem because the returned ElementRef<> will duplicate cursor and
    // still points to the same key/data pair.
    //
    mutable bool _DB_STL_delete_itr_;

public:
    ////////////////////////////////////////////////////////////////////
    //
    // Begin constructors and destructor.
    //
    /// \name Constructors and destructor.
    //@{
    /// Destructor.
    ~ElementRef() {
        if (_DB_STL_delete_itr_) {
            // Prevent recursive destruction.
            _DB_STL_delete_itr_ = false;
            _DB_STL_itr_->delete_me();
        }
    }

    /// Constructor.
    /// If the pitr parameter is NULL or the default value is used, the
    /// object created is a simple wrapper and not connected to a container.
    /// If a valid iterator parameter is passed in, the wrapped element will
    /// be associated with the matching key/data pair in the underlying
    /// container.
    /// \param pitr The iterator owning this object.
    explicit ElementRef(iterator_type *pitr = NULL)
    {
        _DB_STL_delete_itr_ = false;
        _DB_STL_itr_ = pitr;
    }

    /// Constructor.
    /// Initializes an ElementRef wrapper without an iterator. It can only
    /// be used to wrap a data element in memory, it can't access an
    /// unerlying database. 
    /// \param dt The base class object to initialize this object.
    ElementRef(const ddt &dt) : ddt(dt)
    {
        _DB_STL_delete_itr_ = false;
        _DB_STL_itr_ = NULL;
    }

    /// Copy constructor.
    /// The constructor takes a "deep" copy. The created object will be 
    /// identical to, but independent from the original object.
    /// \param other The object to clone from.
    ElementRef(const self &other) : ddt(other)
    {
        // Duplicate iterator if this object lives longer than
        // _DB_STL_itr_.
        _DB_STL_delete_itr_ = other._DB_STL_delete_itr_;
        if (_DB_STL_delete_itr_) {
            // Avoid recursive duplicate iterator calls.
            other._DB_STL_delete_itr_ = false;
            _DB_STL_itr_ = other._DB_STL_itr_->dup_itr();
            other._DB_STL_delete_itr_ = true;
        } else
            _DB_STL_itr_ = other._DB_STL_itr_;
    }
    //@}
    ////////////////////////////////////////////////////////////////////

    /// \name Assignment operators.
    /// The assignment operators are used to store right-values into the
    /// wrapped object, and also to store values into an underlying 
    /// container.
    //@{
    /// Assignment Operator.
    /// \param dt2 The data value to assign with.
    /// \return The object dt2's reference.
    inline const ddt& operator=(const ddt& dt2)
    {
        *((ddt*)this) = dt2;
        if (_DB_STL_itr_ != NULL) {
            if (!_DB_STL_itr_->is_set_iterator())
                _DB_STL_itr_->replace_current(dt2);
            else
                _DB_STL_itr_->replace_current_key(dt2);
        }
        return dt2;
    }

    /// Assignment Operator.
    /// \param me The object to assign with.
    /// \return The object me's reference.
    inline const self& operator=(const self& me)
    {
        ASSIGNMENT_PREDCOND(me)
        *((ddt*)this) = (ddt)me;
        if (_DB_STL_itr_ != NULL) {
            // This object is the reference of an valid data
            // element, so we must keep it that way, we don't
            // use me's iterator here.
            if (!_DB_STL_itr_->is_set_iterator())
                _DB_STL_itr_->replace_current(
                    me._DB_STL_value());
            else
                _DB_STL_itr_->replace_current_key(
                    me._DB_STL_value());
        } else if (me._DB_STL_delete_itr_) {
            // Duplicate an iterator from me.
            _DB_STL_delete_itr_ = true;
            me._DB_STL_delete_itr_ = false;
            _DB_STL_itr_ = me._DB_STL_itr_->dup_itr();
            me._DB_STL_delete_itr_ = true;
        }

        return me;
    }
    //@}

    /// Function to store the data element.
    /// The user needs to call this method after modifying the underlying
    /// object, so that the version stored in the container can be updated.
    /// 
    /// When db_base_iterator's directdb_get_ member is true, this function
    /// must be called after modifying the data member and before any
    /// subsequent container iterator dereference operations. If this step
    /// is not carried out any changes will be lost.
    ///
    /// If the data element is changed via ElementHolder<>::operator=(), 
    /// you don't need to call this function.
    inline void _DB_STL_StoreElement()
    {
        assert(_DB_STL_itr_ != NULL);
        _DB_STL_itr_->replace_current(*this);
    }

    /// Returns the data element this wrapper object wraps.
    inline const ddt& _DB_STL_value() const
    {
        return *((ddt*)this);
    }

    /// Returns the data element this wrapper object wraps.
    inline ddt& _DB_STL_value()
    {
        return *((ddt*)this);
    }

#ifndef DOXYGEN_CANNOT_SEE_THIS
    ////////////////////////////////////////////////////////////////////
    //
    // The following methods are not part of the official public API,
    // but can't be declared as protected, since it is not possible
    // to declare template-specialised classes as friends.
    //

    // Call this function to tell this object that it should delete the
    // _DB_STL_itr_ iterator because that iterator was allocated in
    // the heap. Methods like db_vector/db_map<>::operator[] should call
    // this function.
    inline void _DB_STL_SetDelItr()
    {
        _DB_STL_delete_itr_ = true;
    }

    // Only copy data into this object, do not store into database.
    inline void _DB_STL_CopyData(const self&dt2)
    {
        *((ddt*)this) = (ddt)dt2;
    }

    inline void _DB_STL_CopyData(const ddt&dt2)
    {
        *((ddt*)this) = dt2;
    }

    // Following functions are prefixed with _DB_STL_ to avoid
    // potential name clash with ddt members.
    //
    inline iterator_type* _DB_STL_GetIterator() const
    {
           return _DB_STL_itr_;
    }

    inline int _DB_STL_GetData(ddt& d) const
    {
        d = *((ddt*)this);
        return 0;
    }

    inline void _DB_STL_SetIterator(iterator_type*pitr)
    {
        _DB_STL_itr_ = pitr;
    }

    inline void _DB_STL_SetData(const ddt&d)
    {
        *(ddt*)this = d;
    }
    ////////////////////////////////////////////////////////////////////

}; // ElementRef<>
template<typename T>
class DbstlSeqWriter;
#else
};
#endif // DOXYGEN_CANNOT_SEE_THIS


// The ElementHolder class must have an identical public interface to 
// the ElementRef class.
/// A wrapper class for primitive types. It has identical usage and public 
/// interface to the ElementRef class.
/// \sa ElementRef. 
template <typename ptype>
class _exported ElementHolder
{
protected:
    typedef ElementHolder<ptype> self;


    inline void _DB_STL_put_new_value_to_db()
    {
        if (_DB_STL_itr_ != NULL) {
            if (!_DB_STL_itr_->is_set_iterator())
                _DB_STL_itr_->replace_current(dbstl_my_value_);
            else
                _DB_STL_itr_->replace_current_key(
                    dbstl_my_value_);
        }
    }

    inline void _DB_STL_put_new_value_to_db(const self &me)
    {
        if (_DB_STL_itr_ != NULL) {
            if (!_DB_STL_itr_->is_set_iterator())
                _DB_STL_itr_->replace_current(dbstl_my_value_);
            else
                _DB_STL_itr_->replace_current_key(
                    dbstl_my_value_);
        } else if (me._DB_STL_delete_itr_) {
            // Duplicate an iterator from me.
            _DB_STL_delete_itr_ = true;
            me._DB_STL_delete_itr_ = false;
            _DB_STL_itr_ = me._DB_STL_itr_->dup_itr();
            me._DB_STL_delete_itr_ = true;
        }
    }



public:
    typedef ptype type1;
    typedef db_base_iterator<ptype> iterator_type;
    typedef ptype content_type;

    ////////////////////////////////////////////////////////////////////
    //
    // Begin constructors and destructor.
    //
    /// \name Constructors and destructor.
    //@{
    /// Constructor.
    /// If the pitr parameter is NULL or the default value is used, the
    /// object created is a simple wrapper and not connected to a container.
    /// If a valid iterator parameter is passed in, the wrapped element will
    /// be associated with the matching key/data pair in the underlying
    /// container.
    /// \param pitr The iterator owning this object.
    explicit inline ElementHolder(iterator_type* pitr = NULL)
    {
        _DB_STL_delete_itr_ = false;
        _DB_STL_itr_ = pitr;
        dbstl_str_buf_ = NULL;
        dbstl_str_buf_len_ = 0;
        memset(&dbstl_my_value_, 0, sizeof(dbstl_my_value_));
    }

    /// Constructor.
    /// Initializes an ElementRef wrapper without an iterator. It can only
    /// be used to wrap a data element in memory, it can't access an
    /// unerlying database. 
    /// \param dt The base class object to initialize this object.
    inline ElementHolder(const ptype&dt)
    {
        dbstl_str_buf_ = NULL;
        dbstl_str_buf_len_ = 0;
        _DB_STL_delete_itr_ = false;
        _DB_STL_itr_ = NULL;
        _DB_STL_CopyData_int(dt);
    }

    /// Copy constructor.
    /// The constructor takes a "deep" copy. The created object will be 
    /// identical to, but independent from the original object.
    /// \param other The object to clone from.
    inline ElementHolder(const self& other)
    {
        dbstl_str_buf_ = NULL;
        dbstl_str_buf_len_ = 0;
        _DB_STL_delete_itr_ = other._DB_STL_delete_itr_;
        _DB_STL_CopyData(other);

        // Duplicate iterator if this object lives longer than
        // _DB_STL_itr_.
        _DB_STL_delete_itr_ = other._DB_STL_delete_itr_;
        if (_DB_STL_delete_itr_) {
            // Avoid recursive duplicate iterator calls.
            other._DB_STL_delete_itr_ = false;
            _DB_STL_itr_ = other._DB_STL_itr_->dup_itr();
            other._DB_STL_delete_itr_ = true;
        } else
            _DB_STL_itr_ = other._DB_STL_itr_;
    }

    /// Destructor.
    ~ElementHolder() {
        if (_DB_STL_delete_itr_) {
            _DB_STL_delete_itr_ = false;
            _DB_STL_itr_->delete_me();
        }
        if (dbstl_str_buf_) {
            free(dbstl_str_buf_);
            dbstl_str_buf_ = NULL;
        }
    }
    //@}
    ////////////////////////////////////////////////////////////////////

    /// This operator is a type converter. Where an automatic type 
    /// conversion is needed, this function is called to convert this 
    /// object into the primitive type it wraps.
    operator ptype () const
    {
        return dbstl_my_value_;
    }

    // ElementHolder is a wrapper for primitive types, and backed by db,
    // so we need to override all assignment operations to store updated
    // value to database. We don't need to implement other operators for
    // primitive types because we have a convert operator which can
    // automatically convert to primitive type and use its C++ built in
    // operator.
    //
    /** \name Math operators.
    ElementHolder class templates also have all C/C++ self mutating 
    operators for numeric primitive types, including:
    +=, -=, *=, /=, %=, <<=, >>=, &=, |=, ^=, ++, --
    These operators should not be used when ddt is a sequence pointer type
    like char* or wchar_t* or T*, otherwise the behavior is undefined. 
    These methods exist only to override default bahavior to store the 
    new updated value, otherwise, the type convert operator could have 
    done all the job.
    As you know, some of them are not applicable to float or double types 
    or ElementHolder wrapper types for float/double types. 
    These operators not only modifies the cached data element, but also 
    stores new value to database if it associates a database key/data pair.
    @{
    */
    template <Typename T2>
    const self& operator +=(const ElementHolder<T2> &p2)
    {
        dbstl_my_value_ += p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db(p2);
        return *this;
    }

    template <Typename T2>
    const self& operator -=(const ElementHolder<T2> &p2)
    {
        dbstl_my_value_ -= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db(p2);
        return *this;
    }
    template <Typename T2>
    const self& operator *=(const ElementHolder<T2> &p2)
    {
        dbstl_my_value_ *= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db(p2);
        return *this;
    }
    template <Typename T2>
    const self& operator /=(const ElementHolder<T2> &p2)
    {
        dbstl_my_value_ /= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db(p2);
        return *this;
    }
    template <Typename T2>
    const self& operator %=(const ElementHolder<T2> &p2)
    {
        dbstl_my_value_ %= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db(p2);
        return *this;
    }

    template <Typename T2>
    const self& operator &=(const ElementHolder<T2> &p2)
    {
        dbstl_my_value_ &= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db(p2);
        return *this;
    }
    template <Typename T2>
    const self& operator |=(const ElementHolder<T2> &p2)
    {
        dbstl_my_value_ |= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db(p2);
        return *this;
    }
    template <Typename T2>
    const self& operator ^=(const ElementHolder<T2> &p2)
    {
        dbstl_my_value_ ^= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db(p2);
        return *this;
    }

    const self& operator >>=(size_t n)
    {
        dbstl_my_value_ >>= n;
        _DB_STL_put_new_value_to_db();
        return *this;
    }

    const self& operator <<=(size_t n)
    {
        dbstl_my_value_ <<= n;
        _DB_STL_put_new_value_to_db();
        return *this;
    }

    const self& operator ^=(const self &p2)
    {
        dbstl_my_value_ ^= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db();
        return *this;
    }

    const self& operator &=(const self &p2)
    {
        dbstl_my_value_ &= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db();
        return *this;
    }

    const self& operator |=(const self &p2)
    {
        dbstl_my_value_ |= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db();
        return *this;
    }

    const self& operator %=(const self &p2)
    {
        dbstl_my_value_ %= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db();
        return *this;
    }

    const self& operator +=(const self &p2)
    {
        dbstl_my_value_ += p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db();
        return *this;
    }
    const self& operator -=(const self &p2)
    {
        dbstl_my_value_ -= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db();
        return *this;
    }
    const self& operator /=(const self &p2)
    {
        dbstl_my_value_ /= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db();
        return *this;
    }
    const self& operator *=(const self &p2)
    {
        dbstl_my_value_ *= p2.dbstl_my_value_;
        _DB_STL_put_new_value_to_db();
        return *this;
    }

    self& operator++()
    {
        dbstl_my_value_++;
        _DB_STL_put_new_value_to_db();
        return *this;
    }

    self operator++(int)
    {
        self obj(*this);
        dbstl_my_value_++;
        _DB_STL_put_new_value_to_db();
        return obj;
    }

    self& operator--()
    {
        dbstl_my_value_--;
        _DB_STL_put_new_value_to_db();
        return *this;
    }

    self operator--(int)
    {
        self obj(*this);
        dbstl_my_value_--;
        _DB_STL_put_new_value_to_db();
        return obj;
    }

    inline const ptype& operator=(const ptype& dt2)
    {
        _DB_STL_CopyData_int(dt2);
        _DB_STL_put_new_value_to_db();
        return dt2;
    }

    inline const self& operator=(const self& dt2)
    {
        ASSIGNMENT_PREDCOND(dt2)
        _DB_STL_CopyData(dt2);
        _DB_STL_put_new_value_to_db(dt2);
        return dt2;
    }
    //@}
    
    /// Returns the data element this wrapper object wraps;
    inline const ptype& _DB_STL_value() const
    {
        return dbstl_my_value_;
    }

    /// Returns the data element this wrapper object wraps;
    inline ptype&_DB_STL_value()
    {
        return dbstl_my_value_;
    }
    
    /// Function to store the data element.
    /// The user needs to call this method after modifying the underlying
    /// object, so that the version stored in the container can be updated.
    /// 
    /// When db_base_iterator's directdb_get_ member is true, this function
    /// must be called after modifying the data member and before any
    /// subsequent container iterator dereference operations. If this step
    /// is not carried out any changes will be lost.
    ///
    /// If the data element is changed via ElementHolder<>::operator=(), 
    /// you don't need to call this function.
    inline void _DB_STL_StoreElement()
    {
        assert(_DB_STL_itr_ != NULL);
        _DB_STL_itr_->replace_current(dbstl_my_value_);
    }

#ifndef DOXYGEN_CANNOT_SEE_THIS
    ////////////////////////////////////////////////////////////////////
    //
    // The following methods are not part of the official public API,
    // but can't be declared as protected, since it is not possible
    // to declare template-specialised classes as friends.
    //
    inline void _DB_STL_CopyData(const self&dt2)
    {
        _DB_STL_CopyData_int(dt2.dbstl_my_value_);
    }

    template<Typename T>
    inline void _DB_STL_CopyData_int(const T&src)
    {
        dbstl_my_value_ = src;
    }

    // Try to catch all types of pointers.
    template<Typename T>
    inline void _DB_STL_CopyData_int(T* const &src)
    {
        DbstlSeqWriter<T>::copy_to_holder((ElementHolder<T *> *)this, 
            (T *)src);  
    }

    template<Typename T>
    inline void _DB_STL_CopyData_int(const T* const &src)
    {
        DbstlSeqWriter<T>::copy_to_holder((ElementHolder<T *> *)this, 
            (T *)src);  
    }

    template<Typename T>
    inline void _DB_STL_CopyData_int(T* &src)
    {
        DbstlSeqWriter<T>::copy_to_holder((ElementHolder<T *> *)this, 
            (T *)src);  
    }

    template<Typename T>
    inline void _DB_STL_CopyData_int(const T*&src)
    {
        DbstlSeqWriter<T>::copy_to_holder((ElementHolder<T *> *)this, 
            (T *)src);  
    }

    inline iterator_type* _DB_STL_GetIterator() const
    {
           return _DB_STL_itr_;
    }

    inline int _DB_STL_GetData(ptype& d) const
    {
        d = dbstl_my_value_;
        return 0;
    }

    inline void _DB_STL_SetIterator(iterator_type*pitr)
    {
        _DB_STL_itr_ = pitr;
    }

    inline void _DB_STL_SetData(const ptype&d)
    {
        _DB_STL_CopyData_int(d);
    }

    inline void _DB_STL_SetDelItr()
    {
        _DB_STL_delete_itr_ = true;
    }

    // The two member has to be public for DbstlSeqWriter to access, 
    // but can't be accessed by user. 
    size_t dbstl_str_buf_len_;
    void *dbstl_str_buf_; // Stores a sequence, used when ptype is T*

    iterator_type *_DB_STL_itr_;
    ptype dbstl_my_value_;
    mutable bool _DB_STL_delete_itr_;
};
#else
};
#endif // DOXYGEN_CANNOT_SEE_THIS
//@} // Element_wrappers
//@} //dbstl_helper_classes

// These operators help reading from and writing to iostreams, if the wrapped
// data type has iostream operators.
template<Typename _CharT, Typename _Traits, Typename ddt>
basic_istream<_CharT,_Traits>&
operator>>( basic_istream<_CharT,_Traits> & in, ElementRef<ddt>&p)
{
    in>>(ddt)p;
    return in;
}

template<Typename _CharT, Typename _Traits, Typename ddt>
basic_ostream<_CharT,_Traits>&
operator<<( basic_ostream<_CharT,_Traits> & out,
    const ElementRef<ddt>&p)
{
    out<<(ddt)p;
    return out;
}

template<Typename _CharT, Typename _Traits, Typename ddt>
basic_istream<_CharT,_Traits>&
operator>>( basic_istream<_CharT,_Traits> & in, ElementHolder<ddt>&p)
{
    in>>p._DB_STL_value();
    return in;
}

template<Typename _CharT, Typename _Traits, Typename ddt>
basic_ostream<_CharT,_Traits>&
operator<<( basic_ostream<_CharT,_Traits> & out,
    const ElementHolder<ddt>&p)
{
    out<<p._DB_STL_value();
    return out;
}

template<typename T>
class _exported DbstlSeqWriter
{
public:
    typedef ElementHolder<T *> HolderType;
    static void copy_to_holder(HolderType *holder, T *src)
    {
        size_t i, slen, sql;

        if (src == NULL) {
            free(holder->dbstl_str_buf_);
            holder->dbstl_str_buf_ = NULL;
            holder->dbstl_my_value_ = NULL;
            return;
        }
        if (holder->dbstl_str_buf_len_ > DBSTL_MAX_DATA_BUF_LEN) {
            free(holder->dbstl_str_buf_);
            holder->dbstl_str_buf_ = NULL;
        }

        typedef DbstlElemTraits<T> DM;
        typename DM::SequenceCopyFunct seqcpy =
            DM::instance()->get_sequence_copy_function();
        typename DM::SequenceLenFunct seqlen =
            DM::instance()->get_sequence_len_function();
        typename DM::ElemSizeFunct elemszf = 
            DM::instance()->get_size_function();

        assert(seqcpy != NULL && seqlen != NULL);
        sql = seqlen(src);
        if (elemszf == NULL)
            slen = sizeof(T) * (sql + 1);
        else
            // We don't add the terminating object if it has one.
            // So the registered functions should take care of it.
            for (slen = 0, i = 0; i < sql; i++)
                slen += elemszf(src[i]);

        if (slen > holder->dbstl_str_buf_len_)
            holder->dbstl_str_buf_ = DbstlReAlloc(
                holder->dbstl_str_buf_, 
                holder->dbstl_str_buf_len_ = slen);

        seqcpy((T*)holder->dbstl_str_buf_, src, sql);
        holder->dbstl_my_value_ = (T*)holder->dbstl_str_buf_;
    }
};

template<>
class _exported DbstlSeqWriter<char>
{
public:
    typedef ElementHolder<char *> HolderType;
    static void copy_to_holder(HolderType *holder, char *src)
    {
        size_t slen;

        if (src == NULL) {
            free(holder->dbstl_str_buf_);
            holder->dbstl_str_buf_ = NULL;
            holder->dbstl_my_value_ = NULL;
            return;
        }
        if (holder->dbstl_str_buf_len_ > DBSTL_MAX_DATA_BUF_LEN) {
            free(holder->dbstl_str_buf_);
            holder->dbstl_str_buf_ = NULL;
        }

        slen = sizeof(char) * (strlen(src) + 1);
        if (slen > holder->dbstl_str_buf_len_)
            holder->dbstl_str_buf_ = DbstlReAlloc(
                holder->dbstl_str_buf_, 
                (u_int32_t)(holder->dbstl_str_buf_len_ = slen));

        strcpy((char*)holder->dbstl_str_buf_, src);
        holder->dbstl_my_value_ = (char*)holder->dbstl_str_buf_;

    }
};

template<>
class _exported DbstlSeqWriter<wchar_t>
{
public:
    typedef ElementHolder<wchar_t *> HolderType;
    static void copy_to_holder(HolderType *holder, wchar_t *src)
    {
        size_t slen;

        if (src == NULL) {
            free(holder->dbstl_str_buf_);
            holder->dbstl_str_buf_ = NULL;
            holder->dbstl_my_value_ = NULL;
            return;
        }
        if (holder->dbstl_str_buf_len_ > DBSTL_MAX_DATA_BUF_LEN) {
            free(holder->dbstl_str_buf_);
            holder->dbstl_str_buf_ = NULL;
        }

        slen = sizeof(wchar_t) * (wcslen(src) + 1);
        if (slen > holder->dbstl_str_buf_len_)
            holder->dbstl_str_buf_ = DbstlReAlloc(
                holder->dbstl_str_buf_, 
                holder->dbstl_str_buf_len_ = slen);

        wcscpy((wchar_t*)holder->dbstl_str_buf_, src);
        holder->dbstl_my_value_ = (wchar_t*)holder->dbstl_str_buf_;
    }
};
END_NS

#endif// !_DB_STL_KDPAIR_H
