//=======================================================================
// Copyright (c) Aaron Windsor 2007
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef __BOYER_MYRVOLD_IMPL_HPP__
#define __BOYER_MYRVOLD_IMPL_HPP__

#include <vector>
#include <list>
#include <boost/next_prior.hpp>
#include <boost/config.hpp> //for std::min macros
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/planar_detail/face_handles.hpp>
#include <boost/graph/planar_detail/face_iterators.hpp>
#include <boost/graph/planar_detail/bucket_sort.hpp>

namespace boost
{
namespace detail
{
    enum bm_case_t
    {
        BM_NO_CASE_CHOSEN,
        BM_CASE_A,
        BM_CASE_B,
        BM_CASE_C,
        BM_CASE_D,
        BM_CASE_E
    };
}

template < typename LowPointMap, typename DFSParentMap, typename DFSNumberMap,
    typename LeastAncestorMap, typename DFSParentEdgeMap, typename SizeType >
struct planar_dfs_visitor : public dfs_visitor<>
{
    planar_dfs_visitor(LowPointMap lpm, DFSParentMap dfs_p, DFSNumberMap dfs_n,
        LeastAncestorMap lam, DFSParentEdgeMap dfs_edge)
    : low(lpm)
    , parent(dfs_p)
    , df_number(dfs_n)
    , least_ancestor(lam)
    , df_edge(dfs_edge)
    , count(0)
    {
    }

    template < typename Vertex, typename Graph >
    void start_vertex(const Vertex& u, Graph&)
    {
        put(parent, u, u);
        put(least_ancestor, u, count);
    }

    template < typename Vertex, typename Graph >
    void discover_vertex(const Vertex& u, Graph&)
    {
        put(low, u, count);
        put(df_number, u, count);
        ++count;
    }

    template < typename Edge, typename Graph >
    void tree_edge(const Edge& e, Graph& g)
    {
        typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
        vertex_t s(source(e, g));
        vertex_t t(target(e, g));

        put(parent, t, s);
        put(df_edge, t, e);
        put(least_ancestor, t, get(df_number, s));
    }

    template < typename Edge, typename Graph >
    void back_edge(const Edge& e, Graph& g)
    {
        typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
        typedef typename graph_traits< Graph >::vertices_size_type v_size_t;

        vertex_t s(source(e, g));
        vertex_t t(target(e, g));
        BOOST_USING_STD_MIN();

        if (t != get(parent, s))
        {
            v_size_t s_low_df_number = get(low, s);
            v_size_t t_df_number = get(df_number, t);
            v_size_t s_least_ancestor_df_number = get(least_ancestor, s);

            put(low, s,
                min BOOST_PREVENT_MACRO_SUBSTITUTION(
                    s_low_df_number, t_df_number));

            put(least_ancestor, s,
                min BOOST_PREVENT_MACRO_SUBSTITUTION(
                    s_least_ancestor_df_number, t_df_number));
        }
    }

    template < typename Vertex, typename Graph >
    void finish_vertex(const Vertex& u, Graph&)
    {
        typedef typename graph_traits< Graph >::vertices_size_type v_size_t;

        Vertex u_parent = get(parent, u);
        v_size_t u_parent_lowpoint = get(low, u_parent);
        v_size_t u_lowpoint = get(low, u);
        BOOST_USING_STD_MIN();

        if (u_parent != u)
        {
            put(low, u_parent,
                min BOOST_PREVENT_MACRO_SUBSTITUTION(
                    u_lowpoint, u_parent_lowpoint));
        }
    }

    LowPointMap low;
    DFSParentMap parent;
    DFSNumberMap df_number;
    LeastAncestorMap least_ancestor;
    DFSParentEdgeMap df_edge;
    SizeType count;
};

template < typename Graph, typename VertexIndexMap,
    typename StoreOldHandlesPolicy = graph::detail::store_old_handles,
    typename StoreEmbeddingPolicy = graph::detail::recursive_lazy_list >
class boyer_myrvold_impl
{

    typedef typename graph_traits< Graph >::vertices_size_type v_size_t;
    typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
    typedef typename graph_traits< Graph >::edge_descriptor edge_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef typename graph_traits< Graph >::edge_iterator edge_iterator_t;
    typedef
        typename graph_traits< Graph >::out_edge_iterator out_edge_iterator_t;
    typedef graph::detail::face_handle< Graph, StoreOldHandlesPolicy,
        StoreEmbeddingPolicy >
        face_handle_t;
    typedef std::vector< vertex_t > vertex_vector_t;
    typedef std::vector< edge_t > edge_vector_t;
    typedef std::list< vertex_t > vertex_list_t;
    typedef std::list< face_handle_t > face_handle_list_t;
    typedef boost::shared_ptr< face_handle_list_t > face_handle_list_ptr_t;
    typedef boost::shared_ptr< vertex_list_t > vertex_list_ptr_t;
    typedef boost::tuple< vertex_t, bool, bool > merge_stack_frame_t;
    typedef std::vector< merge_stack_frame_t > merge_stack_t;

    template < typename T > struct map_vertex_to_
    {
        typedef iterator_property_map< typename std::vector< T >::iterator,
            VertexIndexMap >
            type;
    };

    typedef typename map_vertex_to_< v_size_t >::type vertex_to_v_size_map_t;
    typedef typename map_vertex_to_< vertex_t >::type vertex_to_vertex_map_t;
    typedef typename map_vertex_to_< edge_t >::type vertex_to_edge_map_t;
    typedef typename map_vertex_to_< vertex_list_ptr_t >::type
        vertex_to_vertex_list_ptr_map_t;
    typedef typename map_vertex_to_< edge_vector_t >::type
        vertex_to_edge_vector_map_t;
    typedef typename map_vertex_to_< bool >::type vertex_to_bool_map_t;
    typedef typename map_vertex_to_< face_handle_t >::type
        vertex_to_face_handle_map_t;
    typedef typename map_vertex_to_< face_handle_list_ptr_t >::type
        vertex_to_face_handle_list_ptr_map_t;
    typedef typename map_vertex_to_< typename vertex_list_t::iterator >::type
        vertex_to_separated_node_map_t;

    template < typename BicompSideToTraverse = single_side,
        typename VisitorType = lead_visitor, typename Time = current_iteration >
    struct face_vertex_iterator
    {
        typedef face_iterator< Graph, vertex_to_face_handle_map_t, vertex_t,
            BicompSideToTraverse, VisitorType, Time >
            type;
    };

    template < typename BicompSideToTraverse = single_side,
        typename Time = current_iteration >
    struct face_edge_iterator
    {
        typedef face_iterator< Graph, vertex_to_face_handle_map_t, edge_t,
            BicompSideToTraverse, lead_visitor, Time >
            type;
    };

public:
    boyer_myrvold_impl(const Graph& arg_g, VertexIndexMap arg_vm)
    : g(arg_g)
    , vm(arg_vm)
    ,

        low_point_vector(num_vertices(g))
    , dfs_parent_vector(num_vertices(g))
    , dfs_number_vector(num_vertices(g))
    , least_ancestor_vector(num_vertices(g))
    , pertinent_roots_vector(num_vertices(g))
    , backedge_flag_vector(num_vertices(g), num_vertices(g) + 1)
    , visited_vector(num_vertices(g), num_vertices(g) + 1)
    , face_handles_vector(num_vertices(g))
    , dfs_child_handles_vector(num_vertices(g))
    , separated_dfs_child_list_vector(num_vertices(g))
    , separated_node_in_parent_list_vector(num_vertices(g))
    , canonical_dfs_child_vector(num_vertices(g))
    , flipped_vector(num_vertices(g), false)
    , backedges_vector(num_vertices(g))
    , dfs_parent_edge_vector(num_vertices(g))
    ,

        vertices_by_dfs_num(num_vertices(g))
    ,

