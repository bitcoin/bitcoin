//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_ARRAY_HPP
#define BOOST_JSON_ARRAY_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/kind.hpp>
#include <boost/json/pilfer.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/detail/array.hpp>
#include <cstdlib>
#include <initializer_list>
#include <iterator>

BOOST_JSON_NS_BEGIN

#ifndef BOOST_JSON_DOCS
class value;
class value_ref;
#endif

/** A dynamically sized array of JSON values

    This is the type used to represent a JSON array as
    a modifiable container. The interface and performance
    characteristics are modeled after `std::vector<value>`.
\n
    Elements are stored contiguously, which means that
    they can be accessed not only through iterators, but
    also using offsets to regular pointers to elements. A
    pointer to an element of an @ref array may be passed to
    any function that expects a pointer to @ref value.
\n
    The storage of the array is handled automatically, being
    expanded and contracted as needed. Arrays usually occupy
    more space than array language constructs, because more
    memory is allocated to handle future growth. This way an
    array does not need to reallocate each time an element
    is inserted, but only when the additional memory is used
    up. The total amount of allocated memory can be queried
    using the @ref capacity function. Extra memory can be
    relinquished by calling @ref shrink_to_fit.
    \n

    Reallocations are usually costly operations in terms of
    performance. The @ref reserve function can be used to
    eliminate reallocations if the number of elements is
    known beforehand.
\n
    The complexity (efficiency) of common operations on
    arrays is as follows:

    @li Random access - constant *O(1)*.
    @li Insertion or removal of elements at the
        end - amortized constant *O(1)*.
    @li Insertion or removal of elements - linear in
        the distance to the end of the array *O(n)*.

    @par Allocators

    All elements stored in the container, and their
    children if any, will use the same memory resource
    that was used to construct the container.

    @par Thread Safety

    Non-const member functions may not be called
    concurrently with any other member functions.

    @par Satisfies
        <a href="https://en.cppreference.com/w/cpp/named_req/ContiguousContainer"><em>ContiguousContainer</em></a>,
        <a href="https://en.cppreference.com/w/cpp/named_req/ReversibleContainer"><em>ReversibleContainer</em></a>, and
        <a href="https://en.cppreference.com/w/cpp/named_req/SequenceContainer"><em>SequenceContainer</em></a>.
*/
class array
{
    struct table;
    class revert_construct;
    class revert_insert;
    friend class value;

    storage_ptr sp_;        // must come first
    kind k_ = kind::array;  // must come second
    table* t_;

    BOOST_JSON_DECL
    static table empty_;

    inline
    static
    void
    relocate(
        value* dest,
        value* src,
        std::size_t n) noexcept;

    inline
    void
    destroy(
        value* first,
        value* last) noexcept;

    BOOST_JSON_DECL
    void
    destroy() noexcept;

    BOOST_JSON_DECL
    explicit
    array(detail::unchecked_array&& ua);

public:
    /** The type of <em>Allocator</em> returned by @ref get_allocator

        This type is a @ref polymorphic_allocator.
    */
#ifdef BOOST_JSON_DOCS
    // VFALCO doc toolchain renders this incorrectly
    using allocator_type = __see_below__;
#else
    using allocator_type = polymorphic_allocator<value>;
#endif

    /// The type used to represent unsigned integers
    using size_type = std::size_t;

    /// The type of each element
    using value_type = value;

    /// The type used to represent signed integers
    using difference_type = std::ptrdiff_t;

    /// A reference to an element
    using reference = value&;

    /// A const reference to an element
    using const_reference = value const&;

    /// A pointer to an element
    using pointer = value*;

    /// A const pointer to an element
    using const_pointer = value const*;

    /// A random access iterator to an element
    using iterator = value*;

    /// A random access const iterator to an element
    using const_iterator = value const*;

    /// A reverse random access iterator to an element
    using reverse_iterator =
        std::reverse_iterator<iterator>;

    /// A reverse random access const iterator to an element
    using const_reverse_iterator =
        std::reverse_iterator<const_iterator>;

    //------------------------------------------------------

