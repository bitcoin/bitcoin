// Copyright (C) 2006-2009 Dmitry Bufistov and Andrey Parfenov

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_CYCLE_RATIO_HOWARD_HPP
#define BOOST_GRAPH_CYCLE_RATIO_HOWARD_HPP

#include <vector>
#include <list>
#include <algorithm>
#include <functional>
#include <limits>

#include <boost/bind/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/concept_check.hpp>
#include <boost/pending/queue.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/concept/assert.hpp>
#include <boost/algorithm/minmax_element.hpp>

/** @file howard_cycle_ratio.hpp
 * @brief The implementation of the maximum/minimum cycle ratio/mean algorithm.
 * @author Dmitry Bufistov
 * @author Andrey Parfenov
 */

namespace boost
{

/**
 * The mcr_float is like numeric_limits, but only for floating point types
 * and only defines infinity() and epsilon(). This class is primarily used
 * to encapsulate a less-precise epsilon than natively supported by the
 * floating point type.
 */
template < typename Float = double > struct mcr_float
{
    typedef Float value_type;

    static Float infinity()
    {
        return std::numeric_limits< value_type >::infinity();
    }

    static Float epsilon() { return Float(-0.005); }
};

namespace detail
{

    template < typename FloatTraits > struct min_comparator_props
    {
        typedef std::greater< typename FloatTraits::value_type > comparator;
        static const int multiplier = 1;
    };

    template < typename FloatTraits > struct max_comparator_props
    {
        typedef std::less< typename FloatTraits::value_type > comparator;
        static const int multiplier = -1;
    };

    template < typename FloatTraits, typename ComparatorProps >
    struct float_wrapper
    {
        typedef typename FloatTraits::value_type value_type;
        typedef ComparatorProps comparator_props_t;
        typedef typename ComparatorProps::comparator comparator;

        static value_type infinity()
        {
            return FloatTraits::infinity() * ComparatorProps::multiplier;
        }

        static value_type epsilon()
        {
            return FloatTraits::epsilon() * ComparatorProps::multiplier;
        }
    };

    /*! @class mcr_howard
     * @brief Calculates optimum (maximum/minimum) cycle ratio of a directed
     * graph. Uses  Howard's iteration policy algorithm. </br>(It is described
     * in the paper "Experimental Analysis of the Fastest Optimum Cycle Ratio
     * and Mean Algorithm" by Ali Dasdan).
     */
    template < typename FloatTraits, typename Graph, typename VertexIndexMap,
        typename EdgeWeight1, typename EdgeWeight2 >
    class mcr_howard
    {
    public:
        typedef typename FloatTraits::value_type float_t;
        typedef typename FloatTraits::comparator_props_t cmp_props_t;
        typedef typename FloatTraits::comparator comparator_t;
        typedef enum
        {
            my_white = 0,
            my_black
        } my_color_type;
        typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
        typedef typename graph_traits< Graph >::edge_descriptor edge_t;
        typedef typename graph_traits< Graph >::vertices_size_type vn_t;
        typedef std::vector< float_t > vp_t;
        typedef typename boost::iterator_property_map< typename vp_t::iterator,
            VertexIndexMap >
            distance_map_t; // V -> float_t

        typedef typename std::vector< edge_t > ve_t;
        typedef std::vector< my_color_type > vcol_t;
        typedef
            typename ::boost::iterator_property_map< typename ve_t::iterator,
                VertexIndexMap >
                policy_t; // Vertex -> Edge
        typedef
            typename ::boost::iterator_property_map< typename vcol_t::iterator,
                VertexIndexMap >
                color_map_t;

        typedef typename std::list< vertex_t >
            pinel_t; // The in_edges list of the policy graph
        typedef typename std::vector< pinel_t > inedges1_t;
        typedef typename ::boost::iterator_property_map<
            typename inedges1_t::iterator, VertexIndexMap >
            inedges_t;
        typedef typename std::vector< edge_t > critical_cycle_t;

        // Bad  vertex flag. If true, then the vertex is "bad".
        // Vertex is "bad" if its out_degree is equal to zero.
        typedef
            typename boost::iterator_property_map< std::vector< int >::iterator,
                VertexIndexMap >
                badv_t;

