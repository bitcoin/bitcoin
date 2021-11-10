//=======================================================================
// Copyright (c) 2018 Yi Ji
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//=======================================================================

#ifndef BOOST_GRAPH_MAXIMUM_WEIGHTED_MATCHING_HPP
#define BOOST_GRAPH_MAXIMUM_WEIGHTED_MATCHING_HPP

#include <algorithm> // for std::iter_swap
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/graph/max_cardinality_matching.hpp>

namespace boost
{
template < typename Graph, typename MateMap, typename VertexIndexMap >
typename property_traits<
    typename property_map< Graph, edge_weight_t >::type >::value_type
matching_weight_sum(const Graph& g, MateMap mate, VertexIndexMap vm)
{
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef
        typename graph_traits< Graph >::vertex_descriptor vertex_descriptor_t;
    typedef typename property_traits< typename property_map< Graph,
        edge_weight_t >::type >::value_type edge_property_t;

    edge_property_t weight_sum = 0;
    vertex_iterator_t vi, vi_end;

    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    {
        vertex_descriptor_t v = *vi;
        if (get(mate, v) != graph_traits< Graph >::null_vertex()
            && get(vm, v) < get(vm, get(mate, v)))
            weight_sum += get(edge_weight, g, edge(v, mate[v], g).first);
    }
    return weight_sum;
}

template < typename Graph, typename MateMap >
inline typename property_traits<
    typename property_map< Graph, edge_weight_t >::type >::value_type
matching_weight_sum(const Graph& g, MateMap mate)
{
    return matching_weight_sum(g, mate, get(vertex_index, g));
}

template < typename Graph, typename MateMap, typename VertexIndexMap >
class weighted_augmenting_path_finder
{
public:
    template < typename T > struct map_vertex_to_
    {
        typedef boost::iterator_property_map<
            typename std::vector< T >::iterator, VertexIndexMap >
            type;
    };
    typedef typename graph::detail::VERTEX_STATE vertex_state_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef
        typename graph_traits< Graph >::vertex_descriptor vertex_descriptor_t;
    typedef typename std::vector< vertex_descriptor_t >::const_iterator
        vertex_vec_iter_t;
    typedef
        typename graph_traits< Graph >::out_edge_iterator out_edge_iterator_t;
    typedef typename graph_traits< Graph >::edge_descriptor edge_descriptor_t;
    typedef typename graph_traits< Graph >::edge_iterator edge_iterator_t;
    typedef typename property_traits< typename property_map< Graph,
        edge_weight_t >::type >::value_type edge_property_t;
    typedef std::deque< vertex_descriptor_t > vertex_list_t;
    typedef std::vector< edge_descriptor_t > edge_list_t;
    typedef typename map_vertex_to_< vertex_descriptor_t >::type
        vertex_to_vertex_map_t;
    typedef
        typename map_vertex_to_< edge_property_t >::type vertex_to_weight_map_t;
    typedef typename map_vertex_to_< bool >::type vertex_to_bool_map_t;
    typedef typename map_vertex_to_< std::pair< vertex_descriptor_t,
        vertex_descriptor_t > >::type vertex_to_pair_map_t;
    typedef
        typename map_vertex_to_< std::pair< edge_descriptor_t, bool > >::type
            vertex_to_edge_map_t;
    typedef typename map_vertex_to_< vertex_to_edge_map_t >::type
        vertex_pair_to_edge_map_t;

    class blossom
    {
    public:
        typedef boost::shared_ptr< blossom > blossom_ptr_t;
        std::vector< blossom_ptr_t > sub_blossoms;
        edge_property_t dual_var;
        blossom_ptr_t father;

        blossom() : dual_var(0), father(blossom_ptr_t()) {}

        // get the base vertex of a blossom by recursively getting
        // its base sub-blossom, which is always the first one in
        // sub_blossoms because of how we create and maintain blossoms
        virtual vertex_descriptor_t get_base() const
        {
            const blossom* b = this;
            while (!b->sub_blossoms.empty())
                b = b->sub_blossoms[0].get();
            return b->get_base();
        }

        // set a sub-blossom as a blossom's base by exchanging it
        // with its first sub-blossom
        void set_base(const blossom_ptr_t& sub)
        {
            for (blossom_iterator_t bi = sub_blossoms.begin();
                 bi != sub_blossoms.end(); ++bi)
            {
                if (sub.get() == bi->get())
                {
                    std::iter_swap(sub_blossoms.begin(), bi);
                    break;
                }
            }
        }

        // get all vertices inside recursively
        virtual std::vector< vertex_descriptor_t > vertices() const
        {
            std::vector< vertex_descriptor_t > all_vertices;
            for (typename std::vector< blossom_ptr_t >::const_iterator bi
                 = sub_blossoms.begin();
                 bi != sub_blossoms.end(); ++bi)
            {
                std::vector< vertex_descriptor_t > some_vertices
                    = (*bi)->vertices();
                all_vertices.insert(all_vertices.end(), some_vertices.begin(),
                    some_vertices.end());
            }
            return all_vertices;
        }
    };

    // a trivial_blossom only has one vertex and no sub-blossom;
    // for each vertex v, in_blossom[v] is the trivial_blossom that contains it
    // directly
    class trivial_blossom : public blossom
    {
    public:
        trivial_blossom(vertex_descriptor_t v) : trivial_vertex(v) {}
        virtual vertex_descriptor_t get_base() const { return trivial_vertex; }

        virtual std::vector< vertex_descriptor_t > vertices() const
        {
            std::vector< vertex_descriptor_t > all_vertices;
            all_vertices.push_back(trivial_vertex);
            return all_vertices;
        }

    private:
        vertex_descriptor_t trivial_vertex;
    };

    typedef boost::shared_ptr< blossom > blossom_ptr_t;
    typedef typename std::vector< blossom_ptr_t >::iterator blossom_iterator_t;
    typedef
        typename map_vertex_to_< blossom_ptr_t >::type vertex_to_blossom_map_t;

