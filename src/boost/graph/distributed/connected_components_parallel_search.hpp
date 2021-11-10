// Copyright (C) 2004-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Brian Barrett
//           Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_PARALLEL_CC_PS_HPP
#define BOOST_GRAPH_PARALLEL_CC_PS_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/assert.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/parallel/algorithm.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/overloading.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/graph/parallel/properties.hpp>
#include <boost/graph/parallel/process_group.hpp>
#include <boost/optional.hpp>
#include <algorithm>
#include <vector>
#include <queue>
#include <limits>
#include <map>
#include <boost/graph/parallel/container_traits.hpp>
#include <boost/graph/iteration_macros.hpp>


// Connected components algorithm based on a parallel search.
//
// Every N nodes starts a parallel search from the first vertex in
// their local vertex list during the first superstep (the other nodes
// remain idle during the first superstep to reduce the number of
// conflicts in numbering the components).  At each superstep, all new
// component mappings from remote nodes are handled.  If there is no
// work from remote updates, a new vertex is removed from the local
// list and added to the work queue.
//
// Components are allocated from the component_value_allocator object,
// which ensures that a given component number is unique in the
// system, currently by using the rank and number of processes to
// stride allocations.
//
// When two components are discovered to actually be the same
// component, a mapping is created in the collisions object.  The
// lower component number is prefered in the resolution, so component
// numbering resolution is consistent.  After the search has exhausted
// all vertices in the graph, the mapping is shared with all
// processes, and they independently resolve the comonent mapping (so
// O((N * NP) + (V * NP)) work, in O(N + V) time, where N is the
// number of mappings and V is the number of local vertices).  This
// phase can likely be significantly sped up if a clever algorithm for
// the reduction can be found.
namespace boost { namespace graph { namespace distributed {
  namespace cc_ps_detail {
    // Local object for allocating component numbers.  There are two
    // places this happens in the code, and I was getting sick of them
    // getting out of sync.  Components are not tightly packed in
    // numbering, but are numbered to ensure each rank has its own
    // independent sets of numberings.
    template<typename component_value_type>
    class component_value_allocator {
    public:
      component_value_allocator(int num, int size) :
        last(0), num(num), size(size)
      {
      }

      component_value_type allocate(void)
      {
        component_value_type ret = num + (last * size);
        last++;
        return ret;
      }

    private:
      component_value_type last;
      int num;
      int size;
    };


    // Map of the "collisions" between component names in the global
    // component mapping.  TO make cleanup easier, component numbers
    // are added, pointing to themselves, when a new component is
    // found.  In order to make the results deterministic, the lower
    // component number is always taken.  The resolver will drill
    // through the map until it finds a component entry that points to
    // itself as the next value, allowing some cleanup to happen at
    // update() time.  Attempts are also made to update the mapping
    // when new entries are created.
    //
    // Note that there's an assumption that the entire mapping is
    // shared during the end of the algorithm, but before component
    // name resolution.
    template<typename component_value_type>
    class collision_map {
    public:
      collision_map() : num_unique(0)
      {
      }

      // add new component mapping first time component is used.  Own
      // function only so that we can sanity check there isn't already
      // a mapping for that component number (which would be bad)
      void add(const component_value_type &a) 
      {
        BOOST_ASSERT(collisions.count(a) == 0);
        collisions[a] = a;
      }

      // add a mapping between component values saying they're the
      // same component
      void add(const component_value_type &a, const component_value_type &b)
      {
        component_value_type high, low, tmp;
        if (a > b) {
          high = a;
          low = b;
        } else {
          high = b;
          low = a;
        }

        if (collisions.count(high) != 0 && collisions[high] != low) {
          tmp = collisions[high];
          if (tmp > low) {
            collisions[tmp] = low;
            collisions[high] = low;
          } else {
            collisions[low] = tmp;
            collisions[high] = tmp;
          }
        } else {
          collisions[high] = low;
        }

      }

