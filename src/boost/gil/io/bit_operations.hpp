//
// Copyright 2007-2008 Christian Henning, Andreas Pokorny, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_IO_BIT_OPERATIONS_HPP
#define BOOST_GIL_IO_BIT_OPERATIONS_HPP

#include <boost/gil/io/typedefs.hpp>

#include <array>
#include <cstddef>
#include <type_traits>

namespace boost { namespace gil { namespace detail {

// 1110 1100 -> 0011 0111
template <typename Buffer, typename IsBitAligned>
struct mirror_bits
{
    mirror_bits(bool) {};

    void operator()(Buffer&) {}
    void operator()(byte_t*, std::size_t){}
};

// The functor will generate a lookup table since the
// mirror operation is quite costly.
template <typename Buffer>
struct mirror_bits<Buffer, std::true_type>
{
    mirror_bits(bool apply_operation = true)
        : apply_operation_(apply_operation)
    {
        if(apply_operation_)
        {
            byte_t i = 0;
            do
            {
                lookup_[i] = mirror(i);
            }
            while (i++ != 255);
        }
   }

    void operator()(Buffer& buffer)
    {
        if (apply_operation_)
            for_each(buffer.begin(), buffer.end(), [this](byte_t& c) { lookup(c); });
    }

    void operator()(byte_t *dst, std::size_t size)
    {
        for (std::size_t i = 0; i < size; ++i)
        {
            lookup(*dst);
            ++dst;
        }
    }

private:

    void lookup(byte_t& c)
    {
        c = lookup_[c];
    }

    static byte_t mirror(byte_t c)
    {
        byte_t result = 0;
        for (int i = 0; i < 8; ++i)
        {
            result = result << 1;
            result |= (c & 1);
            c = c >> 1;
        }

        return result;
    }

    std::array<byte_t, 256> lookup_;
    bool apply_operation_;

};

// 0011 1111 -> 1100 0000
template <typename Buffer, typename IsBitAligned>
struct negate_bits
{
    void operator()(Buffer&) {};
};

template <typename Buffer>
struct negate_bits<Buffer, std::true_type>
{
    void operator()(Buffer& buffer)
    {
        for_each(buffer.begin(), buffer.end(),
            negate_bits<Buffer, std::true_type>::negate);
    }

    void operator()(byte_t* dst, std::size_t size)
    {
        for (std::size_t i = 0; i < size; ++i)
        {
            negate(*dst);
            ++dst;
        }
    }

private:

    static void negate(byte_t& b)
    {
        b = ~b;
    }
};

// 11101100 -> 11001110
template <typename Buffer, typename IsBitAligned>
struct swap_half_bytes
{
    void operator()(Buffer&) {};
};

template <typename Buffer>
struct swap_half_bytes<Buffer, std::true_type>
{
    void operator()(Buffer& buffer)
    {
        for_each(buffer.begin(), buffer.end(),
            swap_half_bytes<Buffer, std::true_type>::swap);
    }

    void operator()(byte_t* dst, std::size_t size)
    {
        for (std::size_t i = 0; i < size; ++i)
        {
            swap(*dst);
            ++dst;
        }
    }

private:

    static void swap(byte_t& c)
    {
        c = ((c << 4) & 0xF0) | ((c >> 4) & 0x0F);
    }
};

template <typename Buffer>
struct do_nothing
{
   do_nothing() = default;

   void operator()(Buffer&) {}
};

/// Count consecutive zeros on the right
template <typename T>
inline unsigned int trailing_zeros(T x) noexcept
{
    unsigned int n = 0;

    x = ~x & (x - 1);
    while (x)
    {
        n = n + 1;
        x = x >> 1;
    }

    return n;
}

/// Counts ones in a bit-set
template <typename T>
inline
unsigned int count_ones(T x) noexcept
{
    unsigned int n = 0;

    while (x)
    {
        // clear the least significant bit set
        x &= x - 1;
        ++n;
    }

    return n;
}

}}} // namespace boost::gil::detail

#endif