        /*!
         * Constructor
         * \param g = (V, E) - a directed multigraph.
         * \param vim  Vertex Index Map. Read property Map: V -> [0,
         * num_vertices(g)). \param ewm  edge weight map. Read property map: E
         * -> R \param ew2m  edge weight map. Read property map: E -> R+ \param
         * infty A big enough value to guaranty that there exist a cycle with
         *  better ratio.
         * \param cmp The compare operator for float_ts.
         */
        mcr_howard(const Graph& g, VertexIndexMap vim, EdgeWeight1 ewm,
            EdgeWeight2 ew2m)
        : m_g(g)
        , m_vim(vim)
        , m_ew1m(ewm)
        , m_ew2m(ew2m)
        , m_bound(mcr_bound())
        , m_cr(m_bound)
        , m_V(num_vertices(m_g))
        , m_dis(m_V, 0)
        , m_dm(m_dis.begin(), m_vim)
        , m_policyc(m_V)
        , m_policy(m_policyc.begin(), m_vim)
        , m_inelc(m_V)
        , m_inel(m_inelc.begin(), m_vim)
        , m_badvc(m_V, false)
        , m_badv(m_badvc.begin(), m_vim)
        , m_colcv(m_V)
        , m_col_bfs(m_V)
        {
        }

        /*!
         * \return maximum/minimum_{for all cycles C}
         *         [sum_{e in C} w1(e)] / [sum_{e in C} w2(e)],
         * or FloatTraits::infinity() if graph has no cycles.
         */
        float_t ocr_howard()
        {
            construct_policy_graph();
            int k = 0;
            float_t mcr = 0;
            do
            {
                mcr = policy_mcr();
                ++k;
            } while (
                try_improve_policy(mcr) && k < 100); // To avoid infinite loop

            const float_t eps_ = -0.00000001 * cmp_props_t::multiplier;
            if (m_cmp(mcr, m_bound + eps_))
            {
                return FloatTraits::infinity();
            }
            else
            {
                return mcr;
            }
        }
        virtual ~mcr_howard() {}

    protected:
        virtual void store_critical_edge(edge_t, critical_cycle_t&) {}
        virtual void store_critical_cycle(critical_cycle_t&) {}

    private:
        /*!
         * \return lower/upper bound for the maximal/minimal cycle ratio
         */
        float_t mcr_bound()
        {
            typename graph_traits< Graph >::vertex_iterator vi, vie;
            typename graph_traits< Graph >::out_edge_iterator oei, oeie;
            float_t cz = (std::numeric_limits< float_t >::max)(); // Closest to
                                                                  // zero value
            float_t s = 0;
            const float_t eps_ = std::numeric_limits< float_t >::epsilon();
            for (boost::tie(vi, vie) = vertices(m_g); vi != vie; ++vi)
            {
                for (boost::tie(oei, oeie) = out_edges(*vi, m_g); oei != oeie;
                     ++oei)
                {
                    s += std::abs(m_ew1m[*oei]);
                    float_t a = std::abs(m_ew2m[*oei]);
                    if (a > eps_ && a < cz)
                    {
                        cz = a;
                    }
                }
            }
            return cmp_props_t::multiplier * (s / cz);
        }

        /*!
         *  Constructs an arbitrary policy graph.
         */
        void construct_policy_graph()
        {
            m_sink = graph_traits< Graph >().null_vertex();
            typename graph_traits< Graph >::vertex_iterator vi, vie;
            typename graph_traits< Graph >::out_edge_iterator oei, oeie;
            for (boost::tie(vi, vie) = vertices(m_g); vi != vie; ++vi)
            {
                using namespace boost::placeholders;

                boost::tie(oei, oeie) = out_edges(*vi, m_g);
                typename graph_traits< Graph >::out_edge_iterator mei
                    = boost::first_max_element(oei, oeie,
                        boost::bind(m_cmp,
                            boost::bind(&EdgeWeight1::operator[], m_ew1m, _1),
                            boost::bind(&EdgeWeight1::operator[], m_ew1m, _2)));
                if (mei == oeie)
                {
                    if (m_sink == graph_traits< Graph >().null_vertex())
                    {
                        m_sink = *vi;
                    }
                    m_badv[*vi] = true;
                    m_inel[m_sink].push_back(*vi);
                }
                else
                {
                    m_inel[target(*mei, m_g)].push_back(*vi);
                    m_policy[*vi] = *mei;
                }
            }
        }
        /*! Sets the distance value for all vertices "v" such that there is
         * a path from "v" to "sv". It does "inverse" breadth first visit of the
         * policy graph, starting from the vertex "sv".
         */
        void mcr_bfv(vertex_t sv, float_t cr, color_map_t c)
        {
            boost::queue< vertex_t > Q;
            c[sv] = my_black;
            Q.push(sv);
            while (!Q.empty())
            {
                vertex_t v = Q.top();
                Q.pop();
                for (typename pinel_t::const_iterator itr = m_inel[v].begin();
                     itr != m_inel[v].end(); ++itr)
                // For all in_edges of the policy graph
                {
                    if (*itr != sv)
                    {
                        if (m_badv[*itr])
                        {
                            m_dm[*itr] = m_dm[v] + m_bound - cr;
                        }
                        else
                        {
                            m_dm[*itr] = m_dm[v] + m_ew1m[m_policy[*itr]]
                                - m_ew2m[m_policy[*itr]] * cr;
                        }
                        c[*itr] = my_black;
                        Q.push(*itr);
                    }
                }
            }
        }

