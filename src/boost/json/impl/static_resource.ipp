//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_STATIC_RESOURCE_IPP
#define BOOST_JSON_IMPL_STATIC_RESOURCE_IPP

#include <boost/json/static_resource.hpp>
#include <boost/json/detail/align.hpp>
#include <boost/json/detail/except.hpp>
#include <memory>

BOOST_JSON_NS_BEGIN

static_resource::
~static_resource() noexcept = default;

static_resource::
static_resource(
    unsigned char* buffer,
    std::size_t size) noexcept
    : p_(buffer)
    , n_(size)
    , size_(size)
{
}

void
static_resource::
release() noexcept
{
    p_ = reinterpret_cast<
        char*>(p_) - (size_ - n_);
    n_ = size_;
}

void*
static_resource::
do_allocate(
    std::size_t n,
    std::size_t align)
{
    auto p = detail::align(
        align, n, p_, n_);
    if(! p)
        detail::throw_bad_alloc(
            BOOST_JSON_SOURCE_POS);
    p_ = reinterpret_cast<char*>(p) + n;
    n_ -= n;
    return p;
}

void
static_resource::
do_deallocate(
    void*,
    std::size_t,
    std::size_t)
{
    // do nothing
}

bool
static_resource::
do_is_equal(
    memory_resource const& mr) const noexcept
{
    return this == &mr;
}

BOOST_JSON_NS_END

#endif
