//
// Copyright 2007-2012 Christian Henning, Andreas Pokorny
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_IO_DEVICE_HPP
#define BOOST_GIL_IO_DEVICE_HPP

#include <boost/gil/detail/mp11.hpp>
#include <boost/gil/io/base.hpp>

#include <cstdio>
#include <memory>
#include <type_traits>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

namespace detail {

template < typename T > struct buff_item
{
    static const unsigned int size = sizeof( T );
};

template <> struct buff_item< void >
{
    static const unsigned int size = 1;
};

/*!
 * Implements the IODevice concept c.f. to \ref IODevice required by Image libraries like
 * libjpeg and libpng.
 *
 * \todo switch to a sane interface as soon as there is
 * something good in boost. I.E. the IOChains library
 * would fit very well here.
 *
 * This implementation is based on FILE*.
 */
template< typename FormatTag >
class file_stream_device
{
public:

   using format_tag_t = FormatTag;

public:

    /// Used to overload the constructor.
    struct read_tag {};
    struct write_tag {};

    ///
    /// Constructor
    ///
    file_stream_device( const std::string& file_name
                      , read_tag tag  = read_tag()
                      )
        : file_stream_device(file_name.c_str(), tag)
    {}

    ///
    /// Constructor
    ///
    file_stream_device( const char* file_name
                      , read_tag   = read_tag()
                      )
    {
        FILE* file = nullptr;

        io_error_if( ( file = fopen( file_name, "rb" )) == nullptr
                   , "file_stream_device: failed to open file for reading"
                   );

        _file = file_ptr_t( file
                          , file_deleter
                          );
    }

    ///
    /// Constructor
    ///
    file_stream_device( const std::string& file_name
                      , write_tag tag
                      )
        : file_stream_device(file_name.c_str(), tag)
    {}

    ///
    /// Constructor
    ///
    file_stream_device( const char* file_name
                      , write_tag
                      )
    {
        FILE* file = nullptr;

        io_error_if( ( file = fopen( file_name, "wb" )) == nullptr
                   , "file_stream_device: failed to open file for writing"
                   );

        _file = file_ptr_t( file
                          , file_deleter
                          );
    }

    ///
    /// Constructor
    ///
    file_stream_device( FILE* file )
    : _file( file
           , file_deleter
           )
    {}

    FILE*       get()       { return _file.get(); }
    const FILE* get() const { return _file.get(); }

    int getc_unchecked()
    {
        return std::getc( get() );
    }

    char getc()
    {
        int ch;

        io_error_if( ( ch = std::getc( get() )) == EOF
                   , "file_stream_device: unexpected EOF"
                   );

        return ( char ) ch;
    }

    ///@todo: change byte_t* to void*
    std::size_t read( byte_t*     data
                    , std::size_t count
                    )
    {
        std::size_t num_elements = fread( data
                                        , 1
                                        , static_cast<int>( count )
                                        , get()
                                        );

        ///@todo: add compiler symbol to turn error checking on and off.
        io_error_if( ferror( get() )
                   , "file_stream_device: file read error"
                   );

        //libjpeg sometimes reads blocks in 4096 bytes even when the file is smaller than that.
        //return value indicates how much was actually read
        //returning less than "count" is not an error
        return num_elements;
    }

    /// Reads array
    template< typename T
            , int      N
            >
    void read( T (&buf)[N] )
    {
        io_error_if( read( buf, N ) < N
                   , "file_stream_device: file read error"
                   );
    }

    /// Reads byte
    uint8_t read_uint8()
    {
        byte_t m[1];

        read( m );
        return m[0];
    }

    /// Reads 16 bit little endian integer
    uint16_t read_uint16()
    {
        byte_t m[2];

        read( m );
        return (m[1] << 8) | m[0];
    }

    /// Reads 32 bit little endian integer
    uint32_t read_uint32()
    {
        byte_t m[4];

        read( m );
        return (m[3] << 24) | (m[2] << 16) | (m[1] << 8) | m[0];
    }

    /// Writes number of elements from a buffer
    template < typename T >
    std::size_t write( const T*    buf
                     , std::size_t count
                     )
    {
        std::size_t num_elements = fwrite( buf
                                         , buff_item<T>::size
                                         , count
                                         , get()
                                         );

        //return value indicates how much was actually written
        //returning less than "count" is not an error
        return num_elements;
    }

    /// Writes array
    template < typename    T
             , std::size_t N
             >
    void write( const T (&buf)[N] )
    {
        io_error_if( write( buf, N ) < N
                   , "file_stream_device: file write error"
                   );
        return ;
    }

    /// Writes byte
    void write_uint8( uint8_t x )
    {
        byte_t m[1] = { x };
        write(m);
    }

    /// Writes 16 bit little endian integer
    void write_uint16( uint16_t x )
    {
        byte_t m[2];

        m[0] = byte_t( x >> 0 );
        m[1] = byte_t( x >> 8 );

        write( m );
    }

