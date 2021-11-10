// Copyright (C) 2004-2008 The Trustees of Indiana University.
// Copyright (C) 2007   Douglas Gregor
// Copyright (C) 2007  Matthias Troyer  <troyer@boost-consulting.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Matthias Troyer
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_DISTRIBUTED_MPI_PROCESS_GROUP
#define BOOST_GRAPH_DISTRIBUTED_MPI_PROCESS_GROUP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

//#define NO_SPLIT_BATCHES
#define SEND_OOB_BSEND

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <utility>
#include <memory>
#include <boost/function/function1.hpp>
#include <boost/function/function2.hpp>
#include <boost/function/function0.hpp>
#include <boost/mpi.hpp>
#include <boost/property_map/parallel/process_group.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace graph { namespace distributed {

// Process group tags
struct mpi_process_group_tag : virtual boost::parallel::linear_process_group_tag { };

class mpi_process_group
{
  struct impl;

 public:
  /// Number of tags available to each data structure.
  static const int max_tags = 256;

  /**
   * The type of a "receive" handler, that will be provided with
   * (source, tag) pairs when a message is received. Users can provide a
   * receive handler for a distributed data structure, for example, to
   * automatically pick up and respond to messages as needed.  
   */
  typedef function<void(int source, int tag)> receiver_type;

  /**
   * The type of a handler for the on-synchronize event, which will be
   * executed at the beginning of synchronize().
   */
  typedef function0<void>      on_synchronize_event_type;

  /// Used as a tag to help create an "empty" process group.
  struct create_empty {};

  /// The type used to buffer message data
  typedef boost::mpi::packed_oprimitive::buffer_type buffer_type;

  /// The type used to identify a process
  typedef int process_id_type;

  /// The type used to count the number of processes
  typedef int process_size_type;

  /// The type of communicator used to transmit data via MPI
  typedef boost::mpi::communicator communicator_type;

  /// Classification of the capabilities of this process group
  struct communication_category
    : virtual boost::parallel::bsp_process_group_tag, 
      virtual mpi_process_group_tag { };

  // TBD: We can eliminate the "source" field and possibly the
  // "offset" field.
  struct message_header {
    /// The process that sent the message
    process_id_type source;

    /// The message tag
    int tag;

    /// The offset of the message into the buffer
    std::size_t offset;

    /// The length of the message in the buffer, in bytes
    std::size_t bytes;
    
    template <class Archive>
    void serialize(Archive& ar, int)
    {
      ar & source & tag & offset & bytes;
    }
  };

  /**
   * Stores the outgoing messages for a particular processor.
   *
   * @todo Evaluate whether we should use a deque instance, which
   * would reduce could reduce the cost of "sending" messages but
   * increases the time spent in the synchronization step.
   */
  struct outgoing_messages {
        outgoing_messages() {}
        ~outgoing_messages() {}

    std::vector<message_header> headers;
    buffer_type                 buffer;
    
    template <class Archive>
    void serialize(Archive& ar, int)
    {
      ar & headers & buffer;
    }
    
    void swap(outgoing_messages& x) 
    {
      headers.swap(x.headers);
      buffer.swap(x.buffer);
    }
  };

private:
  /**
   * Virtual base from which every trigger will be launched. See @c
   * trigger_launcher for more information.
   */
  class trigger_base : boost::noncopyable
  {
  public:
    explicit trigger_base(int tag) : tag_(tag) { }

    /// Retrieve the tag associated with this trigger  
    int tag() const { return tag_; }

    virtual ~trigger_base() { }

    /**
     * Invoked to receive a message that matches a particular trigger. 
     *
     * @param source      the source of the message
     * @param tag         the (local) tag of the message
     * @param context     the context under which the trigger is being
     *                    invoked
     */
    virtual void 
    receive(mpi_process_group const& pg, int source, int tag, 
            trigger_receive_context context, int block=-1) const = 0;

  protected:
    // The message tag associated with this trigger
    int tag_;
  };

