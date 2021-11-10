//
// Copyright 2012 Olivier Tournaire
// Copyright 2007 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_RAW_DETAIL_DEVICE_HPP
#define BOOST_GIL_EXTENSION_IO_RAW_DETAIL_DEVICE_HPP

#include <boost/gil/extension/io/raw/tags.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/device.hpp>

#include <memory>
#include <string>
#include <type_traits>

namespace boost { namespace gil { namespace detail {

class raw_device_base
{
public:

    ///
    /// Constructor
    ///
    raw_device_base()
    : _processor_ptr( new LibRaw )
    {}

    // iparams getters
    std::string get_camera_manufacturer() { return std::string(_processor_ptr.get()->imgdata.idata.make);  }
    std::string get_camera_model()        { return std::string(_processor_ptr.get()->imgdata.idata.model); }
    unsigned get_raw_count()              { return _processor_ptr.get()->imgdata.idata.raw_count; }
    unsigned get_dng_version()            { return _processor_ptr.get()->imgdata.idata.dng_version; }
    int get_colors()                      { return _processor_ptr.get()->imgdata.idata.colors; }
    unsigned get_filters()                { return _processor_ptr.get()->imgdata.idata.filters; }
    std::string get_cdesc()               { return std::string(_processor_ptr.get()->imgdata.idata.cdesc); }

    // image_sizes getters
    unsigned short get_raw_width()    { return _processor_ptr.get()->imgdata.sizes.raw_width;  }
    unsigned short get_raw_height()   { return _processor_ptr.get()->imgdata.sizes.raw_height; }
    unsigned short get_image_width()  { return _processor_ptr.get()->imgdata.sizes.width;  }
    unsigned short get_image_height() { return _processor_ptr.get()->imgdata.sizes.height; }
    unsigned short get_top_margin()   { return _processor_ptr.get()->imgdata.sizes.top_margin;  }
    unsigned short get_left_margin()  { return _processor_ptr.get()->imgdata.sizes.left_margin; }
    unsigned short get_iwidth()       { return _processor_ptr.get()->imgdata.sizes.iwidth;  }
    unsigned short get_iheight()      { return _processor_ptr.get()->imgdata.sizes.iheight; }
    double get_pixel_aspect()         { return _processor_ptr.get()->imgdata.sizes.pixel_aspect;  }
    int get_flip()                    { return _processor_ptr.get()->imgdata.sizes.flip; }

    // colordata getters
    // TODO

    // imgother getters
    float get_iso_speed()     { return _processor_ptr.get()->imgdata.other.iso_speed; }
    float get_shutter()       { return _processor_ptr.get()->imgdata.other.shutter; }
    float get_aperture()      { return _processor_ptr.get()->imgdata.other.aperture; }
    float get_focal_len()     { return _processor_ptr.get()->imgdata.other.focal_len; }
    time_t get_timestamp()    { return _processor_ptr.get()->imgdata.other.timestamp; }
    unsigned int get_shot_order() { return _processor_ptr.get()->imgdata.other.shot_order; }
    unsigned* get_gpsdata()   { return _processor_ptr.get()->imgdata.other.gpsdata; }
    std::string get_desc()    { return std::string(_processor_ptr.get()->imgdata.other.desc); }
    std::string get_artist()  { return std::string(_processor_ptr.get()->imgdata.other.artist); }

    std::string get_version()               { return std::string(_processor_ptr.get()->version()); }
    std::string get_unpack_function_name()  { return std::string(_processor_ptr.get()->unpack_function_name()); }

    void get_mem_image_format(int *widthp, int *heightp, int *colorsp, int *bpp) { _processor_ptr.get()->get_mem_image_format(widthp, heightp, colorsp, bpp); }

    int unpack()                                                         { return _processor_ptr.get()->unpack(); }
    int dcraw_process()                                                  { return _processor_ptr.get()->dcraw_process(); }
    libraw_processed_image_t* dcraw_make_mem_image(int* error_code=nullptr) { return _processor_ptr.get()->dcraw_make_mem_image(error_code); }

protected:

    using libraw_ptr_t = std::shared_ptr<LibRaw>;
    libraw_ptr_t _processor_ptr;
};

/*!
 *
 * file_stream_device specialization for raw images
 */
template<>
class file_stream_device< raw_tag > : public raw_device_base
{
public:

    struct read_tag {};

    ///
    /// Constructor
    ///
    file_stream_device( std::string const& file_name
                      , read_tag   = read_tag()
                      )
    {
        io_error_if( _processor_ptr.get()->open_file( file_name.c_str() ) != LIBRAW_SUCCESS
                   , "file_stream_device: failed to open file"
                   );
    }

    ///
    /// Constructor
    ///
    file_stream_device( const char* file_name
                      , read_tag   = read_tag()
                      )
    {
        io_error_if( _processor_ptr.get()->open_file( file_name ) != LIBRAW_SUCCESS
                   , "file_stream_device: failed to open file"
                   );
    }
};

template< typename FormatTag >
struct is_adaptable_input_device<FormatTag, LibRaw, void> : std::true_type
{
    using device_type = file_stream_device<FormatTag>;
};


} // namespace detail
} // namespace gil
} // namespace boost

#endif
