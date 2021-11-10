// (C) Copyright Reimar Döffinger 2018.
// Based on zstd.hpp by:
// (C) Copyright Milan Svoboda 2008.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_ZSTD_HPP_INCLUDED
#define BOOST_IOSTREAMS_ZSTD_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <cassert>
#include <iosfwd>            // streamsize.
#include <memory>            // allocator, bad_alloc.
#include <new>
#include <boost/config.hpp>  // MSVC, STATIC_CONSTANT, DEDUCED_TYPENAME, DINKUM.
#include <boost/detail/workaround.hpp>
#include <boost/iostreams/constants.hpp>   // buffer size.
#include <boost/iostreams/detail/config/auto_link.hpp>
#include <boost/iostreams/detail/config/dyn_link.hpp>
#include <boost/iostreams/detail/config/wide_streams.hpp>
#include <boost/iostreams/detail/ios.hpp>  // failure, streamsize.
#include <boost/iostreams/filter/symmetric.hpp>
#include <boost/iostreams/pipeline.hpp>
#include <boost/type_traits/is_same.hpp>

// Must come last.
#ifdef BOOST_MSVC
# pragma warning(push)
# pragma warning(disable:4251 4231 4660)         // Dependencies not exported.
#endif
#include <boost/config/abi_prefix.hpp>

namespace boost { namespace iostreams {

namespace zstd {

typedef void* (*alloc_func)(void*, size_t, size_t);
typedef void (*free_func)(void*, void*);

                    // Compression levels

BOOST_IOSTREAMS_DECL extern const uint32_t best_speed;
BOOST_IOSTREAMS_DECL extern const uint32_t best_compression;
BOOST_IOSTREAMS_DECL extern const uint32_t default_compression;

                    // Status codes

BOOST_IOSTREAMS_DECL extern const int okay;
BOOST_IOSTREAMS_DECL extern const int stream_end;

                    // Flush codes

BOOST_IOSTREAMS_DECL extern const int finish;
BOOST_IOSTREAMS_DECL extern const int flush;
BOOST_IOSTREAMS_DECL extern const int run;

                    // Code for current OS

                    // Null pointer constant.

const int null                               = 0;

                    // Default values

} // End namespace zstd.

//
// Class name: zstd_params.
// Description: Encapsulates the parameters passed to zstddec_init
//      to customize compression and decompression.
//
struct zstd_params {

