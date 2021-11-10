// Copyright (C) 2005, 2006 Douglas Gregor <doug.gregor -at- gmail.com>.
// Copyright (C) 2016 K. Noel Belcourt <kbelco -at- sandia.gov>.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** @file communicator.hpp
 *
 *  This header defines the @c communicator class, which is the basis
 *  of all communication within Boost.MPI, and provides point-to-point
 *  communication operations.
 */
#ifndef BOOST_MPI_COMMUNICATOR_HPP
#define BOOST_MPI_COMMUNICATOR_HPP

#include <boost/assert.hpp>
#include <boost/mpi/config.hpp>
#include <boost/mpi/exception.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/mpi/datatype.hpp>
#include <boost/mpi/nonblocking.hpp>
#include <boost/static_assert.hpp>
#include <utility>
#include <iterator>
#include <stdexcept> // for std::range_error
#include <vector>

// For (de-)serializing sends and receives
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/mpi/packed_iarchive.hpp>

// For (de-)serializing skeletons and content
#include <boost/mpi/skeleton_and_content_fwd.hpp>

#include <boost/mpi/detail/point_to_point.hpp>
#include <boost/mpi/status.hpp>
#include <boost/mpi/request.hpp>

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4800) // forcing to bool 'true' or 'false'
#endif

namespace boost { namespace mpi {

/**
 * @brief A constant representing "any process."
 *
 * This constant may be used for the @c source parameter of @c receive
 * operations to indicate that a message may be received from any
 * source.
 */
const int any_source = MPI_ANY_SOURCE;

/**
 * @brief A constant representing "any tag."
 *
 * This constant may be used for the @c tag parameter of @c receive
 * operations to indicate that a @c send with any tag will be matched
 * by the receive.
 */
const int any_tag = MPI_ANY_TAG;

/**
 * @brief Enumeration used to describe how to adopt a C @c MPI_Comm into
 * a Boost.MPI communicator.
 *
 * The values for this enumeration determine how a Boost.MPI
 * communicator will behave when constructed with an MPI
 * communicator. The options are:
 *
 *   - @c comm_duplicate: Duplicate the MPI_Comm communicator to
 *   create a new communicator (e.g., with MPI_Comm_dup). This new
 *   MPI_Comm communicator will be automatically freed when the
 *   Boost.MPI communicator (and all copies of it) is destroyed.
 *
 *   - @c comm_take_ownership: Take ownership of the communicator. It
 *   will be freed automatically when all of the Boost.MPI
 *   communicators go out of scope. This option must not be used with
 *   MPI_COMM_WORLD.
 *
 *   - @c comm_attach: The Boost.MPI communicator will reference the
 *   existing MPI communicator but will not free it when the Boost.MPI
 *   communicator goes out of scope. This option should only be used
 *   when the communicator is managed by the user or MPI library
 *   (e.g., MPI_COMM_WORLD).
 */
enum comm_create_kind { comm_duplicate, comm_take_ownership, comm_attach };

/**
 * INTERNAL ONLY
 * 
 * Forward declaration of @c group needed for the @c group
 * constructor and accessor.
 */
class group;

/**
 * INTERNAL ONLY
 *
 * Forward declaration of @c intercommunicator needed for the "cast"
 * from a communicator to an intercommunicator.
 */
class intercommunicator;

/**
 * INTERNAL ONLY
 *
 * Forward declaration of @c graph_communicator needed for the "cast"
 * from a communicator to a graph communicator.
 */
class graph_communicator;

/**
 * INTERNAL ONLY
 *
 * Forward declaration of @c cartesian_communicator needed for the "cast"
 * from a communicator to a cartesian communicator.
 */
class cartesian_communicator;

/**
 * @brief A communicator that permits communication and
 * synchronization among a set of processes.
 *
 * The @c communicator class abstracts a set of communicating
 * processes in MPI. All of the processes that belong to a certain
 * communicator can determine the size of the communicator, their rank
 * within the communicator, and communicate with any other processes
 * in the communicator.
 */
class BOOST_MPI_DECL communicator
{
 public:
  /**
   * Build a new Boost.MPI communicator for @c MPI_COMM_WORLD.
   *
   * Constructs a Boost.MPI communicator that attaches to @c
   * MPI_COMM_WORLD. This is the equivalent of constructing with
   * @c (MPI_COMM_WORLD, comm_attach).
   */
  communicator();

  /**
   * Build a new Boost.MPI communicator based on the MPI communicator
   * @p comm.
   *
   * @p comm may be any valid MPI communicator. If @p comm is
   * MPI_COMM_NULL, an empty communicator (that cannot be used for
   * communication) is created and the @p kind parameter is
   * ignored. Otherwise, the @p kind parameters determines how the
   * Boost.MPI communicator will be related to @p comm:
   *
   *   - If @p kind is @c comm_duplicate, duplicate @c comm to create
   *   a new communicator. This new communicator will be freed when
   *   the Boost.MPI communicator (and all copies of it) is destroyed.
   *   This option is only permitted if @p comm is a valid MPI
   *   intracommunicator or if the underlying MPI implementation
   *   supports MPI 2.0 (which supports duplication of
   *   intercommunicators).
   *
   *   - If @p kind is @c comm_take_ownership, take ownership of @c
   *   comm. It will be freed automatically when all of the Boost.MPI
   *   communicators go out of scope. This option must not be used
   *   when @c comm is MPI_COMM_WORLD.
   *
   *   - If @p kind is @c comm_attach, this Boost.MPI communicator
   *   will reference the existing MPI communicator @p comm but will
   *   not free @p comm when the Boost.MPI communicator goes out of
   *   scope. This option should only be used when the communicator is
   *   managed by the user or MPI library (e.g., MPI_COMM_WORLD).
   */
  communicator(const MPI_Comm& comm, comm_create_kind kind);

  /**
   * Build a new Boost.MPI communicator based on a subgroup of another
   * MPI communicator.
   *
   * This routine will construct a new communicator containing all of
   * the processes from communicator @c comm that are listed within
   * the group @c subgroup. Equivalent to @c MPI_Comm_create.
   *
   * @param comm An MPI communicator.
   *
   * @param subgroup A subgroup of the MPI communicator, @p comm, for
   * which we will construct a new communicator.
   */
  communicator(const communicator& comm, const boost::mpi::group& subgroup);

  /**
   * @brief Determine the rank of the executing process in a
   * communicator.
   *
   * This routine is equivalent to @c MPI_Comm_rank.
   *
   *   @returns The rank of the process in the communicator, which
   *   will be a value in [0, size())
   */
  int rank() const;

