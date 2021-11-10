//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_JPEG_DETAIL_WRITER_BACKEND_HPP
#define BOOST_GIL_EXTENSION_IO_JPEG_DETAIL_WRITER_BACKEND_HPP

#include <boost/gil/extension/io/jpeg/tags.hpp>
#include <boost/gil/extension/io/jpeg/detail/base.hpp>

#include <memory>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#pragma warning(disable:4611) //interaction between '_setjmp' and C++ object destruction is non-portable
#endif

namespace detail {

///
/// Wrapper for libjpeg's compress object. Implements value semantics.
///
struct jpeg_compress_wrapper
{
protected:

    using jpeg_compress_ptr_t = std::shared_ptr<jpeg_compress_struct>;

protected:

    ///
    /// Default Constructor
    ///
    jpeg_compress_wrapper()
    : _jpeg_compress_ptr( new jpeg_compress_struct()
                        , jpeg_compress_deleter
                        )
    {}

    jpeg_compress_struct*       get()       { return _jpeg_compress_ptr.get(); }
    const jpeg_compress_struct* get() const { return _jpeg_compress_ptr.get(); }

private:

    static void jpeg_compress_deleter( jpeg_compress_struct* jpeg_compress_ptr )
    {
        if( jpeg_compress_ptr )
        {
            jpeg_destroy_compress( jpeg_compress_ptr );

            delete jpeg_compress_ptr;
            jpeg_compress_ptr = nullptr;
        }
    }

private:

   jpeg_compress_ptr_t _jpeg_compress_ptr;

};

} // namespace detail

///
/// JPEG Writer Backend
///
template< typename Device >
struct writer_backend< Device
                     , jpeg_tag
                     >
    : public jpeg_io_base
    , public detail::jpeg_compress_wrapper
{
public:

    using format_tag_t = jpeg_tag;

public:
    ///
    /// Constructor
    ///
    writer_backend( const Device&                       io_dev
                  , const image_write_info< jpeg_tag >& info
                  )
    : _io_dev( io_dev )
    , _info( info )
    {
        get()->err         = jpeg_std_error( &_jerr );
        get()->client_data = this;

        // Error exit handler: does not return to caller.
        _jerr.error_exit = &writer_backend< Device, jpeg_tag >::error_exit;

        // Fire exception in case of error.
        if( setjmp( _mark )) { raise_error(); }

        _dest._jdest.free_in_buffer      = sizeof( buffer );
        _dest._jdest.next_output_byte    = buffer;
        _dest._jdest.init_destination    = reinterpret_cast< void(*)   ( j_compress_ptr ) >( &writer_backend< Device, jpeg_tag >::init_device  );
        _dest._jdest.empty_output_buffer = reinterpret_cast< boolean(*)( j_compress_ptr ) >( &writer_backend< Device, jpeg_tag >::empty_buffer );
        _dest._jdest.term_destination    = reinterpret_cast< void(*)   ( j_compress_ptr ) >( &writer_backend< Device, jpeg_tag >::close_device );
        _dest._this = this;

        jpeg_create_compress( get() );
        get()->dest = &_dest._jdest;
    }

    ~writer_backend()
    {
        // JPEG compression object destruction does not signal errors,
        // unlike jpeg_finish_compress called elsewhere,
        // so there is no need for the setjmp bookmark here.
        jpeg_destroy_compress( get() );
    }

protected:

    struct gil_jpeg_destination_mgr
    {
        jpeg_destination_mgr _jdest;
        writer_backend< Device
                      , jpeg_tag
                      >* _this;
    };

    static void init_device( jpeg_compress_struct* cinfo )
    {
        gil_jpeg_destination_mgr* dest = reinterpret_cast< gil_jpeg_destination_mgr* >( cinfo->dest );

        dest->_jdest.free_in_buffer   = sizeof( dest->_this->buffer );
        dest->_jdest.next_output_byte = dest->_this->buffer;
    }

    static boolean empty_buffer( jpeg_compress_struct* cinfo )
    {
        gil_jpeg_destination_mgr* dest = reinterpret_cast< gil_jpeg_destination_mgr* >( cinfo->dest );

        dest->_this->_io_dev.write( dest->_this->buffer
                                  , buffer_size
                                  );

        writer_backend<Device,jpeg_tag>::init_device( cinfo );
        return static_cast<boolean>(TRUE);
    }

    static void close_device( jpeg_compress_struct* cinfo )
    {
        writer_backend< Device
                      , jpeg_tag
                      >::empty_buffer( cinfo );

        gil_jpeg_destination_mgr* dest = reinterpret_cast< gil_jpeg_destination_mgr* >( cinfo->dest );

        dest->_this->_io_dev.flush();
    }

    void raise_error()
    {
        io_error( "Cannot write jpeg file." );
    }

    static void error_exit( j_common_ptr cinfo )
    {
        writer_backend< Device, jpeg_tag >* mgr = reinterpret_cast< writer_backend< Device, jpeg_tag >* >( cinfo->client_data );

        longjmp( mgr->_mark, 1 );
    }

public:

    Device _io_dev;

    image_write_info< jpeg_tag > _info;

    gil_jpeg_destination_mgr _dest;

    static const unsigned int buffer_size = 1024;
    JOCTET buffer[buffer_size];
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // namespace gil
} // namespace boost

#endif