    /** Destructor.

        The destructor for each element is called if needed,
        any used memory is deallocated, and shared ownership
        of the @ref memory_resource is released.

        @par Complexity
        Constant, or linear in @ref size().

        @par Exception Safety
        No-throw guarantee.
    */
    BOOST_JSON_DECL
    ~array();

    //------------------------------------------------------

    /** Constructor.

        The constructed array is empty with zero
        capacity, using the default memory resource.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    array() noexcept
        : t_(&empty_)
    {
    }

    /** Constructor.

        The constructed array is empty with zero
        capacity, using the specified memory resource.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param sp A pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
    */
    explicit
    array(storage_ptr sp) noexcept
        : sp_(std::move(sp))
        , k_(kind::array)
        , t_(&empty_)
    {
    }

    /** Constructor.

        The array is constructed with `count`
        copies of the value `v`, using the
        specified memory resource.

        @par Complexity
        Linear in `count`

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param count The number of copies to insert.

        @param v The value to be inserted.

        @param sp A pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
    */
    BOOST_JSON_DECL
    array(
        std::size_t count,
        value const& v,
        storage_ptr sp = {});

    /** Constructor.

        The array is constructed with `count` null values,
        using the specified memory resource.

        @par Complexity
        Linear in `count`

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param count The number of nulls to insert.

        @param sp A pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
    */
    BOOST_JSON_DECL
    array(
        std::size_t count,
        storage_ptr sp = {});

    /** Constructor.

        The array is constructed with the elements
        in the range `{first, last)`, preserving order,
        using the specified memory resource.

        @par Constraints

        @code
        std::is_constructible_v<value, std::iterator_traits<InputIt>::value_type>
        @endcode

        @par Complexity
        Linear in `std::distance(first, last)`

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param first An input iterator pointing to the
        first element to insert, or pointing to the end
        of the range.

        @param last An input iterator pointing to the end
        of the range.

        @param sp A pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.

        @tparam InputIt a type satisfying the requirements
        of __InputIterator__.
    */
    template<
        class InputIt
    #ifndef BOOST_JSON_DOCS
        ,class = typename std::enable_if<
            std::is_constructible<value,
                typename std::iterator_traits<
                    InputIt>::value_type>::value>::type
    #endif
    >
    array(
        InputIt first, InputIt last,
        storage_ptr sp = {});

    /** Copy constructor.

        The array is constructed with a copy of the
        contents of `other`, using `other`'s memory resource.

        @par Complexity
        Linear in `other.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param other The array to copy
    */
    BOOST_JSON_DECL
    array(array const& other);

    /** Copy constructor.

        The array is constructed with a copy of the
        contents of `other`, using the specified memory resource.

        @par Complexity
        Linear in `other.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param other The array to copy

        @param sp A pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
    */
    BOOST_JSON_DECL
    array(
        array const& other,
        storage_ptr sp);

    /** Pilfer constructor.

        The array is constructed by acquiring ownership
        of the contents of `other` using pilfer semantics.
        This is more efficient than move construction, when
        it is known that the moved-from object will be
        immediately destroyed afterwards.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param other The value to pilfer. After pilfer
        construction, `other` is not in a usable state
        and may only be destroyed.

        @see @ref pilfer,
            <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0308r0.html">
                Valueless Variants Considered Harmful</a>
    */
    array(pilfered<array> other) noexcept
        : sp_(std::move(other.get().sp_))
        , t_(detail::exchange(
            other.get().t_, &empty_))
    {
    }

    /** Move constructor.

        The array is constructed by acquiring ownership of
        the contents of `other` and shared ownership of
        `other`'s memory resource.

        @note

        After construction, the moved-from array behaves
        as if newly constructed with its current storage
        pointer.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param other The container to move
    */
    array(array&& other) noexcept
        : sp_(other.sp_)
        , t_(detail::exchange(
            other.t_, &empty_))
    {
    }

