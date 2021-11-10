//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_STRING_HPP
#define BOOST_JSON_STRING_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/pilfer.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/string_view.hpp>
#include <boost/json/detail/digest.hpp>
#include <boost/json/detail/except.hpp>
#include <boost/json/detail/string_impl.hpp>
#include <boost/json/detail/value.hpp>
#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

BOOST_JSON_NS_BEGIN

class value;

/** The native type of string values.

    Instances of string store and manipulate sequences
    of `char` using the UTF-8 encoding. The elements of
    a string are stored contiguously. A pointer to any
    character in a string may be passed to functions
    that expect a pointer to the first element of a
    null-terminated `char` array.

    String iterators are regular `char` pointers.

    @note `string` member functions do not validate
    any UTF-8 byte sequences passed to them.

    @par Thread Safety

    Non-const member functions may not be called
    concurrently with any other member functions.

    @par Satisfies
        <a href="https://en.cppreference.com/w/cpp/named_req/ContiguousContainer"><em>ContiguousContainer</em></a>,
        <a href="https://en.cppreference.com/w/cpp/named_req/ReversibleContainer"><em>ReversibleContainer</em></a>, and
        <a href="https://en.cppreference.com/w/cpp/named_req/SequenceContainer"><em>SequenceContainer</em></a>.
*/
class string
{
    friend class value;
#ifndef BOOST_JSON_DOCS
    // VFALCO doc toolchain shouldn't show this but does
    friend struct detail::access;
#endif

    using string_impl = detail::string_impl;

    inline
    string(
        detail::key_t const&,
        string_view s,
        storage_ptr sp);

    inline
    string(
        detail::key_t const&,
        string_view s1,
        string_view s2,
        storage_ptr sp);

public:
    /** The type of _Allocator_ returned by @ref get_allocator

        This type is a @ref polymorphic_allocator.
    */
#ifdef BOOST_JSON_DOCS
    // VFALCO doc toolchain renders this incorrectly
    using allocator_type = __see_below__;
#else
    using allocator_type = polymorphic_allocator<value>;
#endif

    /// The type of a character
    using value_type        = char;

    /// The type used to represent unsigned integers
    using size_type         = std::size_t;

    /// The type used to represent signed integers
    using difference_type   = std::ptrdiff_t;

    /// A pointer to an element
    using pointer           = char*;

    /// A const pointer to an element
    using const_pointer     = char const*;

    /// A reference to an element
    using reference         = char&;

    /// A const reference to an element
    using const_reference   = const char&;

    /// A random access iterator to an element
    using iterator          = char*;

    /// A random access const iterator to an element
    using const_iterator    = char const*;

    /// A reverse random access iterator to an element
    using reverse_iterator =
        std::reverse_iterator<iterator>;

    /// A reverse random access const iterator to an element
    using const_reverse_iterator =
        std::reverse_iterator<const_iterator>;

    /// A special index
    static constexpr std::size_t npos =
        string_view::npos;

private:
    template<class T>
    using is_inputit = typename std::enable_if<
        std::is_convertible<typename
            std::iterator_traits<T>::value_type,
            char>::value>::type;

    storage_ptr sp_; // must come first
    string_impl impl_;

public:
    /** Destructor.

        Any dynamically allocated internal storage
        is freed.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    ~string()
    {
        impl_.destroy(sp_);
    }

    //------------------------------------------------------
    //
    // Construction
    //
    //------------------------------------------------------

    /** Default constructor.

        The string will have a zero size and a non-zero,
        unspecified capacity, using the default memory resource.

        @par Complexity

        Constant.
    */
    string() = default;

    /** Pilfer constructor.

        The string is constructed by acquiring ownership
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
    string(pilfered<string> other) noexcept
        : sp_(std::move(other.get().sp_))
        , impl_(other.get().impl_)
    {
        ::new(&other.get().impl_) string_impl();
    }

    /** Constructor.

        The string will have zero size and a non-zero,
        unspecified capacity, obtained from the specified
        memory resource.

        @par Complexity

        Constant.

        @param sp A pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
    */
    explicit
    string(storage_ptr sp)
        : sp_(std::move(sp))
    {
    }

    /** Constructor.

        Construct the contents with `count` copies of
        character `ch`.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param count The size of the resulting string.

        @param ch The value to initialize characters
        of the string with.

        @param sp An optional pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
        The default argument for this parameter is `{}`.

        @throw std::length_error `count > max_size()`.
    */
    BOOST_JSON_DECL
    explicit
    string(
        std::size_t count,
        char ch,
        storage_ptr sp = {});

    /** Constructor.

        Construct the contents with those of the null
        terminated string pointed to by `s`. The length
        of the string is determined by the first null
        character.

        @par Complexity

        Linear in `strlen(s)`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param s A pointer to a character string used to
        copy from.

        @param sp An optional pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
        The default argument for this parameter is `{}`.

        @throw std::length_error `strlen(s) > max_size()`.
    */
    BOOST_JSON_DECL
    string(
        char const* s,
        storage_ptr sp = {});

    /** Constructor.

        Construct the contents with copies of the
        characters in the range `{s, s+count)`.
        This range can contain null characters.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param count The number of characters to copy.

        @param s A pointer to a character string used to
        copy from.

        @param sp An optional pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
        The default argument for this parameter is `{}`.

        @throw std::length_error `count > max_size()`.
    */
    BOOST_JSON_DECL
    explicit
    string(
        char const* s,
        std::size_t count,
        storage_ptr sp = {});

    /** Constructor.

        Construct the contents with copies of characters
        in the range `{first, last)`.

        @par Complexity

        Linear in `std::distance(first, last)`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @tparam InputIt The type of the iterators.

        @par Constraints

        `InputIt` satisfies __InputIterator__.

        @param first An input iterator pointing to the
        first character to insert, or pointing to the
        end of the range.

        @param last An input iterator pointing to the end
        of the range.

        @param sp An optional pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
        The default argument for this parameter is `{}`.

        @throw std::length_error `std::distance(first, last) > max_size()`.
    */
    template<class InputIt
    #ifndef BOOST_JSON_DOCS
        ,class = is_inputit<InputIt>
    #endif
    >
    explicit
    string(
        InputIt first,
        InputIt last,
        storage_ptr sp = {});

