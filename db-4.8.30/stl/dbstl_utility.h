/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_UTILITY_H__
#define _DB_STL_UTILITY_H__

#include "dbstl_inner_utility.h"

START_NS(dbstl)

// This class allows users to configure dynamically how a specific type of 
// object is copied, stored, restored, and how to measure the type's
// instance size.
/** \defgroup dbstl_helper_classes dbstl helper classes
Classes of this module help to achieve various features of dbstl.
*/
/** \ingroup dbstl_helper_classes
@{
This class is used to register callbacks to manipulate an object of a 
complex type. These callbacks are used by dbstl at runtime  to 
manipulate the object.

A complex type is a type whose members are not located in a contiguous 
chunk of memory. For example, the following class A is a complex type 
because for any  instance a of class A, a.b_ points to another object 
of type B, and dbstl treats the  object that a.b_ points to as part of 
the data of the instance a. Hence, if the user needs to store a.b_ into 
a dbstl container, the user needs to register an appropriate callback to 
de-reference and store the object referenced by a.b. Similarly, the 
user also needs to register callbacks to marshall an array as well as 
to count the  number of elements in such an array.

class A { int m; B *p_; };
class B { int n; };

The user also needs to register callbacks for  
i). returning an object¡¯s size in bytes; 
ii). Marshalling and unmarshalling an object;
iii). Copying a complex object and  and assigning an object to another 
object of the same type; 
iv). Element comparison. 
v). Compare two sequences of any type of objects; Measuring the length 
of an object sequence and copy an object sequence.
 
Several elements  located in a contiguous chunk of memory form a sequence. 
An element of a sequence may be a simple object located at a contigous 
memory chunk, or a complex object, i.e. some of its members may contain 
references (pointers) to another region of memory. It is not necessary 
to store a special object to denote the end of the sequence.  The callback 
to traverse the constituent elements of the sequence needs to able to 
determine the end of the sequence.

Marshalling means packing the object's data members into a contiguous 
chunk of memory; unmarshalling is the opposite of marshalling. In other 
words, when you unmarshall an object, its data members are populated 
with values from a previously marshalled version of the object.

The callbacks need not be set to every type explicitly.  . dbstl will 
check if a needed callback function of this type is provided. 
If one is available, dbstl will use the registered callback.  If the 
appropriate callback is not provided, dbstl will use reasonable defaults 
to do the job.

For returning the size of an object,  the default behavior is to use the 
sizeof() operator;  For marshalling and unmarshalling, dbstl uses memcpy, 
so the default behavior is  sufficient for simple types whose data reside 
in a contiguous  chunk of memory; Dbstl uses  uses >, == and < for 
comparison operations; For char* and wchar_t * strings, dbstl already 
provides the appropriate callbacks, so you do not need to register them. 
In general, if the default behavior is adequate, you don't need to 
register the corresponding callback.

If you have registered proper callbacks, the DbstlElemTraits<T> can also be
used as the char_traits<T> class for std::basic_string<T, char_traits<T> >,
and you can enable your class T to form a basic_string<T, DbstlElemTraits<T>>,
and use basic_string's functionality and the algorithms to manipulate it.
*/
template <typename T>
class _exported DbstlElemTraits : public DbstlGlobalInnerObject
{
public:
    /// \name Callback_typedefs Function callback type definitions.
    /// Following are the callback function types, there is one function
    /// pointer data member for each of the type, and a pair of set/get
    /// functions for each function callback data member, and this is 
    /// the structure of this class.
    //@{
    /// Assign object src to object dest. Most often assignment callback 
    /// is not needed - the class copy constructor is sufficient.
    /// This description talks about the function of this type, rather 
    /// than the type itself, this is true to all the types in the group.
    typedef void (*ElemAssignFunct)(T& dest, const T&src);
    /// Read data from the chunk of memory pointed by srcdata, and assign
    /// to the object dest. This is also called unmashall.
    typedef void (*ElemRstoreFunct)(T& dest, const void *srcdata);
    /// Return object elem's size in bytes.
    typedef u_int32_t (*ElemSizeFunct)(const T& elem);
    /// Copy object elem's data to be persisted into a memory chunk 
    /// referenced by dest. The dest memory is large enough. 
    /// elem may not reside on a 
    /// consecutive chunk of memory. This is also called marshal.
    typedef void (*ElemCopyFunct)(void *dest, const T&elem);
    typedef int (*ElemCompareFunct)(const T&a, const T&b);
    /// Compares first num number of elements of sequence a and b, returns 
    /// negative/0/positive value if a is less/equal/greater than b.
    typedef int (*SequenceNCompareFunct)(const T *a, const T *b, 
        size_t num);
    /// Compares sequence a and b, returns negative/0/positive
    /// value if a is less/equal/greater than b.
    typedef int (*SequenceCompareFunct)(const T *a, const T *b);
    /// Return the sequence's number of elements.
    typedef u_int32_t (*SequenceLenFunct)(const T *seqs);

