//-----------------------------------------------------------------------------
// boost variant/bad_visit.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2002-2003
// Eric Friedman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_BAD_VISIT_HPP
#define BOOST_VARIANT_BAD_VISIT_HPP

#include <boost/config.hpp>

#include <exception>

namespace boost {

//////////////////////////////////////////////////////////////////////////
// class bad_visit
//
// Exception thrown when a visitation attempt via apply_visitor fails due
// to invalid visited subtype or contents.
//
struct BOOST_SYMBOL_VISIBLE bad_visit
    : std::exception
{
public: // std::exception interface

    const char * what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE
    {
        return "boost::bad_visit: "
               "failed visitation using boost::apply_visitor";
    }

};

} // namespace boost

#endif // BOOST_VARIANT_BAD_VISIT_HPP