    /** Copy constructor.

        Construct the contents with a copy of `other`.

        @par Complexity

        Linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param other The string to use as a source
        to copy from.
    */
    BOOST_JSON_DECL
    string(string const& other);

    /** Constructor.

        Construct the contents with a copy of `other`.

        @par Complexity

        Linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param other The string to use as a source
        to copy from.

        @param sp An optional pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
        The default argument for this parameter is `{}`.
    */
    BOOST_JSON_DECL
    explicit
    string(
        string const& other,
        storage_ptr sp);

    /** Move constructor.

        Constructs the string with the contents of `other`
        using move semantics. Ownership of the underlying
        memory is transferred.
        The container acquires shared ownership of the
        @ref memory_resource used by `other`. After construction,
        the moved-from string behaves as if newly
        constructed with its current memory resource.

        @par Complexity

        Constant.

        @param other The string to move
    */
    string(string&& other) noexcept
        : sp_(other.sp_)
        , impl_(other.impl_)
    {
        ::new(&other.impl_) string_impl();
    }

    /** Constructor.

        Construct the contents with those of `other`
        using move semantics.

        @li If `*other.storage() == *sp`,
        ownership of the underlying memory is transferred
        in constant time, with no possibility
        of exceptions. After construction, the moved-from
        string behaves as if newly constructed with
        its current @ref memory_resource. Otherwise,

        @li If `*other.storage() != *sp`,
        a copy of the characters in `other` is made. In
        this case, the moved-from string is not changed.

        @par Complexity

        Constant or linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param other The string to assign from.

        @param sp An optional pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
        The default argument for this parameter is `{}`.
    */
    BOOST_JSON_DECL
    explicit
    string(
        string&& other,
        storage_ptr sp);

    /** Constructor.

        Construct the contents with those of a
        string view. This view can contain
        null characters.

        @par Complexity

        Linear in `s.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param s The string view to copy from.

        @param sp An optional pointer to the @ref memory_resource
        to use. The container will acquire shared
        ownership of the memory resource.
        The default argument for this parameter is `{}`.

        @throw std::length_error `s.size() > max_size()`.
    */
    BOOST_JSON_DECL
    string(
        string_view s,
        storage_ptr sp = {});

    //------------------------------------------------------
    //
    // Assignment
    //
    //------------------------------------------------------

    /** Copy assignment.

        Replace the contents with a copy of `other`.

        @par Complexity

        Linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @return `*this`

        @param other The string to use as a source
        to copy from.
    */
    BOOST_JSON_DECL
    string&
    operator=(string const& other);

    /** Move assignment.

        Replace the contents with those of `other`
        using move semantics.

        @li If `*other.storage() == *this->storage()`,
        ownership of the underlying memory is transferred
        in constant time, with no possibility
        of exceptions. After construction, the moved-from
        string behaves as if newly constructed with its
        current @ref memory_resource. Otherwise,

        @li If `*other.storage() != *this->storage()`,
        a copy of the characters in `other` is made. In
        this case, the moved-from container is not changed.

        @par Complexity

        Constant or linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @return `*this`

        @param other The string to use as a source
        to move from.
    */
    BOOST_JSON_DECL
    string&
    operator=(string&& other);

    /** Assign a value to the string.

        Replaces the contents with those of the null
        terminated string pointed to by `s`. The length
        of the string is determined by the first null
        character.

        @par Complexity

        Linear in `std::strlen(s)`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @return `*this`

        @param s The null-terminated character string.

        @throw std::length_error `std::strlen(s) > max_size()`.
    */
    BOOST_JSON_DECL
    string&
    operator=(char const* s);

    /** Assign a value to the string.

        Replaces the contents with those of a
        string view. This view can contain
        null characters.

        @par Complexity

        Linear in `s.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @return `*this`

        @param s The string view to copy from.

        @throw std::length_error `s.size() > max_size()`.
    */
    BOOST_JSON_DECL
    string&
    operator=(string_view s);

    //------------------------------------------------------

    /** Assign characters to a string.

        Replace the contents with `count` copies of
        character `ch`.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @return `*this`

        @param count The size of the resulting string.

        @param ch The value to initialize characters
        of the string with.

        @throw std::length_error `count > max_size()`.
    */
    BOOST_JSON_DECL
    string&
    assign(
        std::size_t count,
        char ch);

    /** Assign characters to a string.

        Replace the contents with a copy of `other`.

        @par Complexity

        Linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @return `*this`

        @param other The string to use as a source
        to copy from.
    */
    BOOST_JSON_DECL
    string&
    assign(
        string const& other);

    /** Assign characters to a string.

        Replace the contents with those of `other`
        using move semantics.

        @li If `*other.storage() == *this->storage()`,
        ownership of the underlying memory is transferred
        in constant time, with no possibility of
        exceptions. After construction, the moved-from
        string behaves as if newly constructed with
        its current  @ref memory_resource, otherwise

        @li If `*other.storage() != *this->storage()`,
        a copy of the characters in `other` is made.
        In this case, the moved-from container
        is not changed.

        @par Complexity

        Constant or linear in `other.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @return `*this`

        @param other The string to assign from.
    */
    BOOST_JSON_DECL
    string&
    assign(string&& other);

    /** Assign characters to a string.

        Replaces the contents with copies of the
        characters in the range `{s, s+count)`. This
        range can contain null characters.

        @par Complexity

        Linear in `count`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @return `*this`

        @param count The number of characters to copy.

        @param s A pointer to a character string used to
        copy from.

        @throw std::length_error `count > max_size()`.
    */
    BOOST_JSON_DECL
    string&
    assign(
        char const* s,
        std::size_t count);

