// Copyright 2004 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_KAMADA_KAWAI_SPRING_LAYOUT_HPP
#define BOOST_GRAPH_KAMADA_KAWAI_SPRING_LAYOUT_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/topology.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/johnson_all_pairs_shortest.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <utility>
#include <iterator>
#include <vector>
#include <iostream>
#include <boost/limits.hpp>
#include <boost/config/no_tr1/cmath.hpp>

namespace boost
{
namespace detail
{
    namespace graph
    {
        /**
         * Denotes an edge or display area side length used to scale a
         * Kamada-Kawai drawing.
         */
        template < bool Edge, typename T > struct edge_or_side
        {
            explicit edge_or_side(T value) : value(value) {}

            T value;
        };

        /**
         * Compute the edge length from an edge length. This is trivial.
         */
        template < typename Graph, typename DistanceMap, typename IndexMap,
            typename T >
        T compute_edge_length(
            const Graph&, DistanceMap, IndexMap, edge_or_side< true, T > length)
        {
            return length.value;
        }

        /**
         * Compute the edge length based on the display area side
           length. We do this by dividing the side length by the largest
           shortest distance between any two vertices in the graph.
         */
        template < typename Graph, typename DistanceMap, typename IndexMap,
            typename T >
        T compute_edge_length(const Graph& g, DistanceMap distance,
            IndexMap index, edge_or_side< false, T > length)
        {
            T result(0);

            typedef
                typename graph_traits< Graph >::vertex_iterator vertex_iterator;

            for (vertex_iterator ui = vertices(g).first,
                                 end = vertices(g).second;
                 ui != end; ++ui)
            {
                vertex_iterator vi = ui;
                for (++vi; vi != end; ++vi)
                {
                    T dij = distance[get(index, *ui)][get(index, *vi)];
                    if (dij > result)
                        result = dij;
                }
            }
            return length.value / result;
        }

        /**
         * Dense linear solver for fixed-size matrices.
         */
        template < std::size_t Size > struct linear_solver
        {
            // Indices in mat are (row, column)
            // template <typename Vec>
            // static Vec solve(double mat[Size][Size], Vec rhs);
        };

        template <> struct linear_solver< 1 >
        {
            template < typename Vec >
            static Vec solve(double mat[1][1], Vec rhs)
            {
                return rhs / mat[0][0];
            }
        };

        // These are from http://en.wikipedia.org/wiki/Cramer%27s_rule
        template <> struct linear_solver< 2 >
        {
            template < typename Vec >
            static Vec solve(double mat[2][2], Vec rhs)
            {
                double denom = mat[0][0] * mat[1][1] - mat[1][0] * mat[0][1];
                double x_num = rhs[0] * mat[1][1] - rhs[1] * mat[0][1];
                double y_num = mat[0][0] * rhs[1] - mat[1][0] * rhs[0];
                Vec result;
                result[0] = x_num / denom;
                result[1] = y_num / denom;
                return result;
            }
        };

        template <> struct linear_solver< 3 >
        {
            template < typename Vec >
            static Vec solve(double mat[3][3], Vec rhs)
            {
                double denom = mat[0][0]
                        * (mat[1][1] * mat[2][2] - mat[2][1] * mat[1][2])
                    - mat[1][0]
                        * (mat[0][1] * mat[2][2] - mat[2][1] * mat[0][2])
                    + mat[2][0]
                        * (mat[0][1] * mat[1][2] - mat[1][1] * mat[0][2]);
                double x_num
                    = rhs[0] * (mat[1][1] * mat[2][2] - mat[2][1] * mat[1][2])
                    - rhs[1] * (mat[0][1] * mat[2][2] - mat[2][1] * mat[0][2])
                    + rhs[2] * (mat[0][1] * mat[1][2] - mat[1][1] * mat[0][2]);
                double y_num
                    = mat[0][0] * (rhs[1] * mat[2][2] - rhs[2] * mat[1][2])
                    - mat[1][0] * (rhs[0] * mat[2][2] - rhs[2] * mat[0][2])
                    + mat[2][0] * (rhs[0] * mat[1][2] - rhs[1] * mat[0][2]);
                double z_num
                    = mat[0][0] * (mat[1][1] * rhs[2] - mat[2][1] * rhs[1])
                    - mat[1][0] * (mat[0][1] * rhs[2] - mat[2][1] * rhs[0])
                    + mat[2][0] * (mat[0][1] * rhs[1] - mat[1][1] * rhs[0]);
                Vec result;
                result[0] = x_num / denom;
                result[1] = y_num / denom;
                result[2] = z_num / denom;
                return result;
            }
        };