    weighted_augmenting_path_finder(
        const Graph& arg_g, MateMap arg_mate, VertexIndexMap arg_vm)
    : g(arg_g)
    , vm(arg_vm)
    , null_edge(std::pair< edge_descriptor_t, bool >(
          num_edges(g) == 0 ? edge_descriptor_t() : *edges(g).first, false))
    , mate_vector(num_vertices(g))
    , label_S_vector(num_vertices(g), graph_traits< Graph >::null_vertex())
    , label_T_vector(num_vertices(g), graph_traits< Graph >::null_vertex())
    , outlet_vector(num_vertices(g), graph_traits< Graph >::null_vertex())
    , tau_idx_vector(num_vertices(g), graph_traits< Graph >::null_vertex())
    , dual_var_vector(std::vector< edge_property_t >(
          num_vertices(g), std::numeric_limits< edge_property_t >::min()))
    , pi_vector(std::vector< edge_property_t >(
          num_vertices(g), std::numeric_limits< edge_property_t >::max()))
    , gamma_vector(std::vector< edge_property_t >(
          num_vertices(g), std::numeric_limits< edge_property_t >::max()))
    , tau_vector(std::vector< edge_property_t >(
          num_vertices(g), std::numeric_limits< edge_property_t >::max()))
    , in_blossom_vector(num_vertices(g))
    , old_label_vector(num_vertices(g))
    , critical_edge_vectors(num_vertices(g),
          std::vector< std::pair< edge_descriptor_t, bool > >(
              num_vertices(g), null_edge))
    ,

        mate(mate_vector.begin(), vm)
    , label_S(label_S_vector.begin(), vm)
    , label_T(label_T_vector.begin(), vm)
    , outlet(outlet_vector.begin(), vm)
    , tau_idx(tau_idx_vector.begin(), vm)
    , dual_var(dual_var_vector.begin(), vm)
    , pi(pi_vector.begin(), vm)
    , gamma(gamma_vector.begin(), vm)
    , tau(tau_vector.begin(), vm)
    , in_blossom(in_blossom_vector.begin(), vm)
    , old_label(old_label_vector.begin(), vm)
    {
        vertex_iterator_t vi, vi_end;
        edge_iterator_t ei, ei_end;

        edge_property_t max_weight
            = std::numeric_limits< edge_property_t >::min();
        for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
            max_weight = std::max(max_weight, get(edge_weight, g, *ei));

        typename std::vector<
            std::vector< std::pair< edge_descriptor_t, bool > > >::iterator vei;

        for (boost::tie(vi, vi_end) = vertices(g),
                            vei = critical_edge_vectors.begin();
             vi != vi_end; ++vi, ++vei)
        {
            vertex_descriptor_t u = *vi;
            mate[u] = get(arg_mate, u);
            dual_var[u] = 2 * max_weight;
            in_blossom[u] = boost::make_shared< trivial_blossom >(u);
            outlet[u] = u;
            critical_edge_vector.push_back(
                vertex_to_edge_map_t(vei->begin(), vm));
        }

        critical_edge
            = vertex_pair_to_edge_map_t(critical_edge_vector.begin(), vm);

        init();
    }

    // return the top blossom where v is contained inside
    blossom_ptr_t in_top_blossom(vertex_descriptor_t v) const
    {
        blossom_ptr_t b = in_blossom[v];
        while (b->father != blossom_ptr_t())
            b = b->father;
        return b;
    }

    // check if vertex v is in blossom b
    bool is_in_blossom(blossom_ptr_t b, vertex_descriptor_t v) const
    {
        if (v == graph_traits< Graph >::null_vertex())
            return false;
        blossom_ptr_t vb = in_blossom[v]->father;
        while (vb != blossom_ptr_t())
        {
            if (vb.get() == b.get())
                return true;
            vb = vb->father;
        }
        return false;
    }

    // return the base vertex of the top blossom that contains v
    inline vertex_descriptor_t base_vertex(vertex_descriptor_t v) const
    {
        return in_top_blossom(v)->get_base();
    }

    // add an existed top blossom of base vertex v into new top
    // blossom b as its sub-blossom
    void add_sub_blossom(blossom_ptr_t b, vertex_descriptor_t v)
    {
        blossom_ptr_t sub = in_top_blossom(v);
        sub->father = b;
        b->sub_blossoms.push_back(sub);
        if (sub->sub_blossoms.empty())
            return;
        for (blossom_iterator_t bi = top_blossoms.begin();
             bi != top_blossoms.end(); ++bi)
        {
            if (bi->get() == sub.get())
            {
                top_blossoms.erase(bi);
                break;
            }
        }
    }

    // when a top blossom is created or its base vertex getting an S-label,
    // add all edges incident to this blossom into even_edges
    void bloom(blossom_ptr_t b)
    {
        std::vector< vertex_descriptor_t > vertices_of_b = b->vertices();
        vertex_vec_iter_t vi;
        for (vi = vertices_of_b.begin(); vi != vertices_of_b.end(); ++vi)
        {
            out_edge_iterator_t oei, oei_end;
            for (boost::tie(oei, oei_end) = out_edges(*vi, g); oei != oei_end;
                 ++oei)
            {
                if (target(*oei, g) != *vi && mate[*vi] != target(*oei, g))
                    even_edges.push_back(*oei);
            }
        }
    }

    // assigning a T-label to a non S-vertex, along with outlet and updating pi
    // value if updated pi[v] equals zero, augment the matching from its mate
    // vertex
    void put_T_label(vertex_descriptor_t v, vertex_descriptor_t T_label,
        vertex_descriptor_t outlet_v, edge_property_t pi_v)
    {
        if (label_S[v] != graph_traits< Graph >::null_vertex())
            return;

        label_T[v] = T_label;
        outlet[v] = outlet_v;
        pi[v] = pi_v;

        vertex_descriptor_t v_mate = mate[v];
        if (pi[v] == 0)
        {
            label_T[v_mate] = graph_traits< Graph >::null_vertex();
            label_S[v_mate] = v;
            bloom(in_top_blossom(v_mate));
        }
    }

