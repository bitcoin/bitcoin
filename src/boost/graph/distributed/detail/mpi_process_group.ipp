// -*- C++ -*-

// Copyright (C) 2004-2008 The Trustees of Indiana University.
// Copyright (C) 2007  Douglas Gregor <doug.gregor@gmail.com>
// Copyright (C) 2007  Matthias Troyer  <troyer@boost-consulting.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
//           Matthias Troyer

//#define PBGL_PROCESS_GROUP_DEBUG

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/assert.hpp>
#include <algorithm>
#include <boost/graph/parallel/detail/untracked_pair.hpp>
#include <numeric>
#include <iterator>
#include <functional>
#include <vector>
#include <queue>
#include <stack>
#include <list>
#include <boost/graph/distributed/detail/tag_allocator.hpp>
#include <stdio.h>

// #define PBGL_PROCESS_GROUP_DEBUG

#ifdef PBGL_PROCESS_GROUP_DEBUG
#  include <iostream>
#endif

namespace boost { namespace graph { namespace distributed {

struct mpi_process_group::impl
{
  
  typedef mpi_process_group::message_header message_header;
  typedef mpi_process_group::outgoing_messages outgoing_messages;

  /**
   * Stores the incoming messages from a particular processor.
   *
   * @todo Evaluate whether we should use a deque instance, which
   * would reduce could reduce the cost of "receiving" messages and
     allow us to deallocate memory earlier, but increases the time
     spent in the synchronization step.
   */
  struct incoming_messages {
    incoming_messages();
    ~incoming_messages() {}

    std::vector<message_header> headers;
    buffer_type                 buffer;
    std::vector<std::vector<message_header>::iterator> next_header;
  };

  struct batch_request {
    MPI_Request request;
    buffer_type buffer;
  };

  // send once we have a certain number of messages or bytes in the buffer
  // these numbers need to be tuned, we keep them small at first for testing
  std::size_t batch_header_number;
  std::size_t batch_buffer_size;
  std::size_t batch_message_size;
  
  /**
   * The actual MPI communicator used to transmit data.
   */
  boost::mpi::communicator             comm;

  /**
   * The MPI communicator used to transmit out-of-band replies.
   */
  boost::mpi::communicator             oob_reply_comm;

  /// Outgoing message information, indexed by destination processor.
  std::vector<outgoing_messages> outgoing;

  /// Incoming message information, indexed by source processor.
  std::vector<incoming_messages> incoming;

  /// The numbers of processors that have entered a synchronization stage
  std::vector<int> processors_synchronizing_stage;
  
  /// The synchronization stage of a processor
  std::vector<int> synchronizing_stage;

  /// Number of processors still sending messages
  std::vector<int> synchronizing_unfinished;
  
  /// Number of batches sent since last synchronization stage
  std::vector<int> number_sent_batches;
  
  /// Number of batches received minus number of expected batches
  std::vector<int> number_received_batches;
  

  /// The context of the currently-executing trigger, or @c trc_none
  /// if no trigger is executing.
  trigger_receive_context trigger_context;

  /// Non-zero indicates that we're processing batches
  /// Increment this when processing patches,
  /// decrement it when you're done.
  int processing_batches;

  /**
   * Contains all of the active blocks corresponding to attached
   * distributed data structures.
   */
  blocks_type blocks;

  /// Whether we are currently synchronizing
  bool synchronizing;

  /// The MPI requests for posted sends of oob messages
  std::vector<MPI_Request> requests;
  
  /// The MPI buffers for posted irecvs of oob messages
  std::map<int,buffer_type> buffers;

  /// Queue for message batches received while already processing messages
  std::queue<std::pair<int,outgoing_messages> > new_batches;
  /// Maximum encountered size of the new_batches queue
  std::size_t max_received;

  /// The MPI requests and buffers for batchess being sent
  std::list<batch_request> sent_batches;
  /// Maximum encountered size of the sent_batches list
  std::size_t max_sent;

  /// Pre-allocated requests in a pool
  std::vector<batch_request> batch_pool;
  /// A stack controlling which batches are available
  std::stack<std::size_t> free_batches;

  void free_sent_batches();
  
  // Tag allocator
  detail::tag_allocator allocated_tags;

