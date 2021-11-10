/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_HAS_SEMANTIC_ACTION_SEP_20_2009_0626PM)
#define BOOST_SPIRIT_HAS_SEMANTIC_ACTION_SEP_20_2009_0626PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/bool.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit { namespace traits
{
    // finding out, whether a component contains a semantic action
    template <typename T, typename Enable = void>
    struct has_semantic_action
      : mpl::false_ {};

    template <typename Subject>
    struct unary_has_semantic_action 
      : has_semantic_action<Subject> {};

    template <typename Left, typename Right>
    struct binary_has_semantic_action 
      : mpl::or_<has_semantic_action<Left>, has_semantic_action<Right> > {};

    template <typename Elements>
    struct nary_has_semantic_action
      : mpl::not_<
            is_same<
                typename mpl::find_if<
                    Elements, has_semantic_action<mpl::_> 
                >::type
              , typename mpl::end<Elements>::type
            > 
        > {};
}}}

#endif