        low_point(low_point_vector.begin(), vm)
    , dfs_parent(dfs_parent_vector.begin(), vm)
    , dfs_number(dfs_number_vector.begin(), vm)
    , least_ancestor(least_ancestor_vector.begin(), vm)
    , pertinent_roots(pertinent_roots_vector.begin(), vm)
    , backedge_flag(backedge_flag_vector.begin(), vm)
    , visited(visited_vector.begin(), vm)
    , face_handles(face_handles_vector.begin(), vm)
    , dfs_child_handles(dfs_child_handles_vector.begin(), vm)
    , separated_dfs_child_list(separated_dfs_child_list_vector.begin(), vm)
    , separated_node_in_parent_list(
          separated_node_in_parent_list_vector.begin(), vm)
    , canonical_dfs_child(canonical_dfs_child_vector.begin(), vm)
    , flipped(flipped_vector.begin(), vm)
    , backedges(backedges_vector.begin(), vm)
    , dfs_parent_edge(dfs_parent_edge_vector.begin(), vm)

    {

        planar_dfs_visitor< vertex_to_v_size_map_t, vertex_to_vertex_map_t,
            vertex_to_v_size_map_t, vertex_to_v_size_map_t,
            vertex_to_edge_map_t, v_size_t >
            vis(low_point, dfs_parent, dfs_number, least_ancestor,
                dfs_parent_edge);

        // Perform a depth-first search to find each vertex's low point, least
        // ancestor, and dfs tree information
        depth_first_search(g, visitor(vis).vertex_index_map(vm));

        // Sort vertices by their lowpoint - need this later in the constructor
        vertex_vector_t vertices_by_lowpoint(num_vertices(g));
        std::copy(vertices(g).first, vertices(g).second,
            vertices_by_lowpoint.begin());
        bucket_sort(vertices_by_lowpoint.begin(), vertices_by_lowpoint.end(),
            low_point, num_vertices(g));

        // Sort vertices by their dfs number - need this to iterate by reverse
        // DFS number in the main loop.
        std::copy(
            vertices(g).first, vertices(g).second, vertices_by_dfs_num.begin());
        bucket_sort(vertices_by_dfs_num.begin(), vertices_by_dfs_num.end(),
            dfs_number, num_vertices(g));

        // Initialize face handles. A face handle is an abstraction that serves
        // two uses in our implementation - it allows us to efficiently move
        // along the outer face of embedded bicomps in a partially embedded
        // graph, and it provides storage for the planar embedding. Face
        // handles are implemented by a sequence of edges and are associated
        // with a particular vertex - the sequence of edges represents the
        // current embedding of edges around that vertex, and the first and
        // last edges in the sequence represent the pair of edges on the outer
        // face that are adjacent to the associated vertex. This lets us embed
        // edges in the graph by just pushing them on the front or back of the
        // sequence of edges held by the face handles.
        //
        // Our algorithm starts with a DFS tree of edges (where every vertex is
        // an articulation point and every edge is a singleton bicomp) and
        // repeatedly merges bicomps by embedding additional edges. Note that
        // any bicomp at any point in the algorithm can be associated with a
        // unique edge connecting the vertex of that bicomp with the lowest DFS
        // number (which we refer to as the "root" of the bicomp) with its DFS
        // child in the bicomp: the existence of two such edges would contradict
        // the properties of a DFS tree. We refer to the DFS child of the root
        // of a bicomp as the "canonical DFS child" of the bicomp. Note that a
        // vertex can be the root of more than one bicomp.
        //
        // We move around the external faces of a bicomp using a few property
        // maps, which we'll initialize presently:
        //
        // - face_handles: maps a vertex to a face handle that can be used to
        //   move "up" a bicomp. For a vertex that isn't an articulation point,
        //   this holds the face handles that can be used to move around that
        //   vertex's unique bicomp. For a vertex that is an articulation point,
        //   this holds the face handles associated with the unique bicomp that
        //   the vertex is NOT the root of. These handles can therefore be used
        //   to move from any point on the outer face of the tree of bicomps
        //   around the current outer face towards the root of the DFS tree.
        //
        // - dfs_child_handles: these are used to hold face handles for
        //   vertices that are articulation points - dfs_child_handles[v] holds
        //   the face handles corresponding to vertex u in the bicomp with root
        //   u and canonical DFS child v.
        //
        // - canonical_dfs_child: this property map allows one to determine the
        //   canonical DFS child of a bicomp while traversing the outer face.
        //   This property map is only valid when applied to one of the two
        //   vertices adjacent to the root of the bicomp on the outer face. To
        //   be more precise, if v is the canonical DFS child of a bicomp,
        //   canonical_dfs_child[dfs_child_handles[v].first_vertex()] == v and
        //   canonical_dfs_child[dfs_child_handles[v].second_vertex()] == v.
        //
        // - pertinent_roots: given a vertex v, pertinent_roots[v] contains a
        //   list of face handles pointing to the top of bicomps that need to
        //   be visited by the current walkdown traversal (since they lead to
        //   backedges that need to be embedded). These lists are populated by
        //   the walkup and consumed by the walkdown.

        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            vertex_t v(*vi);
            vertex_t parent = dfs_parent[v];

            if (parent != v)
            {
                edge_t parent_edge = dfs_parent_edge[v];
                add_to_embedded_edges(parent_edge, StoreOldHandlesPolicy());
                face_handles[v] = face_handle_t(v, parent_edge, g);
                dfs_child_handles[v] = face_handle_t(parent, parent_edge, g);
            }
            else
            {
                face_handles[v] = face_handle_t(v);
                dfs_child_handles[v] = face_handle_t(parent);
            }

            canonical_dfs_child[v] = v;
            pertinent_roots[v] = face_handle_list_ptr_t(new face_handle_list_t);
            separated_dfs_child_list[v] = vertex_list_ptr_t(new vertex_list_t);
        }

        // We need to create a list of not-yet-merged depth-first children for
        // each vertex that will be updated as bicomps get merged. We sort each
        // list by ascending lowpoint, which allows the externally_active
        // function to run in constant time, and we keep a pointer to each
        // vertex's representation in its parent's list, which allows merging
        // in constant time.

        for (typename vertex_vector_t::iterator itr
             = vertices_by_lowpoint.begin();
             itr != vertices_by_lowpoint.end(); ++itr)
        {
            vertex_t v(*itr);
            vertex_t parent(dfs_parent[v]);
            if (v != parent)
            {
                separated_node_in_parent_list[v]
                    = separated_dfs_child_list[parent]->insert(
                        separated_dfs_child_list[parent]->end(), v);
            }
        }

        // The merge stack holds path information during a walkdown iteration
        merge_stack.reserve(num_vertices(g));
    }

