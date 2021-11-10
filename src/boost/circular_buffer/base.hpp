// Implementation of the base circular buffer.

// Copyright (c) 2003-2008 Jan Gaspar
// Copyright (c) 2013 Paul A. Bristow  // Doxygen comments changed.
// Copyright (c) 2013 Antony Polukhin  // Move semantics implementation.

// Copyright 2014,2018 Glen Joseph Fernandes
// (glenjofe@gmail.com)

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_CIRCULAR_BUFFER_BASE_HPP)
#define BOOST_CIRCULAR_BUFFER_BASE_HPP

#if defined(_MSC_VER)
    #pragma once
#endif

#include <boost/config.hpp>
#include <boost/concept_check.hpp>
#include <boost/limits.hpp>
#include <boost/core/allocator_access.hpp>
#include <boost/core/empty_value.hpp>
#include <boost/type_traits/is_stateless.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#include <boost/type_traits/is_nothrow_move_assignable.hpp>
#include <boost/type_traits/is_copy_constructible.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/move/adl_move_swap.hpp>
#include <boost/move/move.hpp>
#include <algorithm>
#include <iterator>
#include <utility>
#include <deque>
#include <stdexcept>

#if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3205))
    #include <stddef.h>
#endif

namespace boost {

/*!
    \class circular_buffer
    \brief Circular buffer - a STL compliant container.
    \tparam T The type of the elements stored in the <code>circular_buffer</code>.
    \par Type Requirements T
         The <code>T</code> has to be <a href="https://www.boost.org/sgi/stl/Assignable.html">
         SGIAssignable</a> (SGI STL defined combination of <a href="../../../utility/Assignable.html">
         Assignable</a> and <a href="../../../utility/CopyConstructible.html">CopyConstructible</a>).
         Moreover <code>T</code> has to be <a href="https://www.boost.org/sgi/stl/DefaultConstructible.html">
         DefaultConstructible</a> if supplied as a default parameter when invoking some of the
         <code>circular_buffer</code>'s methods e.g.
         <code>insert(iterator pos, const value_type& item = %value_type())</code>. And
         <a href="https://www.boost.org/sgi/stl/EqualityComparable.html">EqualityComparable</a> and/or
         <a href="../../../utility/LessThanComparable.html">LessThanComparable</a> if the <code>circular_buffer</code>
         will be compared with another container.
    \tparam Alloc The allocator type used for all internal memory management.
    \par Type Requirements Alloc
         The <code>Alloc</code> has to meet the allocator requirements imposed by STL.
    \par Default Alloc
         std::allocator<T>

    For detailed documentation of the circular_buffer visit:
    http://www.boost.org/libs/circular_buffer/doc/circular_buffer.html
*/
template <class T, class Alloc>
class circular_buffer
:
/*! \cond */
#if BOOST_CB_ENABLE_DEBUG
public cb_details::debug_iterator_registry,
#endif
/*! \endcond */
private empty_value<Alloc>
{
    typedef empty_value<Alloc> base;

  // Requirements
    //BOOST_CLASS_REQUIRE(T, boost, SGIAssignableConcept);


    //BOOST_CONCEPT_ASSERT((Assignable<T>));
    //BOOST_CONCEPT_ASSERT((CopyConstructible<T>));
    //BOOST_CONCEPT_ASSERT((DefaultConstructible<T>));

    // Required if the circular_buffer will be compared with anther container.
    //BOOST_CONCEPT_ASSERT((EqualityComparable<T>));
    //BOOST_CONCEPT_ASSERT((LessThanComparable<T>));

public:
// Basic types
    
    //! The type of this <code>circular_buffer</code>.
    typedef circular_buffer<T, Alloc> this_type;

    //! The type of elements stored in the <code>circular_buffer</code>.
    typedef typename Alloc::value_type value_type;

    //! A pointer to an element.
    typedef typename allocator_pointer<Alloc>::type pointer;

    //! A const pointer to the element.
    typedef typename allocator_const_pointer<Alloc>::type const_pointer;

    //! A reference to an element.
    typedef value_type& reference;

    //! A const reference to an element.
    typedef const value_type& const_reference;

    //! The distance type.
    /*!
        (A signed integral type used to represent the distance between two iterators.)
    */
    typedef typename allocator_difference_type<Alloc>::type difference_type;

    //! The size type.
    /*!
        (An unsigned integral type that can represent any non-negative value of the container's distance type.)
    */
    typedef typename allocator_size_type<Alloc>::type size_type;

    //! The type of an allocator used in the <code>circular_buffer</code>.
    typedef Alloc allocator_type;

// Iterators

    //! A const (random access) iterator used to iterate through the <code>circular_buffer</code>.
    typedef cb_details::iterator< circular_buffer<T, Alloc>, cb_details::const_traits<Alloc> > const_iterator;

    //! A (random access) iterator used to iterate through the <code>circular_buffer</code>.
    typedef cb_details::iterator< circular_buffer<T, Alloc>, cb_details::nonconst_traits<Alloc> > iterator;

    //! A const iterator used to iterate backwards through a <code>circular_buffer</code>.
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    //! An iterator used to iterate backwards through a <code>circular_buffer</code>.
    typedef std::reverse_iterator<iterator> reverse_iterator;

// Container specific types

    //! An array range.
    /*!
        (A typedef for the <a href="https://www.boost.org/sgi/stl/pair.html"><code>std::pair</code></a> where
        its first element is a pointer to a beginning of an array and its second element represents
        a size of the array.)
    */
    typedef std::pair<pointer, size_type> array_range;

    //! A range of a const array.
    /*!
        (A typedef for the <a href="https://www.boost.org/sgi/stl/pair.html"><code>std::pair</code></a> where
        its first element is a pointer to a beginning of a const array and its second element represents
        a size of the const array.)
    */
    typedef std::pair<const_pointer, size_type> const_array_range;

    //! The capacity type.
    /*!
        (Same as <code>size_type</code> - defined for consistency with the  __cbso class.

    */
    // <a href="space_optimized.html"><code>circular_buffer_space_optimized</code></a>.)

    typedef size_type capacity_type;

// Helper types

    //! A type representing the "best" way to pass the value_type to a method.
    typedef const value_type& param_value_type;

    //! A type representing rvalue from param type.
    //! On compilers without rvalue references support this type is the Boost.Moves type used for emulation.
    typedef BOOST_RV_REF(value_type) rvalue_type;

private:
// Member variables

    //! The internal buffer used for storing elements in the circular buffer.
    pointer m_buff;

    //! The internal buffer's end (end of the storage space).
    pointer m_end;

    //! The virtual beginning of the circular buffer.
    pointer m_first;

    //! The virtual end of the circular buffer (one behind the last element).
    pointer m_last;

    //! The number of items currently stored in the circular buffer.
    size_type m_size;

// Friends
#if defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
    friend iterator;
    friend const_iterator;
#else
    template <class Buff, class Traits> friend struct cb_details::iterator;
#endif

public:
// Allocator

    //! Get the allocator.
    /*!
        \return The allocator.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>get_allocator()</code> for obtaining an allocator %reference.
    */
    allocator_type get_allocator() const BOOST_NOEXCEPT { return alloc(); }

    //! Get the allocator reference.
    /*!
        \return A reference to the allocator.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \note This method was added in order to optimize obtaining of the allocator with a state,
              although use of stateful allocators in STL is discouraged.
        \sa <code>get_allocator() const</code>
    */
    allocator_type& get_allocator() BOOST_NOEXCEPT { return alloc(); }

// Element access

    //! Get the iterator pointing to the beginning of the <code>circular_buffer</code>.
    /*!
        \return A random access iterator pointing to the first element of the <code>circular_buffer</code>. If the
                <code>circular_buffer</code> is empty it returns an iterator equal to the one returned by
                <code>end()</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>end()</code>, <code>rbegin()</code>, <code>rend()</code>
    */
    iterator begin() BOOST_NOEXCEPT { return iterator(this, empty() ? 0 : m_first); }

    //! Get the iterator pointing to the end of the <code>circular_buffer</code>.
    /*!
        \return A random access iterator pointing to the element "one behind" the last element of the <code>
                circular_buffer</code>. If the <code>circular_buffer</code> is empty it returns an iterator equal to
                the one returned by <code>begin()</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>begin()</code>, <code>rbegin()</code>, <code>rend()</code>
    */
    iterator end() BOOST_NOEXCEPT { return iterator(this, 0); }

    //! Get the const iterator pointing to the beginning of the <code>circular_buffer</code>.
    /*!
        \return A const random access iterator pointing to the first element of the <code>circular_buffer</code>. If
                the <code>circular_buffer</code> is empty it returns an iterator equal to the one returned by
                <code>end() const</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>end() const</code>, <code>rbegin() const</code>, <code>rend() const</code>
    */
    const_iterator begin() const BOOST_NOEXCEPT { return const_iterator(this, empty() ? 0 : m_first); }

    const_iterator cbegin() const BOOST_NOEXCEPT { return begin(); }
    //! Get the const iterator pointing to the end of the <code>circular_buffer</code>.
    /*!
        \return A const random access iterator pointing to the element "one behind" the last element of the <code>
                circular_buffer</code>. If the <code>circular_buffer</code> is empty it returns an iterator equal to
                the one returned by <code>begin() const</code> const.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>begin() const</code>, <code>rbegin() const</code>, <code>rend() const</code>
    */
    const_iterator end() const BOOST_NOEXCEPT { return const_iterator(this, 0); }

    const_iterator cend() const BOOST_NOEXCEPT { return end(); }
    //! Get the iterator pointing to the beginning of the "reversed" <code>circular_buffer</code>.
    /*!
        \return A reverse random access iterator pointing to the last element of the <code>circular_buffer</code>.
                If the <code>circular_buffer</code> is empty it returns an iterator equal to the one returned by
                <code>rend()</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>rend()</code>, <code>begin()</code>, <code>end()</code>
    */
    reverse_iterator rbegin() BOOST_NOEXCEPT { return reverse_iterator(end()); }

    //! Get the iterator pointing to the end of the "reversed" <code>circular_buffer</code>.
    /*!
        \return A reverse random access iterator pointing to the element "one before" the first element of the <code>
                circular_buffer</code>. If the <code>circular_buffer</code> is empty it returns an iterator equal to
                the one returned by <code>rbegin()</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>rbegin()</code>, <code>begin()</code>, <code>end()</code>
    */
    reverse_iterator rend() BOOST_NOEXCEPT { return reverse_iterator(begin()); }

    //! Get the const iterator pointing to the beginning of the "reversed" <code>circular_buffer</code>.
    /*!
        \return A const reverse random access iterator pointing to the last element of the
                <code>circular_buffer</code>. If the <code>circular_buffer</code> is empty it returns an iterator equal
                to the one returned by <code>rend() const</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>rend() const</code>, <code>begin() const</code>, <code>end() const</code>
    */
    const_reverse_iterator rbegin() const BOOST_NOEXCEPT { return const_reverse_iterator(end()); }

    //! Get the const iterator pointing to the end of the "reversed" <code>circular_buffer</code>.
    /*!
        \return A const reverse random access iterator pointing to the element "one before" the first element of the
                <code>circular_buffer</code>. If the <code>circular_buffer</code> is empty it returns an iterator equal
                to the one returned by <code>rbegin() const</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>rbegin() const</code>, <code>begin() const</code>, <code>end() const</code>
    */
    const_reverse_iterator rend() const BOOST_NOEXCEPT { return const_reverse_iterator(begin()); }

    //! Get the element at the <code>index</code> position.
    /*!
        \pre <code>0 \<= index \&\& index \< size()</code>
        \param index The position of the element.
        \return A reference to the element at the <code>index</code> position.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>at()</code>
    */
    reference operator [] (size_type index) {
        BOOST_CB_ASSERT(index < size()); // check for invalid index
        return *add(m_first, index);
    }

    //! Get the element at the <code>index</code> position.
    /*!
        \pre <code>0 \<= index \&\& index \< size()</code>
        \param index The position of the element.
        \return A const reference to the element at the <code>index</code> position.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>\link at(size_type)const at() const \endlink</code>
    */
    const_reference operator [] (size_type index) const {
        BOOST_CB_ASSERT(index < size()); // check for invalid index
        return *add(m_first, index);
    }

    //! Get the element at the <code>index</code> position.
    /*!
        \param index The position of the element.
        \return A reference to the element at the <code>index</code> position.
        \throws <code>std::out_of_range</code> when the <code>index</code> is invalid (when
                <code>index >= size()</code>).
        \par Exception Safety
             Strong.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>\link operator[](size_type) operator[] \endlink</code>
    */
    reference at(size_type index) {
        check_position(index);
        return (*this)[index];
    }

    //! Get the element at the <code>index</code> position.
    /*!
        \param index The position of the element.
        \return A const reference to the element at the <code>index</code> position.
        \throws <code>std::out_of_range</code> when the <code>index</code> is invalid (when
                <code>index >= size()</code>).
        \par Exception Safety
             Strong.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>\link operator[](size_type)const operator[] const \endlink</code>
    */
    const_reference at(size_type index) const {
        check_position(index);
        return (*this)[index];
    }

    //! Get the first element.
    /*!
        \pre <code>!empty()</code>
        \return A reference to the first element of the <code>circular_buffer</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>back()</code>
    */
    reference front() {
        BOOST_CB_ASSERT(!empty()); // check for empty buffer (front element not available)
        return *m_first;
    }

    //! Get the last element.
    /*!
        \pre <code>!empty()</code>
        \return A reference to the last element of the <code>circular_buffer</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>front()</code>
    */
    reference back() {
        BOOST_CB_ASSERT(!empty()); // check for empty buffer (back element not available)
        return *((m_last == m_buff ? m_end : m_last) - 1);
    }

    //! Get the first element.
    /*!
        \pre <code>!empty()</code>
        \return A const reference to the first element of the <code>circular_buffer</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>back() const</code>
    */
    const_reference front() const {
        BOOST_CB_ASSERT(!empty()); // check for empty buffer (front element not available)
        return *m_first;
    }

    //! Get the last element.
    /*!
        \pre <code>!empty()</code>
        \return A const reference to the last element of the <code>circular_buffer</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>front() const</code>
    */
    const_reference back() const {
        BOOST_CB_ASSERT(!empty()); // check for empty buffer (back element not available)
        return *((m_last == m_buff ? m_end : m_last) - 1);
    }

    //! Get the first continuous array of the internal buffer.
    /*!
        This method in combination with <code>array_two()</code> can be useful when passing the stored data into
        a legacy C API as an array. Suppose there is a <code>circular_buffer</code> of capacity 10, containing 7
        characters <code>'a', 'b', ..., 'g'</code> where <code>buff[0] == 'a'</code>, <code>buff[1] == 'b'</code>,
        ... and <code>buff[6] == 'g'</code>:<br><br>
        <code>circular_buffer<char> buff(10);</code><br><br>
        The internal representation is often not linear and the state of the internal buffer may look like this:<br>
        <br><code>
        |e|f|g| | | |a|b|c|d|<br>
        end ___^<br>
        begin _______^</code><br><br>

        where <code>|a|b|c|d|</code> represents the "array one", <code>|e|f|g|</code> represents the "array two" and
        <code>| | | |</code> is a free space.<br>
        Now consider a typical C style function for writing data into a file:<br><br>
        <code>int write(int file_desc, char* buff, int num_bytes);</code><br><br>
        There are two ways how to write the content of the <code>circular_buffer</code> into a file. Either relying
        on <code>array_one()</code> and <code>array_two()</code> methods and calling the write function twice:<br><br>
        <code>array_range ar = buff.array_one();<br>
        write(file_desc, ar.first, ar.second);<br>
        ar = buff.array_two();<br>
        write(file_desc, ar.first, ar.second);</code><br><br>
        Or relying on the <code>linearize()</code> method:<br><br><code>
        write(file_desc, buff.linearize(), buff.size());</code><br><br>
        Since the complexity of <code>array_one()</code> and <code>array_two()</code> methods is constant the first
        option is suitable when calling the write method is "cheap". On the other hand the second option is more
        suitable when calling the write method is more "expensive" than calling the <code>linearize()</code> method
        whose complexity is linear.
        \return The array range of the first continuous array of the internal buffer. In the case the
                <code>circular_buffer</code> is empty the size of the returned array is <code>0</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \warning In general invoking any method which modifies the internal state of the circular_buffer  may
                 delinearize the internal buffer and invalidate the array ranges returned by <code>array_one()</code>
                 and <code>array_two()</code> (and their const versions).
        \note In the case the internal buffer is linear e.g. <code>|a|b|c|d|e|f|g| | | |</code> the "array one" is
              represented by <code>|a|b|c|d|e|f|g|</code> and the "array two" does not exist (the
              <code>array_two()</code> method returns an array with the size <code>0</code>).
        \sa <code>array_two()</code>, <code>linearize()</code>
    */
    array_range array_one() {
        return array_range(m_first, (m_last <= m_first && !empty() ? m_end : m_last) - m_first);
    }

    //! Get the second continuous array of the internal buffer.
    /*!
        This method in combination with <code>array_one()</code> can be useful when passing the stored data into
        a legacy C API as an array.
        \return The array range of the second continuous array of the internal buffer. In the case the internal buffer
                is linear or the <code>circular_buffer</code> is empty the size of the returned array is
                <code>0</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>array_one()</code>
    */
    array_range array_two() {
        return array_range(m_buff, m_last <= m_first && !empty() ? m_last - m_buff : 0);
    }

    //! Get the first continuous array of the internal buffer.
    /*!
        This method in combination with <code>array_two() const</code> can be useful when passing the stored data into
        a legacy C API as an array.
        \return The array range of the first continuous array of the internal buffer. In the case the
                <code>circular_buffer</code> is empty the size of the returned array is <code>0</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>array_two() const</code>; <code>array_one()</code> for more details how to pass data into a legacy C
            API.
    */
    const_array_range array_one() const {
        return const_array_range(m_first, (m_last <= m_first && !empty() ? m_end : m_last) - m_first);
    }

    //! Get the second continuous array of the internal buffer.
    /*!
        This method in combination with <code>array_one() const</code> can be useful when passing the stored data into
        a legacy C API as an array.
        \return The array range of the second continuous array of the internal buffer. In the case the internal buffer
                is linear or the <code>circular_buffer</code> is empty the size of the returned array is
                <code>0</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>array_one() const</code>
    */
    const_array_range array_two() const {
        return const_array_range(m_buff, m_last <= m_first && !empty() ? m_last - m_buff : 0);
    }

    //! Linearize the internal buffer into a continuous array.
    /*!
        This method can be useful when passing the stored data into a legacy C API as an array.
        \post <code>\&(*this)[0] \< \&(*this)[1] \< ... \< \&(*this)[size() - 1]</code>
        \return A pointer to the beginning of the array or <code>0</code> if empty.
        \throws <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operations in the <i>Throws</i> section do not throw anything.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>); does not invalidate any iterators if the postcondition (the <i>Effect</i>) is already
             met prior calling this method.
        \par Complexity
             Linear (in the size of the <code>circular_buffer</code>); constant if the postcondition (the
             <i>Effect</i>) is already met.
        \warning In general invoking any method which modifies the internal state of the <code>circular_buffer</code>
                 may delinearize the internal buffer and invalidate the returned pointer.
        \sa <code>array_one()</code> and <code>array_two()</code> for the other option how to pass data into a legacy
            C API; <code>is_linearized()</code>, <code>rotate(const_iterator)</code>
    */
    pointer linearize() {
        if (empty())
            return 0;
        if (m_first < m_last || m_last == m_buff)
            return m_first;
        pointer src = m_first;
        pointer dest = m_buff;
        size_type moved = 0;
        size_type constructed = 0;
        BOOST_TRY {
            for (pointer first = m_first; dest < src; src = first) {
                for (size_type ii = 0; src < m_end; ++src, ++dest, ++moved, ++ii) {
                    if (moved == size()) {
                        first = dest;
                        break;
                    }
                    if (dest == first) {
                        first += ii;
                        break;
                    }
                    if (is_uninitialized(dest)) {
                        boost::allocator_construct(alloc(), boost::to_address(dest), boost::move_if_noexcept(*src));
                        ++constructed;
                    } else {
                        value_type tmp = boost::move_if_noexcept(*src); 
                        replace(src, boost::move_if_noexcept(*dest));
                        replace(dest, boost::move(tmp));
                    }
                }
            }
        } BOOST_CATCH(...) {
            m_last += constructed;
            m_size += constructed;
            BOOST_RETHROW
        }
        BOOST_CATCH_END
        for (src = m_end - constructed; src < m_end; ++src)
            destroy_item(src);
        m_first = m_buff;
        m_last = add(m_buff, size());
#if BOOST_CB_ENABLE_DEBUG
        invalidate_iterators_except(end());
#endif
        return m_buff;
    }

    //! Is the <code>circular_buffer</code> linearized?
    /*!
        \return <code>true</code> if the internal buffer is linearized into a continuous array (i.e. the
                <code>circular_buffer</code> meets a condition
                <code>\&(*this)[0] \< \&(*this)[1] \< ... \< \&(*this)[size() - 1]</code>);
                <code>false</code> otherwise.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>linearize()</code>, <code>array_one()</code>, <code>array_two()</code>
    */
    bool is_linearized() const BOOST_NOEXCEPT { return m_first < m_last || m_last == m_buff; }

    //! Rotate elements in the <code>circular_buffer</code>.
    /*!
        A more effective implementation of
        <code><a href="https://www.boost.org/sgi/stl/rotate.html">std::rotate</a></code>.
        \pre <code>new_begin</code> is a valid iterator pointing to the <code>circular_buffer</code> <b>except</b> its
             end.
        \post Before calling the method suppose:<br><br>
              <code>m == std::distance(new_begin, end())</code><br><code>n == std::distance(begin(), new_begin)</code>
              <br><code>val_0 == *new_begin, val_1 == *(new_begin + 1), ... val_m == *(new_begin + m)</code><br>
              <code>val_r1 == *(new_begin - 1), val_r2 == *(new_begin - 2), ... val_rn == *(new_begin - n)</code><br>
              <br>then after call to the method:<br><br>
              <code>val_0 == (*this)[0] \&\& val_1 == (*this)[1] \&\& ... \&\& val_m == (*this)[m - 1] \&\& val_r1 ==
              (*this)[m + n - 1] \&\& val_r2 == (*this)[m + n - 2] \&\& ... \&\& val_rn == (*this)[m]</code>
        \param new_begin The new beginning.
        \throws See <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the <code>circular_buffer</code> is full or <code>new_begin</code> points to
             <code>begin()</code> or if the operations in the <i>Throws</i> section do not throw anything.
        \par Iterator Invalidation
             If <code>m \< n</code> invalidates iterators pointing to the last <code>m</code> elements
             (<b>including</b> <code>new_begin</code>, but not iterators equal to <code>end()</code>) else invalidates
             iterators pointing to the first <code>n</code> elements; does not invalidate any iterators if the
             <code>circular_buffer</code> is full.
        \par Complexity
             Linear (in <code>(std::min)(m, n)</code>); constant if the <code>circular_buffer</code> is full.
        \sa <code><a href="https://www.boost.org/sgi/stl/rotate.html">std::rotate</a></code>
    */
    void rotate(const_iterator new_begin) {
        BOOST_CB_ASSERT(new_begin.is_valid(this)); // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(new_begin.m_it != 0);      // check for iterator pointing to end()
        if (full()) {
            m_first = m_last = const_cast<pointer>(new_begin.m_it);
        } else {
            difference_type m = end() - new_begin;
            difference_type n = new_begin - begin();
            if (m < n) {
                for (; m > 0; --m) {
                    push_front(boost::move_if_noexcept(back()));
                    pop_back();
                }
            } else {
                for (; n > 0; --n) {
                    push_back(boost::move_if_noexcept(front()));
                    pop_front();
                }
            }
        }
    }

// Size and capacity

    //! Get the number of elements currently stored in the <code>circular_buffer</code>.
    /*!
        \return The number of elements stored in the <code>circular_buffer</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>capacity()</code>, <code>max_size()</code>, <code>reserve()</code>,
            <code>\link resize() resize(size_type, const_reference)\endlink</code>
    */
    size_type size() const BOOST_NOEXCEPT { return m_size; }

    /*! \brief Get the largest possible size or capacity of the <code>circular_buffer</code>. (It depends on
               allocator's %max_size()).
        \return The maximum size/capacity the <code>circular_buffer</code> can be set to.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>size()</code>, <code>capacity()</code>, <code>reserve()</code>
    */
    size_type max_size() const BOOST_NOEXCEPT {
        return (std::min<size_type>)(boost::allocator_max_size(alloc()), (std::numeric_limits<difference_type>::max)());
    }

    //! Is the <code>circular_buffer</code> empty?
    /*!
        \return <code>true</code> if there are no elements stored in the <code>circular_buffer</code>;
                <code>false</code> otherwise.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>full()</code>
    */
    bool empty() const BOOST_NOEXCEPT { return size() == 0; }

    //! Is the <code>circular_buffer</code> full?
    /*!
        \return <code>true</code> if the number of elements stored in the <code>circular_buffer</code>
                equals the capacity of the <code>circular_buffer</code>; <code>false</code> otherwise.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>empty()</code>
    */
    bool full() const BOOST_NOEXCEPT { return capacity() == size(); }

    /*! \brief Get the maximum number of elements which can be inserted into the <code>circular_buffer</code> without
               overwriting any of already stored elements.
        \return <code>capacity() - size()</code>
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>capacity()</code>, <code>size()</code>, <code>max_size()</code>
    */
    size_type reserve() const BOOST_NOEXCEPT { return capacity() - size(); }

    //! Get the capacity of the <code>circular_buffer</code>.
    /*!
        \return The maximum number of elements which can be stored in the <code>circular_buffer</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Does not invalidate any iterators.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>reserve()</code>, <code>size()</code>, <code>max_size()</code>,
            <code>set_capacity(capacity_type)</code>
    */
    capacity_type capacity() const BOOST_NOEXCEPT { return m_end - m_buff; }

    //! Change the capacity of the <code>circular_buffer</code>.
    /*! 
        \pre If <code>T</code> is a move only type, then compiler shall support <code>noexcept</code> modifiers
                and move constructor of <code>T</code> must be marked with it (must not throw exceptions).
        \post <code>capacity() == new_capacity \&\& size() \<= new_capacity</code><br><br>
              If the current number of elements stored in the <code>circular_buffer</code> is greater than the desired
              new capacity then number of <code>[size() - new_capacity]</code> <b>last</b> elements will be removed and
              the new size will be equal to <code>new_capacity</code>.
        \param new_capacity The new capacity.
        \throws "An allocation error" if memory is exhausted, (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws or nothing if <code>T::T(T&&)</code> is noexcept.
        \par Exception Safety
             Strong.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>) if the new capacity is different from the original.
        \par Complexity
             Linear (in <code>min[size(), new_capacity]</code>).
        \sa <code>rset_capacity(capacity_type)</code>,
            <code>\link resize() resize(size_type, const_reference)\endlink</code>
    */
    void set_capacity(capacity_type new_capacity) {
        if (new_capacity == capacity())
            return;
        pointer buff = allocate(new_capacity);
        iterator b = begin();
        BOOST_TRY {
            reset(buff,
                cb_details::uninitialized_move_if_noexcept(b, b + (std::min)(new_capacity, size()), buff, alloc()),
                new_capacity);
        } BOOST_CATCH(...) {
            deallocate(buff, new_capacity);
            BOOST_RETHROW
        }
        BOOST_CATCH_END
    }

    //! Change the size of the <code>circular_buffer</code>.
    /*!
        \post <code>size() == new_size \&\& capacity() >= new_size</code><br><br>
              If the new size is greater than the current size, copies of <code>item</code> will be inserted at the
              <b>back</b> of the of the <code>circular_buffer</code> in order to achieve the desired size. In the case
              the resulting size exceeds the current capacity the capacity will be set to <code>new_size</code>.<br>
              If the current number of elements stored in the <code>circular_buffer</code> is greater than the desired
              new size then number of <code>[size() - new_size]</code> <b>last</b> elements will be removed. (The
              capacity will remain unchanged.)
        \param new_size The new size.
        \param item The element the <code>circular_buffer</code> will be filled with in order to gain the requested
                    size. (See the <i>Effect</i>.)
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws or nothing if <code>T::T(T&&)</code> is noexcept.
        \par Exception Safety
             Basic.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>) if the new size is greater than the current capacity. Invalidates iterators pointing
             to the removed elements if the new size is lower that the original size. Otherwise it does not invalidate
             any iterator.
        \par Complexity
             Linear (in the new size of the <code>circular_buffer</code>).
        \sa <code>\link rresize() rresize(size_type, const_reference)\endlink</code>,
            <code>set_capacity(capacity_type)</code>
    */
    void resize(size_type new_size, param_value_type item = value_type()) {
        if (new_size > size()) {
            if (new_size > capacity())
                set_capacity(new_size);
            insert(end(), new_size - size(), item);
        } else {
            iterator e = end();
            erase(e - (size() - new_size), e);
        }
    }

    //! Change the capacity of the <code>circular_buffer</code>.
    /*! 
        \pre If <code>T</code> is a move only type, then compiler shall support <code>noexcept</code> modifiers
                and move constructor of <code>T</code> must be marked with it (must not throw exceptions).
        \post <code>capacity() == new_capacity \&\& size() \<= new_capacity</code><br><br>
              If the current number of elements stored in the <code>circular_buffer</code> is greater than the desired
              new capacity then number of <code>[size() - new_capacity]</code> <b>first</b> elements will be removed
              and the new size will be equal to <code>new_capacity</code>.
        \param new_capacity The new capacity.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws or nothing if <code>T::T(T&&)</code> is noexcept.
        \par Exception Safety
             Strong.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>) if the new capacity is different from the original.
        \par Complexity
             Linear (in <code>min[size(), new_capacity]</code>).
        \sa <code>set_capacity(capacity_type)</code>,
            <code>\link rresize() rresize(size_type, const_reference)\endlink</code>
    */
    void rset_capacity(capacity_type new_capacity) {
        if (new_capacity == capacity())
            return;
        pointer buff = allocate(new_capacity);
        iterator e = end();
        BOOST_TRY {
            reset(buff, cb_details::uninitialized_move_if_noexcept(e - (std::min)(new_capacity, size()),
                e, buff, alloc()), new_capacity);
        } BOOST_CATCH(...) {
            deallocate(buff, new_capacity);
            BOOST_RETHROW
        }
        BOOST_CATCH_END
    }

    //! Change the size of the <code>circular_buffer</code>.
    /*!
        \post <code>size() == new_size \&\& capacity() >= new_size</code><br><br>
              If the new size is greater than the current size, copies of <code>item</code> will be inserted at the
              <b>front</b> of the of the <code>circular_buffer</code> in order to achieve the desired size. In the case
              the resulting size exceeds the current capacity the capacity will be set to <code>new_size</code>.<br>
              If the current number of elements stored in the <code>circular_buffer</code> is greater than the desired
              new size then number of <code>[size() - new_size]</code> <b>first</b> elements will be removed. (The
              capacity will remain unchanged.)
        \param new_size The new size.
        \param item The element the <code>circular_buffer</code> will be filled with in order to gain the requested
                    size. (See the <i>Effect</i>.)
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws or nothing if <code>T::T(T&&)</code> is noexcept.
        \par Exception Safety
             Basic.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>) if the new size is greater than the current capacity. Invalidates iterators pointing
             to the removed elements if the new size is lower that the original size. Otherwise it does not invalidate
             any iterator.
        \par Complexity
             Linear (in the new size of the <code>circular_buffer</code>).
        \sa <code>\link resize() resize(size_type, const_reference)\endlink</code>,
            <code>rset_capacity(capacity_type)</code>
    */
    void rresize(size_type new_size, param_value_type item = value_type()) {
        if (new_size > size()) {
            if (new_size > capacity())
                set_capacity(new_size);
            rinsert(begin(), new_size - size(), item);
        } else {
            rerase(begin(), end() - new_size);
        }
    }

// Construction/Destruction

    //! Create an empty <code>circular_buffer</code> with zero capacity.
    /*!
        \post <code>capacity() == 0 \&\& size() == 0</code>
        \param alloc The allocator.
        \throws Nothing.
        \par Complexity
             Constant.
        \warning Since Boost version 1.36 the behaviour of this constructor has changed. Now the constructor does not
                 allocate any memory and both capacity and size are set to zero. Also note when inserting an element
                 into a <code>circular_buffer</code> with zero capacity (e.g. by
                 <code>\link push_back() push_back(const_reference)\endlink</code> or
                 <code>\link insert(iterator, param_value_type) insert(iterator, value_type)\endlink</code>) nothing
                 will be inserted and the size (as well as capacity) remains zero.
        \note You can explicitly set the capacity by calling the <code>set_capacity(capacity_type)</code> method or you
              can use the other constructor with the capacity specified.
        \sa <code>circular_buffer(capacity_type, const allocator_type& alloc)</code>,
            <code>set_capacity(capacity_type)</code>
    */
    explicit circular_buffer(const allocator_type& alloc = allocator_type()) BOOST_NOEXCEPT
    : base(boost::empty_init_t(), alloc), m_buff(0), m_end(0), m_first(0), m_last(0), m_size(0) {}

    //! Create an empty <code>circular_buffer</code> with the specified capacity.
    /*!
        \post <code>capacity() == buffer_capacity \&\& size() == 0</code>
        \param buffer_capacity The maximum number of elements which can be stored in the <code>circular_buffer</code>.
        \param alloc The allocator.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
        \par Complexity
             Constant.
    */
    explicit circular_buffer(capacity_type buffer_capacity, const allocator_type& alloc = allocator_type())
    : base(boost::empty_init_t(), alloc), m_size(0) {
        initialize_buffer(buffer_capacity);
        m_first = m_last = m_buff;
    }

    /*! \brief Create a full <code>circular_buffer</code> with the specified capacity and filled with <code>n</code>
               copies of <code>item</code>.
        \post <code>capacity() == n \&\& full() \&\& (*this)[0] == item \&\& (*this)[1] == item \&\& ... \&\&
              (*this)[n - 1] == item </code>
        \param n The number of elements the created <code>circular_buffer</code> will be filled with.
        \param item The element the created <code>circular_buffer</code> will be filled with.
        \param alloc The allocator.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws.
        \par Complexity
             Linear (in the <code>n</code>).
    */
    circular_buffer(size_type n, param_value_type item, const allocator_type& alloc = allocator_type())
    : base(boost::empty_init_t(), alloc), m_size(n) {
        initialize_buffer(n, item);
        m_first = m_last = m_buff;
    }

    /*! \brief Create a <code>circular_buffer</code> with the specified capacity and filled with <code>n</code>
               copies of <code>item</code>.
        \pre <code>buffer_capacity >= n</code>
        \post <code>capacity() == buffer_capacity \&\& size() == n \&\& (*this)[0] == item \&\& (*this)[1] == item
              \&\& ... \&\& (*this)[n - 1] == item</code>
        \param buffer_capacity The capacity of the created <code>circular_buffer</code>.
        \param n The number of elements the created <code>circular_buffer</code> will be filled with.
        \param item The element the created <code>circular_buffer</code> will be filled with.
        \param alloc The allocator.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws.
        \par Complexity
             Linear (in the <code>n</code>).
    */
    circular_buffer(capacity_type buffer_capacity, size_type n, param_value_type item,
        const allocator_type& alloc = allocator_type())
    : base(boost::empty_init_t(), alloc), m_size(n) {
        BOOST_CB_ASSERT(buffer_capacity >= size()); // check for capacity lower than size
        initialize_buffer(buffer_capacity, item);
        m_first = m_buff;
        m_last = buffer_capacity == n ? m_buff : m_buff + n;
    }

    //! The copy constructor.
    /*!
        Creates a copy of the specified <code>circular_buffer</code>.
        \post <code>*this == cb</code>
        \param cb The <code>circular_buffer</code> to be copied.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws.
        \par Complexity
             Linear (in the size of <code>cb</code>).
    */
    circular_buffer(const circular_buffer<T, Alloc>& cb)
    :
#if BOOST_CB_ENABLE_DEBUG
    debug_iterator_registry(),
#endif
    base(boost::empty_init_t(), cb.get_allocator()),
    m_size(cb.size()) {
        initialize_buffer(cb.capacity());
        m_first = m_buff;
        BOOST_TRY {
            m_last = cb_details::uninitialized_copy(cb.begin(), cb.end(), m_buff, alloc());
        } BOOST_CATCH(...) {
            deallocate(m_buff, cb.capacity());
            BOOST_RETHROW
        }
        BOOST_CATCH_END
        if (m_last == m_end)
            m_last = m_buff;
    }
    
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    //! The move constructor.
    /*! \brief Move constructs a <code>circular_buffer</code> from <code>cb</code>, leaving <code>cb</code> empty.
        \pre C++ compiler with rvalue references support.
        \post <code>cb.empty()</code>
        \param cb <code>circular_buffer</code> to 'steal' value from.
        \throws Nothing.
        \par Constant.
    */
    circular_buffer(circular_buffer<T, Alloc>&& cb) BOOST_NOEXCEPT
    : base(boost::empty_init_t(), cb.get_allocator()), m_buff(0), m_end(0), m_first(0), m_last(0), m_size(0) {
        cb.swap(*this);
    }
#endif // BOOST_NO_CXX11_RVALUE_REFERENCES

    //! Create a full <code>circular_buffer</code> filled with a copy of the range.
    /*!
        \pre Valid range <code>[first, last)</code>.<br>
             <code>first</code> and <code>last</code> have to meet the requirements of
             <a href="https://www.boost.org/sgi/stl/InputIterator.html">InputIterator</a>.
        \post <code>capacity() == std::distance(first, last) \&\& full() \&\& (*this)[0]== *first \&\&
              (*this)[1] == *(first + 1) \&\& ... \&\& (*this)[std::distance(first, last) - 1] == *(last - 1)</code>
        \param first The beginning of the range to be copied.
        \param last The end of the range to be copied.
        \param alloc The allocator.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws.
        \par Complexity
             Linear (in the <code>std::distance(first, last)</code>).
    */
    template <class InputIterator>
    circular_buffer(InputIterator first, InputIterator last, const allocator_type& alloc = allocator_type())
    : base(boost::empty_init_t(), alloc) {
        initialize(first, last, is_integral<InputIterator>());
    }

    //! Create a <code>circular_buffer</code> with the specified capacity and filled with a copy of the range.
    /*!
        \pre Valid range <code>[first, last)</code>.<br>
             <code>first</code> and <code>last</code> have to meet the requirements of
             <a href="https://www.boost.org/sgi/stl/InputIterator.html">InputIterator</a>.
        \post <code>capacity() == buffer_capacity \&\& size() \<= std::distance(first, last) \&\&
             (*this)[0]== *(last - buffer_capacity) \&\& (*this)[1] == *(last - buffer_capacity + 1) \&\& ... \&\&
             (*this)[buffer_capacity - 1] == *(last - 1)</code><br><br>
             If the number of items to be copied from the range <code>[first, last)</code> is greater than the
             specified <code>buffer_capacity</code> then only elements from the range
             <code>[last - buffer_capacity, last)</code> will be copied.
        \param buffer_capacity The capacity of the created <code>circular_buffer</code>.
        \param first The beginning of the range to be copied.
        \param last The end of the range to be copied.
        \param alloc The allocator.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws.
        \par Complexity
             Linear (in <code>std::distance(first, last)</code>; in
             <code>min[capacity, std::distance(first, last)]</code> if the <code>InputIterator</code> is a
             <a href="https://www.boost.org/sgi/stl/RandomAccessIterator.html">RandomAccessIterator</a>).
    */
    template <class InputIterator>
    circular_buffer(capacity_type buffer_capacity, InputIterator first, InputIterator last,
        const allocator_type& alloc = allocator_type())
    : base(boost::empty_init_t(), alloc) {
        initialize(buffer_capacity, first, last, is_integral<InputIterator>());
    }

    //! The destructor.
    /*!
        Destroys the <code>circular_buffer</code>.
        \throws Nothing.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (including iterators equal to
             <code>end()</code>).
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>) for scalar types; linear for other types.
        \sa <code>clear()</code>
    */
    ~circular_buffer() BOOST_NOEXCEPT {
        destroy();
#if BOOST_CB_ENABLE_DEBUG
        invalidate_all_iterators();
#endif
    }

public:
// Assign methods

    //! The assign operator.
    /*!
        Makes this <code>circular_buffer</code> to become a copy of the specified <code>circular_buffer</code>.
        \post <code>*this == cb</code>
        \param cb The <code>circular_buffer</code> to be copied.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws.
        \par Exception Safety
             Strong.
        \par Iterator Invalidation
             Invalidates all iterators pointing to this <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>).
        \par Complexity
             Linear (in the size of <code>cb</code>).
        \sa <code>\link assign(size_type, param_value_type) assign(size_type, const_reference)\endlink</code>,
            <code>\link assign(capacity_type, size_type, param_value_type)
            assign(capacity_type, size_type, const_reference)\endlink</code>,
            <code>assign(InputIterator, InputIterator)</code>,
            <code>assign(capacity_type, InputIterator, InputIterator)</code>
    */
    circular_buffer<T, Alloc>& operator = (const circular_buffer<T, Alloc>& cb) {
        if (this == &cb)
            return *this;
        pointer buff = allocate(cb.capacity());
        BOOST_TRY {
            reset(buff, cb_details::uninitialized_copy(cb.begin(), cb.end(), buff, alloc()), cb.capacity());
        } BOOST_CATCH(...) {
            deallocate(buff, cb.capacity());
            BOOST_RETHROW
        }
        BOOST_CATCH_END
        return *this;
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    /*! \brief Move assigns content of <code>cb</code> to <code>*this</code>, leaving <code>cb</code> empty.
        \pre C++ compiler with rvalue references support.
        \post <code>cb.empty()</code>
        \param cb <code>circular_buffer</code> to 'steal' value from.
        \throws Nothing.
        \par Complexity
             Constant.
    */
    circular_buffer<T, Alloc>& operator = (circular_buffer<T, Alloc>&& cb) BOOST_NOEXCEPT {
        cb.swap(*this); // now `this` holds `cb`
        circular_buffer<T, Alloc>(get_allocator()) // temporary that holds initial `cb` allocator
            .swap(cb); // makes `cb` empty
        return *this;
    }
#endif // BOOST_NO_CXX11_RVALUE_REFERENCES

    //! Assign <code>n</code> items into the <code>circular_buffer</code>.
    /*!
        The content of the <code>circular_buffer</code> will be removed and replaced with <code>n</code> copies of the
        <code>item</code>.
        \post <code>capacity() == n \&\& size() == n \&\& (*this)[0] == item \&\& (*this)[1] == item \&\& ... \&\&
              (*this) [n - 1] == item</code>
        \param n The number of elements the <code>circular_buffer</code> will be filled with.
        \param item The element the <code>circular_buffer</code> will be filled with.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws.
        \par Exception Safety
             Basic.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>).
        \par Complexity
             Linear (in the <code>n</code>).
        \sa <code>\link operator=(const circular_buffer&) operator=\endlink</code>,
            <code>\link assign(capacity_type, size_type, param_value_type)
            assign(capacity_type, size_type, const_reference)\endlink</code>,
            <code>assign(InputIterator, InputIterator)</code>,
            <code>assign(capacity_type, InputIterator, InputIterator)</code>
    */
    void assign(size_type n, param_value_type item) {
        assign_n(n, n, cb_details::assign_n<param_value_type, allocator_type>(n, item, alloc()));
    }

    //! Assign <code>n</code> items into the <code>circular_buffer</code> specifying the capacity.
    /*!
        The capacity of the <code>circular_buffer</code> will be set to the specified value and the content of the
        <code>circular_buffer</code> will be removed and replaced with <code>n</code> copies of the <code>item</code>.
        \pre <code>capacity >= n</code>
        \post <code>capacity() == buffer_capacity \&\& size() == n \&\& (*this)[0] == item \&\& (*this)[1] == item
              \&\& ... \&\& (*this) [n - 1] == item </code>
        \param buffer_capacity The new capacity.
        \param n The number of elements the <code>circular_buffer</code> will be filled with.
        \param item The element the <code>circular_buffer</code> will be filled with.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws.
        \par Exception Safety
             Basic.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>).
        \par Complexity
             Linear (in the <code>n</code>).
        \sa <code>\link operator=(const circular_buffer&) operator=\endlink</code>,
            <code>\link assign(size_type, param_value_type) assign(size_type, const_reference)\endlink</code>,
            <code>assign(InputIterator, InputIterator)</code>,
            <code>assign(capacity_type, InputIterator, InputIterator)</code>
    */
    void assign(capacity_type buffer_capacity, size_type n, param_value_type item) {
        BOOST_CB_ASSERT(buffer_capacity >= n); // check for new capacity lower than n
        assign_n(buffer_capacity, n, cb_details::assign_n<param_value_type, allocator_type>(n, item, alloc()));
    }

    //! Assign a copy of the range into the <code>circular_buffer</code>.
    /*!
        The content of the <code>circular_buffer</code> will be removed and replaced with copies of elements from the
        specified range.
        \pre Valid range <code>[first, last)</code>.<br>
             <code>first</code> and <code>last</code> have to meet the requirements of
             <a href="https://www.boost.org/sgi/stl/InputIterator.html">InputIterator</a>.
        \post <code>capacity() == std::distance(first, last) \&\& size() == std::distance(first, last) \&\&
             (*this)[0]== *first \&\& (*this)[1] == *(first + 1) \&\& ... \&\& (*this)[std::distance(first, last) - 1]
             == *(last - 1)</code>
        \param first The beginning of the range to be copied.
        \param last The end of the range to be copied.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws.
        \par Exception Safety
             Basic.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>).
        \par Complexity
             Linear (in the <code>std::distance(first, last)</code>).
        \sa <code>\link operator=(const circular_buffer&) operator=\endlink</code>,
            <code>\link assign(size_type, param_value_type) assign(size_type, const_reference)\endlink</code>,
            <code>\link assign(capacity_type, size_type, param_value_type)
            assign(capacity_type, size_type, const_reference)\endlink</code>,
            <code>assign(capacity_type, InputIterator, InputIterator)</code>
    */
    template <class InputIterator>
    void assign(InputIterator first, InputIterator last) {
        assign(first, last, is_integral<InputIterator>());
    }

    //! Assign a copy of the range into the <code>circular_buffer</code> specifying the capacity.
    /*!
        The capacity of the <code>circular_buffer</code> will be set to the specified value and the content of the
        <code>circular_buffer</code> will be removed and replaced with copies of elements from the specified range.
        \pre Valid range <code>[first, last)</code>.<br>
             <code>first</code> and <code>last</code> have to meet the requirements of
             <a href="https://www.boost.org/sgi/stl/InputIterator.html">InputIterator</a>.
        \post <code>capacity() == buffer_capacity \&\& size() \<= std::distance(first, last) \&\&
             (*this)[0]== *(last - buffer_capacity) \&\& (*this)[1] == *(last - buffer_capacity + 1) \&\& ... \&\&
             (*this)[buffer_capacity - 1] == *(last - 1)</code><br><br>
             If the number of items to be copied from the range <code>[first, last)</code> is greater than the
             specified <code>buffer_capacity</code> then only elements from the range
             <code>[last - buffer_capacity, last)</code> will be copied.
        \param buffer_capacity The new capacity.
        \param first The beginning of the range to be copied.
        \param last The end of the range to be copied.
        \throws "An allocation error" if memory is exhausted (<code>std::bad_alloc</code> if the standard allocator is
                used).
                Whatever <code>T::T(const T&)</code> throws.
        \par Exception Safety
             Basic.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>).
        \par Complexity
             Linear (in <code>std::distance(first, last)</code>; in
             <code>min[capacity, std::distance(first, last)]</code> if the <code>InputIterator</code> is a
             <a href="https://www.boost.org/sgi/stl/RandomAccessIterator.html">RandomAccessIterator</a>).
        \sa <code>\link operator=(const circular_buffer&) operator=\endlink</code>,
            <code>\link assign(size_type, param_value_type) assign(size_type, const_reference)\endlink</code>,
            <code>\link assign(capacity_type, size_type, param_value_type)
            assign(capacity_type, size_type, const_reference)\endlink</code>,
            <code>assign(InputIterator, InputIterator)</code>
    */
    template <class InputIterator>
    void assign(capacity_type buffer_capacity, InputIterator first, InputIterator last) {
        assign(buffer_capacity, first, last, is_integral<InputIterator>());
    }

    //! Swap the contents of two <code>circular_buffer</code>s.
    /*!
        \post <code>this</code> contains elements of <code>cb</code> and vice versa; the capacity of <code>this</code>
              equals to the capacity of <code>cb</code> and vice versa.
        \param cb The <code>circular_buffer</code> whose content will be swapped.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Invalidates all iterators of both <code>circular_buffer</code>s. (On the other hand the iterators still
             point to the same elements but within another container. If you want to rely on this feature you have to
             turn the <a href="#debug">Debug Support</a> off otherwise an assertion will report an error if such
             invalidated iterator is used.)
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>swap(circular_buffer<T, Alloc>&, circular_buffer<T, Alloc>&)</code>
    */
    void swap(circular_buffer<T, Alloc>& cb) BOOST_NOEXCEPT {
        swap_allocator(cb, is_stateless<allocator_type>());
        adl_move_swap(m_buff, cb.m_buff);
        adl_move_swap(m_end, cb.m_end);
        adl_move_swap(m_first, cb.m_first);
        adl_move_swap(m_last, cb.m_last);
        adl_move_swap(m_size, cb.m_size);
#if BOOST_CB_ENABLE_DEBUG
        invalidate_all_iterators();
        cb.invalidate_all_iterators();
#endif
    }

// push and pop
private:
    /*! INTERNAL ONLY */
    template <class ValT>
    void push_back_impl(ValT item) {
        if (full()) {
            if (empty())
                return;
            replace(m_last, static_cast<ValT>(item));
            increment(m_last);
            m_first = m_last;
        } else {
            boost::allocator_construct(alloc(), boost::to_address(m_last), static_cast<ValT>(item));
            increment(m_last);
            ++m_size;
        }        
    }

    /*! INTERNAL ONLY */
    template <class ValT>
    void push_front_impl(ValT item) {
        BOOST_TRY {
            if (full()) {
                if (empty())
                    return;
                decrement(m_first);
                replace(m_first, static_cast<ValT>(item));
                m_last = m_first;
            } else {
                decrement(m_first);
                boost::allocator_construct(alloc(), boost::to_address(m_first), static_cast<ValT>(item));
                ++m_size;
            }
        } BOOST_CATCH(...) {
            increment(m_first);
            BOOST_RETHROW
        }
        BOOST_CATCH_END
    }

public:
    //! Insert a new element at the end of the <code>circular_buffer</code>.
    /*!
        \post if <code>capacity() > 0</code> then <code>back() == item</code><br>
              If the <code>circular_buffer</code> is full, the first element will be removed. If the capacity is
              <code>0</code>, nothing will be inserted.
        \param item The element to be inserted.
        \throws Whatever <code>T::T(const T&)</code> throws.
                Whatever <code>T::operator = (const T&)</code> throws.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Does not invalidate any iterators with the exception of iterators pointing to the overwritten element.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>\link push_front() push_front(const_reference)\endlink</code>,
            <code>pop_back()</code>, <code>pop_front()</code>
    */
    void push_back(param_value_type item) {
        push_back_impl<param_value_type>(item);
    }

    //! Insert a new element at the end of the <code>circular_buffer</code> using rvalue references or rvalues references emulation.
    /*!
        \post if <code>capacity() > 0</code> then <code>back() == item</code><br>
              If the <code>circular_buffer</code> is full, the first element will be removed. If the capacity is
              <code>0</code>, nothing will be inserted.
        \param item The element to be inserted.
        \throws Whatever <code>T::T(T&&)</code> throws.
                Whatever <code>T::operator = (T&&)</code> throws.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Does not invalidate any iterators with the exception of iterators pointing to the overwritten element.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>\link push_front() push_front(const_reference)\endlink</code>,
            <code>pop_back()</code>, <code>pop_front()</code>
    */
    void push_back(rvalue_type item) {
        push_back_impl<rvalue_type>(boost::move(item));
    }

    //! Insert a new default-constructed element at the end of the <code>circular_buffer</code>.
    /*!
        \post if <code>capacity() > 0</code> then <code>back() == item</code><br>
              If the <code>circular_buffer</code> is full, the first element will be removed. If the capacity is
              <code>0</code>, nothing will be inserted.
        \throws Whatever <code>T::T()</code> throws.
                Whatever <code>T::T(T&&)</code> throws.
                Whatever <code>T::operator = (T&&)</code> throws.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Does not invalidate any iterators with the exception of iterators pointing to the overwritten element.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>\link push_front() push_front(const_reference)\endlink</code>,
            <code>pop_back()</code>, <code>pop_front()</code>
    */
    void push_back() {
        value_type temp;
        push_back(boost::move(temp));
    }

    //! Insert a new element at the beginning of the <code>circular_buffer</code>.
    /*!
        \post if <code>capacity() > 0</code> then <code>front() == item</code><br>
              If the <code>circular_buffer</code> is full, the last element will be removed. If the capacity is
              <code>0</code>, nothing will be inserted.
        \param item The element to be inserted.
        \throws Whatever <code>T::T(const T&)</code> throws.
                Whatever <code>T::operator = (const T&)</code> throws.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Does not invalidate any iterators with the exception of iterators pointing to the overwritten element.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>\link push_back() push_back(const_reference)\endlink</code>,
            <code>pop_back()</code>, <code>pop_front()</code>
    */
    void push_front(param_value_type item) {
        push_front_impl<param_value_type>(item);
    }

    //! Insert a new element at the beginning of the <code>circular_buffer</code> using rvalue references or rvalues references emulation.
    /*!
        \post if <code>capacity() > 0</code> then <code>front() == item</code><br>
              If the <code>circular_buffer</code> is full, the last element will be removed. If the capacity is
              <code>0</code>, nothing will be inserted.
        \param item The element to be inserted.
        \throws Whatever <code>T::T(T&&)</code> throws.
                Whatever <code>T::operator = (T&&)</code> throws.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Does not invalidate any iterators with the exception of iterators pointing to the overwritten element.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>\link push_back() push_back(const_reference)\endlink</code>,
            <code>pop_back()</code>, <code>pop_front()</code>
    */
    void push_front(rvalue_type item) {
        push_front_impl<rvalue_type>(boost::move(item));
    }

    //! Insert a new default-constructed element at the beginning of the <code>circular_buffer</code>.
    /*!
        \post if <code>capacity() > 0</code> then <code>front() == item</code><br>
              If the <code>circular_buffer</code> is full, the last element will be removed. If the capacity is
              <code>0</code>, nothing will be inserted.
        \throws Whatever <code>T::T()</code> throws.
                Whatever <code>T::T(T&&)</code> throws.
                Whatever <code>T::operator = (T&&)</code> throws.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Does not invalidate any iterators with the exception of iterators pointing to the overwritten element.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>\link push_back() push_back(const_reference)\endlink</code>,
            <code>pop_back()</code>, <code>pop_front()</code>
    */
    void push_front() {
        value_type temp;
        push_front(boost::move(temp));
    }

    //! Remove the last element from the <code>circular_buffer</code>.
    /*!
        \pre <code>!empty()</code>
        \post The last element is removed from the <code>circular_buffer</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Invalidates only iterators pointing to the removed element.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>pop_front()</code>, <code>\link push_back() push_back(const_reference)\endlink</code>,
            <code>\link push_front() push_front(const_reference)\endlink</code>
    */
    void pop_back() {
        BOOST_CB_ASSERT(!empty()); // check for empty buffer (back element not available)
        decrement(m_last);
        destroy_item(m_last);
        --m_size;
    }

    //! Remove the first element from the <code>circular_buffer</code>.
    /*!
        \pre <code>!empty()</code>
        \post The first element is removed from the <code>circular_buffer</code>.
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Invalidates only iterators pointing to the removed element.
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>).
        \sa <code>pop_back()</code>, <code>\link push_back() push_back(const_reference)\endlink</code>,
            <code>\link push_front() push_front(const_reference)\endlink</code>
    */
    void pop_front() {
        BOOST_CB_ASSERT(!empty()); // check for empty buffer (front element not available)
        destroy_item(m_first);
        increment(m_first);
        --m_size;
    }
private:
    /*! INTERNAL ONLY */
    template <class ValT>
    iterator insert_impl(iterator pos, ValT item) {
        BOOST_CB_ASSERT(pos.is_valid(this)); // check for uninitialized or invalidated iterator
        iterator b = begin();
        if (full() && pos == b)
            return b;
        return insert_item<ValT>(pos, static_cast<ValT>(item));
    }

public:
// Insert

    //! Insert an element at the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> or its end.
        \post The <code>item</code> will be inserted at the position <code>pos</code>.<br>
              If the <code>circular_buffer</code> is full, the first element will be overwritten. If the
              <code>circular_buffer</code> is full and the <code>pos</code> points to <code>begin()</code>, then the
              <code>item</code> will not be inserted. If the capacity is <code>0</code>, nothing will be inserted.
        \param pos An iterator specifying the position where the <code>item</code> will be inserted.
        \param item The element to be inserted.
        \return Iterator to the inserted element or <code>begin()</code> if the <code>item</code> is not inserted. (See
                the <i>Effect</i>.)
        \throws Whatever <code>T::T(const T&)</code> throws.
                Whatever <code>T::operator = (const T&)</code> throws.
                <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
         
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the elements at the insertion point (including <code>pos</code>) and
             iterators behind the insertion point (towards the end; except iterators equal to <code>end()</code>). It
             also invalidates iterators pointing to the overwritten element.
        \par Complexity
             Linear (in <code>std::distance(pos, end())</code>).
        \sa <code>\link insert(iterator, size_type, param_value_type)
            insert(iterator, size_type, value_type)\endlink</code>,
            <code>insert(iterator, InputIterator, InputIterator)</code>,
            <code>\link rinsert(iterator, param_value_type) rinsert(iterator, value_type)\endlink</code>,
            <code>\link rinsert(iterator, size_type, param_value_type)
            rinsert(iterator, size_type, value_type)\endlink</code>,
            <code>rinsert(iterator, InputIterator, InputIterator)</code>
    */
    iterator insert(iterator pos, param_value_type item) {
        return insert_impl<param_value_type>(pos, item);
    }

    //! Insert an element at the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> or its end.
        \post The <code>item</code> will be inserted at the position <code>pos</code>.<br>
              If the <code>circular_buffer</code> is full, the first element will be overwritten. If the
              <code>circular_buffer</code> is full and the <code>pos</code> points to <code>begin()</code>, then the
              <code>item</code> will not be inserted. If the capacity is <code>0</code>, nothing will be inserted.
        \param pos An iterator specifying the position where the <code>item</code> will be inserted.
        \param item The element to be inserted.
        \return Iterator to the inserted element or <code>begin()</code> if the <code>item</code> is not inserted. (See
                the <i>Effect</i>.)
        \throws Whatever <code>T::T(T&&)</code> throws.
                Whatever <code>T::operator = (T&&)</code> throws.
                <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the elements at the insertion point (including <code>pos</code>) and
             iterators behind the insertion point (towards the end; except iterators equal to <code>end()</code>). It
             also invalidates iterators pointing to the overwritten element.
        \par Complexity
             Linear (in <code>std::distance(pos, end())</code>).
        \sa <code>\link insert(iterator, size_type, param_value_type)
            insert(iterator, size_type, value_type)\endlink</code>,
            <code>insert(iterator, InputIterator, InputIterator)</code>,
            <code>\link rinsert(iterator, param_value_type) rinsert(iterator, value_type)\endlink</code>,
            <code>\link rinsert(iterator, size_type, param_value_type)
            rinsert(iterator, size_type, value_type)\endlink</code>,
            <code>rinsert(iterator, InputIterator, InputIterator)</code>
    */
    iterator insert(iterator pos, rvalue_type item) {
        return insert_impl<rvalue_type>(pos, boost::move(item));
    }

    //! Insert a default-constructed element at the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> or its end.
        \post The <code>item</code> will be inserted at the position <code>pos</code>.<br>
              If the <code>circular_buffer</code> is full, the first element will be overwritten. If the
              <code>circular_buffer</code> is full and the <code>pos</code> points to <code>begin()</code>, then the
              <code>item</code> will not be inserted. If the capacity is <code>0</code>, nothing will be inserted.
        \param pos An iterator specifying the position where the <code>item</code> will be inserted.
        \return Iterator to the inserted element or <code>begin()</code> if the <code>item</code> is not inserted. (See
                the <i>Effect</i>.)
        \throws Whatever <code>T::T()</code> throws.
                Whatever <code>T::T(T&&)</code> throws.
                Whatever <code>T::operator = (T&&)</code> throws.
                <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the elements at the insertion point (including <code>pos</code>) and
             iterators behind the insertion point (towards the end; except iterators equal to <code>end()</code>). It
             also invalidates iterators pointing to the overwritten element.
        \par Complexity
             Linear (in <code>std::distance(pos, end())</code>).
        \sa <code>\link insert(iterator, size_type, param_value_type)
            insert(iterator, size_type, value_type)\endlink</code>,
            <code>insert(iterator, InputIterator, InputIterator)</code>,
            <code>\link rinsert(iterator, param_value_type) rinsert(iterator, value_type)\endlink</code>,
            <code>\link rinsert(iterator, size_type, param_value_type)
            rinsert(iterator, size_type, value_type)\endlink</code>,
            <code>rinsert(iterator, InputIterator, InputIterator)</code>
    */
    iterator insert(iterator pos) {
        value_type temp;
        return insert(pos, boost::move(temp));
    }

    //! Insert <code>n</code> copies of the <code>item</code> at the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> or its end.
        \post The number of <code>min[n, (pos - begin()) + reserve()]</code> elements will be inserted at the position
              <code>pos</code>.<br>The number of <code>min[pos - begin(), max[0, n - reserve()]]</code> elements will
              be overwritten at the beginning of the <code>circular_buffer</code>.<br>(See <i>Example</i> for the
              explanation.)
        \param pos An iterator specifying the position where the <code>item</code>s will be inserted.
        \param n The number of <code>item</code>s the to be inserted.
        \param item The element whose copies will be inserted.
        \throws Whatever <code>T::T(const T&)</code> throws.
                Whatever <code>T::operator = (const T&)</code> throws.
                <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operations in the <i>Throws</i> section do not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the elements at the insertion point (including <code>pos</code>) and
             iterators behind the insertion point (towards the end; except iterators equal to <code>end()</code>). It
             also invalidates iterators pointing to the overwritten elements.
        \par Complexity
             Linear (in <code>min[capacity(), std::distance(pos, end()) + n]</code>).
        \par Example
             Consider a <code>circular_buffer</code> with the capacity of 6 and the size of 4. Its internal buffer may
             look like the one below.<br><br>
             <code>|1|2|3|4| | |</code><br>
             <code>p ___^</code><br><br>After inserting 5 elements at the position <code>p</code>:<br><br>
             <code>insert(p, (size_t)5, 0);</code><br><br>actually only 4 elements get inserted and elements
             <code>1</code> and <code>2</code> are overwritten. This is due to the fact the insert operation preserves
             the capacity. After insertion the internal buffer looks like this:<br><br><code>|0|0|0|0|3|4|</code><br>
             <br>For comparison if the capacity would not be preserved the internal buffer would then result in
             <code>|1|2|0|0|0|0|0|3|4|</code>.
        \sa <code>\link insert(iterator, param_value_type) insert(iterator, value_type)\endlink</code>,
            <code>insert(iterator, InputIterator, InputIterator)</code>,
            <code>\link rinsert(iterator, param_value_type) rinsert(iterator, value_type)\endlink</code>,
            <code>\link rinsert(iterator, size_type, param_value_type)
            rinsert(iterator, size_type, value_type)\endlink</code>,
            <code>rinsert(iterator, InputIterator, InputIterator)</code>
    */
    void insert(iterator pos, size_type n, param_value_type item) {
        BOOST_CB_ASSERT(pos.is_valid(this)); // check for uninitialized or invalidated iterator
        if (n == 0)
            return;
        size_type copy = capacity() - (end() - pos);
        if (copy == 0)
            return;
        if (n > copy)
            n = copy;
        insert_n(pos, n, cb_details::item_wrapper<const_pointer, param_value_type>(item));
    }

    //! Insert the range <code>[first, last)</code> at the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> or its end.<br>
             Valid range <code>[first, last)</code> where <code>first</code> and <code>last</code> meet the
             requirements of an <a href="https://www.boost.org/sgi/stl/InputIterator.html">InputIterator</a>.
        \post Elements from the range
              <code>[first + max[0, distance(first, last) - (pos - begin()) - reserve()], last)</code> will be
              inserted at the position <code>pos</code>.<br>The number of <code>min[pos - begin(), max[0,
              distance(first, last) - reserve()]]</code> elements will be overwritten at the beginning of the
              <code>circular_buffer</code>.<br>(See <i>Example</i> for the explanation.)
        \param pos An iterator specifying the position where the range will be inserted.
        \param first The beginning of the range to be inserted.
        \param last The end of the range to be inserted.
        \throws Whatever <code>T::T(const T&)</code> throws if the <code>InputIterator</code> is not a move iterator.
                Whatever <code>T::operator = (const T&)</code> throws if the <code>InputIterator</code> is not a move iterator.
                Whatever <code>T::T(T&&)</code> throws if the <code>InputIterator</code> is a move iterator.
                Whatever <code>T::operator = (T&&)</code> throws if the <code>InputIterator</code> is a move iterator.
        \par Exception Safety
             Basic; no-throw if the operations in the <i>Throws</i> section do not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the elements at the insertion point (including <code>pos</code>) and
             iterators behind the insertion point (towards the end; except iterators equal to <code>end()</code>). It
             also invalidates iterators pointing to the overwritten elements.
        \par Complexity
             Linear (in <code>[std::distance(pos, end()) + std::distance(first, last)]</code>; in
             <code>min[capacity(), std::distance(pos, end()) + std::distance(first, last)]</code> if the
             <code>InputIterator</code> is a
             <a href="https://www.boost.org/sgi/stl/RandomAccessIterator.html">RandomAccessIterator</a>).
        \par Example
             Consider a <code>circular_buffer</code> with the capacity of 6 and the size of 4. Its internal buffer may
             look like the one below.<br><br>
             <code>|1|2|3|4| | |</code><br>
             <code>p ___^</code><br><br>After inserting a range of elements at the position <code>p</code>:<br><br>
             <code>int array[] = { 5, 6, 7, 8, 9 };</code><br><code>insert(p, array, array + 5);</code><br><br>
             actually only elements <code>6</code>, <code>7</code>, <code>8</code> and <code>9</code> from the
             specified range get inserted and elements <code>1</code> and <code>2</code> are overwritten. This is due
             to the fact the insert operation preserves the capacity. After insertion the internal buffer looks like
             this:<br><br><code>|6|7|8|9|3|4|</code><br><br>For comparison if the capacity would not be preserved the
             internal buffer would then result in <code>|1|2|5|6|7|8|9|3|4|</code>.
        \sa <code>\link insert(iterator, param_value_type) insert(iterator, value_type)\endlink</code>,
            <code>\link insert(iterator, size_type, param_value_type)
            insert(iterator, size_type, value_type)\endlink</code>, <code>\link rinsert(iterator, param_value_type)
            rinsert(iterator, value_type)\endlink</code>, <code>\link rinsert(iterator, size_type, param_value_type)
            rinsert(iterator, size_type, value_type)\endlink</code>,
            <code>rinsert(iterator, InputIterator, InputIterator)</code>
    */
    template <class InputIterator>
    void insert(iterator pos, InputIterator first, InputIterator last) {
        BOOST_CB_ASSERT(pos.is_valid(this)); // check for uninitialized or invalidated iterator
        insert(pos, first, last, is_integral<InputIterator>());
    }

private:
    /*! INTERNAL ONLY */
    template <class ValT>
    iterator rinsert_impl(iterator pos, ValT item) {
        BOOST_CB_ASSERT(pos.is_valid(this)); // check for uninitialized or invalidated iterator
        if (full() && pos.m_it == 0)
            return end();
        if (pos == begin()) {
            BOOST_TRY {
                decrement(m_first);
                construct_or_replace(!full(), m_first, static_cast<ValT>(item));
            } BOOST_CATCH(...) {
                increment(m_first);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
            pos.m_it = m_first;
        } else {
            pointer src = m_first;
            pointer dest = m_first;
            decrement(dest);
            pos.m_it = map_pointer(pos.m_it);
            bool construct = !full();
            BOOST_TRY {
                while (src != pos.m_it) {
                    construct_or_replace(construct, dest, boost::move_if_noexcept(*src));
                    increment(src);
                    increment(dest);
                    construct = false;
                }
                decrement(pos.m_it);
                replace(pos.m_it, static_cast<ValT>(item));
            } BOOST_CATCH(...) {
                if (!construct && !full()) {
                    decrement(m_first);
                    ++m_size;
                }
                BOOST_RETHROW
            }
            BOOST_CATCH_END
            decrement(m_first);
        }
        if (full())
            m_last = m_first;
        else
            ++m_size;
        return iterator(this, pos.m_it);
    }

public:
   
    //! Insert an element before the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> or its end.
        \post The <code>item</code> will be inserted before the position <code>pos</code>.<br>
              If the <code>circular_buffer</code> is full, the last element will be overwritten. If the
              <code>circular_buffer</code> is full and the <code>pos</code> points to <code>end()</code>, then the
              <code>item</code> will not be inserted. If the capacity is <code>0</code>, nothing will be inserted.
        \param pos An iterator specifying the position before which the <code>item</code> will be inserted.
        \param item The element to be inserted.
        \return Iterator to the inserted element or <code>end()</code> if the <code>item</code> is not inserted. (See
                the <i>Effect</i>.)
        \throws Whatever <code>T::T(const T&)</code> throws.
                Whatever <code>T::operator = (const T&)</code> throws.
                <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operations in the <i>Throws</i> section do not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the elements before the insertion point (towards the beginning and
             excluding <code>pos</code>). It also invalidates iterators pointing to the overwritten element.
        \par Complexity
             Linear (in <code>std::distance(begin(), pos)</code>).
        \sa <code>\link rinsert(iterator, size_type, param_value_type)
            rinsert(iterator, size_type, value_type)\endlink</code>,
            <code>rinsert(iterator, InputIterator, InputIterator)</code>,
            <code>\link insert(iterator, param_value_type) insert(iterator, value_type)\endlink</code>,
            <code>\link insert(iterator, size_type, param_value_type)
            insert(iterator, size_type, value_type)\endlink</code>,
            <code>insert(iterator, InputIterator, InputIterator)</code>
    */
    iterator rinsert(iterator pos, param_value_type item) {
        return rinsert_impl<param_value_type>(pos, item);
    }

    //! Insert an element before the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> or its end.
        \post The <code>item</code> will be inserted before the position <code>pos</code>.<br>
              If the <code>circular_buffer</code> is full, the last element will be overwritten. If the
              <code>circular_buffer</code> is full and the <code>pos</code> points to <code>end()</code>, then the
              <code>item</code> will not be inserted. If the capacity is <code>0</code>, nothing will be inserted.
        \param pos An iterator specifying the position before which the <code>item</code> will be inserted.
        \param item The element to be inserted.
        \return Iterator to the inserted element or <code>end()</code> if the <code>item</code> is not inserted. (See
                the <i>Effect</i>.)
        \throws Whatever <code>T::T(T&&)</code> throws.
                Whatever <code>T::operator = (T&&)</code> throws.
                <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operations in the <i>Throws</i> section do not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the elements before the insertion point (towards the beginning and
             excluding <code>pos</code>). It also invalidates iterators pointing to the overwritten element.
        \par Complexity
             Linear (in <code>std::distance(begin(), pos)</code>).
        \sa <code>\link rinsert(iterator, size_type, param_value_type)
            rinsert(iterator, size_type, value_type)\endlink</code>,
            <code>rinsert(iterator, InputIterator, InputIterator)</code>,
            <code>\link insert(iterator, param_value_type) insert(iterator, value_type)\endlink</code>,
            <code>\link insert(iterator, size_type, param_value_type)
            insert(iterator, size_type, value_type)\endlink</code>,
            <code>insert(iterator, InputIterator, InputIterator)</code>
    */
    iterator rinsert(iterator pos, rvalue_type item) {
        return rinsert_impl<rvalue_type>(pos, boost::move(item));
    }

    //! Insert an element before the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> or its end.
        \post The <code>item</code> will be inserted before the position <code>pos</code>.<br>
              If the <code>circular_buffer</code> is full, the last element will be overwritten. If the
              <code>circular_buffer</code> is full and the <code>pos</code> points to <code>end()</code>, then the
              <code>item</code> will not be inserted. If the capacity is <code>0</code>, nothing will be inserted.
        \param pos An iterator specifying the position before which the <code>item</code> will be inserted.
        \return Iterator to the inserted element or <code>end()</code> if the <code>item</code> is not inserted. (See
                the <i>Effect</i>.)
        \throws Whatever <code>T::T()</code> throws.
                Whatever <code>T::T(T&&)</code> throws.
                Whatever <code>T::operator = (T&&)</code> throws.
                <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operations in the <i>Throws</i> section do not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the elements before the insertion point (towards the beginning and
             excluding <code>pos</code>). It also invalidates iterators pointing to the overwritten element.
        \par Complexity
             Linear (in <code>std::distance(begin(), pos)</code>).
        \sa <code>\link rinsert(iterator, size_type, param_value_type)
            rinsert(iterator, size_type, value_type)\endlink</code>,
            <code>rinsert(iterator, InputIterator, InputIterator)</code>,
            <code>\link insert(iterator, param_value_type) insert(iterator, value_type)\endlink</code>,
            <code>\link insert(iterator, size_type, param_value_type)
            insert(iterator, size_type, value_type)\endlink</code>,
            <code>insert(iterator, InputIterator, InputIterator)</code>
    */
    iterator rinsert(iterator pos) {
        value_type temp;
        return rinsert(pos, boost::move(temp));
    }

    //! Insert <code>n</code> copies of the <code>item</code> before the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> or its end.
        \post The number of <code>min[n, (end() - pos) + reserve()]</code> elements will be inserted before the
              position <code>pos</code>.<br>The number of <code>min[end() - pos, max[0, n - reserve()]]</code> elements
              will be overwritten at the end of the <code>circular_buffer</code>.<br>(See <i>Example</i> for the
              explanation.)
        \param pos An iterator specifying the position where the <code>item</code>s will be inserted.
        \param n The number of <code>item</code>s the to be inserted.
        \param item The element whose copies will be inserted.
        \throws Whatever <code>T::T(const T&)</code> throws.
                Whatever <code>T::operator = (const T&)</code> throws.
                <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operations in the <i>Throws</i> section do not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the elements before the insertion point (towards the beginning and
             excluding <code>pos</code>). It also invalidates iterators pointing to the overwritten elements.
        \par Complexity
             Linear (in <code>min[capacity(), std::distance(begin(), pos) + n]</code>).
        \par Example
             Consider a <code>circular_buffer</code> with the capacity of 6 and the size of 4. Its internal buffer may
             look like the one below.<br><br>
             <code>|1|2|3|4| | |</code><br>
             <code>p ___^</code><br><br>After inserting 5 elements before the position <code>p</code>:<br><br>
             <code>rinsert(p, (size_t)5, 0);</code><br><br>actually only 4 elements get inserted and elements
             <code>3</code> and <code>4</code> are overwritten. This is due to the fact the rinsert operation preserves
             the capacity. After insertion the internal buffer looks like this:<br><br><code>|1|2|0|0|0|0|</code><br>
             <br>For comparison if the capacity would not be preserved the internal buffer would then result in
             <code>|1|2|0|0|0|0|0|3|4|</code>.
        \sa <code>\link rinsert(iterator, param_value_type) rinsert(iterator, value_type)\endlink</code>,
            <code>rinsert(iterator, InputIterator, InputIterator)</code>,
            <code>\link insert(iterator, param_value_type) insert(iterator, value_type)\endlink</code>,
            <code>\link insert(iterator, size_type, param_value_type)
            insert(iterator, size_type, value_type)\endlink</code>,
            <code>insert(iterator, InputIterator, InputIterator)</code>
    */
    void rinsert(iterator pos, size_type n, param_value_type item) {
        BOOST_CB_ASSERT(pos.is_valid(this)); // check for uninitialized or invalidated iterator
        rinsert_n(pos, n, cb_details::item_wrapper<const_pointer, param_value_type>(item));
    }

    //! Insert the range <code>[first, last)</code> before the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> or its end.<br>
             Valid range <code>[first, last)</code> where <code>first</code> and <code>last</code> meet the
             requirements of an <a href="https://www.boost.org/sgi/stl/InputIterator.html">InputIterator</a>.
        \post Elements from the range
              <code>[first, last - max[0, distance(first, last) - (end() - pos) - reserve()])</code> will be inserted
              before the position <code>pos</code>.<br>The number of <code>min[end() - pos, max[0,
              distance(first, last) - reserve()]]</code> elements will be overwritten at the end of the
              <code>circular_buffer</code>.<br>(See <i>Example</i> for the explanation.)
        \param pos An iterator specifying the position where the range will be inserted.
        \param first The beginning of the range to be inserted.
        \param last The end of the range to be inserted.
        \throws Whatever <code>T::T(const T&)</code> throws if the <code>InputIterator</code> is not a move iterator.
                Whatever <code>T::operator = (const T&)</code> throws if the <code>InputIterator</code> is not a move iterator.
                Whatever <code>T::T(T&&)</code> throws if the <code>InputIterator</code> is a move iterator.
                Whatever <code>T::operator = (T&&)</code> throws if the <code>InputIterator</code> is a move iterator.
        \par Exception Safety
             Basic; no-throw if the operations in the <i>Throws</i> section do not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the elements before the insertion point (towards the beginning and
             excluding <code>pos</code>). It also invalidates iterators pointing to the overwritten elements.
        \par Complexity
             Linear (in <code>[std::distance(begin(), pos) + std::distance(first, last)]</code>; in
             <code>min[capacity(), std::distance(begin(), pos) + std::distance(first, last)]</code> if the
             <code>InputIterator</code> is a
             <a href="https://www.boost.org/sgi/stl/RandomAccessIterator.html">RandomAccessIterator</a>).
        \par Example
             Consider a <code>circular_buffer</code> with the capacity of 6 and the size of 4. Its internal buffer may
             look like the one below.<br><br>
             <code>|1|2|3|4| | |</code><br>
             <code>p ___^</code><br><br>After inserting a range of elements before the position <code>p</code>:<br><br>
             <code>int array[] = { 5, 6, 7, 8, 9 };</code><br><code>insert(p, array, array + 5);</code><br><br>
             actually only elements <code>5</code>, <code>6</code>, <code>7</code> and <code>8</code> from the
             specified range get inserted and elements <code>3</code> and <code>4</code> are overwritten. This is due
             to the fact the rinsert operation preserves the capacity. After insertion the internal buffer looks like
             this:<br><br><code>|1|2|5|6|7|8|</code><br><br>For comparison if the capacity would not be preserved the
             internal buffer would then result in <code>|1|2|5|6|7|8|9|3|4|</code>.
        \sa <code>\link rinsert(iterator, param_value_type) rinsert(iterator, value_type)\endlink</code>,
            <code>\link rinsert(iterator, size_type, param_value_type)
            rinsert(iterator, size_type, value_type)\endlink</code>, <code>\link insert(iterator, param_value_type)
            insert(iterator, value_type)\endlink</code>, <code>\link insert(iterator, size_type, param_value_type)
            insert(iterator, size_type, value_type)\endlink</code>,
            <code>insert(iterator, InputIterator, InputIterator)</code>
    */
    template <class InputIterator>
    void rinsert(iterator pos, InputIterator first, InputIterator last) {
        BOOST_CB_ASSERT(pos.is_valid(this)); // check for uninitialized or invalidated iterator
        rinsert(pos, first, last, is_integral<InputIterator>());
    }

// Erase

    //! Remove an element at the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> (but not an
             <code>end()</code>).
        \post The element at the position <code>pos</code> is removed.
        \param pos An iterator pointing at the element to be removed.
        \return Iterator to the first element remaining beyond the removed element or <code>end()</code> if no such
                element exists.
        \throws <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the erased element and iterators pointing to the elements behind
             the erased element (towards the end; except iterators equal to <code>end()</code>).
        \par Complexity
             Linear (in <code>std::distance(pos, end())</code>).
        \sa <code>erase(iterator, iterator)</code>, <code>rerase(iterator)</code>,
            <code>rerase(iterator, iterator)</code>, <code>erase_begin(size_type)</code>,
            <code>erase_end(size_type)</code>, <code>clear()</code>
    */
    iterator erase(iterator pos) {
        BOOST_CB_ASSERT(pos.is_valid(this)); // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(pos.m_it != 0);      // check for iterator pointing to end()
        pointer next = pos.m_it;
        increment(next);
        for (pointer p = pos.m_it; next != m_last; p = next, increment(next))
            replace(p, boost::move_if_noexcept(*next));
        decrement(m_last);
        destroy_item(m_last);
        --m_size;
#if BOOST_CB_ENABLE_DEBUG
        return m_last == pos.m_it ? end() : iterator(this, pos.m_it);
#else
        return m_last == pos.m_it ? end() : pos;
#endif
    }

    //! Erase the range <code>[first, last)</code>.
    /*!
        \pre Valid range <code>[first, last)</code>.
        \post The elements from the range <code>[first, last)</code> are removed. (If <code>first == last</code>
              nothing is removed.)
        \param first The beginning of the range to be removed.
        \param last The end of the range to be removed.
        \return Iterator to the first element remaining beyond the removed elements or <code>end()</code> if no such
                element exists.
        \throws <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the erased elements and iterators pointing to the elements behind
             the erased range (towards the end; except iterators equal to <code>end()</code>).
        \par Complexity
             Linear (in <code>std::distance(first, end())</code>).
        \sa <code>erase(iterator)</code>, <code>rerase(iterator)</code>, <code>rerase(iterator, iterator)</code>,
            <code>erase_begin(size_type)</code>, <code>erase_end(size_type)</code>, <code>clear()</code>
    */
    iterator erase(iterator first, iterator last) {
        BOOST_CB_ASSERT(first.is_valid(this)); // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(last.is_valid(this));  // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(first <= last);        // check for wrong range
        if (first == last)
            return first;
        pointer p = first.m_it;
        while (last.m_it != 0)
            replace((first++).m_it, boost::move_if_noexcept(*last++));
        do {
            decrement(m_last);
            destroy_item(m_last);
            --m_size;
        } while(m_last != first.m_it);
        return m_last == p ? end() : iterator(this, p);
    }

    //! Remove an element at the specified position.
    /*!
        \pre <code>pos</code> is a valid iterator pointing to the <code>circular_buffer</code> (but not an
             <code>end()</code>).
        \post The element at the position <code>pos</code> is removed.
        \param pos An iterator pointing at the element to be removed.
        \return Iterator to the first element remaining in front of the removed element or <code>begin()</code> if no
                such element exists.
        \throws <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the erased element and iterators pointing to the elements in front of
             the erased element (towards the beginning).
        \par Complexity
             Linear (in <code>std::distance(begin(), pos)</code>).
        \note This method is symmetric to the <code>erase(iterator)</code> method and is more effective than
              <code>erase(iterator)</code> if the iterator <code>pos</code> is close to the beginning of the
              <code>circular_buffer</code>. (See the <i>Complexity</i>.)
        \sa <code>erase(iterator)</code>, <code>erase(iterator, iterator)</code>,
            <code>rerase(iterator, iterator)</code>, <code>erase_begin(size_type)</code>,
            <code>erase_end(size_type)</code>, <code>clear()</code>
    */
    iterator rerase(iterator pos) {
        BOOST_CB_ASSERT(pos.is_valid(this)); // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(pos.m_it != 0);      // check for iterator pointing to end()
        pointer prev = pos.m_it;
        pointer p = prev;
        for (decrement(prev); p != m_first; p = prev, decrement(prev))
            replace(p, boost::move_if_noexcept(*prev));
        destroy_item(m_first);
        increment(m_first);
        --m_size;
#if BOOST_CB_ENABLE_DEBUG
        return p == pos.m_it ? begin() : iterator(this, pos.m_it);
#else
        return p == pos.m_it ? begin() : pos;
#endif
    }

    //! Erase the range <code>[first, last)</code>.
    /*!
        \pre Valid range <code>[first, last)</code>.
        \post The elements from the range <code>[first, last)</code> are removed. (If <code>first == last</code>
              nothing is removed.)
        \param first The beginning of the range to be removed.
        \param last The end of the range to be removed.
        \return Iterator to the first element remaining in front of the removed elements or <code>begin()</code> if no
                such element exists.
        \throws <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything.
        \par Iterator Invalidation
             Invalidates iterators pointing to the erased elements and iterators pointing to the elements in front of
             the erased range (towards the beginning).
        \par Complexity
             Linear (in <code>std::distance(begin(), last)</code>).
        \note This method is symmetric to the <code>erase(iterator, iterator)</code> method and is more effective than
              <code>erase(iterator, iterator)</code> if <code>std::distance(begin(), first)</code> is lower that
              <code>std::distance(last, end())</code>.
        \sa <code>erase(iterator)</code>, <code>erase(iterator, iterator)</code>, <code>rerase(iterator)</code>,
            <code>erase_begin(size_type)</code>, <code>erase_end(size_type)</code>, <code>clear()</code>
    */
    iterator rerase(iterator first, iterator last) {
        BOOST_CB_ASSERT(first.is_valid(this)); // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(last.is_valid(this));  // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(first <= last);        // check for wrong range
        if (first == last)
            return first;
        pointer p = map_pointer(last.m_it);
        last.m_it = p;
        while (first.m_it != m_first) {
            decrement(first.m_it);
            decrement(p);
            replace(p, boost::move_if_noexcept(*first.m_it));
        }
        do {
            destroy_item(m_first);
            increment(m_first);
            --m_size;
        } while(m_first != p);
        if (m_first == last.m_it)
            return begin();
        decrement(last.m_it);
        return iterator(this, last.m_it);
    }

    //! Remove first <code>n</code> elements (with constant complexity for scalar types).
    /*!
        \pre <code>n \<= size()</code>
        \post The <code>n</code> elements at the beginning of the <code>circular_buffer</code> will be removed.
        \param n The number of elements to be removed.
        \throws <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything. (I.e. no throw in
             case of scalars.)
        \par Iterator Invalidation
             Invalidates iterators pointing to the first <code>n</code> erased elements.
        \par Complexity
             Constant (in <code>n</code>) for scalar types; linear for other types.
        \note This method has been specially designed for types which do not require an explicit destructruction (e.g.
              integer, float or a pointer). For these scalar types a call to a destructor is not required which makes
              it possible to implement the "erase from beginning" operation with a constant complexity. For non-sacalar
              types the complexity is linear (hence the explicit destruction is needed) and the implementation is
              actually equivalent to
              <code>\link circular_buffer::rerase(iterator, iterator) rerase(begin(), begin() + n)\endlink</code>.
        \sa <code>erase(iterator)</code>, <code>erase(iterator, iterator)</code>,
            <code>rerase(iterator)</code>, <code>rerase(iterator, iterator)</code>,
            <code>erase_end(size_type)</code>, <code>clear()</code>
    */
    void erase_begin(size_type n) {
        BOOST_CB_ASSERT(n <= size()); // check for n greater than size
#if BOOST_CB_ENABLE_DEBUG
        erase_begin(n, false_type());
#else
        erase_begin(n, is_scalar<value_type>());
#endif
    }

    //! Remove last <code>n</code> elements (with constant complexity for scalar types).
    /*!
        \pre <code>n \<= size()</code>
        \post The <code>n</code> elements at the end of the <code>circular_buffer</code> will be removed.
        \param n The number of elements to be removed.
        \throws <a href="circular_buffer/implementation.html#circular_buffer.implementation.exceptions_of_move_if_noexcept_t">Exceptions of move_if_noexcept(T&)</a>.
        \par Exception Safety
             Basic; no-throw if the operation in the <i>Throws</i> section does not throw anything. (I.e. no throw in
             case of scalars.)
        \par Iterator Invalidation
             Invalidates iterators pointing to the last <code>n</code> erased elements.
        \par Complexity
             Constant (in <code>n</code>) for scalar types; linear for other types.
        \note This method has been specially designed for types which do not require an explicit destructruction (e.g.
              integer, float or a pointer). For these scalar types a call to a destructor is not required which makes
              it possible to implement the "erase from end" operation with a constant complexity. For non-sacalar
              types the complexity is linear (hence the explicit destruction is needed) and the implementation is
              actually equivalent to
              <code>\link circular_buffer::erase(iterator, iterator) erase(end() - n, end())\endlink</code>.
        \sa <code>erase(iterator)</code>, <code>erase(iterator, iterator)</code>,
            <code>rerase(iterator)</code>, <code>rerase(iterator, iterator)</code>,
            <code>erase_begin(size_type)</code>, <code>clear()</code>
    */
    void erase_end(size_type n) {
        BOOST_CB_ASSERT(n <= size()); // check for n greater than size
#if BOOST_CB_ENABLE_DEBUG
        erase_end(n, false_type());
#else
        erase_end(n, is_scalar<value_type>());
#endif
    }

    //! Remove all stored elements from the <code>circular_buffer</code>.
    /*!
        \post <code>size() == 0</code>
        \throws Nothing.
        \par Exception Safety
             No-throw.
        \par Iterator Invalidation
             Invalidates all iterators pointing to the <code>circular_buffer</code> (except iterators equal to
             <code>end()</code>).
        \par Complexity
             Constant (in the size of the <code>circular_buffer</code>) for scalar types; linear for other types.
        \sa <code>~circular_buffer()</code>, <code>erase(iterator)</code>, <code>erase(iterator, iterator)</code>,
            <code>rerase(iterator)</code>, <code>rerase(iterator, iterator)</code>,
            <code>erase_begin(size_type)</code>, <code>erase_end(size_type)</code>
    */
    void clear() BOOST_NOEXCEPT {
        destroy_content();
        m_size = 0;
    }

private:
// Helper methods

    /*! INTERNAL ONLY */
    void check_position(size_type index) const {
        if (index >= size())
            throw_exception(std::out_of_range("circular_buffer"));
    }

    /*! INTERNAL ONLY */
    template <class Pointer>
    void increment(Pointer& p) const {
        if (++p == m_end)
            p = m_buff;
    }

    /*! INTERNAL ONLY */
    template <class Pointer>
    void decrement(Pointer& p) const {
        if (p == m_buff)
            p = m_end;
        --p;
    }

    /*! INTERNAL ONLY */
    template <class Pointer>
    Pointer add(Pointer p, difference_type n) const {
        return p + (n < (m_end - p) ? n : n - (m_end - m_buff));
    }

    /*! INTERNAL ONLY */
    template <class Pointer>
    Pointer sub(Pointer p, difference_type n) const {
        return p - (n > (p - m_buff) ? n - (m_end - m_buff) : n);
    }

    /*! INTERNAL ONLY */
    pointer map_pointer(pointer p) const { return p == 0 ? m_last : p; }

    /*! INTERNAL ONLY */
    const Alloc& alloc() const {
        return base::get();
    }

    /*! INTERNAL ONLY */
    Alloc& alloc() {
        return base::get();
    }

    /*! INTERNAL ONLY */
    pointer allocate(size_type n) {
        if (n > max_size())
            throw_exception(std::length_error("circular_buffer"));
#if BOOST_CB_ENABLE_DEBUG
        pointer p = (n == 0) ? 0 : alloc().allocate(n);
        cb_details::do_fill_uninitialized_memory(p, sizeof(value_type) * n);
        return p;
#else
        return (n == 0) ? 0 : alloc().allocate(n);
#endif
    }

    /*! INTERNAL ONLY */
    void deallocate(pointer p, size_type n) {
        if (p != 0)
            alloc().deallocate(p, n);
    }

    /*! INTERNAL ONLY */
    bool is_uninitialized(const_pointer p) const BOOST_NOEXCEPT {
        return (m_first < m_last)
            ? (p >= m_last || p < m_first)
            : (p >= m_last && p < m_first);
    }

    /*! INTERNAL ONLY */
    void replace(pointer pos, param_value_type item) {
        *pos = item;
#if BOOST_CB_ENABLE_DEBUG
        invalidate_iterators(iterator(this, pos));
#endif
    }

    /*! INTERNAL ONLY */
    void replace(pointer pos, rvalue_type item) {
        *pos = boost::move(item);
#if BOOST_CB_ENABLE_DEBUG
        invalidate_iterators(iterator(this, pos));
#endif
    }

    /*! INTERNAL ONLY */
    void construct_or_replace(bool construct, pointer pos, param_value_type item) {
        if (construct)
            boost::allocator_construct(alloc(), boost::to_address(pos), item);
        else
            replace(pos, item);
    }

    /*! INTERNAL ONLY */
    void construct_or_replace(bool construct, pointer pos, rvalue_type item) {
        if (construct)
            boost::allocator_construct(alloc(), boost::to_address(pos), boost::move(item));
        else
            replace(pos, boost::move(item));
    }

    /*! INTERNAL ONLY */
    void destroy_item(pointer p) {
        boost::allocator_destroy(alloc(), boost::to_address(p));
#if BOOST_CB_ENABLE_DEBUG
        invalidate_iterators(iterator(this, p));
        cb_details::do_fill_uninitialized_memory(p, sizeof(value_type));
#endif
    }

    /*! INTERNAL ONLY */
    void destroy_if_constructed(pointer pos) {
        if (is_uninitialized(pos))
            destroy_item(pos);
    }

    /*! INTERNAL ONLY */
    void destroy_content() {
#if BOOST_CB_ENABLE_DEBUG
        destroy_content(false_type());
#else
        destroy_content(is_scalar<value_type>());
#endif
    }

    /*! INTERNAL ONLY */
    void destroy_content(const true_type&) {
        m_first = add(m_first, size());
    }

    /*! INTERNAL ONLY */
    void destroy_content(const false_type&) {
        for (size_type ii = 0; ii < size(); ++ii, increment(m_first))
            destroy_item(m_first);
    }

    /*! INTERNAL ONLY */
    void destroy() BOOST_NOEXCEPT {
        destroy_content();
        deallocate(m_buff, capacity());
#if BOOST_CB_ENABLE_DEBUG
        m_buff = 0;
        m_first = 0;
        m_last = 0;
        m_end = 0;
#endif
    }

    /*! INTERNAL ONLY */
    void initialize_buffer(capacity_type buffer_capacity) {
        m_buff = allocate(buffer_capacity);
        m_end = m_buff + buffer_capacity;
    }

    /*! INTERNAL ONLY */
    void initialize_buffer(capacity_type buffer_capacity, param_value_type item) {
        initialize_buffer(buffer_capacity);
        BOOST_TRY {
            cb_details::uninitialized_fill_n_with_alloc(m_buff, size(), item, alloc());
        } BOOST_CATCH(...) {
            deallocate(m_buff, size());
            BOOST_RETHROW
        }
        BOOST_CATCH_END
    }

    /*! INTERNAL ONLY */
    template <class IntegralType>
    void initialize(IntegralType n, IntegralType item, const true_type&) {
        m_size = static_cast<size_type>(n);
        initialize_buffer(size(), item);
        m_first = m_last = m_buff;
    }

    /*! INTERNAL ONLY */
    template <class Iterator>
    void initialize(Iterator first, Iterator last, const false_type&) {
        BOOST_CB_IS_CONVERTIBLE(Iterator, value_type); // check for invalid iterator type
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x581))
        initialize(first, last, std::iterator_traits<Iterator>::iterator_category());
#else
        initialize(first, last, BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::iterator_category());
#endif
    }

