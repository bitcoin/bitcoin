// Boost.Container varray
//
// Copyright (c) 2011-2013 Andrew Hundt.
// Copyright (c) 2012-2015 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_HPP

// TODO - REMOVE/CHANGE
#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <boost/move/detail/fwd_macros.hpp>
#endif

#include <boost/concept_check.hpp>
#include <boost/config.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/swap.hpp>
#include <boost/integer.hpp>

// TODO - use std::reverse_iterator and std::iterator_traits
// instead Boost.Iterator to remove dependency?
// or boost/detail/iterator.hpp ?
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/iterator/iterator_concepts.hpp>

#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/aligned_storage.hpp>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/index/detail/assert.hpp>
#include <boost/geometry/index/detail/exception.hpp>

#include <boost/geometry/index/detail/varray_detail.hpp>

/*!
\defgroup varray_non_member varray non-member functions
*/

namespace boost { namespace geometry { namespace index { namespace detail {
    
namespace varray_detail {

template <typename Value, std::size_t Capacity>
struct varray_traits
{
    typedef Value value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef Value * pointer;
    typedef const Value * const_pointer;
    typedef Value & reference;
    typedef const Value & const_reference;

    typedef std::false_type use_memop_in_swap_and_move;
    typedef std::false_type use_optimized_swap;
    typedef std::false_type disable_trivial_init;
};

template <typename Varray>
struct checker
{
    typedef typename Varray::size_type size_type;
    typedef typename Varray::const_iterator const_iterator;

    static inline void check_capacity(Varray const& v, size_type s)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(s <= v.capacity(), "size too big");

        ::boost::ignore_unused(v, s);
    }

    static inline void throw_out_of_bounds(Varray const& v, size_type i)
    {
        if ( v.size() <= i )
            throw_out_of_range("index out of bounds");

        ::boost::ignore_unused(v, i);
    }

    static inline void check_index(Varray const& v, size_type i)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(i < v.size(), "index out of bounds");

        ::boost::ignore_unused(v, i);
    }

    static inline void check_not_empty(Varray const& v)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(!v.empty(), "the container is empty");
        
        ::boost::ignore_unused(v);
    }

    static inline void check_iterator_end_neq(Varray const& v, const_iterator position)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(v.begin() <= position && position < v.end(), "iterator out of bounds");

        ::boost::ignore_unused(v, position);
    }

    static inline void check_iterator_end_eq(Varray const& v, const_iterator position)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(v.begin() <= position && position <= v.end(), "iterator out of bounds");

        ::boost::ignore_unused(v, position);
    }
};

} // namespace varray_detail

/*!
\brief A variable-size array container with fixed capacity.

varray is a sequence container like boost::container::vector with contiguous storage that can
change in size, along with the static allocation, low overhead, and fixed capacity of boost::array.

A varray is a sequence that supports random access to elements, constant time insertion and
removal of elements at the end, and linear time insertion and removal of elements at the beginning or 
in the middle. The number of elements in a varray may vary dynamically up to a fixed capacity
because elements are stored within the object itself similarly to an array. However, objects are 
initialized as they are inserted into varray unlike C arrays or std::array which must construct
all elements on instantiation. The behavior of varray enables the use of statically allocated
elements in cases with complex object lifetime requirements that would otherwise not be trivially 
possible.

\par Error Handling
 Insertion beyond the capacity and out of bounds errors result in undefined behavior unless
 otherwise specified. In this respect if size() == capacity(), then varray::push_back()
 behaves like std::vector pop_front() if size() == empty(). The reason for this difference
 is because unlike vectors, varray does not perform allocation.

\par Advanced Usage
 Error handling behavior can be modified to more closely match std::vector exception behavior
 when exceeding bounds by providing an alternate Strategy and varray_traits instantiation.

\tparam Value    The type of element that will be stored.
\tparam Capacity The maximum number of elements varray can store, fixed at compile time.
\tparam Strategy Defines the public typedefs and error handlers,
        implements StaticVectorStrategy and has some similarities
        to an Allocator.
*/
template <typename Value, std::size_t Capacity>
class varray
{
    typedef varray_detail::varray_traits<Value, Capacity> vt;
    typedef varray_detail::checker<varray> errh;

    BOOST_GEOMETRY_STATIC_ASSERT(
        ( std::is_unsigned<typename vt::size_type>::value &&
          sizeof(typename boost::uint_value_t<Capacity>::least) <= sizeof(typename vt::size_type) ),
        "Size type is too small for specified capacity.",
        typename vt::size_type, std::integral_constant<std::size_t, Capacity>
    );

    typedef boost::aligned_storage<
        sizeof(Value[Capacity]),
        boost::alignment_of<Value[Capacity]>::value
    > aligned_storage_type;

    template <typename V, std::size_t C>
    friend class varray;

    BOOST_COPYABLE_AND_MOVABLE(varray)

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
public:
    template <std::size_t C>
    varray & operator=(varray<Value, C> & sv)
    {
        typedef varray<Value, C> other;
        this->operator=(static_cast<const ::boost::rv<other> &>(const_cast<const other &>(sv)));
        return *this;
    }
#endif

public:
    //! @brief The type of elements stored in the container.
    typedef typename vt::value_type value_type;
    //! @brief The unsigned integral type used by the container.
    typedef typename vt::size_type size_type;
    //! @brief The pointers difference type.
    typedef typename vt::difference_type difference_type;
    //! @brief The pointer type.
    typedef typename vt::pointer pointer;
    //! @brief The const pointer type.
    typedef typename vt::const_pointer const_pointer;
    //! @brief The value reference type.
    typedef typename vt::reference reference;
    //! @brief The value const reference type.
    typedef typename vt::const_reference const_reference;

    //! @brief The iterator type.
    typedef pointer iterator;
    //! @brief The const iterator type.
    typedef const_pointer const_iterator;
    //! @brief The reverse iterator type.
    typedef boost::reverse_iterator<iterator> reverse_iterator;
    //! @brief The const reverse iterator.
    typedef boost::reverse_iterator<const_iterator> const_reverse_iterator;

    //! @brief Constructs an empty varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    varray()
        : m_size(0)
    {}

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief Constructs a varray containing count default constructed Values.
    //!
    //! @param count    The number of values which will be contained in the container.
    //!
    //! @par Throws
    //!   If Value's default constructor throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    explicit varray(size_type count)
        : m_size(0)
    {
        this->resize(count);                                                        // may throw
    }

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief Constructs a varray containing count copies of value.
    //!
    //! @param count    The number of copies of a values that will be contained in the container.
    //! @param value    The value which will be used to copy construct values.
    //!
    //! @par Throws
    //!   If Value's copy constructor throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    varray(size_type count, value_type const& value)
        : m_size(0)
    {
        this->resize(count, value);                                                 // may throw
    }

