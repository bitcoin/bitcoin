// Copyright (C) 2009 Andrew Sutton

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_LABELED_GRAPH_TRAITS_HPP
#define BOOST_GRAPH_LABELED_GRAPH_TRAITS_HPP

#include <boost/graph/graph_mutability_traits.hpp>

namespace boost
{

// Extend the graph mutability traits (and metafunctions) to include options
// for labeled graphs.

// NOTE: the label_vertex tag denotes the fact that you can basically assign
// arbitrary labels to vertices without modifying the actual graph.

// TODO: We might also overlay the uniqueness/multiplicity of labels in this
// hierarchy also. For now, we just assumed that labels are unique.

struct label_vertex_tag
{
};
struct labeled_add_vertex_tag : virtual label_vertex_tag
{
};
struct labeled_add_vertex_property_tag : virtual labeled_add_vertex_tag
{
};
struct labeled_remove_vertex_tag
{
};
struct labeled_add_edge_tag : virtual label_vertex_tag
{
};
struct labeled_add_edge_property_tag : virtual labeled_add_edge_tag
{
};
struct labeled_remove_edge_tag
{
};

struct labeled_mutable_vertex_graph_tag : virtual labeled_add_vertex_tag,
                                          virtual labeled_remove_vertex_tag
{
};
struct labeled_mutable_vertex_property_graph_tag
: virtual labeled_add_vertex_property_tag,
  virtual labeled_remove_vertex_tag
{
};
struct labeled_mutable_edge_graph_tag : virtual labeled_add_edge_tag,
                                        virtual labeled_remove_edge_tag
{
};
struct labeled_mutable_edge_property_graph_tag
: virtual labeled_add_edge_property_tag,
  virtual labeled_remove_edge_tag
{
};

struct labeled_graph_tag : virtual label_vertex_tag
{
};
struct labeled_mutable_graph_tag : virtual labeled_mutable_vertex_graph_tag,
                                   virtual labeled_mutable_edge_graph_tag
{
};
struct labeled_mutable_property_graph_tag
: virtual labeled_mutable_vertex_property_graph_tag,
  virtual labeled_mutable_edge_property_graph_tag
{
};
struct labeled_add_only_property_graph_tag
: virtual labeled_add_vertex_property_tag,
  virtual labeled_mutable_edge_property_graph_tag
{
};

// Metafunctions

template < typename Graph >
struct graph_has_add_vertex_by_label
: mpl::bool_<
      is_convertible< typename graph_mutability_traits< Graph >::category,
          labeled_add_vertex_tag >::value >
{
};

template < typename Graph >
struct graph_has_add_vertex_by_label_with_property
: mpl::bool_<
      is_convertible< typename graph_mutability_traits< Graph >::category,
          labeled_add_vertex_property_tag >::value >
{
};

template < typename Graph >
struct graph_has_remove_vertex_by_label
: mpl::bool_<
      is_convertible< typename graph_mutability_traits< Graph >::category,
          labeled_remove_vertex_tag >::value >
{
};

template < typename Graph >
struct graph_has_add_edge_by_label
: mpl::bool_<
      is_convertible< typename graph_mutability_traits< Graph >::category,
          labeled_add_edge_tag >::value >
{
};

template < typename Graph >
struct graph_has_add_edge_by_label_with_property
: mpl::bool_<
      is_convertible< typename graph_mutability_traits< Graph >::category,
          labeled_add_edge_property_tag >::value >
{
};

template < typename Graph >
struct graph_has_remove_edge_by_label
: mpl::bool_<
      is_convertible< typename graph_mutability_traits< Graph >::category,
          labeled_remove_edge_tag >::value >
{
};

template < typename Graph >
struct is_labeled_mutable_vertex_graph
: mpl::and_< graph_has_add_vertex_by_label< Graph >,
      graph_has_remove_vertex_by_label< Graph > >
{
};

template < typename Graph >
struct is_labeled_mutable_vertex_property_graph
: mpl::and_< graph_has_add_vertex_by_label< Graph >,
      graph_has_remove_vertex_by_label< Graph > >
{
};

template < typename Graph >
struct is_labeled_mutable_edge_graph
: mpl::and_< graph_has_add_edge_by_label< Graph >,
      graph_has_remove_edge_by_label< Graph > >
{
};

template < typename Graph >
struct is_labeled_mutable_edge_property_graph
: mpl::and_< graph_has_add_edge_by_label< Graph >,
      graph_has_remove_edge_by_label< Graph > >
{
};

template < typename Graph >
struct is_labeled_mutable_graph
: mpl::and_< is_labeled_mutable_vertex_graph< Graph >,
      is_labeled_mutable_edge_graph< Graph > >
{
};

template < typename Graph >
struct is_labeled_mutable_property_graph
: mpl::and_< is_labeled_mutable_vertex_property_graph< Graph >,
      is_labeled_mutable_edge_property_graph< Graph > >
{
};

template < typename Graph >
struct is_labeled_add_only_property_graph
: mpl::bool_<
      is_convertible< typename graph_mutability_traits< Graph >::category,
          labeled_add_only_property_graph_tag >::value >
{
};

template < typename Graph >
struct is_labeled_graph
: mpl::bool_<
      is_convertible< typename graph_mutability_traits< Graph >::category,
          label_vertex_tag >::value >
{
};

template < typename > struct graph_mutability_traits;

namespace graph_detail
{
    // The determine mutability metafunction computes a labeled mutability tag
    // based on the mutability of the given graph type. This is used by the
    // graph_mutability_traits specialization below.
    template < typename Graph > struct determine_mutability
    {
        typedef typename mpl::if_< is_add_only_property_graph< Graph >,
            labeled_add_only_property_graph_tag,
            typename mpl::if_< is_mutable_property_graph< Graph >,
                labeled_mutable_property_graph_tag,
                typename mpl::if_< is_mutable_graph< Graph >,
                    labeled_mutable_graph_tag,
                    typename mpl::if_< is_mutable_edge_graph< Graph >,
                        labeled_graph_tag,
                        typename graph_mutability_traits< Graph >::category >::
                        type >::type >::type >::type type;
    };
} // namespace graph_detail

#define LABELED_GRAPH_PARAMS typename G, typename L, typename S
#define LABELED_GRAPH labeled_graph< G, L, S >

// Specialize mutability traits for the labeled graph.
// This specialization depends on the mutability of the underlying graph type.
// If the underlying graph is fully mutable, this is also fully mutable.
// Otherwise, it's different.
template < LABELED_GRAPH_PARAMS >
struct graph_mutability_traits< LABELED_GRAPH >
{
    typedef typename graph_detail::determine_mutability<
        typename LABELED_GRAPH::graph_type >::type category;
};

#undef LABELED_GRAPH_PARAMS
#undef LABELED_GRAPH

} // namespace boost

#endif