  /**
   * Launches a specific handler in response to a trigger. This
   * function object wraps up the handler function object and a buffer
   * for incoming data. 
   */
  template<typename Type, typename Handler>
  class trigger_launcher : public trigger_base
  {
  public:
    explicit trigger_launcher(mpi_process_group& self, int tag, 
                              const Handler& handler) 
      : trigger_base(tag), self(self), handler(handler) 
      {}

    void 
    receive(mpi_process_group const& pg, int source, int tag,  
            trigger_receive_context context, int block=-1) const;

  private:
    mpi_process_group& self;
    mutable Handler handler;
  };

  /**
   * Launches a specific handler with a message reply in response to a
   * trigger. This function object wraps up the handler function
   * object and a buffer for incoming data.
   */
  template<typename Type, typename Handler>
  class reply_trigger_launcher : public trigger_base
  {
  public:
    explicit reply_trigger_launcher(mpi_process_group& self, int tag, 
                                    const Handler& handler) 
      : trigger_base(tag), self(self), handler(handler) 
      {}

    void 
    receive(mpi_process_group const& pg, int source, int tag, 
            trigger_receive_context context, int block=-1) const;

  private:
    mpi_process_group& self;
    mutable Handler handler;
  };

  template<typename Type, typename Handler>
  class global_trigger_launcher : public trigger_base
  {
  public:
    explicit global_trigger_launcher(mpi_process_group& self, int tag, 
                              const Handler& handler) 
      : trigger_base(tag), handler(handler) 
      { 
      }

    void 
    receive(mpi_process_group const& pg, int source, int tag, 
            trigger_receive_context context, int block=-1) const;

  private:
    mutable Handler handler;
    // TBD: do not forget to cancel any outstanding Irecv when deleted,
    // if we decide to use Irecv
  };

  template<typename Type, typename Handler>
  class global_irecv_trigger_launcher : public trigger_base
  {
  public:
    explicit global_irecv_trigger_launcher(mpi_process_group& self, int tag, 
                              const Handler& handler, int sz) 
      : trigger_base(tag), handler(handler), buffer_size(sz)
      { 
        prepare_receive(self,tag);
      }

    void 
    receive(mpi_process_group const& pg, int source, int tag, 
            trigger_receive_context context, int block=-1) const;

  private:
    void prepare_receive(mpi_process_group const& pg, int tag, bool force=false) const;
    Handler handler;
    int buffer_size;
    // TBD: do not forget to cancel any outstanding Irecv when deleted,
    // if we decide to use Irecv
  };

public:
  /** 
   * Construct a new BSP process group from an MPI communicator. The
   * MPI communicator will be duplicated to create a new communicator
   * for this process group to use.
   */
  mpi_process_group(communicator_type parent_comm = communicator_type());

  /** 
   * Construct a new BSP process group from an MPI communicator. The
   * MPI communicator will be duplicated to create a new communicator
   * for this process group to use. This constructor allows to tune the
   * size of message batches.
   *    
   *   @param num_headers The maximum number of headers in a message batch
   *
   *   @param buffer_size The maximum size of the message buffer in a batch.
   *
   */
  mpi_process_group( std::size_t num_headers, std::size_t buffer_size, 
                     communicator_type parent_comm = communicator_type());

  /**
   * Construct a copy of the BSP process group for a new distributed
   * data structure. This data structure will synchronize with all
   * other members of the process group's equivalence class (including
   * @p other), but will have its own set of tags. 
   *
   *   @param other The process group that this new process group will
   *   be based on, using a different set of tags within the same
   *   communication and synchronization space.
   *
   *   @param handler A message handler that will be passed (source,
   *   tag) pairs for each message received by this data
   *   structure. The handler is expected to receive the messages
   *   immediately. The handler can be changed after-the-fact by
   *   calling @c replace_handler.
   *
   *   @param out_of_band_receive An anachronism. TODO: remove this.
   */
  mpi_process_group(const mpi_process_group& other,
                    const receiver_type& handler,
                    bool out_of_band_receive = false);

