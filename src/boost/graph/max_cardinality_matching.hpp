//=======================================================================
// Copyright (c) 2005 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//=======================================================================

#ifndef BOOST_GRAPH_MAXIMUM_CARDINALITY_MATCHING_HPP
#define BOOST_GRAPH_MAXIMUM_CARDINALITY_MATCHING_HPP

#include <vector>
#include <list>
#include <deque>
#include <algorithm> // for std::sort and std::stable_sort
#include <utility> // for std::pair
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/assert.hpp>

namespace boost
{
namespace graph
{
    namespace detail
    {
        enum VERTEX_STATE
        {
            V_EVEN,
            V_ODD,
            V_UNREACHED
        };
    }
} // end namespace graph::detail

template < typename Graph, typename MateMap, typename VertexIndexMap >
typename graph_traits< Graph >::vertices_size_type matching_size(
    const Graph& g, MateMap mate, VertexIndexMap vm)
{
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef
        typename graph_traits< Graph >::vertex_descriptor vertex_descriptor_t;
    typedef typename graph_traits< Graph >::vertices_size_type v_size_t;

    v_size_t size_of_matching = 0;
    vertex_iterator_t vi, vi_end;

    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    {
        vertex_descriptor_t v = *vi;
        if (get(mate, v) != graph_traits< Graph >::null_vertex()
            && get(vm, v) < get(vm, get(mate, v)))
            ++size_of_matching;
    }
    return size_of_matching;
}

template < typename Graph, typename MateMap >
inline typename graph_traits< Graph >::vertices_size_type matching_size(
    const Graph& g, MateMap mate)
{
    return matching_size(g, mate, get(vertex_index, g));
}

template < typename Graph, typename MateMap, typename VertexIndexMap >
bool is_a_matching(const Graph& g, MateMap mate, VertexIndexMap)
{
    typedef
        typename graph_traits< Graph >::vertex_descriptor vertex_descriptor_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;

    vertex_iterator_t vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    {
        vertex_descriptor_t v = *vi;
        if (get(mate, v) != graph_traits< Graph >::null_vertex()
            && v != get(mate, get(mate, v)))
            return false;
    }
    return true;
}

template < typename Graph, typename MateMap >
inline bool is_a_matching(const Graph& g, MateMap mate)
{
    return is_a_matching(g, mate, get(vertex_index, g));
}

//***************************************************************************
//***************************************************************************
//               Maximum Cardinality Matching Functors
//***************************************************************************
//***************************************************************************

template < typename Graph, typename MateMap,
    typename VertexIndexMap = dummy_property_map >
struct no_augmenting_path_finder
{
    no_augmenting_path_finder(const Graph&, MateMap, VertexIndexMap) {}

    inline bool augment_matching() { return false; }

    template < typename PropertyMap > void get_current_matching(PropertyMap) {}
};

template < typename Graph, typename MateMap, typename VertexIndexMap >
class edmonds_augmenting_path_finder
{
    // This implementation of Edmonds' matching algorithm closely
    // follows Tarjan's description of the algorithm in "Data
    // Structures and Network Algorithms."

public:
    // generates the type of an iterator property map from vertices to type X
    template < typename X > struct map_vertex_to_
    {
        typedef boost::iterator_property_map<
            typename std::vector< X >::iterator, VertexIndexMap >
            type;
    };

    typedef
        typename graph_traits< Graph >::vertex_descriptor vertex_descriptor_t;
    typedef typename std::pair< vertex_descriptor_t, vertex_descriptor_t >
        vertex_pair_t;
    typedef typename graph_traits< Graph >::edge_descriptor edge_descriptor_t;
    typedef typename graph_traits< Graph >::vertices_size_type v_size_t;
    typedef typename graph_traits< Graph >::edges_size_type e_size_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef
        typename graph_traits< Graph >::out_edge_iterator out_edge_iterator_t;
    typedef typename std::deque< vertex_descriptor_t > vertex_list_t;
    typedef typename std::vector< edge_descriptor_t > edge_list_t;
    typedef typename map_vertex_to_< vertex_descriptor_t >::type
        vertex_to_vertex_map_t;
    typedef typename map_vertex_to_< int >::type vertex_to_int_map_t;
    typedef typename map_vertex_to_< vertex_pair_t >::type
        vertex_to_vertex_pair_map_t;
    typedef typename map_vertex_to_< v_size_t >::type vertex_to_vsize_map_t;
    typedef typename map_vertex_to_< e_size_t >::type vertex_to_esize_map_t;