    /** Move constructor.

        The array is constructed with the contents of
        `other` by move semantics, using the specified
        memory resource:

        @li If `*other.storage() == *sp`, ownership of
        the underlying memory is transferred in constant
        time, with no possibility of exceptions.
        After construction, the moved-from array behaves
        as if newly constructed with its current storage
        pointer.

        @li If `*other.storage() != *sp`, an
        element-wise copy is performed, which may throw.
        In this case, the moved-from array is not
        changed.

        @par Complexity
        At most, linear in `other.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param other The container to move

        @param sp A pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
    */
    BOOST_JSON_DECL
    array(
        array&& other,
        storage_ptr sp);

    /** Constructor.

        The array is constructed with a copy of the values
        in the initializer-list in order, using the
        specified memory resource.

        @par Complexity
        Linear in `init.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param init The initializer list to insert

        @param sp A pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
    */
    BOOST_JSON_DECL
    array(
        std::initializer_list<value_ref> init,
        storage_ptr sp = {});

    //------------------------------------------------------

    /** Copy assignment.

        The contents of the array are replaced with an
        element-wise copy of `other`.

        @par Complexity
        Linear in @ref size() plus `other.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param other The array to copy.
    */
    BOOST_JSON_DECL
    array&
    operator=(array const& other);

    /** Move assignment.

        The contents of the array are replaced with the
        contents of `other` using move semantics:

        @li If `*other.storage() == *sp`, ownership of
        the underlying memory is transferred in constant
        time, with no possibility of exceptions.
        After assignment, the moved-from array behaves
        as if newly constructed with its current storage
        pointer.

        @li If `*other.storage() != *sp`, an
        element-wise copy is performed, which may throw.
        In this case, the moved-from array is not
        changed.

        @par Complexity
        Constant, or linear in
        `this->size()` plus `other.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param other The array to move.
    */
    BOOST_JSON_DECL
    array&
    operator=(array&& other);

    /** Assignment.

        The contents of the array are replaced with a
        copy of the values in the initializer-list.

        @par Complexity
        Linear in `this->size()` plus `init.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param init The initializer list to copy.
    */
    BOOST_JSON_DECL
    array&
    operator=(
        std::initializer_list<value_ref> init);

    //------------------------------------------------------

    /** Return the associated @ref memory_resource

        This returns the @ref memory_resource used by
        the container.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    storage_ptr const&
    storage() const noexcept
    {
        return sp_;
    }

    /** Return the associated @ref memory_resource

        This function returns an instance of
        @ref polymorphic_allocator constructed from the
        associated @ref memory_resource.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    allocator_type
    get_allocator() const noexcept
    {
        return sp_.get();
    }

    //------------------------------------------------------
    //
    // Element access
    //
    //------------------------------------------------------

    /** Access an element, with bounds checking.

        Returns a reference to the element specified at
        location `pos`, with bounds checking. If `pos` is
        not within the range of the container, an exception
        of type `std::out_of_range` is thrown.

        @par Complexity
        Constant.

        @param pos A zero-based index.

        @throw std::out_of_range `pos >= size()`
    */
    inline
    value&
    at(std::size_t pos);

    /** Access an element, with bounds checking.

        Returns a reference to the element specified at
        location `pos`, with bounds checking. If `pos` is
        not within the range of the container, an exception
        of type `std::out_of_range` is thrown.

        @par Complexity
        Constant.

        @param pos A zero-based index.

        @throw std::out_of_range `pos >= size()`
    */
    inline
    value const&
    at(std::size_t pos) const;

    /** Access an element.

        Returns a reference to the element specified at
        location `pos`. No bounds checking is performed.

        @par Precondition
        `pos < size()`

        @par Complexity
        Constant.

        @param pos A zero-based index
    */
    inline
    value&
    operator[](std::size_t pos) noexcept;

    /** Access an element.

        Returns a reference to the element specified at
        location `pos`. No bounds checking is performed.

        @par Precondition
        `pos < size()`

        @par Complexity
        Constant.

        @param pos A zero-based index
    */
    inline
    value const&
    operator[](std::size_t pos) const noexcept;

    /** Access the first element.

        Returns a reference to the first element.

        @par Precondition
        `not empty()`

        @par Complexity
        Constant.
    */
    inline
    value&
    front() noexcept;

