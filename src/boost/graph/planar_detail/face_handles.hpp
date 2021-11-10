//=======================================================================
// Copyright (c) Aaron Windsor 2007
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef __FACE_HANDLES_HPP__
#define __FACE_HANDLES_HPP__

#include <list>
#include <boost/graph/graph_traits.hpp>
#include <boost/shared_ptr.hpp>

// A "face handle" is an optimization meant to serve two purposes in
// the implementation of the Boyer-Myrvold planarity test: (1) it holds
// the partial planar embedding of a particular vertex as it's being
// constructed, and (2) it allows for efficient traversal around the
// outer face of the partial embedding at that particular vertex. A face
// handle is lightweight, just a shared pointer to the actual implementation,
// since it is passed around/copied liberally in the algorithm. It consists
// of an "anchor" (the actual vertex it's associated with) as well as a
// sequence of edges. The functions first_vertex/second_vertex and
// first_edge/second_edge allow fast access to the beginning and end of the
// stored sequence, which allows one to traverse the outer face of the partial
// planar embedding as it's being created.
//
// There are some policies below that define the contents of the face handles:
// in the case no embedding is needed (for example, if one just wants to use
// the Boyer-Myrvold algorithm as a true/false test for planarity, the
// no_embedding class can be passed as the StoreEmbedding policy. Otherwise,
// either std_list (which uses as std::list) or recursive_lazy_list can be
// passed as this policy. recursive_lazy_list has the best theoretical
// performance (O(n) for a sequence of interleaved concatenations and reversals
// of the underlying list), but I've noticed little difference between std_list
// and recursive_lazy_list in my tests, even though using std_list changes
// the worst-case complexity of the planarity test to O(n^2)
//
// Another policy is StoreOldHandlesPolicy, which specifies whether or not
// to keep a record of the previous first/second vertex/edge - this is needed
// if a Kuratowski subgraph needs to be isolated.

namespace boost
{
namespace graph
{
    namespace detail
    {

        // face handle policies

        // EmbeddingStorage policy
        struct store_embedding
        {
        };
        struct recursive_lazy_list : public store_embedding
        {
        };
        struct std_list : public store_embedding
        {
        };
        struct no_embedding
        {
        };

        // StoreOldHandlesPolicy
        struct store_old_handles
        {
        };
        struct no_old_handles
        {
        };

        template < typename DataType > struct lazy_list_node
        {
            typedef shared_ptr< lazy_list_node< DataType > > ptr_t;

            lazy_list_node(const DataType& data)
            : m_reversed(false), m_data(data), m_has_data(true)
            {
            }

            lazy_list_node(ptr_t left_child, ptr_t right_child)
            : m_reversed(false)
            , m_has_data(false)
            , m_left_child(left_child)
            , m_right_child(right_child)
            {
            }

            bool m_reversed;
            DataType m_data;
            bool m_has_data;
            shared_ptr< lazy_list_node > m_left_child;
            shared_ptr< lazy_list_node > m_right_child;
        };

        template < typename StoreOldHandlesPolicy, typename Vertex,
            typename Edge >
        struct old_handles_storage;

        template < typename Vertex, typename Edge >
        struct old_handles_storage< store_old_handles, Vertex, Edge >
        {
            Vertex first_vertex;
            Vertex second_vertex;
            Edge first_edge;
            Edge second_edge;
        };

        template < typename Vertex, typename Edge >
        struct old_handles_storage< no_old_handles, Vertex, Edge >
        {
        };

        template < typename StoreEmbeddingPolicy, typename Edge >
        struct edge_list_storage;

        template < typename Edge >
        struct edge_list_storage< no_embedding, Edge >
        {
            typedef void type;

            void push_back(Edge) {}
            void push_front(Edge) {}
            void reverse() {}
            void concat_front(edge_list_storage< no_embedding, Edge >) {}
            void concat_back(edge_list_storage< no_embedding, Edge >) {}
            template < typename OutputIterator > void get_list(OutputIterator)
            {
            }
        };

        template < typename Edge >
        struct edge_list_storage< recursive_lazy_list, Edge >
        {
            typedef lazy_list_node< Edge > node_type;
            typedef shared_ptr< node_type > type;
            type value;

            void push_back(Edge e)
            {
                type new_node(new node_type(e));
                value = type(new node_type(value, new_node));
            }

            void push_front(Edge e)
            {
                type new_node(new node_type(e));
                value = type(new node_type(new_node, value));
            }

            void reverse() { value->m_reversed = !value->m_reversed; }

