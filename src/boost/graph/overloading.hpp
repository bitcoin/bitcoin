// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

//
// This file contains helps that enable concept-based overloading
// within the Boost Graph Library.
//
#ifndef BOOST_GRAPH_OVERLOADING_HPP
#define BOOST_GRAPH_OVERLOADING_HPP

#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost
{
namespace graph
{
    namespace detail
    {

        struct no_parameter
        {
        };

    }
}
} // end namespace boost::graph::detail

#ifndef BOOST_NO_SFINAE

#define BOOST_GRAPH_ENABLE_IF_MODELS(Graph, Tag, Type)                    \
    typename enable_if_c<                                                 \
        (is_base_and_derived< Tag,                                        \
            typename graph_traits< Graph >::traversal_category >::value), \
        Type >::type

#define BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph, Tag)         \
    ,                                                         \
        BOOST_GRAPH_ENABLE_IF_MODELS(                         \
            Graph, Tag, ::boost::graph::detail::no_parameter) \
        = ::boost::graph::detail::no_parameter()

#else

#define BOOST_GRAPH_ENABLE_IF_MODELS(Graph, Tag, Type) Type
#define BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph, Tag)

#endif // no SFINAE support

#endif // BOOST_GRAPH_OVERLOADING_HPP