    /** Access the first element.

        Returns a reference to the first element.

        @par Precondition
        `not empty()`

        @par Complexity
        Constant.
    */
    inline
    value const&
    front() const noexcept;

    /** Access the last element.

        Returns a reference to the last element.

        @par Precondition
        `not empty()`

        @par Complexity
        Constant.
    */
    inline
    value&
    back() noexcept;

    /** Access the last element.

        Returns a reference to the last element.

        @par Precondition
        `not empty()`

        @par Complexity
        Constant.
    */
    inline
    value const&
    back() const noexcept;

    /** Access the underlying array directly.

        Returns a pointer to the underlying array serving
        as element storage. The value returned is such that
        the range `{data(), data() + size())` is always a
        valid range, even if the container is empty.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @note

        If `size() == 0`, the function may or may not return
        a null pointer.
    */
    inline
    value*
    data() noexcept;

    /** Access the underlying array directly.

        Returns a pointer to the underlying array serving
        as element storage. The value returned is such that
        the range `{data(), data() + size())` is always a
        valid range, even if the container is empty.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @note

        If `size() == 0`, the function may or may not return
        a null pointer.
    */
    inline
    value const*
    data() const noexcept;

    /** Return a pointer to an element, or nullptr if the index is invalid

        This function returns a pointer to the element
        at index `pos` when the index is less then the size
        of the container. Otherwise it returns null.

        @par Example
        @code
        if( auto p = arr.if_contains( 1 ) )
            std::cout << *p;
        @endcode

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param pos The index of the element to return.
    */
    inline
    value const*
    if_contains(std::size_t pos) const noexcept;

    /** Return a pointer to an element, or nullptr if the index is invalid

        This function returns a pointer to the element
        at index `pos` when the index is less then the size
        of the container. Otherwise it returns null.

        @par Example
        @code
        if( auto p = arr.if_contains( 1 ) )
            std::cout << *p;
        @endcode

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param pos The index of the element to return.
    */
    inline
    value*
    if_contains(std::size_t pos) noexcept;

    //------------------------------------------------------
    //
    // Iterators
    //
    //------------------------------------------------------

    /** Return an iterator to the first element.

        If the container is empty, @ref end() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    iterator
    begin() noexcept;

    /** Return a const iterator to the first element.

        If the container is empty, @ref end() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    const_iterator
    begin() const noexcept;

    /** Return a const iterator to the first element.

        If the container is empty, @ref cend() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    const_iterator
    cbegin() const noexcept;

    /** Return an iterator to the element following the last element.

        The element acts as a placeholder; attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    iterator
    end() noexcept;

    /** Return a const iterator to the element following the last element.

        The element acts as a placeholder; attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    const_iterator
    end() const noexcept;

    /** Return a const iterator to the element following the last element.

        The element acts as a placeholder; attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    const_iterator
    cend() const noexcept;

    /** Return a reverse iterator to the first element of the reversed container.

        The pointed-to element corresponds to the
        last element of the non-reversed container.
        If the container is empty, @ref rend() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    reverse_iterator
    rbegin() noexcept;

    /** Return a const reverse iterator to the first element of the reversed container.

        The pointed-to element corresponds to the
        last element of the non-reversed container.
        If the container is empty, @ref rend() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    const_reverse_iterator
    rbegin() const noexcept;

    /** Return a const reverse iterator to the first element of the reversed container.

        The pointed-to element corresponds to the
        last element of the non-reversed container.
        If the container is empty, @ref crend() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    const_reverse_iterator
    crbegin() const noexcept;

    /** Return a reverse iterator to the element following the last element of the reversed container.

        The pointed-to element corresponds to the element
        preceding the first element of the non-reversed container.
        The element acts as a placeholder; attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    reverse_iterator
    rend() noexcept;

    /** Return a const reverse iterator to the element following the last element of the reversed container.

        The pointed-to element corresponds to the element
        preceding the first element of the non-reversed container.
        The element acts as a placeholder; attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    const_reverse_iterator
    rend() const noexcept;

    /** Return a const reverse iterator to the element following the last element of the reversed container.

        The pointed-to element corresponds to the element
        preceding the first element of the non-reversed container.
        The element acts as a placeholder; attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    const_reverse_iterator
    crend() const noexcept;

    //------------------------------------------------------
    //
    // Capacity
    //
    //------------------------------------------------------

    /** Return the number of elements in the array.

        This returns the number of elements in the array.
        The value returned may be different from the number
        returned from @ref capacity.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    std::size_t
    size() const noexcept;

    /** Return the maximum number of elements any array can hold.

        The maximum is an implementation-defined number.
        This value is a theoretical limit; at runtime,
        the actual maximum size may be less due to
        resource limits.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    static
    inline
    constexpr
    std::size_t
    max_size() noexcept;

    /** Return the number of elements that can be held in currently allocated memory.

        This number may be larger than the value returned
        by @ref size().

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    std::size_t
    capacity() const noexcept;

    /** Check if the array has no elements.

        Returns `true` if there are no elements in the
        array, i.e. @ref size() returns 0.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    inline
    bool
    empty() const noexcept;

    /** Increase the capacity to at least a certain amount.

        This increases the @ref capacity() to a value
        that is greater than or equal to `new_capacity`.
        If `new_capacity > capacity()`, new memory is
        allocated. Otherwise, the call has no effect.
        The number of elements and therefore the
        @ref size() of the container is not changed.
    \n
        If new memory is allocated, all iterators
        including any past-the-end iterators, and all
        references to the elements are invalidated.
        Otherwise, no iterators or references are
        invalidated.

        @par Complexity
        At most, linear in @ref size().

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param new_capacity The new capacity of the array.

        @throw std::length_error `new_capacity > max_size()`
    */
    inline
    void
    reserve(std::size_t new_capacity);