    // get the missing T-label for a to-be-expanded base vertex
    // the missing T-label is the last vertex of the path from outlet[v] to v
    std::pair< vertex_descriptor_t, vertex_descriptor_t > missing_label(
        vertex_descriptor_t b_base)
    {
        vertex_descriptor_t missing_outlet = outlet[b_base];

        if (outlet[b_base] == b_base)
            return std::make_pair(
                graph_traits< Graph >::null_vertex(), missing_outlet);

        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            old_label[*vi] = std::make_pair(label_T[*vi], outlet[*vi]);

        std::pair< vertex_descriptor_t, vertex_state_t > child(
            outlet[b_base], graph::detail::V_EVEN);
        blossom_ptr_t b = in_blossom[child.first];
        for (; b->father->father != blossom_ptr_t(); b = b->father)
            ;
        child.first = b->get_base();

        if (child.first == b_base)
            return std::make_pair(
                graph_traits< Graph >::null_vertex(), missing_outlet);

        while (true)
        {
            std::pair< vertex_descriptor_t, vertex_state_t > child_parent
                = parent(child, true);

            for (b = in_blossom[child_parent.first];
                 b->father->father != blossom_ptr_t(); b = b->father)
                ;
            missing_outlet = child_parent.first;
            child_parent.first = b->get_base();

            if (child_parent.first == b_base)
                break;
            else
                child = child_parent;
        }
        return std::make_pair(child.first, missing_outlet);
    }

    // expand a top blossom, put all its non-trivial sub-blossoms into
    // top_blossoms
    blossom_iterator_t expand_blossom(
        blossom_iterator_t bi, std::vector< blossom_ptr_t >& new_ones)
    {
        blossom_ptr_t b = *bi;
        for (blossom_iterator_t i = b->sub_blossoms.begin();
             i != b->sub_blossoms.end(); ++i)
        {
            blossom_ptr_t sub_blossom = *i;
            vertex_descriptor_t sub_base = sub_blossom->get_base();
            label_S[sub_base] = label_T[sub_base]
                = graph_traits< Graph >::null_vertex();
            outlet[sub_base] = sub_base;
            sub_blossom->father = blossom_ptr_t();
            // new top blossoms cannot be pushed back into top_blossoms
            // immediately, because push_back() may cause reallocation and then
            // invalid iterators
            if (!sub_blossom->sub_blossoms.empty())
                new_ones.push_back(sub_blossom);
        }
        return top_blossoms.erase(bi);
    }

    // when expanding a T-blossom with base v, it requires more operations:
    // supply the missing T-labels for new base vertices by picking the minimum
    // tau from vertices of each corresponding new top-blossoms; when label_T[v]
    // is null or we have a smaller tau from missing_label(v), replace T-label
    // and outlet of v (but don't bloom v)
    blossom_iterator_t expand_T_blossom(
        blossom_iterator_t bi, std::vector< blossom_ptr_t >& new_ones)
    {
        blossom_ptr_t b = *bi;

        vertex_descriptor_t b_base = b->get_base();
        std::pair< vertex_descriptor_t, vertex_descriptor_t > T_and_outlet
            = missing_label(b_base);

        blossom_iterator_t next_bi = expand_blossom(bi, new_ones);

        for (blossom_iterator_t i = b->sub_blossoms.begin();
             i != b->sub_blossoms.end(); ++i)
        {
            blossom_ptr_t sub_blossom = *i;
            vertex_descriptor_t sub_base = sub_blossom->get_base();
            vertex_descriptor_t min_tau_v
                = graph_traits< Graph >::null_vertex();
            edge_property_t min_tau
                = std::numeric_limits< edge_property_t >::max();

            std::vector< vertex_descriptor_t > sub_vertices
                = sub_blossom->vertices();
            for (vertex_vec_iter_t v = sub_vertices.begin();
                 v != sub_vertices.end(); ++v)
            {
                if (tau[*v] < min_tau)
                {
                    min_tau = tau[*v];
                    min_tau_v = *v;
                }
            }

            if (min_tau < std::numeric_limits< edge_property_t >::max())
                put_T_label(
                    sub_base, tau_idx[min_tau_v], min_tau_v, tau[min_tau_v]);
        }

        if (label_T[b_base] == graph_traits< Graph >::null_vertex()
            || tau[old_label[b_base].second] < pi[b_base])
            boost::tie(label_T[b_base], outlet[b_base]) = T_and_outlet;

        return next_bi;
    }

    // when vertices v and w are matched to each other by augmenting,
    // we must set v/w as base vertex of any blossom who contains v/w and
    // is a sub-blossom of their lowest (smallest) common blossom
    void adjust_blossom(vertex_descriptor_t v, vertex_descriptor_t w)
    {
        blossom_ptr_t vb = in_blossom[v], wb = in_blossom[w],
                      lowest_common_blossom;
        std::vector< blossom_ptr_t > v_ancestors, w_ancestors;

        while (vb->father != blossom_ptr_t())
        {
            v_ancestors.push_back(vb->father);
            vb = vb->father;
        }
        while (wb->father != blossom_ptr_t())
        {
            w_ancestors.push_back(wb->father);
            wb = wb->father;
        }

        typename std::vector< blossom_ptr_t >::reverse_iterator i, j;
        i = v_ancestors.rbegin();
        j = w_ancestors.rbegin();
        while (i != v_ancestors.rend() && j != w_ancestors.rend()
            && i->get() == j->get())
        {
            lowest_common_blossom = *i;
            ++i;
            ++j;
        }

        vb = in_blossom[v];
        wb = in_blossom[w];
        while (vb->father != lowest_common_blossom)
        {
            vb->father->set_base(vb);
            vb = vb->father;
        }
        while (wb->father != lowest_common_blossom)
        {
            wb->father->set_base(wb);
            wb = wb->father;
        }
    }

