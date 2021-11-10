#ifndef BOOST_SERIALIZATION_CONFIG_HPP
#define BOOST_SERIALIZATION_CONFIG_HPP

//  config.hpp  ---------------------------------------------//

//  (c) Copyright Robert Ramey 2004
//  Use, modification, and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/serialization

//----------------------------------------------------------------------------//

// This header implements separate compilation features as described in
// http://www.boost.org/more/separate_compilation.html

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

// note: this version incorporates the related code into the the
// the same library as BOOST_ARCHIVE.  This could change some day in the
// future

// if BOOST_SERIALIZATION_DECL is defined undefine it now:
#ifdef BOOST_SERIALIZATION_DECL
    #undef BOOST_SERIALIZATION_DECL
#endif

// we need to import/export our code only if the user has specifically
// asked for it by defining either BOOST_ALL_DYN_LINK if they want all boost
// libraries to be dynamically linked, or BOOST_SERIALIZATION_DYN_LINK
// if they want just this one to be dynamically liked:
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_SERIALIZATION_DYN_LINK)
    #if !defined(BOOST_DYN_LINK)
        #define BOOST_DYN_LINK
    #endif
    // export if this is our own source, otherwise import:
    #if defined(BOOST_SERIALIZATION_SOURCE)
        #define BOOST_SERIALIZATION_DECL BOOST_SYMBOL_EXPORT
    #else
        #define BOOST_SERIALIZATION_DECL BOOST_SYMBOL_IMPORT
    #endif // defined(BOOST_SERIALIZATION_SOURCE)
#endif // defined(BOOST_ALL_DYN_LINK) || defined(BOOST_SERIALIZATION_DYN_LINK)

// if BOOST_SERIALIZATION_DECL isn't defined yet define it now:
#ifndef BOOST_SERIALIZATION_DECL
    #define BOOST_SERIALIZATION_DECL
#endif

//  enable automatic library variant selection  ------------------------------//

#if !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_SERIALIZATION_NO_LIB) \
&&  !defined(BOOST_ARCHIVE_SOURCE) && !defined(BOOST_WARCHIVE_SOURCE)  \
&&  !defined(BOOST_SERIALIZATION_SOURCE)
    //
    // Set the name of our library, this will get undef'ed by auto_link.hpp
    // once it's done with it:
    //
    #define BOOST_LIB_NAME boost_serialization
    //
    // If we're importing code from a dll, then tell auto_link.hpp about it:
    //
    #if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_SERIALIZATION_DYN_LINK)
    #  define BOOST_DYN_LINK
    #endif
    //
    // And include the header that does the work:
    //
    #include <boost/config/auto_link.hpp>

#endif

#endif // BOOST_SERIALIZATION_CONFIG_HPP