    bool is_planar()
    {

        // This is the main algorithm: starting with a DFS tree of embedded
        // edges (which, since it's a tree, is planar), iterate through all
        // vertices by reverse DFS number, attempting to embed all backedges
        // connecting the current vertex to vertices with higher DFS numbers.
        //
        // The walkup is a procedure that examines all such backedges and sets
        // up the required data structures so that they can be searched by the
        // walkdown in linear time. The walkdown does the actual work of
        // embedding edges and flipping bicomps, and can identify when it has
        // come across a kuratowski subgraph.
        //
        // store_old_face_handles caches face handles from the previous
        // iteration - this is used only for the kuratowski subgraph isolation,
        // and is therefore dispatched based on the StoreOldHandlesPolicy.
        //
        // clean_up_embedding does some clean-up and fills in values that have
        // to be computed lazily during the actual execution of the algorithm
        // (for instance, whether or not a bicomp is flipped in the final
        // embedding). It's dispatched on the the StoreEmbeddingPolicy, since
        // it's not needed if an embedding isn't desired.

        typename vertex_vector_t::reverse_iterator vi, vi_end;

        vi_end = vertices_by_dfs_num.rend();
        for (vi = vertices_by_dfs_num.rbegin(); vi != vi_end; ++vi)
        {

            store_old_face_handles(StoreOldHandlesPolicy());

            vertex_t v(*vi);

            walkup(v);

            if (!walkdown(v))
                return false;
        }

        clean_up_embedding(StoreEmbeddingPolicy());

        return true;
    }

private:
    void walkup(vertex_t v)
    {

        // The point of the walkup is to follow all backedges from v to
        // vertices with higher DFS numbers, and update pertinent_roots
        // for the bicomp roots on the path from backedge endpoints up
        // to v. This will set the stage for the walkdown to efficiently
        // traverse the graph of bicomps down from v.

        typedef
            typename face_vertex_iterator< both_sides >::type walkup_iterator_t;

        out_edge_iterator_t oi, oi_end;
        for (boost::tie(oi, oi_end) = out_edges(v, g); oi != oi_end; ++oi)
        {
            edge_t e(*oi);
            vertex_t e_source(source(e, g));
            vertex_t e_target(target(e, g));

            if (e_source == e_target)
            {
                self_loops.push_back(e);
                continue;
            }

            vertex_t w(e_source == v ? e_target : e_source);

            // continue if not a back edge or already embedded
            if (dfs_number[w] < dfs_number[v] || e == dfs_parent_edge[w])
                continue;

            backedges[w].push_back(e);

            v_size_t timestamp = dfs_number[v];
            backedge_flag[w] = timestamp;

            walkup_iterator_t walkup_itr(w, face_handles);
            walkup_iterator_t walkup_end;
            vertex_t lead_vertex = w;

            while (true)
            {

                // Move to the root of the current bicomp or the first visited
                // vertex on the bicomp by going up each side in parallel

                while (walkup_itr != walkup_end
                    && visited[*walkup_itr] != timestamp)
                {
                    lead_vertex = *walkup_itr;
                    visited[lead_vertex] = timestamp;
                    ++walkup_itr;
                }

                // If we've found the root of a bicomp through a path we haven't
                // seen before, update pertinent_roots with a handle to the
                // current bicomp. Otherwise, we've just seen a path we've been
                // up before, so break out of the main while loop.

                if (walkup_itr == walkup_end)
                {
                    vertex_t dfs_child = canonical_dfs_child[lead_vertex];
                    vertex_t parent = dfs_parent[dfs_child];

                    visited[dfs_child_handles[dfs_child].first_vertex()]
                        = timestamp;
                    visited[dfs_child_handles[dfs_child].second_vertex()]
                        = timestamp;

                    if (low_point[dfs_child] < dfs_number[v]
                        || least_ancestor[dfs_child] < dfs_number[v])
                    {
                        pertinent_roots[parent]->push_back(
                            dfs_child_handles[dfs_child]);
                    }
                    else
                    {
                        pertinent_roots[parent]->push_front(
                            dfs_child_handles[dfs_child]);
                    }

                    if (parent != v && visited[parent] != timestamp)
                    {
                        walkup_itr = walkup_iterator_t(parent, face_handles);
                        lead_vertex = parent;
                    }
                    else
                        break;
                }
                else
                    break;
            }
        }
    }

    bool walkdown(vertex_t v)
    {
        // This procedure is where all of the action is - pertinent_roots
        // has already been set up by the walkup, so we just need to move
        // down bicomps from v until we find vertices that have been
        // labeled as backedge endpoints. Once we find such a vertex, we
        // embed the corresponding edge and glue together the bicomps on
        // the path connecting the two vertices in the edge. This may
        // involve flipping bicomps along the way.

        vertex_t w; // the other endpoint of the edge we're embedding

        while (!pertinent_roots[v]->empty())
        {

            face_handle_t root_face_handle = pertinent_roots[v]->front();
            face_handle_t curr_face_handle = root_face_handle;
            pertinent_roots[v]->pop_front();

            merge_stack.clear();

            while (true)
            {

                typename face_vertex_iterator<>::type first_face_itr,
                    second_face_itr, face_end;
                vertex_t first_side_vertex
                    = graph_traits< Graph >::null_vertex();
                vertex_t second_side_vertex
                    = graph_traits< Graph >::null_vertex();
                vertex_t first_tail, second_tail;

                first_tail = second_tail = curr_face_handle.get_anchor();
                first_face_itr = typename face_vertex_iterator<>::type(
                    curr_face_handle, face_handles, first_side());
                second_face_itr = typename face_vertex_iterator<>::type(
                    curr_face_handle, face_handles, second_side());

                for (; first_face_itr != face_end; ++first_face_itr)
                {
                    vertex_t face_vertex(*first_face_itr);
                    if (pertinent(face_vertex, v)
                        || externally_active(face_vertex, v))
                    {
                        first_side_vertex = face_vertex;
                        second_side_vertex = face_vertex;
                        break;
                    }
                    first_tail = face_vertex;
                }

                if (first_side_vertex == graph_traits< Graph >::null_vertex()
                    || first_side_vertex == curr_face_handle.get_anchor())
                    break;

                for (; second_face_itr != face_end; ++second_face_itr)
                {
                    vertex_t face_vertex(*second_face_itr);
                    if (pertinent(face_vertex, v)
                        || externally_active(face_vertex, v))
                    {
                        second_side_vertex = face_vertex;
                        break;
                    }
                    second_tail = face_vertex;
                }

                vertex_t chosen;
                bool chose_first_upper_path;
                if (internally_active(first_side_vertex, v))
                {
                    chosen = first_side_vertex;
                    chose_first_upper_path = true;
                }
                else if (internally_active(second_side_vertex, v))
                {
                    chosen = second_side_vertex;
                    chose_first_upper_path = false;
                }
                else if (pertinent(first_side_vertex, v))
                {
                    chosen = first_side_vertex;
                    chose_first_upper_path = true;
                }
                else if (pertinent(second_side_vertex, v))
                {
                    chosen = second_side_vertex;
                    chose_first_upper_path = false;
                }
                else
                {

                    // If there's a pertinent vertex on the lower face
                    // between the first_face_itr and the second_face_itr,
                    // this graph isn't planar.
                    for (; *first_face_itr != second_side_vertex;
                         ++first_face_itr)
                    {
                        vertex_t p(*first_face_itr);
                        if (pertinent(p, v))
                        {
                            // Found a Kuratowski subgraph
                            kuratowski_v = v;
                            kuratowski_x = first_side_vertex;
                            kuratowski_y = second_side_vertex;
                            return false;
                        }
                    }

                    // Otherwise, the fact that we didn't find a pertinent
                    // vertex on this face is fine - we should set the
                    // short-circuit edges and break out of this loop to
                    // start looking at a different pertinent root.

                    if (first_side_vertex == second_side_vertex)
                    {
                        if (first_tail != v)
                        {
                            vertex_t first
                                = face_handles[first_tail].first_vertex();
                            vertex_t second
                                = face_handles[first_tail].second_vertex();
                            boost::tie(first_side_vertex, first_tail)
                                = make_tuple(first_tail,
                                    first == first_side_vertex ? second
                                                               : first);
                        }
                        else if (second_tail != v)
                        {
                            vertex_t first
                                = face_handles[second_tail].first_vertex();
                            vertex_t second
                                = face_handles[second_tail].second_vertex();
                            boost::tie(second_side_vertex, second_tail)
                                = make_tuple(second_tail,
                                    first == second_side_vertex ? second
                                                                : first);
                        }
                        else
                            break;
                    }

                    canonical_dfs_child[first_side_vertex]
                        = canonical_dfs_child[root_face_handle.first_vertex()];
                    canonical_dfs_child[second_side_vertex]
                        = canonical_dfs_child[root_face_handle.second_vertex()];
                    root_face_handle.set_first_vertex(first_side_vertex);
                    root_face_handle.set_second_vertex(second_side_vertex);

                    if (face_handles[first_side_vertex].first_vertex()
                        == first_tail)
                        face_handles[first_side_vertex].set_first_vertex(v);
                    else
                        face_handles[first_side_vertex].set_second_vertex(v);

                    if (face_handles[second_side_vertex].first_vertex()
                        == second_tail)
                        face_handles[second_side_vertex].set_first_vertex(v);
                    else
                        face_handles[second_side_vertex].set_second_vertex(v);

                    break;
                }

                // When we unwind the stack, we need to know which direction
                // we came down from on the top face handle

                bool chose_first_lower_path
                    = (chose_first_upper_path
                          && face_handles[chosen].first_vertex() == first_tail)
                    || (!chose_first_upper_path
                        && face_handles[chosen].first_vertex() == second_tail);

                // If there's a backedge at the chosen vertex, embed it now
                if (backedge_flag[chosen] == dfs_number[v])
                {
                    w = chosen;

                    backedge_flag[chosen] = num_vertices(g) + 1;
                    add_to_merge_points(chosen, StoreOldHandlesPolicy());

                    typename edge_vector_t::iterator ei, ei_end;
                    ei_end = backedges[chosen].end();
                    for (ei = backedges[chosen].begin(); ei != ei_end; ++ei)
                    {
                        edge_t e(*ei);
                        add_to_embedded_edges(e, StoreOldHandlesPolicy());

                        if (chose_first_lower_path)
                            face_handles[chosen].push_first(e, g);
                        else
                            face_handles[chosen].push_second(e, g);
                    }
                }
                else
                {
                    merge_stack.push_back(make_tuple(chosen,
                        chose_first_upper_path, chose_first_lower_path));
                    curr_face_handle = *pertinent_roots[chosen]->begin();
                    continue;
                }

                // Unwind the merge stack to the root, merging all bicomps

                bool bottom_path_follows_first;
                bool top_path_follows_first;
                bool next_bottom_follows_first = chose_first_upper_path;

                vertex_t merge_point = chosen;

                while (!merge_stack.empty())
                {

                    bottom_path_follows_first = next_bottom_follows_first;
                    boost::tie(merge_point, next_bottom_follows_first,
                        top_path_follows_first)
                        = merge_stack.back();
                    merge_stack.pop_back();

                    face_handle_t top_handle(face_handles[merge_point]);
                    face_handle_t bottom_handle(
                        *pertinent_roots[merge_point]->begin());

                    vertex_t bottom_dfs_child = canonical_dfs_child
                        [pertinent_roots[merge_point]->begin()->first_vertex()];

                    remove_vertex_from_separated_dfs_child_list(
                        canonical_dfs_child[pertinent_roots[merge_point]
                                                ->begin()
                                                ->first_vertex()]);

                    pertinent_roots[merge_point]->pop_front();

                    add_to_merge_points(
                        top_handle.get_anchor(), StoreOldHandlesPolicy());

                    if (top_path_follows_first && bottom_path_follows_first)
                    {
                        bottom_handle.flip();
                        top_handle.glue_first_to_second(bottom_handle);
                    }
                    else if (!top_path_follows_first
                        && bottom_path_follows_first)
                    {
                        flipped[bottom_dfs_child] = true;
                        top_handle.glue_second_to_first(bottom_handle);
                    }
                    else if (top_path_follows_first
                        && !bottom_path_follows_first)
                    {
                        flipped[bottom_dfs_child] = true;
                        top_handle.glue_first_to_second(bottom_handle);
                    }
                    else //! top_path_follows_first &&
                         //! !bottom_path_follows_first
                    {
                        bottom_handle.flip();
                        top_handle.glue_second_to_first(bottom_handle);
                    }
                }

                // Finally, embed all edges (v,w) at their upper end points
                canonical_dfs_child[w]
                    = canonical_dfs_child[root_face_handle.first_vertex()];

                add_to_merge_points(
                    root_face_handle.get_anchor(), StoreOldHandlesPolicy());

                typename edge_vector_t::iterator ei, ei_end;
                ei_end = backedges[chosen].end();
                for (ei = backedges[chosen].begin(); ei != ei_end; ++ei)
                {
                    if (next_bottom_follows_first)
                        root_face_handle.push_first(*ei, g);
                    else
                        root_face_handle.push_second(*ei, g);
                }

                backedges[chosen].clear();
                curr_face_handle = root_face_handle;

            } // while(true)

        } // while(!pertinent_roots[v]->empty())

        return true;
    }