  /**
   * Construct a copy of the BSP process group for a new distributed
   * data structure. This data structure will synchronize with all
   * other members of the process group's equivalence class (including
   * @p other), but will have its own set of tags. 
   */
  mpi_process_group(const mpi_process_group& other, 
                    attach_distributed_object,
                    bool out_of_band_receive = false);

  /**
   * Create an "empty" process group, with no information. This is an
   * internal routine that users should never need.
   */
  explicit mpi_process_group(create_empty) {}

  /**
   * Destroys this copy of the process group.
   */
  ~mpi_process_group();

  /**
   * Replace the current message handler with a new message handler.
   *
   * @param handle The new message handler.
   * @param out_of_band_receive An anachronism: remove this
   */
  void replace_handler(const receiver_type& handler,
                       bool out_of_band_receive = false);

  /**
   * Turns this process group into the process group for a new
   * distributed data structure or object, allocating its own tag
   * block.
   */
  void make_distributed_object();

  /**
   * Replace the handler to be invoked at the beginning of synchronize.
   */
  void
  replace_on_synchronize_handler(const on_synchronize_event_type& handler = 0);

  /** 
   * Return the block number of the current data structure. A value of
   * 0 indicates that this particular instance of the process group is
   * not associated with any distributed data structure.
   */
  int my_block_number() const { return block_num? *block_num : 0; }

  /**
   * Encode a block number/tag pair into a single encoded tag for
   * transmission.
   */
  int encode_tag(int block_num, int tag) const
  { return block_num * max_tags + tag; }

  /**
   * Decode an encoded tag into a block number/tag pair. 
   */
  std::pair<int, int> decode_tag(int encoded_tag) const
  { return std::make_pair(encoded_tag / max_tags, encoded_tag % max_tags); }

  // @todo Actually write up the friend declarations so these could be
  // private.

  // private:

  /** Allocate a block of tags for this instance. The block should not
   * have been allocated already, e.g., my_block_number() ==
   * 0. Returns the newly-allocated block number.
   */
  int allocate_block(bool out_of_band_receive = false);

  /** Potentially emit a receive event out of band. Returns true if an event 
   *  was actually sent, false otherwise. 
   */
  bool maybe_emit_receive(int process, int encoded_tag) const;

  /** Emit a receive event. Returns true if an event was actually
   * sent, false otherwise. 
   */
  bool emit_receive(int process, int encoded_tag) const;

  /** Emit an on-synchronize event to all block handlers. */
  void emit_on_synchronize() const;

  /** Retrieve a reference to the stored receiver in this block.  */
  template<typename Receiver>
  Receiver* get_receiver();

  template<typename T>
  void
  send_impl(int dest, int tag, const T& value,
            mpl::true_ /*is_mpi_datatype*/) const;

  template<typename T>
  void
  send_impl(int dest, int tag, const T& value,
            mpl::false_ /*is_mpi_datatype*/) const;

  template<typename T>
  typename disable_if<boost::mpi::is_mpi_datatype<T>, void>::type
  array_send_impl(int dest, int tag, const T values[], std::size_t n) const;

  template<typename T>
  bool
  receive_impl(int source, int tag, T& value,
               mpl::true_ /*is_mpi_datatype*/) const;

  template<typename T>
  bool
  receive_impl(int source, int tag, T& value,
               mpl::false_ /*is_mpi_datatype*/) const;

  // Receive an array of values
  template<typename T>
  typename disable_if<boost::mpi::is_mpi_datatype<T>, bool>::type
  array_receive_impl(int source, int tag, T* values, std::size_t& n) const;

  optional<std::pair<mpi_process_group::process_id_type, int> > probe() const;

  void synchronize() const;

  operator bool() { return bool(impl_); }

  mpi_process_group base() const;

  /**
   * Create a new trigger for a specific message tag. Triggers handle
   * out-of-band messaging, and the handler itself will be called
   * whenever a message is available. The handler itself accepts four
   * arguments: the source of the message, the message tag (which will
   * be the same as @p tag), the message data (of type @c Type), and a
   * boolean flag that states whether the message was received
   * out-of-band. The last will be @c true for out-of-band receives,
   * or @c false for receives at the end of a synchronization step.
   */
  template<typename Type, typename Handler>
  void trigger(int tag, const Handler& handler);