    /// Writes 32 bit little endian integer
    void write_uint32( uint32_t x )
    {
        byte_t m[4];

        m[0] = byte_t( x >>  0 );
        m[1] = byte_t( x >>  8 );
        m[2] = byte_t( x >> 16 );
        m[3] = byte_t( x >> 24 );

        write( m );
    }

    void seek( long count, int whence = SEEK_SET )
    {
        io_error_if( fseek( get()
                          , count
                          , whence
                          ) != 0
                   , "file_stream_device: file seek error"
                   );
    }

    long int tell()
    {
        long int pos = ftell( get() );

        io_error_if( pos == -1L
                   , "file_stream_device: file position error"
                   );

        return pos;
    }

    void flush()
    {
        fflush( get() );
    }

    /// Prints formatted ASCII text
    void print_line( const std::string& line )
    {
        std::size_t num_elements = fwrite( line.c_str()
                                         , sizeof( char )
                                         , line.size()
                                         , get()
                                         );

        io_error_if( num_elements < line.size()
                   , "file_stream_device: line print error"
                   );
    }

    int error()
    {
        return ferror( get() );
    }

private:

    static void file_deleter( FILE* file )
    {
        if( file )
        {
            fclose( file );
        }
    }

private:

    using file_ptr_t = std::shared_ptr<FILE> ;
    file_ptr_t _file;
};

/**
 * Input stream device
 */
template< typename FormatTag >
class istream_device
{
public:
   istream_device( std::istream& in )
   : _in( in )
   {
       // does the file exists?
       io_error_if( !in
                  , "istream_device: Stream is not valid."
                  );
   }

    int getc_unchecked()
    {
        return _in.get();
    }

    char getc()
    {
        int ch;

        io_error_if( ( ch = _in.get() ) == EOF
                   , "istream_device: unexpected EOF"
                   );

        return ( char ) ch;
    }

    std::size_t read( byte_t*     data
                    , std::size_t count )
    {
        std::streamsize cr = 0;

        do
        {
            _in.peek();
            std::streamsize c = _in.readsome( reinterpret_cast< char* >( data )
                                            , static_cast< std::streamsize >( count ));

            count -= static_cast< std::size_t >( c );
            data += c;
            cr += c;

        } while( count && _in );

        return static_cast< std::size_t >( cr );
    }

    /// Reads array
    template<typename T, int N>
    void read(T (&buf)[N])
    {
        read(buf, N);
    }

    /// Reads byte
    uint8_t read_uint8()
    {
        byte_t m[1];

        read( m );
        return m[0];
    }

    /// Reads 16 bit little endian integer
    uint16_t read_uint16()
    {
        byte_t m[2];

        read( m );
        return (m[1] << 8) | m[0];
    }

    /// Reads 32 bit little endian integer
    uint32_t read_uint32()
    {
        byte_t m[4];

        read( m );
        return (m[3] << 24) | (m[2] << 16) | (m[1] << 8) | m[0];
    }

    void seek( long count, int whence = SEEK_SET )
    {
        _in.seekg( count
                 , whence == SEEK_SET ? std::ios::beg
                                      :( whence == SEEK_CUR ? std::ios::cur
                                                            : std::ios::end )
                 );
    }

    void write(const byte_t*, std::size_t)
    {
        io_error( "istream_device: Bad io error." );
    }

    void flush() {}

private:

    std::istream& _in;
};

/**
 * Output stream device
 */
template< typename FormatTag >
class ostream_device
{
public:
    ostream_device( std::ostream & out )
        : _out( out )
    {
    }

    std::size_t read(byte_t *, std::size_t)
    {
        io_error( "ostream_device: Bad io error." );
        return 0;
    }

    void seek( long count, int whence )
    {
        _out.seekp( count
                  , whence == SEEK_SET
                    ? std::ios::beg
                    : ( whence == SEEK_CUR
                        ?std::ios::cur
                        :std::ios::end )
                  );
    }

    void write( const byte_t* data
              , std::size_t   count )
    {
        _out.write( reinterpret_cast<char const*>( data )
                 , static_cast<std::streamsize>( count )
                 );
    }

    /// Writes array
    template < typename    T
             , std::size_t N
             >
    void write( const T (&buf)[N] )
    {
        write( buf, N );
    }

    /// Writes byte
    void write_uint8( uint8_t x )
    {
        byte_t m[1] = { x };
        write(m);
    }

    /// Writes 16 bit little endian integer
    void write_uint16( uint16_t x )
    {
        byte_t m[2];

        m[0] = byte_t( x >> 0 );
        m[1] = byte_t( x >> 8 );

        write( m );
    }

    /// Writes 32 bit little endian integer
    void write_uint32( uint32_t x )
    {
        byte_t m[4];

        m[0] = byte_t( x >>  0 );
        m[1] = byte_t( x >>  8 );
        m[2] = byte_t( x >> 16 );
        m[3] = byte_t( x >> 24 );

        write( m );
    }

