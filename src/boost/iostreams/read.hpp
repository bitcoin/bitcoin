// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_READ_HPP_INCLUDED
#define BOOST_IOSTREAMS_READ_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>  // DEDUCED_TYPENAME, MSVC.
#include <boost/detail/workaround.hpp>
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/detail/char_traits.hpp>
#include <boost/iostreams/detail/dispatch.hpp>
#include <boost/iostreams/detail/ios.hpp>  // streamsize.
#include <boost/iostreams/detail/streambuf.hpp>
#include <boost/iostreams/detail/wrap_unwrap.hpp>
#include <boost/iostreams/operations_fwd.hpp>
#include <boost/mpl/if.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp>

namespace boost { namespace iostreams {

namespace detail {

template<typename T>
struct read_device_impl;

template<typename T>
struct read_filter_impl;

} // End namespace detail.

template<typename T>
typename int_type_of<T>::type get(T& t)
{ return detail::read_device_impl<T>::get(detail::unwrap(t)); }

template<typename T>
inline std::streamsize
read(T& t, typename char_type_of<T>::type* s, std::streamsize n)
{ return detail::read_device_impl<T>::read(detail::unwrap(t), s, n); }

template<typename T, typename Source>
std::streamsize
read(T& t, Source& src, typename char_type_of<T>::type* s, std::streamsize n)
{ return detail::read_filter_impl<T>::read(detail::unwrap(t), src, s, n); }

template<typename T>
bool putback(T& t, typename char_type_of<T>::type c)
{ return detail::read_device_impl<T>::putback(detail::unwrap(t), c); }

//----------------------------------------------------------------------------//

namespace detail {

// Helper function for adding -1 as EOF indicator.
inline std::streamsize check_eof(std::streamsize n) { return n != 0 ? n : -1; }

// Helper templates for reading from streambufs.
template<bool IsLinked>
struct true_eof_impl;

template<>
struct true_eof_impl<true> {
    template<typename T>
    static bool true_eof(T& t) { return t.true_eof(); }
};

template<>
struct true_eof_impl<false> {
    template<typename T>
    static bool true_eof(T&) { return true; }
};

template<typename T>
inline bool true_eof(T& t)
{
    const bool linked = is_linked<T>::value;
    return true_eof_impl<linked>::true_eof(t);
}

//------------------Definition of read_device_impl----------------------------//

template<typename T>
struct read_device_impl
    : mpl::if_<
          detail::is_custom<T>,
          operations<T>,
          read_device_impl<
              BOOST_DEDUCED_TYPENAME
              detail::dispatch<
                  T, istream_tag, streambuf_tag, input
              >::type
          >
      >::type
    { };

template<>
struct read_device_impl<istream_tag> {
    template<typename T>
    static typename int_type_of<T>::type get(T& t)
    { return t.get(); }

    template<typename T>
    static std::streamsize
    read(T& t, typename char_type_of<T>::type* s, std::streamsize n)
    { return check_eof(t.rdbuf()->sgetn(s, n)); }

    template<typename T>
    static bool putback(T& t, typename char_type_of<T>::type c)
    {
        typedef typename char_type_of<T>::type          char_type;
        typedef BOOST_IOSTREAMS_CHAR_TRAITS(char_type)  traits_type;
        return !traits_type::eq_int_type( t.rdbuf()->sputbackc(c),
                                          traits_type::eof() );
    }
};

template<>
struct read_device_impl<streambuf_tag> {
    template<typename T>
    static typename int_type_of<T>::type
    get(T& t)
    {   // gcc 2.95 needs namespace qualification for char_traits.
        typedef typename char_type_of<T>::type     char_type;
        typedef iostreams::char_traits<char_type>  traits_type;
        typename int_type_of<T>::type c;
        return !traits_type::is_eof(c = t.sbumpc()) ||
                detail::true_eof(t)
                    ?
                c : traits_type::would_block();
    }

    template<typename T>
    static std::streamsize
    read(T& t, typename char_type_of<T>::type* s, std::streamsize n)
    {
        std::streamsize amt;
        return (amt = t.sgetn(s, n)) != 0 ?
            amt :
            detail::true_eof(t) ?
                -1 :
                0;
    }

    template<typename T>
    static bool putback(T& t, typename char_type_of<T>::type c)
    {   // gcc 2.95 needs namespace qualification for char_traits.
        typedef typename char_type_of<T>::type     char_type;
        typedef iostreams::char_traits<char_type>  traits_type;
        return !traits_type::is_eof(t.sputbackc(c));
    }
};

template<>
struct read_device_impl<input> {
    template<typename T>
    static typename int_type_of<T>::type
    get(T& t)
    {   // gcc 2.95 needs namespace qualification for char_traits.
        typedef typename char_type_of<T>::type     char_type;
        typedef iostreams::char_traits<char_type>  traits_type;
        char_type c;
        std::streamsize amt;
        return (amt = t.read(&c, 1)) == 1 ?
            traits_type::to_int_type(c) :
            amt == -1 ?
                traits_type::eof() :
                traits_type::would_block();
    }

    template<typename T>
    static std::streamsize
    read(T& t, typename char_type_of<T>::type* s, std::streamsize n)
    { return t.read(s, n); }

    template<typename T>
    static bool putback(T& t, typename char_type_of<T>::type c)
    {   // T must be Peekable.
        return t.putback(c);
    }
};

//------------------Definition of read_filter_impl----------------------------//

template<typename T>
struct read_filter_impl
    : mpl::if_<
          detail::is_custom<T>,
          operations<T>,
          read_filter_impl<
              BOOST_DEDUCED_TYPENAME
              detail::dispatch<
                  T, multichar_tag, any_tag
              >::type
          >
      >::type
    { };

template<>
struct read_filter_impl<multichar_tag> {
    template<typename T, typename Source>
    static std::streamsize read
       (T& t, Source& src, typename char_type_of<T>::type* s, std::streamsize n)
    { return t.read(src, s, n); }
};

template<>
struct read_filter_impl<any_tag> {
    template<typename T, typename Source>
    static std::streamsize read
       (T& t, Source& src, typename char_type_of<T>::type* s, std::streamsize n)
    {   // gcc 2.95 needs namespace qualification for char_traits.
        typedef typename char_type_of<T>::type     char_type;
        typedef iostreams::char_traits<char_type>  traits_type;
        for (std::streamsize off = 0; off < n; ++off) {
            typename traits_type::int_type c = t.get(src);
            if (traits_type::is_eof(c))
                return check_eof(off);
            if (traits_type::would_block(c))
                return off;
            s[off] = traits_type::to_char_type(c);
        }
        return n;
    }
};

} // End namespace detail.

} } // End namespaces iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp>

#endif // #ifndef BOOST_IOSTREAMS_READ_HPP_INCLUDED