  /**
   * Create a new trigger for a specific message tag, along with a way
   * to send a reply with data back to the sender. Triggers handle
   * out-of-band messaging, and the handler itself will be called
   * whenever a message is available. The handler itself accepts four
   * arguments: the source of the message, the message tag (which will
   * be the same as @p tag), the message data (of type @c Type), and a
   * boolean flag that states whether the message was received
   * out-of-band. The last will be @c true for out-of-band receives,
   * or @c false for receives at the end of a synchronization
   * step. The handler also returns a value, which will be routed back
   * to the sender.
   */
  template<typename Type, typename Handler>
  void trigger_with_reply(int tag, const Handler& handler);

  template<typename Type, typename Handler>
  void global_trigger(int tag, const Handler& handler, std::size_t buffer_size=0); 



  /**
   * Poll for any out-of-band messages. This routine will check if any
   * out-of-band messages are available. Those that are available will
   * be handled immediately, if possible.
   *
   * @returns if an out-of-band message has been received, but we are
   * unable to actually receive the message, a (source, tag) pair will
   * be returned. Otherwise, returns an empty optional.
   *
   * @param wait When true, we should block until a message comes in.
   *
   * @param synchronizing whether we are currently synchronizing the
   *                      process group
   */
  optional<std::pair<int, int> > 
  poll(bool wait = false, int block = -1, bool synchronizing = false) const;

  /**
   * Determines the context of the trigger currently executing. If
   * multiple triggers are executing (recursively), then the context
   * for the most deeply nested trigger will be returned. If no
   * triggers are executing, returns @c trc_none. This might be used,
   * for example, to determine whether a reply to a message should
   * itself be sent out-of-band or whether it can go via the normal,
   * slower communication route.
   */
  trigger_receive_context trigger_context() const;

  /// INTERNAL ONLY
  void receive_batch(process_id_type source, outgoing_messages& batch) const;

  /// INTERNAL ONLY
  ///
  /// Determine the actual communicator and tag will be used for a
  /// transmission with the given tag.
  std::pair<boost::mpi::communicator, int> 
  actual_communicator_and_tag(int tag, int block) const;

  /// set the size of the message buffer used for buffered oob sends
  
  static void set_message_buffer_size(std::size_t s);

  /// get the size of the message buffer used for buffered oob sends

  static std::size_t message_buffer_size();
  static int old_buffer_size;
  static void* old_buffer;
private:

  void install_trigger(int tag, int block, 
      shared_ptr<trigger_base> const& launcher); 

  void poll_requests(int block=-1) const;

  
  // send a batch if the buffer is full now or would get full
  void maybe_send_batch(process_id_type dest) const;

  // actually send a batch
  void send_batch(process_id_type dest, outgoing_messages& batch) const;
  void send_batch(process_id_type dest) const;

  void pack_headers() const;

  /**
   * Process a batch of incoming messages immediately.
   *
   * @param source         the source of these messages
   */
  void process_batch(process_id_type source) const;
  void receive_batch(boost::mpi::status& status) const;

  //void free_finished_sends() const;
          
  /// Status messages used internally by the process group
  enum status_messages {
    /// the first of the reserved message tags
    msg_reserved_first = 126,
    /// Sent from a processor when sending batched messages
    msg_batch = 126,
    /// Sent from a processor when sending large batched messages, larger than
    /// the maximum buffer size for messages to be received by MPI_Irecv
    msg_large_batch = 127,
    /// Sent from a source processor to everyone else when that
    /// processor has entered the synchronize() function.
    msg_synchronizing = 128,
    /// the last of the reserved message tags
    msg_reserved_last = 128
  };

  /**
   * Description of a block of tags associated to a particular
   * distributed data structure. This structure will live as long as
   * the distributed data structure is around, and will be used to
   * help send messages to the data structure.
   */
  struct block_type
  {
    block_type() { }

    /// Handler for receive events
    receiver_type     on_receive;

