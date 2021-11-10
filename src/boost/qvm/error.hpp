#ifndef BOOST_QVM_ERROR_HPP_INCLUDED
#define BOOST_QVM_ERROR_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <exception>

namespace boost { namespace qvm {

struct
error:
    std::exception
    {
    char const *
    what() const throw()
        {
        return "Boost QVM error";
        }

    ~error() throw()
        {
        }
    };

struct zero_determinant_error: error { };
struct zero_magnitude_error: error { };

} }

#endif
