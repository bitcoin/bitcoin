#ifndef BOOST_QVM_TO_STRING_HPP_INCLUDED
#define BOOST_QVM_TO_STRING_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if __cplusplus >= 201103L

#include <string>

namespace boost { namespace qvm {

namespace
qvm_to_string_detail
    {
    template <class T>
    std::string
    to_string( T const & x )
        {
        return std::to_string(x);
        }
    }

} }

#else

#include <sstream>

namespace boost { namespace qvm {

namespace
qvm_to_string_detail
    {
    template <class T>
    std::string
    to_string( T const & x )
        {
        std::stringstream s;
        s << x;
        return s.str();
        }
    }

} }

#endif

#endif