    /// Handler executed at the start of  synchronization 
    on_synchronize_event_type  on_synchronize;

    /// Individual message triggers. Note: at present, this vector is
    /// indexed by the (local) tag of the trigger.  Any tags that
    /// don't have triggers will have NULL pointers in that spot.
    std::vector<shared_ptr<trigger_base> > triggers;
  };

  /**
   * Data structure containing all of the blocks for the distributed
   * data structures attached to a process group.
   */
  typedef std::vector<block_type*> blocks_type;

  /// Iterator into @c blocks_type.
  typedef blocks_type::iterator block_iterator;

  /**
   * Deleter used to deallocate a block when its distributed data
   * structure is destroyed. This type will be used as the deleter for
   * @c block_num.
   */
  struct deallocate_block;
  
  static std::vector<char> message_buffer;

public:
  /**
   * Data associated with the process group and all of its attached
   * distributed data structures.
   */
  shared_ptr<impl> impl_;

  /**
   * When non-null, indicates that this copy of the process group is
   * associated with a particular distributed data structure. The
   * integer value contains the block number (a value > 0) associated
   * with that data structure. The deleter for this @c shared_ptr is a
   * @c deallocate_block object that will deallocate the associated
   * block in @c impl_->blocks.
   */
  shared_ptr<int>  block_num;

  /**
   * Rank of this process, to avoid having to call rank() repeatedly.
   */
  int rank;

  /**
   * Number of processes in this process group, to avoid having to
   * call communicator::size() repeatedly.
   */
  int size;
};

inline mpi_process_group::process_id_type 
process_id(const mpi_process_group& pg)
{ return pg.rank; }

inline mpi_process_group::process_size_type 
num_processes(const mpi_process_group& pg)
{ return pg.size; }

mpi_process_group::communicator_type communicator(const mpi_process_group& pg);

template<typename T>
void
send(const mpi_process_group& pg, mpi_process_group::process_id_type dest,
     int tag, const T& value);

template<typename InputIterator>
void
send(const mpi_process_group& pg, mpi_process_group::process_id_type dest,
     int tag, InputIterator first, InputIterator last);

template<typename T>
inline void
send(const mpi_process_group& pg, mpi_process_group::process_id_type dest,
     int tag, T* first, T* last)
{ send(pg, dest, tag, first, last - first); }

template<typename T>
inline void
send(const mpi_process_group& pg, mpi_process_group::process_id_type dest,
     int tag, const T* first, const T* last)
{ send(pg, dest, tag, first, last - first); }

template<typename T>
mpi_process_group::process_id_type
receive(const mpi_process_group& pg, int tag, T& value);

template<typename T>
mpi_process_group::process_id_type
receive(const mpi_process_group& pg,
        mpi_process_group::process_id_type source, int tag, T& value);

optional<std::pair<mpi_process_group::process_id_type, int> >
probe(const mpi_process_group& pg);

void synchronize(const mpi_process_group& pg);

template<typename T, typename BinaryOperation>
T*
all_reduce(const mpi_process_group& pg, T* first, T* last, T* out,
           BinaryOperation bin_op);

template<typename T, typename BinaryOperation>
T*
scan(const mpi_process_group& pg, T* first, T* last, T* out,
           BinaryOperation bin_op);

template<typename InputIterator, typename T>
void
all_gather(const mpi_process_group& pg,
           InputIterator first, InputIterator last, std::vector<T>& out);

template<typename InputIterator>
mpi_process_group
process_subgroup(const mpi_process_group& pg,
                 InputIterator first, InputIterator last);

template<typename T>
void
broadcast(const mpi_process_group& pg, T& val, 
          mpi_process_group::process_id_type root);


/// optimized swap for outgoing messages
inline void
swap(mpi_process_group::outgoing_messages& x,
     mpi_process_group::outgoing_messages& y)
{
  x.swap(y);
}


/*******************************************************************
 * Out-of-band communication                                       *
 *******************************************************************/