  /**
   * @brief Determine the number of processes in a communicator.
   *
   * This routine is equivalent to @c MPI_Comm_size.
   *
   *   @returns The number of processes in the communicator.
   */
  int size() const;

  /**
   * This routine constructs a new group whose members are the
   * processes within this communicator. Equivalent to
   * calling @c MPI_Comm_group.
   */
  boost::mpi::group group() const;

  // ----------------------------------------------------------------
  // Point-to-point communication
  // ----------------------------------------------------------------

  /**
   *  @brief Send data to another process.
   *
   *  This routine executes a potentially blocking send with tag @p tag
   *  to the process with rank @p dest. It can be received by the
   *  destination process with a matching @c recv call.
   *
   *  The given @p value must be suitable for transmission over
   *  MPI. There are several classes of types that meet these
   *  requirements:
   *
   *    - Types with mappings to MPI data types: If @c
   *    is_mpi_datatype<T> is convertible to @c mpl::true_, then @p
   *    value will be transmitted using the MPI data type
   *    @c get_mpi_datatype<T>(). All primitive C++ data types that have
   *    MPI equivalents, e.g., @c int, @c float, @c char, @c double,
   *    etc., have built-in mappings to MPI data types. You may turn a
   *    Serializable type with fixed structure into an MPI data type by
   *    specializing @c is_mpi_datatype for your type.
   *
   *    - Serializable types: Any type that provides the @c serialize()
   *    functionality required by the Boost.Serialization library can be
   *    transmitted and received.
   *
   *    - Packed archives and skeletons: Data that has been packed into
   *    an @c mpi::packed_oarchive or the skeletons of data that have
   *    been backed into an @c mpi::packed_skeleton_oarchive can be
   *    transmitted, but will be received as @c mpi::packed_iarchive and
   *    @c mpi::packed_skeleton_iarchive, respectively, to allow the
   *    values (or skeletons) to be extracted by the destination process.
   *
   *    - Content: Content associated with a previously-transmitted
   *    skeleton can be transmitted by @c send and received by @c
   *    recv. The receiving process may only receive content into the
   *    content of a value that has been constructed with the matching
   *    skeleton.
   *
   *  For types that have mappings to an MPI data type (including the
   *  concent of a type), an invocation of this routine will result in
   *  a single MPI_Send call. For variable-length data, e.g.,
   *  serialized types and packed archives, two messages will be sent
   *  via MPI_Send: one containing the length of the data and the
   *  second containing the data itself.
   * 
   *  Std::vectors of MPI data type
   *  are considered variable size, e.g. their number of elements is 
   *  unknown and must be transmited (although the serialization process
   *  is skipped). You can use the array specialized versions of 
   *  communication methods is both sender and receiver know the vector 
   *  size.
   *  
   *  Note that the transmission mode for variable-length data is an 
   *  implementation detail that is subject to change.
   *
   *  @param dest The rank of the remote process to which the data
   *  will be sent.
   *
   *  @param tag The tag that will be associated with this message. Tags
   *  may be any integer between zero and an implementation-defined
   *  upper limit. This limit is accessible via @c environment::max_tag().
   *
   *  @param value The value that will be transmitted to the
   *  receiver. The type @c T of this value must meet the aforementioned
   *  criteria for transmission. 
   */
  template<typename T>
  void send(int dest, int tag, const T& value) const;

  template<typename T, typename A>
  void send(int dest, int tag, const std::vector<T,A>& value) const;

  /**
   *  @brief Send the skeleton of an object.
   *
   *  This routine executes a potentially blocking send with tag @p
   *  tag to the process with rank @p dest. It can be received by the
   *  destination process with a matching @c recv call. This variation
   *  on @c send will be used when a send of a skeleton is explicitly
   *  requested via code such as:
   *
   *  @code
   *    comm.send(dest, tag, skeleton(object));
   *  @endcode
   *
   *  The semantics of this routine are equivalent to that of sending
   *  a @c packed_skeleton_oarchive storing the skeleton of the @c
   *  object.
   *
   *  @param dest The rank of the remote process to which the skeleton
   *  will be sent.
   *
   *  @param tag The tag that will be associated with this message. Tags
   *  may be any integer between zero and an implementation-defined
   *  upper limit. This limit is accessible via @c environment::max_tag().
   *
   *  @param proxy The @c skeleton_proxy containing a reference to the
   *  object whose skeleton will be transmitted.
   *
   */
  template<typename T>
  void send(int dest, int tag, const skeleton_proxy<T>& proxy) const;

  /**
   *  @brief Send an array of values to another process.
   *
   *  This routine executes a potentially blocking send of an array of
   *  data with tag @p tag to the process with rank @p dest. It can be
   *  received by the destination process with a matching array @c
   *  recv call.
   *
   *  If @c T is an MPI datatype, an invocation of this routine will
   *  be mapped to a single call to MPI_Send, using the datatype @c
   *  get_mpi_datatype<T>().
   *
   *  @param dest The process rank of the remote process to which
   *  the data will be sent.
   *
   *  @param tag The tag that will be associated with this message. Tags
   *  may be any integer between zero and an implementation-defined
   *  upper limit. This limit is accessible via @c environment::max_tag().
   *
   *  @param values The array of values that will be transmitted to the
   *  receiver. The type @c T of these values must be mapped to an MPI
   *  data type.
   *
   *  @param n The number of values stored in the array. The destination
   *  process must call receive with at least this many elements to
   *  correctly receive the message.
   */
  template<typename T>
  void send(int dest, int tag, const T* values, int n) const;

  /**
   *  @brief Send a message to another process without any data.
   *
   *  This routine executes a potentially blocking send of a message
   *  to another process. The message contains no extra data, and can
   *  therefore only be received by a matching call to @c recv().
   *
   *  @param dest The process rank of the remote process to which
   *  the message will be sent.
   *
   *  @param tag The tag that will be associated with this message. Tags
   *  may be any integer between zero and an implementation-defined
   *  upper limit. This limit is accessible via @c environment::max_tag().
   *
   */
  void send(int dest, int tag) const;

