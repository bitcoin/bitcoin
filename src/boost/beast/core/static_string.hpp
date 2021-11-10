//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_STATIC_STRING_HPP
#define BOOST_BEAST_STATIC_STRING_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/core/detail/static_string.hpp>
#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace boost {
namespace beast {

/** A modifiable string with a fixed-size storage area.

    These objects behave like `std::string` except that the storage
    is not dynamically allocated but rather fixed in size.

    These strings offer performance advantages when a protocol
    imposes a natural small upper limit on the size of a value.

    @note The stored string is always null-terminated.

    @see to_static_string 
*/
template<
    std::size_t N,
    class CharT = char,
    class Traits = std::char_traits<CharT>>
class static_string
{
    template<std::size_t, class, class>
    friend class static_string;

    void
    term()
    {
        Traits::assign(s_[n_], 0);
    }

    std::size_t n_;
    CharT s_[N+1];

public:
    //
    // Member types
    //

    using traits_type = Traits;
    using value_type = typename Traits::char_type;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;
    using const_pointer = value_type const*;
    using const_reference = value_type const&;
    using iterator = value_type*;
    using const_iterator = value_type const*;
    using reverse_iterator =
        std::reverse_iterator<iterator>;
    using const_reverse_iterator =
        std::reverse_iterator<const_iterator>;

    /// The type of `string_view` returned by the interface
    using string_view_type =
        basic_string_view<CharT, Traits>;

    //
    // Constants
    //

    /// Maximum size of the string excluding the null terminator
    static std::size_t constexpr max_size_n = N;

    /// A special index
    static constexpr size_type npos = size_type(-1);

    //
    // (constructor)
    //

    /// Default constructor (empty string).
    static_string();

    /** Construct with count copies of character `ch`.
    
        The behavior is undefined if `count >= npos`
    */
    static_string(size_type count, CharT ch);

    /// Construct with a substring (pos, other.size()) of `other`.
    template<std::size_t M>
    static_string(static_string<M, CharT, Traits> const& other,
        size_type pos);

    /// Construct with a substring (pos, count) of `other`.
    template<std::size_t M>
    static_string(static_string<M, CharT, Traits> const& other,
        size_type pos, size_type count);

    /// Construct with the first `count` characters of `s`, including nulls.
    static_string(CharT const* s, size_type count);

    /// Construct from a null terminated string.
    static_string(CharT const* s);

    /// Construct from a range of characters
    template<class InputIt>
    static_string(InputIt first, InputIt last);

    /// Copy constructor.
    static_string(static_string const& other);

    /// Copy constructor.
    template<std::size_t M>
    static_string(static_string<M, CharT, Traits> const& other);

    /// Construct from an initializer list
    static_string(std::initializer_list<CharT> init);

    /// Construct from a `string_view`
    explicit
    static_string(string_view_type sv);

    /** Construct from any object convertible to `string_view_type`.

        The range (pos, n) is extracted from the value
        obtained by converting `t` to `string_view_type`,
        and used to construct the string.
    */
#if BOOST_BEAST_DOXYGEN
    template<class T>
#else
    template<class T, class = typename std::enable_if<
        std::is_convertible<T, string_view_type>::value>::type>
#endif
    static_string(T const& t, size_type pos, size_type n);

    //
    // (assignment)
    //

    /// Copy assignment.
    static_string&
    operator=(static_string const& str)
    {
        return assign(str);
    }

    /// Copy assignment.
    template<std::size_t M>
    static_string&
    operator=(static_string<M, CharT, Traits> const& str)
    {
        return assign(str);
    }

    /// Assign from null-terminated string.
    static_string&
    operator=(CharT const* s);

    /// Assign from single character.
    static_string&
    operator=(CharT ch)
    {
        return assign_char(ch,
            std::integral_constant<bool, (N>0)>{});
    }

    /// Assign from initializer list.
    static_string&
    operator=(std::initializer_list<CharT> init)
    {
        return assign(init);
    }

    /// Assign from `string_view_type`.
    static_string&
    operator=(string_view_type sv)
    {
        return assign(sv);
    }

    /// Assign `count` copies of `ch`.
    static_string&
    assign(size_type count, CharT ch);

    /// Assign from another `static_string`
    static_string&
    assign(static_string const& str);

    // VFALCO NOTE this could come in two flavors,
    //             N>M and N<M, and skip the exception
    //             check when N>M

    /// Assign from another `static_string`
    template<std::size_t M>
    static_string&
    assign(static_string<M, CharT, Traits> const& str)
    {
        return assign(str.data(), str.size());
    }

