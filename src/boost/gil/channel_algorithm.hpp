//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_GIL_CHANNEL_ALGORITHM_HPP
#define BOOST_GIL_GIL_CHANNEL_ALGORITHM_HPP

#include <boost/gil/channel.hpp>
#include <boost/gil/promote_integral.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/detail/is_channel_integral.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <limits>
#include <type_traits>

namespace boost { namespace gil {

namespace detail {

// some forward declarations
template <typename SrcChannelV, typename DstChannelV, bool SrcIsIntegral, bool DstIsIntegral>
struct channel_converter_unsigned_impl;

template <typename SrcChannelV, typename DstChannelV, bool SrcIsGreater>
struct channel_converter_unsigned_integral;

template <typename SrcChannelV, typename DstChannelV, bool SrcLessThanDst, bool SrcDivisible>
struct channel_converter_unsigned_integral_impl;

template <typename SrcChannelV, typename DstChannelV, bool SrcLessThanDst, bool CannotFitInInteger>
struct channel_converter_unsigned_integral_nondivisible;

//////////////////////////////////////
////  unsigned_integral_max_value - given an unsigned integral channel type,
//// returns its maximum value as an integral constant
//////////////////////////////////////

template <typename UnsignedIntegralChannel>
struct unsigned_integral_max_value
    : std::integral_constant
    <
        UnsignedIntegralChannel,
        (std::numeric_limits<UnsignedIntegralChannel>::max)()
    >
{};

template <>
struct unsigned_integral_max_value<uint8_t>
    : std::integral_constant<uint32_t, 0xFF>
{};

template <>
struct unsigned_integral_max_value<uint16_t>
    : std::integral_constant<uint32_t, 0xFFFF>
{};

template <>
struct unsigned_integral_max_value<uint32_t>
    : std::integral_constant<uintmax_t, 0xFFFFFFFF>
{};

template <int K>
struct unsigned_integral_max_value<packed_channel_value<K>>
    : std::integral_constant
    <
        typename packed_channel_value<K>::integer_t,
        (uint64_t(1)<<K)-1
    >
{};

//////////////////////////////////////
//// unsigned_integral_num_bits - given an unsigned integral channel type,
//// returns the minimum number of bits needed to represent it
//////////////////////////////////////

template <typename UnsignedIntegralChannel>
struct unsigned_integral_num_bits
    : std::integral_constant<int, sizeof(UnsignedIntegralChannel) * 8>
{};

template <int K>
struct unsigned_integral_num_bits<packed_channel_value<K>>
    : std::integral_constant<int, K>
{};

} // namespace detail

/// \defgroup ChannelConvertAlgorithm channel_convert
/// \brief Converting from one channel type to another
/// \ingroup ChannelAlgorithm
///
/// Conversion is done as a simple linear mapping of one channel range to the other,
/// such that the minimum/maximum value of the source maps to the minimum/maximum value of the destination.
/// One implication of this is that the value 0 of signed channels may not be preserved!
///
/// When creating new channel models, it is often a good idea to provide specializations for the channel conversion algorithms, for
/// example, for performance optimizations. If the new model is an integral type that can be signed, it is easier to define the conversion
/// only for the unsigned type (\p channel_converter_unsigned) and provide specializations of \p detail::channel_convert_to_unsigned
/// and \p detail::channel_convert_from_unsigned to convert between the signed and unsigned type.
///
/// Example:
/// \code
/// // float32_t is a floating point channel with range [0.0f ... 1.0f]
/// float32_t src_channel = channel_traits<float32_t>::max_value();
/// assert(src_channel == 1);
///
/// // uint8_t is 8-bit unsigned integral channel (aliased from unsigned char)
/// uint8_t dst_channel = channel_convert<uint8_t>(src_channel);
/// assert(dst_channel == 255);     // max value goes to max value
/// \endcode

///
/// \defgroup ChannelConvertUnsignedAlgorithm channel_converter_unsigned
/// \ingroup ChannelConvertAlgorithm
/// \brief Convert one unsigned/floating point channel to another. Converts both the channel type and range
/// @{

//////////////////////////////////////
////  channel_converter_unsigned
//////////////////////////////////////

template <typename SrcChannelV, typename DstChannelV>     // Model ChannelValueConcept
struct channel_converter_unsigned
    : detail::channel_converter_unsigned_impl
    <
        SrcChannelV,
        DstChannelV,
        detail::is_channel_integral<SrcChannelV>::value,
        detail::is_channel_integral<DstChannelV>::value
    >
{};

/// \brief Converting a channel to itself - identity operation
template <typename T> struct channel_converter_unsigned<T,T> : public detail::identity<T> {};

namespace detail {

//////////////////////////////////////
////  channel_converter_unsigned_impl
//////////////////////////////////////

/// \brief This is the default implementation. Performance specializatons are provided
template <typename SrcChannelV, typename DstChannelV, bool SrcIsIntegral, bool DstIsIntegral>
struct channel_converter_unsigned_impl {
    using argument_type = SrcChannelV;
    using result_type = DstChannelV;
    DstChannelV operator()(SrcChannelV src) const {
        return DstChannelV(channel_traits<DstChannelV>::min_value() +
            (src - channel_traits<SrcChannelV>::min_value()) / channel_range<SrcChannelV>() * channel_range<DstChannelV>());
    }
private:
    template <typename C>
    static double channel_range() {
        return double(channel_traits<C>::max_value()) - double(channel_traits<C>::min_value());
    }
};

// When both the source and the destination are integral channels, perform a faster conversion
template <typename SrcChannelV, typename DstChannelV>
struct channel_converter_unsigned_impl<SrcChannelV, DstChannelV, true, true>
    : channel_converter_unsigned_integral
    <
        SrcChannelV,
        DstChannelV,
        mp11::mp_less
        <
            unsigned_integral_max_value<SrcChannelV>,
            unsigned_integral_max_value<DstChannelV>
        >::value
    >
{};

//////////////////////////////////////
////  channel_converter_unsigned_integral
//////////////////////////////////////

template <typename SrcChannelV, typename DstChannelV>
struct channel_converter_unsigned_integral<SrcChannelV,DstChannelV,true>
    : public channel_converter_unsigned_integral_impl<SrcChannelV,DstChannelV,true,
    !(unsigned_integral_max_value<DstChannelV>::value % unsigned_integral_max_value<SrcChannelV>::value) > {};

template <typename SrcChannelV, typename DstChannelV>
struct channel_converter_unsigned_integral<SrcChannelV,DstChannelV,false>
    : public channel_converter_unsigned_integral_impl<SrcChannelV,DstChannelV,false,
    !(unsigned_integral_max_value<SrcChannelV>::value % unsigned_integral_max_value<DstChannelV>::value) > {};


//////////////////////////////////////
////  channel_converter_unsigned_integral_impl
//////////////////////////////////////

// Both source and destination are unsigned integral channels,
// the src max value is less than the dst max value,
// and the dst max value is divisible by the src max value
template <typename SrcChannelV, typename DstChannelV>
struct channel_converter_unsigned_integral_impl<SrcChannelV,DstChannelV,true,true> {
    DstChannelV operator()(SrcChannelV src) const {
        using integer_t = typename unsigned_integral_max_value<DstChannelV>::value_type;
        static const integer_t mul = unsigned_integral_max_value<DstChannelV>::value / unsigned_integral_max_value<SrcChannelV>::value;
        return DstChannelV(src * mul);
    }
};

// Both source and destination are unsigned integral channels,
// the dst max value is less than (or equal to) the src max value,
// and the src max value is divisible by the dst max value
template <typename SrcChannelV, typename DstChannelV>
struct channel_converter_unsigned_integral_impl<SrcChannelV,DstChannelV,false,true> {
    DstChannelV operator()(SrcChannelV src) const {
        using integer_t = typename unsigned_integral_max_value<SrcChannelV>::value_type;
        static const integer_t div = unsigned_integral_max_value<SrcChannelV>::value / unsigned_integral_max_value<DstChannelV>::value;
        static const integer_t div2 = div/2;
        return DstChannelV((src + div2) / div);
    }
};

// Prevent overflow for the largest integral type
template <typename DstChannelV>
struct channel_converter_unsigned_integral_impl<uintmax_t,DstChannelV,false,true> {
    DstChannelV operator()(uintmax_t src) const {
        static const uintmax_t div = unsigned_integral_max_value<uint32_t>::value / unsigned_integral_max_value<DstChannelV>::value;
        static const uintmax_t div2 = div/2;
        if (src > unsigned_integral_max_value<uintmax_t>::value - div2)
            return unsigned_integral_max_value<DstChannelV>::value;
        return DstChannelV((src + div2) / div);
    }
};

// Both source and destination are unsigned integral channels,
// and the dst max value is not divisible by the src max value
// See if you can represent the expression (src * dst_max) / src_max in integral form
template <typename SrcChannelV, typename DstChannelV, bool SrcLessThanDst>
struct channel_converter_unsigned_integral_impl<SrcChannelV, DstChannelV, SrcLessThanDst, false>
    : channel_converter_unsigned_integral_nondivisible
    <
        SrcChannelV,
        DstChannelV,
        SrcLessThanDst,
        mp11::mp_less
        <
            unsigned_integral_num_bits<uintmax_t>,
            mp11::mp_plus
            <
                unsigned_integral_num_bits<SrcChannelV>,
                unsigned_integral_num_bits<DstChannelV>
            >
        >::value
    >
{};

// Both source and destination are unsigned integral channels,
// the src max value is less than the dst max value,
// and the dst max value is not divisible by the src max value
// The expression (src * dst_max) / src_max fits in an integer
template <typename SrcChannelV, typename DstChannelV>
struct channel_converter_unsigned_integral_nondivisible<SrcChannelV, DstChannelV, true, false>
{
    DstChannelV operator()(SrcChannelV src) const
    {
        using dest_t = typename base_channel_type<DstChannelV>::type;
        return DstChannelV(
            static_cast<dest_t>(src * unsigned_integral_max_value<DstChannelV>::value)
            / unsigned_integral_max_value<SrcChannelV>::value);
    }
};

// Both source and destination are unsigned integral channels,
// the src max value is less than the dst max value,
// and the dst max value is not divisible by the src max value
// The expression (src * dst_max) / src_max cannot fit in an integer (overflows). Use a double
template <typename SrcChannelV, typename DstChannelV>
struct channel_converter_unsigned_integral_nondivisible<SrcChannelV, DstChannelV, true, true>
{
    DstChannelV operator()(SrcChannelV src) const
    {
        static const double mul
            = unsigned_integral_max_value<DstChannelV>::value
            / double(unsigned_integral_max_value<SrcChannelV>::value);
        return DstChannelV(src * mul);
    }
};

// Both source and destination are unsigned integral channels,
// the dst max value is less than (or equal to) the src max value,
// and the src max value is not divisible by the dst max value
template <typename SrcChannelV, typename DstChannelV, bool CannotFit>
struct channel_converter_unsigned_integral_nondivisible<SrcChannelV,DstChannelV,false,CannotFit> {
    DstChannelV operator()(SrcChannelV src) const {

        using src_integer_t = typename detail::unsigned_integral_max_value<SrcChannelV>::value_type;
        using dst_integer_t = typename detail::unsigned_integral_max_value<DstChannelV>::value_type;

        static const double div = unsigned_integral_max_value<SrcChannelV>::value
                                / static_cast< double >( unsigned_integral_max_value<DstChannelV>::value );

        static const src_integer_t div2 = static_cast< src_integer_t >( div / 2.0 );

        return DstChannelV( static_cast< dst_integer_t >(( static_cast< double >( src + div2 ) / div )));
    }
};

} // namespace detail

/////////////////////////////////////////////////////
///  float32_t conversion
/////////////////////////////////////////////////////

template <typename DstChannelV> struct channel_converter_unsigned<float32_t,DstChannelV> {
    using argument_type = float32_t;
    using result_type = DstChannelV;
    DstChannelV operator()(float32_t x) const
    {
        using dst_integer_t = typename detail::unsigned_integral_max_value<DstChannelV>::value_type;
        return DstChannelV( static_cast< dst_integer_t >(x*channel_traits<DstChannelV>::max_value()+0.5f ));
    }
};

template <typename SrcChannelV> struct channel_converter_unsigned<SrcChannelV,float32_t> {
    using argument_type = float32_t;
    using result_type = SrcChannelV;
    float32_t operator()(SrcChannelV   x) const { return float32_t(x/float(channel_traits<SrcChannelV>::max_value())); }
};

template <> struct channel_converter_unsigned<float32_t,float32_t> {
    using argument_type = float32_t;
    using result_type = float32_t;
    float32_t operator()(float32_t   x) const { return x; }
};


/// \brief 32 bit <-> float channel conversion
template <> struct channel_converter_unsigned<uint32_t,float32_t> {
    using argument_type = uint32_t;
    using result_type = float32_t;
    float32_t operator()(uint32_t x) const {
        // unfortunately without an explicit check it is possible to get a round-off error. We must ensure that max_value of uint32_t matches max_value of float32_t
        if (x>=channel_traits<uint32_t>::max_value()) return channel_traits<float32_t>::max_value();
        return float(x) / float(channel_traits<uint32_t>::max_value());
    }
};
/// \brief 32 bit <-> float channel conversion
template <> struct channel_converter_unsigned<float32_t,uint32_t> {
    using argument_type = float32_t;
    using result_type = uint32_t;
    uint32_t operator()(float32_t x) const {
        // unfortunately without an explicit check it is possible to get a round-off error. We must ensure that max_value of uint32_t matches max_value of float32_t
        if (x>=channel_traits<float32_t>::max_value())
            return channel_traits<uint32_t>::max_value();

        auto const max_value = channel_traits<uint32_t>::max_value();
        auto const result = x * static_cast<float32_t::base_channel_t>(max_value) + 0.5f;
        return static_cast<uint32_t>(result);
    }
};

/// @}

namespace detail {
// Converting from signed to unsigned integral channel.
// It is both a unary function, and a metafunction (thus requires the 'type' nested alias, which equals result_type)
template <typename ChannelValue>     // Model ChannelValueConcept
struct channel_convert_to_unsigned : public detail::identity<ChannelValue> {
    using type = ChannelValue;
};

template <> struct channel_convert_to_unsigned<int8_t> {
    using argument_type = int8_t;
    using result_type = uint8_t;
    using type = uint8_t;
    type operator()(int8_t val) const {
        return static_cast<uint8_t>(static_cast<uint32_t>(val) + 128u);
    }
};

template <> struct channel_convert_to_unsigned<int16_t> {
    using argument_type = int16_t;
    using result_type = uint16_t;
    using type = uint16_t;
    type operator()(int16_t val) const {
        return static_cast<uint16_t>(static_cast<uint32_t>(val) + 32768u);
    }
};

template <> struct channel_convert_to_unsigned<int32_t> {
    using argument_type = int32_t;
    using result_type = uint32_t;
    using type = uint32_t;
    type operator()(int32_t val) const {
        return static_cast<uint32_t>(val)+(1u<<31);
    }
};


// Converting from unsigned to signed integral channel
// It is both a unary function, and a metafunction (thus requires the 'type' nested alias, which equals result_type)
template <typename ChannelValue>     // Model ChannelValueConcept
struct channel_convert_from_unsigned : public detail::identity<ChannelValue> {
    using type = ChannelValue;
};

template <> struct channel_convert_from_unsigned<int8_t> {
    using argument_type = uint8_t;
    using result_type = int8_t;
    using type = int8_t;
    type  operator()(uint8_t val) const {
        return static_cast<int8_t>(static_cast<int32_t>(val) - 128);
    }
};

template <> struct channel_convert_from_unsigned<int16_t> {
    using argument_type = uint16_t;
    using result_type = int16_t;
    using type = int16_t;
    type operator()(uint16_t val) const {
        return static_cast<int16_t>(static_cast<int32_t>(val) - 32768);
    }
};

template <> struct channel_convert_from_unsigned<int32_t> {
    using argument_type = uint32_t;
    using result_type = int32_t;
    using type = int32_t;
    type operator()(uint32_t val) const {
        return static_cast<int32_t>(val - (1u<<31));
    }
};

}   // namespace detail

/// \ingroup ChannelConvertAlgorithm
/// \brief A unary function object converting between channel types
template <typename SrcChannelV, typename DstChannelV> // Model ChannelValueConcept
struct channel_converter {
    using argument_type = SrcChannelV;
    using result_type = DstChannelV;
    DstChannelV operator()(const SrcChannelV& src) const {
        using to_unsigned = detail::channel_convert_to_unsigned<SrcChannelV>;
        using from_unsigned = detail::channel_convert_from_unsigned<DstChannelV>;
        using converter_unsigned = channel_converter_unsigned<typename to_unsigned::result_type, typename from_unsigned::argument_type>;
        return from_unsigned()(converter_unsigned()(to_unsigned()(src)));
    }
};

/// \ingroup ChannelConvertAlgorithm
/// \brief Converting from one channel type to another.
template <typename DstChannel, typename SrcChannel> // Model ChannelConcept (could be channel references)
inline typename channel_traits<DstChannel>::value_type channel_convert(const SrcChannel& src) {
    return channel_converter<typename channel_traits<SrcChannel>::value_type,
                             typename channel_traits<DstChannel>::value_type>()(src);
}

/// \ingroup ChannelConvertAlgorithm
/// \brief Same as channel_converter, except it takes the destination channel by reference, which allows
///        us to move the templates from the class level to the method level. This is important when invoking it
///        on heterogeneous pixels.
struct default_channel_converter {
    template <typename Ch1, typename Ch2>
    void operator()(const Ch1& src, Ch2& dst) const {
        dst=channel_convert<Ch2>(src);
    }
};

namespace detail {
    // fast integer division by 255
    inline uint32_t div255(uint32_t in) { uint32_t tmp=in+128; return (tmp + (tmp>>8))>>8; }

