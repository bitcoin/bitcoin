#ifndef BOOST_SERIALIZATION_STATIC_WARNING_HPP
#define BOOST_SERIALIZATION_STATIC_WARNING_HPP

//  (C) Copyright Robert Ramey 2003. Jonathan Turkanis 2004.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/static_assert for documentation.

/*
 Revision history:
   15 June  2003 - Initial version.
   31 March 2004 - improved diagnostic messages and portability
                   (Jonathan Turkanis)
   03 April 2004 - works on VC6 at class and namespace scope
                 - ported to DigitalMars
                 - static warnings disabled by default; when enabled,
                   uses pragmas to enable required compiler warnings
                   on MSVC, Intel, Metrowerks and Borland 5.x.
                   (Jonathan Turkanis)
   30 May 2004   - tweaked for msvc 7.1 and gcc 3.3
                 - static warnings ENabled by default; when enabled,
                   (Robert Ramey)
*/

#include <boost/config.hpp>

//
// Implementation
// Makes use of the following warnings:
//  1. GCC prior to 3.3: division by zero.
//  2. BCC 6.0 preview: unreferenced local variable.
//  3. DigitalMars: returning address of local automatic variable.
//  4. VC6: class previously seen as struct (as in 'boost/mpl/print.hpp')
//  5. All others: deletion of pointer to incomplete type.
//
// The trick is to find code which produces warnings containing the name of
// a structure or variable. Details, with same numbering as above:
// 1. static_warning_impl<B>::value is zero iff B is false, so diving an int
//    by this value generates a warning iff B is false.
// 2. static_warning_impl<B>::type has a constructor iff B is true, so an
//    unreferenced variable of this type generates a warning iff B is false.
// 3. static_warning_impl<B>::type overloads operator& to return a dynamically
//    allocated int pointer only is B is true, so  returning the address of an
//    automatic variable of this type generates a warning iff B is fasle.
// 4. static_warning_impl<B>::STATIC_WARNING is decalred as a struct iff B is
//    false.
// 5. static_warning_impl<B>::type is incomplete iff B is false, so deleting a
//    pointer to this type generates a warning iff B is false.
//

//------------------Enable selected warnings----------------------------------//

// Enable the warnings relied on by BOOST_STATIC_WARNING, where possible.

// 6. replaced implementation with one which depends solely on
//    mpl::print<>.  The previous one was found to fail for functions
//    under recent versions of gcc and intel compilers - Robert Ramey

#include <boost/mpl/bool.hpp>
#include <boost/mpl/print.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/bool_fwd.hpp>
#include <boost/static_assert.hpp>

namespace boost {
namespace serialization {

template<int L>
struct BOOST_SERIALIZATION_STATIC_WARNING_LINE{};

template<bool B, int L>
struct static_warning_test{
    typename boost::mpl::eval_if_c<
        B,
        boost::mpl::true_,
        typename boost::mpl::identity<
            boost::mpl::print<
                BOOST_SERIALIZATION_STATIC_WARNING_LINE<L>
            >
        >
    >::type type;
};

template<int i>
struct BOOST_SERIALIZATION_SS {};

} // serialization
} // boost

#define BOOST_SERIALIZATION_BSW(B, L) \
    typedef boost::serialization::BOOST_SERIALIZATION_SS< \
        sizeof( boost::serialization::static_warning_test< B, L > ) \
    > BOOST_JOIN(STATIC_WARNING_LINE, L) BOOST_ATTRIBUTE_UNUSED;
#define BOOST_STATIC_WARNING(B) BOOST_SERIALIZATION_BSW(B, __LINE__)

#endif // BOOST_SERIALIZATION_STATIC_WARNING_HPP
