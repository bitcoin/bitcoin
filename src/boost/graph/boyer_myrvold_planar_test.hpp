//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef __BOYER_MYRVOLD_PLANAR_TEST_HPP__
#define __BOYER_MYRVOLD_PLANAR_TEST_HPP__

#include <boost/graph/planar_detail/boyer_myrvold_impl.hpp>
#include <boost/parameter.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/bool.hpp>

namespace boost
{

struct no_kuratowski_subgraph_isolation
{
};
struct no_planar_embedding
{
};

namespace boyer_myrvold_params
{

    BOOST_PARAMETER_KEYWORD(tag, graph)
    BOOST_PARAMETER_KEYWORD(tag, embedding)
    BOOST_PARAMETER_KEYWORD(tag, kuratowski_subgraph)
    BOOST_PARAMETER_KEYWORD(tag, vertex_index_map)
    BOOST_PARAMETER_KEYWORD(tag, edge_index_map)

    typedef parameter::parameters< parameter::required< tag::graph >,
        tag::embedding, tag::kuratowski_subgraph, tag::vertex_index_map,
        tag::edge_index_map >
        boyer_myrvold_params_t;

    namespace core
    {

        template < typename ArgumentPack >
        bool dispatched_boyer_myrvold(
            ArgumentPack const& args, mpl::true_, mpl::true_)
        {
            // Dispatch for no planar embedding, no kuratowski subgraph
            // isolation

            typedef typename remove_const< typename parameter::value_type<
                ArgumentPack, tag::graph >::type >::type graph_t;

            typedef typename property_map< graph_t, vertex_index_t >::const_type
                vertex_default_index_map_t;

            typedef typename parameter::value_type< ArgumentPack,
                tag::vertex_index_map, vertex_default_index_map_t >::type
                vertex_index_map_t;

            graph_t const& g = args[graph];
            vertex_default_index_map_t v_d_map = get(vertex_index, g);
            vertex_index_map_t v_i_map = args[vertex_index_map | v_d_map];
            boyer_myrvold_impl< graph_t, vertex_index_map_t,
                graph::detail::no_old_handles, graph::detail::no_embedding >
                planarity_tester(g, v_i_map);

            return planarity_tester.is_planar() ? true : false;
        }

        template < typename ArgumentPack >
        bool dispatched_boyer_myrvold(
            ArgumentPack const& args, mpl::true_, mpl::false_)
        {
            // Dispatch for no planar embedding, kuratowski subgraph isolation
            typedef typename remove_const< typename parameter::value_type<
                ArgumentPack, tag::graph >::type >::type graph_t;

            typedef typename property_map< graph_t, vertex_index_t >::const_type
                vertex_default_index_map_t;

            typedef typename parameter::value_type< ArgumentPack,
                tag::vertex_index_map, vertex_default_index_map_t >::type
                vertex_index_map_t;

            typedef typename property_map< graph_t, edge_index_t >::const_type
                edge_default_index_map_t;

            typedef typename parameter::value_type< ArgumentPack,
                tag::edge_index_map, edge_default_index_map_t >::type
                edge_index_map_t;

            graph_t const& g = args[graph];
            vertex_default_index_map_t v_d_map = get(vertex_index, g);
            vertex_index_map_t v_i_map = args[vertex_index_map | v_d_map];
            edge_default_index_map_t e_d_map = get(edge_index, g);
            edge_index_map_t e_i_map = args[edge_index_map | e_d_map];
            boyer_myrvold_impl< graph_t, vertex_index_map_t,
                graph::detail::store_old_handles, graph::detail::no_embedding >
                planarity_tester(g, v_i_map);

            if (planarity_tester.is_planar())
                return true;
            else
            {
                planarity_tester.extract_kuratowski_subgraph(
                    args[kuratowski_subgraph], e_i_map);
                return false;
            }
        }

        template < typename ArgumentPack >
        bool dispatched_boyer_myrvold(
            ArgumentPack const& args, mpl::false_, mpl::true_)
        {
            // Dispatch for planar embedding, no kuratowski subgraph isolation
            typedef typename remove_const< typename parameter::value_type<
                ArgumentPack, tag::graph >::type >::type graph_t;

            typedef typename property_map< graph_t, vertex_index_t >::const_type
                vertex_default_index_map_t;

            typedef typename parameter::value_type< ArgumentPack,
                tag::vertex_index_map, vertex_default_index_map_t >::type
                vertex_index_map_t;

            graph_t const& g = args[graph];
            vertex_default_index_map_t v_d_map = get(vertex_index, g);
            vertex_index_map_t v_i_map = args[vertex_index_map | v_d_map];
            boyer_myrvold_impl< graph_t, vertex_index_map_t,
                graph::detail::no_old_handles,
#ifdef BOOST_GRAPH_PREFER_STD_LIB
                graph::detail::std_list
#else
                graph::detail::recursive_lazy_list
#endif
                >
                planarity_tester(g, v_i_map);

            if (planarity_tester.is_planar())
            {
                planarity_tester.make_edge_permutation(args[embedding]);
                return true;
            }
            else
                return false;
        }

