//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_IMPL_BUFFERS_PREFIX_HPP
#define BOOST_BEAST_IMPL_BUFFERS_PREFIX_HPP

#include <boost/beast/core/buffer_traits.hpp>
#include <boost/config/workaround.hpp>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {

template<class Buffers>
class buffers_prefix_view<Buffers>::const_iterator
{
    friend class buffers_prefix_view<Buffers>;

    buffers_prefix_view const* b_ = nullptr;
    std::size_t remain_ = 0;
    iter_type it_{};

public:
#if BOOST_WORKAROUND(BOOST_MSVC, < 1910)
    using value_type = typename std::conditional<
        boost::is_convertible<typename
            std::iterator_traits<iter_type>::value_type,
                net::mutable_buffer>::value,
                    net::mutable_buffer,
                        net::const_buffer>::type;
#else
    using value_type = buffers_type<Buffers>;
#endif

    BOOST_STATIC_ASSERT(std::is_same<
        typename const_iterator::value_type,
        typename buffers_prefix_view::value_type>::value);

    using pointer = value_type const*;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category =
        std::bidirectional_iterator_tag;

    const_iterator() = default;
    const_iterator(
        const_iterator const& other) = default;
    const_iterator& operator=(
        const_iterator const& other) = default;

    bool
    operator==(const_iterator const& other) const
    {
        return b_ == other.b_ && it_ == other.it_;
    }

    bool
    operator!=(const_iterator const& other) const
    {
        return !(*this == other);
    }

    reference
    operator*() const
    {
        value_type v(*it_);
        if(remain_ < v.size())
            return {v.data(), remain_};
        return v;
    }

    pointer
    operator->() const = delete;

    const_iterator&
    operator++()
    {
        value_type const v = *it_++;
        remain_ -= v.size();
        return *this;
    }

    const_iterator
    operator++(int)
    {
        auto temp = *this;
        value_type const v = *it_++;
        remain_ -= v.size();
        return temp;
    }

    const_iterator&
    operator--()
    {
        value_type const v = *--it_;
        remain_ += v.size();
        return *this;
    }

    const_iterator
    operator--(int)
    {
        auto temp = *this;
        value_type const v = *--it_;
        remain_ += v.size();
        return temp;
    }

private:
    const_iterator(
        buffers_prefix_view const& b,
        std::true_type)
        : b_(&b)
        , remain_(b.remain_)
        , it_(b_->end_)
    {
    }

    const_iterator(
        buffers_prefix_view const& b,
        std::false_type)
        : b_(&b)
        , remain_(b_->size_)
        , it_(net::buffer_sequence_begin(b_->bs_))
    {
    }
};

//------------------------------------------------------------------------------

template<class Buffers>
void
buffers_prefix_view<Buffers>::
setup(std::size_t size)
{
    size_ = 0;
    remain_ = 0;
    end_ = net::buffer_sequence_begin(bs_);
    auto const last = bs_.end();
    while(end_ != last)
    {
        auto const len = buffer_bytes(*end_++);
        if(len >= size)
        {
            size_ += size;

            // by design, this subtraction can wrap
            BOOST_STATIC_ASSERT(std::is_unsigned<
                decltype(remain_)>::value);
            remain_ = size - len;
            break;
        }
        size -= len;
        size_ += len;
    }
}

template<class Buffers>
buffers_prefix_view<Buffers>::
buffers_prefix_view(
    buffers_prefix_view const& other,
    std::size_t dist)
    : bs_(other.bs_)
    , size_(other.size_)
    , remain_(other.remain_)
    , end_(std::next(bs_.begin(), dist))
{
}

template<class Buffers>
buffers_prefix_view<Buffers>::
buffers_prefix_view(buffers_prefix_view const& other)
    : buffers_prefix_view(other,
        std::distance<iter_type>(
            net::buffer_sequence_begin(other.bs_),
                other.end_))
{
}

template<class Buffers>
auto
buffers_prefix_view<Buffers>::
operator=(buffers_prefix_view const& other) ->
    buffers_prefix_view&
{
    auto const dist = std::distance<iter_type>(
        net::buffer_sequence_begin(other.bs_),
        other.end_);
    bs_ = other.bs_;
    size_ = other.size_;
    remain_ = other.remain_;
    end_ = std::next(
        net::buffer_sequence_begin(bs_),
            dist);
    return *this;
}

template<class Buffers>
buffers_prefix_view<Buffers>::
buffers_prefix_view(
    std::size_t size,
    Buffers const& bs)
    : bs_(bs)
{
    setup(size);
}

template<class Buffers>
template<class... Args>
buffers_prefix_view<Buffers>::
buffers_prefix_view(
    std::size_t size,
    boost::in_place_init_t,
    Args&&... args)
    : bs_(std::forward<Args>(args)...)
{
    setup(size);
}

template<class Buffers>
auto
buffers_prefix_view<Buffers>::
begin() const ->
    const_iterator
{
    return const_iterator{
        *this, std::false_type{}};
}

template<class Buffers>
auto
buffers_prefix_view<Buffers>::
end() const ->
    const_iterator
{
    return const_iterator{
        *this, std::true_type{}};
}

//------------------------------------------------------------------------------

template<>
class buffers_prefix_view<net::const_buffer>
    : public net::const_buffer
{
public:
    using net::const_buffer::const_buffer;
    buffers_prefix_view(buffers_prefix_view const&) = default;
    buffers_prefix_view& operator=(buffers_prefix_view const&) = default;

    buffers_prefix_view(
        std::size_t size,
        net::const_buffer buffer)
        : net::const_buffer(
            buffer.data(),
            std::min<std::size_t>(size, buffer.size())
        #if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
            , buffer.get_debug_check()
        #endif
            )
    {
    }

    template<class... Args>
    buffers_prefix_view(
        std::size_t size,
        boost::in_place_init_t,
        Args&&... args)
        : buffers_prefix_view(size,
            net::const_buffer(
                std::forward<Args>(args)...))
    {
    }
};

//------------------------------------------------------------------------------

template<>
class buffers_prefix_view<net::mutable_buffer>
    : public net::mutable_buffer
{
public:
    using net::mutable_buffer::mutable_buffer;
    buffers_prefix_view(buffers_prefix_view const&) = default;
    buffers_prefix_view& operator=(buffers_prefix_view const&) = default;

    buffers_prefix_view(
        std::size_t size,
        net::mutable_buffer buffer)
        : net::mutable_buffer(
            buffer.data(),
            std::min<std::size_t>(size, buffer.size())
        #if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
            , buffer.get_debug_check()
        #endif
            )
    {
    }

    template<class... Args>
    buffers_prefix_view(
        std::size_t size,
        boost::in_place_init_t,
        Args&&... args)
        : buffers_prefix_view(size,
            net::mutable_buffer(
                std::forward<Args>(args)...))
    {
    }
};

} // beast
} // boost

#endif