    edmonds_augmenting_path_finder(
        const Graph& arg_g, MateMap arg_mate, VertexIndexMap arg_vm)
    : g(arg_g)
    , vm(arg_vm)
    , n_vertices(num_vertices(arg_g))
    ,

        mate_vector(n_vertices)
    , ancestor_of_v_vector(n_vertices)
    , ancestor_of_w_vector(n_vertices)
    , vertex_state_vector(n_vertices)
    , origin_vector(n_vertices)
    , pred_vector(n_vertices)
    , bridge_vector(n_vertices)
    , ds_parent_vector(n_vertices)
    , ds_rank_vector(n_vertices)
    ,

        mate(mate_vector.begin(), vm)
    , ancestor_of_v(ancestor_of_v_vector.begin(), vm)
    , ancestor_of_w(ancestor_of_w_vector.begin(), vm)
    , vertex_state(vertex_state_vector.begin(), vm)
    , origin(origin_vector.begin(), vm)
    , pred(pred_vector.begin(), vm)
    , bridge(bridge_vector.begin(), vm)
    , ds_parent_map(ds_parent_vector.begin(), vm)
    , ds_rank_map(ds_rank_vector.begin(), vm)
    ,

        ds(ds_rank_map, ds_parent_map)
    {
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            mate[*vi] = get(arg_mate, *vi);
    }

    bool augment_matching()
    {
        // As an optimization, some of these values can be saved from one
        // iteration to the next instead of being re-initialized each
        // iteration, allowing for "lazy blossom expansion." This is not
        // currently implemented.

        e_size_t timestamp = 0;
        even_edges.clear();

        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            vertex_descriptor_t u = *vi;

            origin[u] = u;
            pred[u] = u;
            ancestor_of_v[u] = 0;
            ancestor_of_w[u] = 0;
            ds.make_set(u);

            if (mate[u] == graph_traits< Graph >::null_vertex())
            {
                vertex_state[u] = graph::detail::V_EVEN;
                out_edge_iterator_t ei, ei_end;
                for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end;
                     ++ei)
                {
                    if (target(*ei, g) != u)
                    {
                        even_edges.push_back(*ei);
                    }
                }
            }
            else
                vertex_state[u] = graph::detail::V_UNREACHED;
        }

        // end initializations

        vertex_descriptor_t v, w, w_free_ancestor, v_free_ancestor;
        w_free_ancestor = graph_traits< Graph >::null_vertex();
        v_free_ancestor = graph_traits< Graph >::null_vertex();
        bool found_alternating_path = false;