  impl(std::size_t num_headers, std::size_t buffers_size,
       communicator_type parent_comm);
  ~impl();
  
private:
  void set_batch_size(std::size_t header_num, std::size_t buffer_sz);
};

inline trigger_receive_context mpi_process_group::trigger_context() const
{
  return impl_->trigger_context;
}

template<typename T>
void
mpi_process_group::send_impl(int dest, int tag, const T& value,
                             mpl::true_ /*is_mpi_datatype*/) const
{
  BOOST_ASSERT(tag <  msg_reserved_first || tag > msg_reserved_last);

  impl::outgoing_messages& outgoing = impl_->outgoing[dest];

  // Start constructing the message header
  impl::message_header header;
  header.source = process_id(*this);
  header.tag = tag;
  header.offset = outgoing.buffer.size();
  
  boost::mpi::packed_oarchive oa(impl_->comm, outgoing.buffer);
  oa << value;

#ifdef PBGL_PROCESS_GROUP_DEBUG
  std::cerr << "SEND: " << process_id(*this) << " -> " << dest << ", tag = "
            << tag << ", bytes = " << packed_size << std::endl;
#endif

  // Store the header
  header.bytes = outgoing.buffer.size() - header.offset;
  outgoing.headers.push_back(header);

  maybe_send_batch(dest);
}


template<typename T>
void
mpi_process_group::send_impl(int dest, int tag, const T& value,
                             mpl::false_ /*is_mpi_datatype*/) const
{
  BOOST_ASSERT(tag <  msg_reserved_first || tag > msg_reserved_last);

  impl::outgoing_messages& outgoing = impl_->outgoing[dest];

  // Start constructing the message header
  impl::message_header header;
  header.source = process_id(*this);
  header.tag = tag;
  header.offset = outgoing.buffer.size();

  // Serialize into the buffer
  boost::mpi::packed_oarchive out(impl_->comm, outgoing.buffer);
  out << value;

  // Store the header
  header.bytes = outgoing.buffer.size() - header.offset;
  outgoing.headers.push_back(header);
  maybe_send_batch(dest);

#ifdef PBGL_PROCESS_GROUP_DEBUG
  std::cerr << "SEND: " << process_id(*this) << " -> " << dest << ", tag = "
            << tag << ", bytes = " << header.bytes << std::endl;
#endif
}

template<typename T>
inline void
send(const mpi_process_group& pg, mpi_process_group::process_id_type dest,
     int tag, const T& value)
{
  pg.send_impl(dest, pg.encode_tag(pg.my_block_number(), tag), value,
               boost::mpi::is_mpi_datatype<T>());
}

template<typename T>
typename enable_if<boost::mpi::is_mpi_datatype<T>, void>::type
send(const mpi_process_group& pg, mpi_process_group::process_id_type dest,
     int tag, const T values[], std::size_t n)
{
  pg.send_impl(dest, pg.encode_tag(pg.my_block_number(), tag),
                 boost::serialization::make_array(values,n), 
                 boost::mpl::true_());
}

template<typename T>
typename disable_if<boost::mpi::is_mpi_datatype<T>, void>::type
mpi_process_group::
array_send_impl(int dest, int tag, const T values[], std::size_t n) const
{
  BOOST_ASSERT(tag <  msg_reserved_first || tag > msg_reserved_last);

  impl::outgoing_messages& outgoing = impl_->outgoing[dest];

  // Start constructing the message header
  impl::message_header header;
  header.source = process_id(*this);
  header.tag = tag;
  header.offset = outgoing.buffer.size();

  // Serialize into the buffer
  boost::mpi::packed_oarchive out(impl_->comm, outgoing.buffer);
  out << n;

  for (std::size_t i = 0; i < n; ++i)
    out << values[i];

  // Store the header
  header.bytes = outgoing.buffer.size() - header.offset;
  outgoing.headers.push_back(header);
  maybe_send_batch(dest);

#ifdef PBGL_PROCESS_GROUP_DEBUG
  std::cerr << "SEND: " << process_id(*this) << " -> " << dest << ", tag = "
            << tag << ", bytes = " << header.bytes << std::endl;
#endif
}

template<typename T>
typename disable_if<boost::mpi::is_mpi_datatype<T>, void>::type
send(const mpi_process_group& pg, mpi_process_group::process_id_type dest,
     int tag, const T values[], std::size_t n)
{
  pg.array_send_impl(dest, pg.encode_tag(pg.my_block_number(), tag), 
                     values, n);
}

template<typename InputIterator>
void
send(const mpi_process_group& pg, mpi_process_group::process_id_type dest,
     int tag, InputIterator first, InputIterator last)
{
  typedef typename std::iterator_traits<InputIterator>::value_type value_type;
  std::vector<value_type> values(first, last);
  if (values.empty()) send(pg, dest, tag, static_cast<value_type*>(0), 0);
  else send(pg, dest, tag, &values[0], values.size());
}

template<typename T>
bool
mpi_process_group::receive_impl(int source, int tag, T& value,
                                mpl::true_ /*is_mpi_datatype*/) const
{
#ifdef PBGL_PROCESS_GROUP_DEBUG
  std::cerr << "RECV: " << process_id(*this) << " <- " << source << ", tag = "
            << tag << std::endl;
#endif

  impl::incoming_messages& incoming = impl_->incoming[source];

  // Find the next header with the right tag
  std::vector<impl::message_header>::iterator header =
    incoming.next_header[my_block_number()];
  while (header != incoming.headers.end() && header->tag != tag) ++header;

  // If no header is found, notify the caller
  if (header == incoming.headers.end()) return false;

  // Unpack the data
  if (header->bytes > 0) {
    boost::mpi::packed_iarchive ia(impl_->comm, incoming.buffer, 
                                   archive::no_header, header->offset);
    ia >> value;
  }

  // Mark this message as received
  header->tag = -1;

  // Move the "next header" indicator to the next unreceived message
  while (incoming.next_header[my_block_number()] != incoming.headers.end()
         && incoming.next_header[my_block_number()]->tag == -1)
    ++incoming.next_header[my_block_number()];

  if (incoming.next_header[my_block_number()] == incoming.headers.end()) {
    bool finished = true;
    for (std::size_t i = 0; i < incoming.next_header.size() && finished; ++i) {
      if (incoming.next_header[i] != incoming.headers.end()) finished = false;
    }

    if (finished) {
      std::vector<impl::message_header> no_headers;
      incoming.headers.swap(no_headers);
      buffer_type empty_buffer;
      incoming.buffer.swap(empty_buffer);
      for (std::size_t i = 0; i < incoming.next_header.size(); ++i)
        incoming.next_header[i] = incoming.headers.end();
    }
  }

  return true;
}

template<typename T>
bool
mpi_process_group::receive_impl(int source, int tag, T& value,
                                mpl::false_ /*is_mpi_datatype*/) const
{
  impl::incoming_messages& incoming = impl_->incoming[source];

  // Find the next header with the right tag
  std::vector<impl::message_header>::iterator header =
    incoming.next_header[my_block_number()];
  while (header != incoming.headers.end() && header->tag != tag) ++header;

  // If no header is found, notify the caller
  if (header == incoming.headers.end()) return false;

  // Deserialize the data
  boost::mpi::packed_iarchive in(impl_->comm, incoming.buffer, 
                                 archive::no_header, header->offset);
  in >> value;

  // Mark this message as received
  header->tag = -1;

  // Move the "next header" indicator to the next unreceived message
  while (incoming.next_header[my_block_number()] != incoming.headers.end()
         && incoming.next_header[my_block_number()]->tag == -1)
    ++incoming.next_header[my_block_number()];

  if (incoming.next_header[my_block_number()] == incoming.headers.end()) {
    bool finished = true;
    for (std::size_t i = 0; i < incoming.next_header.size() && finished; ++i) {
      if (incoming.next_header[i] != incoming.headers.end()) finished = false;
    }

    if (finished) {
      std::vector<impl::message_header> no_headers;
      incoming.headers.swap(no_headers);
      buffer_type empty_buffer;
      incoming.buffer.swap(empty_buffer);
      for (std::size_t i = 0; i < incoming.next_header.size(); ++i)
        incoming.next_header[i] = incoming.headers.end();
    }
  }

  return true;
}

template<typename T>
typename disable_if<boost::mpi::is_mpi_datatype<T>, bool>::type
mpi_process_group::
array_receive_impl(int source, int tag, T* values, std::size_t& n) const
{
  impl::incoming_messages& incoming = impl_->incoming[source];

  // Find the next header with the right tag
  std::vector<impl::message_header>::iterator header =
    incoming.next_header[my_block_number()];
  while (header != incoming.headers.end() && header->tag != tag) ++header;

  // If no header is found, notify the caller
  if (header == incoming.headers.end()) return false;

  // Deserialize the data
  boost::mpi::packed_iarchive in(impl_->comm, incoming.buffer, 
                                 archive::no_header, header->offset);
  std::size_t num_sent;
  in >> num_sent;
  if (num_sent > n)
    std::cerr << "ERROR: Have " << num_sent << " items but only space for "
              << n << " items\n";

  for (std::size_t i = 0; i < num_sent; ++i)
    in >> values[i];
  n = num_sent;

  // Mark this message as received
  header->tag = -1;

  // Move the "next header" indicator to the next unreceived message
  while (incoming.next_header[my_block_number()] != incoming.headers.end()
         && incoming.next_header[my_block_number()]->tag == -1)
    ++incoming.next_header[my_block_number()];

  if (incoming.next_header[my_block_number()] == incoming.headers.end()) {
    bool finished = true;
    for (std::size_t i = 0; i < incoming.next_header.size() && finished; ++i) {
      if (incoming.next_header[i] != incoming.headers.end()) finished = false;
    }

    if (finished) {
      std::vector<impl::message_header> no_headers;
      incoming.headers.swap(no_headers);
      buffer_type empty_buffer;
      incoming.buffer.swap(empty_buffer);
      for (std::size_t i = 0; i < incoming.next_header.size(); ++i)
        incoming.next_header[i] = incoming.headers.end();
    }
  }

  return true;
}

// Construct triggers
template<typename Type, typename Handler>
void mpi_process_group::trigger(int tag, const Handler& handler)
{
  BOOST_ASSERT(block_num);
  install_trigger(tag,my_block_number(),shared_ptr<trigger_base>(
    new trigger_launcher<Type, Handler>(*this, tag, handler)));
}

template<typename Type, typename Handler>
void mpi_process_group::trigger_with_reply(int tag, const Handler& handler)
{
  BOOST_ASSERT(block_num);
  install_trigger(tag,my_block_number(),shared_ptr<trigger_base>(
    new reply_trigger_launcher<Type, Handler>(*this, tag, handler)));
}

template<typename Type, typename Handler>
void mpi_process_group::global_trigger(int tag, const Handler& handler, 
      std::size_t sz)
{
  if (sz==0) // normal trigger
    install_trigger(tag,0,shared_ptr<trigger_base>(
      new global_trigger_launcher<Type, Handler>(*this, tag, handler)));
  else // trigger with irecv
    install_trigger(tag,0,shared_ptr<trigger_base>(
      new global_irecv_trigger_launcher<Type, Handler>(*this, tag, handler,sz)));
  
}

namespace detail {

template<typename Type>
void  do_oob_receive(mpi_process_group const& self,
    int source, int tag, Type& data, mpl::true_ /*is_mpi_datatype*/) 
{
  using boost::mpi::get_mpi_datatype;

  //self.impl_->comm.recv(source,tag,data);
  MPI_Recv(&data, 1, get_mpi_datatype<Type>(data), source, tag, self.impl_->comm,
           MPI_STATUS_IGNORE);
}

template<typename Type>
void do_oob_receive(mpi_process_group const& self,
    int source, int tag, Type& data, mpl::false_ /*is_mpi_datatype*/) 
{
  //  self.impl_->comm.recv(source,tag,data);
  // Receive the size of the data packet
  boost::mpi::status status;
  status = self.impl_->comm.probe(source, tag);

#if BOOST_VERSION >= 103600
  int size = status.count<boost::mpi::packed>().get();
#else
  int size;
  MPI_Status& mpi_status = status;
  MPI_Get_count(&mpi_status, MPI_PACKED, &size);
#endif

  // Receive the data packed itself
  boost::mpi::packed_iarchive in(self.impl_->comm);
  in.resize(size);
  MPI_Recv(in.address(), size, MPI_PACKED, source, tag, self.impl_->comm,
       MPI_STATUS_IGNORE);

  // Deserialize the data
  in >> data;
}

template<typename Type>
void do_oob_receive(mpi_process_group const& self, int source, int tag, Type& data) 
{
  do_oob_receive(self, source, tag, data,
                           boost::mpi::is_mpi_datatype<Type>());
}


} // namespace detail


template<typename Type, typename Handler>
void 
mpi_process_group::trigger_launcher<Type, Handler>::
receive(mpi_process_group const&, int source, int tag, 
        trigger_receive_context context, int block) const
{
#ifdef PBGL_PROCESS_GROUP_DEBUG
  std::cerr << (out_of_band? "OOB trigger" : "Trigger") 
            << " receive from source " << source << " and tag " << tag
        << " in block " << (block == -1 ? self.my_block_number() : block) << std::endl;
#endif

  Type data;

  if (context == trc_out_of_band) {
    // Receive the message directly off the wire
    int realtag  = self.encode_tag(
      block == -1 ? self.my_block_number() : block, tag);
    detail::do_oob_receive(self,source,realtag,data);
  }
  else
    // Receive the message out of the local buffer
    boost::graph::distributed::receive(self, source, tag, data);

  // Pass the message off to the handler
  handler(source, tag, data, context);
}

template<typename Type, typename Handler>
void 
mpi_process_group::reply_trigger_launcher<Type, Handler>::
receive(mpi_process_group const&, int source, int tag, 
        trigger_receive_context context, int block) const
{
#ifdef PBGL_PROCESS_GROUP_DEBUG
  std::cerr << (out_of_band? "OOB reply trigger" : "Reply trigger") 
            << " receive from source " << source << " and tag " << tag
        << " in block " << (block == -1 ? self.my_block_number() : block) << std::endl;
#endif
  BOOST_ASSERT(context == trc_out_of_band);

  boost::parallel::detail::untracked_pair<int, Type> data;

  // Receive the message directly off the wire
  int realtag  = self.encode_tag(block == -1 ? self.my_block_number() : block,
                                 tag);
  detail::do_oob_receive(self, source, realtag, data);

  // Pass the message off to the handler and send the result back to
  // the source.
  send_oob(self, source, data.first, 
           handler(source, tag, data.second, context), -2);
}

template<typename Type, typename Handler>
void 
mpi_process_group::global_trigger_launcher<Type, Handler>::
receive(mpi_process_group const& self, int source, int tag, 
        trigger_receive_context context, int block) const
{
#ifdef PBGL_PROCESS_GROUP_DEBUG
  std::cerr << (out_of_band? "OOB trigger" : "Trigger") 
            << " receive from source " << source << " and tag " << tag
        << " in block " << (block == -1 ? self.my_block_number() : block) << std::endl;
#endif

  Type data;

  if (context == trc_out_of_band) {
    // Receive the message directly off the wire
    int realtag  = self.encode_tag(
      block == -1 ? self.my_block_number() : block, tag);
    detail::do_oob_receive(self,source,realtag,data);
  }
  else
    // Receive the message out of the local buffer
    boost::graph::distributed::receive(self, source, tag, data);

  // Pass the message off to the handler
  handler(self, source, tag, data, context);
}


template<typename Type, typename Handler>
void 
mpi_process_group::global_irecv_trigger_launcher<Type, Handler>::
receive(mpi_process_group const& self, int source, int tag, 
        trigger_receive_context context, int block) const
{
#ifdef PBGL_PROCESS_GROUP_DEBUG
  std::cerr << (out_of_band? "OOB trigger" : "Trigger") 
            << " receive from source " << source << " and tag " << tag
        << " in block " << (block == -1 ? self.my_block_number() : block) << std::endl;
#endif

  Type data;

  if (context == trc_out_of_band) {
    return;
  }
  BOOST_ASSERT (context == trc_irecv_out_of_band);

  // force posting of new MPI_Irecv, even though buffer is already allocated
  boost::mpi::packed_iarchive ia(self.impl_->comm,self.impl_->buffers[tag]);
  ia >> data;
  // Start a new receive
  prepare_receive(self,tag,true);
  // Pass the message off to the handler
  handler(self, source, tag, data, context);
}


template<typename Type, typename Handler>
void 
mpi_process_group::global_irecv_trigger_launcher<Type, Handler>::
prepare_receive(mpi_process_group const& self, int tag, bool force) const
{
#ifdef PBGL_PROCESS_GROUP_DEBUG
 std::cerr << ("Posting Irecv for trigger") 
      << " receive with tag " << tag << std::endl;
#endif
  if (self.impl_->buffers.find(tag) == self.impl_->buffers.end()) {
    self.impl_->buffers[tag].resize(buffer_size);
    force = true;
  }
  BOOST_ASSERT(static_cast<int>(self.impl_->buffers[tag].size()) >= buffer_size);
  
  //BOOST_MPL_ASSERT(mpl::not_<is_mpi_datatype<Type> >);
  if (force) {
    self.impl_->requests.push_back(MPI_Request());
    MPI_Request* request = &self.impl_->requests.back();
    MPI_Irecv(&self.impl_->buffers[tag].front(),buffer_size,
               MPI_PACKED,MPI_ANY_SOURCE,tag,self.impl_->comm,request);
  }
}


template<typename T>
inline mpi_process_group::process_id_type
receive(const mpi_process_group& pg, int tag, T& value)
{
  for (std::size_t source = 0; source < pg.impl_->incoming.size(); ++source) {
    if (pg.receive_impl(source, pg.encode_tag(pg.my_block_number(), tag),
                        value, boost::mpi::is_mpi_datatype<T>()))
      return source;
  }
  BOOST_ASSERT (false);
}

template<typename T>
typename 
  enable_if<boost::mpi::is_mpi_datatype<T>, 
            std::pair<mpi_process_group::process_id_type, std::size_t> >::type
receive(const mpi_process_group& pg, int tag, T values[], std::size_t n)
{
  for (std::size_t source = 0; source < pg.impl_->incoming.size(); ++source) {
    bool result =
      pg.receive_impl(source, pg.encode_tag(pg.my_block_number(), tag),
                 boost::serialization::make_array(values,n),
                 boost::mpl::true_());
    if (result) 
      return std::make_pair(source, n);
  }
  BOOST_ASSERT(false);
}

template<typename T>
typename 
  disable_if<boost::mpi::is_mpi_datatype<T>, 
             std::pair<mpi_process_group::process_id_type, std::size_t> >::type
receive(const mpi_process_group& pg, int tag, T values[], std::size_t n)
{
  for (std::size_t source = 0; source < pg.impl_->incoming.size(); ++source) {
    if (pg.array_receive_impl(source, pg.encode_tag(pg.my_block_number(), tag),
                              values, n))
      return std::make_pair(source, n);
  }
  BOOST_ASSERT(false);
}

template<typename T>
mpi_process_group::process_id_type
receive(const mpi_process_group& pg,
        mpi_process_group::process_id_type source, int tag, T& value)
{
  if (pg.receive_impl(source, pg.encode_tag(pg.my_block_number(), tag),
                      value, boost::mpi::is_mpi_datatype<T>()))
    return source;
  else {
    fprintf(stderr,
            "Process %d failed to receive a message from process %d with tag %d in block %d.\n",
            process_id(pg), source, tag, pg.my_block_number());

    BOOST_ASSERT(false);
    abort();
  }
}

template<typename T>
typename 
  enable_if<boost::mpi::is_mpi_datatype<T>, 
            std::pair<mpi_process_group::process_id_type, std::size_t> >::type
receive(const mpi_process_group& pg, int source, int tag, T values[], 
        std::size_t n)
{
  if (pg.receive_impl(source, pg.encode_tag(pg.my_block_number(), tag),
                      boost::serialization::make_array(values,n), 
                      boost::mpl::true_()))
    return std::make_pair(source,n);
  else {
    fprintf(stderr,
            "Process %d failed to receive a message from process %d with tag %d in block %d.\n",
            process_id(pg), source, tag, pg.my_block_number());

    BOOST_ASSERT(false);
    abort();
  }
}

template<typename T>
typename 
  disable_if<boost::mpi::is_mpi_datatype<T>, 
             std::pair<mpi_process_group::process_id_type, std::size_t> >::type
receive(const mpi_process_group& pg, int source, int tag, T values[], 
        std::size_t n)
{
  pg.array_receive_impl(source, pg.encode_tag(pg.my_block_number(), tag),
                        values, n);

  return std::make_pair(source, n);
}

template<typename T, typename BinaryOperation>
T*
all_reduce(const mpi_process_group& pg, T* first, T* last, T* out,
           BinaryOperation bin_op)
{
  synchronize(pg);

  bool inplace = first == out;

  if (inplace) out = new T [last-first];

  boost::mpi::all_reduce(boost::mpi::communicator(communicator(pg),
                                                  boost::mpi::comm_attach), 
                         first, last-first, out, bin_op);

  if (inplace) {
    std::copy(out, out + (last-first), first);
    delete [] out;
    return last;
  }

  return out;
}

template<typename T>
void
broadcast(const mpi_process_group& pg, T& val, 
          mpi_process_group::process_id_type root)
{
  // broadcast the seed  
  boost::mpi::communicator comm(communicator(pg),boost::mpi::comm_attach);
  boost::mpi::broadcast(comm,val,root);
}


template<typename T, typename BinaryOperation>
T*
scan(const mpi_process_group& pg, T* first, T* last, T* out,
           BinaryOperation bin_op)
{
  synchronize(pg);

  bool inplace = first == out;

  if (inplace) out = new T [last-first];

  boost::mpi::scan(communicator(pg), first, last-first, out, bin_op);

  if (inplace) {
    std::copy(out, out + (last-first), first);
    delete [] out;
    return last;
  }

  return out;
}


template<typename InputIterator, typename T>
void
all_gather(const mpi_process_group& pg, InputIterator first,
           InputIterator last, std::vector<T>& out)
{
  synchronize(pg);

  // Stick a copy of the local values into a vector, so we can broadcast it
  std::vector<T> local_values(first, last);

  // Collect the number of vertices stored in each process
  int size = local_values.size();
  std::vector<int> sizes(num_processes(pg));
  int result = MPI_Allgather(&size, 1, MPI_INT,
                             &sizes[0], 1, MPI_INT,
                             communicator(pg));
  BOOST_ASSERT(result == MPI_SUCCESS);
  (void)result;

  // Adjust sizes based on the number of bytes
  //
  // std::transform(sizes.begin(), sizes.end(), sizes.begin(),
  //               std::bind2nd(std::multiplies<int>(), sizeof(T)));
  //
  // std::bind2nd has been removed from C++17

  for( std::size_t i = 0, n = sizes.size(); i < n; ++i )
  {
    sizes[ i ] *= sizeof( T );
  }

  // Compute displacements
  std::vector<int> displacements;
  displacements.reserve(sizes.size() + 1);
  displacements.push_back(0);
  std::partial_sum(sizes.begin(), sizes.end(),
                   std::back_inserter(displacements));

  // Gather all of the values
  out.resize(displacements.back() / sizeof(T));
  if (!out.empty()) {
    result = MPI_Allgatherv(local_values.empty()? (void*)&local_values
                            /* local results */: (void*)&local_values[0],
                            local_values.size() * sizeof(T),
                            MPI_BYTE,
                            &out[0], &sizes[0], &displacements[0], MPI_BYTE,
                            communicator(pg));
  }
  BOOST_ASSERT(result == MPI_SUCCESS);
}

template<typename InputIterator>
mpi_process_group
process_subgroup(const mpi_process_group& pg,
                 InputIterator first, InputIterator last)
{
/*
  boost::mpi::group current_group = communicator(pg).group();
  boost::mpi::group new_group = current_group.include(first,last);
  boost::mpi::communicator new_comm(communicator(pg),new_group);
  return mpi_process_group(new_comm);
*/
  std::vector<int> ranks(first, last);

  MPI_Group current_group;
  int result = MPI_Comm_group(communicator(pg), &current_group);
  BOOST_ASSERT(result == MPI_SUCCESS);
  (void)result;

  MPI_Group new_group;
  result = MPI_Group_incl(current_group, ranks.size(), &ranks[0], &new_group);
  BOOST_ASSERT(result == MPI_SUCCESS);

  MPI_Comm new_comm;
  result = MPI_Comm_create(communicator(pg), new_group, &new_comm);
  BOOST_ASSERT(result == MPI_SUCCESS);

  result = MPI_Group_free(&new_group);
  BOOST_ASSERT(result == MPI_SUCCESS);
  result = MPI_Group_free(&current_group);
  BOOST_ASSERT(result == MPI_SUCCESS);

  if (new_comm != MPI_COMM_NULL) {
    mpi_process_group result_pg(boost::mpi::communicator(new_comm,boost::mpi::comm_attach));
    result = MPI_Comm_free(&new_comm);
    BOOST_ASSERT(result == 0);
    return result_pg;
  } else {
    return mpi_process_group(mpi_process_group::create_empty());
  }

}


template<typename Receiver>
Receiver* mpi_process_group::get_receiver()
{
  return impl_->blocks[my_block_number()]->on_receive
           .template target<Receiver>();
}

template<typename T>
typename enable_if<boost::mpi::is_mpi_datatype<T> >::type
receive_oob(const mpi_process_group& pg, 
            mpi_process_group::process_id_type source, int tag, T& value, int block)
{
  using boost::mpi::get_mpi_datatype;

  // Determine the actual message we expect to receive, and which
  // communicator it will come by.
  std::pair<boost::mpi::communicator, int> actual
    = pg.actual_communicator_and_tag(tag, block);

  // Post a non-blocking receive that waits until we complete this request.
  MPI_Request request;
  MPI_Irecv(&value, 1, get_mpi_datatype<T>(value),  
            source, actual.second, actual.first, &request); 

  int done = 0;
  do {
    MPI_Test(&request, &done, MPI_STATUS_IGNORE);
    if (!done)
      pg.poll(/*wait=*/false, block);
  } while (!done);
}

template<typename T>
typename disable_if<boost::mpi::is_mpi_datatype<T> >::type
receive_oob(const mpi_process_group& pg, 
            mpi_process_group::process_id_type source, int tag, T& value, int block)
{
  // Determine the actual message we expect to receive, and which
  // communicator it will come by.
  std::pair<boost::mpi::communicator, int> actual
    = pg.actual_communicator_and_tag(tag, block);

  boost::optional<boost::mpi::status> status;
  do {
    status = actual.first.iprobe(source, actual.second);
    if (!status)
      pg.poll();
  } while (!status);

  //actual.first.recv(status->source(), status->tag(),value);

  // Allocate the receive buffer
  boost::mpi::packed_iarchive in(actual.first);

#if BOOST_VERSION >= 103600
  in.resize(status->count<boost::mpi::packed>().get());
#else
  int size;
  MPI_Status mpi_status = *status;
  MPI_Get_count(&mpi_status, MPI_PACKED, &size);
  in.resize(size);
#endif
  
  // Receive the message data
  MPI_Recv(in.address(), in.size(), MPI_PACKED,
           status->source(), status->tag(), actual.first, MPI_STATUS_IGNORE);
  
  // Unpack the message data
  in >> value;
}


template<typename SendT, typename ReplyT>
typename enable_if<boost::mpi::is_mpi_datatype<ReplyT> >::type
send_oob_with_reply(const mpi_process_group& pg, 
                    mpi_process_group::process_id_type dest,
                    int tag, const SendT& send_value, ReplyT& reply_value,
                    int block)
{
  detail::tag_allocator::token reply_tag = pg.impl_->allocated_tags.get_tag();
  send_oob(pg, dest, tag, boost::parallel::detail::make_untracked_pair(
        (int)reply_tag, send_value), block);
  receive_oob(pg, dest, reply_tag, reply_value);
}

template<typename SendT, typename ReplyT>
typename disable_if<boost::mpi::is_mpi_datatype<ReplyT> >::type
send_oob_with_reply(const mpi_process_group& pg, 
                    mpi_process_group::process_id_type dest,
                    int tag, const SendT& send_value, ReplyT& reply_value,
                    int block)
{
  detail::tag_allocator::token reply_tag = pg.impl_->allocated_tags.get_tag();
  send_oob(pg, dest, tag, 
           boost::parallel::detail::make_untracked_pair((int)reply_tag, 
                                                        send_value), block);
  receive_oob(pg, dest, reply_tag, reply_value);
}

} } } // end namespace boost::graph::distributed
