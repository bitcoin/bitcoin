//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_IO_MAKE_WRITER_HPP
#define BOOST_GIL_IO_MAKE_WRITER_HPP

#include <boost/gil/detail/mp11.hpp>
#include <boost/gil/io/get_writer.hpp>

#include <type_traits>

namespace boost { namespace gil {

template <typename String, typename FormatTag>
inline
auto make_writer(String const& file_name, image_write_info<FormatTag> const& info,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_supported_path_spec<String>,
            is_format_tag<FormatTag>
        >::value>::type* /*dummy*/ = nullptr)
    -> typename get_writer<String, FormatTag>::type
{
    typename get_write_device<String, FormatTag>::type device(
    detail::convert_to_native_string(file_name),
    typename detail::file_stream_device<FormatTag>::write_tag());

    return typename get_writer<String, FormatTag>::type(device, info);
}

template< typename FormatTag >
inline
typename get_writer< std::wstring
                   , FormatTag
                   >::type
make_writer( const std::wstring&                  file_name
           , const image_write_info< FormatTag >& info
           )
{
    const char* str = detail::convert_to_native_string( file_name );

    typename get_write_device< std::wstring
                    , FormatTag
                    >::type device( str
                                  , typename detail::file_stream_device< FormatTag >::write_tag()
                                  );

    delete[] str;

    return typename get_writer< std::wstring
                              , FormatTag
                              >::type( device
                                     , info
                                     );
}

#ifdef BOOST_GIL_IO_ADD_FS_PATH_SUPPORT
template< typename FormatTag >
inline
typename get_writer< std::wstring
                   , FormatTag
                   >::type
make_writer( const filesystem::path&              path
           , const image_write_info< FormatTag >& info
           )
{
    return make_writer( path.wstring()
                      , info
                      );
}
#endif // BOOST_GIL_IO_ADD_FS_PATH_SUPPORT

template <typename Device, typename FormatTag>
inline
auto make_writer(Device& file, image_write_info<FormatTag> const& info,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_adaptable_output_device<FormatTag, Device>::type,
            is_format_tag<FormatTag>
        >::value
    >::type* /*dummy*/ = nullptr)
    -> typename get_writer<Device, FormatTag>::type
{
    typename get_write_device<Device, FormatTag>::type device(file);
    return typename get_writer<Device, FormatTag>::type(device, info);
}

// no image_write_info

template <typename String, typename FormatTag>
inline
auto make_writer(String const& file_name, FormatTag const&,
    typename std::enable_if
    <
        mp11::mp_and
        <
            detail::is_supported_path_spec<String>,
            is_format_tag<FormatTag>
        >::value
    >::type* /*dummy*/ = nullptr)
    -> typename get_writer<String, FormatTag>::type
{
    return make_writer(file_name, image_write_info<FormatTag>());
}

template< typename FormatTag >
inline
typename get_writer< std::wstring
                   , FormatTag
                   >::type
make_writer( const std::wstring& file_name
           , const FormatTag&
           )
{
    return make_writer( file_name
                      , image_write_info< FormatTag >()
                      );
}

#ifdef BOOST_GIL_IO_ADD_FS_PATH_SUPPORT
template< typename FormatTag >
inline
typename get_writer< std::wstring
                   , FormatTag
                   >::type
make_writer( const filesystem::path& path
           , const FormatTag&        tag
           )
{
    return make_writer( path.wstring()
                      , tag
                      );
}
#endif // BOOST_GIL_IO_ADD_FS_PATH_SUPPORT

template <typename Device, typename FormatTag>
inline
auto make_writer(Device& file, FormatTag const&,
    typename std::enable_if
    <
        mp11::mp_and
        <
            typename detail::is_adaptable_output_device<FormatTag, Device>::type,
            is_format_tag<FormatTag>
        >::value
    >::type* /*dummy*/ = nullptr)
    -> typename get_writer<Device, FormatTag>::type
{
    return make_writer(file, image_write_info<FormatTag>());
}

}} // namespace boost::gil

#endif
