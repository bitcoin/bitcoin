//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_ARRAY_HPP
#define BOOST_JSON_DETAIL_ARRAY_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/storage_ptr.hpp>
#include <cstddef>

BOOST_JSON_NS_BEGIN

class value;

namespace detail {

class unchecked_array
{
    value* data_;
    std::size_t size_;
    storage_ptr const& sp_;

public:
    inline
    ~unchecked_array();

    unchecked_array(
        value* data,
        std::size_t size,
        storage_ptr const& sp) noexcept
        : data_(data)
        , size_(size)
        , sp_(sp)
    {
    }

    unchecked_array(
        unchecked_array&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
        , sp_(other.sp_)
    {
        other.data_ = nullptr;
    }

    storage_ptr const&
    storage() const noexcept
    {
        return sp_;
    }

    std::size_t
    size() const noexcept
    {
        return size_;
    }

    inline
    void
    relocate(value* dest) noexcept;
};

} // detail

BOOST_JSON_NS_END

// includes are at the bottom of <boost/json/value.hpp>

#endif