    void flush()
    {
        _out << std::flush;
    }

    /// Prints formatted ASCII text
    void print_line( const std::string& line )
    {
        _out << line;
    }



private:

    std::ostream& _out;
};


/**
 * Metafunction to detect input devices.
 * Should be replaced by an external facility in the future.
 */
template< typename IODevice  > struct is_input_device : std::false_type{};
template< typename FormatTag > struct is_input_device< file_stream_device< FormatTag > > : std::true_type{};
template< typename FormatTag > struct is_input_device<     istream_device< FormatTag > > : std::true_type{};

template< typename FormatTag
        , typename T
        , typename D = void
        >
struct is_adaptable_input_device : std::false_type{};

template <typename FormatTag, typename T>
struct is_adaptable_input_device
<
    FormatTag,
    T,
    typename std::enable_if
    <
        mp11::mp_or
        <
            std::is_base_of<std::istream, T>,
            std::is_same<std::istream, T>
        >::value
    >::type
> : std::true_type
{
    using device_type = istream_device<FormatTag>;
};

template< typename FormatTag >
struct is_adaptable_input_device< FormatTag
                                , FILE*
                                , void
                                >
    : std::true_type
{
    using device_type = file_stream_device<FormatTag>;
};

///
/// Metafunction to decide if a given type is an acceptable read device type.
///
template< typename FormatTag
        , typename T
        , typename D = void
        >
struct is_read_device : std::false_type
{};

template <typename FormatTag, typename T>
struct is_read_device
<
    FormatTag,
    T,
    typename std::enable_if
    <
        mp11::mp_or
        <
            is_input_device<FormatTag>,
            is_adaptable_input_device<FormatTag, T>
        >::value
    >::type
> : std::true_type
{
};


/**
 * Metafunction to detect output devices.
 * Should be replaced by an external facility in the future.
 */
template<typename IODevice> struct is_output_device : std::false_type{};

template< typename FormatTag > struct is_output_device< file_stream_device< FormatTag > > : std::true_type{};
template< typename FormatTag > struct is_output_device< ostream_device    < FormatTag > > : std::true_type{};

template< typename FormatTag
        , typename IODevice
        , typename D = void
        >
struct is_adaptable_output_device : std::false_type {};

template <typename FormatTag, typename T>
struct is_adaptable_output_device
<
    FormatTag,
    T,
    typename std::enable_if
    <
        mp11::mp_or
        <
            std::is_base_of<std::ostream, T>,
            std::is_same<std::ostream, T>
        >::value
    >::type
> : std::true_type
{
    using device_type = ostream_device<FormatTag>;
};

template<typename FormatTag> struct is_adaptable_output_device<FormatTag,FILE*,void>
  : std::true_type
{
    using device_type = file_stream_device<FormatTag>;
};


///
/// Metafunction to decide if a given type is an acceptable read device type.
///
template< typename FormatTag
        , typename T
        , typename D = void
        >
struct is_write_device : std::false_type
{};

template <typename FormatTag, typename T>
struct is_write_device
<
    FormatTag,
    T,
    typename std::enable_if
    <
        mp11::mp_or
        <
            is_output_device<FormatTag>,
            is_adaptable_output_device<FormatTag, T>
        >::value
    >::type
> : std::true_type
{
};

} // namespace detail

template< typename Device, typename FormatTag > class scanline_reader;
template< typename Device, typename FormatTag, typename ConversionPolicy > class reader;

template< typename Device, typename FormatTag, typename Log = no_log > class writer;

template< typename Device, typename FormatTag > class dynamic_image_reader;
template< typename Device, typename FormatTag, typename Log = no_log > class dynamic_image_writer;


namespace detail {

template< typename T >
struct is_reader : std::false_type
{};

template< typename Device
        , typename FormatTag
        , typename ConversionPolicy
        >
struct is_reader< reader< Device
                        , FormatTag
                        , ConversionPolicy
                        >
                > : std::true_type
{};

template< typename T >
struct is_dynamic_image_reader : std::false_type
{};

template< typename Device
        , typename FormatTag
        >
struct is_dynamic_image_reader< dynamic_image_reader< Device
                                                    , FormatTag
                                                    >
                              > : std::true_type
{};

template< typename T >
struct is_writer : std::false_type
{};

template< typename Device
        , typename FormatTag
        >
struct is_writer< writer< Device
                        , FormatTag
                        >
                > : std::true_type
{};

template< typename T >
struct is_dynamic_image_writer : std::false_type
{};

template< typename Device
        , typename FormatTag
        >
struct is_dynamic_image_writer< dynamic_image_writer< Device
                                                    , FormatTag
                                                    >
                > : std::true_type
{};

} // namespace detail

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // namespace gil
} // namespace boost

#endif
