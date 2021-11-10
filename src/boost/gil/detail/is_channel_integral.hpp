//
// Copyright 2019 Mateusz Loskot <mateusz at loskot dot net>
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_DETAIL_IS_CHANNEL_INTEGRAL_HPP
#define BOOST_GIL_DETAIL_IS_CHANNEL_INTEGRAL_HPP

#include <boost/gil/channel.hpp>

#include <type_traits>

namespace boost { namespace gil { namespace detail {

template <typename ChannelValue>
struct is_channel_integral : std::is_integral<ChannelValue> {};

template <int NumBits>
struct is_channel_integral<boost::gil::packed_channel_value<NumBits>> : std::true_type {};

template <typename BitField, int FirstBit, int NumBits, bool IsMutable>
struct is_channel_integral
    <
        boost::gil::packed_channel_reference<BitField, FirstBit, NumBits, IsMutable>
    > : std::true_type
{};

template <typename BitField, int NumBits, bool IsMutable>
struct is_channel_integral
    <
        boost::gil::packed_dynamic_channel_reference<BitField, NumBits, IsMutable>
    > : std::true_type
{};

template <typename BaseChannelValue, typename MinVal, typename MaxVal>
struct is_channel_integral
    <
        boost::gil::scoped_channel_value<BaseChannelValue, MinVal, MaxVal>
    > : std::is_integral<BaseChannelValue>
{};

}}} //namespace boost::gil::detail

#endif