    /// Assign `count` characterss starting at `npos` from `other`.
    template<std::size_t M>
    static_string&
    assign(static_string<M, CharT, Traits> const& str,
        size_type pos, size_type count = npos);

    /// Assign the first `count` characters of `s`, including nulls.
    static_string&
    assign(CharT const* s, size_type count);

    /// Assign a null terminated string.
    static_string&
    assign(CharT const* s)
    {
        return assign(s, Traits::length(s));
    }

    /// Assign from an iterator range of characters.
    template<class InputIt>
    static_string&
    assign(InputIt first, InputIt last);

    /// Assign from initializer list.
    static_string&
    assign(std::initializer_list<CharT> init)
    {
        return assign(init.begin(), init.end());
    }

    /// Assign from `string_view_type`.
    static_string&
    assign(string_view_type str)
    {
        return assign(str.data(), str.size());
    }

    /** Assign from any object convertible to `string_view_type`.

        The range (pos, n) is extracted from the value
        obtained by converting `t` to `string_view_type`,
        and used to assign the string.
    */
    template<class T>
#if BOOST_BEAST_DOXYGEN
    static_string&
#else
    typename std::enable_if<std::is_convertible<T,
        string_view_type>::value, static_string&>::type
#endif
    assign(T const& t,
        size_type pos, size_type count = npos);

    //
    // Element access
    //

    /// Access specified character with bounds checking.
    reference
    at(size_type pos);

    /// Access specified character with bounds checking.
    const_reference
    at(size_type pos) const;

    /// Access specified character.
    reference
    operator[](size_type pos)
    {
        return s_[pos];
    }

    /// Access specified character.
    const_reference
    operator[](size_type pos) const
    {
        return s_[pos];
    }

    /// Accesses the first character.
    CharT&
    front()
    {
        return s_[0];
    }

    /// Accesses the first character.
    CharT const&
    front() const
    {
        return s_[0];
    }

    /// Accesses the last character.
    CharT&
    back()
    {
        return s_[n_-1];
    }

    /// Accesses the last character.
    CharT const&
    back() const
    {
        return s_[n_-1];
    }

    /// Returns a pointer to the first character of a string.
    CharT*
    data()
    {
        return &s_[0];
    }

    /// Returns a pointer to the first character of a string.
    CharT const*
    data() const
    {
        return &s_[0];
    }

    /// Returns a non-modifiable standard C character array version of the string.
    CharT const*
    c_str() const
    {
        return data();
    }

    /// Convert a static string to a `string_view_type`
    operator string_view_type() const
    {
        return basic_string_view<
            CharT, Traits>{data(), size()};
    }

    //
    // Iterators
    //

    /// Returns an iterator to the beginning.
    iterator
    begin()
    {
        return &s_[0];
    }

    /// Returns an iterator to the beginning.
    const_iterator
    begin() const
    {
        return &s_[0];
    }

    /// Returns an iterator to the beginning.
    const_iterator
    cbegin() const
    {
        return &s_[0];
    }

    /// Returns an iterator to the end.
    iterator
    end()
    {
        return &s_[n_];
    }

    /// Returns an iterator to the end.
    const_iterator
    end() const
    {
        return &s_[n_];
    }

    /// Returns an iterator to the end.
    const_iterator
    cend() const
    {
        return &s_[n_];
    }

    /// Returns a reverse iterator to the beginning.
    reverse_iterator
    rbegin()
    {
        return reverse_iterator{end()};
    }

    /// Returns a reverse iterator to the beginning.
    const_reverse_iterator
    rbegin() const
    {
        return const_reverse_iterator{cend()};
    }

    /// Returns a reverse iterator to the beginning.
    const_reverse_iterator
    crbegin() const
    {
        return const_reverse_iterator{cend()};
    }

    /// Returns a reverse iterator to the end.
    reverse_iterator
    rend()
    {
        return reverse_iterator{begin()};
    }

    /// Returns a reverse iterator to the end.
    const_reverse_iterator
    rend() const
    {
        return const_reverse_iterator{cbegin()};
    }

    /// Returns a reverse iterator to the end.
    const_reverse_iterator
    crend() const
    {
        return const_reverse_iterator{cbegin()};
    }

    //
    // Capacity
    //

    /// Returns `true` if the string is empty.
    bool
    empty() const
    {
        return n_ == 0;
    }

    /// Returns the number of characters, excluding the null terminator.
    size_type
    size() const
    {
        return n_;
    }

    /// Returns the number of characters, excluding the null terminator.
    size_type
    length() const
    {
        return size();
    }

    /// Returns the maximum number of characters that can be stored, excluding the null terminator.
    size_type constexpr
    max_size() const
    {
        return N;
    }