    //! @pre
    //!  @li <tt>distance(first, last) <= capacity()</tt>
    //!  @li Iterator must meet the \c ForwardTraversalIterator concept.
    //!
    //! @brief Constructs a varray containing copy of a range <tt>[first, last)</tt>.
    //!
    //! @param first    The iterator to the first element in range.
    //! @param last     The iterator to the one after the last element in range.
    //!
    //! @par Throws
    //!   If Value's constructor taking a dereferenced Iterator throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <typename Iterator>
    varray(Iterator first, Iterator last)
        : m_size(0)
    {
        BOOST_CONCEPT_ASSERT((boost_concepts::ForwardTraversal<Iterator>)); // Make sure you passed a ForwardIterator
        
        this->assign(first, last);                                                    // may throw
    }

    //! @brief Constructs a copy of other varray.
    //!
    //! @param other    The varray which content will be copied to this one.
    //!
    //! @par Throws
    //!   If Value's copy constructor throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    varray(varray const& other)
        : m_size(other.size())
    {
        namespace sv = varray_detail;
        sv::uninitialized_copy(other.begin(), other.end(), this->begin());          // may throw
    }

    //! @pre <tt>other.size() <= capacity()</tt>.
    //!
    //! @brief Constructs a copy of other varray.
    //!
    //! @param other    The varray which content will be copied to this one.
    //!
    //! @par Throws
    //!   If Value's copy constructor throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <std::size_t C>
    varray(varray<value_type, C> const& other)
        : m_size(other.size())
    {
        errh::check_capacity(*this, other.size());                                  // may throw
        
        namespace sv = varray_detail;
        sv::uninitialized_copy(other.begin(), other.end(), this->begin());          // may throw
    }

    //! @brief Copy assigns Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be copied to this one.
    //!
    //! @par Throws
    //!   If Value's copy constructor or copy assignment throws.
    //!
    //! @par Complexity
    //! Linear O(N).
    varray & operator=(BOOST_COPY_ASSIGN_REF(varray) other)
    {
        this->assign(other.begin(), other.end());                                     // may throw

        return *this;
    }

    //! @pre <tt>other.size() <= capacity()</tt>
    //!
    //! @brief Copy assigns Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be copied to this one.
    //!
    //! @par Throws
    //!   If Value's copy constructor or copy assignment throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <std::size_t C>
    varray & operator=(BOOST_COPY_ASSIGN_REF_2_TEMPL_ARGS(varray, value_type, C) other)
    {
        this->assign(other.begin(), other.end());                                     // may throw

        return *this;
    }

    //! @brief Move constructor. Moves Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be moved to this one.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor throws.
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor throws.
    //! @internal
    //!   @li It throws only if \c use_memop_in_swap_and_move is \c false_type - default.
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    varray(BOOST_RV_REF(varray) other)
    {
        typedef typename
        vt::use_memop_in_swap_and_move use_memop_in_swap_and_move;

        this->move_ctor_dispatch(other, use_memop_in_swap_and_move());
    }

    //! @pre <tt>other.size() <= capacity()</tt>
    //!
    //! @brief Move constructor. Moves Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be moved to this one.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor throws.
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor throws.
    //! @internal
    //!   @li It throws only if \c use_memop_in_swap_and_move is false_type - default.
    //! @endinternal
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <std::size_t C>
    varray(BOOST_RV_REF_2_TEMPL_ARGS(varray, value_type, C) other)
        : m_size(other.m_size)
    {
        errh::check_capacity(*this, other.size());                                  // may throw

        typedef typename
        vt::use_memop_in_swap_and_move use_memop_in_swap_and_move;

        this->move_ctor_dispatch(other, use_memop_in_swap_and_move());
    }

    //! @brief Move assignment. Moves Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be moved to this one.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor or move assignment throws.
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor or copy assignment throws.
    //! @internal
    //!   @li It throws only if \c use_memop_in_swap_and_move is \c false_type - default.
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    varray & operator=(BOOST_RV_REF(varray) other)
    {
        if ( &other == this )
            return *this;

        typedef typename
        vt::use_memop_in_swap_and_move use_memop_in_swap_and_move;

        this->move_assign_dispatch(other, use_memop_in_swap_and_move());

        return *this;
    }

    //! @pre <tt>other.size() <= capacity()</tt>
    //!
    //! @brief Move assignment. Moves Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be moved to this one.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor or move assignment throws.
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor or copy assignment throws.
    //! @internal
    //!   @li It throws only if \c use_memop_in_swap_and_move is \c false_type - default.
    //! @endinternal
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <std::size_t C>
    varray & operator=(BOOST_RV_REF_2_TEMPL_ARGS(varray, value_type, C) other)
    {
        errh::check_capacity(*this, other.size());                                  // may throw

        typedef typename
        vt::use_memop_in_swap_and_move use_memop_in_swap_and_move;

        this->move_assign_dispatch(other, use_memop_in_swap_and_move());

        return *this;
    }

    //! @brief Destructor. Destroys Values stored in this container.
    //!
    //! @par Throws
    //!   Nothing
    //!
    //! @par Complexity
    //!   Linear O(N).
    ~varray()
    {
        namespace sv = varray_detail;
        sv::destroy(this->begin(), this->end());
    }

    //! @brief Swaps contents of the other varray and this one.
    //!
    //! @param other    The varray which content will be swapped with this one's content.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor or move assignment throws,
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor or copy assignment throws,
    //! @internal
    //!   @li It throws only if \c use_memop_in_swap_and_move and \c use_optimized_swap are \c false_type - default.
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    void swap(varray & other)
    {
        typedef typename
        vt::use_optimized_swap use_optimized_swap;

        this->swap_dispatch(other, use_optimized_swap());
    }

    //! @pre <tt>other.size() <= capacity() && size() <= other.capacity()</tt>
    //!
    //! @brief Swaps contents of the other varray and this one.
    //!
    //! @param other    The varray which content will be swapped with this one's content.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor or move assignment throws,
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor or copy assignment throws,
    //! @internal
    //!   @li It throws only if \c use_memop_in_swap_and_move and \c use_optimized_swap are \c false_type - default.
    //! @endinternal
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <std::size_t C>
    void swap(varray<value_type, C> & other)
    {
        errh::check_capacity(*this, other.size());
        errh::check_capacity(other, this->size());

        typedef typename
        vt::use_optimized_swap use_optimized_swap;

        this->swap_dispatch(other, use_optimized_swap()); 
    }

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief Inserts or erases elements at the end such that
    //!   the size becomes count. New elements are default constructed.
    //!
    //! @param count    The number of elements which will be stored in the container.
    //!
    //! @par Throws
    //!   If Value's default constructor throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    void resize(size_type count)
    {
        namespace sv = varray_detail;
        typedef typename vt::disable_trivial_init dti;

        if ( count < m_size )
        {
            sv::destroy(this->begin() + count, this->end());
        }
        else
        {
            errh::check_capacity(*this, count);                                     // may throw

            sv::uninitialized_fill(this->end(), this->begin() + count, dti()); // may throw
        }
        m_size = count; // update end
    }

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief Inserts or erases elements at the end such that
    //!   the size becomes count. New elements are copy constructed from value.
    //!
    //! @param count    The number of elements which will be stored in the container.
    //! @param value    The value used to copy construct the new element.
    //!
    //! @par Throws
    //!   If Value's copy constructor throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    void resize(size_type count, value_type const& value)
    {
        if ( count < m_size )
        {
            namespace sv = varray_detail;
            sv::destroy(this->begin() + count, this->end());
        }
        else
        {
            errh::check_capacity(*this, count);                                     // may throw
            
            std::uninitialized_fill(this->end(), this->begin() + count, value);     // may throw
        }
        m_size = count; // update end
    }

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief This call has no effect because the Capacity of this container is constant.
    //!
    //! @param count    The number of elements which the container should be able to contain.
    //!
    //! @par Throws
    //!   Nothing.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    void reserve(size_type count)
    {
        errh::check_capacity(*this, count);                                         // may throw
    }