        while (!even_edges.empty() && !found_alternating_path)
        {
            // since we push even edges onto the back of the list as
            // they're discovered, taking them off the back will search
            // for augmenting paths depth-first.
            edge_descriptor_t current_edge = even_edges.back();
            even_edges.pop_back();

            v = source(current_edge, g);
            w = target(current_edge, g);

            vertex_descriptor_t v_prime = origin[ds.find_set(v)];
            vertex_descriptor_t w_prime = origin[ds.find_set(w)];

            // because of the way we put all of the edges on the queue,
            // v_prime should be labeled V_EVEN; the following is a
            // little paranoid but it could happen...
            if (vertex_state[v_prime] != graph::detail::V_EVEN)
            {
                std::swap(v_prime, w_prime);
                std::swap(v, w);
            }

            if (vertex_state[w_prime] == graph::detail::V_UNREACHED)
            {
                vertex_state[w_prime] = graph::detail::V_ODD;
                vertex_descriptor_t w_prime_mate = mate[w_prime];
                vertex_state[w_prime_mate] = graph::detail::V_EVEN;
                out_edge_iterator_t ei, ei_end;
                for (boost::tie(ei, ei_end) = out_edges(w_prime_mate, g);
                     ei != ei_end; ++ei)
                {
                    if (target(*ei, g) != w_prime_mate)
                    {
                        even_edges.push_back(*ei);
                    }
                }
                pred[w_prime] = v;
            }

            // w_prime == v_prime can happen below if we get an edge that has
            // been shrunk into a blossom
            else if (vertex_state[w_prime] == graph::detail::V_EVEN
                && w_prime != v_prime)
            {
                vertex_descriptor_t w_up = w_prime;
                vertex_descriptor_t v_up = v_prime;
                vertex_descriptor_t nearest_common_ancestor
                    = graph_traits< Graph >::null_vertex();
                w_free_ancestor = graph_traits< Graph >::null_vertex();
                v_free_ancestor = graph_traits< Graph >::null_vertex();

                // We now need to distinguish between the case that
                // w_prime and v_prime share an ancestor under the
                // "parent" relation, in which case we've found a
                // blossom and should shrink it, or the case that
                // w_prime and v_prime both have distinct ancestors that
                // are free, in which case we've found an alternating
                // path between those two ancestors.

                ++timestamp;

                while (nearest_common_ancestor
                        == graph_traits< Graph >::null_vertex()
                    && (v_free_ancestor == graph_traits< Graph >::null_vertex()
                        || w_free_ancestor
                            == graph_traits< Graph >::null_vertex()))
                {
                    ancestor_of_w[w_up] = timestamp;
                    ancestor_of_v[v_up] = timestamp;

                    if (w_free_ancestor == graph_traits< Graph >::null_vertex())
                        w_up = parent(w_up);
                    if (v_free_ancestor == graph_traits< Graph >::null_vertex())
                        v_up = parent(v_up);

                    if (mate[v_up] == graph_traits< Graph >::null_vertex())
                        v_free_ancestor = v_up;
                    if (mate[w_up] == graph_traits< Graph >::null_vertex())
                        w_free_ancestor = w_up;

                    if (ancestor_of_w[v_up] == timestamp)
                        nearest_common_ancestor = v_up;
                    else if (ancestor_of_v[w_up] == timestamp)
                        nearest_common_ancestor = w_up;
                    else if (v_free_ancestor == w_free_ancestor
                        && v_free_ancestor
                            != graph_traits< Graph >::null_vertex())
                        nearest_common_ancestor = v_up;
                }

                if (nearest_common_ancestor
                    == graph_traits< Graph >::null_vertex())
                    found_alternating_path = true; // to break out of the loop
                else
                {
                    // shrink the blossom
                    link_and_set_bridges(
                        w_prime, nearest_common_ancestor, std::make_pair(w, v));
                    link_and_set_bridges(
                        v_prime, nearest_common_ancestor, std::make_pair(v, w));
                }
            }
        }

        if (!found_alternating_path)
            return false;

        // retrieve the augmenting path and put it in aug_path
        reversed_retrieve_augmenting_path(v, v_free_ancestor);
        retrieve_augmenting_path(w, w_free_ancestor);

        // augment the matching along aug_path
        vertex_descriptor_t a, b;
        while (!aug_path.empty())
        {
            a = aug_path.front();
            aug_path.pop_front();
            b = aug_path.front();
            aug_path.pop_front();
            mate[a] = b;
            mate[b] = a;
        }

        return true;
    }

    template < typename PropertyMap > void get_current_matching(PropertyMap pm)
    {
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            put(pm, *vi, mate[*vi]);
    }

    template < typename PropertyMap > void get_vertex_state_map(PropertyMap pm)
    {
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            put(pm, *vi, vertex_state[origin[ds.find_set(*vi)]]);
    }