      // get the "real" component number for the given component.
      // Used to resolve mapping at end of run.
      component_value_type update(component_value_type a)
      {
        BOOST_ASSERT(num_unique > 0);
        BOOST_ASSERT(collisions.count(a) != 0);
        return collisions[a];
      }

      // collapse the collisions tree, so that update is a one lookup
      // operation.  Count unique components at the same time.
      void uniqify(void)
      {
        typename std::map<component_value_type, component_value_type>::iterator i, end;

        end = collisions.end();
        for (i = collisions.begin() ; i != end ; ++i) {
          if (i->first == i->second) {
            num_unique++;
          } else {
            i->second = collisions[i->second];
          }
        }
      }

      // get the number of component entries that have an associated
      // component number of themselves, which are the real components
      // used in the final mapping.  This is the number of unique
      // components in the graph.
      int unique(void)
      {
        BOOST_ASSERT(num_unique > 0);
        return num_unique;
      }

      // "serialize" into a vector for communication.
      std::vector<component_value_type> serialize(void)
      {
        std::vector<component_value_type> ret;
        typename std::map<component_value_type, component_value_type>::iterator i, end;

        end = collisions.end();
        for (i = collisions.begin() ; i != end ; ++i) {
          ret.push_back(i->first);
          ret.push_back(i->second);
        }

        return ret;
      }

    private:
      std::map<component_value_type, component_value_type> collisions;
      int num_unique;
    };


    // resolver to handle remote updates.  The resolver will add
    // entries into the collisions map if required, and if it is the
    // first time the vertex has been touched, it will add the vertex
    // to the remote queue.  Note that local updates are handled
    // differently, in the main loop (below).

      // BWB - FIX ME - don't need graph anymore - can pull from key value of Component Map.
    template<typename ComponentMap, typename work_queue>
    struct update_reducer {
      BOOST_STATIC_CONSTANT(bool, non_default_resolver = false);

      typedef typename property_traits<ComponentMap>::value_type component_value_type;
      typedef typename property_traits<ComponentMap>::key_type vertex_descriptor;

      update_reducer(work_queue *q,
                     cc_ps_detail::collision_map<component_value_type> *collisions, 
                     processor_id_type pg_id) :
        q(q), collisions(collisions), pg_id(pg_id)
      {
      }

      // ghost cell initialization routine.  This should never be
      // called in this imlementation.
      template<typename K>
      component_value_type operator()(const K&) const
      { 
        return component_value_type(0); 
      }

      // resolver for remote updates.  I'm not entirely sure why, but
      // I decided to not change the value of the vertex if it's
      // already non-infinite.  It doesn't matter in the end, as we'll
      // touch every vertex in the cleanup phase anyway.  If the
      // component is currently infinite, set to the new component
      // number and add the vertex to the work queue.  If it's not
      // infinite, we've touched it already so don't add it to the
      // work queue.  Do add a collision entry so that we know the two
      // components are the same.
      component_value_type operator()(const vertex_descriptor &v,
                                      const component_value_type& current,
                                      const component_value_type& update) const
      {
        const component_value_type max = (std::numeric_limits<component_value_type>::max)();
        component_value_type ret = current;

        if (max == current) {
          q->push(v);
          ret = update;
        } else if (current != update) {
          collisions->add(current, update);
        }

        return ret;
      }                                    

      // So for whatever reason, the property map can in theory call
      // the resolver with a local descriptor in addition to the
      // standard global descriptor.  As far as I can tell, this code
      // path is never taken in this implementation, but I need to
      // have this code here to make it compile.  We just make a
      // global descriptor and call the "real" operator().
      template<typename K>
      component_value_type operator()(const K& v, 
                                      const component_value_type& current, 
                                      const component_value_type& update) const
      {
          return (*this)(vertex_descriptor(pg_id, v), current, update);
      }

    private:
      work_queue *q;
      collision_map<component_value_type> *collisions;
      boost::processor_id_type pg_id;
    };

  } // namespace cc_ps_detail