        /*!
         * \param sv an arbitrary (undiscovered) vertex of the policy graph.
         * \return a vertex in the policy graph that belongs to a cycle.
         * Performs a depth first visit until a cycle edge is found.
         */
        vertex_t find_cycle_vertex(vertex_t sv)
        {
            vertex_t gv = sv;
            std::fill(m_colcv.begin(), m_colcv.end(), my_white);
            color_map_t cm(m_colcv.begin(), m_vim);
            do
            {
                cm[gv] = my_black;
                if (!m_badv[gv])
                {
                    gv = target(m_policy[gv], m_g);
                }
                else
                {
                    gv = m_sink;
                }
            } while (cm[gv] != my_black);
            return gv;
        }

        /*!
         * \param sv - vertex that belongs to a cycle in the policy graph.
         */
        float_t cycle_ratio(vertex_t sv)
        {
            if (sv == m_sink)
                return m_bound;
            std::pair< float_t, float_t > sums_(float_t(0), float_t(0));
            vertex_t v = sv;
            critical_cycle_t cc;
            do
            {
                store_critical_edge(m_policy[v], cc);
                sums_.first += m_ew1m[m_policy[v]];
                sums_.second += m_ew2m[m_policy[v]];
                v = target(m_policy[v], m_g);
            } while (v != sv);
            float_t cr = sums_.first / sums_.second;
            if (m_cmp(m_cr, cr))
            {
                m_cr = cr;
                store_critical_cycle(cc);
            }
            return cr;
        }

        /*!
         *  Finds the optimal cycle ratio of the policy graph
         */
        float_t policy_mcr()
        {
            using namespace boost::placeholders;

            std::fill(m_col_bfs.begin(), m_col_bfs.end(), my_white);
            color_map_t vcm_ = color_map_t(m_col_bfs.begin(), m_vim);
            typename graph_traits< Graph >::vertex_iterator uv_itr, vie;
            boost::tie(uv_itr, vie) = vertices(m_g);
            float_t mcr = m_bound;
            while ((uv_itr = std::find_if(uv_itr, vie,
                        boost::bind(std::equal_to< my_color_type >(), my_white,
                            boost::bind(&color_map_t::operator[], vcm_, _1))))
                != vie)
            /// While there are undiscovered vertices
            {
                vertex_t gv = find_cycle_vertex(*uv_itr);
                float_t cr = cycle_ratio(gv);
                mcr_bfv(gv, cr, vcm_);
                if (m_cmp(mcr, cr))
                    mcr = cr;
                ++uv_itr;
            }
            return mcr;
        }

        /*!
         * Changes the edge m_policy[s] to the new_edge.
         */
        void improve_policy(vertex_t s, edge_t new_edge)
        {
            vertex_t t = target(m_policy[s], m_g);
            typename property_traits< VertexIndexMap >::value_type ti
                = m_vim[t];
            m_inelc[ti].erase(
                std::find(m_inelc[ti].begin(), m_inelc[ti].end(), s));
            m_policy[s] = new_edge;
            t = target(new_edge, m_g);
            m_inel[t].push_back(s); /// Maintain in_edge list
        }

        /*!
         * A negative cycle detector.
         */
        bool try_improve_policy(float_t cr)
        {
            bool improved = false;
            typename graph_traits< Graph >::vertex_iterator vi, vie;
            typename graph_traits< Graph >::out_edge_iterator oei, oeie;
            const float_t eps_ = FloatTraits::epsilon();
            for (boost::tie(vi, vie) = vertices(m_g); vi != vie; ++vi)
            {
                if (!m_badv[*vi])
                {
                    for (boost::tie(oei, oeie) = out_edges(*vi, m_g);
                         oei != oeie; ++oei)
                    {
                        vertex_t t = target(*oei, m_g);
                        // Current distance from *vi to some vertex
                        float_t dis_
                            = m_ew1m[*oei] - m_ew2m[*oei] * cr + m_dm[t];
                        if (m_cmp(m_dm[*vi] + eps_, dis_))
                        {
                            improve_policy(*vi, *oei);
                            m_dm[*vi] = dis_;
                            improved = true;
                        }
                    }
                }
                else
                {
                    float_t dis_ = m_bound - cr + m_dm[m_sink];
                    if (m_cmp(m_dm[*vi] + eps_, dis_))
                    {
                        m_dm[*vi] = dis_;
                    }
                }
            }
            return improved;
        }