private:
    vertex_descriptor_t parent(vertex_descriptor_t x)
    {
        if (vertex_state[x] == graph::detail::V_EVEN
            && mate[x] != graph_traits< Graph >::null_vertex())
            return mate[x];
        else if (vertex_state[x] == graph::detail::V_ODD)
            return origin[ds.find_set(pred[x])];
        else
            return x;
    }

    void link_and_set_bridges(vertex_descriptor_t x,
        vertex_descriptor_t stop_vertex, vertex_pair_t the_bridge)
    {
        for (vertex_descriptor_t v = x; v != stop_vertex; v = parent(v))
        {
            ds.union_set(v, stop_vertex);
            origin[ds.find_set(stop_vertex)] = stop_vertex;

            if (vertex_state[v] == graph::detail::V_ODD)
            {
                bridge[v] = the_bridge;
                out_edge_iterator_t oei, oei_end;
                for (boost::tie(oei, oei_end) = out_edges(v, g); oei != oei_end;
                     ++oei)
                {
                    if (target(*oei, g) != v)
                    {
                        even_edges.push_back(*oei);
                    }
                }
            }
        }
    }

    // Since none of the STL containers support both constant-time
    // concatenation and reversal, the process of expanding an
    // augmenting path once we know one exists is a little more
    // complicated than it has to be. If we know the path is from v to
    // w, then the augmenting path is recursively defined as:
    //
    // path(v,w) = [v], if v = w
    //           = concat([v, mate[v]], path(pred[mate[v]], w),
    //                if v != w and vertex_state[v] == graph::detail::V_EVEN
    //           = concat([v], reverse(path(x,mate[v])), path(y,w)),
    //                if v != w, vertex_state[v] == graph::detail::V_ODD, and
    //                bridge[v] = (x,y)
    //
    // These next two mutually recursive functions implement this definition.

    void retrieve_augmenting_path(vertex_descriptor_t v, vertex_descriptor_t w)
    {
        if (v == w)
            aug_path.push_back(v);
        else if (vertex_state[v] == graph::detail::V_EVEN)
        {
            aug_path.push_back(v);
            aug_path.push_back(mate[v]);
            retrieve_augmenting_path(pred[mate[v]], w);
        }
        else // vertex_state[v] == graph::detail::V_ODD
        {
            aug_path.push_back(v);
            reversed_retrieve_augmenting_path(bridge[v].first, mate[v]);
            retrieve_augmenting_path(bridge[v].second, w);
        }
    }

    void reversed_retrieve_augmenting_path(
        vertex_descriptor_t v, vertex_descriptor_t w)
    {

        if (v == w)
            aug_path.push_back(v);
        else if (vertex_state[v] == graph::detail::V_EVEN)
        {
            reversed_retrieve_augmenting_path(pred[mate[v]], w);
            aug_path.push_back(mate[v]);
            aug_path.push_back(v);
        }
        else // vertex_state[v] == graph::detail::V_ODD
        {
            reversed_retrieve_augmenting_path(bridge[v].second, w);
            retrieve_augmenting_path(bridge[v].first, mate[v]);
            aug_path.push_back(v);
        }
    }

    // private data members

    const Graph& g;
    VertexIndexMap vm;
    v_size_t n_vertices;

    // storage for the property maps below
    std::vector< vertex_descriptor_t > mate_vector;
    std::vector< e_size_t > ancestor_of_v_vector;
    std::vector< e_size_t > ancestor_of_w_vector;
    std::vector< int > vertex_state_vector;
    std::vector< vertex_descriptor_t > origin_vector;
    std::vector< vertex_descriptor_t > pred_vector;
    std::vector< vertex_pair_t > bridge_vector;
    std::vector< vertex_descriptor_t > ds_parent_vector;
    std::vector< v_size_t > ds_rank_vector;

    // iterator property maps
    vertex_to_vertex_map_t mate;
    vertex_to_esize_map_t ancestor_of_v;
    vertex_to_esize_map_t ancestor_of_w;
    vertex_to_int_map_t vertex_state;
    vertex_to_vertex_map_t origin;
    vertex_to_vertex_map_t pred;
    vertex_to_vertex_pair_map_t bridge;
    vertex_to_vertex_map_t ds_parent_map;
    vertex_to_vsize_map_t ds_rank_map;

    vertex_list_t aug_path;
    edge_list_t even_edges;
    disjoint_sets< vertex_to_vsize_map_t, vertex_to_vertex_map_t > ds;
};