    /** Assign characters to a string.

        Replaces the contents with those of the null
        terminated string pointed to by `s`. The length
        of the string is determined by the first null
        character.

        @par Complexity

        Linear in `strlen(s)`.

        @par Exception Safety

        Strong guarantee.

        @note

        Calls to `memory_resource::allocate` may throw.

        @return `*this`

        @param s A pointer to a character string used to
        copy from.

        @throw std::length_error `strlen(s) > max_size()`.
    */
    BOOST_JSON_DECL
    string&
    assign(
        char const* s);

    /** Assign characters to a string.

        Replaces the contents with copies of characters
        in the range `{first, last)`.

        @par Complexity

        Linear in `std::distance(first, last)`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @tparam InputIt The type of the iterators.

        @par Constraints

        `InputIt` satisfies __InputIterator__.

        @return `*this`

        @param first An input iterator pointing to the
        first character to insert, or pointing to the
        end of the range.

        @param last An input iterator pointing to the end
        of the range.

        @throw std::length_error `std::distance(first, last) > max_size()`.
    */
    template<class InputIt
    #ifndef BOOST_JSON_DOCS
        ,class = is_inputit<InputIt>
    #endif
    >
    string&
    assign(
        InputIt first,
        InputIt last);

    /** Assign characters to a string.

        Replaces the contents with those of a
        string view. This view can contain
        null characters.

        @par Complexity

        Linear in `s.size()`.

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @return `*this`

        @param s The string view to copy from.

        @throw std::length_error `s.size() > max_size()`.
    */
    string&
    assign(string_view s)
    {
        return assign(s.data(), s.size());
    }

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
    // Element Access
    //
    //------------------------------------------------------

    /** Return a character with bounds checking.

        Returns a reference to the character specified at
        location `pos`.

        @par Complexity

        Constant.

        @par Exception Safety

        Strong guarantee.

        @param pos A zero-based index to access.

        @throw std::out_of_range `pos >= size()`
    */
    char&
    at(std::size_t pos)
    {
        if(pos >= size())
            detail::throw_out_of_range(
                BOOST_JSON_SOURCE_POS);
        return impl_.data()[pos];
    }

    /** Return a character with bounds checking.

        Returns a reference to the character specified at
        location `pos`.

        @par Complexity

        Constant.

        @par Exception Safety

        Strong guarantee.

        @param pos A zero-based index to access.

        @throw std::out_of_range `pos >= size()`
    */
    char const&
    at(std::size_t pos) const
    {
        if(pos >= size())
            detail::throw_out_of_range(
                BOOST_JSON_SOURCE_POS);
        return impl_.data()[pos];
    }

    /** Return a character without bounds checking.

        Returns a reference to the character specified at
        location `pos`.

        @par Complexity

        Constant.

        @par Precondition

        @code
        pos >= size
        @endcode

        @param pos A zero-based index to access.
    */
    char&
    operator[](std::size_t pos)
    {
        return impl_.data()[pos];
    }

   /**  Return a character without bounds checking.

        Returns a reference to the character specified at
        location `pos`.

        @par Complexity

        Constant.

        @par Precondition

        @code
        pos >= size
        @endcode

        @param pos A zero-based index to access.
    */
    const char&
    operator[](std::size_t pos) const
    {
        return impl_.data()[pos];
    }

    /** Return the first character.

        Returns a reference to the first character.

        @par Complexity

        Constant.

        @par Precondition

        @code
        not empty()
        @endcode
    */
    char&
    front()
    {
        return impl_.data()[0];
    }

    /** Return the first character.

        Returns a reference to the first character.

        @par Complexity

        Constant.

        @par Precondition

        @code
        not empty()
        @endcode
    */
    char const&
    front() const
    {
        return impl_.data()[0];
    }

    /** Return the last character.

        Returns a reference to the last character.

        @par Complexity

        Constant.

        @par Precondition

        @code
        not empty()
        @endcode
    */
    char&
    back()
    {
        return impl_.data()[impl_.size() - 1];
    }

    /** Return the last character.

        Returns a reference to the last character.

        @par Complexity

        Constant.

        @par Precondition

        @code
        not empty()
        @endcode
    */
    char const&
    back() const
    {
        return impl_.data()[impl_.size() - 1];
    }

    /** Return the underlying character array directly.

        Returns a pointer to the underlying array
        serving as storage. The value returned is such that
        the range `{data(), data()+size())` is always a
        valid range, even if the container is empty.

        @par Complexity

        Constant.

        @note The value returned from
        this function is never equal to `nullptr`.
    */
    char*
    data() noexcept
    {
        return impl_.data();
    }

    /** Return the underlying character array directly.

        Returns a pointer to the underlying array
        serving as storage.

        @note The value returned is such that
        the range `{data(), data() + size())` is always a
        valid range, even if the container is empty.
        The value returned from
        this function is never equal to `nullptr`.

        @par Complexity

        Constant.
    */
    char const*
    data() const noexcept
    {
        return impl_.data();
    }

    /** Return the underlying character array directly.

        Returns a pointer to the underlying array
        serving as storage. The value returned is such that
        the range `{c_str(), c_str() + size()}` is always a
        valid range, even if the container is empty.

        @par Complexity

        Constant.

        @note The value returned from
        this function is never equal to `nullptr`.
    */
    char const*
    c_str() const noexcept
    {
        return impl_.data();
    }

    /** Convert to a `string_view` referring to the string.

        Returns a string view to the
        underlying character string. The size of the view
        does not include the null terminator.

        @par Complexity

        Constant.
    */
    operator string_view() const noexcept
    {
        return {data(), size()};
    }

#if ! defined(BOOST_JSON_STANDALONE) && \
    ! defined(BOOST_NO_CXX17_HDR_STRING_VIEW)
    /** Convert to a `std::string_view` referring to the string.

        Returns a string view to the underlying character string. The size of
        the view does not include the null terminator.

        This overload is not defined when either `BOOST_JSON_STANDALONE` or
        `BOOST_NO_CXX17_HDR_STRING_VIEW` is defined.

        @par Complexity

        Constant.
    */
    operator std::string_view() const noexcept
    {
        return {data(), size()};
    }
#endif