    //! @pre <tt>size() < capacity()</tt>
    //!
    //! @brief Adds a copy of value at the end.
    //!
    //! @param value    The value used to copy construct the new element.
    //!
    //! @par Throws
    //!   If Value's copy constructor throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Constant O(1).
    void push_back(value_type const& value)
    {
        typedef typename vt::disable_trivial_init dti;

        errh::check_capacity(*this, m_size + 1);                                    // may throw
        
        namespace sv = varray_detail;
        sv::construct(dti(), this->end(), value);                                          // may throw
        ++m_size; // update end
    }

    //! @pre <tt>size() < capacity()</tt>
    //!
    //! @brief Moves value to the end.
    //!
    //! @param value    The value to move construct the new element.
    //!
    //! @par Throws
    //!   If Value's move constructor throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Constant O(1).
    void push_back(BOOST_RV_REF(value_type) value)
    {
        typedef typename vt::disable_trivial_init dti;

        errh::check_capacity(*this, m_size + 1);                                    // may throw

        namespace sv = varray_detail;
        sv::construct(dti(), this->end(), ::boost::move(value));                           // may throw
        ++m_size; // update end
    }

    //! @pre <tt>!empty()</tt>
    //!
    //! @brief Destroys last value and decreases the size.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    void pop_back()
    {
        errh::check_not_empty(*this);

        namespace sv = varray_detail;
        sv::destroy(this->end() - 1);
        --m_size; // update end
    }

    //! @pre
    //!  @li \c position must be a valid iterator of \c *this in range <tt>[begin(), end()]</tt>.
    //!  @li <tt>size() < capacity()</tt>
    //!
    //! @brief Inserts a copy of element at position.
    //!
    //! @param position    The position at which the new value will be inserted.
    //! @param value       The value used to copy construct the new element.
    //!
    //! @par Throws
    //!   @li If Value's copy constructor or copy assignment throws
    //!   @li If Value's move constructor or move assignment throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Constant or linear.
    iterator insert(iterator position, value_type const& value)
    {
        typedef typename vt::disable_trivial_init dti;
        namespace sv = varray_detail;

        errh::check_iterator_end_eq(*this, position);
        errh::check_capacity(*this, m_size + 1);                                    // may throw

        if ( position == this->end() )
        {
            sv::construct(dti(), position, value);                                  // may throw
            ++m_size; // update end
        }
        else
        {
            // TODO - should move be used only if it's nonthrowing?
            value_type & r = *(this->end() - 1);
            sv::construct(dti(), this->end(), boost::move(r));                      // may throw
            ++m_size; // update end
            sv::move_backward(position, this->end() - 2, this->end() - 1);          // may throw
            sv::assign(position, value);                                            // may throw
        }

        return position;
    }

    //! @pre
    //!  @li \c position must be a valid iterator of \c *this in range <tt>[begin(), end()]</tt>.
    //!  @li <tt>size() < capacity()</tt>
    //!
    //! @brief Inserts a move-constructed element at position.
    //!
    //! @param position    The position at which the new value will be inserted.
    //! @param value       The value used to move construct the new element.
    //!
    //! @par Throws
    //!   If Value's move constructor or move assignment throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Constant or linear.
    iterator insert(iterator position, BOOST_RV_REF(value_type) value)
    {
        typedef typename vt::disable_trivial_init dti;
        namespace sv = varray_detail;

        errh::check_iterator_end_eq(*this, position);
        errh::check_capacity(*this, m_size + 1);                                    // may throw

        if ( position == this->end() )
        {
            sv::construct(dti(), position, boost::move(value));                     // may throw
            ++m_size; // update end
        }
        else
        {
            // TODO - should move be used only if it's nonthrowing?
            value_type & r = *(this->end() - 1);
            sv::construct(dti(), this->end(), boost::move(r));                      // may throw
            ++m_size; // update end
            sv::move_backward(position, this->end() - 2, this->end() - 1);          // may throw
            sv::assign(position, boost::move(value));                               // may throw
        }

        return position;
    }

    //! @pre
    //!  @li \c position must be a valid iterator of \c *this in range <tt>[begin(), end()]</tt>.
    //!  @li <tt>size() + count <= capacity()</tt>
    //!
    //! @brief Inserts a count copies of value at position.
    //!
    //! @param position    The position at which new elements will be inserted.
    //! @param count       The number of new elements which will be inserted.
    //! @param value       The value used to copy construct new elements.
    //!
    //! @par Throws
    //!   @li If Value's copy constructor or copy assignment throws.
    //!   @li If Value's move constructor or move assignment throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    iterator insert(iterator position, size_type count, value_type const& value)
    {
        errh::check_iterator_end_eq(*this, position);
        errh::check_capacity(*this, m_size + count);                                // may throw

        if ( position == this->end() )
        {
            std::uninitialized_fill(position, position + count, value);             // may throw
            m_size += count; // update end
        }
        else
        {
            namespace sv = varray_detail;

            difference_type to_move = std::distance(position, this->end());
            
            // TODO - should following lines check for exception and revert to the old size?

            if ( count < static_cast<size_type>(to_move) )
            {
                sv::uninitialized_move(this->end() - count, this->end(), this->end());          // may throw
                m_size += count; // update end
                sv::move_backward(position, position + to_move - count, this->end() - count);   // may throw
                std::fill_n(position, count, value);                                            // may throw
            }
            else
            {
                std::uninitialized_fill(this->end(), position + count, value);                  // may throw
                m_size += count - to_move; // update end
                sv::uninitialized_move(position, position + to_move, position + count);         // may throw
                m_size += to_move; // update end
                std::fill_n(position, to_move, value);                                          // may throw
            }
        }

        return position;
    }