//***************************************************************************
//***************************************************************************
//               Initial Matching Functors
//***************************************************************************
//***************************************************************************

template < typename Graph, typename MateMap > struct greedy_matching
{
    typedef
        typename graph_traits< Graph >::vertex_descriptor vertex_descriptor_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef typename graph_traits< Graph >::edge_descriptor edge_descriptor_t;
    typedef typename graph_traits< Graph >::edge_iterator edge_iterator_t;

    static void find_matching(const Graph& g, MateMap mate)
    {
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            put(mate, *vi, graph_traits< Graph >::null_vertex());

        edge_iterator_t ei, ei_end;
        for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
        {
            edge_descriptor_t e = *ei;
            vertex_descriptor_t u = source(e, g);
            vertex_descriptor_t v = target(e, g);

            if (u != v && get(mate, u) == get(mate, v))
            // only way equality can hold is if
            //   mate[u] == mate[v] == null_vertex
            {
                put(mate, u, v);
                put(mate, v, u);
            }
        }
    }
};

template < typename Graph, typename MateMap > struct extra_greedy_matching
{
    // The "extra greedy matching" is formed by repeating the
    // following procedure as many times as possible: Choose the
    // unmatched vertex v of minimum non-zero degree.  Choose the
    // neighbor w of v which is unmatched and has minimum degree over
    // all of v's neighbors. Add (u,v) to the matching. Ties for
    // either choice are broken arbitrarily. This procedure takes time
    // O(m log n), where m is the number of edges in the graph and n
    // is the number of vertices.

    typedef
        typename graph_traits< Graph >::vertex_descriptor vertex_descriptor_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef typename graph_traits< Graph >::edge_descriptor edge_descriptor_t;
    typedef typename graph_traits< Graph >::edge_iterator edge_iterator_t;
    typedef std::pair< vertex_descriptor_t, vertex_descriptor_t > vertex_pair_t;

    struct select_first
    {
        inline static vertex_descriptor_t select_vertex(const vertex_pair_t p)
        {
            return p.first;
        }
    };

    struct select_second
    {
        inline static vertex_descriptor_t select_vertex(const vertex_pair_t p)
        {
            return p.second;
        }
    };

    template < class PairSelector > class less_than_by_degree
    {
    public:
        less_than_by_degree(const Graph& g) : m_g(g) {}
        bool operator()(const vertex_pair_t x, const vertex_pair_t y)
        {
            return out_degree(PairSelector::select_vertex(x), m_g)
                < out_degree(PairSelector::select_vertex(y), m_g);
        }

    private:
        const Graph& m_g;
    };

    static void find_matching(const Graph& g, MateMap mate)
    {
        typedef std::vector<
            std::pair< vertex_descriptor_t, vertex_descriptor_t > >
            directed_edges_vector_t;

        directed_edges_vector_t edge_list;
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            put(mate, *vi, graph_traits< Graph >::null_vertex());

        edge_iterator_t ei, ei_end;
        for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
        {
            edge_descriptor_t e = *ei;
            vertex_descriptor_t u = source(e, g);
            vertex_descriptor_t v = target(e, g);
            if (u == v)
                continue;
            edge_list.push_back(std::make_pair(u, v));
            edge_list.push_back(std::make_pair(v, u));
        }

        // sort the edges by the degree of the target, then (using a
        // stable sort) by degree of the source
        std::sort(edge_list.begin(), edge_list.end(),
            less_than_by_degree< select_second >(g));
        std::stable_sort(edge_list.begin(), edge_list.end(),
            less_than_by_degree< select_first >(g));

        // construct the extra greedy matching
        for (typename directed_edges_vector_t::const_iterator itr
             = edge_list.begin();
             itr != edge_list.end(); ++itr)
        {
            if (get(mate, itr->first) == get(mate, itr->second))
            // only way equality can hold is if mate[itr->first] ==
            // mate[itr->second] == null_vertex
            {
                put(mate, itr->first, itr->second);
                put(mate, itr->second, itr->first);
            }
        }
    }
};

