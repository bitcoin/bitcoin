// Copyright Antony Polukhin, 2020-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/any for Documentation.

#ifndef BOOST_ANYS_BAD_ANY_CAST_HPP_INCLUDED
#define BOOST_ANYS_BAD_ANY_CAST_HPP_INCLUDED

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#ifndef BOOST_NO_RTTI
#include <typeinfo>
#endif

#include <stdexcept>

namespace boost {

class BOOST_SYMBOL_VISIBLE bad_any_cast :
#ifndef BOOST_NO_RTTI
    public std::bad_cast
#else
    public std::exception
#endif
{
public:
    const char * what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE
    {
        return "boost::bad_any_cast: "
               "failed conversion using boost::any_cast";
    }
};

} // namespace boost


#endif // #ifndef BOOST_ANYS_BAD_ANY_CAST_HPP_INCLUDED