    void store_old_face_handles(graph::detail::no_old_handles) {}

    void store_old_face_handles(graph::detail::store_old_handles)
    {
        for (typename std::vector< vertex_t >::iterator mp_itr
             = current_merge_points.begin();
             mp_itr != current_merge_points.end(); ++mp_itr)
        {
            face_handles[*mp_itr].store_old_face_handles();
        }
        current_merge_points.clear();
    }

    void add_to_merge_points(vertex_t, graph::detail::no_old_handles) {}

    void add_to_merge_points(vertex_t v, graph::detail::store_old_handles)
    {
        current_merge_points.push_back(v);
    }

    void add_to_embedded_edges(edge_t, graph::detail::no_old_handles) {}

    void add_to_embedded_edges(edge_t e, graph::detail::store_old_handles)
    {
        embedded_edges.push_back(e);
    }

    void clean_up_embedding(graph::detail::no_embedding) {}

    void clean_up_embedding(graph::detail::store_embedding)
    {

        // If the graph isn't biconnected, we'll still have entries
        // in the separated_dfs_child_list for some vertices. Since
        // these represent articulation points, we can obtain a
        // planar embedding no matter what order we embed them in.

        vertex_iterator_t xi, xi_end;
        for (boost::tie(xi, xi_end) = vertices(g); xi != xi_end; ++xi)
        {
            if (!separated_dfs_child_list[*xi]->empty())
            {
                typename vertex_list_t::iterator yi, yi_end;
                yi_end = separated_dfs_child_list[*xi]->end();
                for (yi = separated_dfs_child_list[*xi]->begin(); yi != yi_end;
                     ++yi)
                {
                    dfs_child_handles[*yi].flip();
                    face_handles[*xi].glue_first_to_second(
                        dfs_child_handles[*yi]);
                }
            }
        }

        // Up until this point, we've flipped bicomps lazily by setting
        // flipped[v] to true if the bicomp rooted at v was flipped (the
        // lazy aspect of this flip is that all descendents of that vertex
        // need to have their orientations reversed as well). Now, we
        // traverse the DFS tree by DFS number and perform the actual
        // flipping as needed

        typedef typename vertex_vector_t::iterator vertex_vector_itr_t;
        vertex_vector_itr_t vi_end = vertices_by_dfs_num.end();
        for (vertex_vector_itr_t vi = vertices_by_dfs_num.begin(); vi != vi_end;
             ++vi)
        {
            vertex_t v(*vi);
            bool v_flipped = flipped[v];
            bool p_flipped = flipped[dfs_parent[v]];
            if (v_flipped && !p_flipped)
            {
                face_handles[v].flip();
            }
            else if (p_flipped && !v_flipped)
            {
                face_handles[v].flip();
                flipped[v] = true;
            }
            else
            {
                flipped[v] = false;
            }
        }

        // If there are any self-loops in the graph, they were flagged
        // during the walkup, and we should add them to the embedding now.
        // Adding a self loop anywhere in the embedding could never
        // invalidate the embedding, but they would complicate the traversal
        // if they were added during the walkup/walkdown.

        typename edge_vector_t::iterator ei, ei_end;
        ei_end = self_loops.end();
        for (ei = self_loops.begin(); ei != ei_end; ++ei)
        {
            edge_t e(*ei);
            face_handles[source(e, g)].push_second(e, g);
        }
    }

    bool pertinent(vertex_t w, vertex_t v)
    {
        // w is pertinent with respect to v if there is a backedge (v,w) or if
        // w is the root of a bicomp that contains a pertinent vertex.

        return backedge_flag[w] == dfs_number[v]
            || !pertinent_roots[w]->empty();
    }