    // every edge weight is multiplied by 4 to ensure integer weights
    // throughout the algorithm if all input weights are integers
    inline edge_property_t slack(const edge_descriptor_t& e) const
    {
        vertex_descriptor_t v, w;
        v = source(e, g);
        w = target(e, g);
        return dual_var[v] + dual_var[w] - 4 * get(edge_weight, g, e);
    }

    // backtrace one step on vertex v along the augmenting path
    // by its labels and its vertex state;
    // boolean parameter "use_old" means whether we are updating labels,
    // if we are, then we use old labels to backtrace and also we
    // don't jump to its base vertex when we reach an odd vertex
    std::pair< vertex_descriptor_t, vertex_state_t > parent(
        std::pair< vertex_descriptor_t, vertex_state_t > v,
        bool use_old = false) const
    {
        if (v.second == graph::detail::V_EVEN)
        {
            // a paranoid check: label_S shoule be the same as mate in
            // backtracing
            if (label_S[v.first] == graph_traits< Graph >::null_vertex())
                label_S[v.first] = mate[v.first];
            return std::make_pair(label_S[v.first], graph::detail::V_ODD);
        }
        else if (v.second == graph::detail::V_ODD)
        {
            vertex_descriptor_t w = use_old ? old_label[v.first].first
                                            : base_vertex(label_T[v.first]);
            return std::make_pair(w, graph::detail::V_EVEN);
        }
        return std::make_pair(v.first, graph::detail::V_UNREACHED);
    }

    // backtrace from vertices v and w to their free (unmatched) ancesters,
    // return the nearest common ancestor (null_vertex if none) of v and w
    vertex_descriptor_t nearest_common_ancestor(vertex_descriptor_t v,
        vertex_descriptor_t w, vertex_descriptor_t& v_free_ancestor,
        vertex_descriptor_t& w_free_ancestor) const
    {
        std::pair< vertex_descriptor_t, vertex_state_t > v_up(
            v, graph::detail::V_EVEN);
        std::pair< vertex_descriptor_t, vertex_state_t > w_up(
            w, graph::detail::V_EVEN);
        vertex_descriptor_t nca;
        nca = w_free_ancestor = v_free_ancestor
            = graph_traits< Graph >::null_vertex();

        std::vector< bool > ancestor_of_w_vector(num_vertices(g), false);
        std::vector< bool > ancestor_of_v_vector(num_vertices(g), false);
        vertex_to_bool_map_t ancestor_of_w(ancestor_of_w_vector.begin(), vm);
        vertex_to_bool_map_t ancestor_of_v(ancestor_of_v_vector.begin(), vm);

        while (nca == graph_traits< Graph >::null_vertex()
            && (v_free_ancestor == graph_traits< Graph >::null_vertex()
                || w_free_ancestor == graph_traits< Graph >::null_vertex()))
        {
            ancestor_of_w[w_up.first] = true;
            ancestor_of_v[v_up.first] = true;

            if (w_free_ancestor == graph_traits< Graph >::null_vertex())
                w_up = parent(w_up);
            if (v_free_ancestor == graph_traits< Graph >::null_vertex())
                v_up = parent(v_up);

            if (mate[v_up.first] == graph_traits< Graph >::null_vertex())
                v_free_ancestor = v_up.first;
            if (mate[w_up.first] == graph_traits< Graph >::null_vertex())
                w_free_ancestor = w_up.first;

            if (ancestor_of_w[v_up.first] == true || v_up.first == w_up.first)
                nca = v_up.first;
            else if (ancestor_of_v[w_up.first] == true)
                nca = w_up.first;
            else if (v_free_ancestor == w_free_ancestor
                && v_free_ancestor != graph_traits< Graph >::null_vertex())
                nca = v_up.first;
        }

        return nca;
    }

    // when a new top blossom b is created by connecting (v, w), we add
    // sub-blossoms into b along backtracing from v_prime and w_prime to
    // stop_vertex (the base vertex); also, we set labels and outlet for each
    // base vertex we pass by
    void make_blossom(blossom_ptr_t b, vertex_descriptor_t w_prime,
        vertex_descriptor_t v_prime, vertex_descriptor_t stop_vertex)
    {
        std::pair< vertex_descriptor_t, vertex_state_t > u(
            v_prime, graph::detail::V_ODD);
        std::pair< vertex_descriptor_t, vertex_state_t > u_up(
            w_prime, graph::detail::V_EVEN);

        for (; u_up.first != stop_vertex; u = u_up, u_up = parent(u))
        {
            if (u_up.second == graph::detail::V_EVEN)
            {
                if (!in_top_blossom(u_up.first)->sub_blossoms.empty())
                    outlet[u_up.first] = label_T[u.first];
                label_T[u_up.first] = outlet[u.first];
            }
            else if (u_up.second == graph::detail::V_ODD)
                label_S[u_up.first] = u.first;

            add_sub_blossom(b, u_up.first);
        }
    }

    // the design of recursively expanding augmenting path in
    // (reversed_)retrieve_augmenting_path functions is inspired by same
    // functions in max_cardinality_matching.hpp; except that in weighted
    // matching, we use "outlet" vertices instead of "bridge" vertex pairs: if
    // blossom b is the smallest non-trivial blossom that contains its base
    // vertex v, then v and outlet[v] are where augmenting path enters and
    // leaves b
    void retrieve_augmenting_path(
        vertex_descriptor_t v, vertex_descriptor_t w, vertex_state_t v_state)
    {
        if (v == w)
            aug_path.push_back(v);
        else if (v_state == graph::detail::V_EVEN)
        {
            aug_path.push_back(v);
            retrieve_augmenting_path(label_S[v], w, graph::detail::V_ODD);
        }
        else if (v_state == graph::detail::V_ODD)
        {
            if (outlet[v] == v)
                aug_path.push_back(v);
            else
                reversed_retrieve_augmenting_path(
                    outlet[v], v, graph::detail::V_EVEN);
            retrieve_augmenting_path(label_T[v], w, graph::detail::V_EVEN);
        }
    }