    /** Request the removal of unused capacity.

        This performs a non-binding request to reduce the
        capacity to the current size. The request may or
        may not be fulfilled. If reallocation occurs, all
        iterators including any past-the-end iterators,
        and all references to the elements are invalidated.
        Otherwise, no iterators or references are
        invalidated.

        @par Complexity
        At most, linear in @ref size().

        @par Exception Safety
        No-throw guarantee.
    */
    BOOST_JSON_DECL
    void
    shrink_to_fit() noexcept;

    //------------------------------------------------------
    //
    // Modifiers
    //
    //------------------------------------------------------

    /** Clear the contents.

        Erases all elements from the container. After this
        call, @ref size() returns zero but @ref capacity()
        is unchanged. All references, pointers, or iterators
        referring to contained elements are invalidated. Any
        past-the-end iterators are also invalidated.

        @par Complexity
        Linear in @ref size().

        @par Exception Safety
        No-throw guarantee.
    */
    BOOST_JSON_DECL
    void
    clear() noexcept;

    /** Insert elements before the specified location.

        This inserts a copy of `v` before `pos`.
        If `capacity() < size() + 1`, a reallocation
        occurs first, and all iterators and references
        are invalidated.
        Otherwise, only the iterators and references from
        the insertion point forward are invalidated. All
        past-the-end iterators are also invalidated.

        @par Complexity
        Constant plus linear in `std::distance(pos, end())`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param pos Iterator before which the content will
        be inserted. This may be the @ref end() iterator.

        @param v The value to insert. A copy will be made
        using container's associated @ref memory_resource.

        @return An iterator to the inserted value
    */
    BOOST_JSON_DECL
    iterator
    insert(
        const_iterator pos,
        value const& v);

    /** Insert elements before the specified location.

        This inserts `v` before `pos` via move-construction.
        If `capacity() < size() + 1`, a reallocation occurs
        first, and all iterators and references are
        invalidated.
        Otherwise, only the iterators and references from
        the insertion point forward are invalidated. All
        past-the-end iterators are also invalidated.

        @par Complexity
        Constant plus linear in `std::distance(pos, end())`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param pos Iterator before which the content will
        be inserted. This may be the @ref end() iterator.

        @param v The value to insert. Ownership of the
        value will be transferred via move construction,
        using the container's associated @ref memory_resource.

        @return An iterator to the inserted value
    */
    BOOST_JSON_DECL
    iterator
    insert(
        const_iterator pos,
        value&& v);