    /** Reserves storage.

        This actually just throws an exception if `n > N`,
        otherwise does nothing since the storage is fixed.
    */
    void
    reserve(std::size_t n);

    /// Returns the number of characters that can be held in currently allocated storage.
    size_type constexpr
    capacity() const
    {
        return max_size();
    }
    
    /** Reduces memory usage by freeing unused memory.

        This actually does nothing, since the storage is fixed.
    */
    void
    shrink_to_fit()
    {
    }

    //
    // Operations
    //

    /// Clears the contents.
    void
    clear();

    static_string&
    insert(size_type index, size_type count, CharT ch);

    static_string&
    insert(size_type index, CharT const* s)
    {
        return insert(index, s, Traits::length(s));
    }

    static_string&
    insert(size_type index, CharT const* s, size_type count);

    template<std::size_t M>
    static_string&
    insert(size_type index,
        static_string<M, CharT, Traits> const& str)
    {
        return insert(index, str.data(), str.size());
    }

    template<std::size_t M>
    static_string&
    insert(size_type index,
        static_string<M, CharT, Traits> const& str,
            size_type index_str, size_type count = npos);

    iterator
    insert(const_iterator pos, CharT ch)
    {
        return insert(pos, 1, ch);
    }

    iterator
    insert(const_iterator pos, size_type count, CharT ch);

    template<class InputIt>
#if BOOST_BEAST_DOXYGEN
    iterator
#else
    typename std::enable_if<
        detail::is_input_iterator<InputIt>::value,
            iterator>::type
#endif
    insert(const_iterator pos, InputIt first, InputIt last);

    iterator
    insert(const_iterator pos, std::initializer_list<CharT> init)
    {
        return insert(pos, init.begin(), init.end());
    }

    static_string&
    insert(size_type index, string_view_type str)
    {
        return insert(index, str.data(), str.size());
    }

    template<class T>
#if BOOST_BEAST_DOXYGEN
    static_string&
#else
    typename std::enable_if<
        std::is_convertible<T const&, string_view_type>::value &&
        ! std::is_convertible<T const&, CharT const*>::value,
            static_string&>::type
#endif
    insert(size_type index, T const& t,
        size_type index_str, size_type count = npos);

    static_string&
    erase(size_type index = 0, size_type count = npos);

    iterator
    erase(const_iterator pos);

    iterator
    erase(const_iterator first, const_iterator last);

    void
    push_back(CharT ch);

    void
    pop_back()
    {
        Traits::assign(s_[--n_], 0);
    }

    static_string&
    append(size_type count, CharT ch)
    {
        insert(end(), count, ch);
        return *this;
    }

    template<std::size_t M>
    static_string&
    append(static_string<M, CharT, Traits> const& str)
    {
        insert(size(), str);
        return *this;
    }

    template<std::size_t M>
    static_string&
    append(static_string<M, CharT, Traits> const& str,
        size_type pos, size_type count = npos);

    static_string&
    append(CharT const* s, size_type count)
    {
        insert(size(), s, count);
        return *this;
    }

    static_string&
    append(CharT const* s)
    {
        insert(size(), s);
        return *this;
    }

    template<class InputIt>
#if BOOST_BEAST_DOXYGEN
    static_string&
#else
    typename std::enable_if<
        detail::is_input_iterator<InputIt>::value,
            static_string&>::type
#endif
    append(InputIt first, InputIt last)
    {
        insert(end(), first, last);
        return *this;
    }

    static_string&
    append(std::initializer_list<CharT> init)
    {
        insert(end(), init);
        return *this;
    }

    static_string&
    append(string_view_type sv)
    {
        insert(size(), sv);
        return *this;
    }

    template<class T>
    typename std::enable_if<
        std::is_convertible<T const&, string_view_type>::value &&
        ! std::is_convertible<T const&, CharT const*>::value,
            static_string&>::type
    append(T const& t, size_type pos, size_type count = npos)
    {
        insert(size(), t, pos, count);
        return *this;
    }

    template<std::size_t M>
    static_string&
    operator+=(static_string<M, CharT, Traits> const& str)
    {
        return append(str.data(), str.size());
    }

    static_string&
    operator+=(CharT ch)
    {
        push_back(ch);
        return *this;
    }

    static_string&
    operator+=(CharT const* s)
    {
        return append(s);
    }

    static_string&
    operator+=(std::initializer_list<CharT> init)
    {
        return append(init);
    }

    static_string&
    operator+=(string_view_type const& str)
    {
        return append(str);
    }

