#ifndef BOOST_ARCHIVE_ITERATORS_XML_UNESCAPE_EXCEPTION_HPP
#define BOOST_ARCHIVE_ITERATORS_XML_UNESCAPE_EXCEPTION_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// xml_unescape_exception.hpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>
#ifndef BOOST_NO_EXCEPTIONS
#include <exception>

#include <boost/assert.hpp>

namespace boost {
namespace archive {
namespace iterators {

//////////////////////////////////////////////////////////////////////
// exceptions thrown by xml_unescapes
//
class xml_unescape_exception : public std::exception
{
public:
    xml_unescape_exception()
    {}

    virtual const char *what( ) const throw( )
    {
        return "xml contained un-recognized escape code";
    }
};

} // namespace iterators
} // namespace archive
} // namespace boost

#endif //BOOST_NO_EXCEPTIONS
#endif //BOOST_ARCHIVE_ITERATORS_XML_UNESCAPE_EXCEPTION_HPP