    //------------------------------------------------------
    //
    // Iterators
    //
    //------------------------------------------------------

    /** Return an iterator to the beginning.

        If the container is empty, @ref end() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    iterator
    begin() noexcept
    {
        return impl_.data();
    }

    /** Return an iterator to the beginning.

        If the container is empty, @ref end() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    const_iterator
    begin() const noexcept
    {
        return impl_.data();
    }

    /** Return an iterator to the beginning.

        If the container is empty, @ref cend() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    const_iterator
    cbegin() const noexcept
    {
        return impl_.data();
    }

    /** Return an iterator to the end.

        Returns an iterator to the character
        following the last character of the string.
        This character acts as a placeholder, attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    iterator
    end() noexcept
    {
        return impl_.end();
    }

    /** Return an iterator to the end.

        Returns an iterator to the character following
        the last character of the string.
        This character acts as a placeholder, attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    const_iterator
    end() const noexcept
    {
        return impl_.end();
    }

    /** Return an iterator to the end.

        Returns an iterator to the character following
        the last character of the string.
        This character acts as a placeholder, attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    const_iterator
    cend() const noexcept
    {
        return impl_.end();
    }

    /** Return a reverse iterator to the first character of the reversed container.

        Returns the pointed-to character that
        corresponds to the last character of the
        non-reversed container.
        If the container is empty, @ref rend() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    reverse_iterator
    rbegin() noexcept
    {
        return reverse_iterator(impl_.end());
    }

    /** Return a reverse iterator to the first character of the reversed container.

        Returns the pointed-to character that
        corresponds to the last character of the
        non-reversed container.
        If the container is empty, @ref rend() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    const_reverse_iterator
    rbegin() const noexcept
    {
        return const_reverse_iterator(impl_.end());
    }

    /** Return a reverse iterator to the first character of the reversed container.

        Returns the pointed-to character that
        corresponds to the last character of the
        non-reversed container.
        If the container is empty, @ref crend() is returned.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    const_reverse_iterator
    crbegin() const noexcept
    {
        return const_reverse_iterator(impl_.end());
    }

    /** Return a reverse iterator to the character following the last character of the reversed container.

        Returns the pointed-to character that corresponds
        to the character preceding the first character of
        the non-reversed container.
        This character acts as a placeholder, attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    reverse_iterator
    rend() noexcept
    {
        return reverse_iterator(begin());
    }

    /** Return a reverse iterator to the character following the last character of the reversed container.

        Returns the pointed-to character that corresponds
        to the character preceding the first character of
        the non-reversed container.
        This character acts as a placeholder, attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    const_reverse_iterator
    rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    /** Return a reverse iterator to the character following the last character of the reversed container.

        Returns the pointed-to character that corresponds
        to the character preceding the first character of
        the non-reversed container.
        This character acts as a placeholder, attempting
        to access it results in undefined behavior.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    const_reverse_iterator
    crend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    //------------------------------------------------------
    //
    // Capacity
    //
    //------------------------------------------------------

    /** Check if the string has no characters.

        Returns `true` if there are no characters in
        the string, i.e. @ref size() returns 0.

        @par Complexity

        Constant.
    */
    bool
    empty() const noexcept
    {
        return impl_.size() == 0;
    }

    /** Return the number of characters in the string.

        The value returned does not include the
        null terminator, which is always present.

        @par Complexity

        Constant.
    */
    std::size_t
    size() const noexcept
    {
        return impl_.size();
    }

    /** Return the maximum number of characters any string can hold.

        The maximum is an implementation-defined number.
        This value is a theoretical limit; at runtime,
        the actual maximum size may be less due to
        resource limits.

        @par Complexity

        Constant.
    */
    static
    constexpr
    std::size_t
    max_size() noexcept
    {
        return string_impl::max_size();
    }

    /** Return the number of characters that can be held without a reallocation.

        This number represents the largest number of
        characters the currently allocated storage can contain.
        This number may be larger than the value returned
        by @ref size().

        @par Complexity

        Constant.
    */
    std::size_t
    capacity() const noexcept
    {
        return impl_.capacity();
    }

    /** Increase the capacity to at least a certain amount.

        This increases the capacity of the array to a value
        that is greater than or equal to `new_capacity`. If
        `new_capacity > capacity()`, new memory is
        allocated. Otherwise, the call has no effect.
        The number of elements and therefore the
        @ref size() of the container is not changed.

        @par Complexity

        At most, linear in @ref size().

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @note

        If new memory is allocated, all iterators including
        any past-the-end iterators, and all references to
        the elements are invalidated. Otherwise, no
        iterators or references are invalidated.

        @param new_capacity The new capacity of the array.

        @throw std::length_error `new_capacity > max_size()`
    */
    void
    reserve(std::size_t new_capacity)
    {
        if(new_capacity <= capacity())
            return;
        reserve_impl(new_capacity);
    }

    /** Request the removal of unused capacity.

        This performs a non-binding request to reduce
        @ref capacity() to @ref size(). The request may
        or may not be fulfilled.

        @par Complexity

        At most, linear in @ref size().

        @note If reallocation occurs, all iterators
        including  any past-the-end iterators, and all
        references to characters are invalidated.
        Otherwise, no iterators or references are
        invalidated.
    */
    BOOST_JSON_DECL
    void
    shrink_to_fit();

    //------------------------------------------------------
    //
    // Operations
    //
    //------------------------------------------------------

    /** Clear the contents.

        Erases all characters from the string. After this
        call, @ref size() returns zero but @ref capacity()
        is unchanged.

        @par Complexity

        Linear in @ref size().

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.
    */
    BOOST_JSON_DECL
    void
    clear() noexcept;

    //------------------------------------------------------

