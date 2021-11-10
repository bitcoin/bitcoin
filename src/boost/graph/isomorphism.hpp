// Copyright (C) 2001 Jeremy Siek, Douglas Gregor, Brian Osman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_GRAPH_ISOMORPHISM_HPP
#define BOOST_GRAPH_ISOMORPHISM_HPP

#include <utility>
#include <vector>
#include <iterator>
#include <algorithm>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/detail/algorithm.hpp>
#include <boost/pending/indirect_cmp.hpp> // for make_indirect_pmap
#include <boost/concept/assert.hpp>

#ifndef BOOST_GRAPH_ITERATION_MACROS_HPP
#define BOOST_ISO_INCLUDED_ITER_MACROS // local macro, see bottom of file
#include <boost/graph/iteration_macros.hpp>
#endif

namespace boost
{

namespace detail
{

    template < typename Graph1, typename Graph2, typename IsoMapping,
        typename Invariant1, typename Invariant2, typename IndexMap1,
        typename IndexMap2 >
    class isomorphism_algo
    {
        typedef typename graph_traits< Graph1 >::vertex_descriptor vertex1_t;
        typedef typename graph_traits< Graph2 >::vertex_descriptor vertex2_t;
        typedef typename graph_traits< Graph1 >::edge_descriptor edge1_t;
        typedef typename graph_traits< Graph1 >::vertices_size_type size_type;
        typedef typename Invariant1::result_type invar1_value;
        typedef typename Invariant2::result_type invar2_value;

        const Graph1& G1;
        const Graph2& G2;
        IsoMapping f;
        Invariant1 invariant1;
        Invariant2 invariant2;
        std::size_t max_invariant;
        IndexMap1 index_map1;
        IndexMap2 index_map2;

        std::vector< vertex1_t > dfs_vertices;
        typedef typename std::vector< vertex1_t >::iterator vertex_iter;
        std::vector< int > dfs_num_vec;
        typedef safe_iterator_property_map<
            typename std::vector< int >::iterator, IndexMap1
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
            ,
            int, int&
#endif /* BOOST_NO_STD_ITERATOR_TRAITS */
            >
            DFSNumMap;
        DFSNumMap dfs_num;
        std::vector< edge1_t > ordered_edges;
        typedef typename std::vector< edge1_t >::iterator edge_iter;

        std::vector< char > in_S_vec;
        typedef safe_iterator_property_map<
            typename std::vector< char >::iterator, IndexMap2
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
            ,
            char, char&
#endif /* BOOST_NO_STD_ITERATOR_TRAITS */
            >
            InSMap;
        InSMap in_S;

        int num_edges_on_k;

        friend struct compare_multiplicity;
        struct compare_multiplicity
        {
            compare_multiplicity(Invariant1 invariant1, size_type* multiplicity)
            : invariant1(invariant1), multiplicity(multiplicity)
            {
            }
            bool operator()(const vertex1_t& x, const vertex1_t& y) const
            {
                return multiplicity[invariant1(x)]
                    < multiplicity[invariant1(y)];
            }
            Invariant1 invariant1;
            size_type* multiplicity;
        };

        struct record_dfs_order : default_dfs_visitor
        {
            record_dfs_order(
                std::vector< vertex1_t >& v, std::vector< edge1_t >& e)
            : vertices(v), edges(e)
            {
            }

            void discover_vertex(vertex1_t v, const Graph1&) const
            {
                vertices.push_back(v);
            }
            void examine_edge(edge1_t e, const Graph1&) const
            {
                edges.push_back(e);
            }
            std::vector< vertex1_t >& vertices;
            std::vector< edge1_t >& edges;
        };

        struct edge_cmp
        {
            edge_cmp(const Graph1& G1, DFSNumMap dfs_num)
            : G1(G1), dfs_num(dfs_num)
            {
            }
            bool operator()(const edge1_t& e1, const edge1_t& e2) const
            {
                using namespace std;
                int u1 = dfs_num[source(e1, G1)], v1 = dfs_num[target(e1, G1)];
                int u2 = dfs_num[source(e2, G1)], v2 = dfs_num[target(e2, G1)];
                int m1 = (max)(u1, v1);
                int m2 = (max)(u2, v2);
                // lexicographical comparison
                return std::make_pair(m1, std::make_pair(u1, v1))
                    < std::make_pair(m2, std::make_pair(u2, v2));
            }
            const Graph1& G1;
            DFSNumMap dfs_num;
        };