    bool externally_active(vertex_t w, vertex_t v)
    {
        // Let a be any proper depth-first search ancestor of v. w is externally
        // active with respect to v if there exists a backedge (a,w) or a
        // backedge (a,w_0) for some w_0 in a descendent bicomp of w.

        v_size_t dfs_number_of_v = dfs_number[v];
        return (least_ancestor[w] < dfs_number_of_v)
            || (!separated_dfs_child_list[w]->empty()
                && low_point[separated_dfs_child_list[w]->front()]
                    < dfs_number_of_v);
    }

    bool internally_active(vertex_t w, vertex_t v)
    {
        return pertinent(w, v) && !externally_active(w, v);
    }

    void remove_vertex_from_separated_dfs_child_list(vertex_t v)
    {
        typename vertex_list_t::iterator to_delete
            = separated_node_in_parent_list[v];
        garbage.splice(garbage.end(), *separated_dfs_child_list[dfs_parent[v]],
            to_delete, boost::next(to_delete));
    }

    // End of the implementation of the basic Boyer-Myrvold Algorithm. The rest
    // of the code below implements the isolation of a Kuratowski subgraph in
    // the case that the input graph is not planar. This is by far the most
    // complicated part of the implementation.

public:
    template < typename EdgeToBoolPropertyMap, typename EdgeContainer >
    vertex_t kuratowski_walkup(vertex_t v, EdgeToBoolPropertyMap forbidden_edge,
        EdgeToBoolPropertyMap goal_edge, EdgeToBoolPropertyMap is_embedded,
        EdgeContainer& path_edges)
    {
        vertex_t current_endpoint;
        bool seen_goal_edge = false;
        out_edge_iterator_t oi, oi_end;

        for (boost::tie(oi, oi_end) = out_edges(v, g); oi != oi_end; ++oi)
            forbidden_edge[*oi] = true;

        for (boost::tie(oi, oi_end) = out_edges(v, g); oi != oi_end; ++oi)
        {
            path_edges.clear();

            edge_t e(*oi);
            current_endpoint
                = target(*oi, g) == v ? source(*oi, g) : target(*oi, g);

            if (dfs_number[current_endpoint] < dfs_number[v] || is_embedded[e]
                || v == current_endpoint // self-loop
            )
            {
                // Not a backedge
                continue;
            }

            path_edges.push_back(e);
            if (goal_edge[e])
            {
                return current_endpoint;
            }

            typedef typename face_edge_iterator<>::type walkup_itr_t;

            walkup_itr_t walkup_itr(
                current_endpoint, face_handles, first_side());
            walkup_itr_t walkup_end;

            seen_goal_edge = false;

            while (true)
            {

                if (walkup_itr != walkup_end && forbidden_edge[*walkup_itr])
                    break;

                while (walkup_itr != walkup_end && !goal_edge[*walkup_itr]
                    && !forbidden_edge[*walkup_itr])
                {
                    edge_t f(*walkup_itr);
                    forbidden_edge[f] = true;
                    path_edges.push_back(f);
                    current_endpoint = source(f, g) == current_endpoint
                        ? target(f, g)
                        : source(f, g);
                    ++walkup_itr;
                }

                if (walkup_itr != walkup_end && goal_edge[*walkup_itr])
                {
                    path_edges.push_back(*walkup_itr);
                    seen_goal_edge = true;
                    break;
                }

                walkup_itr = walkup_itr_t(
                    current_endpoint, face_handles, first_side());
            }

            if (seen_goal_edge)
                break;
        }

        if (seen_goal_edge)
            return current_endpoint;
        else
            return graph_traits< Graph >::null_vertex();
    }

    template < typename OutputIterator, typename EdgeIndexMap >
    void extract_kuratowski_subgraph(OutputIterator o_itr, EdgeIndexMap em)
    {

        // If the main algorithm has failed to embed one of the back-edges from
        // a vertex v, we can use the current state of the algorithm to isolate
        // a Kuratowksi subgraph. The isolation process breaks down into five
        // cases, A - E. The general configuration of all five cases is shown in
        //                  figure 1. There is a vertex v from which the planar
        //         v        embedding process could not proceed. This means that
        //         |        there exists some bicomp containing three vertices
        //       -----      x,y, and z as shown such that x and y are externally
        //      |     |     active with respect to v (which means that there are
        //      x     y     two vertices x_0 and y_0 such that (1) both x_0 and
        //      |     |     y_0 are proper depth-first search ancestors of v and
        //       --z--      (2) there are two disjoint paths, one connecting x
        //                  and x_0 and one connecting y and y_0, both
        //                  consisting
        //       fig. 1     entirely of unembedded edges). Furthermore, there
        //                  exists a vertex z_0 such that z is a depth-first
        // search ancestor of z_0 and (v,z_0) is an unembedded back-edge from v.
        // x,y and z all exist on the same bicomp, which consists entirely of
        // embedded edges. The five subcases break down as follows, and are
        // handled by the algorithm logically in the order A-E: First, if v is
        // not on the same bicomp as x,y, and z, a K_3_3 can be isolated - this
        // is case A. So, we'll assume that v is on the same bicomp as x,y, and
        // z. If z_0 is on a different bicomp than x,y, and z, a K_3_3 can also
        // be isolated - this is a case B - so we'll assume from now on that v
        // is on the same bicomp as x, y, and z=z_0. In this case, one can use
        // properties of the Boyer-Myrvold algorithm to show the existence of an
        // "x-y path" connecting some vertex on the "left side" of the x,y,z
        // bicomp with some vertex on the "right side" of the bicomp (where the
        // left and right are split by a line drawn through v and z.If either of
        // the endpoints of the x-y path is above x or y on the bicomp, a K_3_3
        // can be isolated - this is a case C. Otherwise, both endpoints are at
        // or below x and y on the bicomp. If there is a vertex alpha on the x-y
        // path such that alpha is not x or y and there's a path from alpha to v
        // that's disjoint from any of the edges on the bicomp and the x-y path,
        // a K_3_3 can be isolated - this is a case D. Otherwise, properties of
        // the Boyer-Myrvold algorithm can be used to show that another vertex
        // w exists on the lower half of the bicomp such that w is externally
        // active with respect to v. w can then be used to isolate a K_5 - this
        // is the configuration of case E.

        vertex_iterator_t vi, vi_end;
        edge_iterator_t ei, ei_end;
        out_edge_iterator_t oei, oei_end;
        typename std::vector< edge_t >::iterator xi, xi_end;

        // Clear the short-circuit edges - these are needed for the planar
        // testing/embedding algorithm to run in linear time, but they'll
        // complicate the kuratowski subgraph isolation
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            face_handles[*vi].reset_vertex_cache();
            dfs_child_handles[*vi].reset_vertex_cache();
        }

        vertex_t v = kuratowski_v;
        vertex_t x = kuratowski_x;
        vertex_t y = kuratowski_y;

        typedef iterator_property_map< typename std::vector< bool >::iterator,
            EdgeIndexMap >
            edge_to_bool_map_t;

        std::vector< bool > is_in_subgraph_vector(num_edges(g), false);
        edge_to_bool_map_t is_in_subgraph(is_in_subgraph_vector.begin(), em);

        std::vector< bool > is_embedded_vector(num_edges(g), false);
        edge_to_bool_map_t is_embedded(is_embedded_vector.begin(), em);

        typename std::vector< edge_t >::iterator embedded_itr, embedded_end;
        embedded_end = embedded_edges.end();
        for (embedded_itr = embedded_edges.begin();
             embedded_itr != embedded_end; ++embedded_itr)
            is_embedded[*embedded_itr] = true;

        // upper_face_vertex is true for x,y, and all vertices above x and y in
        // the bicomp
        std::vector< bool > upper_face_vertex_vector(num_vertices(g), false);
        vertex_to_bool_map_t upper_face_vertex(
            upper_face_vertex_vector.begin(), vm);