    private:
        const Graph& m_g;
        VertexIndexMap m_vim;
        EdgeWeight1 m_ew1m;
        EdgeWeight2 m_ew2m;
        comparator_t m_cmp;
        float_t m_bound; //> The lower/upper bound to the maximal/minimal cycle
                         // ratio
        float_t m_cr; //>The best cycle ratio that has been found so far

        vn_t m_V; //>The number of the vertices in the graph
        vp_t m_dis; //>Container for the distance map
        distance_map_t m_dm; //>Distance map

        ve_t m_policyc; //>Container for the policy graph
        policy_t m_policy; //>The interface for the policy graph

        inedges1_t m_inelc; //>Container fot in edges list
        inedges_t m_inel; //>Policy graph, input edges list

        std::vector< int > m_badvc;
        badv_t m_badv; // Marks "bad" vertices

        vcol_t m_colcv, m_col_bfs; // Color maps
        vertex_t m_sink; // To convert any graph to "good"
    };

    /*! \class mcr_howard1
     * \brief Finds optimum cycle raio and a critical cycle
     */
    template < typename FloatTraits, typename Graph, typename VertexIndexMap,
        typename EdgeWeight1, typename EdgeWeight2 >
    class mcr_howard1 : public mcr_howard< FloatTraits, Graph, VertexIndexMap,
                            EdgeWeight1, EdgeWeight2 >
    {
    public:
        typedef mcr_howard< FloatTraits, Graph, VertexIndexMap, EdgeWeight1,
            EdgeWeight2 >
            inhr_t;
        mcr_howard1(const Graph& g, VertexIndexMap vim, EdgeWeight1 ewm,
            EdgeWeight2 ew2m)
        : inhr_t(g, vim, ewm, ew2m)
        {
        }

        void get_critical_cycle(typename inhr_t::critical_cycle_t& cc)
        {
            return cc.swap(m_cc);
        }

    protected:
        void store_critical_edge(
            typename inhr_t::edge_t ed, typename inhr_t::critical_cycle_t& cc)
        {
            cc.push_back(ed);
        }

        void store_critical_cycle(typename inhr_t::critical_cycle_t& cc)
        {
            m_cc.swap(cc);
        }

    private:
        typename inhr_t::critical_cycle_t m_cc; // Critical cycle
    };

    /*!
     * \param g a directed multigraph.
     * \param vim Vertex Index Map. A map V->[0, num_vertices(g))
     * \param ewm Edge weight1 map.
     * \param ew2m Edge weight2 map.
     * \param pcc  pointer to the critical edges list.
     * \return Optimum cycle ratio of g or FloatTraits::infinity() if g has no
     * cycles.
     */
    template < typename FT, typename TG, typename TVIM, typename TEW1,
        typename TEW2, typename EV >
    typename FT::value_type optimum_cycle_ratio(
        const TG& g, TVIM vim, TEW1 ewm, TEW2 ew2m, EV* pcc)
    {
        typedef typename graph_traits< TG >::directed_category DirCat;
        BOOST_STATIC_ASSERT(
            (is_convertible< DirCat*, directed_tag* >::value == true));
        BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< TG >));
        BOOST_CONCEPT_ASSERT((VertexListGraphConcept< TG >));
        typedef typename graph_traits< TG >::vertex_descriptor Vertex;
        BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< TVIM, Vertex >));
        typedef typename graph_traits< TG >::edge_descriptor Edge;
        BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< TEW1, Edge >));
        BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< TEW2, Edge >));

        if (pcc == 0)
        {
            return detail::mcr_howard< FT, TG, TVIM, TEW1, TEW2 >(
                g, vim, ewm, ew2m)
                .ocr_howard();
        }

        detail::mcr_howard1< FT, TG, TVIM, TEW1, TEW2 > obj(g, vim, ewm, ew2m);
        double ocr = obj.ocr_howard();
        obj.get_critical_cycle(*pcc);
        return ocr;
    }
} // namespace detail

// Algorithms
// Maximum Cycle Ratio

template < typename FloatTraits, typename Graph, typename VertexIndexMap,
    typename EdgeWeight1Map, typename EdgeWeight2Map >