    /** Insert a string.

        Inserts the `string_view` `sv` at the position `pos`.

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @return `*this`

        @param pos The index to insert at.

        @param sv The `string_view` to insert.

        @throw std::length_error `size() + s.size() > max_size()`

        @throw std::out_of_range `pos > size()`
    */
    BOOST_JSON_DECL
    string&
    insert(
        std::size_t pos,
        string_view sv);

    /** Insert a character.

        Inserts `count` copies of `ch` at the position `pos`.

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @return `*this`

        @param pos The index to insert at.

        @param count The number of characters to insert.

        @param ch The character to insert.

        @throw std::length_error `size() + count > max_size()`

        @throw std::out_of_range `pos > size()`
    */
    BOOST_JSON_DECL
    string&
    insert(
        std::size_t pos,
        std::size_t count,
        char ch);

    /** Insert a character.

        Inserts the character `ch` before the character
        at index `pos`.

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @return `*this`

        @param pos The index to insert at.

        @param ch The character to insert.

        @throw std::length_error `size() + 1 > max_size()`

        @throw std::out_of_range `pos > size()`
    */
    string&
    insert(
        size_type pos,
        char ch)
    {
        return insert(pos, 1, ch);
    }

    /** Insert a range of characters.

        Inserts characters from the range `{first, last)`
        before the character at index `pos`.

        @par Precondition

        `{first, last)` is a valid range.

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @tparam InputIt The type of the iterators.

        @par Constraints

        `InputIt` satisfies __InputIterator__.

        @return `*this`

        @param pos The index to insert at.

        @param first The beginning of the character range.

        @param last The end of the character range.

        @throw std::length_error `size() + insert_count > max_size()`

        @throw std::out_of_range `pos > size()`
    */
    template<class InputIt
    #ifndef BOOST_JSON_DOCS
        ,class = is_inputit<InputIt>
    #endif
    >
    string&
    insert(
        size_type pos,
        InputIt first,
        InputIt last);

    //------------------------------------------------------

    /** Erase characters from the string.

        Erases `num` characters from the string, starting
        at `pos`.  `num` is determined as the smaller of
        `count` and `size() - pos`.

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @return `*this`

        @param pos The index to erase at.
        The default argument for this parameter is `0`.

        @param count The number of characters to erase.
        The default argument for this parameter
        is @ref npos.

        @throw std::out_of_range `pos > size()`
    */
    BOOST_JSON_DECL
    string&
    erase(
        std::size_t pos = 0,
        std::size_t count = npos);

    /** Erase a character from the string.

        Erases the character at `pos`.

        @par Precondition

        @code
        pos >= data() && pos <= data() + size()
        @endcode

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @return An iterator referring to character
        immediately following the erased character, or
        @ref end() if one does not exist.

        @param pos An iterator referring to the
        character to erase.
    */
    BOOST_JSON_DECL
    iterator
    erase(const_iterator pos);

    /** Erase a range from the string.

        Erases the characters in the range `{first, last)`.

        @par Precondition

        `{first, last}` shall be valid within
        @code
        {data(), data() + size()}
        @endcode

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @return An iterator referring to the character
        `last` previously referred to, or @ref end()
        if one does not exist.

        @param first An iterator representing the first
        character to erase.

        @param last An iterator one past the last
        character to erase.
    */
    BOOST_JSON_DECL
    iterator
    erase(
        const_iterator first,
        const_iterator last);

    //------------------------------------------------------

    /** Append a character.

        Appends a character to the end of the string.

        @par Exception Safety

        Strong guarantee.

        @param ch The character to append.

        @throw std::length_error `size() + 1 > max_size()`
    */
    BOOST_JSON_DECL
    void
    push_back(char ch);

    /** Remove the last character.

        Removes a character from the end of the string.

        @par Precondition

        @code
        not empty()
        @endcode
    */
    BOOST_JSON_DECL
    void
    pop_back();

    //------------------------------------------------------

    /** Append characters to the string.

        Appends `count` copies of `ch` to the end of
        the string.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param count The number of characters to append.

        @param ch The character to append.

        @throw std::length_error `size() + count > max_size()`
    */
    BOOST_JSON_DECL
    string&
    append(
        std::size_t count,
        char ch);

    /** Append a string to the string.

        Appends `sv` the end of the string.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param sv The `string_view` to append.

        @throw std::length_error `size() + s.size() > max_size()`
    */
    BOOST_JSON_DECL
    string&
    append(string_view sv);

    /** Append a range of characters.

        Appends characters from the range `{first, last)`
        to the end of the string.

        @par Precondition

        `{first, last)` shall be a valid range

        @par Exception Safety

        Strong guarantee.

        @tparam InputIt The type of the iterators.

        @par Constraints

        `InputIt` satisfies __InputIterator__.

        @return `*this`

        @param first An iterator representing the
        first character to append.

        @param last An iterator one past the
        last character to append.

        @throw std::length_error `size() + insert_count > max_size()`
    */
    template<class InputIt
    #ifndef BOOST_JSON_DOCS
        ,class = is_inputit<InputIt>
    #endif
    >
    string&
    append(InputIt first, InputIt last);

    //------------------------------------------------------

    /** Append characters from a string.

        Appends `{sv.begin(), sv.end())` to the end of
        the string.

        @par Exception Safety

        Strong guarantee.

        @return `*this`

        @param sv The `string_view` to append.

        @throw std::length_error `size() + sv.size() > max_size()`
    */
    string&
    operator+=(string_view sv)
    {
        return append(sv);
    }

    /** Append a character.

        Appends a character to the end of the string.

        @par Exception Safety

        Strong guarantee.

        @param ch The character to append.

        @throw std::length_error `size() + 1 > max_size()`
    */
    string&
    operator+=(char ch)
    {
        push_back(ch);
        return *this;
    }

    //------------------------------------------------------

