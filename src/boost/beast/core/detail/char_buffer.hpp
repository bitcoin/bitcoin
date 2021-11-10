//
// Copyright (c) 2019 Damian Jarek(damian.jarek93@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_DETAIL_CHAR_BUFFER_HPP
#define BOOST_BEAST_CORE_DETAIL_CHAR_BUFFER_HPP

#include <boost/config.hpp>
#include <cstddef>
#include <cstring>
#include <cstdint>

namespace boost {
namespace beast {
namespace detail {

template <std::size_t N>
class char_buffer
{
public:
    bool try_push_back(char c)
    {
        if (size_ == N)
            return false;
        buf_[size_++] = c;
        return true;
    }

    bool try_append(char const* first, char const* last)
    {
        std::size_t const n = last - first;
        if (n > N - size_)
            return false;
        std::memmove(&buf_[size_], first, n);
        size_ += n;
        return true;
    }

    void clear() noexcept
    {
        size_ = 0;
    }

    char* data() noexcept
    {
        return buf_;
    }

    char const* data() const noexcept
    {
        return buf_;
    }

    std::size_t size() const noexcept
    {
        return size_;
    }

    bool empty() const noexcept
    {
        return size_ == 0;
    }

private:
    std::size_t size_= 0;
    char buf_[N];
};

} // detail
} // beast
} // boost

#endif
