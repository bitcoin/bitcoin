//
// Copyright 2008 Christian Henning, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_TIFF_DETAIL_IS_ALLOWED_HPP
#define BOOST_GIL_EXTENSION_IO_TIFF_DETAIL_IS_ALLOWED_HPP

#include <boost/gil/extension/io/tiff/tags.hpp>
#include <boost/gil/extension/toolbox/metafunctions/is_bit_aligned.hpp>
#include <boost/gil/detail/mp11.hpp>
#include <boost/gil/io/base.hpp>

#include <type_traits>

namespace boost { namespace gil { namespace detail {

using channel_sizes_t = std::vector<tiff_bits_per_sample::type>;

template <typename View, typename Channel, typename Enable = void>
struct Format_Type {};

// is_bit_aligned< View >
template <typename View, typename Channel>
struct Format_Type
<
    View,
    Channel,
    typename std::enable_if
    <
        is_bit_aligned
        <
            typename get_pixel_type<View>::type
        >::value
    >::type
>
{
    static const int value = SAMPLEFORMAT_UINT;
};

// is_not_bit_aligned< View > && is_unsigned< Channel >
template <typename View, typename Channel>
struct Format_Type
<
    View,
    Channel,
    typename std::enable_if
    <
        mp11::mp_and
        <
            mp11::mp_not
            <
                typename is_bit_aligned<typename get_pixel_type<View>::type>::type
            >,
            std::is_unsigned<Channel>
        >::value
    >::type
>
{
    static const int value = SAMPLEFORMAT_UINT;
};

// is_not_bit_aligned< View > && is_signed< Channel >
template <typename View, typename Channel>
struct Format_Type
<
    View,
    Channel,
    typename std::enable_if
    <
        mp11::mp_and
        <
            mp11::mp_not
            <
                typename is_bit_aligned<typename get_pixel_type<View>::type>::type
            >,
            std::is_signed<Channel>
        >::value
    >::type
>
{
    static const int value = SAMPLEFORMAT_INT;
};

// is_not_bit_aligned< View > && is_floating_point< Channel >
template <typename View, typename Channel>
struct Format_Type
<
    View,
    Channel,
    typename std::enable_if
    <
        mp11::mp_and
        <
            mp11::mp_not
            <
                typename is_bit_aligned<typename get_pixel_type<View>::type>::type
            >,
            is_floating_point<Channel>
        >::value
    >::type
>
{
    static const int value = SAMPLEFORMAT_IEEEFP;
};

//template< typename Channel >
//int format_value( std::true_type ) // is_bit_aligned
//{
//    return SAMPLEFORMAT_UINT;
//}
//
//template< typename Channel >
//int format_value( std::false_type ) // is_bit_aligned
//{
//    if( is_unsigned< Channel >::value )
//    {
//        return SAMPLEFORMAT_UINT;
//    }
//
//    if( is_signed< Channel >::value )
//    {
//        return SAMPLEFORMAT_INT;
//    }
//
//    else if( is_floating_point< Channel >::value )
//    {
//        return SAMPLEFORMAT_IEEEFP;
//    }
//
//    io_error( "Unkown channel format." );
//}

// The following two functions look the same but are different since one is using
// a pixel_t as template parameter whereas the other is using reference_t.
template< typename View >
bool compare_channel_sizes( const channel_sizes_t& channel_sizes // in bits
                          , std::false_type                      // is_bit_aligned
                          , std::true_type                       // is_homogeneous
                          )
{
    using pixel_t = typename View::value_type;
    using channel_t = typename channel_traits<typename element_type<pixel_t>::type>::value_type;

    unsigned int s = detail::unsigned_integral_num_bits< channel_t >::value;

    return ( s == channel_sizes[0] );
}


template< typename View >
bool compare_channel_sizes( const channel_sizes_t& channel_sizes // in bits
                          , std::true_type                       // is_bit_aligned
                          , std::true_type                       // is_homogeneous
                          )
{
    using ref_t = typename View::reference;
    using channel_t = typename channel_traits<typename element_type<ref_t>::type>::value_type;

    unsigned int s = detail::unsigned_integral_num_bits< channel_t >::value;
    return ( s == channel_sizes[0] );
}

struct compare_channel_sizes_fn
{
    compare_channel_sizes_fn( uint16_t* a )
    : _a( a )
    , _b( true )
    {}

    template< typename ChannelSize >
    void operator()( ChannelSize x)
    {
        if( x != *_a++ )
        {
            _b = false;
        }
    }

    uint16_t* _a;
    bool _b;
};

template< typename T >
struct channel_sizes_type {};

template< typename B, typename C, typename L, bool M >
struct channel_sizes_type< bit_aligned_pixel_reference< B, C, L, M > > { using type = C; };

template< typename B, typename C, typename L, bool M >
struct channel_sizes_type< const bit_aligned_pixel_reference< B, C, L, M > > { using type = C; };

template< typename View >
bool compare_channel_sizes( channel_sizes_t& channel_sizes // in bits
                          , std::true_type                 // is_bit_aligned
                          , std::false_type                // is_homogeneous
                          )
{
    // loop through all channels and compare

    using ref_t = typename View::reference;
    using cs_t = typename channel_sizes_type<ref_t>::type;

    compare_channel_sizes_fn fn( &channel_sizes.front() );
    mp11::mp_for_each<cs_t>(fn);

    return fn._b;
}

template< typename View >
bool is_allowed( const image_read_info< tiff_tag >& info
               , std::true_type // is read_and_no_convert
               )
{
    channel_sizes_t channel_sizes( info._samples_per_pixel
                                 , info._bits_per_sample
                                 );

    using pixel_t = typename get_pixel_type<View>::type;
    using channel_t = typename channel_traits<typename element_type<pixel_t>::type>::value_type;

    using num_channel_t = typename num_channels<pixel_t>::value_type;

    const num_channel_t dst_samples_per_pixel = num_channels< pixel_t >::value;

    //const num_channel_t dst_sample_format     = format_value< channel_t >( typename is_bit_aligned< pixel_t >::type() );
    const num_channel_t dst_sample_format     = Format_Type<View, channel_t>::value;


    return (  dst_samples_per_pixel == info._samples_per_pixel
           && compare_channel_sizes< View >( channel_sizes
                                           , typename is_bit_aligned< pixel_t >::type()
                                           , typename is_homogeneous< pixel_t >::type()
                                           )
           && dst_sample_format == info._sample_format
           );
}

template< typename View >
bool is_allowed( const image_read_info< tiff_tag >& /* info */
               , std::false_type // is read_and_no_convert
               )
{
    return true;
}

} // namespace detail
} // namespace gil
} // namespace boost

#endif