    template<std::size_t M>
    int
    compare(static_string<M, CharT, Traits> const& str) const
    {
        return detail::lexicographical_compare<CharT, Traits>(
            &s_[0], n_, &str.s_[0], str.n_);
    }

    template<std::size_t M>
    int
    compare(size_type pos1, size_type count1,
        static_string<M, CharT, Traits> const& str) const
    {
        return detail::lexicographical_compare<CharT, Traits>(
            substr(pos1, count1), str.data(), str.size());
    }

    template<std::size_t M>
    int
    compare(size_type pos1, size_type count1,
        static_string<M, CharT, Traits> const& str,
            size_type pos2, size_type count2 = npos) const
    {
        return detail::lexicographical_compare(
            substr(pos1, count1), str.substr(pos2, count2));
    }

    int
    compare(CharT const* s) const
    {
        return detail::lexicographical_compare<CharT, Traits>(
            &s_[0], n_, s, Traits::length(s));
    }

    int
    compare(size_type pos1, size_type count1,
        CharT const* s) const
    {
        return detail::lexicographical_compare<CharT, Traits>(
            substr(pos1, count1), s, Traits::length(s));
    }

    int
    compare(size_type pos1, size_type count1,
        CharT const*s, size_type count2) const
    {
        return detail::lexicographical_compare<CharT, Traits>(
            substr(pos1, count1), s, count2);
    }

    int
    compare(string_view_type str) const
    {
        return detail::lexicographical_compare<CharT, Traits>(
            &s_[0], n_, str.data(), str.size());
    }

    int
    compare(size_type pos1, size_type count1,
        string_view_type str) const
    {
        return detail::lexicographical_compare<CharT, Traits>(
            substr(pos1, count1), str);
    }

    template<class T>
#if BOOST_BEAST_DOXYGEN
    int
#else
    typename std::enable_if<
        std::is_convertible<T const&, string_view_type>::value &&
        ! std::is_convertible<T const&, CharT const*>::value,
            int>::type
#endif
    compare(size_type pos1, size_type count1,
        T const& t, size_type pos2,
            size_type count2 = npos) const
    {
        return compare(pos1, count1,
            string_view_type(t).substr(pos2, count2));
    }

    string_view_type
    substr(size_type pos = 0, size_type count = npos) const;

    /// Copy a substring (pos, pos+count) to character string pointed to by `dest`.
    size_type
    copy(CharT* dest, size_type count, size_type pos = 0) const;

    /** Changes the number of characters stored.

        If the resulting string is larger, the new
        characters are uninitialized.
    */
    void
    resize(std::size_t n);

    /** Changes the number of characters stored.

        If the resulting string is larger, the new
        characters are initialized to the value of `c`.
    */
    void
    resize(std::size_t n, CharT c);

    /// Exchange the contents of this string with another.
    void
    swap(static_string& str);

    /// Exchange the contents of this string with another.
    template<std::size_t M>
    void
    swap(static_string<M, CharT, Traits>& str);

    //
    // Search
    //

private:
    static_string&
    assign_char(CharT ch, std::true_type);

