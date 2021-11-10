//
// Copyright 2007-2012 Christian Henning, Andreas Pokorny, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_IO_READ_IMAGE_HPP
#define BOOST_GIL_IO_READ_IMAGE_HPP

#include <boost/gil/extension/toolbox/dynamic_images.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/get_reader.hpp>
#include <boost/gil/io/path_spec.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <type_traits>

namespace boost { namespace gil {

/// \ingroup IO

/// \brief Reads an image without conversion. Image memory is allocated.
/// \param reader    An image reader.
/// \param img       The image in which the data is read into. Must satisfy is_read_supported metafunction.
/// \throw std::ios_base::failure
template <typename Reader, typename Image>
inline
void read_image(Reader reader, Image& img,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_reader<Reader>,
            is_format_tag<typename Reader::format_tag_t>,
            is_read_supported
            <
                typename get_pixel_type<typename Image::view_t>::type,
                typename Reader::format_tag_t
            >
        >::value
    >::type* /*dummy*/ = nullptr)
{
    reader.init_image(img, reader._settings);
    reader.apply(view(img));
}

/// \brief Reads an image without conversion. Image memory is allocated.
/// \param file      It's a device. Must satisfy is_input_device metafunction.
/// \param img       The image in which the data is read into. Must satisfy is_read_supported metafunction.
/// \param settings  Specifies read settings depending on the image format.
/// \throw std::ios_base::failure
template <typename Device, typename Image, typename FormatTag>
inline
void read_image(
    Device& file,
    Image& img,
    image_read_settings<FormatTag> const& settings,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_read_device<FormatTag, Device>,
            is_format_tag<FormatTag>,
            is_read_supported
            <
                typename get_pixel_type<typename Image::view_t>::type,
                FormatTag
            >
        >::value
    >::type* /*dummy*/ = nullptr)
{
    using reader_t =
        typename get_reader<Device, FormatTag, detail::read_and_no_convert>::type;

    reader_t reader = make_reader(file, settings, detail::read_and_no_convert());
    read_image(reader, img);
}

/// \brief Reads an image without conversion. Image memory is allocated.
/// \param file      It's a device. Must satisfy is_input_device metafunction.
/// \param img       The image in which the data is read into. Must satisfy is_read_supported metafunction.
/// \param tag       Defines the image format. Must satisfy is_format_tag metafunction.
/// \throw std::ios_base::failure
template <typename Device, typename Image, typename FormatTag>
inline
void read_image(Device& file, Image& img, FormatTag const& tag,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_read_device<FormatTag, Device>,
            is_format_tag<FormatTag>,
            is_read_supported
            <
                typename get_pixel_type<typename Image::view_t>::type,
                FormatTag
            >
        >::value
    >::type* /*dummy*/ = nullptr)
{
    using reader_t =
        typename get_reader<Device, FormatTag, detail::read_and_no_convert>::type;

    reader_t reader = make_reader(file, tag, detail::read_and_no_convert());
    read_image(reader, img);
}

/// \brief Reads an image without conversion. Image memory is allocated.
/// \param file_name File name. Must satisfy is_supported_path_spec metafunction.
/// \param img       The image in which the data is read into. Must satisfy is_read_supported metafunction.
/// \param settings  Specifies read settings depending on the image format.
/// \throw std::ios_base::failure
template <typename String, typename Image, typename FormatTag>
inline
void read_image(
    String const& file_name,
    Image& img,
    image_read_settings<FormatTag> const& settings,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_supported_path_spec<String>,
            is_format_tag<FormatTag>,
            is_read_supported
            <
                typename get_pixel_type<typename Image::view_t>::type,
                FormatTag
            >
        >::value
    >::type* /*dummy*/ = nullptr)
{
    using reader_t =
        typename get_reader<String, FormatTag, detail::read_and_no_convert>::type;

    reader_t reader = make_reader(file_name, settings, detail::read_and_no_convert());
    read_image(reader, img);
}

/// \brief Reads an image without conversion. Image memory is allocated.
/// \param file_name File name. Must satisfy is_supported_path_spec metafunction.
/// \param img       The image in which the data is read into. Must satisfy is_read_supported metafunction.
/// \param tag       Defines the image format. Must satisfy is_format_tag metafunction.
/// \throw std::ios_base::failure
template <typename String, typename Image, typename FormatTag>
inline
void read_image(String const& file_name, Image& img, FormatTag const& tag,
    typename std::enable_if
    <
        mp11::mp_and<detail::is_supported_path_spec<String>,
        is_format_tag<FormatTag>,
        is_read_supported
        <
            typename get_pixel_type<typename Image::view_t>::type,
            FormatTag
        >
    >::value
>::type* /*dummy*/ = nullptr)
{
    using reader_t =
        typename get_reader<String, FormatTag, detail::read_and_no_convert>::type;

    reader_t reader = make_reader(file_name, tag, detail::read_and_no_convert());
    read_image(reader, img);
}

///

template <typename Reader, typename ...Images>
inline
void read_image(Reader& reader, any_image<Images...>& images,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_dynamic_image_reader<Reader>,
            is_format_tag<typename Reader::format_tag_t>
        >::value
    >::type* /*dummy*/ = nullptr)
{
    reader.apply(images);
}

/// \brief Reads an image without conversion. Image memory is allocated.
/// \param file      It's a device. Must satisfy is_adaptable_input_device metafunction.
/// \param images    Dynamic image (mp11::mp_list). See boost::gil::dynamic_image extension.
/// \param settings  Specifies read settings depending on the image format.
/// \throw std::ios_base::failure
template <typename Device, typename ...Images, typename FormatTag>
inline
void read_image(
    Device& file,
    any_image<Images...>& images,
    image_read_settings<FormatTag> const& settings,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_read_device<FormatTag, Device>,
            is_format_tag<FormatTag>
        >::value
    >::type* /*dummy*/ = nullptr)
{
    using reader_t = typename get_dynamic_image_reader<Device, FormatTag>::type;

    reader_t reader = make_dynamic_image_reader(file, settings);
    read_image(reader, images);
}

/// \brief Reads an image without conversion. Image memory is allocated.
/// \param file      It's a device. Must satisfy is_adaptable_input_device metafunction.
/// \param images    Dynamic image (mp11::mp_list). See boost::gil::dynamic_image extension.
/// \param tag       Defines the image format. Must satisfy is_format_tag metafunction.
/// \throw std::ios_base::failure
template <typename Device, typename ...Images, typename FormatTag>
inline
void read_image(Device& file, any_image<Images...>& images, FormatTag const& tag,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_read_device<FormatTag, Device>,
            is_format_tag<FormatTag>
        >::value
        >::type* /*dummy*/ = nullptr)
{
    using reader_t = typename get_dynamic_image_reader<Device, FormatTag>::type;

    reader_t reader = make_dynamic_image_reader(file, tag);
    read_image(reader, images);
}

/// \brief Reads an image without conversion. Image memory is allocated.
/// \param file_name File name. Must satisfy is_supported_path_spec metafunction.
/// \param images    Dynamic image (mp11::mp_list). See boost::gil::dynamic_image extension.
/// \param settings  Specifies read settings depending on the image format.
/// \throw std::ios_base::failure
template <typename String, typename ...Images, typename FormatTag>
inline
void read_image(
    String const& file_name,
    any_image<Images...>& images,
    image_read_settings<FormatTag> const& settings,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_supported_path_spec<String>,
            is_format_tag<FormatTag>
        >::value
    >::type* /*dummy*/ = nullptr)
{
    using reader_t = typename get_dynamic_image_reader<String, FormatTag>::type;

    reader_t reader = make_dynamic_image_reader(file_name, settings);
    read_image(reader, images);
}

/// \brief Reads an image without conversion. Image memory is allocated.
/// \param file_name File name. Must satisfy is_supported_path_spec metafunction.
/// \param images    Dynamic image (mp11::mp_list). See boost::gil::dynamic_image extension.
/// \param tag       Defines the image format. Must satisfy is_format_tag metafunction.
/// \throw std::ios_base::failure
template <typename String, typename ...Images, typename FormatTag>
inline
void read_image(String const& file_name, any_image<Images...>& images, FormatTag const& tag,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_supported_path_spec<String>,
            is_format_tag<FormatTag>
        >::value
    >::type* /*dummy*/ = nullptr)
{
    using reader_t = typename get_dynamic_image_reader<String, FormatTag>::type;

    reader_t reader = make_dynamic_image_reader(file_name, tag);
    read_image(reader, images);
}

}}  // namespace boost::gil

#endif