    /*! INTERNAL ONLY */
    template <class InputIterator>
    void initialize(InputIterator first, InputIterator last, const std::input_iterator_tag&) {
        BOOST_CB_ASSERT_TEMPLATED_ITERATOR_CONSTRUCTORS // check if the STL provides templated iterator constructors
                                                        // for containers
        std::deque<value_type, allocator_type> tmp(first, last, alloc());
        size_type distance = tmp.size();
        initialize(distance, boost::make_move_iterator(tmp.begin()), boost::make_move_iterator(tmp.end()), distance);
    }

    /*! INTERNAL ONLY */
    template <class ForwardIterator>
    void initialize(ForwardIterator first, ForwardIterator last, const std::forward_iterator_tag&) {
        BOOST_CB_ASSERT(std::distance(first, last) >= 0); // check for wrong range
        size_type distance = std::distance(first, last);
        initialize(distance, first, last, distance);
    }

    /*! INTERNAL ONLY */
    template <class IntegralType>
    void initialize(capacity_type buffer_capacity, IntegralType n, IntegralType item, const true_type&) {
        BOOST_CB_ASSERT(buffer_capacity >= static_cast<size_type>(n)); // check for capacity lower than n
        m_size = static_cast<size_type>(n);
        initialize_buffer(buffer_capacity, item);
        m_first = m_buff;
        m_last = buffer_capacity == size() ? m_buff : m_buff + size();
    }

