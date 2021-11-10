//
// Copyright 2007-2012 Christian Henning, Andreas Pokorny, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_IO_WRITE_VIEW_HPP
#define BOOST_GIL_IO_WRITE_VIEW_HPP

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/get_writer.hpp>
#include <boost/gil/io/path_spec.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <type_traits>

namespace boost{ namespace gil {

/// \ingroup IO
template<typename Writer, typename View>
inline
void write_view(Writer& writer, View const& view,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_writer<Writer>::type,
            typename is_format_tag<typename Writer::format_tag_t>::type,
            typename is_write_supported
            <
                typename get_pixel_type<View>::type,
                typename Writer::format_tag_t
            >::type
        >::value
    >::type* /* ptr */ = nullptr)
{
    writer.apply(view);
}

/// \ingroup IO
template<typename Device, typename View, typename FormatTag>
inline
void write_view(Device& device, View const& view, FormatTag const& tag,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_write_device<FormatTag, Device>::type,
            typename is_format_tag<FormatTag>::type,
            typename is_write_supported
            <
                typename get_pixel_type<View>::type,
                FormatTag
            >::type
        >::value
    >::type* /* ptr */ = nullptr)
{
    using writer_t = typename get_writer<Device, FormatTag>::type;
    writer_t writer = make_writer(device, tag);
    write_view(writer, view);
}

/// \ingroup IO
template<typename String, typename View, typename FormatTag>
inline
void write_view(String const& file_name, View const& view, FormatTag const& tag,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_supported_path_spec<String>::type,
            typename is_format_tag<FormatTag>::type,
            typename is_write_supported
            <
                typename get_pixel_type<View>::type,
                FormatTag
            >::type
        >::value
    >::type* /* ptr */ = nullptr)
{
    using writer_t = typename get_writer<String, FormatTag>::type;
    writer_t writer = make_writer(file_name, tag);
    write_view(writer, view);
}

/// \ingroup IO
template<typename Device, typename View, typename FormatTag, typename Log>
inline
void write_view(
    Device& device, View const& view, image_write_info<FormatTag, Log> const& info,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_write_device<FormatTag, Device>::type,
            typename is_format_tag<FormatTag>::type,
            typename is_write_supported
            <
                typename get_pixel_type<View>::type,
                FormatTag
            >::type
        >::value
    >::type* /* ptr */ = nullptr)
{
    using writer_t = typename get_writer<Device, FormatTag>::type;
    writer_t writer = make_writer(device, info);
    write_view(writer, view);
}

/// \ingroup IO
template<typename String, typename View, typename FormatTag, typename Log>
inline
void write_view(
    String const& file_name, View const& view, image_write_info<FormatTag, Log> const& info,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_supported_path_spec<String>::type,
            typename is_format_tag<FormatTag>::type,
            typename is_write_supported
            <
                typename get_pixel_type<View>::type,
                FormatTag
            >::type
        >::value
    >::type* /* ptr */ = nullptr)
{
    using writer_t = typename get_writer<String, FormatTag>::type;
    writer_t writer = make_writer(file_name, info);
    write_view(writer, view);
}

////////////////////////////////////// dynamic_image

// without image_write_info
template <typename Writer, typename ...Views>
inline
void write_view(Writer& writer, any_image_view<Views...> const& view,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_dynamic_image_writer<Writer>::type,
            typename is_format_tag<typename Writer::format_tag_t>::type
        >::value
    >::type * /* ptr */ = nullptr)
{
    writer.apply(view);
}

// without image_write_info
template <typename Device, typename ...Views, typename FormatTag>
inline
void write_view(
    Device& device, any_image_view<Views...> const& views, FormatTag const& tag,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_write_device<FormatTag, Device>::type,
            typename is_format_tag<FormatTag>::type
        >::value
    >::type * /* ptr */ = 0)
{
    using writer_t = typename get_dynamic_image_writer<Device, FormatTag>::type;
    writer_t writer = make_dynamic_image_writer(device, tag);
    write_view(writer, views);
}

template <typename String, typename ...Views, typename FormatTag>
inline
void write_view(
    String const& file_name, any_image_view<Views...> const& views, FormatTag const& tag,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_supported_path_spec<String>::type,
            typename is_format_tag<FormatTag>::type
        >::value
    >::type * /* ptr */ = nullptr)
{
    using writer_t = typename get_dynamic_image_writer<String, FormatTag>::type;
    writer_t writer = make_dynamic_image_writer(file_name, tag);
    write_view(writer, views);
}

// with image_write_info
/// \ingroup IO
template <typename Device, typename ...Views, typename FormatTag, typename Log>
inline
void write_view(
    Device& device, any_image_view<Views...> const& views, image_write_info<FormatTag, Log> const& info,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_write_device<FormatTag, Device>::type,
            typename is_format_tag<FormatTag>::type
        >::value
    >::type * /* ptr */ = 0)
{
    using writer_t = typename get_dynamic_image_writer<Device, FormatTag>::type;
    writer_t writer = make_dynamic_image_writer(device, info);
    write_view(writer, views);
}

template <typename String, typename ...Views, typename FormatTag, typename Log>
inline
void write_view(
    String const& file_name, any_image_view<Views...> const& views, image_write_info<FormatTag, Log> const& info,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_supported_path_spec<String>::type,
            typename is_format_tag<FormatTag>::type
        >::value
    >::type * /* ptr */ = nullptr)
{
    using writer_t = typename get_dynamic_image_writer<String, FormatTag>::type;
    writer_t writer = make_dynamic_image_writer(file_name, info);
    write_view(writer, views);
}

}} // namespace boost::gil

#endif
