// Copyright 2004, 2005 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Nick Edmonds
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_DISTRIBUTED_RMAT_GENERATOR_HPP
#define BOOST_GRAPH_DISTRIBUTED_RMAT_GENERATOR_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/assert.hpp>
#include <boost/graph/parallel/algorithm.hpp>
#include <boost/graph/parallel/process_group.hpp>
#include <math.h>

namespace boost {

  // Memory-scalable (amount of memory required will scale down
  // linearly as the number of processes increases) generator, which
  // requires an MPI process group.  Run-time is slightly worse than
  // the unique rmat generator.  Edge list generated is sorted and
  // unique.
  template<typename ProcessGroup, typename Distribution, 
           typename RandomGenerator, typename Graph>
  class scalable_rmat_iterator
  {
      typedef typename graph_traits<Graph>::directed_category directed_category;
      typedef typename graph_traits<Graph>::vertices_size_type vertices_size_type;
      typedef typename graph_traits<Graph>::edges_size_type edges_size_type;

  public:
      typedef std::input_iterator_tag iterator_category;
      typedef std::pair<vertices_size_type, vertices_size_type> value_type;
      typedef const value_type& reference;
      typedef const value_type* pointer;
      typedef void difference_type;

      // No argument constructor, set to terminating condition
      scalable_rmat_iterator()
        : gen(), done(true)
      { }

      // Initialize for edge generation
      scalable_rmat_iterator(ProcessGroup pg, Distribution distrib,
                             RandomGenerator& gen, vertices_size_type n, 
                             edges_size_type m, double a, double b, double c, 
                             double d, bool permute_vertices = true)
          : gen(), done(false)
      {
          BOOST_ASSERT(a + b + c + d == 1);
          int id = process_id(pg);

          this->gen.reset(new uniform_01<RandomGenerator>(gen));

          std::vector<vertices_size_type> vertexPermutation;
          if (permute_vertices) 
              generate_permutation_vector(gen, vertexPermutation, n);

          int SCALE = int(floor(log(double(n))/log(2.)));
          boost::uniform_01<RandomGenerator> prob(gen);
      
          std::map<value_type, bool> edge_map;

          edges_size_type generated = 0, local_edges = 0;
          do {
              edges_size_type tossed = 0;
              do {
                  vertices_size_type u, v;
                  boost::tie(u, v) = generate_edge(this->gen, n, SCALE, a, b, c, d);

                  if (permute_vertices) {
                      u = vertexPermutation[u];
                      v = vertexPermutation[v];
                  }

                  // Lowest vertex number always comes first (this
                  // means we don't have to worry about i->j and j->i
                  // being in the edge list)
                  if (u > v && is_same<directed_category, undirected_tag>::value)
                      std::swap(u, v);

                  if (distrib(u) == id || distrib(v) == id) {
                      if (edge_map.find(std::make_pair(u, v)) == edge_map.end()) {
                          edge_map[std::make_pair(u, v)] = true;
                          local_edges++;
                      } else {
                          tossed++;

                          // special case - if both u and v are on same
                          // proc, ++ twice, since we divide by two (to
                          // cover the two process case)
                          if (distrib(u) == id && distrib(v) == id)
                              tossed++;
                      }
                  }
                  generated++;

              } while (generated < m);
              tossed = all_reduce(pg, tossed, boost::parallel::sum<vertices_size_type>());
              generated -= (tossed / 2);
          } while (generated < m);
          // NGE - Asking for more than n^2 edges will result in an infinite loop here
          //       Asking for a value too close to n^2 edges may as well

          values.reserve(local_edges);
          typename std::map<value_type, bool>::reverse_iterator em_end = edge_map.rend();
          for (typename std::map<value_type, bool>::reverse_iterator em_i = edge_map.rbegin();
               em_i != em_end ;
               ++em_i) {
              values.push_back(em_i->first);
          }

          current = values.back();
          values.pop_back();
      }

      reference operator*() const { return current; }
      pointer operator->() const { return &current; }
    
      scalable_rmat_iterator& operator++()
      {
          if (!values.empty()) {
              current = values.back();
              values.pop_back();
          } else 
              done = true;

          return *this;
      }

      scalable_rmat_iterator operator++(int)
      {
          scalable_rmat_iterator temp(*this);
          ++(*this);
          return temp;
      }

      bool operator==(const scalable_rmat_iterator& other) const
      {
          return values.empty() && other.values.empty() && done && other.done;
      }

      bool operator!=(const scalable_rmat_iterator& other) const
      { return !(*this == other); }

  private:

      // Parameters
      shared_ptr<uniform_01<RandomGenerator> > gen;

      // Internal data structures
      std::vector<value_type> values;
      value_type              current;
      bool                    done;
  };

} // end namespace boost

#endif // BOOST_GRAPH_DISTRIBUTED_RMAT_GENERATOR_HPP