    //! @pre
    //!  @li \c position must be a valid iterator of \c *this in range <tt>[begin(), end()]</tt>.
    //!  @li <tt>distance(first, last) <= capacity()</tt>
    //!  @li \c Iterator must meet the \c ForwardTraversalIterator concept.
    //!
    //! @brief Inserts a copy of a range <tt>[first, last)</tt> at position.
    //!
    //! @param position    The position at which new elements will be inserted.
    //! @param first       The iterator to the first element of a range used to construct new elements.
    //! @param last        The iterator to the one after the last element of a range used to construct new elements.
    //!
    //! @par Throws
    //!   @li If Value's constructor and assignment taking a dereferenced \c Iterator.
    //!   @li If Value's move constructor or move assignment throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <typename Iterator>
    iterator insert(iterator position, Iterator first, Iterator last)
    {
        BOOST_CONCEPT_ASSERT((boost_concepts::ForwardTraversal<Iterator>)); // Make sure you passed a ForwardIterator

        typedef typename boost::iterator_traversal<Iterator>::type traversal;
        this->insert_dispatch(position, first, last, traversal());

        return position;
    }

    //! @pre \c position must be a valid iterator of \c *this in range <tt>[begin(), end())</tt>
    //!
    //! @brief Erases Value from position.
    //!
    //! @param position    The position of the element which will be erased from the container.
    //!
    //! @par Throws
    //!   If Value's move assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    iterator erase(iterator position)
    {
        namespace sv = varray_detail;

        errh::check_iterator_end_neq(*this, position);

        //TODO - add empty check?
        //errh::check_empty(*this);

        sv::move(position + 1, this->end(), position);                              // may throw
        sv::destroy(this->end() - 1);
        --m_size;

        return position;
    }

    //! @pre
    //!  @li \c first and \c last must define a valid range
    //!  @li iterators must be in range <tt>[begin(), end()]</tt>
    //!
    //! @brief Erases Values from a range <tt>[first, last)</tt>.
    //!
    //! @param first    The position of the first element of a range which will be erased from the container.
    //! @param last     The position of the one after the last element of a range which will be erased from the container.
    //!
    //! @par Throws
    //!   If Value's move assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    iterator erase(iterator first, iterator last)
    {
        namespace sv = varray_detail;

        errh::check_iterator_end_eq(*this, first);
        errh::check_iterator_end_eq(*this, last);
        
        difference_type n = std::distance(first, last);
        
        //TODO - add invalid range check?
        //BOOST_GEOMETRY_INDEX_ASSERT(0 <= n, "invalid range");
        //TODO - add this->size() check?
        //BOOST_GEOMETRY_INDEX_ASSERT(n <= this->size(), "invalid range");

        sv::move(last, this->end(), first);                                         // may throw
        sv::destroy(this->end() - n, this->end());
        m_size -= n;

        return first;
    }

    //! @pre <tt>distance(first, last) <= capacity()</tt>
    //!
    //! @brief Assigns a range <tt>[first, last)</tt> of Values to this container.
    //!
    //! @param first       The iterator to the first element of a range used to construct new content of this container.
    //! @param last        The iterator to the one after the last element of a range used to construct new content of this container.
    //!
    //! @par Throws
    //!   If Value's copy constructor or copy assignment throws,
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <typename Iterator>
    void assign(Iterator first, Iterator last)
    {
        BOOST_CONCEPT_ASSERT((boost_concepts::ForwardTraversal<Iterator>)); // Make sure you passed a ForwardIterator

        typedef typename boost::iterator_traversal<Iterator>::type traversal;
        this->assign_dispatch(first, last, traversal());                            // may throw
    }

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief Assigns a count copies of value to this container.
    //!
    //! @param count       The new number of elements which will be container in the container.
    //! @param value       The value which will be used to copy construct the new content.
    //!
    //! @par Throws
    //!   If Value's copy constructor or copy assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    void assign(size_type count, value_type const& value)
    {
        if ( count < m_size )
        {
            namespace sv = varray_detail;

            std::fill_n(this->begin(), count, value);                               // may throw
            sv::destroy(this->begin() + count, this->end());
        }
        else
        {
            errh::check_capacity(*this, count);                                     // may throw

            std::fill_n(this->begin(), m_size, value);                              // may throw
            std::uninitialized_fill(this->end(), this->begin() + count, value);     // may throw
        }
        m_size = count; // update end
    }

#if !defined(BOOST_CONTAINER_VARRAY_DISABLE_EMPLACE)
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
    //! @pre <tt>size() < capacity()</tt>
    //!
    //! @brief Inserts a Value constructed with
    //!   \c std::forward<Args>(args)... in the end of the container.
    //!
    //! @param args     The arguments of the constructor of the new element which will be created at the end of the container.
    //!
    //! @par Throws
    //!   If in-place constructor throws or Value's move constructor throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Constant O(1).
    template<class ...Args>
    void emplace_back(BOOST_FWD_REF(Args) ...args)
    {
        typedef typename vt::disable_trivial_init dti;

        errh::check_capacity(*this, m_size + 1);                                    // may throw

        namespace sv = varray_detail;
        sv::construct(dti(), this->end(), ::boost::forward<Args>(args)...);                // may throw
        ++m_size; // update end
    }

    //! @pre
    //!  @li \c position must be a valid iterator of \c *this in range <tt>[begin(), end()]</tt>
    //!  @li <tt>size() < capacity()</tt>
    //!
    //! @brief Inserts a Value constructed with
    //!   \c std::forward<Args>(args)... before position
    //!
    //! @param position The position at which new elements will be inserted.
    //! @param args     The arguments of the constructor of the new element.
    //!
    //! @par Throws
    //!   If in-place constructor throws or if Value's move constructor or move assignment throws.
    //! @internal
    //!   @li If a throwing error handler is specified, throws when the capacity is exceeded. (not by default).
    //! @endinternal
    //!
    //! @par Complexity
    //!   Constant or linear.
    template<class ...Args>
    iterator emplace(iterator position, BOOST_FWD_REF(Args) ...args)
    {
        typedef typename vt::disable_trivial_init dti;

        namespace sv = varray_detail;

        errh::check_iterator_end_eq(*this, position);
        errh::check_capacity(*this, m_size + 1);                                    // may throw

        if ( position == this->end() )
        {
            sv::construct(dti(), position, ::boost::forward<Args>(args)...);               // may throw
            ++m_size; // update end
        }
        else
        {
            // TODO - should following lines check for exception and revert to the old size?

            // TODO - should move be used only if it's nonthrowing?
            value_type & r = *(this->end() - 1);
            sv::construct(dti(), this->end(), boost::move(r));                             // may throw
            ++m_size; // update end
            sv::move_backward(position, this->end() - 2, this->end() - 1);          // may throw

            aligned_storage<sizeof(value_type), alignment_of<value_type>::value> temp_storage;
            value_type * val_p = static_cast<value_type *>(temp_storage.address());
            sv::construct(dti(), val_p, ::boost::forward<Args>(args)...);                  // may throw
            sv::scoped_destructor<value_type> d(val_p);
            sv::assign(position, ::boost::move(*val_p));                            // may throw
        }

        return position;
    }

#else // !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)