        /**
         * Implementation of the Kamada-Kawai spring layout algorithm.
         */
        template < typename Topology, typename Graph, typename PositionMap,
            typename WeightMap, typename EdgeOrSideLength, typename Done,
            typename VertexIndexMap, typename DistanceMatrix,
            typename SpringStrengthMatrix, typename PartialDerivativeMap >
        struct kamada_kawai_spring_layout_impl
        {
            typedef
                typename property_traits< WeightMap >::value_type weight_type;
            typedef typename Topology::point_type Point;
            typedef
                typename Topology::point_difference_type point_difference_type;
            typedef point_difference_type deriv_type;
            typedef
                typename graph_traits< Graph >::vertex_iterator vertex_iterator;
            typedef typename graph_traits< Graph >::vertex_descriptor
                vertex_descriptor;

            kamada_kawai_spring_layout_impl(const Topology& topology,
                const Graph& g, PositionMap position, WeightMap weight,
                EdgeOrSideLength edge_or_side_length, Done done,
                weight_type spring_constant, VertexIndexMap index,
                DistanceMatrix distance, SpringStrengthMatrix spring_strength,
                PartialDerivativeMap partial_derivatives)
            : topology(topology)
            , g(g)
            , position(position)
            , weight(weight)
            , edge_or_side_length(edge_or_side_length)
            , done(done)
            , spring_constant(spring_constant)
            , index(index)
            , distance(distance)
            , spring_strength(spring_strength)
            , partial_derivatives(partial_derivatives)
            {
            }

            // Compute contribution of vertex i to the first partial
            // derivatives (dE/dx_m, dE/dy_m) (for vertex m)
            deriv_type compute_partial_derivative(
                vertex_descriptor m, vertex_descriptor i)
            {
#ifndef BOOST_NO_STDC_NAMESPACE
                using std::sqrt;
#endif // BOOST_NO_STDC_NAMESPACE

                deriv_type result;
                if (i != m)
                {
                    point_difference_type diff
                        = topology.difference(position[m], position[i]);
                    weight_type dist = topology.norm(diff);
                    result = spring_strength[get(index, m)][get(index, i)]
                        * (diff
                            - distance[get(index, m)][get(index, i)] / dist
                                * diff);
                }

                return result;
            }

            // Compute partial derivatives dE/dx_m and dE/dy_m
            deriv_type compute_partial_derivatives(vertex_descriptor m)
            {
#ifndef BOOST_NO_STDC_NAMESPACE
                using std::sqrt;
#endif // BOOST_NO_STDC_NAMESPACE

                deriv_type result;

                // TBD: looks like an accumulate to me
                BGL_FORALL_VERTICES_T(i, g, Graph)
                {
                    deriv_type deriv = compute_partial_derivative(m, i);
                    result += deriv;
                }

                return result;
            }