    /*! INTERNAL ONLY */
    template <class Iterator>
    void initialize(capacity_type buffer_capacity, Iterator first, Iterator last, const false_type&) {
        BOOST_CB_IS_CONVERTIBLE(Iterator, value_type); // check for invalid iterator type
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x581))
        initialize(buffer_capacity, first, last, std::iterator_traits<Iterator>::iterator_category());
#else
        initialize(buffer_capacity, first, last, BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::iterator_category());
#endif
    }

    /*! INTERNAL ONLY */
    template <class InputIterator>
    void initialize(capacity_type buffer_capacity,
        InputIterator first,
        InputIterator last,
        const std::input_iterator_tag&) {
        initialize_buffer(buffer_capacity);
        m_first = m_last = m_buff;
        m_size = 0;
        if (buffer_capacity == 0)
            return;
        while (first != last && !full()) {
            boost::allocator_construct(alloc(), boost::to_address(m_last), *first++);
            increment(m_last);
            ++m_size;
        }
        while (first != last) {
            replace(m_last, *first++);
            increment(m_last);
            m_first = m_last;
        }
    }

    /*! INTERNAL ONLY */
    template <class ForwardIterator>
    void initialize(capacity_type buffer_capacity,
        ForwardIterator first,
        ForwardIterator last,
        const std::forward_iterator_tag&) {
        BOOST_CB_ASSERT(std::distance(first, last) >= 0); // check for wrong range
        initialize(buffer_capacity, first, last, std::distance(first, last));
    }

    /*! INTERNAL ONLY */
    template <class ForwardIterator>
    void initialize(capacity_type buffer_capacity,
        ForwardIterator first,
        ForwardIterator last,
        size_type distance) {
        initialize_buffer(buffer_capacity);
        m_first = m_buff;
        if (distance > buffer_capacity) {
            std::advance(first, distance - buffer_capacity);
            m_size = buffer_capacity;
        } else {
            m_size = distance;
        }
        BOOST_TRY {
            m_last = cb_details::uninitialized_copy(first, last, m_buff, alloc());
        } BOOST_CATCH(...) {
            deallocate(m_buff, buffer_capacity);
            BOOST_RETHROW
        }
        BOOST_CATCH_END
        if (m_last == m_end)
            m_last = m_buff;
    }

    /*! INTERNAL ONLY */
    void reset(pointer buff, pointer last, capacity_type new_capacity) {
        destroy();
        m_size = last - buff;
        m_first = m_buff = buff;
        m_end = m_buff + new_capacity;
        m_last = last == m_end ? m_buff : last;
    }

    /*! INTERNAL ONLY */
    void swap_allocator(circular_buffer<T, Alloc>&, const true_type&) {
        // Swap is not needed because allocators have no state.
    }

    /*! INTERNAL ONLY */
    void swap_allocator(circular_buffer<T, Alloc>& cb, const false_type&) {
        adl_move_swap(alloc(), cb.alloc());
    }

    /*! INTERNAL ONLY */
    template <class IntegralType>
    void assign(IntegralType n, IntegralType item, const true_type&) {
        assign(static_cast<size_type>(n), static_cast<value_type>(item));
    }

    /*! INTERNAL ONLY */
    template <class Iterator>
    void assign(Iterator first, Iterator last, const false_type&) {
        BOOST_CB_IS_CONVERTIBLE(Iterator, value_type); // check for invalid iterator type
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x581))
        assign(first, last, std::iterator_traits<Iterator>::iterator_category());