    #define BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_EMPLACE(N)                                        \
    BOOST_MOVE_TMPL_LT##N BOOST_MOVE_CLASS##N BOOST_MOVE_GT##N                                   \
    void emplace_back(BOOST_MOVE_UREF##N)                                                        \
    {                                                                                            \
        typedef typename vt::disable_trivial_init dti;                                           \
                                                                                                 \
        errh::check_capacity(*this, m_size + 1);                                    /*may throw*/\
                                                                                                 \
        namespace sv = varray_detail;                                                            \
        sv::construct(dti(), this->end() BOOST_MOVE_I##N BOOST_MOVE_FWD##N );       /*may throw*/\
        ++m_size; /*update end*/                                                                 \
    }                                                                                            \
    \
    BOOST_MOVE_TMPL_LT##N BOOST_MOVE_CLASS##N BOOST_MOVE_GT##N                                      \
    iterator emplace(iterator position BOOST_MOVE_I##N BOOST_MOVE_UREF##N)                          \
    {                                                                                               \
        typedef typename vt::disable_trivial_init dti;                                              \
        namespace sv = varray_detail;                                                               \
                                                                                                    \
        errh::check_iterator_end_eq(*this, position);                                               \
        errh::check_capacity(*this, m_size + 1);                                       /*may throw*/\
                                                                                                    \
        if ( position == this->end() )                                                              \
        {                                                                                           \
            sv::construct(dti(), position BOOST_MOVE_I##N BOOST_MOVE_FWD##N );         /*may throw*/\
            ++m_size; /*update end*/                                                                \
        }                                                                                           \
        else                                                                                        \
        {                                                                                           \
            /* TODO - should following lines check for exception and revert to the old size? */     \
            /* TODO - should move be used only if it's nonthrowing? */                              \
                                                                                                    \
            value_type & r = *(this->end() - 1);                                                    \
            sv::construct(dti(), this->end(), boost::move(r));                         /*may throw*/\
            ++m_size; /*update end*/                                                                \
            sv::move_backward(position, this->end() - 2, this->end() - 1);             /*may throw*/\
                                                                                                    \
            aligned_storage<sizeof(value_type), alignment_of<value_type>::value> temp_storage;      \
            value_type * val_p = static_cast<value_type *>(temp_storage.address());                 \
            sv::construct(dti(), val_p BOOST_MOVE_I##N BOOST_MOVE_FWD##N );            /*may throw*/\
            sv::scoped_destructor<value_type> d(val_p);                                             \
            sv::assign(position, ::boost::move(*val_p));                               /*may throw*/\
        }                                                                                           \
                                                                                                    \
        return position;                                                                            \
    }                                                                                               \
    
    BOOST_MOVE_ITERATE_0TO9(BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_EMPLACE)
    #undef BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_EMPLACE

#endif // !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
#endif // !BOOST_CONTAINER_VARRAY_DISABLE_EMPLACE

    //! @brief Removes all elements from the container.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    void clear()
    {
        namespace sv = varray_detail;
        sv::destroy(this->begin(), this->end());
        m_size = 0; // update end
    }

    //! @pre <tt>i < size()</tt>
    //!
    //! @brief Returns reference to the i-th element.
    //!
    //! @param i    The element's index.
    //!
    //! @return reference to the i-th element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   \c std::out_of_range exception by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reference at(size_type i)
    {
        errh::throw_out_of_bounds(*this, i);                                   // may throw
        return *(this->begin() + i);
    }

    //! @pre <tt>i < size()</tt>
    //!
    //! @brief Returns const reference to the i-th element.
    //!
    //! @param i    The element's index.
    //!
    //! @return const reference to the i-th element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   \c std::out_of_range exception by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reference at(size_type i) const
    {
        errh::throw_out_of_bounds(*this, i);                                   // may throw
        return *(this->begin() + i);
    }

    //! @pre <tt>i < size()</tt>
    //!
    //! @brief Returns reference to the i-th element.
    //!
    //! @param i    The element's index.
    //!
    //! @return reference to the i-th element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reference operator[](size_type i)
    {
        // TODO: Remove bounds check? std::vector and std::array operator[] don't check.
        errh::check_index(*this, i);
        return *(this->begin() + i);
    }

    //! @pre <tt>i < size()</tt>
    //!
    //! @brief Returns const reference to the i-th element.
    //!
    //! @param i    The element's index.
    //!
    //! @return const reference to the i-th element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reference operator[](size_type i) const
    {
        errh::check_index(*this, i);
        return *(this->begin() + i);
    }

    //! @pre \c !empty()
    //!
    //! @brief Returns reference to the first element.
    //!
    //! @return reference to the first element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reference front()
    {
        errh::check_not_empty(*this);
        return *(this->begin());
    }

    //! @pre \c !empty()
    //!
    //! @brief Returns const reference to the first element.
    //!
    //! @return const reference to the first element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reference front() const
    {
        errh::check_not_empty(*this);
        return *(this->begin());
    }

    //! @pre \c !empty()
    //!
    //! @brief Returns reference to the last element.
    //!
    //! @return reference to the last element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reference back()
    {
        errh::check_not_empty(*this);
        return *(this->end() - 1);
    }

    //! @pre \c !empty()
    //!
    //! @brief Returns const reference to the first element.
    //!
    //! @return const reference to the last element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reference back() const
    {
        errh::check_not_empty(*this);
        return *(this->end() - 1);
    }

    //! @brief Pointer such that <tt>[data(), data() + size())</tt> is a valid range.
    //!   For a non-empty vector <tt>data() == &front()</tt>.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    Value * data()
    {
        return boost::addressof(*(this->ptr()));
    }

    //! @brief Const pointer such that <tt>[data(), data() + size())</tt> is a valid range.
    //!   For a non-empty vector <tt>data() == &front()</tt>.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const Value * data() const
    {
        return boost::addressof(*(this->ptr()));
    }

    
    //! @brief Returns iterator to the first element.
    //!
    //! @return iterator to the first element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    iterator begin() { return this->ptr(); }

    //! @brief Returns const iterator to the first element.
    //!
    //! @return const_iterator to the first element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_iterator begin() const { return this->ptr(); }

    //! @brief Returns const iterator to the first element.
    //!
    //! @return const_iterator to the first element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_iterator cbegin() const { return this->ptr(); }

    //! @brief Returns iterator to the one after the last element.
    //!
    //! @return iterator pointing to the one after the last element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    iterator end() { return this->begin() + m_size; }

    //! @brief Returns const iterator to the one after the last element.
    //!
    //! @return const_iterator pointing to the one after the last element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_iterator end() const { return this->begin() + m_size; }

    //! @brief Returns const iterator to the one after the last element.
    //!
    //! @return const_iterator pointing to the one after the last element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_iterator cend() const { return this->cbegin() + m_size; }

    //! @brief Returns reverse iterator to the first element of the reversed container.
    //!
    //! @return reverse_iterator pointing to the beginning
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reverse_iterator rbegin() { return reverse_iterator(this->end()); }

    //! @brief Returns const reverse iterator to the first element of the reversed container.
    //!
    //! @return const_reverse_iterator pointing to the beginning
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reverse_iterator rbegin() const { return const_reverse_iterator(this->end()); }

    //! @brief Returns const reverse iterator to the first element of the reversed container.
    //!
    //! @return const_reverse_iterator pointing to the beginning
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reverse_iterator crbegin() const { return const_reverse_iterator(this->end()); }

    //! @brief Returns reverse iterator to the one after the last element of the reversed container.
    //!
    //! @return reverse_iterator pointing to the one after the last element
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reverse_iterator rend() { return reverse_iterator(this->begin()); }

    //! @brief Returns const reverse iterator to the one after the last element of the reversed container.
    //!
    //! @return const_reverse_iterator pointing to the one after the last element
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reverse_iterator rend() const { return const_reverse_iterator(this->begin()); }

    //! @brief Returns const reverse iterator to the one after the last element of the reversed container.
    //!
    //! @return const_reverse_iterator pointing to the one after the last element
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reverse_iterator crend() const { return const_reverse_iterator(this->begin()); }

    //! @brief Returns container's capacity.
    //!
    //! @return container's capacity.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    static size_type capacity() { return Capacity; }

    //! @brief Returns container's capacity.
    //!
    //! @return container's capacity.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    static size_type max_size() { return Capacity; }

    //! @brief Returns the number of stored elements.
    //!
    //! @return Number of elements contained in the container.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    size_type size() const { return m_size; }

    //! @brief Queries if the container contains elements.
    //!
    //! @return true if the number of elements contained in the
    //!   container is equal to 0.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    bool empty() const { return 0 == m_size; }

private:

    // @par Throws
    //   Nothing.
    // @par Complexity
    //   Linear O(N).
    template <std::size_t C>
    void move_ctor_dispatch(varray<value_type, C> & other, std::true_type /*use_memop*/)
    {
        ::memcpy(this->data(), other.data(), sizeof(Value) * other.m_size);
        m_size = other.m_size;
    }

    // @par Throws
    //   @li If boost::has_nothrow_move<Value>::value is true and Value's move constructor throws
    //   @li If boost::has_nothrow_move<Value>::value is false and Value's copy constructor throws.
    // @par Complexity
    //   Linear O(N).
    template <std::size_t C>
    void move_ctor_dispatch(varray<value_type, C> & other, std::false_type /*use_memop*/)
    {
        namespace sv = varray_detail;
        sv::uninitialized_move_if_noexcept(other.begin(), other.end(), this->begin());                  // may throw
        m_size = other.m_size;
    }

    // @par Throws
    //   Nothing.
    // @par Complexity
    //   Linear O(N).
    template <std::size_t C>
    void move_assign_dispatch(varray<value_type, C> & other, std::true_type /*use_memop*/)
    {
        this->clear();

        ::memcpy(this->data(), other.data(), sizeof(Value) * other.m_size);
        std::swap(m_size, other.m_size);
    }

    // @par Throws
    //   @li If boost::has_nothrow_move<Value>::value is true and Value's move constructor or move assignment throws
    //   @li If boost::has_nothrow_move<Value>::value is false and Value's copy constructor or move assignment throws.
    // @par Complexity
    //   Linear O(N).
    template <std::size_t C>
    void move_assign_dispatch(varray<value_type, C> & other, std::false_type /*use_memop*/)
    {
        namespace sv = varray_detail;
        if ( m_size <= static_cast<size_type>(other.size()) )
        {
            sv::move_if_noexcept(other.begin(), other.begin() + m_size, this->begin());             // may throw
            // TODO - perform uninitialized_copy first?
            sv::uninitialized_move_if_noexcept(other.begin() + m_size, other.end(), this->end());   // may throw
        }
        else
        {
            sv::move_if_noexcept(other.begin(), other.end(), this->begin());                        // may throw
            sv::destroy(this->begin() + other.size(), this->end());
        }
        m_size = other.size(); // update end
    }

    // @par Throws
    //   Nothing.
    // @par Complexity
    //   Linear O(N).
    template <std::size_t C>
    void swap_dispatch(varray<value_type, C> & other, std::true_type /*use_optimized_swap*/)
    {
        typedef std::conditional_t
            <
                (Capacity < C),
                aligned_storage_type,
                typename varray<value_type, C>::aligned_storage_type
            > storage_type;
        
        storage_type temp;
        Value * temp_ptr = reinterpret_cast<Value*>(temp.address());

        ::memcpy(temp_ptr, this->data(), sizeof(Value) * this->size());
        ::memcpy(this->data(), other.data(), sizeof(Value) * other.size());
        ::memcpy(other.data(), temp_ptr, sizeof(Value) * this->size());

        std::swap(m_size, other.m_size);
    }

    // @par Throws
    //   If Value's move constructor or move assignment throws
    //   but only if use_memop_in_swap_and_move is false_type - default.
    // @par Complexity
    //   Linear O(N).
    template <std::size_t C>
    void swap_dispatch(varray<value_type, C> & other, std::false_type /*use_optimized_swap*/)
    {
        namespace sv = varray_detail;

        typedef typename
        vt::use_memop_in_swap_and_move use_memop_in_swap_and_move;

        if ( this->size() < other.size() )
            swap_dispatch_impl(this->begin(), this->end(), other.begin(), other.end(), use_memop_in_swap_and_move()); // may throw
        else
            swap_dispatch_impl(other.begin(), other.end(), this->begin(), this->end(), use_memop_in_swap_and_move()); // may throw
        std::swap(m_size, other.m_size);
    }

    // @par Throws
    //   Nothing.
    // @par Complexity
    //   Linear O(N).
    void swap_dispatch_impl(iterator first_sm, iterator last_sm, iterator first_la, iterator last_la, std::true_type /*use_memop*/)
    {
        //BOOST_GEOMETRY_INDEX_ASSERT(std::distance(first_sm, last_sm) <= std::distance(first_la, last_la),
        //                            "incompatible ranges");

        namespace sv = varray_detail;
        for (; first_sm != last_sm ; ++first_sm, ++first_la)
        {
            boost::aligned_storage<
                sizeof(value_type),
                boost::alignment_of<value_type>::value
            > temp_storage;
            value_type * temp_ptr = reinterpret_cast<value_type*>(temp_storage.address());

            ::memcpy(temp_ptr, boost::addressof(*first_sm), sizeof(value_type));
            ::memcpy(boost::addressof(*first_sm), boost::addressof(*first_la), sizeof(value_type));
            ::memcpy(boost::addressof(*first_la), temp_ptr, sizeof(value_type));
        }

        ::memcpy(first_sm, first_la, sizeof(value_type) * std::distance(first_la, last_la));
    }

    // @par Throws
    //   If Value's move constructor or move assignment throws.
    // @par Complexity
    //   Linear O(N).
    void swap_dispatch_impl(iterator first_sm, iterator last_sm, iterator first_la, iterator last_la, std::false_type /*use_memop*/)
    {
        //BOOST_GEOMETRY_INDEX_ASSERT(std::distance(first_sm, last_sm) <= std::distance(first_la, last_la),
        //                            "incompatible ranges");

        namespace sv = varray_detail;
        for (; first_sm != last_sm ; ++first_sm, ++first_la)
        {
            //boost::swap(*first_sm, *first_la);                                    // may throw
            value_type temp(boost::move(*first_sm));                                // may throw
            *first_sm = boost::move(*first_la);                                     // may throw
            *first_la = boost::move(temp);                                          // may throw
        }
        sv::uninitialized_move(first_la, last_la, first_sm);                        // may throw
        sv::destroy(first_la, last_la);
    }

    // insert

    // @par Throws
    //   If Value's move constructor, move assignment throws
    //   or if Value's copy constructor or copy assignment throws.
    // @par Complexity
    //   Linear O(N).
    template <typename Iterator>
    void insert_dispatch(iterator position, Iterator first, Iterator last, boost::random_access_traversal_tag const&)
    {
        BOOST_CONCEPT_ASSERT((boost_concepts::RandomAccessTraversal<Iterator>)); // Make sure you passed a RandomAccessIterator
        
        errh::check_iterator_end_eq(*this, position);
        
        typename boost::iterator_difference<Iterator>::type
            count = std::distance(first, last);

        errh::check_capacity(*this, m_size + count);                                             // may throw

        if ( position == this->end() )
        {
            namespace sv = varray_detail;

            sv::uninitialized_copy(first, last, position);                                      // may throw
            m_size += count; // update end
        }
        else
        {
            this->insert_in_the_middle(position, first, last, count);                           // may throw
        }
    }

    // @par Throws
    //   If Value's move constructor, move assignment throws
    //   or if Value's copy constructor or copy assignment throws.
    // @par Complexity
    //   Linear O(N).
    template <typename Iterator, typename Traversal>
    void insert_dispatch(iterator position, Iterator first, Iterator last, Traversal const& /*not_random_access*/)
    {
        errh::check_iterator_end_eq(*this, position);

        if ( position == this->end() )
        {
            namespace sv = varray_detail;

            std::ptrdiff_t d = std::distance(position, this->begin() + Capacity);
            std::size_t count = sv::uninitialized_copy_s(first, last, position, d);                     // may throw
            
            errh::check_capacity(*this, count <= static_cast<std::size_t>(d) ? m_size + count : Capacity + 1);  // may throw

            m_size += count;
        }
        else
        {
            typename boost::iterator_difference<Iterator>::type
                count = std::distance(first, last);
            
            errh::check_capacity(*this, m_size + count);                                                // may throw

            this->insert_in_the_middle(position, first, last, count);                                   // may throw
        }
    }

    // @par Throws
    //   If Value's move constructor, move assignment throws
    //   or if Value's copy constructor or copy assignment throws.
    // @par Complexity
    //   Linear O(N).
    template <typename Iterator>
    void insert_in_the_middle(iterator position, Iterator first, Iterator last, difference_type count)
    {
        namespace sv = varray_detail;

        difference_type to_move = std::distance(position, this->end());

        // TODO - should following lines check for exception and revert to the old size?

        if ( count < to_move )
        {
            sv::uninitialized_move(this->end() - count, this->end(), this->end());              // may throw
            m_size += count; // update end
            sv::move_backward(position, position + to_move - count, this->end() - count);       // may throw
            sv::copy(first, last, position);                                                    // may throw
        }
        else
        {
            Iterator middle_iter = first;
            std::advance(middle_iter, to_move);

            sv::uninitialized_copy(middle_iter, last, this->end());                             // may throw
            m_size += count - to_move; // update end
            sv::uninitialized_move(position, position + to_move, position + count);             // may throw
            m_size += to_move; // update end
            sv::copy(first, middle_iter, position);                                             // may throw
        }
    }

    // assign

    // @par Throws
    //   If Value's constructor or assignment taking dereferenced Iterator throws.
    // @par Complexity
    //   Linear O(N).
    template <typename Iterator>
    void assign_dispatch(Iterator first, Iterator last, boost::random_access_traversal_tag const& /*not_random_access*/)
    {
        namespace sv = varray_detail;

        typename boost::iterator_difference<Iterator>::type
            s = std::distance(first, last);

        errh::check_capacity(*this, s);                                     // may throw

        if ( m_size <= static_cast<size_type>(s) )
        {
            sv::copy(first, first + m_size, this->begin());                 // may throw
            // TODO - perform uninitialized_copy first?
            sv::uninitialized_copy(first + m_size, last, this->end());      // may throw
        }
        else
        {
            sv::copy(first, last, this->begin());                           // may throw
            sv::destroy(this->begin() + s, this->end());
        }
        m_size = s; // update end
    }

    // @par Throws
    //   If Value's constructor or assignment taking dereferenced Iterator throws.
    // @par Complexity
    //   Linear O(N).
    template <typename Iterator, typename Traversal>
    void assign_dispatch(Iterator first, Iterator last, Traversal const& /*not_random_access*/)
    {
        namespace sv = varray_detail;

        size_type s = 0;
        iterator it = this->begin();

        for ( ; it != this->end() && first != last ; ++it, ++first, ++s )
            *it = *first;                                                                                   // may throw

        sv::destroy(it, this->end());

        std::ptrdiff_t d = std::distance(it, this->begin() + Capacity);
        std::size_t count = sv::uninitialized_copy_s(first, last, it, d);                                   // may throw
        s += count;

        errh::check_capacity(*this, count <= static_cast<std::size_t>(d) ? s : Capacity + 1);               // may throw

        m_size = s; // update end
    }

    pointer ptr()
    {
        return pointer(static_cast<Value*>(m_storage.address()));
    }

    const_pointer ptr() const
    {
        return const_pointer(static_cast<const Value*>(m_storage.address()));
    }

    size_type m_size;
    aligned_storage_type m_storage;
};

#if !defined(BOOST_CONTAINER_DOXYGEN_INVOKED)

template<typename Value>
class varray<Value, 0>
{
    typedef varray_detail::varray_traits<Value, 0> vt;
    typedef varray_detail::checker<varray> errh;

public:
    typedef typename vt::value_type value_type;
    typedef typename vt::size_type size_type;
    typedef typename vt::difference_type difference_type;
    typedef typename vt::pointer pointer;
    typedef typename vt::const_pointer const_pointer;
    typedef typename vt::reference reference;
    typedef typename vt::const_reference const_reference;

    typedef pointer iterator;
    typedef const_pointer const_iterator;
    typedef boost::reverse_iterator<iterator> reverse_iterator;
    typedef boost::reverse_iterator<const_iterator> const_reverse_iterator;

    // nothrow
    varray() {}

    // strong
    explicit varray(size_type count)
    {
        errh::check_capacity(*this, count);                                         // may throw
    }

    // strong
    varray(size_type count, value_type const&)
    {
        errh::check_capacity(*this, count);                                         // may throw
    }

    // strong
    varray(varray const& /*other*/)
    {
        //errh::check_capacity(*this, count);
    }

    // strong
    template <std::size_t C>
    varray(varray<value_type, C> const& other)
    {
        errh::check_capacity(*this, other.size());                                  // may throw
    }

    // strong
    template <typename Iterator>
    varray(Iterator first, Iterator last)
    {
        errh::check_capacity(*this, std::distance(first, last));                    // may throw
    }

    // basic
    varray & operator=(varray const& /*other*/)
    {
        //errh::check_capacity(*this, other.size());
        return *this;
    }

    // basic
    template <size_t C>
    varray & operator=(varray<value_type, C> const& other)
    {
        errh::check_capacity(*this, other.size());                                  // may throw
        return *this;
    }

    // nothrow
    ~varray() {}

    // strong
    void resize(size_type count)
    {
        errh::check_capacity(*this, count);                                         // may throw
    }

    // strong
    void resize(size_type count, value_type const&)
    {
        errh::check_capacity(*this, count);                                         // may throw
    }

    
    // nothrow
    void reserve(size_type count)
    {
        errh::check_capacity(*this, count);                                         // may throw
    }

    // strong
    void push_back(value_type const&)
    {
        errh::check_capacity(*this, 1);                                             // may throw
    }

    // nothrow
    void pop_back()
    {
        errh::check_not_empty(*this);
    }

    // basic
    void insert(iterator position, value_type const&)
    {
        errh::check_iterator_end_eq(*this, position);
        errh::check_capacity(*this, 1);                                             // may throw
    }

    // basic
    void insert(iterator position, size_type count, value_type const&)
    {
        errh::check_iterator_end_eq(*this, position);
        errh::check_capacity(*this, count);                                         // may throw
    }

    // basic
    template <typename Iterator>
    void insert(iterator, Iterator first, Iterator last)
    {
        // TODO - add BOOST_GEOMETRY_STATIC_ASSERT, check if Iterator is really an iterator
        errh::check_capacity(*this, std::distance(first, last));                    // may throw
    }

    // basic
    void erase(iterator position)
    {
        errh::check_iterator_end_neq(*this, position);
    }

    // basic
    void erase(iterator first, iterator last)
    {
        errh::check_iterator_end_eq(*this, first);
        errh::check_iterator_end_eq(*this, last);

        //BOOST_GEOMETRY_INDEX_ASSERT(0 <= n, "invalid range");
    }

    // basic
    template <typename Iterator>
    void assign(Iterator first, Iterator last)
    {
        // TODO - add BOOST_GEOMETRY_STATIC_ASSERT, check if Iterator is really an iterator
        errh::check_capacity(*this, std::distance(first, last));                    // may throw
    }

    // basic
    void assign(size_type count, value_type const&)
    {
        errh::check_capacity(*this, count);                                     // may throw
    }

    // nothrow
    void clear() {}

    // strong
    reference at(size_type i)
    {
        errh::throw_out_of_bounds(*this, i);                                   // may throw
        return *(this->begin() + i);
    }

    // strong
    const_reference at(size_type i) const
    {
        errh::throw_out_of_bounds(*this, i);                                   // may throw
        return *(this->begin() + i);
    }

    // nothrow
    reference operator[](size_type i)
    {
        errh::check_index(*this, i);
        return *(this->begin() + i);
    }

    // nothrow
    const_reference operator[](size_type i) const
    {
        errh::check_index(*this, i);
        return *(this->begin() + i);
    }

    // nothrow
    reference front()
    {
        errh::check_not_empty(*this);
        return *(this->begin());
    }

    // nothrow
    const_reference front() const
    {
        errh::check_not_empty(*this);
        return *(this->begin());
    }

    // nothrow
    reference back()
    {
        errh::check_not_empty(*this);
        return *(this->end() - 1);
    }

    // nothrow
    const_reference back() const
    {
        errh::check_not_empty(*this);
        return *(this->end() - 1);
    }

    // nothrow
    Value * data() { return boost::addressof(*(this->ptr())); }
    const Value * data() const { return boost::addressof(*(this->ptr())); }

    // nothrow
    iterator begin() { return this->ptr(); }
    const_iterator begin() const { return this->ptr(); }
    const_iterator cbegin() const { return this->ptr(); }
    iterator end() { return this->begin(); }
    const_iterator end() const { return this->begin(); }
    const_iterator cend() const { return this->cbegin(); }
    // nothrow
    reverse_iterator rbegin() { return reverse_iterator(this->end()); }
    const_reverse_iterator rbegin() const { return reverse_iterator(this->end()); }
    const_reverse_iterator crbegin() const { return reverse_iterator(this->end()); }
    reverse_iterator rend() { return reverse_iterator(this->begin()); }
    const_reverse_iterator rend() const { return reverse_iterator(this->begin()); }
    const_reverse_iterator crend() const { return reverse_iterator(this->begin()); }

    // nothrow
    size_type capacity() const { return 0; }
    size_type max_size() const { return 0; }
    size_type size() const { return 0; }
    bool empty() const { return true; }

private:
    pointer ptr()
    {
        return pointer(reinterpret_cast<Value*>(this));
    }

    const_pointer ptr() const
    {
        return const_pointer(reinterpret_cast<const Value*>(this));
    }
};

#endif // !BOOST_CONTAINER_DOXYGEN_INVOKED

//! @brief Checks if contents of two varrays are equal.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if containers have the same size and elements in both containers are equal.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator== (varray<V, C1> const& x, varray<V, C2> const& y)
{
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}

//! @brief Checks if contents of two varrays are not equal.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if containers have different size or elements in both containers are not equal.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator!= (varray<V, C1> const& x, varray<V, C2> const& y)
{
    return !(x==y);
}

//! @brief Lexicographically compares varrays.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if x compares lexicographically less than y.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator< (varray<V, C1> const& x, varray<V, C2> const& y)
{
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

//! @brief Lexicographically compares varrays.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if y compares lexicographically less than x.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator> (varray<V, C1> const& x, varray<V, C2> const& y)
{
    return y<x;
}

//! @brief Lexicographically compares varrays.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if y don't compare lexicographically less than x.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator<= (varray<V, C1> const& x, varray<V, C2> const& y)
{
    return !(y<x);
}

//! @brief Lexicographically compares varrays.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if x don't compare lexicographically less than y.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator>= (varray<V, C1> const& x, varray<V, C2> const& y)
{
    return !(x<y);
}

//! @brief Swaps contents of two varrays.
//!
//! This function calls varray::swap().
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
inline void swap(varray<V, C1> & x, varray<V, C2> & y)
{
    x.swap(y);
}

}}}} // namespace boost::geometry::index::detail

// TODO - REMOVE/CHANGE
#include <boost/container/detail/config_end.hpp>

#endif // BOOST_GEOMETRY_INDEX_DETAIL_VARRAY_HPP