            // The actual Kamada-Kawai spring layout algorithm implementation
            bool run()
            {
#ifndef BOOST_NO_STDC_NAMESPACE
                using std::sqrt;
#endif // BOOST_NO_STDC_NAMESPACE

                // Compute d_{ij} and place it in the distance matrix
                if (!johnson_all_pairs_shortest_paths(
                        g, distance, index, weight, weight_type(0)))
                    return false;

                // Compute L based on side length (if needed), or retrieve L
                weight_type edge_length = detail::graph::compute_edge_length(
                    g, distance, index, edge_or_side_length);

                // std::cerr << "edge_length = " << edge_length << std::endl;

                // Compute l_{ij} and k_{ij}
                const weight_type K = spring_constant;
                vertex_iterator ui, end;
                for (ui = vertices(g).first, end = vertices(g).second;
                     ui != end; ++ui)
                {
                    vertex_iterator vi = ui;
                    for (++vi; vi != end; ++vi)
                    {
                        weight_type dij
                            = distance[get(index, *ui)][get(index, *vi)];
                        if (dij == (std::numeric_limits< weight_type >::max)())
                            return false;
                        distance[get(index, *ui)][get(index, *vi)]
                            = edge_length * dij;
                        distance[get(index, *vi)][get(index, *ui)]
                            = edge_length * dij;
                        spring_strength[get(index, *ui)][get(index, *vi)]
                            = K / (dij * dij);
                        spring_strength[get(index, *vi)][get(index, *ui)]
                            = K / (dij * dij);
                    }
                }

                // Compute Delta_i and find max
                vertex_descriptor p = *vertices(g).first;
                weight_type delta_p(0);

                for (ui = vertices(g).first, end = vertices(g).second;
                     ui != end; ++ui)
                {
                    deriv_type deriv = compute_partial_derivatives(*ui);
                    put(partial_derivatives, *ui, deriv);

                    weight_type delta = topology.norm(deriv);

                    if (delta > delta_p)
                    {
                        p = *ui;
                        delta_p = delta;
                    }
                }

                while (!done(delta_p, p, g, true))
                {
                    // The contribution p makes to the partial derivatives of
                    // each vertex. Computing this (at O(n) cost) allows us to
                    // update the delta_i values in O(n) time instead of O(n^2)
                    // time.
                    std::vector< deriv_type > p_partials(num_vertices(g));
                    for (ui = vertices(g).first, end = vertices(g).second;
                         ui != end; ++ui)
                    {
                        vertex_descriptor i = *ui;
                        p_partials[get(index, i)]
                            = compute_partial_derivative(i, p);
                    }

                    do
                    {
                        // For debugging, compute the energy value E
                        double E = 0.;
                        for (ui = vertices(g).first, end = vertices(g).second;
                             ui != end; ++ui)
                        {
                            vertex_iterator vi = ui;
                            for (++vi; vi != end; ++vi)
                            {
                                double dist = topology.distance(
                                    position[*ui], position[*vi]);
                                weight_type k_ij = spring_strength[get(
                                    index, *ui)][get(index, *vi)];
                                weight_type l_ij = distance[get(index, *ui)]
                                                           [get(index, *vi)];
                                E += .5 * k_ij * (dist - l_ij) * (dist - l_ij);
                            }
                        }
                        // std::cerr << "E = " << E << std::endl;

                        // Compute the elements of the Jacobian
                        // From
                        // http://www.cs.panam.edu/~rfowler/papers/1994_kumar_fowler_A_Spring_UTPACSTR.pdf
                        // with the bugs fixed in the off-diagonal case
                        weight_type dE_d_d[Point::dimensions]
                                          [Point::dimensions];
                        for (std::size_t i = 0; i < Point::dimensions; ++i)
                            for (std::size_t j = 0; j < Point::dimensions; ++j)
                                dE_d_d[i][j] = 0.;
                        for (ui = vertices(g).first, end = vertices(g).second;
                             ui != end; ++ui)
                        {
                            vertex_descriptor i = *ui;
                            if (i != p)
                            {
                                point_difference_type diff
                                    = topology.difference(
                                        position[p], position[i]);
                                weight_type dist = topology.norm(diff);
                                weight_type dist_squared = dist * dist;
                                weight_type inv_dist_cubed
                                    = 1. / (dist_squared * dist);
                                weight_type k_mi = spring_strength[get(
                                    index, p)][get(index, i)];
                                weight_type l_mi
                                    = distance[get(index, p)][get(index, i)];
                                for (std::size_t i = 0; i < Point::dimensions;
                                     ++i)
                                {
                                    for (std::size_t j = 0;
                                         j < Point::dimensions; ++j)
                                    {
                                        if (i == j)
                                        {
                                            dE_d_d[i][i] += k_mi
                                                * (1
                                                    + (l_mi
                                                        * (diff[i] * diff[i]
                                                            - dist_squared)
                                                        * inv_dist_cubed));
                                        }
                                        else
                                        {
                                            dE_d_d[i][j] += k_mi * l_mi
                                                * diff[i] * diff[j]
                                                * inv_dist_cubed;
                                            // dE_d_d[i][j] += k_mi * l_mi *
                                            // sqrt(hypot(diff[i], diff[j])) *
                                            // inv_dist_cubed;
                                        }
                                    }
                                }
                            }
                        }

                        deriv_type dE_d = get(partial_derivatives, p);

                        // Solve dE_d_d * delta = -dE_d to get delta
                        point_difference_type delta
                            = -linear_solver< Point::dimensions >::solve(
                                dE_d_d, dE_d);

                        // Move p by delta
                        position[p] = topology.adjust(position[p], delta);

                        // Recompute partial derivatives and delta_p
                        deriv_type deriv = compute_partial_derivatives(p);
                        put(partial_derivatives, p, deriv);

                        delta_p = topology.norm(deriv);
                    } while (!done(delta_p, p, g, false));

                    // Select new p by updating each partial derivative and
                    // delta
                    vertex_descriptor old_p = p;
                    for (ui = vertices(g).first, end = vertices(g).second;
                         ui != end; ++ui)
                    {
                        deriv_type old_deriv_p = p_partials[get(index, *ui)];
                        deriv_type old_p_partial
                            = compute_partial_derivative(*ui, old_p);
                        deriv_type deriv = get(partial_derivatives, *ui);

                        deriv += old_p_partial - old_deriv_p;

                        put(partial_derivatives, *ui, deriv);
                        weight_type delta = topology.norm(deriv);

                        if (delta > delta_p)
                        {
                            p = *ui;
                            delta_p = delta;
                        }
                    }
                }

                return true;
            }

