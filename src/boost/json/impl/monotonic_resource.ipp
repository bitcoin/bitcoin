//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_MONOTONIC_RESOURCE_IPP
#define BOOST_JSON_IMPL_MONOTONIC_RESOURCE_IPP

#include <boost/json/monotonic_resource.hpp>
#include <boost/json/detail/align.hpp>
#include <boost/json/detail/except.hpp>

#include <memory>

BOOST_JSON_NS_BEGIN

struct alignas(detail::max_align_t)
    monotonic_resource::block : block_base
{
};

constexpr
std::size_t
monotonic_resource::
max_size()
{
    return std::size_t(-1) - sizeof(block);
}

// lowest power of 2 greater than or equal to n
std::size_t
monotonic_resource::
round_pow2(
    std::size_t n) noexcept
{
    if(n & (n - 1))
        return next_pow2(n);
    return n;
}

// lowest power of 2 greater than n
std::size_t
monotonic_resource::
next_pow2(
    std::size_t n) noexcept
{
    std::size_t result = min_size_;
    while(result <= n)
    {
        if(result >= max_size() - result)
        {
            // overflow
            result = max_size();
            break;
        }
        result *= 2;
    }
    return result;
}

//----------------------------------------------------------

monotonic_resource::
~monotonic_resource()
{
    release();
}

monotonic_resource::
monotonic_resource(
    std::size_t initial_size,
    storage_ptr upstream) noexcept
    : buffer_{
        nullptr, 0, 0, nullptr}
    , next_size_(round_pow2(initial_size))
    , upstream_(std::move(upstream))
{
}

monotonic_resource::
monotonic_resource(
    unsigned char* buffer,
    std::size_t size,
    storage_ptr upstream) noexcept
    : buffer_{
        buffer, size, size, nullptr}
    , next_size_(next_pow2(size))
    , upstream_(std::move(upstream))
{
}

void
monotonic_resource::
release() noexcept
{
    auto p = head_;
    while(p != &buffer_)
    {
        auto next = p->next;
        upstream_->deallocate(p, p->size);
        p = next;
    }
    buffer_.p = reinterpret_cast<
        unsigned char*>(buffer_.p) - (
            buffer_.size - buffer_.avail);
    buffer_.avail = buffer_.size;
    head_ = &buffer_;
}

void*
monotonic_resource::
do_allocate(
    std::size_t n,
    std::size_t align)
{
    auto p = detail::align(
        align, n, head_->p, head_->avail);
    if(p)
    {
        head_->p = reinterpret_cast<
            unsigned char*>(p) + n;
        head_->avail -= n;
        return p;
    }

    if(next_size_ < n)
        next_size_ = round_pow2(n);
    auto b = ::new(upstream_->allocate(
        sizeof(block) + next_size_)) block;
    b->p = b + 1;
    b->avail = next_size_;
    b->size = next_size_;
    b->next = head_;
    head_ = b;
    next_size_ = next_pow2(next_size_);

    p = detail::align(
        align, n, head_->p, head_->avail);
    BOOST_ASSERT(p);
    head_->p = reinterpret_cast<
        unsigned char*>(p) + n;
    head_->avail -= n;
    return p;
}

void
monotonic_resource::
do_deallocate(
    void*,
    std::size_t,
    std::size_t)
{
    // do nothing
}

bool
monotonic_resource::
do_is_equal(
    memory_resource const& mr) const noexcept
{
    return this == &mr;
}

BOOST_JSON_NS_END

#endif