    public:
        isomorphism_algo(const Graph1& G1, const Graph2& G2, IsoMapping f,
            Invariant1 invariant1, Invariant2 invariant2,
            std::size_t max_invariant, IndexMap1 index_map1,
            IndexMap2 index_map2)
        : G1(G1)
        , G2(G2)
        , f(f)
        , invariant1(invariant1)
        , invariant2(invariant2)
        , max_invariant(max_invariant)
        , index_map1(index_map1)
        , index_map2(index_map2)
        {
            in_S_vec.resize(num_vertices(G1));
            in_S = make_safe_iterator_property_map(
                in_S_vec.begin(), in_S_vec.size(), index_map2
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
                ,
                in_S_vec.front()
#endif /* BOOST_NO_STD_ITERATOR_TRAITS */
            );
        }

        bool test_isomorphism()
        {
            // reset isomapping
            BGL_FORALL_VERTICES_T(v, G1, Graph1)
            f[v] = graph_traits< Graph2 >::null_vertex();

            {
                std::vector< invar1_value > invar1_array;
                BGL_FORALL_VERTICES_T(v, G1, Graph1)
                invar1_array.push_back(invariant1(v));
                sort(invar1_array);

                std::vector< invar2_value > invar2_array;
                BGL_FORALL_VERTICES_T(v, G2, Graph2)
                invar2_array.push_back(invariant2(v));
                sort(invar2_array);
                if (!equal(invar1_array, invar2_array))
                    return false;
            }

            std::vector< vertex1_t > V_mult;
            BGL_FORALL_VERTICES_T(v, G1, Graph1)
            V_mult.push_back(v);
            {
                std::vector< size_type > multiplicity(max_invariant, 0);
                BGL_FORALL_VERTICES_T(v, G1, Graph1)
                ++multiplicity.at(invariant1(v));
                sort(
                    V_mult, compare_multiplicity(invariant1, &multiplicity[0]));
            }

            std::vector< default_color_type > color_vec(num_vertices(G1));
            safe_iterator_property_map<
                std::vector< default_color_type >::iterator, IndexMap1
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
                ,
                default_color_type, default_color_type&
#endif /* BOOST_NO_STD_ITERATOR_TRAITS */
                >
                color_map(color_vec.begin(), color_vec.size(), index_map1);
            record_dfs_order dfs_visitor(dfs_vertices, ordered_edges);
            typedef color_traits< default_color_type > Color;
            for (vertex_iter u = V_mult.begin(); u != V_mult.end(); ++u)
            {
                if (color_map[*u] == Color::white())
                {
                    dfs_visitor.start_vertex(*u, G1);
                    depth_first_visit(G1, *u, dfs_visitor, color_map);
                }
            }
            // Create the dfs_num array and dfs_num_map
            dfs_num_vec.resize(num_vertices(G1));
            dfs_num = make_safe_iterator_property_map(
                dfs_num_vec.begin(), dfs_num_vec.size(), index_map1
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
                ,
                dfs_num_vec.front()
#endif /* BOOST_NO_STD_ITERATOR_TRAITS */
            );
            size_type n = 0;
            for (vertex_iter v = dfs_vertices.begin(); v != dfs_vertices.end();
                 ++v)
                dfs_num[*v] = n++;

            sort(ordered_edges, edge_cmp(G1, dfs_num));

            int dfs_num_k = -1;
            return this->match(ordered_edges.begin(), dfs_num_k);
        }

    private:
        struct match_continuation
        {
            enum
            {
                pos_G2_vertex_loop,
                pos_fi_adj_loop,
                pos_dfs_num
            } position;
            typedef typename graph_traits< Graph2 >::vertex_iterator
                vertex_iterator;
            std::pair< vertex_iterator, vertex_iterator > G2_verts;
            typedef typename graph_traits< Graph2 >::adjacency_iterator
                adjacency_iterator;
            std::pair< adjacency_iterator, adjacency_iterator > fi_adj;
            edge_iter iter;
            int dfs_num_k;
        };

