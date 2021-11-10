// Copyright 2004, 2005 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Jeremiah Willcock
//           Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_ERDOS_RENYI_GENERATOR_HPP
#define BOOST_GRAPH_ERDOS_RENYI_GENERATOR_HPP

#include <boost/assert.hpp>
#include <iterator>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/random/geometric_distribution.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/config/no_tr1/cmath.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace boost
{

template < typename RandomGenerator, typename Graph >
class erdos_renyi_iterator
: public iterator_facade< erdos_renyi_iterator< RandomGenerator, Graph >,
      std::pair< typename graph_traits< Graph >::vertices_size_type,
          typename graph_traits< Graph >::vertices_size_type >,
      std::input_iterator_tag,
      const std::pair< typename graph_traits< Graph >::vertices_size_type,
          typename graph_traits< Graph >::vertices_size_type >& >
{
    typedef typename graph_traits< Graph >::directed_category directed_category;
    typedef
        typename graph_traits< Graph >::vertices_size_type vertices_size_type;
    typedef typename graph_traits< Graph >::edges_size_type edges_size_type;

    BOOST_STATIC_CONSTANT(bool,
        is_undirected
        = (is_base_of< undirected_tag, directed_category >::value));

public:
    erdos_renyi_iterator() : gen(), n(0), edges(0), allow_self_loops(false) {}
    erdos_renyi_iterator(RandomGenerator& gen, vertices_size_type n,
        double fraction = 0.0, bool allow_self_loops = false)
    : gen(&gen)
    , n(n)
    , edges(edges_size_type(fraction * n * n))
    , allow_self_loops(allow_self_loops)
    {
        if (is_undirected)
            edges = edges / 2;
        next();
    }

    erdos_renyi_iterator(RandomGenerator& gen, vertices_size_type n,
        edges_size_type m, bool allow_self_loops = false)
    : gen(&gen), n(n), edges(m), allow_self_loops(allow_self_loops)
    {
        next();
    }

    const std::pair< vertices_size_type, vertices_size_type >&
    dereference() const
    {
        return current;
    }

    void increment()
    {
        --edges;
        next();
    }

    bool equal(const erdos_renyi_iterator& other) const
    {
        return edges == other.edges;
    }

private:
    void next()
    {
        uniform_int< vertices_size_type > rand_vertex(0, n - 1);
        current.first = rand_vertex(*gen);
        do
        {
            current.second = rand_vertex(*gen);
        } while (current.first == current.second && !allow_self_loops);
    }

    RandomGenerator* gen;
    vertices_size_type n;
    edges_size_type edges;
    bool allow_self_loops;
    std::pair< vertices_size_type, vertices_size_type > current;
};

template < typename RandomGenerator, typename Graph >
class sorted_erdos_renyi_iterator
: public iterator_facade< sorted_erdos_renyi_iterator< RandomGenerator, Graph >,
      std::pair< typename graph_traits< Graph >::vertices_size_type,
          typename graph_traits< Graph >::vertices_size_type >,
      std::input_iterator_tag,
      const std::pair< typename graph_traits< Graph >::vertices_size_type,
          typename graph_traits< Graph >::vertices_size_type >& >
{
    typedef typename graph_traits< Graph >::directed_category directed_category;
    typedef
        typename graph_traits< Graph >::vertices_size_type vertices_size_type;
    typedef typename graph_traits< Graph >::edges_size_type edges_size_type;

    BOOST_STATIC_CONSTANT(bool,
        is_undirected
        = (is_base_of< undirected_tag, directed_category >::value));

public:
    sorted_erdos_renyi_iterator()
    : gen()
    , rand_vertex(0.5)
    , n(0)
    , allow_self_loops(false)
    , src((std::numeric_limits< vertices_size_type >::max)())
    , tgt_index(vertices_size_type(-1))
    , prob(.5)
    {
    }

    // NOTE: The default probability has been changed to be the same as that
    // used by the geometic distribution. It was previously 0.0, which would
    // cause an assertion.
    sorted_erdos_renyi_iterator(RandomGenerator& gen, vertices_size_type n,
        double prob = 0.5, bool loops = false)
    : gen()
    , rand_vertex(1. - prob)
    , n(n)
    , allow_self_loops(loops)
    , src(0)
    , tgt_index(vertices_size_type(-1))
    , prob(prob)
    {
        this->gen.reset(new uniform_01< RandomGenerator* >(&gen));

        if (prob == 0.0)
        {
            src = (std::numeric_limits< vertices_size_type >::max)();
            return;
        }
        next();
    }

    const std::pair< vertices_size_type, vertices_size_type >&
    dereference() const
    {
        return current;
    }

    bool equal(const sorted_erdos_renyi_iterator& o) const
    {
        return src == o.src && tgt_index == o.tgt_index;
    }

    void increment() { next(); }

private:
    void next()
    {
        // In order to get the edges from the generator in sorted order, one
        // effective (but slow) procedure would be to use a
        // bernoulli_distribution for each legal (src, tgt_index) pair.  Because
        // of the O(|V|^2) cost of that, a geometric distribution is used.  The
        // geometric distribution tells how many times the
        // bernoulli_distribution would need to be run until it returns true.
        // Thus, this distribution can be used to step through the edges
        // which are actually present.
        BOOST_ASSERT(src != (std::numeric_limits< vertices_size_type >::max)()
            && src != n);
        while (src != n)
        {
            vertices_size_type increment = rand_vertex(*gen);
            size_t tgt_index_limit
                = (is_undirected ? src + 1 : n) + (allow_self_loops ? 0 : -1);
            if (tgt_index + increment >= tgt_index_limit)
            {
                // Overflowed this source; go to the next one and try again.
                ++src;
                // This bias is because the geometric distribution always
                // returns values >=1, and we want to allow 0 as a valid target.
                tgt_index = vertices_size_type(-1);
                continue;
            }
            else
            {
                tgt_index += increment;
                current.first = src;
                current.second = tgt_index
                    + (!allow_self_loops && !is_undirected && tgt_index >= src
                            ? 1
                            : 0);
                break;
            }
        }
        if (src == n)
            src = (std::numeric_limits< vertices_size_type >::max)();
    }

    shared_ptr< uniform_01< RandomGenerator* > > gen;
    geometric_distribution< vertices_size_type > rand_vertex;
    vertices_size_type n;
    bool allow_self_loops;
    vertices_size_type src, tgt_index;
    std::pair< vertices_size_type, vertices_size_type > current;
    double prob;
};

} // end namespace boost

#endif // BOOST_GRAPH_ERDOS_RENYI_GENERATOR_HPP