    // fast integer divison by 32768
    inline uint32_t div32768(uint32_t in) { return (in+16384)>>15; }
}

/// \defgroup ChannelMultiplyAlgorithm channel_multiply
/// \ingroup ChannelAlgorithm
/// \brief Multiplying unsigned channel values of the same type. Performs scaled multiplication result = a * b / max_value
///
/// Example:
/// \code
/// uint8_t x=128;
/// uint8_t y=128;
/// uint8_t mul = channel_multiply(x,y);
/// assert(mul == 64);    // 64 = 128 * 128 / 255
/// \endcode
/// @{

/// \brief This is the default implementation. Performance specializatons are provided
template <typename ChannelValue>
struct channel_multiplier_unsigned {
    using first_argument_type = ChannelValue;
    using second_argument_type = ChannelValue;
    using result_type = ChannelValue;
    ChannelValue operator()(ChannelValue a, ChannelValue b) const {
        return ChannelValue(static_cast<typename base_channel_type<ChannelValue>::type>(a / double(channel_traits<ChannelValue>::max_value()) * b));
    }
};

/// \brief Specialization of channel_multiply for 8-bit unsigned channels
template<> struct channel_multiplier_unsigned<uint8_t> {
    using first_argument_type = uint8_t;
    using second_argument_type = uint8_t;
    using result_type = uint8_t;
    uint8_t operator()(uint8_t a, uint8_t b) const { return uint8_t(detail::div255(uint32_t(a) * uint32_t(b))); }
};

/// \brief Specialization of channel_multiply for 16-bit unsigned channels
template<> struct channel_multiplier_unsigned<uint16_t> {
    using first_argument_type = uint16_t;
    using second_argument_type = uint16_t;
    using result_type = uint16_t;
    uint16_t operator()(uint16_t a, uint16_t b) const { return uint16_t((uint32_t(a) * uint32_t(b))/65535); }
};

/// \brief Specialization of channel_multiply for float 0..1 channels
template<> struct channel_multiplier_unsigned<float32_t> {
    using first_argument_type = float32_t;
    using second_argument_type = float32_t;
    using result_type = float32_t;
    float32_t operator()(float32_t a, float32_t b) const { return a*b; }
};

/// \brief A function object to multiply two channels. result = a * b / max_value
template <typename ChannelValue>
struct channel_multiplier {
    using first_argument_type = ChannelValue;
    using second_argument_type = ChannelValue;
    using result_type = ChannelValue;
    ChannelValue operator()(ChannelValue a, ChannelValue b) const {
        using to_unsigned = detail::channel_convert_to_unsigned<ChannelValue>;
        using from_unsigned = detail::channel_convert_from_unsigned<ChannelValue>;
        using multiplier_unsigned = channel_multiplier_unsigned<typename to_unsigned::result_type>;
        return from_unsigned()(multiplier_unsigned()(to_unsigned()(a), to_unsigned()(b)));
    }
};

/// \brief A function multiplying two channels. result = a * b / max_value
template <typename Channel> // Models ChannelConcept (could be a channel reference)
inline typename channel_traits<Channel>::value_type channel_multiply(Channel a, Channel b) {
    return channel_multiplier<typename channel_traits<Channel>::value_type>()(a,b);
}
/// @}

/// \defgroup ChannelInvertAlgorithm channel_invert
/// \ingroup ChannelAlgorithm
/// \brief Returns the inverse of a channel. result = max_value - x + min_value
///
/// Example:
/// \code
/// // uint8_t == uint8_t == unsigned char
/// uint8_t x=255;
/// uint8_t inv = channel_invert(x);
/// assert(inv == 0);
/// \endcode

/// \brief Default implementation. Provide overloads for performance
/// \ingroup ChannelInvertAlgorithm channel_invert
template <typename Channel> // Models ChannelConcept (could be a channel reference)
inline typename channel_traits<Channel>::value_type channel_invert(Channel x) {

    using base_t = typename base_channel_type<Channel>::type;
    using promoted_t = typename promote_integral<base_t>::type;
    promoted_t const promoted_x = x;
    promoted_t const promoted_max = channel_traits<Channel>::max_value();
    promoted_t const promoted_min = channel_traits<Channel>::min_value();
    promoted_t const promoted_inverted_x = promoted_max - promoted_x + promoted_min;
    auto const inverted_x = static_cast<base_t>(promoted_inverted_x);
    return inverted_x;
}

} }  // namespace boost::gil

#endif