    /// Copy the sequence seqs's first num elements to seqd.
    /// The sequence seqs of type T objects may not reside in a continuous
    /// chunk of memory, but the seqd sequence points to a consecutive 
    /// chunk of memory large enough to hold all objects from seqs.
    /// And if T is a complex type, you should register your ElemCopyFunct
    /// object marshalling manipulator
    typedef void (*SequenceCopyFunct)(T *seqd, const T *seqs, size_t num);
    //@}
    
    typedef T char_type;
    typedef long int_type;

    /// \name Interface compatible with std::string's char_traits.
    /// Following are char_traits funcitons, which make this class 
    /// char_traits compatiable, so that it can be used in 
    /// std::basic_string template, and be manipulated by the c++ stl 
    /// algorithms.
    //@{
    /// Assignone object to another.
    static void  assign(T& left, const T& right)
    {
        if (inst_->assign_)
            inst_->assign_(left, right);
        else
            left = right;
    }

    /// Check for equality of two objects.
    static bool  eq(const T& left, const T& right)
    {
        if (inst_->elemcmp_)
            return inst_->elemcmp_(left, right) == 0;
        else
            return left == right;
    }

    /// \brief Less than comparison.
    ///
    /// Returns if object left is less than object right.
    static bool  lt(const T& left, const T& right)
    {
        if (inst_->elemcmp_)
            return inst_->elemcmp_(left, right) < 0;
        else 
            return left < right;
    }

    /// \brief Sequence comparison.
    ///
    /// Compares the first cnt number of elements in the two 
    /// sequences seq1 and seq2, returns negative/0/positive if seq1 is
    /// less/equal/greater than seq2.
    static int  compare(const T *seq1, const T *seq2, size_t cnt)
    {
        if (inst_->seqncmp_)
            return inst_->seqncmp_(seq1, seq2, cnt);
        else {
            for (; 0 < cnt; --cnt, ++seq1, ++seq2)
                if (!eq(*seq1, *seq2))
                    return (lt(*seq1, *seq2) ? -1 : +1);
        }
        return (0);
    }

    /// Returns the number of elements in sequence seq1. Note that seq1
    /// may or may not end with a trailing '\0', it is completely user's
    /// responsibility for this decision, though seq[0], seq[1],...
    /// seq[length - 1] are all sequence seq's memory.
    static size_t  length(const T *seq)
    {
        assert(inst_->seqlen_ != NULL);
        return (size_t)inst_->seqlen_(seq);
    }

    /// Copy first cnt number of elements from seq2 to seq1.
    static T * copy(T *seq1, const T *seq2, size_t cnt)
    {
        if (inst_->seqcpy_)
            inst_->seqcpy_(seq1, seq2, cnt);
        else {
            T *pnext = seq1;
            for (; 0 < cnt; --cnt, ++pnext, ++seq2)
                assign(*pnext, *seq2);
        }
        return (seq1);
    }

    /// Find within the first cnt elements of sequence seq the position 
    /// of element equal to elem.
    static const T * find(const T *seq, size_t cnt, const T& elem)
    {
        for (; 0 < cnt; --cnt, ++seq)
            if (eq(*seq, elem))
                return (seq);
        return (0);
    }

