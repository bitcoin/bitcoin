//
// Copyright (c) 2019 Damian Jarek(damian.jarek93@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_IMPL_TEMPORARY_BUFFER_IPP
#define BOOST_BEAST_DETAIL_IMPL_TEMPORARY_BUFFER_IPP

#include <boost/beast/core/detail/temporary_buffer.hpp>
#include <boost/beast/core/detail/clamp.hpp>
#include <boost/core/exchange.hpp>
#include <boost/assert.hpp>
#include <memory>
#include <cstring>

namespace boost {
namespace beast {
namespace detail {

void
temporary_buffer::
append(string_view s)
{
    grow(s.size());
    unchecked_append(s);
}

void
temporary_buffer::
append(string_view s1, string_view s2)
{
    grow(s1.size() + s2.size());
    unchecked_append(s1);
    unchecked_append(s2);
}

void
temporary_buffer::
unchecked_append(string_view s)
{
    auto n = s.size();
    std::memcpy(&data_[size_], s.data(), n);
    size_ += n;
}

void
temporary_buffer::
grow(std::size_t n)
{
    if (capacity_ - size_ >= n)
        return;

    auto const capacity = (n + size_) * 2u;
    BOOST_ASSERT(! detail::sum_exceeds(
        n, size_, capacity));
    char* const p = new char[capacity];
    std::memcpy(p, data_, size_);
    deallocate(boost::exchange(data_, p));
    capacity_ = capacity;
}
} // detail
} // beast
} // boost

#endif