#else
        assign(first, last, BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::iterator_category());
#endif
    }

    /*! INTERNAL ONLY */
    template <class InputIterator>
    void assign(InputIterator first, InputIterator last, const std::input_iterator_tag&) {
        BOOST_CB_ASSERT_TEMPLATED_ITERATOR_CONSTRUCTORS // check if the STL provides templated iterator constructors
                                                        // for containers
        std::deque<value_type, allocator_type> tmp(first, last, alloc());
        size_type distance = tmp.size();
        assign_n(distance, distance,
            cb_details::make_assign_range
                (boost::make_move_iterator(tmp.begin()), boost::make_move_iterator(tmp.end()), alloc()));
    }

    /*! INTERNAL ONLY */
    template <class ForwardIterator>
    void assign(ForwardIterator first, ForwardIterator last, const std::forward_iterator_tag&) {
        BOOST_CB_ASSERT(std::distance(first, last) >= 0); // check for wrong range
        size_type distance = std::distance(first, last);
        assign_n(distance, distance, cb_details::make_assign_range(first, last, alloc()));
    }

    /*! INTERNAL ONLY */
    template <class IntegralType>
    void assign(capacity_type new_capacity, IntegralType n, IntegralType item, const true_type&) {
        assign(new_capacity, static_cast<size_type>(n), static_cast<value_type>(item));
    }

    /*! INTERNAL ONLY */
    template <class Iterator>
    void assign(capacity_type new_capacity, Iterator first, Iterator last, const false_type&) {
        BOOST_CB_IS_CONVERTIBLE(Iterator, value_type); // check for invalid iterator type
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x581))
        assign(new_capacity, first, last, std::iterator_traits<Iterator>::iterator_category());
