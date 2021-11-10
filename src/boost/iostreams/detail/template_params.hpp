// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_DETAIL_TEMPLATE_PARAMS_HPP_INCLUDED

#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>

#define BOOST_IOSTREAMS_TEMPLATE_PARAMS(arity, param) \
    BOOST_PP_EXPR_IF(arity, template<) \
    BOOST_PP_ENUM_PARAMS(arity, typename param) \
    BOOST_PP_EXPR_IF(arity, >) \
    /**/

#define BOOST_IOSTREAMS_TEMPLATE_ARGS(arity, param) \
    BOOST_PP_EXPR_IF(arity, <) \
    BOOST_PP_ENUM_PARAMS(arity, param) \
    BOOST_PP_EXPR_IF(arity, >) \
    /**/

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_BUFFERS_HPP_INCLUDED
