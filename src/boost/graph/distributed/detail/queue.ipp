// Copyright (C) 2004-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#include <boost/optional.hpp>
#include <cassert>
#include <boost/graph/parallel/algorithm.hpp>
#include <boost/graph/parallel/process_group.hpp>
#include <functional>
#include <algorithm>
#include <boost/graph/parallel/simple_trigger.hpp>

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

namespace boost { namespace graph { namespace distributed {

template<BOOST_DISTRIBUTED_QUEUE_PARMS>
BOOST_DISTRIBUTED_QUEUE_TYPE::
distributed_queue(const ProcessGroup& process_group, const OwnerMap& owner,
                  const Buffer& buffer, bool polling)
  : process_group(process_group, attach_distributed_object()),
    owner(owner),
    buffer(buffer),
    polling(polling)
{
  if (!polling)
    outgoing_buffers.reset(
      new outgoing_buffers_t(num_processes(process_group)));

  setup_triggers();
}

template<BOOST_DISTRIBUTED_QUEUE_PARMS>
BOOST_DISTRIBUTED_QUEUE_TYPE::
distributed_queue(const ProcessGroup& process_group, const OwnerMap& owner,
                  const Buffer& buffer, const UnaryPredicate& pred,
                  bool polling)
  : process_group(process_group, attach_distributed_object()),
    owner(owner),
    buffer(buffer),
    pred(pred),
    polling(polling)
{
  if (!polling)
    outgoing_buffers.reset(
      new outgoing_buffers_t(num_processes(process_group)));

  setup_triggers();
}

template<BOOST_DISTRIBUTED_QUEUE_PARMS>
BOOST_DISTRIBUTED_QUEUE_TYPE::
distributed_queue(const ProcessGroup& process_group, const OwnerMap& owner,
                  const UnaryPredicate& pred, bool polling)
  : process_group(process_group, attach_distributed_object()),
    owner(owner),
    pred(pred),
    polling(polling)
{
  if (!polling)
    outgoing_buffers.reset(
      new outgoing_buffers_t(num_processes(process_group)));

  setup_triggers();
}

template<BOOST_DISTRIBUTED_QUEUE_PARMS>
void
BOOST_DISTRIBUTED_QUEUE_TYPE::push(const value_type& x)
{
  typename ProcessGroup::process_id_type dest = get(owner, x);
  if (outgoing_buffers)
    outgoing_buffers->at(dest).push_back(x);
  else if (dest == process_id(process_group))
    buffer.push(x);
  else
    send(process_group, get(owner, x), msg_push, x);
}

template<BOOST_DISTRIBUTED_QUEUE_PARMS>
bool
BOOST_DISTRIBUTED_QUEUE_TYPE::empty() const
{
  /* Processes will stay here until the buffer is nonempty or
     synchronization with the other processes indicates that all local
     buffers are empty (and no messages are in transit).
   */
  while (buffer.empty() && !do_synchronize()) ;

  return buffer.empty();
}

template<BOOST_DISTRIBUTED_QUEUE_PARMS>
typename BOOST_DISTRIBUTED_QUEUE_TYPE::size_type
BOOST_DISTRIBUTED_QUEUE_TYPE::size() const
{
  empty();
  return buffer.size();
}

template<BOOST_DISTRIBUTED_QUEUE_PARMS>
void BOOST_DISTRIBUTED_QUEUE_TYPE::setup_triggers()
{
  using boost::graph::parallel::simple_trigger;

  simple_trigger(process_group, msg_push, this, 
                 &distributed_queue::handle_push);
  simple_trigger(process_group, msg_multipush, this, 
                 &distributed_queue::handle_multipush);
}

template<BOOST_DISTRIBUTED_QUEUE_PARMS>
void 
BOOST_DISTRIBUTED_QUEUE_TYPE::
handle_push(int /*source*/, int /*tag*/, const value_type& value, 
            trigger_receive_context)
{
  if (pred(value)) buffer.push(value);
}

template<BOOST_DISTRIBUTED_QUEUE_PARMS>
void 
BOOST_DISTRIBUTED_QUEUE_TYPE::
handle_multipush(int /*source*/, int /*tag*/, 
                 const std::vector<value_type>& values, 
                 trigger_receive_context)
{
  for (std::size_t i = 0; i < values.size(); ++i)
    if (pred(values[i])) buffer.push(values[i]);
}

template<BOOST_DISTRIBUTED_QUEUE_PARMS>
bool
BOOST_DISTRIBUTED_QUEUE_TYPE::do_synchronize() const
{
#ifdef PBGL_ACCOUNTING
  ++num_synchronizations;
#endif

  using boost::parallel::all_reduce;
  using std::swap;

  typedef typename ProcessGroup::process_id_type process_id_type;

  if (outgoing_buffers) {
    // Transfer all of the push requests
    process_id_type id = process_id(process_group);
    process_id_type np = num_processes(process_group);
    for (process_id_type dest = 0; dest < np; ++dest) {
      outgoing_buffer_t& outgoing = outgoing_buffers->at(dest);
      std::size_t size = outgoing.size();
      if (size != 0) {
        if (dest != id) {
          send(process_group, dest, msg_multipush, outgoing);
        } else {
          for (std::size_t i = 0; i < size; ++i)
            buffer.push(outgoing[i]);
        }
        outgoing.clear();
      }
    }
  }
  synchronize(process_group);

  unsigned local_size = buffer.size();
  unsigned global_size =
    all_reduce(process_group, local_size, std::plus<unsigned>());
  return global_size == 0;
}

} } } // end namespace boost::graph::distributed