  /**
   * @brief Receive data from a remote process.
   *
   * This routine blocks until it receives a message from the process @p
   * source with the given @p tag. The type @c T of the @p value must be
   * suitable for transmission over MPI, which includes serializable
   * types, types that can be mapped to MPI data types (including most
   * built-in C++ types), packed MPI archives, skeletons, and content
   * associated with skeletons; see the documentation of @c send for a
   * complete description.
   *
   *   @param source The process that will be sending data. This will
   *   either be a process rank within the communicator or the
   *   constant @c any_source, indicating that we can receive the
   *   message from any process.
   *
   *   @param tag The tag that matches a particular kind of message sent
   *   by the source process. This may be any tag value permitted by @c
   *   send. Alternatively, the argument may be the constant @c any_tag,
   *   indicating that this receive matches a message with any tag.
   *
   *   @param value Will contain the value of the message after a
   *   successful receive. The type of this value must match the value
   *   transmitted by the sender, unless the sender transmitted a packed
   *   archive or skeleton: in these cases, the sender transmits a @c
   *   packed_oarchive or @c packed_skeleton_oarchive and the
   *   destination receives a @c packed_iarchive or @c
   *   packed_skeleton_iarchive, respectively.
   *
   *   @returns Information about the received message.
   */
  template<typename T>
  status recv(int source, int tag, T& value) const;

  template<typename T, typename A>
  status recv(int source, int tag, std::vector<T,A>& value) const;

  /**
   *  @brief Receive a skeleton from a remote process.
   *
   *  This routine blocks until it receives a message from the process @p
   *  source with the given @p tag containing a skeleton.
   *
   *  @param source The process that will be sending data. This will
   *  either be a process rank within the communicator or the constant
   *  @c any_source, indicating that we can receive the message from
   *  any process.
   *
   *  @param tag The tag that matches a particular kind of message
   *  sent by the source process. This may be any tag value permitted
   *  by @c send. Alternatively, the argument may be the constant @c
   *  any_tag, indicating that this receive matches a message with any
   *  tag.
   *
   *  @param proxy The @c skeleton_proxy containing a reference to the
   *  object that will be reshaped to match the received skeleton.
   *
   *  @returns Information about the received message.
   */
  template<typename T>
  status recv(int source, int tag, const skeleton_proxy<T>& proxy) const;

  /**
   *  @brief Receive a skeleton from a remote process.
   *
   *  This routine blocks until it receives a message from the process @p
   *  source with the given @p tag containing a skeleton.
   *
   *  @param source The process that will be sending data. This will
   *  either be a process rank within the communicator or the constant
   *  @c any_source, indicating that we can receive the message from
   *  any process.
   *
   *  @param tag The tag that matches a particular kind of message
   *  sent by the source process. This may be any tag value permitted
   *  by @c send. Alternatively, the argument may be the constant @c
   *  any_tag, indicating that this receive matches a message with any
   *  tag.
   *
   *  @param proxy The @c skeleton_proxy containing a reference to the
   *  object that will be reshaped to match the received skeleton.
   *
   *  @returns Information about the received message.
   */
  template<typename T>
  status recv(int source, int tag, skeleton_proxy<T>& proxy) const;

  /**
   * @brief Receive an array of values from a remote process.
   *
   * This routine blocks until it receives an array of values from the
   * process @p source with the given @p tag. If the type @c T is 
   *
   *   @param source The process that will be sending data. This will
   *   either be a process rank within the communicator or the
   *   constant @c any_source, indicating that we can receive the
   *   message from any process.
   *
   *   @param tag The tag that matches a particular kind of message sent
   *   by the source process. This may be any tag value permitted by @c
   *   send. Alternatively, the argument may be the constant @c any_tag,
   *   indicating that this receive matches a message with any tag.
   *
   *   @param values Will contain the values in the message after a
   *   successful receive. The type of these elements must match the
   *   type of the elements transmitted by the sender.
   *
   *   @param n The number of values that can be stored into the @p
   *   values array. This shall not be smaller than the number of
   *   elements transmitted by the sender.
   *
   *   @throws std::range_error if the message to be received contains
   *   more than @p n values.
   *
   *   @returns Information about the received message.
   */
  template<typename T>
  status recv(int source, int tag, T* values, int n) const;

  /**
   *  @brief Receive a message from a remote process without any data.
   *
   *  This routine blocks until it receives a message from the process
   *  @p source with the given @p tag.
   *
   *  @param source The process that will be sending the message. This
   *  will either be a process rank within the communicator or the
   *  constant @c any_source, indicating that we can receive the
   *  message from any process.
   *
   *  @param tag The tag that matches a particular kind of message
   *  sent by the source process. This may be any tag value permitted
   *  by @c send. Alternatively, the argument may be the constant @c
   *  any_tag, indicating that this receive matches a message with any
   *  tag.
   *
   *  @returns Information about the received message.
   */
  status recv(int source, int tag) const;

  /** @brief Send a message to remote process and receive another message
   *  from another process.
   */
  template<typename T>
  status sendrecv(int dest, int stag, const T& sval, int src, int rtag, T& rval) const;
  
  /**
   *  @brief Send a message to a remote process without blocking.
   *
   *  The @c isend method is functionality identical to the @c send
   *  method and transmits data in the same way, except that @c isend
   *  will not block while waiting for the data to be
   *  transmitted. Instead, a request object will be immediately
   *  returned, allowing one to query the status of the communication
   *  or wait until it has completed.
   *
   *  @param dest The rank of the remote process to which the data
   *  will be sent.
   *
   *  @param tag The tag that will be associated with this message. Tags
   *  may be any integer between zero and an implementation-defined
   *  upper limit. This limit is accessible via @c environment::max_tag().
   *
   *  @param value The value that will be transmitted to the
   *  receiver. The type @c T of this value must meet the aforementioned
   *  criteria for transmission. If modified before transmited, the 
   *  modification may or may not be transmited.
   *
   *  @returns a @c request object that describes this communication.
   */
  template<typename T>
  request isend(int dest, int tag, const T& value) const;

  /**
   *  @brief Send the skeleton of an object without blocking.
   *
   *  This routine is functionally identical to the @c send method for
   *  @c skeleton_proxy objects except that @c isend will not block
   *  while waiting for the data to be transmitted. Instead, a request
   *  object will be immediately returned, allowing one to query the
   *  status of the communication or wait until it has completed.  
   *
   *  The semantics of this routine are equivalent to a non-blocking
   *  send of a @c packed_skeleton_oarchive storing the skeleton of
   *  the @c object.
   *
   *  @param dest The rank of the remote process to which the skeleton
   *  will be sent.
   *
   *  @param tag The tag that will be associated with this message. Tags
   *  may be any integer between zero and an implementation-defined
   *  upper limit. This limit is accessible via @c environment::max_tag().
   *
   *  @param proxy The @c skeleton_proxy containing a reference to the
   *  object whose skeleton will be transmitted.
   *
   *  @returns a @c request object that describes this communication.
   */
  template<typename T>
  request isend(int dest, int tag, const skeleton_proxy<T>& proxy) const;

