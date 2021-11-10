//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_TIFF_DETAIL_WRITER_BACKEND_HPP
#define BOOST_GIL_EXTENSION_IO_TIFF_DETAIL_WRITER_BACKEND_HPP

#include <boost/gil/extension/io/tiff/tags.hpp>
#include <boost/gil/extension/io/tiff/detail/device.hpp>

#include <boost/gil/detail/mp11.hpp>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

///
/// TIFF Writer Backend
///
template< typename Device >
struct writer_backend< Device
                     , tiff_tag
                     >
{
public:

    using format_tag_t = tiff_tag;

public:

    writer_backend( const Device&                       io_dev
                  , const image_write_info< tiff_tag >& info
                  )
    : _io_dev( io_dev )
    , _info( info )
    {}

protected:

    template< typename View >
    void write_header( const View& view )
    {
        using pixel_t = typename View::value_type;

        // get the type of the first channel (heterogeneous pixels might be broken for now!)
        using channel_t = typename channel_traits<typename element_type<pixel_t>::type>::value_type;
				using color_space_t = typename color_space_type<View>::type;

        if(! this->_info._photometric_interpretation_user_defined )
        {
            // write photometric interpretion - Warning: This value is rather
            // subjective. The user should better set this value itself. There
            // is no way to decide if a image is PHOTOMETRIC_MINISWHITE or
            // PHOTOMETRIC_MINISBLACK. If the user has not manually set it, then
            // this writer will assume PHOTOMETRIC_MINISBLACK for gray_t images,
            // PHOTOMETRIC_RGB for rgb_t images, and PHOTOMETRIC_SEPARATED (as
            // is conventional) for cmyk_t images.
            this->_info._photometric_interpretation = detail::photometric_interpretation< color_space_t >::value;
        }

        // write dimensions
        tiff_image_width::type  width  = (tiff_image_width::type)  view.width();
        tiff_image_height::type height = (tiff_image_height::type) view.height();

        this->_io_dev.template set_property< tiff_image_width  >( width  );
        this->_io_dev.template set_property< tiff_image_height >( height );

        // write planar configuration
        this->_io_dev.template set_property<tiff_planar_configuration>( this->_info._planar_configuration );

        // write samples per pixel
        tiff_samples_per_pixel::type samples_per_pixel = num_channels< pixel_t >::value;
        this->_io_dev.template set_property<tiff_samples_per_pixel>( samples_per_pixel );

        if /*constexpr*/ (mp11::mp_contains<color_space_t, alpha_t>::value)
        {
          std:: vector <uint16_t> extra_samples {EXTRASAMPLE_ASSOCALPHA};
          this->_io_dev.template set_property<tiff_extra_samples>( extra_samples );
        }

        // write bits per sample
        // @todo: Settings this value usually requires to write for each sample the bit
        // value seperately in case they are different, like rgb556.
        tiff_bits_per_sample::type bits_per_sample = detail::unsigned_integral_num_bits< channel_t >::value;
        this->_io_dev.template set_property<tiff_bits_per_sample>( bits_per_sample );

        // write sample format
        tiff_sample_format::type sampl_format = detail::sample_format< channel_t >::value;
        this->_io_dev.template set_property<tiff_sample_format>( sampl_format );

        // write photometric format
        this->_io_dev.template set_property<tiff_photometric_interpretation>( this->_info._photometric_interpretation );

        // write compression
        this->_io_dev.template set_property<tiff_compression>( this->_info._compression );

        // write orientation
        this->_io_dev.template set_property<tiff_orientation>( this->_info._orientation );

        // write rows per strip
        this->_io_dev.template set_property<tiff_rows_per_strip>( this->_io_dev.get_default_strip_size() );

        // write x, y resolution and units
        this->_io_dev.template set_property<tiff_resolution_unit>( this->_info._resolution_unit );
        this->_io_dev.template set_property<tiff_x_resolution>( this->_info._x_resolution );
        this->_io_dev.template set_property<tiff_y_resolution>( this->_info._y_resolution );

        /// Optional and / or non-baseline tags below here

        // write ICC colour profile, if it's there
        // http://www.color.org/icc_specs2.xalter
        if ( 0 != this->_info._icc_profile.size())
          this->_io_dev.template set_property<tiff_icc_profile>( this->_info._icc_profile );
    }


public:

    Device _io_dev;

    image_write_info< tiff_tag > _info;
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // namespace gil
} // namespace boost

#endif