    // Non-explicit constructor.
    zstd_params( uint32_t level = zstd::default_compression )
        : level(level)
        { }
    uint32_t level;
};

//
// Class name: zstd_error.
// Description: Subclass of std::ios::failure thrown to indicate
//     zstd errors other than out-of-memory conditions.
//
class BOOST_IOSTREAMS_DECL zstd_error : public BOOST_IOSTREAMS_FAILURE {
public:
    explicit zstd_error(size_t error);
    int error() const { return error_; }
    static void check BOOST_PREVENT_MACRO_SUBSTITUTION(size_t error);
private:
    size_t error_;
};

namespace detail {

template<typename Alloc>
struct zstd_allocator_traits {
#ifndef BOOST_NO_STD_ALLOCATOR
#if defined(BOOST_NO_CXX11_ALLOCATOR)
    typedef typename Alloc::template rebind<char>::other type;
#else
    typedef typename std::allocator_traits<Alloc>::template rebind_alloc<char> type;
#endif
#else
    typedef std::allocator<char> type;
#endif
};

template< typename Alloc,
          typename Base = // VC6 workaround (C2516)
              BOOST_DEDUCED_TYPENAME zstd_allocator_traits<Alloc>::type >
struct zstd_allocator : private Base {
private:
#if defined(BOOST_NO_CXX11_ALLOCATOR) || defined(BOOST_NO_STD_ALLOCATOR)
    typedef typename Base::size_type size_type;
#else
    typedef typename std::allocator_traits<Base>::size_type size_type;
#endif
public:
    BOOST_STATIC_CONSTANT(bool, custom =
        (!is_same<std::allocator<char>, Base>::value));
    typedef typename zstd_allocator_traits<Alloc>::type allocator_type;
    static void* allocate(void* self, size_t items, size_t size);
    static void deallocate(void* self, void* address);
};

class BOOST_IOSTREAMS_DECL zstd_base {
public:
    typedef char char_type;
protected:
    zstd_base();
    ~zstd_base();
    template<typename Alloc>
    void init( const zstd_params& p,
               bool compress,
               zstd_allocator<Alloc>& zalloc )
        {
            bool custom = zstd_allocator<Alloc>::custom;
            do_init( p, compress,
                     custom ? zstd_allocator<Alloc>::allocate : 0,
                     custom ? zstd_allocator<Alloc>::deallocate : 0,
                     &zalloc );
        }
    void before( const char*& src_begin, const char* src_end,
                 char*& dest_begin, char* dest_end );
    void after( const char*& src_begin, char*& dest_begin,
                bool compress );
    int deflate(int action);
    int inflate(int action);
    void reset(bool compress, bool realloc);
private:
    void do_init( const zstd_params& p, bool compress,
                  zstd::alloc_func,
                  zstd::free_func,
                  void* derived );
    void*         cstream_;         // Actual type: ZSTD_CStream *
    void*         dstream_;         // Actual type: ZSTD_DStream *
    void*         in_;              // Actual type: ZSTD_inBuffer *
    void*         out_;             // Actual type: ZSTD_outBuffer *
    int eof_;
    uint32_t level;
};

//
// Template name: zstd_compressor_impl
// Description: Model of C-Style Filter implementing compression by
//      delegating to the zstd function deflate.
//
template<typename Alloc = std::allocator<char> >
class zstd_compressor_impl : public zstd_base, public zstd_allocator<Alloc> {
public:
    zstd_compressor_impl(const zstd_params& = zstd::default_compression);
    ~zstd_compressor_impl();
    bool filter( const char*& src_begin, const char* src_end,
                 char*& dest_begin, char* dest_end, bool flush );
    void close();
};

//
// Template name: zstd_compressor_impl
// Description: Model of C-Style Filte implementing decompression by
//      delegating to the zstd function inflate.
//
template<typename Alloc = std::allocator<char> >
class zstd_decompressor_impl : public zstd_base, public zstd_allocator<Alloc> {
public:
    zstd_decompressor_impl(const zstd_params&);
    zstd_decompressor_impl();
    ~zstd_decompressor_impl();
    bool filter( const char*& begin_in, const char* end_in,
                 char*& begin_out, char* end_out, bool flush );
    void close();
};

} // End namespace detail.

//
// Template name: zstd_compressor
// Description: Model of InputFilter and OutputFilter implementing
//      compression using zstd.
//
template<typename Alloc = std::allocator<char> >
struct basic_zstd_compressor
    : symmetric_filter<detail::zstd_compressor_impl<Alloc>, Alloc>
{
private:
    typedef detail::zstd_compressor_impl<Alloc> impl_type;
    typedef symmetric_filter<impl_type, Alloc>  base_type;
public:
    typedef typename base_type::char_type               char_type;
    typedef typename base_type::category                category;
    basic_zstd_compressor( const zstd_params& = zstd::default_compression,
                           std::streamsize buffer_size = default_device_buffer_size );
};
BOOST_IOSTREAMS_PIPABLE(basic_zstd_compressor, 1)

typedef basic_zstd_compressor<> zstd_compressor;

//
// Template name: zstd_decompressor
// Description: Model of InputFilter and OutputFilter implementing
//      decompression using zstd.
//
template<typename Alloc = std::allocator<char> >
struct basic_zstd_decompressor
    : symmetric_filter<detail::zstd_decompressor_impl<Alloc>, Alloc>
{
private:
    typedef detail::zstd_decompressor_impl<Alloc> impl_type;
    typedef symmetric_filter<impl_type, Alloc>    base_type;
public:
    typedef typename base_type::char_type               char_type;
    typedef typename base_type::category                category;
    basic_zstd_decompressor( std::streamsize buffer_size = default_device_buffer_size );
    basic_zstd_decompressor( const zstd_params& p,
                             std::streamsize buffer_size = default_device_buffer_size );
};
BOOST_IOSTREAMS_PIPABLE(basic_zstd_decompressor, 1)

typedef basic_zstd_decompressor<> zstd_decompressor;

//----------------------------------------------------------------------------//

//------------------Implementation of zstd_allocator--------------------------//

namespace detail {

template<typename Alloc, typename Base>
void* zstd_allocator<Alloc, Base>::allocate
    (void* self, size_t items, size_t size)
{
    size_type len = items * size;
    char* ptr =
        static_cast<allocator_type*>(self)->allocate
            (len + sizeof(size_type)
            #if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
                , (char*)0
            #endif
            );
    *reinterpret_cast<size_type*>(ptr) = len;
    return ptr + sizeof(size_type);
}

template<typename Alloc, typename Base>
void zstd_allocator<Alloc, Base>::deallocate(void* self, void* address)
{
    char* ptr = reinterpret_cast<char*>(address) - sizeof(size_type);
    size_type len = *reinterpret_cast<size_type*>(ptr) + sizeof(size_type);
    static_cast<allocator_type*>(self)->deallocate(ptr, len);
}

//------------------Implementation of zstd_compressor_impl--------------------//

template<typename Alloc>
zstd_compressor_impl<Alloc>::zstd_compressor_impl(const zstd_params& p)
{ init(p, true, static_cast<zstd_allocator<Alloc>&>(*this)); }

template<typename Alloc>
zstd_compressor_impl<Alloc>::~zstd_compressor_impl()
{ reset(true, false); }

template<typename Alloc>
bool zstd_compressor_impl<Alloc>::filter
    ( const char*& src_begin, const char* src_end,
      char*& dest_begin, char* dest_end, bool flush )
{
    before(src_begin, src_end, dest_begin, dest_end);
    int result = deflate(flush ? zstd::finish : zstd::run);
    after(src_begin, dest_begin, true);
    return result != zstd::stream_end;
}

template<typename Alloc>
void zstd_compressor_impl<Alloc>::close() { reset(true, true); }

//------------------Implementation of zstd_decompressor_impl------------------//

template<typename Alloc>
zstd_decompressor_impl<Alloc>::zstd_decompressor_impl(const zstd_params& p)
{ init(p, false, static_cast<zstd_allocator<Alloc>&>(*this)); }

template<typename Alloc>
zstd_decompressor_impl<Alloc>::~zstd_decompressor_impl()
{ reset(false, false); }

template<typename Alloc>
zstd_decompressor_impl<Alloc>::zstd_decompressor_impl()
{
    zstd_params p;
    init(p, false, static_cast<zstd_allocator<Alloc>&>(*this));
}

template<typename Alloc>
bool zstd_decompressor_impl<Alloc>::filter
    ( const char*& src_begin, const char* src_end,
      char*& dest_begin, char* dest_end, bool flush )
{
    before(src_begin, src_end, dest_begin, dest_end);
    int result = inflate(flush ? zstd::finish : zstd::run);
    after(src_begin, dest_begin, false);
    return result != zstd::stream_end;
}

template<typename Alloc>
void zstd_decompressor_impl<Alloc>::close() { reset(false, true); }

} // End namespace detail.

//------------------Implementation of zstd_compressor-----------------------//

template<typename Alloc>
basic_zstd_compressor<Alloc>::basic_zstd_compressor
    (const zstd_params& p, std::streamsize buffer_size)
    : base_type(buffer_size, p) { }

//------------------Implementation of zstd_decompressor-----------------------//

template<typename Alloc>
basic_zstd_decompressor<Alloc>::basic_zstd_decompressor
    (std::streamsize buffer_size)
    : base_type(buffer_size) { }

template<typename Alloc>
basic_zstd_decompressor<Alloc>::basic_zstd_decompressor
    (const zstd_params& p, std::streamsize buffer_size)
    : base_type(buffer_size, p) { }

//----------------------------------------------------------------------------//

} } // End namespaces iostreams, boost.

#include <boost/config/abi_suffix.hpp> // Pops abi_suffix.hpp pragmas.
#ifdef BOOST_MSVC
# pragma warning(pop)
#endif

#endif // #ifndef BOOST_IOSTREAMS_ZSTD_HPP_INCLUDED