  /**
   *  @brief Send an array of values to another process without
   *  blocking.
   *
   *  This routine is functionally identical to the @c send method for
   *  arrays except that @c isend will not block while waiting for the
   *  data to be transmitted. Instead, a request object will be
   *  immediately returned, allowing one to query the status of the
   *  communication or wait until it has completed.
   *
   *  @param dest The process rank of the remote process to which
   *  the data will be sent.
   *
   *  @param tag The tag that will be associated with this message. Tags
   *  may be any integer between zero and an implementation-defined
   *  upper limit. This limit is accessible via @c environment::max_tag().
   *
   *  @param values The array of values that will be transmitted to the
   *  receiver. The type @c T of these values must be mapped to an MPI
   *  data type.
   *
   *  @param n The number of values stored in the array. The destination
   *  process must call receive with at least this many elements to
   *  correctly receive the message.
   *
   *  @returns a @c request object that describes this communication.
   */
  template<typename T>
  request isend(int dest, int tag, const T* values, int n) const;

  template<typename T, class A>
  request isend(int dest, int tag, const std::vector<T,A>& values) const;

  /**
   *  @brief Send a message to another process without any data
   *  without blocking.
   *
   *  This routine is functionally identical to the @c send method for
   *  sends with no data, except that @c isend will not block while
   *  waiting for the message to be transmitted. Instead, a request
   *  object will be immediately returned, allowing one to query the
   *  status of the communication or wait until it has completed.
   *
   *  @param dest The process rank of the remote process to which
   *  the message will be sent.
   *
   *  @param tag The tag that will be associated with this message. Tags
   *  may be any integer between zero and an implementation-defined
   *  upper limit. This limit is accessible via @c environment::max_tag().
   *
   *
   *  @returns a @c request object that describes this communication.
   */
  request isend(int dest, int tag) const;

  /**
   *  @brief Prepare to receive a message from a remote process.
   *
   *  The @c irecv method is functionally identical to the @c recv
   *  method and receive data in the same way, except that @c irecv
   *  will not block while waiting for data to be
   *  transmitted. Instead, it immediately returns a request object
   *  that allows one to query the status of the receive or wait until
   *  it has completed.
   *
   *   @param source The process that will be sending data. This will
   *   either be a process rank within the communicator or the
   *   constant @c any_source, indicating that we can receive the
   *   message from any process.
   *
   *   @param tag The tag that matches a particular kind of message sent
   *   by the source process. This may be any tag value permitted by @c
   *   send. Alternatively, the argument may be the constant @c any_tag,
   *   indicating that this receive matches a message with any tag.
   *
   *   @param value Will contain the value of the message after a
   *   successful receive. The type of this value must match the value
   *   transmitted by the sender, unless the sender transmitted a packed
   *   archive or skeleton: in these cases, the sender transmits a @c
   *   packed_oarchive or @c packed_skeleton_oarchive and the
   *   destination receives a @c packed_iarchive or @c
   *   packed_skeleton_iarchive, respectively.
   *
   *   @returns a @c request object that describes this communication.
   */
  template<typename T>
  request irecv(int source, int tag, T& value) const;

  /**
   * @brief Initiate receipt of an array of values from a remote process.
   *
   * This routine initiates a receive operation for an array of values
   * transmitted by process @p source with the given @p tag. 
   *
   *    @param source The process that will be sending data. This will
   *    either be a process rank within the communicator or the
   *    constant @c any_source, indicating that we can receive the
   *    message from any process.
   *
   *    @param tag The tag that matches a particular kind of message sent
   *    by the source process. This may be any tag value permitted by @c
   *    send. Alternatively, the argument may be the constant @c any_tag,
   *    indicating that this receive matches a message with any tag.
   *
   *    @param values Will contain the values in the message after a
   *    successful receive. The type of these elements must match the
   *    type of the elements transmitted by the sender.
   *
   *    @param n The number of values that can be stored into the @p
   *    values array. This shall not be smaller than the number of
   *    elements transmitted by the sender.
   *
   *    @returns a @c request object that describes this communication.
   */
  template<typename T>
  request irecv(int source, int tag, T* values, int n) const;

  template<typename T, typename A>
  request irecv(int source, int tag, std::vector<T,A>& values) const;

  /**
   *  @brief Initiate receipt of a message from a remote process that
   *  carries no data.
   *
   *  This routine initiates a receive operation for a message from
   *  process @p source with the given @p tag that carries no data.
   *
   *    @param source The process that will be sending the message. This
   *    will either be a process rank within the communicator or the
   *    constant @c any_source, indicating that we can receive the
   *    message from any process.
   *
   *    @param tag The tag that matches a particular kind of message
   *    sent by the source process. This may be any tag value permitted
   *    by @c send. Alternatively, the argument may be the constant @c
   *    any_tag, indicating that this receive matches a message with any
   *    tag.
   *
   *    @returns a @c request object that describes this communication.
   */
  request irecv(int source, int tag) const;

  /**
   * @brief Waits until a message is available to be received.
   *
   * This operation waits until a message matching (@p source, @p tag)
   * is available to be received. It then returns information about
   * that message. The functionality is equivalent to @c MPI_Probe. To
   * check if a message is available without blocking, use @c iprobe.
   *
   *   @param source Determine if there is a message available from
   *   this rank. If @c any_source, then the message returned may come
   *   from any source.
   *
   *   @param tag Determine if there is a message available with the
   *   given tag. If @c any_tag, then the message returned may have any
   *   tag.
   *
   *   @returns Returns information about the first message that
   *   matches the given criteria.
   */
  status probe(int source = any_source, int tag = any_tag) const;

  /**
   * @brief Determine if a message is available to be received.
   *
   * This operation determines if a message matching (@p source, @p
   * tag) is available to be received. If so, it returns information
   * about that message; otherwise, it returns immediately with an
   * empty optional. The functionality is equivalent to @c
   * MPI_Iprobe. To wait until a message is available, use @c wait.
   *
   *   @param source Determine if there is a message available from
   *   this rank. If @c any_source, then the message returned may come
   *   from any source.
   *
   *   @param tag Determine if there is a message available with the
   *   given tag. If @c any_tag, then the message returned may have any
   *   tag.
   *
   *   @returns If a matching message is available, returns
   *   information about that message. Otherwise, returns an empty
   *   @c boost::optional.
   */
  optional<status>
  iprobe(int source = any_source, int tag = any_tag) const;

#ifdef barrier
  // Linux defines a function-like macro named "barrier". So, we need
  // to avoid expanding the macro when we define our barrier()
  // function. However, some C++ parsers (Doxygen, for instance) can't
  // handle this syntax, so we only use it when necessary.
  void (barrier)() const;
#else
  /**
   * @brief Wait for all processes within a communicator to reach the
   * barrier.
   *
   * This routine is a collective operation that blocks each process
   * until all processes have entered it, then releases all of the
   * processes "simultaneously". It is equivalent to @c MPI_Barrier.
   */
  void barrier() const;
#endif

