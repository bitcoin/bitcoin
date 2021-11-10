//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_CONCEPTS_CHANNEL_HPP
#define BOOST_GIL_CONCEPTS_CHANNEL_HPP

#include <boost/gil/concepts/basic.hpp>
#include <boost/gil/concepts/concept_check.hpp>
#include <boost/gil/concepts/fwd.hpp>

#include <boost/concept_check.hpp>

#include <utility> // std::swap
#include <type_traits>

#if defined(BOOST_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedefs"
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

namespace boost { namespace gil {

// Forward declarations
template <typename T>
struct channel_traits;

template <typename DstT, typename SrcT>
auto channel_convert(SrcT const& val)
    -> typename channel_traits<DstT>::value_type;

/// \ingroup ChannelConcept
/// \brief A channel is the building block of a color.
/// Color is defined as a mixture of primary colors and a channel defines
/// the degree to which each primary color is used in the mixture.
///
/// For example, in the RGB color space, using 8-bit unsigned channels,
/// the color red is defined as [255 0 0], which means maximum of Red,
/// and no Green and Blue.
///
/// Built-in scalar types, such as \p int and \p float, are valid GIL channels.
/// In more complex scenarios, channels may be represented as bit ranges or
/// even individual bits.
/// In such cases special classes are needed to represent the value and
/// reference to a channel.
///
/// Channels have a traits class, \p channel_traits, which defines their
/// associated types as well as their operating ranges.
///
/// \code
/// concept ChannelConcept<typename T> : EqualityComparable<T>
/// {
///     typename value_type      = T;        // use channel_traits<T>::value_type to access it
///     typename reference       = T&;       // use channel_traits<T>::reference to access it
///     typename pointer         = T*;       // use channel_traits<T>::pointer to access it
///     typename const_reference = const T&; // use channel_traits<T>::const_reference to access it
///     typename const_pointer   = const T*; // use channel_traits<T>::const_pointer to access it
///     static const bool is_mutable;        // use channel_traits<T>::is_mutable to access it
///
///     static T min_value();                // use channel_traits<T>::min_value to access it
///     static T max_value();                // use channel_traits<T>::max_value to access it
/// };
/// \endcode
template <typename T>
struct ChannelConcept
{
    void constraints()
    {
        gil_function_requires<boost::EqualityComparableConcept<T>>();

        using v = typename channel_traits<T>::value_type;
        using r = typename channel_traits<T>::reference;
        using p = typename channel_traits<T>::pointer;
        using cr = typename channel_traits<T>::const_reference;
        using cp = typename channel_traits<T>::const_pointer;

        channel_traits<T>::min_value();
        channel_traits<T>::max_value();
    }

     T c;
};

namespace detail
{

/// \tparam T models ChannelConcept
template <typename T>
struct ChannelIsMutableConcept
{
    void constraints()
    {
        c1 = c2;
        using std::swap;
        swap(c1, c2);
    }
    T c1;
    T c2;
};

} // namespace detail

/// \brief A channel that allows for modifying its value
/// \code
/// concept MutableChannelConcept<ChannelConcept T> : Assignable<T>, Swappable<T> {};
/// \endcode
/// \ingroup ChannelConcept
template <typename T>
struct MutableChannelConcept
{
    void constraints()
    {
        gil_function_requires<ChannelConcept<T>>();
        gil_function_requires<detail::ChannelIsMutableConcept<T>>();
    }
};

/// \brief A channel that supports default construction.
/// \code
/// concept ChannelValueConcept<ChannelConcept T> : Regular<T> {};
/// \endcode
/// \ingroup ChannelConcept
template <typename T>
struct ChannelValueConcept
{
    void constraints()
    {
        gil_function_requires<ChannelConcept<T>>();
        gil_function_requires<Regular<T>>();
    }
};

/// \brief Predicate metafunction returning whether two channels are compatible
///
/// Channels are considered compatible if their value types
/// (ignoring constness and references) are the same.
///
/// Example:
///
/// \code
/// static_assert(channels_are_compatible<uint8_t, const uint8_t&>::value, "");
/// \endcode
/// \ingroup ChannelAlgorithm
template <typename T1, typename T2>  // Models GIL Pixel
struct channels_are_compatible
    : std::is_same
        <
            typename channel_traits<T1>::value_type,
            typename channel_traits<T2>::value_type
        >
{
};

/// \brief Channels are compatible if their associated value types (ignoring constness and references) are the same
///
/// \code
/// concept ChannelsCompatibleConcept<ChannelConcept T1, ChannelConcept T2>
/// {
///     where SameType<T1::value_type, T2::value_type>;
/// };
/// \endcode
/// \ingroup ChannelConcept
template <typename Channel1, typename Channel2>
struct ChannelsCompatibleConcept
{
    void constraints()
    {
        static_assert(channels_are_compatible<Channel1, Channel2>::value, "");
    }
};

/// \brief A channel is convertible to another one if the \p channel_convert algorithm is defined for the two channels.
///
/// Convertibility is non-symmetric and implies that one channel can be
/// converted to another. Conversion is explicit and often lossy operation.
///
/// concept ChannelConvertibleConcept<ChannelConcept SrcChannel, ChannelValueConcept DstChannel>
/// {
///     DstChannel channel_convert(const SrcChannel&);
/// };
/// \endcode
/// \ingroup ChannelConcept
template <typename SrcChannel, typename DstChannel>
struct ChannelConvertibleConcept
{
    void constraints()
    {
        gil_function_requires<ChannelConcept<SrcChannel>>();
        gil_function_requires<MutableChannelConcept<DstChannel>>();
        dst = channel_convert<DstChannel, SrcChannel>(src);
        ignore_unused_variable_warning(dst);
    }
    SrcChannel src;
    DstChannel dst;
};

}} // namespace boost::gil

#if defined(BOOST_CLANG)
#pragma clang diagnostic pop
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic pop
#endif

#endif