inline typename FloatTraits::value_type maximum_cycle_ratio(const Graph& g,
    VertexIndexMap vim, EdgeWeight1Map ew1m, EdgeWeight2Map ew2m,
    std::vector< typename graph_traits< Graph >::edge_descriptor >* pcc = 0,
    FloatTraits = FloatTraits())
{
    typedef detail::float_wrapper< FloatTraits,
        detail::max_comparator_props< FloatTraits > >
        Traits;
    return detail::optimum_cycle_ratio< Traits >(g, vim, ew1m, ew2m, pcc);
}

template < typename Graph, typename VertexIndexMap, typename EdgeWeight1Map,
    typename EdgeWeight2Map >
inline double maximum_cycle_ratio(const Graph& g, VertexIndexMap vim,
    EdgeWeight1Map ew1m, EdgeWeight2Map ew2m,
    std::vector< typename graph_traits< Graph >::edge_descriptor >* pcc = 0)
{
    return maximum_cycle_ratio(g, vim, ew1m, ew2m, pcc, mcr_float<>());
}

// Minimum Cycle Ratio

template < typename FloatTraits, typename Graph, typename VertexIndexMap,
    typename EdgeWeight1Map, typename EdgeWeight2Map >
typename FloatTraits::value_type minimum_cycle_ratio(const Graph& g,
    VertexIndexMap vim, EdgeWeight1Map ew1m, EdgeWeight2Map ew2m,
    std::vector< typename graph_traits< Graph >::edge_descriptor >* pcc = 0,
    FloatTraits = FloatTraits())
{
    typedef detail::float_wrapper< FloatTraits,
        detail::min_comparator_props< FloatTraits > >
        Traits;
    return detail::optimum_cycle_ratio< Traits >(g, vim, ew1m, ew2m, pcc);
}

template < typename Graph, typename VertexIndexMap, typename EdgeWeight1Map,
    typename EdgeWeight2Map >
inline double minimum_cycle_ratio(const Graph& g, VertexIndexMap vim,
    EdgeWeight1Map ew1m, EdgeWeight2Map ew2m,
    std::vector< typename graph_traits< Graph >::edge_descriptor >* pcc = 0)
{
    return minimum_cycle_ratio(g, vim, ew1m, ew2m, pcc, mcr_float<>());
}

// Maximum Cycle Mean

template < typename FloatTraits, typename Graph, typename VertexIndexMap,
    typename EdgeWeightMap, typename EdgeIndexMap >
inline typename FloatTraits::value_type maximum_cycle_mean(const Graph& g,
    VertexIndexMap vim, EdgeWeightMap ewm, EdgeIndexMap eim,
    std::vector< typename graph_traits< Graph >::edge_descriptor >* pcc = 0,
    FloatTraits ft = FloatTraits())
{
    typedef typename remove_const<
        typename property_traits< EdgeWeightMap >::value_type >::type Weight;
    typename std::vector< Weight > ed_w2(boost::num_edges(g), 1);
    return maximum_cycle_ratio(
        g, vim, ewm, make_iterator_property_map(ed_w2.begin(), eim), pcc, ft);
}

template < typename Graph, typename VertexIndexMap, typename EdgeWeightMap,
    typename EdgeIndexMap >
inline double maximum_cycle_mean(const Graph& g, VertexIndexMap vim,
    EdgeWeightMap ewm, EdgeIndexMap eim,
    std::vector< typename graph_traits< Graph >::edge_descriptor >* pcc = 0)
{
    return maximum_cycle_mean(g, vim, ewm, eim, pcc, mcr_float<>());
}

// Minimum Cycle Mean

template < typename FloatTraits, typename Graph, typename VertexIndexMap,
    typename EdgeWeightMap, typename EdgeIndexMap >
inline typename FloatTraits::value_type minimum_cycle_mean(const Graph& g,
    VertexIndexMap vim, EdgeWeightMap ewm, EdgeIndexMap eim,
    std::vector< typename graph_traits< Graph >::edge_descriptor >* pcc = 0,
    FloatTraits ft = FloatTraits())
{
    typedef typename remove_const<
        typename property_traits< EdgeWeightMap >::value_type >::type Weight;
    typename std::vector< Weight > ed_w2(boost::num_edges(g), 1);
    return minimum_cycle_ratio(
        g, vim, ewm, make_iterator_property_map(ed_w2.begin(), eim), pcc, ft);
}

template < typename Graph, typename VertexIndexMap, typename EdgeWeightMap,
    typename EdgeIndexMap >
inline double minimum_cycle_mean(const Graph& g, VertexIndexMap vim,
    EdgeWeightMap ewm, EdgeIndexMap eim,
    std::vector< typename graph_traits< Graph >::edge_descriptor >* pcc = 0)
{
    return minimum_cycle_mean(g, vim, ewm, eim, pcc, mcr_float<>());
}

} // namespace boost

#endif
