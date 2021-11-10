//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_SUPPORT_AUXILIARY_ATTR_CAST_HPP
#define BOOST_SPIRIT_SUPPORT_AUXILIARY_ATTR_CAST_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/attributes.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/proto/traits.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // This one is the function that the user can call directly in order 
    // to create a customized attr_cast component
    template <typename Expr>
    typename enable_if<proto::is_expr<Expr>
      , stateful_tag_type<Expr, tag::attr_cast> >::type
    attr_cast(Expr const& expr)
    {
        return stateful_tag_type<Expr, tag::attr_cast>(expr);
    }

    template <typename Exposed, typename Expr>
    typename enable_if<proto::is_expr<Expr>
      , stateful_tag_type<Expr, tag::attr_cast, Exposed> >::type
    attr_cast(Expr const& expr)
    {
        return stateful_tag_type<Expr, tag::attr_cast, Exposed>(expr);
    }

    template <typename Exposed, typename Transformed, typename Expr>
    typename enable_if<proto::is_expr<Expr>
      , stateful_tag_type<Expr, tag::attr_cast, Exposed, Transformed> >::type
    attr_cast(Expr const& expr)
    {
        return stateful_tag_type<Expr, tag::attr_cast, Exposed, Transformed>(expr);
    }
}}

#endif