            void concat_front(
                edge_list_storage< recursive_lazy_list, Edge > other)
            {
                value = type(new node_type(other.value, value));
            }

            void concat_back(
                edge_list_storage< recursive_lazy_list, Edge > other)
            {
                value = type(new node_type(value, other.value));
            }

            template < typename OutputIterator >
            void get_list(OutputIterator out)
            {
                get_list_helper(out, value);
            }

        private:
            template < typename OutputIterator >
            void get_list_helper(
                OutputIterator o_itr, type root, bool flipped = false)
            {
                if (!root)
                    return;

                if (root->m_has_data)
                    *o_itr = root->m_data;

                if ((flipped && !root->m_reversed)
                    || (!flipped && root->m_reversed))
                {
                    get_list_helper(o_itr, root->m_right_child, true);
                    get_list_helper(o_itr, root->m_left_child, true);
                }
                else
                {
                    get_list_helper(o_itr, root->m_left_child, false);
                    get_list_helper(o_itr, root->m_right_child, false);
                }
            }
        };

        template < typename Edge > struct edge_list_storage< std_list, Edge >
        {
            typedef std::list< Edge > type;
            type value;

            void push_back(Edge e) { value.push_back(e); }

            void push_front(Edge e) { value.push_front(e); }

            void reverse() { value.reverse(); }

            void concat_front(edge_list_storage< std_list, Edge > other)
            {
                value.splice(value.begin(), other.value);
            }

            void concat_back(edge_list_storage< std_list, Edge > other)
            {
                value.splice(value.end(), other.value);
            }

            template < typename OutputIterator >
            void get_list(OutputIterator out)
            {
                std::copy(value.begin(), value.end(), out);
            }
        };

        template < typename Graph, typename StoreOldHandlesPolicy,
            typename StoreEmbeddingPolicy >
        struct face_handle_impl
        {
            typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
            typedef typename graph_traits< Graph >::edge_descriptor edge_t;
            typedef
                typename edge_list_storage< StoreEmbeddingPolicy, edge_t >::type
                    edge_list_storage_t;

            face_handle_impl()
            : cached_first_vertex(graph_traits< Graph >::null_vertex())
            , cached_second_vertex(graph_traits< Graph >::null_vertex())
            , true_first_vertex(graph_traits< Graph >::null_vertex())
            , true_second_vertex(graph_traits< Graph >::null_vertex())
            , anchor(graph_traits< Graph >::null_vertex())
            {
                initialize_old_vertices_dispatch(StoreOldHandlesPolicy());
            }

            void initialize_old_vertices_dispatch(store_old_handles)
            {
                old_handles.first_vertex = graph_traits< Graph >::null_vertex();
                old_handles.second_vertex
                    = graph_traits< Graph >::null_vertex();
            }

            void initialize_old_vertices_dispatch(no_old_handles) {}

            vertex_t cached_first_vertex;
            vertex_t cached_second_vertex;
            vertex_t true_first_vertex;
            vertex_t true_second_vertex;
            vertex_t anchor;
            edge_t cached_first_edge;
            edge_t cached_second_edge;

            edge_list_storage< StoreEmbeddingPolicy, edge_t > edge_list;
            old_handles_storage< StoreOldHandlesPolicy, vertex_t, edge_t >
                old_handles;
        };

        template < typename Graph,
            typename StoreOldHandlesPolicy = store_old_handles,
            typename StoreEmbeddingPolicy = recursive_lazy_list >
        class face_handle
        {
        public:
            typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
            typedef typename graph_traits< Graph >::edge_descriptor edge_t;
            typedef face_handle_impl< Graph, StoreOldHandlesPolicy,
                StoreEmbeddingPolicy >
                impl_t;
            typedef face_handle< Graph, StoreOldHandlesPolicy,
                StoreEmbeddingPolicy >
                self_t;

            face_handle(vertex_t anchor = graph_traits< Graph >::null_vertex())
            : pimpl(new impl_t())
            {
                pimpl->anchor = anchor;
            }

            face_handle(vertex_t anchor, edge_t initial_edge, const Graph& g)
            : pimpl(new impl_t())
            {
                vertex_t s(source(initial_edge, g));
                vertex_t t(target(initial_edge, g));
                vertex_t other_vertex = s == anchor ? t : s;
                pimpl->anchor = anchor;
                pimpl->cached_first_edge = initial_edge;
                pimpl->cached_second_edge = initial_edge;
                pimpl->cached_first_vertex = other_vertex;
                pimpl->cached_second_vertex = other_vertex;
                pimpl->true_first_vertex = other_vertex;
                pimpl->true_second_vertex = other_vertex;

                pimpl->edge_list.push_back(initial_edge);
                store_old_face_handles_dispatch(StoreOldHandlesPolicy());
            }

