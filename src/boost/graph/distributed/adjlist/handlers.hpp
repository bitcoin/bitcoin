// Copyright (C) 2007 Douglas Gregor 

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file contains code for the distributed adjacency list's
// message handlers. It should not be included directly by users.

#ifndef BOOST_GRAPH_DISTRIBUTED_ADJLIST_HANDLERS_HPP
#define BOOST_GRAPH_DISTRIBUTED_ADJLIST_HANDLERS_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/graph/parallel/simple_trigger.hpp>
#include <boost/graph/parallel/detail/untracked_pair.hpp>

namespace boost { 

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
void
PBGL_DISTRIB_ADJLIST_TYPE::
setup_triggers()
{
  using boost::graph::parallel::simple_trigger;

  simple_trigger(process_group_, msg_add_vertex_with_property, this,
                 &adjacency_list::handle_add_vertex_with_property);
  simple_trigger(process_group_, msg_add_vertex_with_property_and_reply, this,
                 &adjacency_list::handle_add_vertex_with_property_and_reply);
  simple_trigger(process_group_, msg_add_edge, this, 
                 &adjacency_list::handle_add_edge);
  simple_trigger(process_group_, msg_add_edge_with_reply, this, 
                 &adjacency_list::handle_add_edge_with_reply);
  simple_trigger(process_group_, msg_add_edge_with_property, this,
                 &adjacency_list::handle_add_edge_with_property);
  simple_trigger(process_group_,  msg_add_edge_with_property_and_reply, this,
                 &adjacency_list::handle_add_edge_with_property_and_reply);
  simple_trigger(process_group_, msg_nonlocal_edge, this,
                 &adjacency_list::handle_nonlocal_edge);
  simple_trigger(process_group_, msg_remove_edge, this,
                 &adjacency_list::handle_remove_edge);
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
void 
PBGL_DISTRIB_ADJLIST_TYPE::
handle_add_vertex_with_property(int source, int tag, 
                                const vertex_property_type& data, 
                                trigger_receive_context)
{
  vertex_descriptor v(this->processor(), 
                      add_vertex(this->build_vertex_property(data), 
                                 this->base()));
  if (on_add_vertex)
    on_add_vertex(v, *this);
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
typename PBGL_DISTRIB_ADJLIST_TYPE::local_vertex_descriptor
PBGL_DISTRIB_ADJLIST_TYPE::
handle_add_vertex_with_property_and_reply(int source, int tag, 
                                          const vertex_property_type& data, 
                                          trigger_receive_context)
{
  // Try to find a vertex with this name
  local_vertex_descriptor local_v
    = add_vertex(this->build_vertex_property(data), this->base());

  vertex_descriptor v(processor(), local_v);
  if (on_add_vertex)
    on_add_vertex(v, *this);

  return local_v;
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
void 
PBGL_DISTRIB_ADJLIST_TYPE::
handle_add_edge(int source, int tag, const msg_add_edge_data& data,
                trigger_receive_context)
{
  add_edge(vertex_descriptor(processor(), data.source), 
           data.target, *this);
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
boost::parallel::detail::untracked_pair<typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor, bool>
PBGL_DISTRIB_ADJLIST_TYPE::
handle_add_edge_with_reply(int source, int tag, const msg_add_edge_data& data,
                           trigger_receive_context)
{
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor, bool> p = 
    add_edge(vertex_descriptor(processor(), data.source),data.target, *this);
  return p;
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
void 
PBGL_DISTRIB_ADJLIST_TYPE::
handle_add_edge_with_property(int source, int tag, 
                              const msg_add_edge_with_property_data& data,
                              trigger_receive_context)
{
  add_edge(vertex_descriptor(processor(), data.source), 
           data.target, data.get_property(), *this);
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
boost::parallel::detail::untracked_pair<typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor, bool>
PBGL_DISTRIB_ADJLIST_TYPE::
handle_add_edge_with_property_and_reply
  (int source, int tag, 
   const msg_add_edge_with_property_data& data,
   trigger_receive_context)
{
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor, bool> p = 
    add_edge(vertex_descriptor(processor(), data.source), 
                  data.target, data.get_property(), *this);
  return p;
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
void 
PBGL_DISTRIB_ADJLIST_TYPE::
handle_nonlocal_edge(int source, int tag, 
                     const msg_nonlocal_edge_data& data,
                     trigger_receive_context)
{
  add_remote_edge(data, source, directed_selector());
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
void 
PBGL_DISTRIB_ADJLIST_TYPE::
handle_remove_edge(int source, int tag, 
                   const msg_remove_edge_data& data,
                   trigger_receive_context)
{
  remove_local_edge(data, source, directed_selector());
}

} 

#endif // BOOST_GRAPH_DISTRIBUTED_ADJLIST_HANDLERS_HPP