        bool match(edge_iter iter, int dfs_num_k)
        {
            std::vector< match_continuation > k;
            typedef typename graph_traits< Graph2 >::vertex_iterator
                vertex_iterator;
            std::pair< vertex_iterator, vertex_iterator > G2_verts(
                vertices(G2));
            typedef typename graph_traits< Graph2 >::adjacency_iterator
                adjacency_iterator;
            std::pair< adjacency_iterator, adjacency_iterator > fi_adj;
            vertex1_t i, j;

        recur:
            if (iter != ordered_edges.end())
            {
                i = source(*iter, G1);
                j = target(*iter, G1);
                if (dfs_num[i] > dfs_num_k)
                {
                    G2_verts = vertices(G2);
                    while (G2_verts.first != G2_verts.second)
                    {
                        {
                            vertex2_t u = *G2_verts.first;
                            vertex1_t kp1 = dfs_vertices[dfs_num_k + 1];
                            if (invariant1(kp1) == invariant2(u)
                                && in_S[u] == false)
                            {
                                {
                                    f[kp1] = u;
                                    in_S[u] = true;
                                    num_edges_on_k = 0;

                                    match_continuation new_k;
                                    new_k.position = match_continuation::
                                        pos_G2_vertex_loop;
                                    new_k.G2_verts = G2_verts;
                                    new_k.iter = iter;
                                    new_k.dfs_num_k = dfs_num_k;
                                    k.push_back(new_k);
                                    ++dfs_num_k;
                                    goto recur;
                                }
                            }
                        }
                    G2_loop_k:
                        ++G2_verts.first;
                    }
                }
                else if (dfs_num[j] > dfs_num_k)
                {
                    {
                        vertex1_t vk = dfs_vertices[dfs_num_k];
                        num_edges_on_k -= count_if(adjacent_vertices(f[vk], G2),
                            make_indirect_pmap(in_S));

                        for (int jj = 0; jj < dfs_num_k; ++jj)
                        {
                            vertex1_t j = dfs_vertices[jj];
                            num_edges_on_k
                                -= count(adjacent_vertices(f[j], G2), f[vk]);
                        }
                    }

                    if (num_edges_on_k != 0)
                        goto return_point_false;
                    fi_adj = adjacent_vertices(f[i], G2);
                    while (fi_adj.first != fi_adj.second)
                    {
                        {
                            vertex2_t v = *fi_adj.first;
                            if (invariant2(v) == invariant1(j)
                                && in_S[v] == false)
                            {
                                f[j] = v;
                                in_S[v] = true;
                                num_edges_on_k = 1;
                                BOOST_USING_STD_MAX();
                                int next_k
                                    = max BOOST_PREVENT_MACRO_SUBSTITUTION(
                                        dfs_num_k,
                                        max BOOST_PREVENT_MACRO_SUBSTITUTION(
                                            dfs_num[i], dfs_num[j]));
                                match_continuation new_k;
                                new_k.position
                                    = match_continuation::pos_fi_adj_loop;
                                new_k.fi_adj = fi_adj;
                                new_k.iter = iter;
                                new_k.dfs_num_k = dfs_num_k;
                                ++iter;
                                dfs_num_k = next_k;
                                k.push_back(new_k);
                                goto recur;
                            }
                        }
                    fi_adj_loop_k:
                        ++fi_adj.first;
                    }
                }
                else
                {
                    if (container_contains(adjacent_vertices(f[i], G2), f[j]))
                    {
                        ++num_edges_on_k;
                        match_continuation new_k;
                        new_k.position = match_continuation::pos_dfs_num;
                        k.push_back(new_k);
                        ++iter;
                        goto recur;
                    }
                }
            }
            else
                goto return_point_true;
            goto return_point_false;

            {
            return_point_true:
                return true;

            return_point_false:
                if (k.empty())
                    return false;
                const match_continuation& this_k = k.back();
                switch (this_k.position)
                {
                case match_continuation::pos_G2_vertex_loop:
                {
                    G2_verts = this_k.G2_verts;
                    iter = this_k.iter;
                    dfs_num_k = this_k.dfs_num_k;
                    k.pop_back();
                    in_S[*G2_verts.first] = false;
                    i = source(*iter, G1);
                    j = target(*iter, G1);
                    goto G2_loop_k;
                }
                case match_continuation::pos_fi_adj_loop:
                {
                    fi_adj = this_k.fi_adj;
                    iter = this_k.iter;
                    dfs_num_k = this_k.dfs_num_k;
                    k.pop_back();
                    in_S[*fi_adj.first] = false;
                    i = source(*iter, G1);
                    j = target(*iter, G1);
                    goto fi_adj_loop_k;
                }
                case match_continuation::pos_dfs_num:
                {
                    k.pop_back();
                    goto return_point_false;
                }
                default:
                {
                    BOOST_ASSERT(!"Bad position");
#ifdef UNDER_CE
                    exit(-1);
#else
                    abort();
#endif
                }
                }
            }
        }
    };