            // default copy construction, assignment okay.

            void push_first(edge_t e, const Graph& g)
            {
                pimpl->edge_list.push_front(e);
                pimpl->cached_first_vertex = pimpl->true_first_vertex
                    = source(e, g) == pimpl->anchor ? target(e, g)
                                                    : source(e, g);
                pimpl->cached_first_edge = e;
            }

            void push_second(edge_t e, const Graph& g)
            {
                pimpl->edge_list.push_back(e);
                pimpl->cached_second_vertex = pimpl->true_second_vertex
                    = source(e, g) == pimpl->anchor ? target(e, g)
                                                    : source(e, g);
                pimpl->cached_second_edge = e;
            }

            inline void store_old_face_handles()
            {
                store_old_face_handles_dispatch(StoreOldHandlesPolicy());
            }

            inline vertex_t first_vertex() const
            {
                return pimpl->cached_first_vertex;
            }

            inline vertex_t second_vertex() const
            {
                return pimpl->cached_second_vertex;
            }

            inline vertex_t true_first_vertex() const
            {
                return pimpl->true_first_vertex;
            }

            inline vertex_t true_second_vertex() const
            {
                return pimpl->true_second_vertex;
            }

            inline vertex_t old_first_vertex() const
            {
                return pimpl->old_handles.first_vertex;
            }

            inline vertex_t old_second_vertex() const
            {
                return pimpl->old_handles.second_vertex;
            }

            inline edge_t old_first_edge() const
            {
                return pimpl->old_handles.first_edge;
            }

            inline edge_t old_second_edge() const
            {
                return pimpl->old_handles.second_edge;
            }

            inline edge_t first_edge() const
            {
                return pimpl->cached_first_edge;
            }

            inline edge_t second_edge() const
            {
                return pimpl->cached_second_edge;
            }

            inline vertex_t get_anchor() const { return pimpl->anchor; }

            void glue_first_to_second(face_handle< Graph, StoreOldHandlesPolicy,
                StoreEmbeddingPolicy >& bottom)
            {
                pimpl->edge_list.concat_front(bottom.pimpl->edge_list);
                pimpl->true_first_vertex = bottom.pimpl->true_first_vertex;
                pimpl->cached_first_vertex = bottom.pimpl->cached_first_vertex;
                pimpl->cached_first_edge = bottom.pimpl->cached_first_edge;
            }

            void glue_second_to_first(face_handle< Graph, StoreOldHandlesPolicy,
                StoreEmbeddingPolicy >& bottom)
            {
                pimpl->edge_list.concat_back(bottom.pimpl->edge_list);
                pimpl->true_second_vertex = bottom.pimpl->true_second_vertex;
                pimpl->cached_second_vertex
                    = bottom.pimpl->cached_second_vertex;
                pimpl->cached_second_edge = bottom.pimpl->cached_second_edge;
            }

            void flip()
            {
                pimpl->edge_list.reverse();
                std::swap(pimpl->true_first_vertex, pimpl->true_second_vertex);
                std::swap(
                    pimpl->cached_first_vertex, pimpl->cached_second_vertex);
                std::swap(pimpl->cached_first_edge, pimpl->cached_second_edge);
            }

            template < typename OutputIterator >
            void get_list(OutputIterator o_itr)
            {
                pimpl->edge_list.get_list(o_itr);
            }

            void reset_vertex_cache()
            {
                pimpl->cached_first_vertex = pimpl->true_first_vertex;
                pimpl->cached_second_vertex = pimpl->true_second_vertex;
            }

            inline void set_first_vertex(vertex_t v)
            {
                pimpl->cached_first_vertex = v;
            }

            inline void set_second_vertex(vertex_t v)
            {
                pimpl->cached_second_vertex = v;
            }

        private:
            void store_old_face_handles_dispatch(store_old_handles)
            {
                pimpl->old_handles.first_vertex = pimpl->true_first_vertex;
                pimpl->old_handles.second_vertex = pimpl->true_second_vertex;
                pimpl->old_handles.first_edge = pimpl->cached_first_edge;
                pimpl->old_handles.second_edge = pimpl->cached_second_edge;
            }

            void store_old_face_handles_dispatch(no_old_handles) {}

            boost::shared_ptr< impl_t > pimpl;
        };

    } /* namespace detail */
} /* namespace graph */
} /* namespace boost */

#endif //__FACE_HANDLES_HPP__