template < typename Graph, typename MateMap > struct empty_matching
{
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;

    static void find_matching(const Graph& g, MateMap mate)
    {
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            put(mate, *vi, graph_traits< Graph >::null_vertex());
    }
};

//***************************************************************************
//***************************************************************************
//               Matching Verifiers
//***************************************************************************
//***************************************************************************

namespace detail
{

    template < typename SizeType >
    class odd_components_counter : public dfs_visitor<>
    // This depth-first search visitor will count the number of connected
    // components with an odd number of vertices. It's used by
    // maximum_matching_verifier.
    {
    public:
        odd_components_counter(SizeType& c_count) : m_count(c_count)
        {
            m_count = 0;
        }

        template < class Vertex, class Graph > void start_vertex(Vertex, Graph&)
        {
            m_parity = false;
        }

        template < class Vertex, class Graph >
        void discover_vertex(Vertex, Graph&)
        {
            m_parity = !m_parity;
            m_parity ? ++m_count : --m_count;
        }

    protected:
        SizeType& m_count;

    private:
        bool m_parity;
    };

} // namespace detail

template < typename Graph, typename MateMap,
    typename VertexIndexMap = dummy_property_map >
struct no_matching_verifier
{
    inline static bool verify_matching(const Graph&, MateMap, VertexIndexMap)
    {
        return true;
    }
};

template < typename Graph, typename MateMap, typename VertexIndexMap >
struct maximum_cardinality_matching_verifier
{

    template < typename X > struct map_vertex_to_
    {
        typedef boost::iterator_property_map<
            typename std::vector< X >::iterator, VertexIndexMap >
            type;
    };

    typedef
        typename graph_traits< Graph >::vertex_descriptor vertex_descriptor_t;
    typedef typename graph_traits< Graph >::vertices_size_type v_size_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef typename map_vertex_to_< int >::type vertex_to_int_map_t;
    typedef typename map_vertex_to_< vertex_descriptor_t >::type
        vertex_to_vertex_map_t;

    template < typename VertexStateMap > struct non_odd_vertex
    {
        // this predicate is used to create a filtered graph that
        // excludes vertices labeled "graph::detail::V_ODD"
        non_odd_vertex() : vertex_state(0) {}

        non_odd_vertex(VertexStateMap* arg_vertex_state)
        : vertex_state(arg_vertex_state)
        {
        }

        template < typename Vertex > bool operator()(const Vertex& v) const
        {
            BOOST_ASSERT(vertex_state);
            return get(*vertex_state, v) != graph::detail::V_ODD;
        }

        VertexStateMap* vertex_state;
    };

