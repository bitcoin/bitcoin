// Copyright 2004, 2005 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Nick Edmonds
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_SSCA_GENERATOR_HPP
#define BOOST_GRAPH_SSCA_GENERATOR_HPP

#include <iterator>
#include <utility>
#include <vector>
#include <queue>
#include <boost/config.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/type_traits/is_same.hpp>

enum Direction
{
    FORWARD = 1,
    BACKWARD = 2,
    BOTH = FORWARD | BACKWARD
};

namespace boost
{

// This generator generates graphs according to the method specified
// in SSCA 1.1.  Current versions of SSCA use R-MAT graphs

template < typename RandomGenerator, typename Graph > class ssca_iterator
{
    typedef typename graph_traits< Graph >::directed_category directed_category;
    typedef
        typename graph_traits< Graph >::vertices_size_type vertices_size_type;

public:
    typedef std::input_iterator_tag iterator_category;
    typedef std::pair< vertices_size_type, vertices_size_type > value_type;
    typedef const value_type& reference;
    typedef const value_type* pointer;
    typedef void difference_type;

    // No argument constructor, set to terminating condition
    ssca_iterator() : gen(), verticesRemaining(0) {}

    // Initialize for edge generation
    ssca_iterator(RandomGenerator& gen, vertices_size_type totVertices,
        vertices_size_type maxCliqueSize, double probUnidirectional,
        int maxParallelEdges, double probIntercliqueEdges)
    : gen(&gen)
    , totVertices(totVertices)
    , maxCliqueSize(maxCliqueSize)
    , probUnidirectional(probUnidirectional)
    , maxParallelEdges(maxParallelEdges)
    , probIntercliqueEdges(probIntercliqueEdges)
    , currentClique(0)
    , verticesRemaining(totVertices)
    {
        cliqueNum = std::vector< int >(totVertices, -1);
        current = std::make_pair(0, 0);
    }

    reference operator*() const { return current; }
    pointer operator->() const { return &current; }

    ssca_iterator& operator++()
    {
        BOOST_USING_STD_MIN();
        while (values.empty() && verticesRemaining > 0)
        { // If there are no values left, generate a new clique
            uniform_int< vertices_size_type > clique_size(1, maxCliqueSize);
            uniform_int< vertices_size_type > rand_vertex(0, totVertices - 1);
            uniform_int< int > num_parallel_edges(1, maxParallelEdges);
            uniform_int< short > direction(0, 1);
            uniform_01< RandomGenerator > prob(*gen);
            std::vector< vertices_size_type > cliqueVertices;

            cliqueVertices.clear();
            vertices_size_type size = min BOOST_PREVENT_MACRO_SUBSTITUTION(
                clique_size(*gen), verticesRemaining);
            while (cliqueVertices.size() < size)
            {
                vertices_size_type v = rand_vertex(*gen);
                if (cliqueNum[v] == -1)
                {
                    cliqueNum[v] = currentClique;
                    cliqueVertices.push_back(v);
                    verticesRemaining--;
                }
            } // Nick: This is inefficient when only a few vertices remain...
              //       I should probably just select the remaining vertices
              //       in order when only a certain fraction remain.

            typename std::vector< vertices_size_type >::iterator first, second;
            for (first = cliqueVertices.begin(); first != cliqueVertices.end();
                 ++first)
                for (second = first + 1; second != cliqueVertices.end();
                     ++second)
                {
                    Direction d;
                    int edges;

                    d = prob() < probUnidirectional
                        ? (direction(*gen) == 0 ? FORWARD : BACKWARD)
                        : BOTH;

                    if (d & FORWARD)
                    {
                        edges = num_parallel_edges(*gen);
                        for (int i = 0; i < edges; ++i)
                            values.push(std::make_pair(*first, *second));
                    }

                    if (d & BACKWARD)
                    {
                        edges = num_parallel_edges(*gen);
                        for (int i = 0; i < edges; ++i)
                            values.push(std::make_pair(*second, *first));
                    }
                }

            if (verticesRemaining == 0)
            {
                // Generate interclique edges
                for (vertices_size_type i = 0; i < totVertices; ++i)
                {
                    double p = probIntercliqueEdges;
                    for (vertices_size_type d = 2; d < totVertices / 2;
                         d *= 2, p /= 2)
                    {
                        vertices_size_type j = (i + d) % totVertices;
                        if (cliqueNum[j] != cliqueNum[i] && prob() < p)
                        {
                            int edges = num_parallel_edges(*gen);
                            for (int i = 0; i < edges; ++i)
                                values.push(std::make_pair(i, j));
                        }
                    }
                }
            }

            currentClique++;
        }

        if (!values.empty())
        { // If we're not done return a value
            current = values.front();
            values.pop();
        }

        return *this;
    }

    ssca_iterator operator++(int)
    {
        ssca_iterator temp(*this);
        ++(*this);
        return temp;
    }

    bool operator==(const ssca_iterator& other) const
    {
        return verticesRemaining == other.verticesRemaining && values.empty()
            && other.values.empty();
    }

    bool operator!=(const ssca_iterator& other) const
    {
        return !(*this == other);
    }

private:
    // Parameters
    RandomGenerator* gen;
    vertices_size_type totVertices;
    vertices_size_type maxCliqueSize;
    double probUnidirectional;
    int maxParallelEdges;
    double probIntercliqueEdges;

    // Internal data structures
    std::vector< int > cliqueNum;
    std::queue< value_type > values;
    int currentClique;
    vertices_size_type verticesRemaining;
    value_type current;
};

} // end namespace boost

#endif // BOOST_GRAPH_SSCA_GENERATOR_HPP