    /** Insert elements before the specified location.

        This inserts `count` copies of `v` before `pos`.
        If `capacity() < size() + count`, a reallocation
        occurs first, and all iterators and references are
        invalidated.
        Otherwise, only the iterators and references from
        the insertion point forward are invalidated. All
        past-the-end iterators are also invalidated.

        @par Complexity
        Linear in `count + std::distance(pos, end())`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param pos Iterator before which the content will
        be inserted. This may be the @ref end() iterator.

        @param count The number of copies to insert.

        @param v The value to insert. Copies will be made
        using container's associated @ref memory_resource.

        @return An iterator to the first inserted value,
        or `pos` if `count == 0`.
    */
    BOOST_JSON_DECL
    iterator
    insert(
        const_iterator pos,
        std::size_t count,
        value const& v);

    /** Insert elements before the specified location.

        The elements in the range `{first, last)` are
        inserted in order.
        If `capacity() < size() + std::distance(first, last)`,
        a reallocation occurs first, and all iterators and
        references are invalidated.
        Otherwise, only the iterators and references from
        the insertion point forward are invalidated. All
        past-the-end iterators are also invalidated.

        @par Precondition
        `first` and `last` are not iterators into `*this`.

        @par Constraints
        @code
        not std::is_convertible_v<InputIt, value>
        @endcode

        @par Mandates
        @code
        std::is_constructible_v<value, std::iterator_traits<InputIt>::value_type>
        @endcode

        @par Complexity
        Linear in `std::distance(first, last) + std::distance(pos, end())`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @return An iterator to the first inserted value, or
        `pos` if `first == last`.

        @param pos Iterator before which the content will
        be inserted. This may be the @ref end() iterator.

        @param first An input iterator pointing to the first
        element to insert, or pointing to the end of the range.

        @param last An input iterator pointing to the end
        of the range.

        @tparam InputIt a type satisfying the requirements
        of __InputIterator__.
    */
    template<
        class InputIt
    #ifndef BOOST_JSON_DOCS
        ,class = typename std::enable_if<
            std::is_constructible<value,
                typename std::iterator_traits<
                    InputIt>::value_type>::value>::type
    #endif
    >
    iterator
    insert(
        const_iterator pos,
        InputIt first, InputIt last);

    /** Insert elements before the specified location.

        The elements in the initializer list `init` are
        inserted in order.
        If `capacity() < size() + init.size()`,
        a reallocation occurs first, and all iterators and
        references are invalidated.
        Otherwise, only the iterators and references from
        the insertion point forward are invalidated. All
        past-the-end iterators are also invalidated.

        @par Complexity
        Linear in `init.size() + std::distance(pos, end())`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param pos Iterator before which the content will
        be inserted. This may be the @ref end() iterator.

        @param init The initializer list to insert

        @return An iterator to the first inserted value, or
        `pos` if `init.size() == 0`.
    */
    BOOST_JSON_DECL
    iterator
    insert(
        const_iterator pos,
        std::initializer_list<value_ref> init);

    /** Insert a constructed element in-place.

        Inserts a new element into the container directly before
        `pos`. The element is constructed using placement-new
        with the parameter `std::forward<Arg>(arg)`.
        If `capacity() < size() + 1`,
        a reallocation occurs first, and all iterators and
        references are invalidated.
        Otherwise, only the iterators and references from
        the insertion point forward are invalidated. All
        past-the-end iterators are also invalidated.

        @par Complexity
        Constant plus linear in `std::distance(pos, end())`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param pos Iterator before which the element will
        be inserted. This may be the @ref end() iterator.

        @param arg The argument to forward to the @ref value
        constructor.

        @return An iterator to the inserted element
    */
    template<class Arg>
    iterator
    emplace(
        const_iterator pos,
        Arg&& arg);