        std::vector< bool > lower_face_vertex_vector(num_vertices(g), false);
        vertex_to_bool_map_t lower_face_vertex(
            lower_face_vertex_vector.begin(), vm);

        // These next few variable declarations are all things that we need
        // to find.
        vertex_t z = graph_traits< Graph >::null_vertex();
        vertex_t bicomp_root;
        vertex_t w = graph_traits< Graph >::null_vertex();
        face_handle_t w_handle;
        face_handle_t v_dfchild_handle;
        vertex_t first_x_y_path_endpoint = graph_traits< Graph >::null_vertex();
        vertex_t second_x_y_path_endpoint
            = graph_traits< Graph >::null_vertex();
        vertex_t w_ancestor = v;

        detail::bm_case_t chosen_case = detail::BM_NO_CASE_CHOSEN;

        std::vector< edge_t > x_external_path;
        std::vector< edge_t > y_external_path;
        std::vector< edge_t > case_d_edges;

        std::vector< edge_t > z_v_path;
        std::vector< edge_t > w_path;

        // first, use a walkup to find a path from V that starts with a
        // backedge from V, then goes up until it hits either X or Y
        //(but doesn't find X or Y as the root of a bicomp)

        typename face_vertex_iterator<>::type x_upper_itr(
            x, face_handles, first_side());
        typename face_vertex_iterator<>::type x_lower_itr(
            x, face_handles, second_side());
        typename face_vertex_iterator<>::type face_itr, face_end;

        // Don't know which path from x is the upper or lower path -
        // we'll find out here
        for (face_itr = x_upper_itr; face_itr != face_end; ++face_itr)
        {
            if (*face_itr == y)
            {
                std::swap(x_upper_itr, x_lower_itr);
                break;
            }
        }

        upper_face_vertex[x] = true;

        vertex_t current_vertex = x;
        vertex_t previous_vertex;
        for (face_itr = x_upper_itr; face_itr != face_end; ++face_itr)
        {
            previous_vertex = current_vertex;
            current_vertex = *face_itr;
            upper_face_vertex[current_vertex] = true;
        }

        v_dfchild_handle
            = dfs_child_handles[canonical_dfs_child[previous_vertex]];

        for (face_itr = x_lower_itr; *face_itr != y; ++face_itr)
        {
            vertex_t current_vertex(*face_itr);
            lower_face_vertex[current_vertex] = true;

            typename face_handle_list_t::iterator roots_itr, roots_end;

            if (w == graph_traits< Graph >::null_vertex()) // haven't found a w
                                                           // yet
            {
                roots_end = pertinent_roots[current_vertex]->end();
                for (roots_itr = pertinent_roots[current_vertex]->begin();
                     roots_itr != roots_end; ++roots_itr)
                {
                    if (low_point
                            [canonical_dfs_child[roots_itr->first_vertex()]]
                        < dfs_number[v])
                    {
                        w = current_vertex;
                        w_handle = *roots_itr;
                        break;
                    }
                }
            }
        }

        for (; face_itr != face_end; ++face_itr)
        {
            vertex_t current_vertex(*face_itr);
            upper_face_vertex[current_vertex] = true;
            bicomp_root = current_vertex;
        }

        typedef typename face_edge_iterator<>::type walkup_itr_t;

        std::vector< bool > outer_face_edge_vector(num_edges(g), false);
        edge_to_bool_map_t outer_face_edge(outer_face_edge_vector.begin(), em);

        walkup_itr_t walkup_end;
        for (walkup_itr_t walkup_itr(x, face_handles, first_side());
             walkup_itr != walkup_end; ++walkup_itr)
        {
            outer_face_edge[*walkup_itr] = true;
            is_in_subgraph[*walkup_itr] = true;
        }

        for (walkup_itr_t walkup_itr(x, face_handles, second_side());
             walkup_itr != walkup_end; ++walkup_itr)
        {
            outer_face_edge[*walkup_itr] = true;
            is_in_subgraph[*walkup_itr] = true;
        }

        std::vector< bool > forbidden_edge_vector(num_edges(g), false);
        edge_to_bool_map_t forbidden_edge(forbidden_edge_vector.begin(), em);

        std::vector< bool > goal_edge_vector(num_edges(g), false);
        edge_to_bool_map_t goal_edge(goal_edge_vector.begin(), em);

        // Find external path to x and to y

        for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
        {
            edge_t e(*ei);
            goal_edge[e] = !outer_face_edge[e]
                && (source(e, g) == x || target(e, g) == x);
            forbidden_edge[*ei] = outer_face_edge[*ei];
        }

        vertex_t x_ancestor = v;
        vertex_t x_endpoint = graph_traits< Graph >::null_vertex();

        while (x_endpoint == graph_traits< Graph >::null_vertex())
        {
            x_ancestor = dfs_parent[x_ancestor];
            x_endpoint = kuratowski_walkup(x_ancestor, forbidden_edge,
                goal_edge, is_embedded, x_external_path);
        }

        for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
        {
            edge_t e(*ei);
            goal_edge[e] = !outer_face_edge[e]
                && (source(e, g) == y || target(e, g) == y);
            forbidden_edge[*ei] = outer_face_edge[*ei];
        }

        vertex_t y_ancestor = v;
        vertex_t y_endpoint = graph_traits< Graph >::null_vertex();

        while (y_endpoint == graph_traits< Graph >::null_vertex())
        {
            y_ancestor = dfs_parent[y_ancestor];
            y_endpoint = kuratowski_walkup(y_ancestor, forbidden_edge,
                goal_edge, is_embedded, y_external_path);
        }

        vertex_t parent, child;