    /** Compare a string with the string.

        Let `comp` be
        `std::char_traits<char>::compare(data(), sv.data(), std::min(size(), sv.size())`.
        If `comp != 0`, then the result is `comp`. Otherwise,
        the result is `0` if `size() == sv.size()`,
        `-1` if `size() < sv.size()`, and `1` otherwise.

        @par Complexity

        Linear.

        @return The result of lexicographically comparing
        the characters of `sv` and the string.

        @param sv The `string_view` to compare.
    */
    int
    compare(string_view sv) const noexcept
    {
        return string_view(*this).compare(sv);
    }

    //------------------------------------------------------

    /** Return whether the string begins with a string.

        Returns `true` if the string begins with `s`,
        and `false` otherwise.

        @par Complexity

        Linear.

        @param s The `string_view` to check for.
    */
    bool
    starts_with(string_view s) const noexcept
    {
        return subview(0, s.size()) == s;
    }

    /** Return whether the string begins with a character.

        Returns `true` if the string begins with `ch`,
        and `false` otherwise.

        @par Complexity

        Constant.

        @param ch The character to check for.
    */
    bool
    starts_with(char ch) const noexcept
    {
        return ! empty() && front() == ch;
    }

    /** Return whether the string end with a string.

        Returns `true` if the string end with `s`,
        and `false` otherwise.

        @par Complexity

        Linear.

        @param s The string to check for.
    */
    bool
    ends_with(string_view s) const noexcept
    {
        return size() >= s.size() &&
            subview(size() - s.size()) == s;
    }

    /** Return whether the string ends with a character.

        Returns `true` if the string ends with `ch`,
        and `false` otherwise.

        @par Complexity

        Constant.

        @param ch The character to check for.
    */
    bool
    ends_with(char ch) const noexcept
    {
        return ! empty() && back() == ch;
    }

    //------------------------------------------------------

    /** Replace a substring with a string.

        Replaces `rcount` characters starting at index
        `pos` with those of `sv`, where `rcount` is
        `std::min(count, size() - pos)`.

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @return `*this`

        @param pos The index to replace at.

        @param count The number of characters to replace.

        @param sv The `string_view` to replace with.

        @throw std::length_error `size() + (sv.size() - rcount) > max_size()`

        @throw std::out_of_range `pos > size()`
    */
    BOOST_JSON_DECL
    string&
    replace(
        std::size_t pos,
        std::size_t count,
        string_view sv);

    /** Replace a range with a string.

        Replaces the characters in the range
        `{first, last)` with those of `sv`.

        @par Precondition

        `{first, last)` is a valid range.

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @return `*this`

        @param first An iterator referring to the first
        character to replace.

        @param last An iterator one past the end of
        the last character to replace.

        @param sv The `string_view` to replace with.

        @throw std::length_error `size() + (sv.size() - std::distance(first, last)) > max_size()`
    */
    string&
    replace(
        const_iterator first,
        const_iterator last,
        string_view sv)
    {
        return replace(first - begin(), last - first, sv);
    }

    /** Replace a range with a range.

        Replaces the characters in the range
        `{first, last)` with those of `{first2, last2)`.

        @par Precondition

        `{first, last)` is a valid range.

        `{first2, last2)` is a valid range.

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @tparam InputIt The type of the iterators.

        @par Constraints

        `InputIt` satisfies __InputIterator__.

        @return `*this`

        @param first An iterator referring to the first
        character to replace.

        @param last An iterator one past the end of
        the last character to replace.

        @param first2 An iterator referring to the first
        character to replace with.

        @param last2 An iterator one past the end of
        the last character to replace with.

        @throw std::length_error `size() + (inserted - std::distance(first, last)) > max_size()`
    */
    template<class InputIt
    #ifndef BOOST_JSON_DOCS
        ,class = is_inputit<InputIt>
    #endif
    >
    string&
    replace(
        const_iterator first,
        const_iterator last,
        InputIt first2,
        InputIt last2);

    /** Replace a substring with copies of a character.

        Replaces `rcount` characters starting at index
        `pos`with `count2` copies of `ch`, where
        `rcount` is `std::min(count, size() - pos)`.

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @return `*this`

        @param pos The index to replace at.

        @param count The number of characters to replace.

        @param count2 The number of characters to
        replace with.

        @param ch The character to replace with.

        @throw std::length_error `size() + (count2 - rcount) > max_size()`

        @throw std::out_of_range `pos > size()`
    */
    BOOST_JSON_DECL
    string&
    replace(
        std::size_t pos,
        std::size_t count,
        std::size_t count2,
        char ch);

    /** Replace a range with copies of a character.

        Replaces the characters in the range
        `{first, last)` with `count` copies of `ch`.

        @par Precondition

        `{first, last)` is a valid range.

        @par Exception Safety

        Strong guarantee.

        @note All references, pointers, or iterators
        referring to contained elements are invalidated.
        Any past-the-end iterators are also invalidated.

        @return `*this`

        @param first An iterator referring to the first
        character to replace.

        @param last An iterator one past the end of
        the last character to replace.

        @param count The number of characters to
        replace with.

        @param ch The character to replace with.

        @throw std::length_error `size() + (count - std::distance(first, last)) > max_size()`
    */
    string&
    replace(
        const_iterator first,
        const_iterator last,
        std::size_t count,
        char ch)
    {
        return replace(first - begin(), last - first, count, ch);
    }

    //------------------------------------------------------

    /** Return a substring.

        Returns a view of a substring.

        @par Exception Safety

        Strong guarantee.

        @return A `string_view` object referring
        to `{data() + pos, std::min(count, size() - pos))`.

        @param pos The index to being the substring at.
        The default argument for this parameter is `0`.

        @param count The length of the substring.
        The default argument for this parameter
        is @ref npos.

        @throw std::out_of_range `pos > size()`
    */
    string_view
    subview(
        std::size_t pos = 0,
        std::size_t count = npos) const
    {
        return string_view(*this).substr(pos, count);
    }

    //------------------------------------------------------