    void reversed_retrieve_augmenting_path(
        vertex_descriptor_t v, vertex_descriptor_t w, vertex_state_t v_state)
    {
        if (v == w)
            aug_path.push_back(v);
        else if (v_state == graph::detail::V_EVEN)
        {
            reversed_retrieve_augmenting_path(
                label_S[v], w, graph::detail::V_ODD);
            aug_path.push_back(v);
        }
        else if (v_state == graph::detail::V_ODD)
        {
            reversed_retrieve_augmenting_path(
                label_T[v], w, graph::detail::V_EVEN);
            if (outlet[v] != v)
                retrieve_augmenting_path(outlet[v], v, graph::detail::V_EVEN);
            else
                aug_path.push_back(v);
        }
    }

    // correct labels for vertices in the augmenting path
    void relabel(vertex_descriptor_t v)
    {
        blossom_ptr_t b = in_blossom[v]->father;

        if (!is_in_blossom(b, mate[v]))
        { // if v is a new base vertex
            std::pair< vertex_descriptor_t, vertex_state_t > u(
                v, graph::detail::V_EVEN);
            while (label_S[u.first] != u.first
                && is_in_blossom(b, label_S[u.first]))
                u = parent(u, true);

            vertex_descriptor_t old_base = u.first;
            if (label_S[old_base] != old_base)
            { // if old base is not exposed
                label_T[v] = label_S[old_base];
                outlet[v] = old_base;
            }
            else
            { // if old base is exposed then new label_T[v] is not in b,
                // we must (i) make b2 the smallest blossom containing v but not
                // as base vertex (ii) backtrace from b2's new base vertex to b
                label_T[v] = graph_traits< Graph >::null_vertex();
                for (b = b->father; b != blossom_ptr_t() && b->get_base() == v;
                     b = b->father)
                    ;
                if (b != blossom_ptr_t())
                {
                    u = std::make_pair(b->get_base(), graph::detail::V_ODD);
                    while (!is_in_blossom(
                        in_blossom[v]->father, old_label[u.first].first))
                        u = parent(u, true);
                    label_T[v] = u.first;
                    outlet[v] = old_label[u.first].first;
                }
            }
        }
        else if (label_S[v] == v || !is_in_blossom(b, label_S[v]))
        { // if v is an old base vertex
            // let u be the new base vertex; backtrace from u's old T-label
            std::pair< vertex_descriptor_t, vertex_state_t > u(
                b->get_base(), graph::detail::V_ODD);
            while (
                old_label[u.first].first != graph_traits< Graph >::null_vertex()
                && old_label[u.first].first != v)
                u = parent(u, true);
            label_T[v] = old_label[u.first].second;
            outlet[v] = v;
        }
        else // if v is neither a new nor an old base vertex
            label_T[v] = label_S[v];
    }

    void augmenting(vertex_descriptor_t v, vertex_descriptor_t v_free_ancestor,
        vertex_descriptor_t w, vertex_descriptor_t w_free_ancestor)
    {
        vertex_iterator_t vi, vi_end;

        // retrieve the augmenting path and put it in aug_path
        reversed_retrieve_augmenting_path(
            v, v_free_ancestor, graph::detail::V_EVEN);
        retrieve_augmenting_path(w, w_free_ancestor, graph::detail::V_EVEN);

        // augment the matching along aug_path
        vertex_descriptor_t a, b;
        vertex_list_t reversed_aug_path;
        while (!aug_path.empty())
        {
            a = aug_path.front();
            aug_path.pop_front();
            reversed_aug_path.push_back(a);
            b = aug_path.front();
            aug_path.pop_front();
            reversed_aug_path.push_back(b);

            mate[a] = b;
            mate[b] = a;

            // reset base vertex for every blossom in augment path
            adjust_blossom(a, b);
        }

        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            old_label[*vi] = std::make_pair(label_T[*vi], outlet[*vi]);

        // correct labels for in-blossom vertices along aug_path
        while (!reversed_aug_path.empty())
        {
            a = reversed_aug_path.front();
            reversed_aug_path.pop_front();

            if (in_blossom[a]->father != blossom_ptr_t())
                relabel(a);
        }

        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            vertex_descriptor_t u = *vi;
            if (mate[u] != graph_traits< Graph >::null_vertex())
                label_S[u] = mate[u];
        }

        // expand blossoms with zero dual variables
        std::vector< blossom_ptr_t > new_top_blossoms;
        for (blossom_iterator_t bi = top_blossoms.begin();
             bi != top_blossoms.end();)
        {
            if ((*bi)->dual_var <= 0)
                bi = expand_blossom(bi, new_top_blossoms);
            else
                ++bi;
        }
        top_blossoms.insert(top_blossoms.end(), new_top_blossoms.begin(),
            new_top_blossoms.end());
        init();
    }

    // create a new blossom and set labels for vertices inside
    void blossoming(vertex_descriptor_t v, vertex_descriptor_t v_prime,
        vertex_descriptor_t w, vertex_descriptor_t w_prime,
        vertex_descriptor_t nca)
    {
        vertex_iterator_t vi, vi_end;

        std::vector< bool > is_old_base_vector(num_vertices(g));
        vertex_to_bool_map_t is_old_base(is_old_base_vector.begin(), vm);
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            if (*vi == base_vertex(*vi))
                is_old_base[*vi] = true;
        }

        blossom_ptr_t b = boost::make_shared< blossom >();
        add_sub_blossom(b, nca);

        label_T[w_prime] = v;
        label_T[v_prime] = w;
        outlet[w_prime] = w;
        outlet[v_prime] = v;