        // If v isn't on the same bicomp as x and y, it's a case A
        if (bicomp_root != v)
        {
            chosen_case = detail::BM_CASE_A;

            for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
                if (lower_face_vertex[*vi])
                    for (boost::tie(oei, oei_end) = out_edges(*vi, g);
                         oei != oei_end; ++oei)
                        if (!outer_face_edge[*oei])
                            goal_edge[*oei] = true;

            for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
                forbidden_edge[*ei] = outer_face_edge[*ei];

            z = kuratowski_walkup(
                v, forbidden_edge, goal_edge, is_embedded, z_v_path);
        }
        else if (w != graph_traits< Graph >::null_vertex())
        {
            chosen_case = detail::BM_CASE_B;

            for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
            {
                edge_t e(*ei);
                goal_edge[e] = false;
                forbidden_edge[e] = outer_face_edge[e];
            }

            goal_edge[w_handle.first_edge()] = true;
            goal_edge[w_handle.second_edge()] = true;

            z = kuratowski_walkup(
                v, forbidden_edge, goal_edge, is_embedded, z_v_path);

            for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
            {
                forbidden_edge[*ei] = outer_face_edge[*ei];
            }

            typename std::vector< edge_t >::iterator pi, pi_end;
            pi_end = z_v_path.end();
            for (pi = z_v_path.begin(); pi != pi_end; ++pi)
            {
                goal_edge[*pi] = true;
            }

            w_ancestor = v;
            vertex_t w_endpoint = graph_traits< Graph >::null_vertex();

            while (w_endpoint == graph_traits< Graph >::null_vertex())
            {
                w_ancestor = dfs_parent[w_ancestor];
                w_endpoint = kuratowski_walkup(
                    w_ancestor, forbidden_edge, goal_edge, is_embedded, w_path);
            }

            // We really want both the w walkup and the z walkup to finish on
            // exactly the same edge, but for convenience (since we don't have
            // control over which side of a bicomp a walkup moves up) we've
            // defined the walkup to either end at w_handle.first_edge() or
            // w_handle.second_edge(). If both walkups ended at different edges,
            // we'll do a little surgery on the w walkup path to make it follow
            // the other side of the final bicomp.

            if ((w_path.back() == w_handle.first_edge()
                    && z_v_path.back() == w_handle.second_edge())
                || (w_path.back() == w_handle.second_edge()
                    && z_v_path.back() == w_handle.first_edge()))
            {
                walkup_itr_t wi, wi_end;
                edge_t final_edge = w_path.back();
                vertex_t anchor = source(final_edge, g) == w_handle.get_anchor()
                    ? target(final_edge, g)
                    : source(final_edge, g);
                if (face_handles[anchor].first_edge() == final_edge)
                    wi = walkup_itr_t(anchor, face_handles, second_side());
                else
                    wi = walkup_itr_t(anchor, face_handles, first_side());

                w_path.pop_back();

                for (; wi != wi_end; ++wi)
                {
                    edge_t e(*wi);
                    if (w_path.back() == e)
                        w_path.pop_back();
                    else
                        w_path.push_back(e);
                }
            }
        }
        else
        {

            // We need to find a valid z, since the x-y path re-defines the
            // lower face, and the z we found earlier may now be on the upper
            // face.

            chosen_case = detail::BM_CASE_E;

            // The z we've used so far is just an externally active vertex on
            // the lower face path, but may not be the z we need for a case C,
            // D, or E subgraph. the z we need now is any externally active
            // vertex on the lower face path with both old_face_handles edges on
            // the outer face. Since we know an x-y path exists, such a z must
            // also exist.

            // TODO: find this z in the first place.

            // find the new z

            for (face_itr = x_lower_itr; *face_itr != y; ++face_itr)
            {
                vertex_t possible_z(*face_itr);
                if (pertinent(possible_z, v)
                    && outer_face_edge[face_handles[possible_z]
                                           .old_first_edge()]
                    && outer_face_edge[face_handles[possible_z]
                                           .old_second_edge()])
                {
                    z = possible_z;
                    break;
                }
            }

            // find x-y path, and a w if one exists.

            if (externally_active(z, v))
                w = z;

            typedef typename face_edge_iterator< single_side,
                previous_iteration >::type old_face_iterator_t;

            old_face_iterator_t first_old_face_itr(
                z, face_handles, first_side());
            old_face_iterator_t second_old_face_itr(
                z, face_handles, second_side());
            old_face_iterator_t old_face_itr, old_face_end;

            std::vector< old_face_iterator_t > old_face_iterators;
            old_face_iterators.push_back(first_old_face_itr);
            old_face_iterators.push_back(second_old_face_itr);

            std::vector< bool > x_y_path_vertex_vector(num_vertices(g), false);
            vertex_to_bool_map_t x_y_path_vertex(
                x_y_path_vertex_vector.begin(), vm);

            typename std::vector< old_face_iterator_t >::iterator of_itr,
                of_itr_end;
            of_itr_end = old_face_iterators.end();
            for (of_itr = old_face_iterators.begin(); of_itr != of_itr_end;
                 ++of_itr)
            {

                old_face_itr = *of_itr;

                vertex_t previous_vertex;
                bool seen_x_or_y = false;
                vertex_t current_vertex = z;
                for (; old_face_itr != old_face_end; ++old_face_itr)
                {
                    edge_t e(*old_face_itr);
                    previous_vertex = current_vertex;
                    current_vertex = source(e, g) == current_vertex
                        ? target(e, g)
                        : source(e, g);

                    if (current_vertex == x || current_vertex == y)
                        seen_x_or_y = true;

                    if (w == graph_traits< Graph >::null_vertex()
                        && externally_active(current_vertex, v)
                        && outer_face_edge[e]
                        && outer_face_edge[*boost::next(old_face_itr)]
                        && !seen_x_or_y)
                    {
                        w = current_vertex;
                    }

                    if (!outer_face_edge[e])
                    {
                        if (!upper_face_vertex[current_vertex]
                            && !lower_face_vertex[current_vertex])
                        {
                            x_y_path_vertex[current_vertex] = true;
                        }

                        is_in_subgraph[e] = true;
                        if (upper_face_vertex[source(e, g)]
                            || lower_face_vertex[source(e, g)])
                        {
                            if (first_x_y_path_endpoint
                                == graph_traits< Graph >::null_vertex())
                                first_x_y_path_endpoint = source(e, g);
                            else
                                second_x_y_path_endpoint = source(e, g);
                        }
                        if (upper_face_vertex[target(e, g)]
                            || lower_face_vertex[target(e, g)])
                        {
                            if (first_x_y_path_endpoint
                                == graph_traits< Graph >::null_vertex())
                                first_x_y_path_endpoint = target(e, g);
                            else
                                second_x_y_path_endpoint = target(e, g);
                        }
                    }
                    else if (previous_vertex == x || previous_vertex == y)
                    {
                        chosen_case = detail::BM_CASE_C;
                    }
                }
            }

            // Look for a case D - one of v's embedded edges will connect to the
            // x-y path along an inner face path.

            // First, get a list of all of v's embedded child edges

            out_edge_iterator_t v_edge_itr, v_edge_end;
            for (boost::tie(v_edge_itr, v_edge_end) = out_edges(v, g);
                 v_edge_itr != v_edge_end; ++v_edge_itr)
            {
                edge_t embedded_edge(*v_edge_itr);

                if (!is_embedded[embedded_edge]
                    || embedded_edge == dfs_parent_edge[v])
                    continue;

                case_d_edges.push_back(embedded_edge);

                vertex_t current_vertex = source(embedded_edge, g) == v
                    ? target(embedded_edge, g)
                    : source(embedded_edge, g);

                typename face_edge_iterator<>::type internal_face_itr,
                    internal_face_end;
                if (face_handles[current_vertex].first_vertex() == v)
                {
                    internal_face_itr = typename face_edge_iterator<>::type(
                        current_vertex, face_handles, second_side());
                }
                else
                {
                    internal_face_itr = typename face_edge_iterator<>::type(
                        current_vertex, face_handles, first_side());
                }

                while (internal_face_itr != internal_face_end
                    && !outer_face_edge[*internal_face_itr]
                    && !x_y_path_vertex[current_vertex])
                {
                    edge_t e(*internal_face_itr);
                    case_d_edges.push_back(e);
                    current_vertex = source(e, g) == current_vertex
                        ? target(e, g)
                        : source(e, g);
                    ++internal_face_itr;
                }

                if (x_y_path_vertex[current_vertex])
                {
                    chosen_case = detail::BM_CASE_D;
                    break;
                }
                else
                {
                    case_d_edges.clear();
                }
            }
        }

        if (chosen_case != detail::BM_CASE_B
            && chosen_case != detail::BM_CASE_A)
        {

            // Finding z and w.

            for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
            {
                edge_t e(*ei);
                goal_edge[e] = !outer_face_edge[e]
                    && (source(e, g) == z || target(e, g) == z);
                forbidden_edge[e] = outer_face_edge[e];
            }

            kuratowski_walkup(
                v, forbidden_edge, goal_edge, is_embedded, z_v_path);

            if (chosen_case == detail::BM_CASE_E)
            {

                for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
                {
                    forbidden_edge[*ei] = outer_face_edge[*ei];
                    goal_edge[*ei] = !outer_face_edge[*ei]
                        && (source(*ei, g) == w || target(*ei, g) == w);
                }

                for (boost::tie(oei, oei_end) = out_edges(w, g); oei != oei_end;
                     ++oei)
                {
                    if (!outer_face_edge[*oei])
                        goal_edge[*oei] = true;
                }

                typename std::vector< edge_t >::iterator pi, pi_end;
                pi_end = z_v_path.end();
                for (pi = z_v_path.begin(); pi != pi_end; ++pi)
                {
                    goal_edge[*pi] = true;
                }

                w_ancestor = v;
                vertex_t w_endpoint = graph_traits< Graph >::null_vertex();

                while (w_endpoint == graph_traits< Graph >::null_vertex())
                {
                    w_ancestor = dfs_parent[w_ancestor];
                    w_endpoint = kuratowski_walkup(w_ancestor, forbidden_edge,
                        goal_edge, is_embedded, w_path);
                }
            }
        }