    static_string&
    assign_char(CharT ch, std::false_type);
};

//
// Disallowed operations
//

// These operations are explicitly deleted since
// there is no reasonable implementation possible.

template<std::size_t N, std::size_t M, class CharT, class Traits>
void
operator+(
    static_string<N, CharT, Traits>const&,
    static_string<M, CharT, Traits>const&) = delete;

template<std::size_t N, class CharT, class Traits>
void
operator+(CharT const*,
    static_string<N, CharT, Traits>const&) = delete;

template<std::size_t N, class CharT, class Traits>
void
operator+(CharT,
    static_string<N, CharT, Traits> const&) = delete;

template<std::size_t N, class CharT, class Traits>
void
operator+(static_string<N, CharT, Traits> const&,
    CharT const*) = delete;

template<std::size_t N, class CharT, class Traits>
void
operator+(static_string<N, CharT, Traits> const&, CharT) = delete;

//
// Non-member functions
//

template<std::size_t N, std::size_t M,
    class CharT, class Traits>
bool
operator==(
    static_string<N, CharT, Traits> const& lhs,
    static_string<M, CharT, Traits> const& rhs)
{
    return lhs.compare(rhs) == 0;
}

template<std::size_t N, std::size_t M,
    class CharT, class Traits>
bool
operator!=(
    static_string<N, CharT, Traits> const& lhs,
    static_string<M, CharT, Traits> const& rhs)
{
    return lhs.compare(rhs) != 0;
}

template<std::size_t N, std::size_t M,
    class CharT, class Traits>
bool
operator<(
    static_string<N, CharT, Traits> const& lhs,
    static_string<M, CharT, Traits> const& rhs)
{
    return lhs.compare(rhs) < 0;
}

template<std::size_t N, std::size_t M,
    class CharT, class Traits>
bool
operator<=(
    static_string<N, CharT, Traits> const& lhs,
    static_string<M, CharT, Traits> const& rhs)
{
    return lhs.compare(rhs) <= 0;
}

template<std::size_t N, std::size_t M,
    class CharT, class Traits>
bool
operator>(
    static_string<N, CharT, Traits> const& lhs,
    static_string<M, CharT, Traits> const& rhs)
{
    return lhs.compare(rhs) > 0;
}

template<std::size_t N, std::size_t M,
    class CharT, class Traits>
bool
operator>=(
    static_string<N, CharT, Traits> const& lhs,
    static_string<M, CharT, Traits> const& rhs)
{
    return lhs.compare(rhs) >= 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator==(
    CharT const* lhs,
    static_string<N, CharT, Traits> const& rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs, Traits::length(lhs),
        rhs.data(), rhs.size()) == 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator==(
    static_string<N, CharT, Traits> const& lhs,
    CharT const* rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs.data(), lhs.size(),
        rhs, Traits::length(rhs)) == 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator!=(
    CharT const* lhs,
    static_string<N, CharT, Traits> const& rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs, Traits::length(lhs),
        rhs.data(), rhs.size()) != 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator!=(
    static_string<N, CharT, Traits> const& lhs,
    CharT const* rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs.data(), lhs.size(),
        rhs, Traits::length(rhs)) != 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator<(
    CharT const* lhs,
    static_string<N, CharT, Traits> const& rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs, Traits::length(lhs),
        rhs.data(), rhs.size()) < 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator<(
    static_string<N, CharT, Traits> const& lhs,
    CharT const* rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs.data(), lhs.size(),
        rhs, Traits::length(rhs)) < 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator<=(
    CharT const* lhs,
    static_string<N, CharT, Traits> const& rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs, Traits::length(lhs),
        rhs.data(), rhs.size()) <= 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator<=(
    static_string<N, CharT, Traits> const& lhs,
    CharT const* rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs.data(), lhs.size(),
        rhs, Traits::length(rhs)) <= 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator>(
    CharT const* lhs,
    static_string<N, CharT, Traits> const& rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs, Traits::length(lhs),
        rhs.data(), rhs.size()) > 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator>(
    static_string<N, CharT, Traits> const& lhs,
    CharT const* rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs.data(), lhs.size(),
        rhs, Traits::length(rhs)) > 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator>=(
    CharT const* lhs,
    static_string<N, CharT, Traits> const& rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs, Traits::length(lhs),
        rhs.data(), rhs.size()) >= 0;
}

template<std::size_t N, class CharT, class Traits>
bool
operator>=(
    static_string<N, CharT, Traits> const& lhs,
    CharT const* rhs)
{
    return detail::lexicographical_compare<CharT, Traits>(
        lhs.data(), lhs.size(),
        rhs, Traits::length(rhs)) >= 0;
}

//
// swap
//

template<std::size_t N, class CharT, class Traits>
void
swap(
    static_string<N, CharT, Traits>& lhs,
    static_string<N, CharT, Traits>& rhs)
{
    lhs.swap(rhs);
}

template<std::size_t N, std::size_t M,
    class CharT, class Traits>
void
swap(
    static_string<N, CharT, Traits>& lhs,
    static_string<M, CharT, Traits>& rhs)
{
    lhs.swap(rhs);
}

//
// Input/Output
//

template<std::size_t N, class CharT, class Traits>
std::basic_ostream<CharT, Traits>& 
operator<<(std::basic_ostream<CharT, Traits>& os, 
    static_string<N, CharT, Traits> const& str)
{
    return os << static_cast<
        beast::basic_string_view<CharT, Traits>>(str);
}

//
// Numeric conversions
//

/** Returns a static string representing an integer as a decimal.

    @param x The signed or unsigned integer to convert.
    This must be an integral type.

    @return A @ref static_string with an implementation defined
    maximum size large enough to hold the longest possible decimal
    representation of any integer of the given type.
*/
template<
    class Integer
#ifndef BOOST_BEAST_DOXYGEN
    ,class = typename std::enable_if<
        std::is_integral<Integer>::value>::type
#endif
>
static_string<detail::max_digits(sizeof(Integer))>
to_static_string(Integer x);

} // beast
} // boost

#include <boost/beast/core/impl/static_string.hpp>

#endif
