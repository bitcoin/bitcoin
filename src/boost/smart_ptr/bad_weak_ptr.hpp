#ifndef BOOST_SMART_PTR_BAD_WEAK_PTR_HPP_INCLUDED
#define BOOST_SMART_PTR_BAD_WEAK_PTR_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  boost/smart_ptr/bad_weak_ptr.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/config.hpp>
#include <exception>

#ifdef BOOST_BORLANDC
# pragma warn -8026     // Functions with excep. spec. are not expanded inline
#endif

namespace boost
{

// The standard library that comes with Borland C++ 5.5.1, 5.6.4
// defines std::exception and its members as having C calling
// convention (-pc). When the definition of bad_weak_ptr
// is compiled with -ps, the compiler issues an error.
// Hence, the temporary #pragma option -pc below.

#if defined(BOOST_BORLANDC) && BOOST_BORLANDC <= 0x564
# pragma option push -pc
#endif

#if defined(BOOST_CLANG)
// Intel C++ on Mac defines __clang__ but doesn't support the pragma
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wweak-vtables"
#endif

class bad_weak_ptr: public std::exception
{
public:

    char const * what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE
    {
        return "tr1::bad_weak_ptr";
    }
};

#if defined(BOOST_CLANG)
# pragma clang diagnostic pop
#endif

#if defined(BOOST_BORLANDC) && BOOST_BORLANDC <= 0x564
# pragma option pop
#endif

} // namespace boost

#ifdef BOOST_BORLANDC
# pragma warn .8026     // Functions with excep. spec. are not expanded inline
#endif

#endif  // #ifndef BOOST_SMART_PTR_BAD_WEAK_PTR_HPP_INCLUDED