  template<typename Graph, typename ComponentMap>
  typename property_traits<ComponentMap>::value_type
  connected_components_ps(const Graph& g, ComponentMap c)
  {
    using boost::graph::parallel::process_group;

    typedef typename property_traits<ComponentMap>::value_type component_value_type;
    typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename boost::graph::parallel::process_group_type<Graph>
      ::type process_group_type;
    typedef typename process_group_type::process_id_type process_id_type;
    typedef std::queue<vertex_descriptor> work_queue;

    static const component_value_type max_component = 
      (std::numeric_limits<component_value_type>::max)();
    typename property_map<Graph, vertex_owner_t>::const_type
      owner = get(vertex_owner, g);

    // standard who am i? stuff
    process_group_type pg = process_group(g);
    process_id_type id = process_id(pg);

    // Initialize every vertex to have infinite component number
    BGL_FORALL_VERTICES_T(v, g, Graph) put(c, v, max_component);

    vertex_iterator current, end;
    boost::tie(current, end) = vertices(g);

    cc_ps_detail::component_value_allocator<component_value_type> cva(process_id(pg), num_processes(pg));
    cc_ps_detail::collision_map<component_value_type> collisions;
    work_queue q;  // this is intentionally a local data structure
    c.set_reduce(cc_ps_detail::update_reducer<ComponentMap, work_queue>(&q, &collisions, id));

    // add starting work
    while (true) {
        bool useful_found = false;
        component_value_type val = cva.allocate();
        put(c, *current, val);
        collisions.add(val);
        q.push(*current);
        if (0 != out_degree(*current, g)) useful_found = true;
        ++current;
        if (useful_found) break;
    }

    // Run the loop until everyone in the system is done
    bool global_done = false;
    while (!global_done) {

      // drain queue of work for this superstep
      while (!q.empty()) {
        vertex_descriptor v = q.front();
        q.pop();
        // iterate through outedges of the vertex currently being
        // examined, setting their component to our component.  There
        // is no way to end up in the queue without having a component
        // number already.

        BGL_FORALL_ADJ_T(v, peer, g, Graph) {
          component_value_type my_component = get(c, v);

          // update other vertex with our component information.
          // Resolver will handle remote collisions as well as whether
          // to put the vertex on the work queue or not.  We have to
          // handle local collisions and work queue management
          if (id == get(owner, peer)) {
            if (max_component == get(c, peer)) {
              put(c, peer, my_component);
              q.push(peer);
            } else if (my_component != get(c, peer)) {
              collisions.add(my_component, get(c, peer));
            }
          } else {
            put(c, peer, my_component);
          }
        }
      }

      // synchronize / start a new superstep.
      synchronize(pg);
      global_done = all_reduce(pg, (q.empty() && (current == end)), boost::parallel::minimum<bool>());

      // If the queue is currently empty, add something to do to start
      // the current superstep (supersteps start at the sync, not at
      // the top of the while loop as one might expect).  Down at the
      // bottom of the while loop so that not everyone starts the
      // algorithm with something to do, to try to reduce component
      // name conflicts
      if (q.empty()) {
        bool useful_found = false;
        for ( ; current != end && !useful_found ; ++current) {
          if (max_component == get(c, *current)) {
            component_value_type val = cva.allocate();
            put(c, *current, val);
            collisions.add(val);
            q.push(*current);
            if (0 != out_degree(*current, g)) useful_found = true;
          }
        }
      }
    }

    // share component mappings
    std::vector<component_value_type> global;
    std::vector<component_value_type> mine = collisions.serialize();
    all_gather(pg, mine.begin(), mine.end(), global);
    for (size_t i = 0 ; i < global.size() ; i += 2) {
      collisions.add(global[i], global[i + 1]);
    }
    collisions.uniqify();

    // update the component mappings
    BGL_FORALL_VERTICES_T(v, g, Graph) {
      put(c, v, collisions.update(get(c, v)));
    }

    return collisions.unique();
  }

} // end namespace distributed

} // end namespace graph

} // end namespace boost

#endif // BOOST_GRAPH_PARALLEL_CC_HPP