    /** Copy a substring to another string.

        Copies `std::min(count, size() - pos)` characters
        starting at index `pos` to the string pointed
        to by `dest`.

        @note The resulting string is not null terminated.

        @return The number of characters copied.

        @param count The number of characters to copy.

        @param dest The string to copy to.

        @param pos The index to begin copying from. The
        default argument for this parameter is `0`.

        @throw std::out_of_range `pos > max_size()`
    */
    std::size_t
    copy(
        char* dest,
        std::size_t count,
        std::size_t pos = 0) const
    {
        return string_view(*this).copy(dest, count, pos);
    }

    //------------------------------------------------------

    /** Change the size of the string.

        Resizes the string to contain `count` characters.
        If `count > size()`, characters with the value `0`
        are appended. Otherwise, `size()` is reduced
        to `count`.

        @param count The size to resize the string to.

        @throw std::out_of_range `count > max_size()`
    */
    void
    resize(std::size_t count)
    {
        resize(count, 0);
    }

    /** Change the size of the string.

        Resizes the string to contain `count` characters.
        If `count > size()`, copies of `ch` are
        appended. Otherwise, `size()` is reduced
        to `count`.

        @param count The size to resize the string to.

        @param ch The characters to append if the size
        increases.

        @throw std::out_of_range `count > max_size()`
    */
    BOOST_JSON_DECL
    void
    resize(std::size_t count, char ch);

    /** Increase size without changing capacity.

        This increases the size of the string by `n`
        characters, adjusting the position of the
        terminating null for the new size. The new
        characters remain uninitialized. This function
        may be used to append characters directly into
        the storage between `end()` and
        `data() + capacity()`.

        @par Precondition

        @code
        count <= capacity() - size()
        @endcode

        @param n The amount to increase the size by.
    */
    void
    grow(std::size_t n) noexcept
    {
        BOOST_ASSERT(
            n <= impl_.capacity() - impl_.size());
        impl_.term(impl_.size() + n);
    }

    //------------------------------------------------------

    /** Swap the contents.

        Exchanges the contents of this string with another
        string. Ownership of the respective @ref memory_resource
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

        Constant or linear in @ref size() plus
        `other.size()`.

        @par Precondition

        @code
        &other != this
        @endcode

        @par Exception Safety

        Strong guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param other The string to swap with
        If `this == &other`, this function call has no effect.
    */
    BOOST_JSON_DECL
    void
    swap(string& other);

    /** Exchange the given values.

        Exchanges the contents of the string `lhs` with
        another string `rhs`. Ownership of the respective
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

        @param lhs The string to exchange.

        @param rhs The string to exchange.
        If `&lhs == &rhs`, this function call has no effect.

        @see @ref string::swap
    */
    friend
    void
    swap(string& lhs, string& rhs)
    {
        lhs.swap(rhs);
    }
    //------------------------------------------------------
    //
    // Search
    //
    //------------------------------------------------------

    /** Find the first occurrence of a string within the string.

        Returns the lowest index `idx` greater than or equal
        to `pos` where each element of `sv`  is equal to
        that of `{begin() + idx, begin() + idx + sv.size())`
        if one exists, and @ref npos otherwise.

        @par Complexity

        Linear.

        @return The first occurrence of `sv` within the
        string starting at the index `pos`, or @ref npos
        if none exists.

        @param sv The `string_view` to search for.

        @param pos The index to start searching at.
        The default argument for this parameter is `0`.
    */
    std::size_t
    find(
        string_view sv,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find(sv, pos);
    }

    /** Find the first occurrence of a character within the string.

        Returns the index corrosponding to the first
        occurrence of `ch` within `{begin() + pos, end())`
        if it exists, and @ref npos otherwise.

        @par Complexity

        Linear.

        @return The first occurrence of `ch` within the
        string starting at the index `pos`, or @ref npos
        if none exists.

        @param ch The character to search for.

        @param pos The index to start searching at.
        The default argument for this parameter is `0`.
    */
    std::size_t
    find(
        char ch,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find(ch, pos);
    }

    //------------------------------------------------------

    /** Find the last occurrence of a string within the string.

        Returns the highest index `idx` less than or equal
        to `pos` where each element of `sv` is equal to that
        of `{begin() + idx, begin() + idx + sv.size())`
        if one exists, and @ref npos otherwise.

        @par Complexity

        Linear.

        @return The last occurrence of `sv` within the
        string starting before or at the index `pos`,
        or @ref npos if none exists.

        @param sv The `string_view` to search for.

        @param pos The index to start searching at.
        The default argument for this parameter
        is @ref npos.
    */
    std::size_t
    rfind(
        string_view sv,
        std::size_t pos = npos) const noexcept
    {
        return string_view(*this).rfind(sv, pos);
    }

    /** Find the last occurrence of a character within the string.

        Returns index corrosponding to the last occurrence
        of `ch` within `{begin(), begin() + pos}` if it
        exists, and @ref npos otherwise.

        @par Complexity

        Linear.

        @return The last occurrence of `ch` within the
        string starting before or at the index `pos`,
        or @ref npos if none exists.

        @param ch The character to search for.

        @param pos The index to stop searching at.
        The default argument for this parameter
        is @ref npos.
    */
    std::size_t
    rfind(
        char ch,
        std::size_t pos = npos) const noexcept
    {
        return string_view(*this).rfind(ch, pos);
    }

    //------------------------------------------------------

    /** Find the first occurrence of any of the characters within the string.

        Returns the index corrosponding to the first
        occurrence of any of the characters of `sv`
        within `{begin() + pos, end())` if it exists,
        and @ref npos otherwise.

        @par Complexity

        Linear.

        @return The first occurrence of any of the
        characters within `sv` within the string
        starting at the index `pos`, or @ref npos
        if none exists.

        @param sv The characters to search for.

        @param pos The index to start searching at.
        The default argument for this parameter is `0`.
    */
    std::size_t
    find_first_of(
        string_view sv,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_first_of(sv, pos);
    }

    //------------------------------------------------------