    template < typename Graph, typename InDegreeMap >
    void compute_in_degree(const Graph& g, InDegreeMap in_degree_map)
    {
        BGL_FORALL_VERTICES_T(v, g, Graph)
        put(in_degree_map, v, 0);

        BGL_FORALL_VERTICES_T(u, g, Graph)
        BGL_FORALL_ADJ_T(u, v, g, Graph)
        put(in_degree_map, v, get(in_degree_map, v) + 1);
    }

} // namespace detail

template < typename InDegreeMap, typename Graph > class degree_vertex_invariant
{
    typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
    typedef typename graph_traits< Graph >::degree_size_type size_type;

public:
    typedef vertex_t argument_type;
    typedef size_type result_type;

    degree_vertex_invariant(const InDegreeMap& in_degree_map, const Graph& g)
    : m_in_degree_map(in_degree_map)
    , m_max_vertex_in_degree(0)
    , m_max_vertex_out_degree(0)
    , m_g(g)
    {
        BGL_FORALL_VERTICES_T(v, g, Graph)
        {
            m_max_vertex_in_degree
                = (std::max)(m_max_vertex_in_degree, get(m_in_degree_map, v));
            m_max_vertex_out_degree
                = (std::max)(m_max_vertex_out_degree, out_degree(v, g));
        }
    }

    size_type operator()(vertex_t v) const
    {
        return (m_max_vertex_in_degree + 1) * out_degree(v, m_g)
            + get(m_in_degree_map, v);
    }
    // The largest possible vertex invariant number
    size_type max BOOST_PREVENT_MACRO_SUBSTITUTION() const
    {
        return (m_max_vertex_in_degree + 1) * (m_max_vertex_out_degree + 1);
    }

private:
    InDegreeMap m_in_degree_map;
    size_type m_max_vertex_in_degree;
    size_type m_max_vertex_out_degree;
    const Graph& m_g;
};

// Count actual number of vertices, even in filtered graphs.
template < typename Graph > size_t count_vertices(const Graph& g)
{
    size_t n = 0;
    BGL_FORALL_VERTICES_T(v, g, Graph)
    {
        (void)v;
        ++n;
    }
    return n;
}

template < typename Graph1, typename Graph2, typename IsoMapping,
    typename Invariant1, typename Invariant2, typename IndexMap1,
    typename IndexMap2 >
bool isomorphism(const Graph1& G1, const Graph2& G2, IsoMapping f,
    Invariant1 invariant1, Invariant2 invariant2, std::size_t max_invariant,
    IndexMap1 index_map1, IndexMap2 index_map2)

{
    // Graph requirements
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph1 >));
    BOOST_CONCEPT_ASSERT((EdgeListGraphConcept< Graph1 >));
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph2 >));
    // BOOST_CONCEPT_ASSERT(( BidirectionalGraphConcept<Graph2> ));

    typedef typename graph_traits< Graph1 >::vertex_descriptor vertex1_t;
    typedef typename graph_traits< Graph2 >::vertex_descriptor vertex2_t;
    typedef typename graph_traits< Graph1 >::vertices_size_type size_type;

    // Vertex invariant requirement
    BOOST_CONCEPT_ASSERT(
        (AdaptableUnaryFunctionConcept< Invariant1, size_type, vertex1_t >));
    BOOST_CONCEPT_ASSERT(
        (AdaptableUnaryFunctionConcept< Invariant2, size_type, vertex2_t >));

    // Property map requirements
    BOOST_CONCEPT_ASSERT(
        (ReadWritePropertyMapConcept< IsoMapping, vertex1_t >));
    typedef typename property_traits< IsoMapping >::value_type IsoMappingValue;
    BOOST_STATIC_ASSERT((is_convertible< IsoMappingValue, vertex2_t >::value));

    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< IndexMap1, vertex1_t >));
    typedef typename property_traits< IndexMap1 >::value_type IndexMap1Value;
    BOOST_STATIC_ASSERT((is_convertible< IndexMap1Value, size_type >::value));

    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< IndexMap2, vertex2_t >));
    typedef typename property_traits< IndexMap2 >::value_type IndexMap2Value;
    BOOST_STATIC_ASSERT((is_convertible< IndexMap2Value, size_type >::value));

    if (count_vertices(G1) != count_vertices(G2))
        return false;
    if (count_vertices(G1) == 0 && count_vertices(G2) == 0)
        return true;

    detail::isomorphism_algo< Graph1, Graph2, IsoMapping, Invariant1,
        Invariant2, IndexMap1, IndexMap2 >
        algo(G1, G2, f, invariant1, invariant2, max_invariant, index_map1,
            index_map2);
    return algo.test_isomorphism();
}

