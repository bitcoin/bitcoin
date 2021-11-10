//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_IMPL_READ_SIZE_HPP
#define BOOST_BEAST_IMPL_READ_SIZE_HPP

#include <boost/asio/buffer.hpp>
#include <boost/assert.hpp>
#include <stdexcept>
#include <type_traits>

namespace boost {
namespace beast {

namespace detail {

template<class T, class = void>
struct has_read_size_helper : std::false_type {};

template<class T>
struct has_read_size_helper<T, decltype(
    read_size_helper(std::declval<T&>(), 512),
    (void)0)> : std::true_type
{
};

template<class DynamicBuffer>
std::size_t
read_size(DynamicBuffer& buffer,
    std::size_t max_size, std::true_type)
{
    return read_size_helper(buffer, max_size);
}

template<class DynamicBuffer>
std::size_t
read_size(DynamicBuffer& buffer,
    std::size_t max_size, std::false_type)
{
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    auto const size = buffer.size();
    auto const limit = buffer.max_size() - size;
    BOOST_ASSERT(size <= buffer.max_size());
    return std::min<std::size_t>(
        std::max<std::size_t>(512, buffer.capacity() - size),
        std::min<std::size_t>(max_size, limit));
}

} // detail

template<class DynamicBuffer>
std::size_t
read_size(
    DynamicBuffer& buffer, std::size_t max_size)
{
    return detail::read_size(buffer, max_size,
        detail::has_read_size_helper<DynamicBuffer>{});
}

template<class DynamicBuffer>
std::size_t
read_size_or_throw(
    DynamicBuffer& buffer, std::size_t max_size)
{
    auto const n = read_size(buffer, max_size);
    if(n == 0)
        BOOST_THROW_EXCEPTION(std::length_error{
            "buffer overflow"});
    return n;
}

} // beast
} // boost

#endif
