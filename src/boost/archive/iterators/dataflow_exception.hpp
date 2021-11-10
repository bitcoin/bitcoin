#ifndef BOOST_ARCHIVE_ITERATORS_DATAFLOW_EXCEPTION_HPP
#define BOOST_ARCHIVE_ITERATORS_DATAFLOW_EXCEPTION_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// dataflow_exception.hpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>
#ifndef BOOST_NO_EXCEPTIONS
#include <exception>
#endif //BOOST_NO_EXCEPTIONS

#include <boost/assert.hpp>

namespace boost {
namespace archive {
namespace iterators {

//////////////////////////////////////////////////////////////////////
// exceptions thrown by dataflows
//
class dataflow_exception : public std::exception
{
public:
    typedef enum {
        invalid_6_bitcode,
        invalid_base64_character,
        invalid_xml_escape_sequence,
        comparison_not_permitted,
        invalid_conversion,
        other_exception
    } exception_code;
    exception_code code;

    dataflow_exception(exception_code c = other_exception) : code(c)
    {}

    const char *what( ) const throw( ) BOOST_OVERRIDE
    {
        const char *msg = "unknown exception code";
        switch(code){
        case invalid_6_bitcode:
            msg = "attempt to encode a value > 6 bits";
            break;
        case invalid_base64_character:
            msg = "attempt to decode a value not in base64 char set";
            break;
        case invalid_xml_escape_sequence:
            msg = "invalid xml escape_sequence";
            break;
        case comparison_not_permitted:
            msg = "cannot invoke iterator comparison now";
            break;
        case invalid_conversion:
            msg = "invalid multbyte/wide char conversion";
            break;
        default:
            BOOST_ASSERT(false);
            break;
        }
        return msg;
    }
};

} // namespace iterators
} // namespace archive
} // namespace boost

#endif //BOOST_ARCHIVE_ITERATORS_DATAFLOW_EXCEPTION_HPP