        make_blossom(b, w_prime, v_prime, nca);
        make_blossom(b, v_prime, w_prime, nca);

        label_T[nca] = graph_traits< Graph >::null_vertex();
        outlet[nca] = nca;

        top_blossoms.push_back(b);
        bloom(b);

        // set gamma[b_base] = min_slack{critical_edge(b_base, other_base)}
        // where each critical edge is updated before, by
        // argmin{slack(old_bases_in_b, other_base)};
        vertex_vec_iter_t i, j;
        std::vector< vertex_descriptor_t > b_vertices = b->vertices(),
                                           old_base_in_b, other_base;
        vertex_descriptor_t b_base = b->get_base();
        for (i = b_vertices.begin(); i != b_vertices.end(); ++i)
        {
            if (is_old_base[*i])
                old_base_in_b.push_back(*i);
        }
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            if (*vi != b_base && *vi == base_vertex(*vi))
                other_base.push_back(*vi);
        }
        for (i = other_base.begin(); i != other_base.end(); ++i)
        {
            edge_property_t min_slack
                = std::numeric_limits< edge_property_t >::max();
            std::pair< edge_descriptor_t, bool > b_vi = null_edge;
            for (j = old_base_in_b.begin(); j != old_base_in_b.end(); ++j)
            {
                if (critical_edge[*j][*i] != null_edge
                    && min_slack > slack(critical_edge[*j][*i].first))
                {
                    min_slack = slack(critical_edge[*j][*i].first);
                    b_vi = critical_edge[*j][*i];
                }
            }
            critical_edge[b_base][*i] = critical_edge[*i][b_base] = b_vi;
        }
        gamma[b_base] = std::numeric_limits< edge_property_t >::max();
        for (i = other_base.begin(); i != other_base.end(); ++i)
        {
            if (critical_edge[b_base][*i] != null_edge)
                gamma[b_base] = std::min(
                    gamma[b_base], slack(critical_edge[b_base][*i].first));
        }
    }

    void init()
    {
        even_edges.clear();

        vertex_iterator_t vi, vi_end;
        typename std::vector<
            std::vector< std::pair< edge_descriptor_t, bool > > >::iterator vei;

        for (boost::tie(vi, vi_end) = vertices(g),
                            vei = critical_edge_vectors.begin();
             vi != vi_end; ++vi, ++vei)
        {
            vertex_descriptor_t u = *vi;
            out_edge_iterator_t ei, ei_end;

            gamma[u] = tau[u] = pi[u]
                = std::numeric_limits< edge_property_t >::max();
            std::fill(vei->begin(), vei->end(), null_edge);

            if (base_vertex(u) != u)
                continue;

            label_S[u] = label_T[u] = graph_traits< Graph >::null_vertex();
            outlet[u] = u;

            if (mate[u] == graph_traits< Graph >::null_vertex())
            {
                label_S[u] = u;
                bloom(in_top_blossom(u));
            }
        }
    }

    bool augment_matching()
    {
        vertex_descriptor_t v, w, w_free_ancestor, v_free_ancestor;
        v = w = w_free_ancestor = v_free_ancestor
            = graph_traits< Graph >::null_vertex();
        bool found_alternating_path = false;

        // note that we only use edges of zero slack value for augmenting
        while (!even_edges.empty() && !found_alternating_path)
        {
            // search for augmenting paths depth-first
            edge_descriptor_t current_edge = even_edges.back();
            even_edges.pop_back();

            v = source(current_edge, g);
            w = target(current_edge, g);

            vertex_descriptor_t v_prime = base_vertex(v);
            vertex_descriptor_t w_prime = base_vertex(w);

            // w_prime == v_prime implies that we get an edge that has been
            // shrunk into a blossom
            if (v_prime == w_prime)
                continue;

            // a paranoid check
            if (label_S[v_prime] == graph_traits< Graph >::null_vertex())
            {
                std::swap(v_prime, w_prime);
                std::swap(v, w);
            }

            // w_prime may be unlabeled or have a T-label; replace the existed
            // T-label if the edge slack is smaller than current pi[w_prime] and
            // update it. Note that a T-label is "deserved" only when pi equals
            // zero. also update tau and tau_idx so that tau_idx becomes T-label
            // when a T-blossom is expanded
            if (label_S[w_prime] == graph_traits< Graph >::null_vertex())
            {
                if (slack(current_edge) < pi[w_prime])
                    put_T_label(w_prime, v, w, slack(current_edge));
                if (slack(current_edge) < tau[w])
                {
                    if (in_blossom[w]->father == blossom_ptr_t()
                        || label_T[w_prime] == v
                        || label_T[w_prime]
                            == graph_traits< Graph >::null_vertex()
                        || nearest_common_ancestor(v_prime, label_T[w_prime],
                               v_free_ancestor, w_free_ancestor)
                            == graph_traits< Graph >::null_vertex())
                    {
                        tau[w] = slack(current_edge);
                        tau_idx[w] = v;
                    }
                }
            }

            else
            {
                if (slack(current_edge) > 0)
                {
                    // update gamma and critical_edges when we have a smaller
                    // edge slack
                    gamma[v_prime]
                        = std::min(gamma[v_prime], slack(current_edge));
                    gamma[w_prime]
                        = std::min(gamma[w_prime], slack(current_edge));
                    if (critical_edge[v_prime][w_prime] == null_edge
                        || slack(critical_edge[v_prime][w_prime].first)
                            > slack(current_edge))
                    {
                        critical_edge[v_prime][w_prime]
                            = std::pair< edge_descriptor_t, bool >(
                                current_edge, true);
                        critical_edge[w_prime][v_prime]
                            = std::pair< edge_descriptor_t, bool >(
                                current_edge, true);
                    }
                    continue;
                }
                else if (slack(current_edge) == 0)
                {
                    // if nca is null_vertex then we have an augmenting path;
                    // otherwise we have a new top blossom with nca as its base
                    // vertex
                    vertex_descriptor_t nca = nearest_common_ancestor(
                        v_prime, w_prime, v_free_ancestor, w_free_ancestor);

                    if (nca == graph_traits< Graph >::null_vertex())
                        found_alternating_path
                            = true; // to break out of the loop
                    else
                        blossoming(v, v_prime, w, w_prime, nca);
                }
            }
        }

        if (!found_alternating_path)
            return false;

        augmenting(v, v_free_ancestor, w, w_free_ancestor);
        return true;
    }

    // slack the vertex and blossom dual variables when there is no augmenting
    // path found according to the primal-dual method
    bool adjust_dual()
    {
        edge_property_t delta1, delta2, delta3, delta4, delta;
        delta1 = delta2 = delta3 = delta4
            = std::numeric_limits< edge_property_t >::max();

        vertex_iterator_t vi, vi_end;

        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            delta1 = std::min(delta1, dual_var[*vi]);
            delta4 = pi[*vi] > 0 ? std::min(delta4, pi[*vi]) : delta4;
            if (*vi == base_vertex(*vi))
                delta3 = std::min(delta3, gamma[*vi] / 2);
        }

        for (blossom_iterator_t bi = top_blossoms.begin();
             bi != top_blossoms.end(); ++bi)
        {
            vertex_descriptor_t b_base = (*bi)->get_base();
            if (label_T[b_base] != graph_traits< Graph >::null_vertex()
                && pi[b_base] == 0)
                delta2 = std::min(delta2, (*bi)->dual_var / 2);
        }

        delta = std::min(std::min(delta1, delta2), std::min(delta3, delta4));

        // start updating dual variables, note that the order is important

        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            vertex_descriptor_t v = *vi, v_prime = base_vertex(v);

            if (label_S[v_prime] != graph_traits< Graph >::null_vertex())
                dual_var[v] -= delta;
            else if (label_T[v_prime] != graph_traits< Graph >::null_vertex()
                && pi[v_prime] == 0)
                dual_var[v] += delta;

            if (v == v_prime)
                gamma[v] -= 2 * delta;
        }

        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            vertex_descriptor_t v_prime = base_vertex(*vi);
            if (pi[v_prime] > 0)
                tau[*vi] -= delta;
        }

        for (blossom_iterator_t bi = top_blossoms.begin();
             bi != top_blossoms.end(); ++bi)
        {
            vertex_descriptor_t b_base = (*bi)->get_base();
            if (label_T[b_base] != graph_traits< Graph >::null_vertex()
                && pi[b_base] == 0)
                (*bi)->dual_var -= 2 * delta;
            if (label_S[b_base] != graph_traits< Graph >::null_vertex())
                (*bi)->dual_var += 2 * delta;
        }

        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            vertex_descriptor_t v = *vi;
            if (pi[v] > 0)
                pi[v] -= delta;

            // when some T-vertices have zero pi value, bloom their mates so
            // that matching can be further augmented
            if (label_T[v] != graph_traits< Graph >::null_vertex()
                && pi[v] == 0)
                put_T_label(v, label_T[v], outlet[v], pi[v]);
        }

        // optimal solution reached, halt
        if (delta == delta1)
            return false;

        // expand odd blossoms with zero dual variables and zero pi value of
        // their base vertices
        if (delta == delta2 && delta != delta3)
        {
            std::vector< blossom_ptr_t > new_top_blossoms;
            for (blossom_iterator_t bi = top_blossoms.begin();
                 bi != top_blossoms.end();)
            {
                const blossom_ptr_t b = *bi;
                vertex_descriptor_t b_base = b->get_base();
                if (b->dual_var == 0
                    && label_T[b_base] != graph_traits< Graph >::null_vertex()
                    && pi[b_base] == 0)
                    bi = expand_T_blossom(bi, new_top_blossoms);
                else
                    ++bi;
            }
            top_blossoms.insert(top_blossoms.end(), new_top_blossoms.begin(),
                new_top_blossoms.end());
        }

        while (true)
        {
            // find a zero-slack critical edge (v, w) of zero gamma values
            std::pair< edge_descriptor_t, bool > best_edge = null_edge;
            std::vector< vertex_descriptor_t > base_nodes;
            for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            {
                if (*vi == base_vertex(*vi))
                    base_nodes.push_back(*vi);
            }
            for (vertex_vec_iter_t i = base_nodes.begin();
                 i != base_nodes.end(); ++i)
            {
                if (gamma[*i] == 0)
                {
                    for (vertex_vec_iter_t j = base_nodes.begin();
                         j != base_nodes.end(); ++j)
                    {
                        if (critical_edge[*i][*j] != null_edge
                            && slack(critical_edge[*i][*j].first) == 0)
                            best_edge = critical_edge[*i][*j];
                    }
                }
            }

            // if not found, continue finding other augment matching
            if (best_edge == null_edge)
            {
                bool augmented = augment_matching();
                return augmented || delta != delta1;
            }
            // if found, determine either augmenting or blossoming
            vertex_descriptor_t v = source(best_edge.first, g),
                                w = target(best_edge.first, g);
            vertex_descriptor_t v_prime = base_vertex(v),
                                w_prime = base_vertex(w), v_free_ancestor,
                                w_free_ancestor;
            vertex_descriptor_t nca = nearest_common_ancestor(
                v_prime, w_prime, v_free_ancestor, w_free_ancestor);
            if (nca == graph_traits< Graph >::null_vertex())
            {
                augmenting(v, v_free_ancestor, w, w_free_ancestor);
                return true;
            }
            else
                blossoming(v, v_prime, w, w_prime, nca);
        }

        return false;
    }

    template < typename PropertyMap > void get_current_matching(PropertyMap pm)
    {
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            put(pm, *vi, mate[*vi]);
    }