        // We're done isolating the Kuratowski subgraph at this point -
        // but there's still some cleaning up to do.

        // Update is_in_subgraph with the paths we just found

        xi_end = x_external_path.end();
        for (xi = x_external_path.begin(); xi != xi_end; ++xi)
            is_in_subgraph[*xi] = true;

        xi_end = y_external_path.end();
        for (xi = y_external_path.begin(); xi != xi_end; ++xi)
            is_in_subgraph[*xi] = true;

        xi_end = z_v_path.end();
        for (xi = z_v_path.begin(); xi != xi_end; ++xi)
            is_in_subgraph[*xi] = true;

        xi_end = case_d_edges.end();
        for (xi = case_d_edges.begin(); xi != xi_end; ++xi)
            is_in_subgraph[*xi] = true;

        xi_end = w_path.end();
        for (xi = w_path.begin(); xi != xi_end; ++xi)
            is_in_subgraph[*xi] = true;

        child = bicomp_root;
        parent = dfs_parent[child];
        while (child != parent)
        {
            is_in_subgraph[dfs_parent_edge[child]] = true;
            boost::tie(parent, child)
                = std::make_pair(dfs_parent[parent], parent);
        }

        // At this point, we've already isolated the Kuratowski subgraph and
        // collected all of the edges that compose it in the is_in_subgraph
        // property map. But we want the verification of such a subgraph to be
        // a deterministic process, and we can simplify the function
        // is_kuratowski_subgraph by cleaning up some edges here.

        if (chosen_case == detail::BM_CASE_B)
        {
            is_in_subgraph[dfs_parent_edge[v]] = false;
        }
        else if (chosen_case == detail::BM_CASE_C)
        {
            // In a case C subgraph, at least one of the x-y path endpoints
            // (call it alpha) is above either x or y on the outer face. The
            // other endpoint may be attached at x or y OR above OR below. In
            // any of these three cases, we can form a K_3_3 by removing the
            // edge attached to v on the outer face that is NOT on the path to
            // alpha.

            typename face_vertex_iterator< single_side, follow_visitor >::type
                face_itr,
                face_end;
            if (face_handles[v_dfchild_handle.first_vertex()].first_edge()
                == v_dfchild_handle.first_edge())
            {
                face_itr = typename face_vertex_iterator< single_side,
                    follow_visitor >::type(v_dfchild_handle.first_vertex(),
                    face_handles, second_side());
            }
            else
            {
                face_itr = typename face_vertex_iterator< single_side,
                    follow_visitor >::type(v_dfchild_handle.first_vertex(),
                    face_handles, first_side());
            }

            for (; true; ++face_itr)
            {
                vertex_t current_vertex(*face_itr);
                if (current_vertex == x || current_vertex == y)
                {
                    is_in_subgraph[v_dfchild_handle.first_edge()] = false;
                    break;
                }
                else if (current_vertex == first_x_y_path_endpoint
                    || current_vertex == second_x_y_path_endpoint)
                {
                    is_in_subgraph[v_dfchild_handle.second_edge()] = false;
                    break;
                }
            }
        }
        else if (chosen_case == detail::BM_CASE_D)
        {
            // Need to remove both of the edges adjacent to v on the outer face.
            // remove the connecting edges from v to bicomp, then
            // is_kuratowski_subgraph will shrink vertices of degree 1
            // automatically...

            is_in_subgraph[v_dfchild_handle.first_edge()] = false;
            is_in_subgraph[v_dfchild_handle.second_edge()] = false;
        }
        else if (chosen_case == detail::BM_CASE_E)
        {
            // Similarly to case C, if the endpoints of the x-y path are both
            // below x and y, we should remove an edge to allow the subgraph to
            // contract to a K_3_3.

            if ((first_x_y_path_endpoint != x && first_x_y_path_endpoint != y)
                || (second_x_y_path_endpoint != x
                    && second_x_y_path_endpoint != y))
            {
                is_in_subgraph[dfs_parent_edge[v]] = false;

                vertex_t deletion_endpoint, other_endpoint;
                if (lower_face_vertex[first_x_y_path_endpoint])
                {
                    deletion_endpoint = second_x_y_path_endpoint;
                    other_endpoint = first_x_y_path_endpoint;
                }
                else
                {
                    deletion_endpoint = first_x_y_path_endpoint;
                    other_endpoint = second_x_y_path_endpoint;
                }

                typename face_edge_iterator<>::type face_itr, face_end;

                bool found_other_endpoint = false;
                for (face_itr = typename face_edge_iterator<>::type(
                         deletion_endpoint, face_handles, first_side());
                     face_itr != face_end; ++face_itr)
                {
                    edge_t e(*face_itr);
                    if (source(e, g) == other_endpoint
                        || target(e, g) == other_endpoint)
                    {
                        found_other_endpoint = true;
                        break;
                    }
                }

                if (found_other_endpoint)
                {
                    is_in_subgraph[face_handles[deletion_endpoint].first_edge()]
                        = false;
                }
                else
                {
                    is_in_subgraph[face_handles[deletion_endpoint]
                                       .second_edge()]
                        = false;
                }
            }
        }

        for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
            if (is_in_subgraph[*ei])
                *o_itr = *ei;
    }

    template < typename EdgePermutation >
    void make_edge_permutation(EdgePermutation perm)
    {
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            vertex_t v(*vi);
            perm[v].clear();
            face_handles[v].get_list(std::back_inserter(perm[v]));
        }
    }

private:
    const Graph& g;
    VertexIndexMap vm;

    vertex_t kuratowski_v;
    vertex_t kuratowski_x;
    vertex_t kuratowski_y;

    vertex_list_t garbage; // we delete items from linked lists by
                           // splicing them into garbage

    // only need these two for kuratowski subgraph isolation
    std::vector< vertex_t > current_merge_points;
    std::vector< edge_t > embedded_edges;

    // property map storage
    std::vector< v_size_t > low_point_vector;
    std::vector< vertex_t > dfs_parent_vector;
    std::vector< v_size_t > dfs_number_vector;
    std::vector< v_size_t > least_ancestor_vector;
    std::vector< face_handle_list_ptr_t > pertinent_roots_vector;
    std::vector< v_size_t > backedge_flag_vector;
    std::vector< v_size_t > visited_vector;
    std::vector< face_handle_t > face_handles_vector;
    std::vector< face_handle_t > dfs_child_handles_vector;
    std::vector< vertex_list_ptr_t > separated_dfs_child_list_vector;
    std::vector< typename vertex_list_t::iterator >
        separated_node_in_parent_list_vector;
    std::vector< vertex_t > canonical_dfs_child_vector;
    std::vector< bool > flipped_vector;
    std::vector< edge_vector_t > backedges_vector;
    edge_vector_t self_loops;
    std::vector< edge_t > dfs_parent_edge_vector;
    vertex_vector_t vertices_by_dfs_num;

    // property maps
    vertex_to_v_size_map_t low_point;
    vertex_to_vertex_map_t dfs_parent;
    vertex_to_v_size_map_t dfs_number;
    vertex_to_v_size_map_t least_ancestor;
    vertex_to_face_handle_list_ptr_map_t pertinent_roots;
    vertex_to_v_size_map_t backedge_flag;
    vertex_to_v_size_map_t visited;
    vertex_to_face_handle_map_t face_handles;
    vertex_to_face_handle_map_t dfs_child_handles;
    vertex_to_vertex_list_ptr_map_t separated_dfs_child_list;
    vertex_to_separated_node_map_t separated_node_in_parent_list;
    vertex_to_vertex_map_t canonical_dfs_child;
    vertex_to_bool_map_t flipped;
    vertex_to_edge_vector_map_t backedges;
    vertex_to_edge_map_t dfs_parent_edge; // only need for kuratowski

    merge_stack_t merge_stack;
};

} // namespace boost

#endif //__BOYER_MYRVOLD_IMPL_HPP__
