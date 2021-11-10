//
// Copyright 2007-2008 Christian Henning, Andreas Pokorny
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_IO_IO_HPP
#define BOOST_GIL_IO_IO_HPP

/*!
 * \page iobackend Adding a new io backend
 * \section Overview of backend requirements
 * To add support for a new IO backend the following is required:
 *  - a format tag, to identify the image format, derived from boost::gil::format_tag
 *  - boolean meta function is_supported<PixelType,FormatTag> must be implemented for
 *    the new format tag
 *  - explicit specialisation of image_read_info<FormatTag> must be provided, containing
 *    runtime information available before/at reading the image
 *  - explicit specialisation of image_write_info<FormatTag> must be provided, containing
 *    runtime encoding parameters for writing an image
 *  - An image reader must be specialized:
 *    \code
 *      template<typename IODevice, typename ConversionPolicy>
 *      struct boost::gil::reader<IODevice,FormatTag,ConversionPolicy>
 *      {
 *        reader( IODevice & device )
 *        reader( IODevice & device, typename ConversionPolicy::color_converter_type const& cc )
 *        image_read_info<FormatTag> get_info();
 *        template<typename Image>
 *        void read_image( Image &, point_t const& top_left );
 *        template<typename View>
 *        void read_view( View &, point_t const& top_left );
 *      };
 *    \endcode
 *  - An image writer must be specialized:
 *    \code
 *      \template <typename IODevice>
 *      struct boost::gil::writer<IODevice,FormatTag>
 *      {
 *        writer( IODevice & device )
 *        template<typename View>
 *        void apply( View const&, point_t const& top_left );
 *        template<typename View>
 *        void apply( View const&, point_t const& top_left, image_write_info<FormatTag> const& );
 *      };
 *    \endcode
 *
 * Or instead of the items above implement overloads of read_view, read_and_convert_view, read_image,
 * read_and_convert_image, write_view and read_image_info.
 *
 * \section ConversionPolicy Interface of the ConversionPolicy
 * There are two different conversion policies in use, when reading images:
 * read_and_convert<ColorConverter> and read_and_no_convert. ColorConverter
 * can be a user defined color converter.
 *
 * \code
 * struct ConversionPolicy
 * {
 *    template<typename InputIterator,typename OutputIterator>
 *    void read( InputIterator in_begin, InputIterator in_end,
 *          OutputIterator out_end );
 * };
 * \endcode
 *
 * Methods like read_view and read_image are supposed to bail out with an
 * exception instead of converting the image
 *
 * \section IODevice Concept of IO Device
 * A Device is simply an object used to read and write data to and from a stream.
 * The IODevice was added as a template paramter to be able to replace the file_name
 * access functionality. This is only an interim solution, as soon as boost provides
 * a good IO library, interfaces/constraints provided by that library could be used.
 *
 * \code
 *  concept IODevice
 *  {
 *      void IODevice::read( unsigned char* data, int count );
 *      void IODevice::write( unsigned char* data, int count );
 *      void IODevice::seek(long count, int whence);
 *      void IODevice::flush();
 *  };
 * \endcode
 *
 * For the time being a boolean meta function must be specialized:
 * \code
 * namespace boost{namespace gil{namespace detail{
 *  template<typename Device>
 *  struct detail::is_input_device;
 * }}}
 * \endcode
 *
 */

#endif