#else
        assign(new_capacity, first, last, BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::iterator_category());
#endif
    }

    /*! INTERNAL ONLY */
    template <class InputIterator>
    void assign(capacity_type new_capacity, InputIterator first, InputIterator last, const std::input_iterator_tag&) {
        if (new_capacity == capacity()) {
            clear();
            insert(begin(), first, last);
        } else {
            circular_buffer<value_type, allocator_type> tmp(new_capacity, first, last, alloc());
            tmp.swap(*this);
        }
    }

    /*! INTERNAL ONLY */
    template <class ForwardIterator>
    void assign(capacity_type new_capacity, ForwardIterator first, ForwardIterator last,
        const std::forward_iterator_tag&) {
        BOOST_CB_ASSERT(std::distance(first, last) >= 0); // check for wrong range
        size_type distance = std::distance(first, last);
        if (distance > new_capacity) {
            std::advance(first, distance - new_capacity);
            distance = new_capacity;
        }
        assign_n(new_capacity, distance,
            cb_details::make_assign_range(first, last, alloc()));
    }

    /*! INTERNAL ONLY */
    template <class Functor>
    void assign_n(capacity_type new_capacity, size_type n, const Functor& fnc) {
        if (new_capacity == capacity()) {
            destroy_content();
            BOOST_TRY {
                fnc(m_buff);
            } BOOST_CATCH(...) {
                m_size = 0;
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        } else {
            pointer buff = allocate(new_capacity);
            BOOST_TRY {
                fnc(buff);
            } BOOST_CATCH(...) {
                deallocate(buff, new_capacity);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
            destroy();
            m_buff = buff;
            m_end = m_buff + new_capacity;
        }
        m_size = n;
        m_first = m_buff;
        m_last = add(m_buff, size());
    }

    /*! INTERNAL ONLY */
    template <class ValT>
    iterator insert_item(const iterator& pos, ValT item) {
        pointer p = pos.m_it;
        if (p == 0) {
            construct_or_replace(!full(), m_last, static_cast<ValT>(item));
            p = m_last;
        } else {
            pointer src = m_last;
            pointer dest = m_last;
            bool construct = !full();
            BOOST_TRY {
                while (src != p) {
                    decrement(src);
                    construct_or_replace(construct, dest, boost::move_if_noexcept(*src));
                    decrement(dest);
                    construct = false;
                }
                replace(p, static_cast<ValT>(item));
            } BOOST_CATCH(...) {
                if (!construct && !full()) {
                    increment(m_last);
                    ++m_size;
                }
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        increment(m_last);
        if (full())
            m_first = m_last;
        else
            ++m_size;
        return iterator(this, p);
    }

    /*! INTERNAL ONLY */
    template <class IntegralType>
    void insert(const iterator& pos, IntegralType n, IntegralType item, const true_type&) {
        insert(pos, static_cast<size_type>(n), static_cast<value_type>(item));
    }

    /*! INTERNAL ONLY */
    template <class Iterator>
    void insert(const iterator& pos, Iterator first, Iterator last, const false_type&) {
        BOOST_CB_IS_CONVERTIBLE(Iterator, value_type); // check for invalid iterator type
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x581))
        insert(pos, first, last, std::iterator_traits<Iterator>::iterator_category());
#else
        insert(pos, first, last, BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::iterator_category());
#endif
    }

    /*! INTERNAL ONLY */
    template <class InputIterator>
    void insert(iterator pos, InputIterator first, InputIterator last, const std::input_iterator_tag&) {
        if (!full() || pos != begin()) {
            for (;first != last; ++pos)
                pos = insert(pos, *first++);
        }
    }

    /*! INTERNAL ONLY */
    template <class ForwardIterator>
    void insert(const iterator& pos, ForwardIterator first, ForwardIterator last, const std::forward_iterator_tag&) {
        BOOST_CB_ASSERT(std::distance(first, last) >= 0); // check for wrong range
        size_type n = std::distance(first, last);
        if (n == 0)
            return;
        size_type copy = capacity() - (end() - pos);
        if (copy == 0)
            return;
        if (n > copy) {
            std::advance(first, n - copy);
            n = copy;
        }
        insert_n(pos, n, cb_details::iterator_wrapper<ForwardIterator>(first));
    }

    /*! INTERNAL ONLY */
    template <class Wrapper>
    void insert_n(const iterator& pos, size_type n, const Wrapper& wrapper) {
        size_type construct = reserve();
        if (construct > n)
            construct = n;
        if (pos.m_it == 0) {
            size_type ii = 0;
            pointer p = m_last;
            BOOST_TRY {
                for (; ii < construct; ++ii, increment(p))
                    boost::allocator_construct(alloc(), boost::to_address(p), *wrapper());
                for (;ii < n; ++ii, increment(p))
                    replace(p, *wrapper());
            } BOOST_CATCH(...) {
                size_type constructed = (std::min)(ii, construct);
                m_last = add(m_last, constructed);
                m_size += constructed;
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        } else {
            pointer src = m_last;
            pointer dest = add(m_last, n - 1);
            pointer p = pos.m_it;
            size_type ii = 0;
            BOOST_TRY {
                while (src != pos.m_it) {
                    decrement(src);
                    construct_or_replace(is_uninitialized(dest), dest, *src);
                    decrement(dest);
                }
                for (; ii < n; ++ii, increment(p))
                    construct_or_replace(is_uninitialized(p), p, *wrapper());
            } BOOST_CATCH(...) {
                for (p = add(m_last, n - 1); p != dest; decrement(p))
                    destroy_if_constructed(p);
                for (n = 0, p = pos.m_it; n < ii; ++n, increment(p))
                    destroy_if_constructed(p);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        m_last = add(m_last, n);
        m_first = add(m_first, n - construct);
        m_size += construct;
    }

    /*! INTERNAL ONLY */
    template <class IntegralType>
    void rinsert(const iterator& pos, IntegralType n, IntegralType item, const true_type&) {
        rinsert(pos, static_cast<size_type>(n), static_cast<value_type>(item));
    }

    /*! INTERNAL ONLY */
    template <class Iterator>
    void rinsert(const iterator& pos, Iterator first, Iterator last, const false_type&) {
        BOOST_CB_IS_CONVERTIBLE(Iterator, value_type); // check for invalid iterator type
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x581))
        rinsert(pos, first, last, std::iterator_traits<Iterator>::iterator_category());
#else
        rinsert(pos, first, last, BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::iterator_category());
#endif
    }

    /*! INTERNAL ONLY */
    template <class InputIterator>
    void rinsert(iterator pos, InputIterator first, InputIterator last, const std::input_iterator_tag&) {
        if (!full() || pos.m_it != 0) {
            for (;first != last; ++pos) {
                pos = rinsert(pos, *first++);
                if (pos.m_it == 0)
                    break;
            }
        }
    }

    /*! INTERNAL ONLY */
    template <class ForwardIterator>
    void rinsert(const iterator& pos, ForwardIterator first, ForwardIterator last, const std::forward_iterator_tag&) {
        BOOST_CB_ASSERT(std::distance(first, last) >= 0); // check for wrong range
        rinsert_n(pos, std::distance(first, last), cb_details::iterator_wrapper<ForwardIterator>(first));
    }

    /*! INTERNAL ONLY */
    template <class Wrapper>
    void rinsert_n(const iterator& pos, size_type n, const Wrapper& wrapper) {
        if (n == 0)
            return;
        iterator b = begin();
        size_type copy = capacity() - (pos - b);
        if (copy == 0)
            return;
        if (n > copy)
            n = copy;
        size_type construct = reserve();
        if (construct > n)
            construct = n;
        if (pos == b) {
            pointer p = sub(m_first, n);
            size_type ii = n;
            BOOST_TRY {
                for (;ii > construct; --ii, increment(p))
                    replace(p, *wrapper());
                for (; ii > 0; --ii, increment(p))
                    boost::allocator_construct(alloc(), boost::to_address(p), *wrapper());
            } BOOST_CATCH(...) {
                size_type constructed = ii < construct ? construct - ii : 0;
                m_last = add(m_last, constructed);
                m_size += constructed;
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        } else {
            pointer src = m_first;
            pointer dest = sub(m_first, n);
            pointer p = map_pointer(pos.m_it);
            BOOST_TRY {
                while (src != p) {
                    construct_or_replace(is_uninitialized(dest), dest, *src);
                    increment(src);
                    increment(dest);
                }
                for (size_type ii = 0; ii < n; ++ii, increment(dest))
                    construct_or_replace(is_uninitialized(dest), dest, *wrapper());
            } BOOST_CATCH(...) {
                for (src = sub(m_first, n); src != dest; increment(src))
                    destroy_if_constructed(src);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        m_first = sub(m_first, n);
        m_last = sub(m_last, n - construct);
        m_size += construct;
    }

    /*! INTERNAL ONLY */
    void erase_begin(size_type n, const true_type&) {
        m_first = add(m_first, n);
        m_size -= n;
    }

    /*! INTERNAL ONLY */
    void erase_begin(size_type n, const false_type&) {
        iterator b = begin();
        rerase(b, b + n);
    }

    /*! INTERNAL ONLY */
    void erase_end(size_type n, const true_type&) {
        m_last = sub(m_last, n);
        m_size -= n;
    }

    /*! INTERNAL ONLY */
    void erase_end(size_type n, const false_type&) {
        iterator e = end();
        erase(e - n, e);
    }
};

// Non-member functions

//! Compare two <code>circular_buffer</code>s element-by-element to determine if they are equal.
/*!
    \param lhs The <code>circular_buffer</code> to compare.
    \param rhs The <code>circular_buffer</code> to compare.
    \return <code>lhs.\link circular_buffer::size() size()\endlink == rhs.\link circular_buffer::size() size()\endlink
            && <a href="https://www.boost.org/sgi/stl/equal.html">std::equal</a>(lhs.\link circular_buffer::begin()
            begin()\endlink, lhs.\link circular_buffer::end() end()\endlink,
            rhs.\link circular_buffer::begin() begin()\endlink)</code>
    \throws Nothing.
    \par Complexity
         Linear (in the size of the <code>circular_buffer</code>s).
    \par Iterator Invalidation
         Does not invalidate any iterators.
*/
template <class T, class Alloc>
inline bool operator == (const circular_buffer<T, Alloc>& lhs, const circular_buffer<T, Alloc>& rhs) {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

/*!
    \brief Compare two <code>circular_buffer</code>s element-by-element to determine if the left one is lesser than the
           right one.
    \param lhs The <code>circular_buffer</code> to compare.
    \param rhs The <code>circular_buffer</code> to compare.
    \return <code><a href="https://www.boost.org/sgi/stl/lexicographical_compare.html">
            std::lexicographical_compare</a>(lhs.\link circular_buffer::begin() begin()\endlink,
            lhs.\link circular_buffer::end() end()\endlink, rhs.\link circular_buffer::begin() begin()\endlink,
            rhs.\link circular_buffer::end() end()\endlink)</code>
    \throws Nothing.
    \par Complexity
         Linear (in the size of the <code>circular_buffer</code>s).
    \par Iterator Invalidation
         Does not invalidate any iterators.
*/
template <class T, class Alloc>
inline bool operator < (const circular_buffer<T, Alloc>& lhs, const circular_buffer<T, Alloc>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) || defined(BOOST_MSVC)

//! Compare two <code>circular_buffer</code>s element-by-element to determine if they are non-equal.
/*!
    \param lhs The <code>circular_buffer</code> to compare.
    \param rhs The <code>circular_buffer</code> to compare.
    \return <code>!(lhs == rhs)</code>
    \throws Nothing.
    \par Complexity
         Linear (in the size of the <code>circular_buffer</code>s).
    \par Iterator Invalidation
         Does not invalidate any iterators.
    \sa <code>operator==(const circular_buffer<T,Alloc>&, const circular_buffer<T,Alloc>&)</code>
*/
template <class T, class Alloc>
inline bool operator != (const circular_buffer<T, Alloc>& lhs, const circular_buffer<T, Alloc>& rhs) {
    return !(lhs == rhs);
}

/*!
    \brief Compare two <code>circular_buffer</code>s element-by-element to determine if the left one is greater than
           the right one.
    \param lhs The <code>circular_buffer</code> to compare.
    \param rhs The <code>circular_buffer</code> to compare.
    \return <code>rhs \< lhs</code>
    \throws Nothing.
    \par Complexity
         Linear (in the size of the <code>circular_buffer</code>s).
    \par Iterator Invalidation
         Does not invalidate any iterators.
    \sa <code>operator<(const circular_buffer<T,Alloc>&, const circular_buffer<T,Alloc>&)</code>
*/
template <class T, class Alloc>
inline bool operator > (const circular_buffer<T, Alloc>& lhs, const circular_buffer<T, Alloc>& rhs) {
    return rhs < lhs;
}

/*!
    \brief Compare two <code>circular_buffer</code>s element-by-element to determine if the left one is lesser or equal
           to the right one.
    \param lhs The <code>circular_buffer</code> to compare.
    \param rhs The <code>circular_buffer</code> to compare.
    \return <code>!(rhs \< lhs)</code>
    \throws Nothing.
    \par Complexity
         Linear (in the size of the <code>circular_buffer</code>s).
    \par Iterator Invalidation
         Does not invalidate any iterators.
    \sa <code>operator<(const circular_buffer<T,Alloc>&, const circular_buffer<T,Alloc>&)</code>
*/
template <class T, class Alloc>
inline bool operator <= (const circular_buffer<T, Alloc>& lhs, const circular_buffer<T, Alloc>& rhs) {
    return !(rhs < lhs);
}

/*!
    \brief Compare two <code>circular_buffer</code>s element-by-element to determine if the left one is greater or
           equal to the right one.
    \param lhs The <code>circular_buffer</code> to compare.
    \param rhs The <code>circular_buffer</code> to compare.
    \return <code>!(lhs < rhs)</code>
    \throws Nothing.
    \par Complexity
         Linear (in the size of the <code>circular_buffer</code>s).
    \par Iterator Invalidation
         Does not invalidate any iterators.
    \sa <code>operator<(const circular_buffer<T,Alloc>&, const circular_buffer<T,Alloc>&)</code>
*/
template <class T, class Alloc>
inline bool operator >= (const circular_buffer<T, Alloc>& lhs, const circular_buffer<T, Alloc>& rhs) {
    return !(lhs < rhs);
}

//! Swap the contents of two <code>circular_buffer</code>s.
/*!
    \post <code>lhs</code> contains elements of <code>rhs</code> and vice versa.
    \param lhs The <code>circular_buffer</code> whose content will be swapped with <code>rhs</code>.
    \param rhs The <code>circular_buffer</code> whose content will be swapped with <code>lhs</code>.
    \throws Nothing.
    \par Complexity
         Constant (in the size of the <code>circular_buffer</code>s).
    \par Iterator Invalidation
         Invalidates all iterators of both <code>circular_buffer</code>s. (On the other hand the iterators still
         point to the same elements but within another container. If you want to rely on this feature you have to
         turn the <a href="#debug">Debug Support</a> off otherwise an assertion will report an error if such
         invalidated iterator is used.)
    \sa <code>\link circular_buffer::swap(circular_buffer<T, Alloc>&) swap(circular_buffer<T, Alloc>&)\endlink</code>
*/
template <class T, class Alloc>
inline void swap(circular_buffer<T, Alloc>& lhs, circular_buffer<T, Alloc>& rhs) BOOST_NOEXCEPT {
    lhs.swap(rhs);
}

#endif // #if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) || defined(BOOST_MSVC)

} // namespace boost

#endif // #if !defined(BOOST_CIRCULAR_BUFFER_BASE_HPP)