namespace detail
{

    template < typename Graph1, typename Graph2, typename IsoMapping,
        typename IndexMap1, typename IndexMap2, typename P, typename T,
        typename R >
    bool isomorphism_impl(const Graph1& G1, const Graph2& G2, IsoMapping f,
        IndexMap1 index_map1, IndexMap2 index_map2,
        const bgl_named_params< P, T, R >& params)
    {
        std::vector< std::size_t > in_degree1_vec(num_vertices(G1));
        typedef safe_iterator_property_map<
            std::vector< std::size_t >::iterator, IndexMap1
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
            ,
            std::size_t, std::size_t&
#endif /* BOOST_NO_STD_ITERATOR_TRAITS */
            >
            InDeg1;
        InDeg1 in_degree1(
            in_degree1_vec.begin(), in_degree1_vec.size(), index_map1);
        compute_in_degree(G1, in_degree1);

        std::vector< std::size_t > in_degree2_vec(num_vertices(G2));
        typedef safe_iterator_property_map<
            std::vector< std::size_t >::iterator, IndexMap2
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
            ,
            std::size_t, std::size_t&
#endif /* BOOST_NO_STD_ITERATOR_TRAITS */
            >
            InDeg2;
        InDeg2 in_degree2(
            in_degree2_vec.begin(), in_degree2_vec.size(), index_map2);
        compute_in_degree(G2, in_degree2);

        degree_vertex_invariant< InDeg1, Graph1 > invariant1(in_degree1, G1);
        degree_vertex_invariant< InDeg2, Graph2 > invariant2(in_degree2, G2);

        return isomorphism(G1, G2, f,
            choose_param(get_param(params, vertex_invariant1_t()), invariant1),
            choose_param(get_param(params, vertex_invariant2_t()), invariant2),
            choose_param(get_param(params, vertex_max_invariant_t()),
                (invariant2.max)()),
            index_map1, index_map2);
    }

    template < typename G, typename Index > struct make_degree_invariant
    {
        const G& g;
        const Index& index;
        make_degree_invariant(const G& g, const Index& index)
        : g(g), index(index)
        {
        }
        typedef typename boost::graph_traits< G >::degree_size_type
            degree_size_type;
        typedef shared_array_property_map< degree_size_type, Index >
            prop_map_type;
        typedef degree_vertex_invariant< prop_map_type, G > result_type;
        result_type operator()() const
        {
            prop_map_type pm = make_shared_array_property_map(
                num_vertices(g), degree_size_type(), index);
            compute_in_degree(g, pm);
            return result_type(pm, g);
        }
    };

} // namespace detail