            const Topology& topology;
            const Graph& g;
            PositionMap position;
            WeightMap weight;
            EdgeOrSideLength edge_or_side_length;
            Done done;
            weight_type spring_constant;
            VertexIndexMap index;
            DistanceMatrix distance;
            SpringStrengthMatrix spring_strength;
            PartialDerivativeMap partial_derivatives;
        };
    }
} // end namespace detail::graph

/// States that the given quantity is an edge length.
template < typename T >
inline detail::graph::edge_or_side< true, T > edge_length(T x)
{
    return detail::graph::edge_or_side< true, T >(x);
}

/// States that the given quantity is a display area side length.
template < typename T >
inline detail::graph::edge_or_side< false, T > side_length(T x)
{
    return detail::graph::edge_or_side< false, T >(x);
}

/**
 * \brief Determines when to terminate layout of a particular graph based
 * on a given relative tolerance.
 */
template < typename T = double > struct layout_tolerance
{
    layout_tolerance(const T& tolerance = T(0.001))
    : tolerance(tolerance)
    , last_energy((std::numeric_limits< T >::max)())
    , last_local_energy((std::numeric_limits< T >::max)())
    {
    }

    template < typename Graph >
    bool operator()(T delta_p,
        typename boost::graph_traits< Graph >::vertex_descriptor p,
        const Graph& g, bool global)
    {
        if (global)
        {
            if (last_energy == (std::numeric_limits< T >::max)())
            {
                last_energy = delta_p;
                return false;
            }

            T diff = last_energy - delta_p;
            if (diff < T(0))
                diff = -diff;
            bool done = (delta_p == T(0) || diff / last_energy < tolerance);
            last_energy = delta_p;
            return done;
        }
        else
        {
            if (last_local_energy == (std::numeric_limits< T >::max)())
            {
                last_local_energy = delta_p;
                return delta_p == T(0);
            }

            T diff = last_local_energy - delta_p;
            bool done
                = (delta_p == T(0) || (diff / last_local_energy) < tolerance);
            last_local_energy = delta_p;
            return done;
        }
    }

private:
    T tolerance;
    T last_energy;
    T last_local_energy;
};

/** \brief Kamada-Kawai spring layout for undirected graphs.
 *
 * This algorithm performs graph layout (in two dimensions) for
 * connected, undirected graphs. It operates by relating the layout
 * of graphs to a dynamic spring system and minimizing the energy
 * within that system. The strength of a spring between two vertices
 * is inversely proportional to the square of the shortest distance
 * (in graph terms) between those two vertices. Essentially,
 * vertices that are closer in the graph-theoretic sense (i.e., by
 * following edges) will have stronger springs and will therefore be
 * placed closer together.
 *
 * Prior to invoking this algorithm, it is recommended that the
 * vertices be placed along the vertices of a regular n-sided
 * polygon.
 *
 * \param g (IN) must be a model of Vertex List Graph, Edge List
 * Graph, and Incidence Graph and must be undirected.
 *
 * \param position (OUT) must be a model of Lvalue Property Map,
 * where the value type is a class containing fields @c x and @c y
 * that will be set to the @c x and @c y coordinates of each vertex.
 *
 * \param weight (IN) must be a model of Readable Property Map,
 * which provides the weight of each edge in the graph @p g.
 *
 * \param topology (IN) must be a topology object (see topology.hpp),
 * which provides operations on points and differences between them.
 *
 * \param edge_or_side_length (IN) provides either the unit length
 * @c e of an edge in the layout or the length of a side @c s of the
 * display area, and must be either @c boost::edge_length(e) or @c
 * boost::side_length(s), respectively.
 *
 * \param done (IN) is a 4-argument function object that is passed
 * the current value of delta_p (i.e., the energy of vertex @p p),
 * the vertex @p p, the graph @p g, and a boolean flag indicating
 * whether @p delta_p is the maximum energy in the system (when @c
 * true) or the energy of the vertex being moved. Defaults to @c
 * layout_tolerance instantiated over the value type of the weight
 * map.
 *
 * \param spring_constant (IN) is the constant multiplied by each
 * spring's strength. Larger values create systems with more energy
 * that can take longer to stabilize; smaller values create systems
 * with less energy that stabilize quickly but do not necessarily
 * result in pleasing layouts. The default value is 1.
 *
 * \param index (IN) is a mapping from vertices to index values
 * between 0 and @c num_vertices(g). The default is @c
 * get(vertex_index,g).
 *
 * \param distance (UTIL/OUT) will be used to store the distance
 * from every vertex to every other vertex, which is computed in the
 * first stages of the algorithm. This value's type must be a model
 * of BasicMatrix with value type equal to the value type of the
 * weight map. The default is a vector of vectors.
 *
 * \param spring_strength (UTIL/OUT) will be used to store the
 * strength of the spring between every pair of vertices. This
 * value's type must be a model of BasicMatrix with value type equal
 * to the value type of the weight map. The default is a vector of
 * vectors.
 *
 * \param partial_derivatives (UTIL) will be used to store the
 * partial derivates of each vertex with respect to the @c x and @c
 * y coordinates. This must be a Read/Write Property Map whose value
 * type is a pair with both types equivalent to the value type of
 * the weight map. The default is an iterator property map.
 *
 * \returns @c true if layout was successful or @c false if a
 * negative weight cycle was detected.
 */
template < typename Topology, typename Graph, typename PositionMap,
    typename WeightMap, typename T, bool EdgeOrSideLength, typename Done,
    typename VertexIndexMap, typename DistanceMatrix,
    typename SpringStrengthMatrix, typename PartialDerivativeMap >
bool kamada_kawai_spring_layout(const Graph& g, PositionMap position,
    WeightMap weight, const Topology& topology,
    detail::graph::edge_or_side< EdgeOrSideLength, T > edge_or_side_length,
    Done done,
    typename property_traits< WeightMap >::value_type spring_constant,
    VertexIndexMap index, DistanceMatrix distance,
    SpringStrengthMatrix spring_strength,
    PartialDerivativeMap partial_derivatives)
{
    BOOST_STATIC_ASSERT(
        (is_convertible< typename graph_traits< Graph >::directed_category*,
            undirected_tag* >::value));

    detail::graph::kamada_kawai_spring_layout_impl< Topology, Graph,
        PositionMap, WeightMap,
        detail::graph::edge_or_side< EdgeOrSideLength, T >, Done,
        VertexIndexMap, DistanceMatrix, SpringStrengthMatrix,
        PartialDerivativeMap >
        alg(topology, g, position, weight, edge_or_side_length, done,
            spring_constant, index, distance, spring_strength,
            partial_derivatives);
    return alg.run();
}

/**
 * \overload
 */
template < typename Topology, typename Graph, typename PositionMap,
    typename WeightMap, typename T, bool EdgeOrSideLength, typename Done,
    typename VertexIndexMap >
bool kamada_kawai_spring_layout(const Graph& g, PositionMap position,
    WeightMap weight, const Topology& topology,
    detail::graph::edge_or_side< EdgeOrSideLength, T > edge_or_side_length,
    Done done,
    typename property_traits< WeightMap >::value_type spring_constant,
    VertexIndexMap index)
{
    typedef typename property_traits< WeightMap >::value_type weight_type;

    typename graph_traits< Graph >::vertices_size_type n = num_vertices(g);
    typedef std::vector< weight_type > weight_vec;

    std::vector< weight_vec > distance(n, weight_vec(n));
    std::vector< weight_vec > spring_strength(n, weight_vec(n));
    std::vector< typename Topology::point_difference_type > partial_derivatives(
        n);

    return kamada_kawai_spring_layout(g, position, weight, topology,
        edge_or_side_length, done, spring_constant, index, distance.begin(),
        spring_strength.begin(),
        make_iterator_property_map(partial_derivatives.begin(), index,
            typename Topology::point_difference_type()));
}

/**
 * \overload
 */
template < typename Topology, typename Graph, typename PositionMap,
    typename WeightMap, typename T, bool EdgeOrSideLength, typename Done >
bool kamada_kawai_spring_layout(const Graph& g, PositionMap position,
    WeightMap weight, const Topology& topology,
    detail::graph::edge_or_side< EdgeOrSideLength, T > edge_or_side_length,
    Done done,
    typename property_traits< WeightMap >::value_type spring_constant)
{
    return kamada_kawai_spring_layout(g, position, weight, topology,
        edge_or_side_length, done, spring_constant, get(vertex_index, g));
}

/**
 * \overload
 */
template < typename Topology, typename Graph, typename PositionMap,
    typename WeightMap, typename T, bool EdgeOrSideLength, typename Done >
bool kamada_kawai_spring_layout(const Graph& g, PositionMap position,
    WeightMap weight, const Topology& topology,
    detail::graph::edge_or_side< EdgeOrSideLength, T > edge_or_side_length,
    Done done)
{
    typedef typename property_traits< WeightMap >::value_type weight_type;
    return kamada_kawai_spring_layout(g, position, weight, topology,
        edge_or_side_length, done, weight_type(1));
}

/**
 * \overload
 */
template < typename Topology, typename Graph, typename PositionMap,
    typename WeightMap, typename T, bool EdgeOrSideLength >
bool kamada_kawai_spring_layout(const Graph& g, PositionMap position,
    WeightMap weight, const Topology& topology,
    detail::graph::edge_or_side< EdgeOrSideLength, T > edge_or_side_length)
{
    typedef typename property_traits< WeightMap >::value_type weight_type;
    return kamada_kawai_spring_layout(g, position, weight, topology,
        edge_or_side_length, layout_tolerance< weight_type >(),
        weight_type(1.0), get(vertex_index, g));
}
} // end namespace boost

#endif // BOOST_GRAPH_KAMADA_KAWAI_SPRING_LAYOUT_HPP
