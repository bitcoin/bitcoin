//
// Copyright 2007-2012 Christian Henning, Andreas Pokorny, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_IO_READ_VIEW_HPP
#define BOOST_GIL_IO_READ_VIEW_HPP

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/get_reader.hpp>
#include <boost/gil/io/path_spec.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <type_traits>

namespace boost { namespace gil {

/// \ingroup IO

/// \brief Reads an image view without conversion. No memory is allocated.
/// \param reader    An image reader.
/// \param view      The image view in which the data is read into.
/// \param settings  Specifies read settings depending on the image format.
/// \throw std::ios_base::failure
template <typename Reader, typename View>
inline
void read_view(Reader reader, View const& view,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_reader<Reader>,
            typename is_format_tag<typename Reader::format_tag_t>::type,
            typename is_read_supported
            <
                typename get_pixel_type<View>::type,
                typename Reader::format_tag_t
            >::type
        >::value
    >::type* /*dummy*/ = nullptr)
{
    reader.check_image_size(view.dimensions());
    reader.init_view(view, reader._settings);
    reader.apply(view);
}

/// \brief Reads an image view without conversion. No memory is allocated.
/// \param file      It's a device. Must satisfy is_input_device metafunction.
/// \param view      The image view in which the data is read into.
/// \param settings  Specifies read settings depending on the image format.
/// \throw std::ios_base::failure
template <typename Device, typename View, typename FormatTag>
inline
void read_view(
    Device& file,
    View const& view,
    image_read_settings<FormatTag> const& settings,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_read_device<FormatTag, Device>,
            typename is_format_tag<FormatTag>::type,
            typename is_read_supported
            <
                typename get_pixel_type<View>::type,
                FormatTag
            >::type
        >::value
    >::type* /*dummy*/ = nullptr)
{
    using reader_t =
        typename get_reader<Device, FormatTag, detail::read_and_no_convert>::type;

    reader_t reader = make_reader(file, settings, detail::read_and_no_convert());
    read_view(reader, view);
}

/// \brief Reads an image view without conversion. No memory is allocated.
/// \param file It's a device. Must satisfy is_input_device metafunction or is_adaptable_input_device.
/// \param view The image view in which the data is read into.
/// \param tag  Defines the image format. Must satisfy is_format_tag metafunction.
/// \throw std::ios_base::failure
template <typename Device, typename View, typename FormatTag>
inline
void read_view(Device& file, View const& view, FormatTag const& tag,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename is_format_tag<FormatTag>::type,
            detail::is_read_device<FormatTag, Device>,
            typename is_read_supported
            <
                typename get_pixel_type<View>::type,
                FormatTag
            >::type
        >::value>::type* /*dummy*/ = nullptr)
{
    using reader_t =
        typename get_reader<Device, FormatTag, detail::read_and_no_convert>::type;

    reader_t reader = make_reader(file, tag, detail::read_and_no_convert());
    read_view(reader, view);
}

/// \brief Reads an image view without conversion. No memory is allocated.
/// \param file_name File name. Must satisfy is_supported_path_spec metafunction.
/// \param view      The image view in which the data is read into.
/// \param settings  Specifies read settings depending on the image format.
/// \throw std::ios_base::failure
template <typename String, typename View, typename FormatTag>
inline
void read_view(
    String const& file_name,
    View const& view,
    image_read_settings<FormatTag> const& settings,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_supported_path_spec<String>::type,
            typename is_format_tag<FormatTag>::type,
            typename is_read_supported
            <
                typename get_pixel_type<View>::type,
                FormatTag
            >::type
        >::value
    >::type* /*dummy*/ = nullptr)
{
    using reader_t =
        typename get_reader<String, FormatTag, detail::read_and_no_convert>::type;

    reader_t reader = make_reader(file_name, settings, detail::read_and_no_convert());
    read_view(reader, view);
}

/// \brief Reads an image view without conversion. No memory is allocated.
/// \param file_name File name. Must satisfy is_supported_path_spec metafunction.
/// \param view      The image view in which the data is read into.
/// \param tag       Defines the image format. Must satisfy is_format_tag metafunction.
/// \throw std::ios_base::failure
template <typename String, typename View, typename FormatTag>
inline
void read_view(String const& file_name, View const& view, FormatTag const& tag,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_supported_path_spec<String>::type,
            typename is_format_tag<FormatTag>::type,
            typename is_read_supported
            <
                typename get_pixel_type<View>::type,
                FormatTag
            >::type
        >::value
    >::type* /*dummy*/ = nullptr)
{
    using reader_t =
        typename get_reader<String, FormatTag, detail::read_and_no_convert>::type;

    reader_t reader = make_reader(file_name, tag, detail::read_and_no_convert());
    read_view(reader, view);
}

}}  // namespace boost::gil

#endif
