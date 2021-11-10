/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ERROR_HANDLING_TYPEOF_HPP)
#define BOOST_SPIRIT_ERROR_HANDLING_TYPEOF_HPP

#include <boost/typeof/typeof.hpp>

#include <boost/spirit/home/classic/core/typeof.hpp>

#include <boost/spirit/home/classic/error_handling/exceptions_fwd.hpp>


#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()


// exceptions.hpp (has forward header)

BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::parser_error,2)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::assertive_parser,2)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::error_status,1)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::fallback_parser,3)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::guard,1)

BOOST_TYPEOF_REGISTER_TYPE(BOOST_SPIRIT_CLASSIC_NS::error_status<>)


#endif