    /// \brief Sequence movement.
    ///
    /// Move first cnt number of elements from seq2 to seq1, seq1 and seq2
    /// may or may not overlap.
    static T * move(T *seq1, const T *seq2, size_t cnt)
    {
        T *pnext = seq1;
        if (seq2 < pnext && pnext < seq2 + cnt)
            for (pnext += cnt, seq2 += cnt; 0 < cnt; --cnt)
                assign(*--pnext, *--seq2);
        else
            for (; 0 < cnt; --cnt, ++pnext, ++seq2)
                assign(*pnext, *seq2);
        return (seq1);
    }

    /// Assign first cnt number of elements of sequence seq with the 
    /// value of elem.
    static T * assign(T *seq, size_t cnt, T elem)
    {
        T *pnext = seq;
        for (; 0 < cnt; --cnt, ++pnext)
            assign(*pnext, elem);
        return (seq);
    }

    static T  to_char_type(const int_type& meta_elem)
    {   // convert metacharacter to character
        return ((T)meta_elem);
    }

    static int_type  to_int_type(const T& elem)
    {   // convert character to metacharacter
        return ((int_type)elem);
    }

    static bool  eq_int_type(const int_type& left,
        const int_type& right)
    {   // test for metacharacter equality
        return (left == right);
    }

    static int_type  eof()
    {   // return end-of-file metacharacter
        return ((int_type)EOF);
    }

    static int_type  not_eof(const int_type& meta_elem)
    {   // return anything but EOF
        return (meta_elem != eof() ? (int_type)meta_elem : 
            (int_type)!eof());
    }

    //@}
    
    /// Factory method to create a singeleton instance of this class. 
    /// The created object will be deleted by dbstl upon process exit.
    inline static DbstlElemTraits *instance()
    {
        if (!inst_) {
            inst_ = new DbstlElemTraits();
            register_global_object(inst_);
        }
        return inst_;
    }

    /// \name Set/get functions for callback function pointers.
    /// These are the setters and getters for each callback function 
    /// pointers.
    //@{
    inline void set_restore_function(ElemRstoreFunct f)
    {
        restore_ = f;
    }

    inline ElemRstoreFunct get_restore_function() { return restore_; }

    inline void set_assign_function(ElemAssignFunct f)
    {
        assign_ = f;
    }

    inline ElemAssignFunct get_assign_function() { return assign_; }

    inline ElemSizeFunct get_size_function() { return size_; }

    inline void set_size_function(ElemSizeFunct f) { size_ = f; }

    inline ElemCopyFunct get_copy_function() { return copy_; }

    inline void set_copy_function(ElemCopyFunct f) { copy_ = f; }

    inline void set_sequence_len_function(SequenceLenFunct f)
    {
        seqlen_ = f;
    }

    inline SequenceLenFunct get_sequence_len_function() { return seqlen_; }

    inline SequenceCopyFunct get_sequence_copy_function()
    {
        return seqcpy_;
    }

    inline void set_sequence_copy_function(SequenceCopyFunct f)
    {
        seqcpy_ = f;
    }

    inline void set_compare_function(ElemCompareFunct f)
    {
        elemcmp_ = f;
    }

    inline ElemCompareFunct get_compare_function()
    {
        return elemcmp_;
    }

    inline void set_sequence_compare_function(SequenceCompareFunct f)
    {
        seqcmp_ = f;
    }

    inline SequenceCompareFunct get_sequence_compare_function()
    {
        return seqcmp_;
    }

    inline void set_sequence_n_compare_function(SequenceNCompareFunct f)
    {
        seqncmp_ = f;
    }

    inline SequenceNCompareFunct get_sequence_n_compare_function()
    {
        return seqncmp_;
    }
    //@}

    ~DbstlElemTraits(){}
protected:
    inline DbstlElemTraits()
    {
        assign_ = NULL;
        restore_ = NULL;
        size_ = NULL;
        copy_ = NULL;
        seqlen_ = NULL;
        seqcpy_ = NULL;
        seqcmp_ = NULL;
        seqncmp_ = NULL;
        elemcmp_ = NULL;
    }

    static DbstlElemTraits *inst_;