    /** Erase elements from the container.

        The element at `pos` is removed.

        @par Complexity
        Constant plus linear in `std::distance(pos, end())`

        @par Exception Safety
        No-throw guarantee.

        @param pos Iterator to the element to remove

        @return Iterator following the last removed element.
        If the iterator `pos` refers to the last element,
        the @ref end() iterator is returned.
    */
    BOOST_JSON_DECL
    iterator
    erase(const_iterator pos) noexcept;

    /** Erase elements from the container.

        The elements in the range `{first, last)` are removed.

        @par Complexity
        Linear in `std::distance(first, last) + std::distance(pos, end())`

        @par Exception Safety
        No-throw guarantee.

        @param first An iterator pointing to the first
        element to erase, or pointing to the end of the range.

        @param last An iterator pointing to one past the
        last element to erase, or pointing to the end of the
        range.

        @return Iterator following the last removed element.
        If the iterator `pos` refers to the last element,
        the @ref end() iterator is returned.
    */
    BOOST_JSON_DECL
    iterator
    erase(
        const_iterator first,
        const_iterator last) noexcept;

    /** Add an element to the end.

        This appends a copy of `v` to the container's
        elements.
        If `capacity() < size() + 1`, a reallocation
        occurs first, and all iterators and references
        are invalidated. Any past-the-end iterators are
        always invalidated.

        @par Complexity
        Amortized constant.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param v The value to insert. A copy will be made
        using container's associated @ref memory_resource.
    */
    BOOST_JSON_DECL
    void
    push_back(value const& v);

    /** Add an element to the end.

        This appends `v` to the container's elements via
        move-construction.
        If `capacity() < size() + 1`, a reallocation
        occurs first, and all iterators and references
        are invalidated. Any past-the-end iterators are
        always invalidated.

        @par Complexity
        Amortized constant.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param v The value to insert. Ownership of the
        value will be transferred via move construction,
        using the container's associated @ref memory_resource.
    */
    BOOST_JSON_DECL
    void
    push_back(value&& v);

    /** Append a constructed element in-place.

        Appends a new element to the end of the container's
        list of elements.
        The element is constructed using placement-new
        with the parameter `std::forward<Arg>(arg)`.
        If `capacity() < size() + 1`,
        a reallocation occurs first, and all iterators and
        references are invalidated.
        Otherwise, only the iterators and references from
        the insertion point forward are invalidated. All
        past-the-end iterators are also invalidated.

        @par Complexity
        Amortized constant.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param arg The argument to forward to the @ref value
        constructor.

        @return A reference to the inserted element
    */
    template<class Arg>
    value&
    emplace_back(Arg&& arg);

    /** Remove the last element

        The last element of the container is erased.

        @par Precondition
        `not empty()`

        @par Exception Safety
        No-throw guarantee.
    */
    BOOST_JSON_DECL
    void
    pop_back() noexcept;

    /** Change the number of elements stored.

        Resizes the container to contain `count` elements.
        If `capacity() < size() + count`, a reallocation
        occurs first, and all iterators and references
        are invalidated. Any past-the-end iterators are
        always invalidated.

        @li If `size() > count`, the container is reduced
        to its first `count` elements.

        @li If `size() < count`, additional null values
        are appended.

        @par Complexity
        Linear in `abs(size() - count)`, plus the cost of
        reallocation if @ref capacity() is less than `count`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param count The new size of the container.
    */
    BOOST_JSON_DECL
    void
    resize(std::size_t count);

    /** Change the number of elements stored.

        Resizes the container to contain `count` elements.
        If `capacity() < size() + count`, a reallocation
        occurs first, and all iterators and references
        are invalidated. Any past-the-end iterators are
        always invalidated.

        @li If `size() > count`, the container is reduced
        to its first `count` elements.

        @li If `size() < count`, additional copies of `v`
        are appended.

        @par Complexity
        Linear in `abs(size() - count)`, plus the cost of
        reallocation if @ref capacity() is less than `count`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param count The new size of the container.

        @param v The @ref value to copy into the new elements.
    */
    BOOST_JSON_DECL
    void
    resize(
        std::size_t count,
        value const& v);