    /** Find the first occurrence of any of the characters not within the string.

        Returns the index corrosponding to the first
        character of `{begin() + pos, end())` that is
        not within `sv` if it exists, and @ref npos
        otherwise.

        @par Complexity

        Linear.

        @return The first occurrence of a character that
        is not within `sv` within the string starting at
        the index `pos`, or @ref npos if none exists.

        @param sv The characters to ignore.

        @param pos The index to start searching at.
        The default argument for this parameter is `0`.
    */
    std::size_t
    find_first_not_of(
        string_view sv,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_first_not_of(sv, pos);
    }

    /** Find the first occurrence of a character not equal to `ch`.

        Returns the index corrosponding to the first
        character of `{begin() + pos, end())` that is
        not equal to `ch` if it exists, and
        @ref npos otherwise.

        @par Complexity

        Linear.

        @return The first occurrence of a character that
        is not equal to `ch`, or @ref npos if none exists.

        @param ch The character to ignore.

        @param pos The index to start searching at.
        The default argument for this parameter is `0`.
    */
    std::size_t
    find_first_not_of(
        char ch,
        std::size_t pos = 0) const noexcept
    {
        return string_view(*this).find_first_not_of(ch, pos);
    }

    //------------------------------------------------------

    /** Find the last occurrence of any of the characters within the string.

        Returns the index corrosponding to the last
        occurrence of any of the characters of `sv` within
        `{begin(), begin() + pos}` if it exists,
        and @ref npos otherwise.

        @par Complexity

        Linear.

        @return The last occurrence of any of the
        characters within `sv` within the string starting
        before or at the index `pos`, or @ref npos if
        none exists.

        @param sv The characters to search for.

        @param pos The index to stop searching at.
        The default argument for this parameter
        is @ref npos.
    */
    std::size_t
    find_last_of(
        string_view sv,
        std::size_t pos = npos) const noexcept
    {
        return string_view(*this).find_last_of(sv, pos);
    }

    //------------------------------------------------------

    /** Find the last occurrence of a character not within the string.

        Returns the index corrosponding to the last
        character of `{begin(), begin() + pos}` that is not
        within `sv` if it exists, and @ref npos otherwise.

        @par Complexity

        Linear.

        @return The last occurrence of a character that is
        not within `sv` within the string before or at the
        index `pos`, or @ref npos if none exists.

        @param sv The characters to ignore.

        @param pos The index to stop searching at.
        The default argument for this parameter
        is @ref npos.
    */
    std::size_t
    find_last_not_of(
        string_view sv,
        std::size_t pos = npos) const noexcept
    {
        return string_view(*this).find_last_not_of(sv, pos);
    }

    /** Find the last occurrence of a character not equal to `ch`.

        Returns the index corrosponding to the last
        character of `{begin(), begin() + pos}` that is
        not equal to `ch` if it exists, and @ref npos
        otherwise.

        @par Complexity

        Linear.

        @return The last occurrence of a character that
        is not equal to `ch` before or at the index `pos`,
        or @ref npos if none exists.

        @param ch The character to ignore.

        @param pos The index to start searching at.
        The default argument for this parameter
        is @ref npos.
    */
    std::size_t
    find_last_not_of(
        char ch,
        std::size_t pos = npos) const noexcept
    {
        return string_view(*this).find_last_not_of(ch, pos);
    }

private:
    class undo;

    template<class It>
    using iter_cat = typename
        std::iterator_traits<It>::iterator_category;

    template<class InputIt>
    void
    assign(InputIt first, InputIt last,
        std::random_access_iterator_tag);

    template<class InputIt>
    void
    assign(InputIt first, InputIt last,
        std::input_iterator_tag);

    template<class InputIt>
    void
    append(InputIt first, InputIt last,
        std::random_access_iterator_tag);

    template<class InputIt>
    void
    append(InputIt first, InputIt last,
        std::input_iterator_tag);

    BOOST_JSON_DECL
    void
    reserve_impl(std::size_t new_capacity);
};

//----------------------------------------------------------

/** Return true if lhs equals rhs.

    A lexicographical comparison is used.
*/
#ifdef BOOST_JSON_DOCS
bool
operator==(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator==(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) == string_view(rhs);
}

/** Return true if lhs does not equal rhs.

    A lexicographical comparison is used.
*/
#ifdef BOOST_JSON_DOCS
bool
operator!=(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator!=(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) != string_view(rhs);
}

/** Return true if lhs is less than rhs.

    A lexicographical comparison is used.
*/
#ifdef BOOST_JSON_DOCS
bool
operator<(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator<(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) < string_view(rhs);
}

/** Return true if lhs is less than or equal to rhs.

    A lexicographical comparison is used.
*/
#ifdef BOOST_JSON_DOCS
bool
operator<=(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator<=(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) <= string_view(rhs);
}

#ifdef BOOST_JSON_DOCS
bool
operator>=(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator>=(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) >= string_view(rhs);
}

/** Return true if lhs is greater than rhs.

    A lexicographical comparison is used.
*/
#ifdef BOOST_JSON_DOCS
bool
operator>(string const& lhs, string const& rhs) noexcept
#else
template<class T, class U>
typename std::enable_if<
    (std::is_same<T, string>::value &&
     std::is_convertible<
        U const&, string_view>::value) ||
    (std::is_same<U, string>::value &&
     std::is_convertible<
        T const&, string_view>::value),
    bool>::type
operator>(T const& lhs, U const& rhs) noexcept
#endif
{
    return string_view(lhs) > string_view(rhs);
}

BOOST_JSON_NS_END

// std::hash specialization
#ifndef BOOST_JSON_DOCS
namespace std {
template<>
struct hash< ::boost::json::string >
{
    hash() = default;
    hash(hash const&) = default;
    hash& operator=(hash const&) = default;

    explicit
    hash(std::size_t salt) noexcept
        : salt_(salt)
    {
    }

    std::size_t
    operator()(::boost::json::string const& js) const noexcept
    {
        return ::boost::json::detail::digest(
            js.data(), js.size(), salt_);
    }

private:
    std::size_t salt_ = 0;
};
} // std
#endif

#include <boost/json/impl/string.hpp>

#endif