private:
    const Graph& g;
    VertexIndexMap vm;
    const std::pair< edge_descriptor_t, bool > null_edge;

    // storage for the property maps below
    std::vector< vertex_descriptor_t > mate_vector;
    std::vector< vertex_descriptor_t > label_S_vector, label_T_vector;
    std::vector< vertex_descriptor_t > outlet_vector;
    std::vector< vertex_descriptor_t > tau_idx_vector;
    std::vector< edge_property_t > dual_var_vector;
    std::vector< edge_property_t > pi_vector, gamma_vector, tau_vector;
    std::vector< blossom_ptr_t > in_blossom_vector;
    std::vector< std::pair< vertex_descriptor_t, vertex_descriptor_t > >
        old_label_vector;
    std::vector< vertex_to_edge_map_t > critical_edge_vector;
    std::vector< std::vector< std::pair< edge_descriptor_t, bool > > >
        critical_edge_vectors;

    // iterator property maps
    vertex_to_vertex_map_t mate;
    vertex_to_vertex_map_t label_S; // v has an S-label -> v can be an even
                                    // vertex, label_S[v] is its mate
    vertex_to_vertex_map_t
        label_T; // v has a T-label -> v can be an odd vertex, label_T[v] is its
                 // predecessor in aug_path
    vertex_to_vertex_map_t outlet;
    vertex_to_vertex_map_t tau_idx;
    vertex_to_weight_map_t dual_var;
    vertex_to_weight_map_t pi, gamma, tau;
    vertex_to_blossom_map_t
        in_blossom; // map any vertex v to the trivial blossom containing v
    vertex_to_pair_map_t old_label; // <old T-label, old outlet> before
                                    // relabeling or expanding T-blossoms
    vertex_pair_to_edge_map_t
        critical_edge; // an not matched edge (v, w) is critical if v and w
                       // belongs to different S-blossoms

    vertex_list_t aug_path;
    edge_list_t even_edges;
    std::vector< blossom_ptr_t > top_blossoms;
};

