/*=============================================================================
Copyright (c) 2001-2011 Joel de Guzman

Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_DETAIL_EXPECTATION_FAILURE_HPP
#define BOOST_SPIRIT_QI_DETAIL_EXPECTATION_FAILURE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/info.hpp>

#include <boost/config.hpp> // for BOOST_SYMBOL_VISIBLE
#include <stdexcept>

namespace boost { namespace spirit { namespace qi {
    template <typename Iterator>
    struct BOOST_SYMBOL_VISIBLE expectation_failure : std::runtime_error
    {
        expectation_failure(Iterator first_, Iterator last_, info const& what)
            : std::runtime_error("boost::spirit::qi::expectation_failure")
            , first(first_), last(last_), what_(what)
        {}
        ~expectation_failure() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE {}

        Iterator first;
        Iterator last;
        info what_;
    };
}}}

#endif
