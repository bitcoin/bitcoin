//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_BUFFER_HPP
#define BOOST_JSON_DETAIL_BUFFER_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/string_view.hpp>
#include <cstring>

BOOST_JSON_NS_BEGIN
namespace detail {

// A simple string-like temporary static buffer
template<std::size_t N>
class buffer
{
public:
    using size_type = std::size_t;

    buffer() = default;

    bool
    empty() const noexcept
    {
        return size_ == 0;
    }

    string_view
    get() const noexcept
    {
        return {buf_, size_};
    }

    operator string_view() const noexcept
    {
        return get();
    }

    char const*
    data() const noexcept
    {
        return buf_;
    }

    size_type
    size() const noexcept
    {
        return size_;
    }

    size_type
    capacity() const noexcept
    {
        return N - size_;
    }

    size_type
    max_size() const noexcept
    {
        return N;
    }

    void
    clear() noexcept
    {
        size_ = 0;
    }

    void
    push_back(char ch) noexcept
    {
        BOOST_ASSERT(capacity() > 0);
        buf_[size_++] = ch;
    }

    // append an unescaped string
    void
    append(
        char const* s,
        size_type n)
    {
        BOOST_ASSERT(n <= N - size_);
        std::memcpy(buf_ + size_, s, n);
        size_ += n;
    }

    // append valid 32-bit code point as utf8
    void
    append_utf8(
        unsigned long cp) noexcept
    {
        auto dest = buf_ + size_;
        if(cp < 0x80)
        {
            BOOST_ASSERT(size_ <= N - 1);
            dest[0] = static_cast<char>(cp);
            size_ += 1;
            return;
        }

        if(cp < 0x800)
        {
            BOOST_ASSERT(size_ <= N - 2);
            dest[0] = static_cast<char>( (cp >> 6)          | 0xc0);
            dest[1] = static_cast<char>( (cp & 0x3f)        | 0x80);
            size_ += 2;
            return;
        }

        if(cp < 0x10000)
        {
            BOOST_ASSERT(size_ <= N - 3);
            dest[0] = static_cast<char>( (cp >> 12)         | 0xe0);
            dest[1] = static_cast<char>(((cp >> 6) & 0x3f)  | 0x80);
            dest[2] = static_cast<char>( (cp       & 0x3f)  | 0x80);
            size_ += 3;
            return;
        }

        {
            BOOST_ASSERT(size_ <= N - 4);
            dest[0] = static_cast<char>( (cp >> 18)         | 0xf0);
            dest[1] = static_cast<char>(((cp >> 12) & 0x3f) | 0x80);
            dest[2] = static_cast<char>(((cp >> 6)  & 0x3f) | 0x80);
            dest[3] = static_cast<char>( (cp        & 0x3f) | 0x80);
            size_ += 4;
        }
    }
private:
    char buf_[N];
    size_type size_ = 0;
};

} // detail
BOOST_JSON_NS_END

#endif