    /** Swap the contents.

        Exchanges the contents of this array with another
        array. Ownership of the respective @ref memory_resource
        objects is not transferred.

        @li If `*other.storage() == *this->storage()`,
        ownership of the underlying memory is swapped in
        constant time, with no possibility of exceptions.
        All iterators and references remain valid.

        @li If `*other.storage() != *this->storage()`,
        the contents are logically swapped by making copies,
        which can throw. In this case all iterators and
        references are invalidated.

        @par Complexity
        Constant or linear in @ref size() plus `other.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param other The value to swap with.
        If `this == &other`, this function call has no effect.
    */
    BOOST_JSON_DECL
    void
    swap(array& other);

    /** Exchange the given values.

        Exchanges the contents of the array `lhs` with
        another array `rhs`. Ownership of the respective
        @ref memory_resource objects is not transferred.

        @li If `*lhs.storage() == *rhs.storage()`,
        ownership of the underlying memory is swapped in
        constant time, with no possibility of exceptions.
        All iterators and references remain valid.

        @li If `*lhs.storage() != *rhs.storage()`,
        the contents are logically swapped by making a copy,
        which can throw. In this case all iterators and
        references are invalidated.

        @par Effects
        @code
        lhs.swap( rhs );
        @endcode

        @par Complexity
        Constant or linear in `lhs.size() + rhs.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param lhs The array to exchange.

        @param rhs The array to exchange.
        If `&lhs == &rhs`, this function call has no effect.

        @see @ref array::swap
    */
    friend
    void
    swap(array& lhs, array& rhs)
    {
        lhs.swap(rhs);
    }

    /** Return `true` if two arrays are equal.

        Arrays are equal when their sizes are
        the same, and they are element-for-element
        equal in order.

        @par Effects
        `return std::equal( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );`

        @par Complexity
        Constant or linear in `lhs.size()`.

        @par Exception Safety
        No-throw guarantee.
    */
    // inline friend speeds up overload resolution
    friend
    bool
    operator==(
        array const& lhs,
        array const& rhs) noexcept
    {
        return lhs.equal(rhs);
    }

    /** Return `true` if two arrays are not equal.

        Arrays are equal when their sizes are
        the same, and they are element-for-element
        equal in order.

        @par Effects
        `return ! std::equal( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );`

        @par Complexity
        Constant or linear in `lhs.size()`.

        @par Exception Safety
        No-throw guarantee.
    */
    // inline friend speeds up overload resolution
    friend
    bool
    operator!=(
        array const& lhs,
        array const& rhs) noexcept
    {
        return ! (lhs == rhs);
    }

private:
    template<class It>
    using iter_cat = typename
        std::iterator_traits<It>::iterator_category;

    template<class InputIt>
    array(
        InputIt first, InputIt last,
        storage_ptr sp,
        std::input_iterator_tag);

    template<class InputIt>
    array(
        InputIt first, InputIt last,
        storage_ptr sp,
        std::forward_iterator_tag);

    inline
    std::size_t
    growth(std::size_t new_size) const;

    BOOST_JSON_DECL
    void
    reserve_impl(
        std::size_t new_capacity);

    BOOST_JSON_DECL
    value&
    push_back(
        pilfered<value> pv);

    BOOST_JSON_DECL
    iterator
    insert(
        const_iterator pos,
        pilfered<value> pv);

    template<class InputIt>
    iterator
    insert(
        const_iterator pos,
        InputIt first, InputIt last,
        std::input_iterator_tag);

    template<class InputIt>
    iterator
    insert(
        const_iterator pos,
        InputIt first, InputIt last,
        std::forward_iterator_tag);

    BOOST_JSON_DECL
    bool
    equal(array const& other) const noexcept;
};

BOOST_JSON_NS_END

// std::hash specialization
#ifndef BOOST_JSON_DOCS
namespace std {
template <>
struct hash< ::boost::json::array > {
    BOOST_JSON_DECL
    std::size_t
    operator()(::boost::json::array const& ja) const noexcept;
};
} // std
#endif

// Must be included here for this file to stand alone
#include <boost/json/value.hpp>

// includes are at the bottom of <boost/json/value.hpp>

#endif