template<typename T>
typename enable_if<boost::mpi::is_mpi_datatype<T> >::type
send_oob(const mpi_process_group& pg, mpi_process_group::process_id_type dest,
         int tag, const T& value, int block=-1)
{
  using boost::mpi::get_mpi_datatype;

  // Determine the actual message tag we will use for the send, and which
  // communicator we will use.
  std::pair<boost::mpi::communicator, int> actual
    = pg.actual_communicator_and_tag(tag, block);

#ifdef SEND_OOB_BSEND
  if (mpi_process_group::message_buffer_size()) {
    MPI_Bsend(const_cast<T*>(&value), 1, get_mpi_datatype<T>(value), dest, 
              actual.second, actual.first);
    return;
  }
#endif
  MPI_Request request;
  MPI_Isend(const_cast<T*>(&value), 1, get_mpi_datatype<T>(value), dest, 
            actual.second, actual.first, &request);
  
  int done=0;
  do {
    pg.poll();
    MPI_Test(&request,&done,MPI_STATUS_IGNORE);
  } while (!done);
}

template<typename T>
typename disable_if<boost::mpi::is_mpi_datatype<T> >::type
send_oob(const mpi_process_group& pg, mpi_process_group::process_id_type dest,
         int tag, const T& value, int block=-1)
{
  using boost::mpi::packed_oarchive;

  // Determine the actual message tag we will use for the send, and which
  // communicator we will use.
  std::pair<boost::mpi::communicator, int> actual
    = pg.actual_communicator_and_tag(tag, block);

  // Serialize the data into a buffer
  packed_oarchive out(actual.first);
  out << value;
  std::size_t size = out.size();

  // Send the actual message data
#ifdef SEND_OOB_BSEND
  if (mpi_process_group::message_buffer_size()) {
    MPI_Bsend(const_cast<void*>(out.address()), size, MPI_PACKED,
            dest, actual.second, actual.first);
   return;
  }
#endif
  MPI_Request request;
  MPI_Isend(const_cast<void*>(out.address()), size, MPI_PACKED,
            dest, actual.second, actual.first, &request);

  int done=0;
  do {
    pg.poll();
    MPI_Test(&request,&done,MPI_STATUS_IGNORE);
  } while (!done);
}

template<typename T>
typename enable_if<boost::mpi::is_mpi_datatype<T> >::type
receive_oob(const mpi_process_group& pg, 
            mpi_process_group::process_id_type source, int tag, T& value, int block=-1);

template<typename T>
typename disable_if<boost::mpi::is_mpi_datatype<T> >::type
receive_oob(const mpi_process_group& pg, 
            mpi_process_group::process_id_type source, int tag, T& value, int block=-1);

template<typename SendT, typename ReplyT>
typename enable_if<boost::mpi::is_mpi_datatype<ReplyT> >::type
send_oob_with_reply(const mpi_process_group& pg, 
                    mpi_process_group::process_id_type dest,
                    int tag, const SendT& send_value, ReplyT& reply_value,
                    int block = -1);

template<typename SendT, typename ReplyT>
typename disable_if<boost::mpi::is_mpi_datatype<ReplyT> >::type
send_oob_with_reply(const mpi_process_group& pg, 
                    mpi_process_group::process_id_type dest,
                    int tag, const SendT& send_value, ReplyT& reply_value,
                    int block = -1);

} } } // end namespace boost::graph::distributed

BOOST_IS_BITWISE_SERIALIZABLE(boost::graph::distributed::mpi_process_group::message_header)
namespace boost { namespace mpi {
    template<>
    struct is_mpi_datatype<boost::graph::distributed::mpi_process_group::message_header> : mpl::true_ { };
} } // end namespace boost::mpi

BOOST_CLASS_IMPLEMENTATION(boost::graph::distributed::mpi_process_group::outgoing_messages,object_serializable)
BOOST_CLASS_TRACKING(boost::graph::distributed::mpi_process_group::outgoing_messages,track_never)

#include <boost/graph/distributed/detail/mpi_process_group.ipp>

#endif // BOOST_PARALLEL_MPI_MPI_PROCESS_GROUP_HPP