namespace graph
{
    namespace detail
    {
        template < typename Graph1, typename Graph2 > struct isomorphism_impl
        {
            typedef bool result_type;
            typedef result_type type;
            template < typename ArgPack >
            bool operator()(const Graph1& g1, const Graph2& g2,
                const ArgPack& arg_pack) const
            {
                using namespace boost::graph::keywords;
                typedef typename boost::detail::override_const_property_result<
                    ArgPack, tag::vertex_index1_map, boost::vertex_index_t,
                    Graph1 >::type index1_map_type;
                typedef typename boost::detail::override_const_property_result<
                    ArgPack, tag::vertex_index2_map, boost::vertex_index_t,
                    Graph2 >::type index2_map_type;
                index1_map_type index1_map
                    = boost::detail::override_const_property(
                        arg_pack, _vertex_index1_map, g1, boost::vertex_index);
                index2_map_type index2_map
                    = boost::detail::override_const_property(
                        arg_pack, _vertex_index2_map, g2, boost::vertex_index);
                typedef typename graph_traits< Graph2 >::vertex_descriptor
                    vertex2_t;
                typename std::vector< vertex2_t >::size_type n
                    = (typename std::vector< vertex2_t >::size_type)
                        num_vertices(g1);
                std::vector< vertex2_t > f(n);
                typename boost::parameter::lazy_binding< ArgPack,
                    tag::vertex_invariant1,
                    boost::detail::make_degree_invariant< Graph1,
                        index1_map_type > >::type invariant1
                    = arg_pack[_vertex_invariant1
                        || boost::detail::make_degree_invariant< Graph1,
                            index1_map_type >(g1, index1_map)];
                typename boost::parameter::lazy_binding< ArgPack,
                    tag::vertex_invariant2,
                    boost::detail::make_degree_invariant< Graph2,
                        index2_map_type > >::type invariant2
                    = arg_pack[_vertex_invariant2
                        || boost::detail::make_degree_invariant< Graph2,
                            index2_map_type >(g2, index2_map)];
                return boost::isomorphism(g1, g2,
                    choose_param(
                        arg_pack[_isomorphism_map | boost::param_not_found()],
                        make_shared_array_property_map(
                            num_vertices(g1), vertex2_t(), index1_map)),
                    invariant1, invariant2,
                    arg_pack[_vertex_max_invariant | (invariant2.max)()],
                    index1_map, index2_map);
            }
        };
    }
    BOOST_GRAPH_MAKE_FORWARDING_FUNCTION(isomorphism, 2, 6)
}

// Named parameter interface
BOOST_GRAPH_MAKE_OLD_STYLE_PARAMETER_FUNCTION(isomorphism, 2)

// Verify that the given mapping iso_map from the vertices of g1 to the
// vertices of g2 describes an isomorphism.
// Note: this could be made much faster by specializing based on the graph
// concepts modeled, but since we're verifying an O(n^(lg n)) algorithm,
// O(n^4) won't hurt us.
template < typename Graph1, typename Graph2, typename IsoMap >
inline bool verify_isomorphism(
    const Graph1& g1, const Graph2& g2, IsoMap iso_map)
{
#if 0
    // problematic for filtered_graph!
    if (num_vertices(g1) != num_vertices(g2) || num_edges(g1) != num_edges(g2))
      return false;
#endif

    BGL_FORALL_EDGES_T(e1, g1, Graph1)
    {
        bool found_edge = false;
        BGL_FORALL_EDGES_T(e2, g2, Graph2)
        {
            if (source(e2, g2) == get(iso_map, source(e1, g1))
                && target(e2, g2) == get(iso_map, target(e1, g1)))
            {
                found_edge = true;
            }
        }

        if (!found_edge)
            return false;
    }

    return true;
}

} // namespace boost

#ifdef BOOST_ISO_INCLUDED_ITER_MACROS
#undef BOOST_ISO_INCLUDED_ITER_MACROS
#include <boost/graph/iteration_macros_undef.hpp>
#endif

#endif // BOOST_GRAPH_ISOMORPHISM_HPP
