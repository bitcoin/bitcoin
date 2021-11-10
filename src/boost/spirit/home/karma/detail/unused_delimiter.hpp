//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_UNUSED_DELIMITER_MAR_15_2009_0923PM)
#define BOOST_SPIRIT_KARMA_UNUSED_DELIMITER_MAR_15_2009_0923PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>

namespace boost { namespace spirit { namespace karma { namespace detail
{
    template <typename Delimiter>
    struct unused_delimiter : unused_type
    {
        unused_delimiter(Delimiter const& delim)
          : delimiter(delim) {}
        Delimiter const& delimiter;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(unused_delimiter& operator= (unused_delimiter const&))
    };

    // If a surrounding verbatim[] directive was specified, the current
    // delimiter is of the type unused_delimiter. In this case we 
    // re-activate the delimiter which was active before the verbatim[]
    // directive.
    template <typename Delimiter, typename Default>
    inline Delimiter const& 
    get_delimiter(unused_delimiter<Delimiter> const& u, Default const&)
    {
        return u.delimiter;
    }

    // If no surrounding verbatim[] directive was specified we activate
    // a single space as the delimiter to use.
    template <typename Delimiter, typename Default>
    inline Default const& 
    get_delimiter(Delimiter const&, Default const& d)
    {
        return d;
    }

}}}}

#endif