        template < typename ArgumentPack >
        bool dispatched_boyer_myrvold(
            ArgumentPack const& args, mpl::false_, mpl::false_)
        {
            // Dispatch for planar embedding, kuratowski subgraph isolation
            typedef typename remove_const< typename parameter::value_type<
                ArgumentPack, tag::graph >::type >::type graph_t;

            typedef typename property_map< graph_t, vertex_index_t >::const_type
                vertex_default_index_map_t;

            typedef typename parameter::value_type< ArgumentPack,
                tag::vertex_index_map, vertex_default_index_map_t >::type
                vertex_index_map_t;

            typedef typename property_map< graph_t, edge_index_t >::const_type
                edge_default_index_map_t;

            typedef typename parameter::value_type< ArgumentPack,
                tag::edge_index_map, edge_default_index_map_t >::type
                edge_index_map_t;

            graph_t const& g = args[graph];
            vertex_default_index_map_t v_d_map = get(vertex_index, g);
            vertex_index_map_t v_i_map = args[vertex_index_map | v_d_map];
            edge_default_index_map_t e_d_map = get(edge_index, g);
            edge_index_map_t e_i_map = args[edge_index_map | e_d_map];
            boyer_myrvold_impl< graph_t, vertex_index_map_t,
                graph::detail::store_old_handles,
#ifdef BOOST_BGL_PREFER_STD_LIB
                graph::detail::std_list
#else
                graph::detail::recursive_lazy_list
#endif
                >
                planarity_tester(g, v_i_map);

            if (planarity_tester.is_planar())
            {
                planarity_tester.make_edge_permutation(args[embedding]);
                return true;
            }
            else
            {
                planarity_tester.extract_kuratowski_subgraph(
                    args[kuratowski_subgraph], e_i_map);
                return false;
            }
        }

        template < typename ArgumentPack >
        bool boyer_myrvold_planarity_test(ArgumentPack const& args)
        {

            typedef typename parameter::binding< ArgumentPack,
                tag::kuratowski_subgraph,
                const no_kuratowski_subgraph_isolation& >::type
                kuratowski_arg_t;

            typedef typename parameter::binding< ArgumentPack, tag::embedding,
                const no_planar_embedding& >::type embedding_arg_t;

            return dispatched_boyer_myrvold(args,
                boost::is_same< embedding_arg_t, const no_planar_embedding& >(),
                boost::is_same< kuratowski_arg_t,
                    const no_kuratowski_subgraph_isolation& >());
        }

    } // namespace core

} // namespace boyer_myrvold_params

template < typename A0 > bool boyer_myrvold_planarity_test(A0 const& arg0)
{
    return boyer_myrvold_params::core::boyer_myrvold_planarity_test(
        boyer_myrvold_params::boyer_myrvold_params_t()(arg0));
}

template < typename A0, typename A1 >
//  bool boyer_myrvold_planarity_test(A0 const& arg0, A1 const& arg1)
bool boyer_myrvold_planarity_test(A0 const& arg0, A1 const& arg1)
{
    return boyer_myrvold_params::core::boyer_myrvold_planarity_test(
        boyer_myrvold_params::boyer_myrvold_params_t()(arg0, arg1));
}

template < typename A0, typename A1, typename A2 >
bool boyer_myrvold_planarity_test(
    A0 const& arg0, A1 const& arg1, A2 const& arg2)
{
    return boyer_myrvold_params::core::boyer_myrvold_planarity_test(
        boyer_myrvold_params::boyer_myrvold_params_t()(arg0, arg1, arg2));
}

template < typename A0, typename A1, typename A2, typename A3 >
bool boyer_myrvold_planarity_test(
    A0 const& arg0, A1 const& arg1, A2 const& arg2, A3 const& arg3)
{
    return boyer_myrvold_params::core::boyer_myrvold_planarity_test(
        boyer_myrvold_params::boyer_myrvold_params_t()(arg0, arg1, arg2, arg3));
}

template < typename A0, typename A1, typename A2, typename A3, typename A4 >
bool boyer_myrvold_planarity_test(A0 const& arg0, A1 const& arg1,
    A2 const& arg2, A3 const& arg3, A4 const& arg4)
{
    return boyer_myrvold_params::core::boyer_myrvold_planarity_test(
        boyer_myrvold_params::boyer_myrvold_params_t()(
            arg0, arg1, arg2, arg3, arg4));
}

}

#endif //__BOYER_MYRVOLD_PLANAR_TEST_HPP__