  /** @brief Determine if this communicator is valid for
   * communication.
   *
   * Evaluates @c true in a boolean context if this communicator is
   * valid for communication, i.e., does not represent
   * MPI_COMM_NULL. Otherwise, evaluates @c false.
   */
  operator bool() const { return (bool)comm_ptr; }

  /**
   * @brief Access the MPI communicator associated with a Boost.MPI
   * communicator.
   *
   * This routine permits the implicit conversion from a Boost.MPI
   * communicator to an MPI communicator.
   *
   *   @returns The associated MPI communicator.
   */
  operator MPI_Comm() const;

  /**
   * Split the communicator into multiple, disjoint communicators
   * each of which is based on a particular color. This is a
   * collective operation that returns a new communicator that is a
   * subgroup of @p this.
   *
   *   @param color The color of this process. All processes with the
   *   same @p color value will be placed into the same group.
   *
   *   @param key A key value that will be used to determine the
   *   ordering of processes with the same color in the resulting
   *   communicator. If omitted, the rank of the processes in @p this
   *   will determine the ordering of processes in the resulting
   *   group.
   *
   *   @returns A new communicator containing all of the processes in
   *   @p this that have the same @p color.
   */
  communicator split(int color, int key) const;
  communicator split(int color) const;

  /**
   * Determine if the communicator is in fact an intercommunicator
   * and, if so, return that intercommunicator.
   *
   * @returns an @c optional containing the intercommunicator, if this
   * communicator is in fact an intercommunicator. Otherwise, returns
   * an empty @c optional.
   */
  optional<intercommunicator> as_intercommunicator() const;

  /**
   * Determine if the communicator has a graph topology and, if so,
   * return that @c graph_communicator. Even though the communicators
   * have different types, they refer to the same underlying
   * communication space and can be used interchangeably for
   * communication.
   *
   * @returns an @c optional containing the graph communicator, if this
   * communicator does in fact have a graph topology. Otherwise, returns
   * an empty @c optional.
   */
  optional<graph_communicator> as_graph_communicator() const;

  /**
   * Determines whether this communicator has a Graph topology.
   */
  bool has_graph_topology() const;

  /**
   * Determine if the communicator has a cartesian topology and, if so,
   * return that @c cartesian_communicator. Even though the communicators
   * have different types, they refer to the same underlying
   * communication space and can be used interchangeably for
   * communication.
   *
   * @returns an @c optional containing the cartesian communicator, if this
   * communicator does in fact have a cartesian topology. Otherwise, returns
   * an empty @c optional.
   */
  optional<cartesian_communicator> as_cartesian_communicator() const;

  /**
   * Determines whether this communicator has a Cartesian topology.
   */
  bool has_cartesian_topology() const;

  /** Abort all tasks in the group of this communicator.
   *
   *  Makes a "best attempt" to abort all of the tasks in the group of
   *  this communicator. Depending on the underlying MPI
   *  implementation, this may either abort the entire program (and
   *  possibly return @p errcode to the environment) or only abort
   *  some processes, allowing the others to continue. Consult the
   *  documentation for your MPI implementation. This is equivalent to
   *  a call to @c MPI_Abort
   *
   *  @param errcode The error code to return from aborted processes.
   *  @returns Will not return.
   */
  void abort(int errcode) const;

 protected:
  
  /**
   * INTERNAL ONLY
   *
   * Implementation of sendrecv for mpi type.
   */
  template<typename T>
  status sendrecv_impl(int dest, int stag, const T& sval, int src, int rtag, T& rval,
                       mpl::true_) const;

  /**
   * INTERNAL ONLY
   *
   * Implementation of sendrecv for complex types, which must be passed as archives.
   */
  template<typename T>
  status sendrecv_impl(int dest, int stag, const T& sval, int src, int rtag, T& rval,
                       mpl::false_) const;

  /**
   * INTERNAL ONLY
   *
   * Function object that frees an MPI communicator and deletes the
   * memory associated with it. Intended to be used as a deleter with
   * shared_ptr.
   */
  struct comm_free
  {
    void operator()(MPI_Comm* comm) const
    {
      BOOST_ASSERT( comm != 0 );
      BOOST_ASSERT(*comm != MPI_COMM_NULL);
      int finalized;
      BOOST_MPI_CHECK_RESULT(MPI_Finalized, (&finalized));
      if (!finalized)
        BOOST_MPI_CHECK_RESULT(MPI_Comm_free, (comm));
      delete comm;
    }
  };

  
  /**
   * INTERNAL ONLY
   *
   * We're sending a type that has an associated MPI datatype, so we
   * map directly to that datatype.
   */
  template<typename T>
  void send_impl(int dest, int tag, const T& value, mpl::true_) const;

  /**
   * INTERNAL ONLY
   *
   * We're sending a type that does not have an associated MPI
   * datatype, so it must be serialized then sent as MPI_PACKED data,
   * to be deserialized on the receiver side.
   */
  template<typename T>
  void send_impl(int dest, int tag, const T& value, mpl::false_) const;

  /**
   * INTERNAL ONLY
   *
   * We're sending an array of a type that has an associated MPI
   * datatype, so we map directly to that datatype.
   */
  template<typename T>
  void 
  array_send_impl(int dest, int tag, const T* values, int n, mpl::true_) const;

  /**
   * INTERNAL ONLY
   *
   * We're sending an array of a type that does not have an associated
   * MPI datatype, so it must be serialized then sent as MPI_PACKED
   * data, to be deserialized on the receiver side.
   */
  template<typename T>
  void 
  array_send_impl(int dest, int tag, const T* values, int n, 
                  mpl::false_) const;

  /**
   * INTERNAL ONLY
   *
   * We're sending a type that has an associated MPI datatype, so we
   * map directly to that datatype.
   */
  template<typename T>
  request isend_impl(int dest, int tag, const T& value, mpl::true_) const;

  /**
   * INTERNAL ONLY
   *
   * We're sending a type that does not have an associated MPI
   * datatype, so it must be serialized then sent as MPI_PACKED data,
   * to be deserialized on the receiver side.
   */
  template<typename T>
  request isend_impl(int dest, int tag, const T& value, mpl::false_) const;

  /**
   * INTERNAL ONLY
   *
   * We're sending an array of a type that has an associated MPI
   * datatype, so we map directly to that datatype.
   */
  template<typename T>
  request 
  array_isend_impl(int dest, int tag, const T* values, int n, 
                   mpl::true_) const;