    static bool verify_matching(const Graph& g, MateMap mate, VertexIndexMap vm)
    {
        // For any graph G, let o(G) be the number of connected
        // components in G of odd size. For a subset S of G's vertex set
        // V(G), let (G - S) represent the subgraph of G induced by
        // removing all vertices in S from G. Let M(G) be the size of the
        // maximum cardinality matching in G. Then the Tutte-Berge
        // formula guarantees that
        //
        //           2 * M(G) = min ( |V(G)| + |U| + o(G - U) )
        //
        // where the minimum is taken over all subsets U of
        // V(G). Edmonds' algorithm finds a set U that achieves the
        // minimum in the above formula, namely the vertices labeled
        //"ODD." This function runs one iteration of Edmonds' algorithm
        // to find U, then verifies that the size of the matching given
        // by mate satisfies the Tutte-Berge formula.

        // first, make sure it's a valid matching
        if (!is_a_matching(g, mate, vm))
            return false;

        // We'll try to augment the matching once. This serves two
        // purposes: first, if we find some augmenting path, the matching
        // is obviously non-maximum. Second, running edmonds' algorithm
        // on a graph with no augmenting path will create the
        // Edmonds-Gallai decomposition that we need as a certificate of
        // maximality - we can get it by looking at the vertex_state map
        // that results.
        edmonds_augmenting_path_finder< Graph, MateMap, VertexIndexMap >
            augmentor(g, mate, vm);
        if (augmentor.augment_matching())
            return false;

        std::vector< int > vertex_state_vector(num_vertices(g));
        vertex_to_int_map_t vertex_state(vertex_state_vector.begin(), vm);
        augmentor.get_vertex_state_map(vertex_state);

        // count the number of graph::detail::V_ODD vertices
        v_size_t num_odd_vertices = 0;
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            if (vertex_state[*vi] == graph::detail::V_ODD)
                ++num_odd_vertices;

        // count the number of connected components with odd cardinality
        // in the graph without graph::detail::V_ODD vertices
        non_odd_vertex< vertex_to_int_map_t > filter(&vertex_state);
        filtered_graph< Graph, keep_all, non_odd_vertex< vertex_to_int_map_t > >
            fg(g, keep_all(), filter);

        v_size_t num_odd_components;
        detail::odd_components_counter< v_size_t > occ(num_odd_components);
        depth_first_search(fg, visitor(occ).vertex_index_map(vm));

        if (2 * matching_size(g, mate, vm)
            == num_vertices(g) + num_odd_vertices - num_odd_components)
            return true;
        else
            return false;
    }
};

template < typename Graph, typename MateMap, typename VertexIndexMap,
    template < typename, typename, typename > class AugmentingPathFinder,
    template < typename, typename > class InitialMatchingFinder,
    template < typename, typename, typename > class MatchingVerifier >
bool matching(const Graph& g, MateMap mate, VertexIndexMap vm)
{

    InitialMatchingFinder< Graph, MateMap >::find_matching(g, mate);

    AugmentingPathFinder< Graph, MateMap, VertexIndexMap > augmentor(
        g, mate, vm);
    bool not_maximum_yet = true;
    while (not_maximum_yet)
    {
        not_maximum_yet = augmentor.augment_matching();
    }
    augmentor.get_current_matching(mate);

    return MatchingVerifier< Graph, MateMap, VertexIndexMap >::verify_matching(
        g, mate, vm);
}

template < typename Graph, typename MateMap, typename VertexIndexMap >
inline bool checked_edmonds_maximum_cardinality_matching(
    const Graph& g, MateMap mate, VertexIndexMap vm)
{
    return matching< Graph, MateMap, VertexIndexMap,
        edmonds_augmenting_path_finder, extra_greedy_matching,
        maximum_cardinality_matching_verifier >(g, mate, vm);
}

template < typename Graph, typename MateMap >
inline bool checked_edmonds_maximum_cardinality_matching(
    const Graph& g, MateMap mate)
{
    return checked_edmonds_maximum_cardinality_matching(
        g, mate, get(vertex_index, g));
}

template < typename Graph, typename MateMap, typename VertexIndexMap >
inline void edmonds_maximum_cardinality_matching(
    const Graph& g, MateMap mate, VertexIndexMap vm)
{
    matching< Graph, MateMap, VertexIndexMap, edmonds_augmenting_path_finder,
        extra_greedy_matching, no_matching_verifier >(g, mate, vm);
}

template < typename Graph, typename MateMap >
inline void edmonds_maximum_cardinality_matching(const Graph& g, MateMap mate)
{
    edmonds_maximum_cardinality_matching(g, mate, get(vertex_index, g));
}

} // namespace boost

#endif // BOOST_GRAPH_MAXIMUM_CARDINALITY_MATCHING_HPP
