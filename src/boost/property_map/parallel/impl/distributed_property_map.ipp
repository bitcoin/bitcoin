// Copyright (C) 2004-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Nick Edmonds
//           Andrew Lumsdaine
#include <boost/assert.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/property_map/parallel/detail/untracked_pair.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/bind.hpp>
#include <boost/property_map/parallel/simple_trigger.hpp>

namespace boost { namespace parallel {

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
template<typename Reduce>
PBGL_DISTRIB_PMAP
::distributed_property_map(const ProcessGroup& pg, const GlobalMap& global,
                           const StorageMap& pm, const Reduce& reduce)
  : data(new data_t(pg, global, pm, reduce, Reduce::non_default_resolver))
{
  typedef handle_message<Reduce> Handler;

  data->ghost_cells.reset(new ghost_cells_type());
  data->reset = &data_t::template do_reset<Reduce>;
  data->process_group.replace_handler(Handler(data, reduce));
  data->process_group.template get_receiver<Handler>()
    ->setup_triggers(data->process_group);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
PBGL_DISTRIB_PMAP::~distributed_property_map() { }

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
template<typename Reduce>
void 
PBGL_DISTRIB_PMAP::set_reduce(const Reduce& reduce)
{
  typedef handle_message<Reduce> Handler;
  data->process_group.replace_handler(Handler(data, reduce));
  Handler* handler = data->process_group.template get_receiver<Handler>();
  BOOST_ASSERT(handler);
  handler->setup_triggers(data->process_group);
  data->get_default_value = reduce;
  data->has_default_resolver = Reduce::non_default_resolver;
  int model = data->model;
  data->reset = &data_t::template do_reset<Reduce>;
  set_consistency_model(model);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void PBGL_DISTRIB_PMAP::prune_ghost_cells() const
{
  if (data->max_ghost_cells == 0)
    return;

  while (data->ghost_cells->size() > data->max_ghost_cells) {
    // Evict the last ghost cell

    if (data->model & cm_flush) {
      // We need to flush values when we evict them.
      boost::parallel::detail::untracked_pair<key_type, value_type> const& victim
        = data->ghost_cells->back();
      send(data->process_group, get(data->global, victim.first).first, 
           property_map_put, victim);
    }

    // Actually remove the ghost cell
    data->ghost_cells->pop_back();
  }
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
typename PBGL_DISTRIB_PMAP::value_type&
PBGL_DISTRIB_PMAP::cell(const key_type& key, bool request_if_missing) const
{
  // Index by key
  ghost_cells_key_index_type const& key_index 
    = data->ghost_cells->template get<1>();

  // Search for the ghost cell by key, and project back to the sequence
  iterator ghost_cell 
    = data->ghost_cells->template project<0>(key_index.find(key));
  if (ghost_cell == data->ghost_cells->end()) {
    value_type value;
    if (data->has_default_resolver)
      // Since we have a default resolver, use it to create a default
      // value for this ghost cell.
      value = data->get_default_value(key);
    else if (request_if_missing)
      // Request the actual value of this key from its owner
      send_oob_with_reply(data->process_group, get(data->global, key).first, 
                          property_map_get, key, value);
    else
      value = value_type();

    // Create a ghost cell containing the new value
    ghost_cell 
      = data->ghost_cells->push_front(std::make_pair(key, value)).first;

    // If we need to, prune the ghost cells
    if (data->max_ghost_cells > 0)
      prune_ghost_cells();
  } else if (data->max_ghost_cells > 0)
    // Put this cell at the beginning of the MRU list
    data->ghost_cells->relocate(data->ghost_cells->begin(), ghost_cell);

  return const_cast<value_type&>(ghost_cell->second);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
template<typename Reduce>
void
PBGL_DISTRIB_PMAP
::handle_message<Reduce>::operator()(process_id_type source, int tag)
{
  BOOST_ASSERT(false);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
template<typename Reduce>
void
PBGL_DISTRIB_PMAP::handle_message<Reduce>::
handle_put(int /*source*/, int /*tag*/, 
           const boost::parallel::detail::untracked_pair<key_type, value_type>& req, trigger_receive_context)
{
  using boost::get;

  shared_ptr<data_t> data(data_ptr);

  owner_local_pair p = get(data->global, req.first);
  BOOST_ASSERT(p.first == process_id(data->process_group));

  detail::maybe_put(data->storage, p.second,
                    reduce(req.first,
                           get(data->storage, p.second),
                           req.second));
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
template<typename Reduce>
typename PBGL_DISTRIB_PMAP::value_type
PBGL_DISTRIB_PMAP::handle_message<Reduce>::
handle_get(int source, int /*tag*/, const key_type& key, 
           trigger_receive_context)
{
  using boost::get;

  shared_ptr<data_t> data(data_ptr);
  BOOST_ASSERT(data);

  owner_local_pair p = get(data->global, key);
  return get(data->storage, p.second);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
template<typename Reduce>
void
PBGL_DISTRIB_PMAP::handle_message<Reduce>::
handle_multiget(int source, int tag, const std::vector<key_type>& keys,
                trigger_receive_context)
{
  shared_ptr<data_t> data(data_ptr);
  BOOST_ASSERT(data);

  typedef boost::parallel::detail::untracked_pair<key_type, value_type> key_value;
  std::vector<key_value> results;
  std::size_t n = keys.size();
  results.reserve(n);

  using boost::get;

  for (std::size_t i = 0; i < n; ++i) {
    local_key_type local_key = get(data->global, keys[i]).second;
    results.push_back(key_value(keys[i], get(data->storage, local_key)));
  }
  send(data->process_group, source, property_map_multiget_reply, results);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
template<typename Reduce>
void
PBGL_DISTRIB_PMAP::handle_message<Reduce>::
handle_multiget_reply
  (int source, int tag, 
   const std::vector<boost::parallel::detail::untracked_pair<key_type, value_type> >& msg,
   trigger_receive_context)
{
  shared_ptr<data_t> data(data_ptr);
  BOOST_ASSERT(data);

  // Index by key
  ghost_cells_key_index_type const& key_index 
    = data->ghost_cells->template get<1>();

  std::size_t n = msg.size();
  for (std::size_t i = 0; i < n; ++i) {
    // Search for the ghost cell by key, and project back to the sequence
    iterator position
      = data->ghost_cells->template project<0>(key_index.find(msg[i].first));

    if (position != data->ghost_cells->end())
      const_cast<value_type&>(position->second) = msg[i].second;
  }
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
template<typename Reduce>
void
PBGL_DISTRIB_PMAP::handle_message<Reduce>::
handle_multiput
  (int source, int tag, 
   const std::vector<unsafe_pair<local_key_type, value_type> >& values,
   trigger_receive_context)
{
  using boost::get;

  shared_ptr<data_t> data(data_ptr);
  BOOST_ASSERT(data);

  std::size_t n = values.size();
  for (std::size_t i = 0; i < n; ++i) {
    local_key_type local_key = values[i].first;
    value_type local_value = get(data->storage, local_key);
    detail::maybe_put(data->storage, values[i].first,
                      reduce(values[i].first,
                             local_value,
                             values[i].second));
  }
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
template<typename Reduce>
void
PBGL_DISTRIB_PMAP::handle_message<Reduce>::
setup_triggers(process_group_type& pg)
{
  using boost::parallel::simple_trigger;

  simple_trigger(pg, property_map_put, this, &handle_message::handle_put);
  simple_trigger(pg, property_map_get, this, &handle_message::handle_get);
  simple_trigger(pg, property_map_multiget, this, 
                 &handle_message::handle_multiget);
  simple_trigger(pg, property_map_multiget_reply, this, 
                 &handle_message::handle_multiget_reply);
  simple_trigger(pg, property_map_multiput, this, 
                 &handle_message::handle_multiput);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void
PBGL_DISTRIB_PMAP
::on_synchronize::operator()()
{
  int stage=0; // we only get called at the start now
  shared_ptr<data_t> data(data_ptr);
  BOOST_ASSERT(data);

  // Determine in which stage backward consistency messages should be sent.
  int backward_stage = -1;
  if (data->model & cm_backward) {
    if (data->model & cm_flush) backward_stage = 1;
    else backward_stage = 0;
  }

  // Flush results in first stage
  if (stage == 0 && data->model & cm_flush)
    data->flush();

  // Backward consistency
  if (stage == backward_stage && !(data->model & (cm_clear | cm_reset)))
    data->refresh_ghost_cells();

  // Optionally clear results
  if (data->model & cm_clear)
    data->clear();

  // Optionally reset results
  if (data->model & cm_reset) {
    if (data->reset) ((*data).*data->reset)();
  }
}


template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void
PBGL_DISTRIB_PMAP::set_consistency_model(int model)
{
  data->model = model;

  bool need_on_synchronize = (model != cm_forward);

  // Backward consistency is a two-stage process.
  if (model & cm_backward) {
    // For backward consistency to work, we absolutely cannot throw
    // away any ghost cells.
    data->max_ghost_cells = 0;
  }

  // attach the on_synchronize handler.
  if (need_on_synchronize)
    data->process_group.replace_on_synchronize_handler(on_synchronize(data));
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void
PBGL_DISTRIB_PMAP::set_max_ghost_cells(std::size_t max_ghost_cells)
{
  if ((data->model & cm_backward) && max_ghost_cells > 0)
      boost::throw_exception(std::runtime_error("distributed_property_map::set_max_ghost_cells: "
                                                "cannot limit ghost-cell usage with a backward "
                                                "consistency model"));

  if (max_ghost_cells == 1)
    // It is not safe to have only 1 ghost cell; the cell() method
    // will fail.
    max_ghost_cells = 2;

  data->max_ghost_cells = max_ghost_cells;
  prune_ghost_cells();
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void PBGL_DISTRIB_PMAP::clear()
{
  data->clear();
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void PBGL_DISTRIB_PMAP::data_t::clear()
{
  ghost_cells->clear();
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void PBGL_DISTRIB_PMAP::reset()
{
  if (data->reset) ((*data).*data->reset)();
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void PBGL_DISTRIB_PMAP::flush()
{
  data->flush();
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void PBGL_DISTRIB_PMAP::data_t::refresh_ghost_cells()
{
  using boost::get;

  std::vector<std::vector<key_type> > keys;
  keys.resize(num_processes(process_group));

  // Collect the set of keys for which we will request values
  for (iterator i = ghost_cells->begin(); i != ghost_cells->end(); ++i)
    keys[get(global, i->first).first].push_back(i->first);

  // Send multiget requests to each of the other processors
  typedef typename ProcessGroup::process_size_type process_size_type;
  process_size_type n = num_processes(process_group);
  process_id_type id = process_id(process_group);
  for (process_size_type p = (id + 1) % n ; p != id ; p = (p + 1) % n) {
    if (!keys[p].empty())
      send(process_group, p, property_map_multiget, keys[p]);
  }  
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void PBGL_DISTRIB_PMAP::data_t::flush()
{
  using boost::get;

  int n = num_processes(process_group);
  std::vector<std::vector<unsafe_pair<local_key_type, value_type> > > values;
  values.resize(n);

  // Collect all of the flushed values
  for (iterator i = ghost_cells->begin(); i != ghost_cells->end(); ++i) {
    std::pair<int, local_key_type> g = get(global, i->first);
    values[g.first].push_back(std::make_pair(g.second, i->second));
  }

  // Transmit flushed values
  for (int p = 0; p < n; ++p) {
    if (!values[p].empty())
      send(process_group, p, property_map_multiput, values[p]);
  }
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
void PBGL_DISTRIB_PMAP::do_synchronize()
{
  if (data->model & cm_backward) {
    synchronize(data->process_group);
    return;
  }

  // Request refreshes of the values of our ghost cells
  data->refresh_ghost_cells();

  // Allows all of the multigets to get to their destinations
  synchronize(data->process_group);

  // Allows all of the multiget responses to get to their destinations
  synchronize(data->process_group);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
template<typename Resolver>
void PBGL_DISTRIB_PMAP::data_t::do_reset()
{
  Resolver* resolver = get_default_value.template target<Resolver>();
  BOOST_ASSERT(resolver);

  for (iterator i = ghost_cells->begin(); i != ghost_cells->end(); ++i)
    const_cast<value_type&>(i->second) = (*resolver)(i->first);
}

} } // end namespace boost::parallel