  /**
   * INTERNAL ONLY
   *
   * We're sending an array of a type that does not have an associated
   * MPI datatype, so it must be serialized then sent as MPI_PACKED
   * data, to be deserialized on the receiver side.
   */
  template<typename T>
  request 
  array_isend_impl(int dest, int tag, const T* values, int n, 
                   mpl::false_) const;

  /**
   * INTERNAL ONLY
   *
   * We're receiving a type that has an associated MPI datatype, so we
   * map directly to that datatype.
   */
  template<typename T>
  status recv_impl(int source, int tag, T& value, mpl::true_) const;

  /**
   * INTERNAL ONLY
   *
   * We're receiving a type that does not have an associated MPI
   * datatype, so it must have been serialized then sent as
   * MPI_PACKED. We'll receive it and then deserialize.
   */
  template<typename T>
  status recv_impl(int source, int tag, T& value, mpl::false_) const;

  /**
   * INTERNAL ONLY
   *
   * We're receiving an array of a type that has an associated MPI
   * datatype, so we map directly to that datatype.
   */
  template<typename T>
  status 
  array_recv_impl(int source, int tag, T* values, int n, mpl::true_) const;

  /**
   * INTERNAL ONLY
   *
   * We're receiving a type that does not have an associated MPI
   * datatype, so it must have been serialized then sent as
   * MPI_PACKED. We'll receive it and then deserialize.
   */
  template<typename T>
  status 
  array_recv_impl(int source, int tag, T* values, int n, mpl::false_) const;

  /**
   * INTERNAL ONLY
   *
   * We're receiving a type that has an associated MPI datatype, so we
   * map directly to that datatype.
   */
  template<typename T>
  request irecv_impl(int source, int tag, T& value, mpl::true_) const;

  /**
   * INTERNAL ONLY
   *
   * We're receiving a type that does not have an associated MPI
   * datatype, so it must have been serialized then sent as
   * MPI_PACKED. We'll receive it and then deserialize.
   */
  template<typename T>
  request irecv_impl(int source, int tag, T& value, mpl::false_) const;

  /**
   * INTERNAL ONLY
   *
   * We're receiving a type that has an associated MPI datatype, so we
   * map directly to that datatype.
   */
  template<typename T>
  request 
  array_irecv_impl(int source, int tag, T* values, int n, mpl::true_) const;

  /**
   * INTERNAL ONLY
   *
   * We're receiving a type that does not have an associated MPI
   * datatype, so it must have been serialized then sent as
   * MPI_PACKED. We'll receive it and then deserialize.
   */
  template<typename T>
  request 
  array_irecv_impl(int source, int tag, T* values, int n, mpl::false_) const;

  // We're sending/receivig a vector with associated MPI datatype.
  // We need to send/recv the size and then the data and make sure 
  // blocking and non blocking method agrees on the format.
  template<typename T, typename A>
  request irecv_vector(int source, int tag, std::vector<T,A>& values, 
                       mpl::true_) const;
  template<typename T, class A>
  request isend_vector(int dest, int tag, const std::vector<T,A>& values,
                       mpl::true_) const;
  template<typename T, typename A>
  void send_vector(int dest, int tag, const std::vector<T,A>& value, 
		   mpl::true_) const;
  template<typename T, typename A>
  status recv_vector(int source, int tag, std::vector<T,A>& value,
		     mpl::true_) const;
  
  // We're sending/receivig a vector with no associated MPI datatype.
  // We need to send/recv it as an archive and make sure 
  // blocking and non blocking method agrees on the format.
  template<typename T, typename A>
  request irecv_vector(int source, int tag, std::vector<T,A>& values, 
                       mpl::false_) const;
  template<typename T, class A>
  request isend_vector(int dest, int tag, const std::vector<T,A>& values,
                       mpl::false_) const;
  template<typename T, typename A>
  void send_vector(int dest, int tag, const std::vector<T,A>& value, 
		   mpl::false_) const;
  template<typename T, typename A>
  status recv_vector(int source, int tag, std::vector<T,A>& value,
		     mpl::false_) const;

 protected:
  shared_ptr<MPI_Comm> comm_ptr;
};

/**
 * @brief Determines whether two communicators are identical.
 *
 * Equivalent to calling @c MPI_Comm_compare and checking whether the
 * result is @c MPI_IDENT.
 *
 * @returns True when the two communicators refer to the same
 * underlying MPI communicator.
 */
BOOST_MPI_DECL bool operator==(const communicator& comm1, const communicator& comm2);

/**
 * @brief Determines whether two communicators are different.
 *
 * @returns @c !(comm1 == comm2)
 */
inline bool operator!=(const communicator& comm1, const communicator& comm2)
{
  return !(comm1 == comm2);
}

}} // boost::mpi

/************************************************************************
 * Implementation details                                               *
 ************************************************************************/

#include <boost/mpi/detail/request_handlers.hpp>

