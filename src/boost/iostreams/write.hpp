// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_WRITE_HPP_INCLUDED
#define BOOST_IOSTREAMS_WRITE_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>  // DEDUCED_TYPENAME, MSVC.
#include <boost/detail/workaround.hpp>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/detail/char_traits.hpp>
#include <boost/iostreams/detail/dispatch.hpp>
#include <boost/iostreams/detail/ios.hpp>  // streamsize.
#include <boost/iostreams/detail/streambuf.hpp>
#include <boost/iostreams/detail/wrap_unwrap.hpp>
#include <boost/iostreams/operations_fwd.hpp>
#include <boost/iostreams/traits.hpp>
#include <boost/mpl/if.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp>

namespace boost { namespace iostreams {

namespace detail {

template<typename T> 
struct write_device_impl;

template<typename T> 
struct write_filter_impl;

} // End namespace detail.

template<typename T>
bool put(T& t, typename char_type_of<T>::type c)
{ return detail::write_device_impl<T>::put(detail::unwrap(t), c); }

template<typename T>
inline std::streamsize write
    (T& t, const typename char_type_of<T>::type* s, std::streamsize n)
{ return detail::write_device_impl<T>::write(detail::unwrap(t), s, n); }

template<typename T, typename Sink>
inline std::streamsize
write( T& t, Sink& snk, const typename char_type_of<T>::type* s, 
       std::streamsize n )
{ return detail::write_filter_impl<T>::write(detail::unwrap(t), snk, s, n); }

namespace detail {

//------------------Definition of write_device_impl---------------------------//

template<typename T>
struct write_device_impl
    : mpl::if_<
          is_custom<T>,
          operations<T>,
          write_device_impl<
              BOOST_DEDUCED_TYPENAME
              dispatch<
                  T, ostream_tag, streambuf_tag, output
              >::type
          >
      >::type
    { };

template<>
struct write_device_impl<ostream_tag> {
    template<typename T>
    static bool put(T& t, typename char_type_of<T>::type c)
    {
        typedef typename char_type_of<T>::type          char_type;
        typedef BOOST_IOSTREAMS_CHAR_TRAITS(char_type)  traits_type;
        return !traits_type::eq_int_type( t.rdbuf()->sputc(c),
                                          traits_type::eof() );
    }

    template<typename T>
    static std::streamsize write
        (T& t, const typename char_type_of<T>::type* s, std::streamsize n)
    { return t.rdbuf()->sputn(s, n); }
};

template<>
struct write_device_impl<streambuf_tag> {
    template<typename T>
    static bool put(T& t, typename char_type_of<T>::type c)
    {
        typedef typename char_type_of<T>::type          char_type;
        typedef BOOST_IOSTREAMS_CHAR_TRAITS(char_type)  traits_type;
        return !traits_type::eq_int_type(t.sputc(c), traits_type::eof());
    }

    template<typename T>
    static std::streamsize write
        (T& t, const typename char_type_of<T>::type* s, std::streamsize n)
    { return t.sputn(s, n); }
};

template<>
struct write_device_impl<output> {
    template<typename T>
    static bool put(T& t, typename char_type_of<T>::type c)
    { return t.write(&c, 1) == 1; }

    template<typename T>
    static std::streamsize
    write(T& t, const typename char_type_of<T>::type* s, std::streamsize n)
    { return t.write(s, n); }
};

//------------------Definition of write_filter_impl---------------------------//

template<typename T>
struct write_filter_impl
    : mpl::if_<
          is_custom<T>,
          operations<T>,
          write_filter_impl<
              BOOST_DEDUCED_TYPENAME
              dispatch<
                  T, multichar_tag, any_tag
              >::type
          >
      >::type
    { };

template<>
struct write_filter_impl<multichar_tag> {
    template<typename T, typename Sink>
    static std::streamsize
    write( T& t, Sink& snk, const typename char_type_of<T>::type* s,
           std::streamsize n )
    { return t.write(snk, s, n); }
};

template<>
struct write_filter_impl<any_tag> {
    template<typename T, typename Sink>
    static std::streamsize
    write( T& t, Sink& snk, const typename char_type_of<T>::type* s,
           std::streamsize n )
    {
        for (std::streamsize off = 0; off < n; ++off)
            if (!t.put(snk, s[off]))
                return off;
        return n;
    }
};

} // End namespace detail.

} } // End namespaces iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp>

#endif // #ifndef BOOST_IOSTREAMS_WRITE_HPP_INCLUDED
