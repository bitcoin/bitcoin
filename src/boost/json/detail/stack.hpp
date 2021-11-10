//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_STACK_HPP
#define BOOST_JSON_DETAIL_STACK_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/storage_ptr.hpp>
#include <cstring>

BOOST_JSON_NS_BEGIN
namespace detail {

class stack
{
    storage_ptr sp_;
    std::size_t cap_ = 0;
    std::size_t size_ = 0;
    char* buf_ = nullptr;

public:
    BOOST_JSON_DECL
    ~stack();

    bool
    empty() const noexcept
    {
        return size_ == 0;
    }

    void
    clear() noexcept
    {
        size_ = 0;
    }

    BOOST_JSON_DECL
    void
    reserve(std::size_t n);

    template<class T>
    void
    push(T const& t)
    {
        auto const n = sizeof(T);
        // If this assert goes off, it
        // means the calling code did not
        // reserve enough to prevent a
        // reallocation.
        //BOOST_ASSERT(cap_ >= size_ + n);
        reserve(size_ + n);
        std::memcpy(
            buf_ + size_, &t, n);
        size_ += n;
    }

    template<class T>
    void
    push_unchecked(T const& t)
    {
        auto const n = sizeof(T);
        BOOST_ASSERT(size_ + n <= cap_);
        std::memcpy(
            buf_ + size_, &t, n);
        size_ += n;
    }

    template<class T>
    void
    peek(T& t)
    {
        auto const n = sizeof(T);
        BOOST_ASSERT(size_ >= n);
        std::memcpy(&t,
            buf_ + size_ - n, n);
    }

    template<class T>
    void
    pop(T& t)
    {
        auto const n = sizeof(T);
        BOOST_ASSERT(size_ >= n);
        size_ -= n;
        std::memcpy(
            &t, buf_ + size_, n);
    }
};

} // detail
BOOST_JSON_NS_END

#endif