template < typename Graph, typename MateMap, typename VertexIndexMap >
void maximum_weighted_matching(const Graph& g, MateMap mate, VertexIndexMap vm)
{
    empty_matching< Graph, MateMap >::find_matching(g, mate);
    weighted_augmenting_path_finder< Graph, MateMap, VertexIndexMap > augmentor(
        g, mate, vm);

    // can have |V| times augmenting at most
    for (std::size_t t = 0; t < num_vertices(g); ++t)
    {
        bool augmented = false;
        while (!augmented)
        {
            augmented = augmentor.augment_matching();
            if (!augmented)
            {
                // halt if adjusting dual variables can't bring potential
                // augment
                if (!augmentor.adjust_dual())
                    break;
            }
        }
        if (!augmented)
            break;
    }

    augmentor.get_current_matching(mate);
}

template < typename Graph, typename MateMap >
inline void maximum_weighted_matching(const Graph& g, MateMap mate)
{
    maximum_weighted_matching(g, mate, get(vertex_index, g));
}

// brute-force matcher searches all possible combinations of matched edges to
// get the maximum weighted matching which can be used for testing on small
// graphs (within dozens vertices)
template < typename Graph, typename MateMap, typename VertexIndexMap >
class brute_force_matching
{
public:
    typedef
        typename graph_traits< Graph >::vertex_descriptor vertex_descriptor_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef
        typename std::vector< vertex_descriptor_t >::iterator vertex_vec_iter_t;
    typedef typename graph_traits< Graph >::edge_iterator edge_iterator_t;
    typedef boost::iterator_property_map< vertex_vec_iter_t, VertexIndexMap >
        vertex_to_vertex_map_t;

    brute_force_matching(
        const Graph& arg_g, MateMap arg_mate, VertexIndexMap arg_vm)
    : g(arg_g)
    , vm(arg_vm)
    , mate_vector(num_vertices(g))
    , best_mate_vector(num_vertices(g))
    , mate(mate_vector.begin(), vm)
    , best_mate(best_mate_vector.begin(), vm)
    {
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            best_mate[*vi] = mate[*vi] = get(arg_mate, *vi);
    }

    template < typename PropertyMap > void find_matching(PropertyMap pm)
    {
        edge_iterator_t ei;
        boost::tie(ei, ei_end) = edges(g);
        select_edge(ei);

        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            put(pm, *vi, best_mate[*vi]);
    }

private:
    const Graph& g;
    VertexIndexMap vm;
    std::vector< vertex_descriptor_t > mate_vector, best_mate_vector;
    vertex_to_vertex_map_t mate, best_mate;
    edge_iterator_t ei_end;

    void select_edge(edge_iterator_t ei)
    {
        if (ei == ei_end)
        {
            if (matching_weight_sum(g, mate)
                > matching_weight_sum(g, best_mate))
            {
                vertex_iterator_t vi, vi_end;
                for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
                    best_mate[*vi] = mate[*vi];
            }
            return;
        }

        vertex_descriptor_t v, w;
        v = source(*ei, g);
        w = target(*ei, g);

        select_edge(++ei);

        if (mate[v] == graph_traits< Graph >::null_vertex()
            && mate[w] == graph_traits< Graph >::null_vertex())
        {
            mate[v] = w;
            mate[w] = v;
            select_edge(ei);
            mate[v] = mate[w] = graph_traits< Graph >::null_vertex();
        }
    }
};

template < typename Graph, typename MateMap, typename VertexIndexMap >
void brute_force_maximum_weighted_matching(
    const Graph& g, MateMap mate, VertexIndexMap vm)
{
    empty_matching< Graph, MateMap >::find_matching(g, mate);
    brute_force_matching< Graph, MateMap, VertexIndexMap > brute_force_matcher(
        g, mate, vm);
    brute_force_matcher.find_matching(mate);
}

template < typename Graph, typename MateMap >
inline void brute_force_maximum_weighted_matching(const Graph& g, MateMap mate)
{
    brute_force_maximum_weighted_matching(g, mate, get(vertex_index, g));
}

}

#endif