namespace boost { namespace mpi {
/**
 * INTERNAL ONLY (using the same 'end' name might be considerd unfortunate
 */
template<>
BOOST_MPI_DECL void
communicator::send<packed_oarchive>(int dest, int tag,
                                    const packed_oarchive& ar) const;

/**
 * INTERNAL ONLY
 */
template<>
BOOST_MPI_DECL void
communicator::send<packed_skeleton_oarchive>
  (int dest, int tag, const packed_skeleton_oarchive& ar) const;

/**
 * INTERNAL ONLY
 */
template<>
BOOST_MPI_DECL void
communicator::send<content>(int dest, int tag, const content& c) const;

/**
 * INTERNAL ONLY
 */
template<>
BOOST_MPI_DECL status
communicator::recv<packed_iarchive>(int source, int tag,
                                    packed_iarchive& ar) const;

/**
 * INTERNAL ONLY
 */
template<>
BOOST_MPI_DECL status
communicator::recv<packed_skeleton_iarchive>
  (int source, int tag, packed_skeleton_iarchive& ar) const;

/**
 * INTERNAL ONLY
 */
template<>
BOOST_MPI_DECL status
communicator::recv<const content>(int source, int tag,
                                  const content& c) const;

/**
 * INTERNAL ONLY
 */
template<>
inline status
communicator::recv<content>(int source, int tag,
                                  content& c) const
{
  return recv<const content>(source,tag,c);
}                                  

/**
 * INTERNAL ONLY
 */
template<>
BOOST_MPI_DECL request
communicator::isend<packed_oarchive>(int dest, int tag,
                                     const packed_oarchive& ar) const;

/**
 * INTERNAL ONLY
 */
template<>
BOOST_MPI_DECL request
communicator::isend<packed_skeleton_oarchive>
  (int dest, int tag, const packed_skeleton_oarchive& ar) const;

/**
 * INTERNAL ONLY
 */
template<>
BOOST_MPI_DECL request
communicator::isend<content>(int dest, int tag, const content& c) const;

/**
 * INTERNAL ONLY
 */
template<>
BOOST_MPI_DECL request
communicator::irecv<packed_skeleton_iarchive>
  (int source, int tag, packed_skeleton_iarchive& ar) const;

/**
 * INTERNAL ONLY
 */
template<>
BOOST_MPI_DECL request
communicator::irecv<const content>(int source, int tag,
                                   const content& c) const;

/**
 * INTERNAL ONLY
 */
template<>
inline request
communicator::irecv<content>(int source, int tag,
                             content& c) const
{
  return irecv<const content>(source, tag, c);
}

// We're sending a type that has an associated MPI datatype, so we
// map directly to that datatype.
template<typename T>
void
communicator::send_impl(int dest, int tag, const T& value, mpl::true_) const
{
  // received by recv or trivial handler.
  BOOST_MPI_CHECK_RESULT(MPI_Send,
                         (const_cast<T*>(&value), 1, get_mpi_datatype<T>(value),
                          dest, tag, MPI_Comm(*this)));
}

// We're sending a type that does not have an associated MPI
// datatype, so it must be serialized then sent as MPI_PACKED data,
// to be deserialized on the receiver side.
template<typename T>
void
communicator::send_impl(int dest, int tag, const T& value, mpl::false_) const
{
  packed_oarchive oa(*this);
  oa << value;
  send(dest, tag, oa);
}

// Single-element receive may either send the element directly or
// serialize it via a buffer.
template<typename T>
void communicator::send(int dest, int tag, const T& value) const
{
  this->send_impl(dest, tag, value, is_mpi_datatype<T>());
}

// We're sending an array of a type that has an associated MPI
// datatype, so we map directly to that datatype.
template<typename T>
void
communicator::array_send_impl(int dest, int tag, const T* values, int n,
                              mpl::true_) const
{
  BOOST_MPI_CHECK_RESULT(MPI_Send,
                         (const_cast<T*>(values), n, 
                          get_mpi_datatype<T>(*values),
                          dest, tag, MPI_Comm(*this)));
}

// We're sending an array of a type that does not have an associated
// MPI datatype, so it must be serialized then sent as MPI_PACKED
// data, to be deserialized on the receiver side.
template<typename T>
void
communicator::array_send_impl(int dest, int tag, const T* values, int n,
                              mpl::false_) const
{
  packed_oarchive oa(*this);
  T const* v = values;
  while (v < values+n) {
    oa << *v++;
  }
  send(dest, tag, oa);
}

template<typename T, typename A>
void communicator::send_vector(int dest, int tag, 
  const std::vector<T,A>& values, mpl::true_ primitive) const
{
#if defined(BOOST_MPI_USE_IMPROBE)
  array_send_impl(dest, tag, values.data(), values.size(), primitive);
#else
  {
    // non blocking recv by legacy_dynamic_primitive_array_handler
    // blocking recv by recv_vector(source,tag,value,primitive)
    // send the vector size
    typename std::vector<T,A>::size_type size = values.size();
    send(dest, tag, size);
    // send the data
    this->array_send_impl(dest, tag, values.data(), size, primitive);
  }
#endif
}

template<typename T, typename A>
void communicator::send_vector(int dest, int tag, 
  const std::vector<T,A>& value, mpl::false_ primitive) const
{
  this->send_impl(dest, tag, value, primitive);
}

template<typename T, typename A>
void communicator::send(int dest, int tag, const std::vector<T,A>& value) const
{
  send_vector(dest, tag, value, is_mpi_datatype<T>());
}

// Array send must send the elements directly
template<typename T>
void communicator::send(int dest, int tag, const T* values, int n) const
{
  this->array_send_impl(dest, tag, values, n, is_mpi_datatype<T>());
}

// We're receiving a type that has an associated MPI datatype, so we
// map directly to that datatype.
template<typename T>
status communicator::recv_impl(int source, int tag, T& value, mpl::true_) const
{
  status stat;
  BOOST_MPI_CHECK_RESULT(MPI_Recv,
                         (const_cast<T*>(&value), 1, 
                          get_mpi_datatype<T>(value),
                          source, tag, MPI_Comm(*this), &stat.m_status));
  return stat;
}

template<typename T>
status
communicator::recv_impl(int source, int tag, T& value, mpl::false_) const
{
  // Receive the message
  packed_iarchive ia(*this);
  status stat = recv(source, tag, ia);

  // Deserialize the data in the message
  ia >> value;

  return stat;
}

// Single-element receive may either receive the element directly or
// deserialize it from a buffer.
template<typename T>
status communicator::recv(int source, int tag, T& value) const
{
  return this->recv_impl(source, tag, value, is_mpi_datatype<T>());
}

template<typename T>
status 
communicator::array_recv_impl(int source, int tag, T* values, int n, 
                              mpl::true_) const
{
  status stat;
  BOOST_MPI_CHECK_RESULT(MPI_Recv,
                         (const_cast<T*>(values), n, 
                          get_mpi_datatype<T>(*values),
                          source, tag, MPI_Comm(*this), &stat.m_status));
  return stat;
}

template<typename T>
status
communicator::array_recv_impl(int source, int tag, T* values, int n, 
                              mpl::false_) const
{
  packed_iarchive ia(*this);
  status stat = recv(source, tag, ia);
  T* v = values;
  while (v != values+n) {
    ia >> *v++;
  }
  stat.m_count = n;
  return stat;
}

template<typename T, typename A>
status communicator::recv_vector(int source, int tag, 
                                 std::vector<T,A>& values, mpl::true_ primitive) const
{
#if defined(BOOST_MPI_USE_IMPROBE)
  {
    MPI_Message msg;
    status stat;
    BOOST_MPI_CHECK_RESULT(MPI_Mprobe, (source,tag,*this,&msg,&stat.m_status));
    int count;
    BOOST_MPI_CHECK_RESULT(MPI_Get_count, (&stat.m_status,get_mpi_datatype<T>(),&count));
    values.resize(count);
    BOOST_MPI_CHECK_RESULT(MPI_Mrecv, (values.data(), count, get_mpi_datatype<T>(), &msg, &stat.m_status));
    return stat;
  }
#else
  {
    // receive the vector size
    typename std::vector<T,A>::size_type size = 0;
    recv(source, tag, size);
    // size the vector
    values.resize(size);
    // receive the data
    return this->array_recv_impl(source, tag, values.data(), size, primitive);
  }
#endif
}

template<typename T, typename A>
status communicator::recv_vector(int source, int tag, 
  std::vector<T,A>& value, mpl::false_ false_type) const
{
  return this->recv_impl(source, tag, value, false_type);
}

template<typename T, typename A>
status communicator::recv(int source, int tag, std::vector<T,A>& value) const
{
  return recv_vector(source, tag, value, is_mpi_datatype<T>());
}

// Array receive must receive the elements directly into a buffer.
template<typename T>
status communicator::recv(int source, int tag, T* values, int n) const
{
  return this->array_recv_impl(source, tag, values, n, is_mpi_datatype<T>());
}

 
template<typename T>
status communicator::sendrecv_impl(int dest, int stag, const T& sval, int src, int rtag, T& rval,
                                    mpl::true_) const
{
  status stat;
  BOOST_MPI_CHECK_RESULT(MPI_Sendrecv,
                         (const_cast<T*>(&sval), 1, 
                          get_mpi_datatype<T>(sval),
                          dest, stag, 
                          &rval, 1,
                          get_mpi_datatype<T>(rval),
                          src, rtag,
                          MPI_Comm(*this), &stat.m_status));
  return stat;
}

template<typename T>
status communicator::sendrecv_impl(int dest, int stag, const T& sval, int src, int rtag, T& rval,
                                   mpl::false_) const
{
  int const SEND = 0;
  int const RECV = 1;
  request srrequests[2];
  srrequests[SEND] = this->isend_impl(dest, stag, sval, mpl::false_());
  srrequests[RECV] = this->irecv_impl(src,  rtag, rval, mpl::false_());
  status srstatuses[2];
  wait_all(srrequests, srrequests + 2, srstatuses);
  return srstatuses[RECV];
}

template<typename T>
status communicator::sendrecv(int dest, int stag, const T& sval, int src, int rtag, T& rval) const
{
  return this->sendrecv_impl(dest, stag, sval, src, rtag, rval, is_mpi_datatype<T>());
}


// We're sending a type that has an associated MPI datatype, so we
// map directly to that datatype.
template<typename T>
request
communicator::isend_impl(int dest, int tag, const T& value, mpl::true_) const
{
  return request::make_trivial_send(*this, dest, tag, value);
}

// We're sending a type that does not have an associated MPI
// datatype, so it must be serialized then sent as MPI_PACKED data,
// to be deserialized on the receiver side.
template<typename T>
request
communicator::isend_impl(int dest, int tag, const T& value, mpl::false_) const
{
  shared_ptr<packed_oarchive> archive(new packed_oarchive(*this));
  *archive << value;
  request result = isend(dest, tag, *archive);
  result.preserve(archive);
  return result;
}

// Single-element receive may either send the element directly or
// serialize it via a buffer.
template<typename T>
request communicator::isend(int dest, int tag, const T& value) const
{
  return this->isend_impl(dest, tag, value, is_mpi_datatype<T>());
}

template<typename T, class A>
request communicator::isend(int dest, int tag, const std::vector<T,A>& values) const
{
  return this->isend_vector(dest, tag, values, is_mpi_datatype<T>());
}

template<typename T, class A>
request
communicator::isend_vector(int dest, int tag, const std::vector<T,A>& values,
                           mpl::true_ primitive) const
{
  return request::make_dynamic_primitive_array_send(*this, dest, tag, values);
}

template<typename T, class A>
request
communicator::isend_vector(int dest, int tag, const std::vector<T,A>& values,
                           mpl::false_ no) const 
{
  return this->isend_impl(dest, tag, values, no);
}

template<typename T>
request
communicator::array_isend_impl(int dest, int tag, const T* values, int n,
                               mpl::true_) const
{
  return request::make_trivial_send(*this, dest, tag, values, n);
}

template<typename T>
request
communicator::array_isend_impl(int dest, int tag, const T* values, int n, 
                               mpl::false_) const
{
  shared_ptr<packed_oarchive> archive(new packed_oarchive(*this));
  T const* v = values;
  while (v < values+n) {
    *archive << *v++;
  }
  request result = isend(dest, tag, *archive);
  result.preserve(archive);
  return result;
}


// Array isend must send the elements directly
template<typename T>
request communicator::isend(int dest, int tag, const T* values, int n) const
{
  return array_isend_impl(dest, tag, values, n, is_mpi_datatype<T>());
}

// We're receiving a type that has an associated MPI datatype, so we
// map directly to that datatype.
template<typename T>
request 
communicator::irecv_impl(int source, int tag, T& value, mpl::true_) const
{
  return request::make_trivial_recv(*this, source, tag, value);
}

template<typename T>
request
communicator::irecv_impl(int source, int tag, T& value, mpl::false_) const
{
  return request::make_serialized(*this, source, tag, value);
}

template<typename T>
request 
communicator::irecv(int source, int tag, T& value) const
{
  return this->irecv_impl(source, tag, value, is_mpi_datatype<T>());
}

template<typename T>
request 
communicator::array_irecv_impl(int source, int tag, T* values, int n, 
                               mpl::true_) const
{
  return request::make_trivial_recv(*this, source, tag, values, n);
}

template<typename T>
request
communicator::array_irecv_impl(int source, int tag, T* values, int n, 
                               mpl::false_) const
{
  return request::make_serialized_array(*this, source, tag, values, n);
}

template<typename T, class A>
request
communicator::irecv_vector(int source, int tag, std::vector<T,A>& values, 
                           mpl::true_ primitive) const
{
  return request::make_dynamic_primitive_array_recv(*this, source, tag, values);
}

template<typename T, class A>
request
communicator::irecv_vector(int source, int tag, std::vector<T,A>& values, 
                           mpl::false_ no) const
{
  return irecv_impl(source, tag, values, no);
}

template<typename T, typename A>
request
communicator::irecv(int source, int tag, std::vector<T,A>& values) const
{
  return irecv_vector(source, tag, values, is_mpi_datatype<T>());
}

// Array receive must receive the elements directly into a buffer.
template<typename T>
request communicator::irecv(int source, int tag, T* values, int n) const
{
  return this->array_irecv_impl(source, tag, values, n, is_mpi_datatype<T>());
}

} } // end namespace boost::mpi

// If the user has already included skeleton_and_content.hpp, include
// the code to send/receive skeletons and content.
#ifdef BOOST_MPI_SKELETON_AND_CONTENT_HPP
#  include <boost/mpi/detail/communicator_sc.hpp>
#endif

#ifdef BOOST_MSVC
#  pragma warning(pop)
#endif

#endif // BOOST_MPI_COMMUNICATOR_HPP
