// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Adapted from <boost/config/auto_link.hpp> and from
// http://www.boost.org/more/separate_compilation.html, by John Maddock.

#ifndef BOOST_IOSTREAMS_DETAIL_CONFIG_BZIP2_HPP_INCLUDED
#define BOOST_IOSTREAMS_DETAIL_CONFIG_BZIP2_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif              

#include <boost/config.hpp> // BOOST_STRINGIZE.

#if defined(BOOST_BZIP2_BINARY)
# if defined(BOOST_MSVC) || \
     defined(BOOST_BORLANDC) || \
     (defined(__MWERKS__) && defined(_WIN32) && (__MWERKS__ >= 0x3000)) || \
     (defined(__ICL) && defined(_MSC_EXTENSIONS)) \
     /**/

// Specify the name of the .lib file.
#  pragma comment(lib, BOOST_STRINGIZE(BOOST_BZIP2_BINARY))
# endif
#else 
# if !defined(BOOST_IOSTREAMS_SOURCE) && \
     !defined(BOOST_ALL_NO_LIB) && \
     !defined(BOOST_IOSTREAMS_NO_LIB) \
     /**/

// Set the name of our library, this will get undef'ed by auto_link.hpp
// once it's done with it.
#  define BOOST_LIB_NAME boost_bzip2

// If we're importing code from a dll, then tell auto_link.hpp about it.
#  if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_IOSTREAMS_DYN_LINK)
#   define BOOST_DYN_LINK
#  endif

// And include the header that does the work.
#  include <boost/config/auto_link.hpp>
# endif
#endif

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_CONFIG_BZIP2_HPP_INCLUDED
