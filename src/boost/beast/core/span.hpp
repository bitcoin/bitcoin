//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_SPAN_HPP
#define BOOST_BEAST_CORE_SPAN_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/detail/type_traits.hpp>
#include <algorithm>
#include <iterator>
#include <string>
#include <type_traits>

namespace boost {
namespace beast {

/** A range of bytes expressed as a ContiguousContainer

    This class implements a non-owning reference to a storage
    area of a certain size and having an underlying integral
    type with size of 1.

    @tparam T The type pointed to by span iterators
*/
template<class T>
class span
{
    T* data_ = nullptr;
    std::size_t size_ = 0;

public:
    /// The type of value, including cv qualifiers
    using element_type = T;

    /// The type of value of each span element
    using value_type = typename std::remove_const<T>::type;

    /// The type of integer used to index the span
    using index_type = std::ptrdiff_t;

    /// A pointer to a span element
    using pointer = T*;

    /// A reference to a span element
    using reference = T&;

    /// The iterator used by the container
    using iterator = pointer;

    /// The const pointer used by the container
    using const_pointer = T const*;

    /// The const reference used by the container
    using const_reference = T const&;

    /// The const iterator used by the container
    using const_iterator = const_pointer;

    /// Constructor
    span() = default;

    /// Constructor
    span(span const&) = default;

    /// Assignment
    span& operator=(span const&) = default;

    /** Constructor

        @param data A pointer to the beginning of the range of elements

        @param size The number of elements pointed to by `data`
    */
    span(T* data, std::size_t size)
        : data_(data), size_(size)
    {
    }

    /** Constructor

        @param container The container to construct from
    */
    template<class ContiguousContainer
#if ! BOOST_BEAST_DOXYGEN
        , class = typename std::enable_if<
          detail::is_contiguous_container<
                ContiguousContainer, T>::value>::type
#endif
    >
    explicit
    span(ContiguousContainer&& container)
        : data_(container.data())
        , size_(container.size())
    {
    }

#if ! BOOST_BEAST_DOXYGEN
    template<class CharT, class Traits, class Allocator>
    explicit
    span(std::basic_string<CharT, Traits, Allocator>& s)
        : data_(&s[0])
        , size_(s.size())
    {
    }

    template<class CharT, class Traits, class Allocator>
    explicit
    span(std::basic_string<CharT, Traits, Allocator> const& s)
        : data_(s.data())
        , size_(s.size())
    {
    }
#endif

    /** Assignment

        @param container The container to assign from
    */
    template<class ContiguousContainer>
#if BOOST_BEAST_DOXYGEN
    span&
#else
    typename std::enable_if<detail::is_contiguous_container<
        ContiguousContainer, T>::value,
    span&>::type
#endif
    operator=(ContiguousContainer&& container)
    {
        data_ = container.data();
        size_ = container.size();
        return *this;
    }

#if ! BOOST_BEAST_DOXYGEN
    template<class CharT, class Traits, class Allocator>
    span&
    operator=(std::basic_string<
        CharT, Traits, Allocator>& s)
    {
        data_ = &s[0];
        size_ = s.size();
        return *this;
    }

    template<class CharT, class Traits, class Allocator>
    span&
    operator=(std::basic_string<
        CharT, Traits, Allocator> const& s)
    {
        data_ = s.data();
        size_ = s.size();
        return *this;
    }
#endif

    /// Returns `true` if the span is empty
    bool
    empty() const
    {
        return size_ == 0;
    }

    /// Returns a pointer to the beginning of the span
    T*
    data() const
    {
        return data_;
    }

    /// Returns the number of elements in the span
    std::size_t
    size() const
    {
        return size_;
    }

    /// Returns an iterator to the beginning of the span
    const_iterator
    begin() const
    {
        return data_;
    }

    /// Returns an iterator to the beginning of the span
    const_iterator
    cbegin() const
    {
        return data_;
    }

    /// Returns an iterator to one past the end of the span
    const_iterator
    end() const
    {
        return data_ + size_;
    }

    /// Returns an iterator to one past the end of the span
    const_iterator
    cend() const
    {
        return data_ + size_;
    }
};

} // beast
} // boost

#endif