    // Data members to hold registered function pointers.
    ElemAssignFunct assign_;
    ElemRstoreFunct restore_;
    ElemSizeFunct size_;
    ElemCopyFunct copy_;
    ElemCompareFunct elemcmp_;
    SequenceCompareFunct seqcmp_;
    SequenceNCompareFunct seqncmp_;
    SequenceLenFunct seqlen_;
    SequenceCopyFunct seqcpy_;
}; //DbstlElemTraits
//@} // dbstl_helper_classes

template<typename T>
DbstlElemTraits<T> *DbstlElemTraits<T>::inst_ = NULL;

/** 
\ingroup dbstl_helper_classes
@{
You can persist all bytes in a chunk of contiguous memory by constructing 
an DbstlDbt object A(use malloc to allocate the required number of bytes for 
A.data and copy the bytes to be stored into A.data, set other 
fields as necessary) and store A into a container, e.g. db_vector<DbstlDbt>, 
this stores the bytes rather than the object A into the underlying database.
The DbstlDbt class can help you avoid memory leaks, 
so it is strongly recommended that you use DbstlDbt rather than Dbt class.

DbstlDbt derives from Dbt class, and it does an deep copy on copy construction 
and assignment --by calling malloc to allocate its own memory and then 
copying the bytes to it; Conversely the destructor will free the memory on 
destruction if the data pointer is non-NULL. The destructor assumes the 
memory is allocated via malloc, hence why you are required to call 
malloc to allocate memory in order to use DbstlDbt. 

DbstlDbt simply inherits all methods from Dbt with no extra 
new methods except the constructors/destructor and assignment operator, so it 
is easy to use. 

In practice you rarely need to use DbstlDbt 
or Dbt because dbstl enables you to store any complex 
objects or primitive data. Only when you need to store raw bytes, 
e.g. a bitmap, do you need to use DbstlDbt. 

Hence, DbstlDbt is the right class to 
use to store any object into Berkeley DB via dbstl without memory leaks.

Don't free the memory referenced by DbstlDbt objects, it will be freed when the
DbstlDbt object is destructed.

Please refer to the two examples using DbstlDbt in 
TestAssoc::test_arbitrary_object_storage and
TestAssoc::test_char_star_string_storage member functions, 
which illustrate how to correctly use DbstlDbt in order to store raw bytes.

This class handles the task of allocating and de-allocating memory internally. 
Although it can be used to store data which cannot be handled by the 
DbstlElemTraits 
class, in practice, it is usually more convenient to register callbacks in the 
DbstlElemTraits class for the type you are storing/retrieving using dbstl.
*/
class DbstlDbt : public Dbt
{
    inline void deep_copy(const DbstlDbt &d)
    {
        u_int32_t len;
        if (d.get_data() != NULL && d.get_size() > 0) {
            if (d.get_flags() & DB_DBT_USERMEM)
                len = d.get_ulen();
            else
                len = d.get_size();

            set_data(DbstlMalloc(len));
            memcpy(get_data(), d.get_data(), len);
        }
    }

public:
    /// Construct an object with an existing chunk of memory of size1 
    /// bytes, refered by data1, 
    DbstlDbt(void *data1, u_int32_t size1) : Dbt(data1, size1){}
    DbstlDbt() : Dbt(){}
    /// The memory will be free'ed by the destructor.
    ~DbstlDbt() 
    { 
        free_mem();
    }

    /// This copy constructor does a deep copy.
    DbstlDbt(const DbstlDbt &d) : Dbt(d) 
    {
        deep_copy(d);
    }

    /// The memory will be reallocated if neccessary. 
    inline const DbstlDbt &operator = (const DbstlDbt &d)
    {
        ASSIGNMENT_PREDCOND(d)

        if (d.get_data() != NULL && d.get_size() > 0) {
            free_mem();
            memcpy(this, &d, sizeof(d));
        }

        deep_copy(d);
        return d;
    }

protected:
    /// Users don't need to call this function.
    inline void free_mem()
    {
        if (get_data()) {
            free(get_data());
            memset(this, 0, sizeof(*this));
        }
    }

};
//@} // dbstl_help_classes
END_NS

#endif // ! _DB_STL_UTILITY_H__


