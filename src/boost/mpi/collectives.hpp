// Copyright (C) 2005-2006 Douglas Gregor <doug.gregor -at- gmail.com>.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Message Passing Interface 1.1 -- Section 4. MPI Collectives

/** @file collectives.hpp
 *
 *  This header contains MPI collective operations, which implement
 *  various parallel algorithms that require the coordination of all
 *  processes within a communicator. The header @c collectives_fwd.hpp
 *  provides forward declarations for each of these operations. To
 *  include only specific collective algorithms, use the headers @c
 *  boost/mpi/collectives/algorithm_name.hpp.
 */
#ifndef BOOST_MPI_COLLECTIVES_HPP
#define BOOST_MPI_COLLECTIVES_HPP

#include <boost/mpi/communicator.hpp>
#include <boost/mpi/inplace.hpp>
#include <vector>

namespace boost { namespace mpi {
/**
 *  @brief Gather the values stored at every process into vectors of
 *  values from each process.
 *
 *  @c all_gather is a collective algorithm that collects the values
 *  stored at each process into a vector of values indexed by the
 *  process number they came from. The type @c T of the values may be
 *  any type that is serializable or has an associated MPI data type.
 *
 *  When the type @c T has an associated MPI data type, this routine
 *  invokes @c MPI_Allgather to gather the values.
 *
 *    @param comm The communicator over which the all-gather will
 *    occur.
 *
 *    @param in_value The value to be transmitted by each process. To
 *    gather an array of values, @c in_values points to the @c n local
 *    values to be transmitted.
 *
 *    @param out_values A vector or pointer to storage that will be
 *    populated with the values from each process, indexed by the
 *    process ID number. If it is a vector, the vector will be resized
 *    accordingly.
 */
template<typename T>
void
all_gather(const communicator& comm, const T& in_value, 
           std::vector<T>& out_values);

/**
 * \overload
 */
template<typename T>
void
all_gather(const communicator& comm, const T& in_value, T* out_values);

/**
 * \overload
 */
template<typename T>
void
all_gather(const communicator& comm, const T* in_values, int n,
           std::vector<T>& out_values);

/**
 * \overload
 */
template<typename T>
void
all_gather(const communicator& comm, const T* in_values, int n, T* out_values);

/**
 *  @brief Combine the values stored by each process into a single
 *  value available to all processes.
 *
 *  @c all_reduce is a collective algorithm that combines the values
 *  stored by each process into a single value available to all
 *  processes. The values are combined in a user-defined way,
 *  specified via a function object. The type @c T of the values may
 *  be any type that is serializable or has an associated MPI data
 *  type. One can think of this operation as a @c all_gather, followed
 *  by an @c std::accumulate() over the gather values and using the
 *  operation @c op.
 *
 *  When the type @c T has an associated MPI data type, this routine
 *  invokes @c MPI_Allreduce to perform the reduction. If possible,
 *  built-in MPI operations will be used; otherwise, @c all_reduce()
 *  will create a custom MPI_Op for the call to MPI_Allreduce.
 *
 *    @param comm The communicator over which the reduction will
 *    occur.
 *    @param value The local value to be combined with the local
 *    values of every other process. For reducing arrays, @c in_values
 *    is a pointer to the local values to be reduced and @c n is the
 *    number of values to reduce. See @c reduce for more information.
 *
 *    If wrapped in a @c inplace_t object, combine the usage of both
 *    input and $c out_value and the local value will be overwritten
 *    (a convenience function @c inplace is provided for the wrapping).
 *
 *    @param out_value Will receive the result of the reduction
 *    operation. If this parameter is omitted, the outgoing value will
 *    instead be returned.
 *
 *    @param op The binary operation that combines two values of type
 *    @c T and returns a third value of type @c T. For types @c T that has
 *    ssociated MPI data types, @c op will either be translated into
 *    an @c MPI_Op (via @c MPI_Op_create) or, if possible, mapped
 *    directly to a built-in MPI operation. See @c is_mpi_op in the @c
 *    operations.hpp header for more details on this mapping. For any
 *    non-built-in operation, commutativity will be determined by the
 *    @c is_commmutative trait (also in @c operations.hpp): users are
 *    encouraged to mark commutative operations as such, because it
 *    gives the implementation additional lattitude to optimize the
 *    reduction operation.
 *
 *    @param n Indicated the size of the buffers of array type.
 *    @returns If no @p out_value parameter is supplied, returns the
 *    result of the reduction operation.
 */
template<typename T, typename Op>
void
all_reduce(const communicator& comm, const T* value, int n, T* out_value, 
           Op op);
/**
 * \overload
 */
template<typename T, typename Op>
void
all_reduce(const communicator& comm, const T& value, T& out_value, Op op);
/**
 * \overload
 */
template<typename T, typename Op>
T all_reduce(const communicator& comm, const T& value, Op op);

/**
 * \overload
 */
template<typename T, typename Op>
void
all_reduce(const communicator& comm, inplace_t<T*> value, int n,
           Op op);
/**
 * \overload
 */
template<typename T, typename Op>
void
all_reduce(const communicator& comm, inplace_t<T> value, Op op);

/**
 *  @brief Send data from every process to every other process.
 *
 *  @c all_to_all is a collective algorithm that transmits @c p values
 *  from every process to every other process. On process i, jth value
 *  of the @p in_values vector is sent to process j and placed in the
 *  ith position of the @p out_values vector in process @p j. The type
 *  @c T of the values may be any type that is serializable or has an
 *  associated MPI data type. If @c n is provided, then arrays of @p n
 *  values will be transferred from one process to another.
 *
 *  When the type @c T has an associated MPI data type, this routine
 *  invokes @c MPI_Alltoall to scatter the values.
 *
 *    @param comm The communicator over which the all-to-all
 *    communication will occur.
 *
 *    @param in_values A vector or pointer to storage that contains
 *    the values to send to each process, indexed by the process ID
 *    number.
 *
 *    @param out_values A vector or pointer to storage that will be
 *    updated to contain the values received from other processes. The
 *    jth value in @p out_values will come from the procss with rank j.
 */
template<typename T>
void
all_to_all(const communicator& comm, const std::vector<T>& in_values,
           std::vector<T>& out_values);

/**
 * \overload
 */
template<typename T>
void all_to_all(const communicator& comm, const T* in_values, T* out_values);

/**
 * \overload
 */
template<typename T>
void
all_to_all(const communicator& comm, const std::vector<T>& in_values, int n,
           std::vector<T>& out_values);

/**
 * \overload
 */
template<typename T>
void 
all_to_all(const communicator& comm, const T* in_values, int n, T* out_values);

/**
 * @brief Broadcast a value from a root process to all other
 * processes.
 *
 * @c broadcast is a collective algorithm that transfers a value from
 * an arbitrary @p root process to every other process that is part of
 * the given communicator. The @c broadcast algorithm can transmit any
 * Serializable value, values that have associated MPI data types,
 * packed archives, skeletons, and the content of skeletons; see the
 * @c send primitive for communicators for a complete list. The type
 * @c T shall be the same for all processes that are a part of the
 * communicator @p comm, unless packed archives are being transferred:
 * with packed archives, the root sends a @c packed_oarchive or @c
 * packed_skeleton_oarchive whereas the other processes receive a
 * @c packed_iarchive or @c packed_skeleton_iarchve, respectively.
 *
 * When the type @c T has an associated MPI data type, this routine
 * invokes @c MPI_Bcast to perform the broadcast.
 *
 *   @param comm The communicator over which the broadcast will
 *   occur.
 *
 *   @param value The value (or values, if @p n is provided) to be
 *   transmitted (if the rank of @p comm is equal to @p root) or
 *   received (if the rank of @p comm is not equal to @p root). When
 *   the @p value is a @c skeleton_proxy, only the skeleton of the
 *   object will be broadcast. In this case, the @p root will build a
 *   skeleton from the object help in the proxy and all of the
 *   non-roots will reshape the objects held in their proxies based on
 *   the skeleton sent from the root.
 *
 *   @param n When supplied, the number of values that the pointer @p
 *   values points to, for broadcasting an array of values. The value
 *   of @p n must be the same for all processes in @p comm.
 *
 *   @param root The rank/process ID of the process that will be
 *   transmitting the value.
 */
template<typename T>
void broadcast(const communicator& comm, T& value, int root);

/**
 * \overload
 */
template<typename T>
void broadcast(const communicator& comm, T* values, int n, int root);

/**
 * \overload
 */
template<typename T>
void broadcast(const communicator& comm, skeleton_proxy<T>& value, int root);

/**
 * \overload
 */
template<typename T>
void
broadcast(const communicator& comm, const skeleton_proxy<T>& value, int root);

/**
 *  @brief Gather the values stored at every process into a vector at
 *  the root process.
 *
 *  @c gather is a collective algorithm that collects the values
 *  stored at each process into a vector of values at the @p root
 *  process. This vector is indexed by the process number that the
 *  value came from. The type @c T of the values may be any type that
 *  is serializable or has an associated MPI data type.
 *
 *  When the type @c T has an associated MPI data type, this routine
 *  invokes @c MPI_Gather to gather the values.
 *
 *    @param comm The communicator over which the gather will occur.
 *
 *    @param in_value The value to be transmitted by each process. For
 *    gathering arrays of values, @c in_values points to storage for
 *    @c n*comm.size() values.
 *
 *    @param out_values A vector or pointer to storage that will be
 *    populated with the values from each process, indexed by the
 *    process ID number. If it is a vector, it will be resized
 *    accordingly. For non-root processes, this parameter may be
 *    omitted. If it is still provided, however, it will be unchanged.
 *
 *    @param root The process ID number that will collect the
 *    values. This value must be the same on all processes.
 */
template<typename T>
void
gather(const communicator& comm, const T& in_value, std::vector<T>& out_values,
       int root);

/**
 * \overload
 */
template<typename T>
void
gather(const communicator& comm, const T& in_value, T* out_values, int root);

/**
 * \overload
 */
template<typename T>
void gather(const communicator& comm, const T& in_value, int root);

/**
 * \overload
 */
template<typename T>
void
gather(const communicator& comm, const T* in_values, int n, 
       std::vector<T>& out_values, int root);

/**
 * \overload
 */
template<typename T>
void
gather(const communicator& comm, const T* in_values, int n, T* out_values, 
       int root);

/**
 * \overload
 */
template<typename T>
void gather(const communicator& comm, const T* in_values, int n, int root);

/**
 *  @brief Similar to boost::mpi::gather with the difference that the number
 *  of values to be send by non-root processes can vary.
 *
 *    @param comm The communicator over which the gather will occur.
 *
 *    @param in_values The array of values to be transmitted by each process.
 *
 *    @param in_size For each non-root process this specifies the size
 *    of @p in_values.
 *
 *    @param out_values A pointer to storage that will be populated with
 *    the values from each process. For non-root processes, this parameter
 *    may be omitted. If it is still provided, however, it will be unchanged.
 *
 *    @param sizes A vector containing the number of elements each non-root
 *    process will send.
 *
 *    @param displs A vector such that the i-th entry specifies the
 *    displacement (relative to @p out_values) from which to take the ingoing
 *    data at the @p root process. Overloaded versions for which @p displs is
 *    omitted assume that the data is to be placed contiguously at the root process.
 *
 *    @param root The process ID number that will collect the
 *    values. This value must be the same on all processes.
 */
template<typename T>
void
gatherv(const communicator& comm, const std::vector<T>& in_values,
        T* out_values, const std::vector<int>& sizes, const std::vector<int>& displs,
        int root);

/**
 * \overload
 */
template<typename T>
void
gatherv(const communicator& comm, const T* in_values, int in_size,
        T* out_values, const std::vector<int>& sizes, const std::vector<int>& displs,
        int root);

/**
 * \overload
 */
template<typename T>
void gatherv(const communicator& comm, const std::vector<T>& in_values, int root);

/**
 * \overload
 */
template<typename T>
void gatherv(const communicator& comm, const T* in_values, int in_size, int root);

/**
 * \overload
 */
template<typename T>
void
gatherv(const communicator& comm, const T* in_values, int in_size,
        T* out_values, const std::vector<int>& sizes, int root);

/**
 * \overload
 */
template<typename T>
void
gatherv(const communicator& comm, const std::vector<T>& in_values,
        T* out_values, const std::vector<int>& sizes, int root);

/**
 *  @brief Scatter the values stored at the root to all processes
 *  within the communicator.
 *
 *  @c scatter is a collective algorithm that scatters the values
 *  stored in the @p root process (inside a vector) to all of the
 *  processes in the communicator. The vector @p out_values (only
 *  significant at the @p root) is indexed by the process number to
 *  which the corresponding value will be sent. The type @c T of the
 *  values may be any type that is serializable or has an associated
 *  MPI data type.
 *
 *  When the type @c T has an associated MPI data type, this routine
 *  invokes @c MPI_Scatter to scatter the values.
 *
 *    @param comm The communicator over which the scatter will occur.
 *
 *    @param in_values A vector or pointer to storage that will contain
 *    the values to send to each process, indexed by the process rank.
 *    For non-root processes, this parameter may be omitted. If it is
 *    still provided, however, it will be unchanged.
 *
 *    @param out_value The value received by each process. When
 *    scattering an array of values, @p out_values points to the @p n
 *    values that will be received by each process.
 *
 *    @param root The process ID number that will scatter the
 *    values. This value must be the same on all processes.
 */
template<typename T>
void
scatter(const communicator& comm, const std::vector<T>& in_values, T& out_value,
        int root);

/**
 * \overload
 */
template<typename T>
void
scatter(const communicator& comm, const T* in_values, T& out_value, int root);

/**
 * \overload
 */
template<typename T>
void scatter(const communicator& comm, T& out_value, int root);

/**
 * \overload
 */
template<typename T>
void
scatter(const communicator& comm, const std::vector<T>& in_values, 
        T* out_values, int n, int root);

/**
 * \overload
 */
template<typename T>
void
scatter(const communicator& comm, const T* in_values, T* out_values, int n,
        int root);

/**
 * \overload
 */
template<typename T>
void scatter(const communicator& comm, T* out_values, int n, int root);

/**
 *  @brief Similar to boost::mpi::scatter with the difference that the number
 *  of values stored at the root process does not need to be a multiple of
 *  the communicator's size.
 *
 *    @param comm The communicator over which the scatter will occur.
 *
 *    @param in_values A vector or pointer to storage that will contain
 *    the values to send to each process, indexed by the process rank.
 *    For non-root processes, this parameter may be omitted. If it is
 *    still provided, however, it will be unchanged.
 *
 *    @param sizes A vector containing the number of elements each non-root
 *    process will receive.
 *
 *    @param displs A vector such that the i-th entry specifies the
 *    displacement (relative to @p in_values) from which to take the outgoing
 *    data to process i. Overloaded versions for which @p displs is omitted
 *    assume that the data is contiguous at the @p root process.
 *
 *    @param out_values The array of values received by each process.
 *
 *    @param out_size For each non-root process this will contain the size
 *    of @p out_values.
 *
 *    @param root The process ID number that will scatter the
 *    values. This value must be the same on all processes.
 */
template<typename T>
void
scatterv(const communicator& comm, const std::vector<T>& in_values,
         const std::vector<int>& sizes, const std::vector<int>& displs,
         T* out_values, int out_size, int root);

/**
 * \overload
 */
template<typename T>
void
scatterv(const communicator& comm, const T* in_values,
         const std::vector<int>& sizes, const std::vector<int>& displs,
         T* out_values, int out_size, int root);

/**
 * \overload
 */
template<typename T>
void scatterv(const communicator& comm, T* out_values, int out_size, int root);

/**
 * \overload
 */
template<typename T>
void
scatterv(const communicator& comm, const T* in_values,
         const std::vector<int>& sizes, T* out_values, int root);

/**
 * \overload
 */
template<typename T>
void
scatterv(const communicator& comm, const std::vector<T>& in_values,
         const std::vector<int>& sizes, T* out_values, int root);

/**
 *  @brief Combine the values stored by each process into a single
 *  value at the root.
 *
 *  @c reduce is a collective algorithm that combines the values
 *  stored by each process into a single value at the @c root. The
 *  values can be combined arbitrarily, specified via a function
 *  object. The type @c T of the values may be any type that is
 *  serializable or has an associated MPI data type. One can think of
 *  this operation as a @c gather to the @p root, followed by an @c
 *  std::accumulate() over the gathered values and using the operation
 *  @c op. 
 *
 *  When the type @c T has an associated MPI data type, this routine
 *  invokes @c MPI_Reduce to perform the reduction. If possible,
 *  built-in MPI operations will be used; otherwise, @c reduce() will
 *  create a custom MPI_Op for the call to MPI_Reduce.
 *
 *    @param comm The communicator over which the reduction will
 *    occur.
 *
 *    @param in_value The local value to be combined with the local
 *    values of every other process. For reducing arrays, @c in_values
 *    contains a pointer to the local values. In this case, @c n is
 *    the number of values that will be reduced. Reduction occurs
 *    independently for each of the @p n values referenced by @p
 *    in_values, e.g., calling reduce on an array of @p n values is
 *    like calling @c reduce @p n separate times, one for each
 *    location in @p in_values and @p out_values.
 *
 *    @param out_value Will receive the result of the reduction
 *    operation, but only for the @p root process. Non-root processes
 *    may omit if parameter; if they choose to supply the parameter,
 *    it will be unchanged. For reducing arrays, @c out_values
 *    contains a pointer to the storage for the output values.
 *
 *    @param op The binary operation that combines two values of type
 *    @c T into a third value of type @c T. For types @c T that has
 *    ssociated MPI data types, @c op will either be translated into
 *    an @c MPI_Op (via @c MPI_Op_create) or, if possible, mapped
 *    directly to a built-in MPI operation. See @c is_mpi_op in the @c
 *    operations.hpp header for more details on this mapping. For any
 *    non-built-in operation, commutativity will be determined by the
 *    @c is_commmutative trait (also in @c operations.hpp): users are
 *    encouraged to mark commutative operations as such, because it
 *    gives the implementation additional lattitude to optimize the
 *    reduction operation.
 *
 *    @param root The process ID number that will receive the final,
 *    combined value. This value must be the same on all processes.
 */
template<typename T, typename Op>
void
reduce(const communicator& comm, const T& in_value, T& out_value, Op op,
       int root);

/**
 * \overload
 */
template<typename T, typename Op>
void reduce(const communicator& comm, const T& in_value, Op op, int root);

/**
 * \overload
 */
template<typename T, typename Op>
void
reduce(const communicator& comm, const T* in_values, int n, T* out_values, 
       Op op, int root);

/**
 * \overload
 */
template<typename T, typename Op>
void 
reduce(const communicator& comm, const T* in_values, int n, Op op, int root);

/**
 *  @brief Compute a prefix reduction of values from all processes in
 *  the communicator.
 *
 *  @c scan is a collective algorithm that combines the values stored
 *  by each process with the values of all processes with a smaller
 *  rank. The values can be arbitrarily combined, specified via a
 *  function object @p op. The type @c T of the values may be any type
 *  that is serializable or has an associated MPI data type. One can
 *  think of this operation as a @c gather to some process, followed
 *  by an @c std::prefix_sum() over the gathered values using the
 *  operation @c op. The ith process returns the ith value emitted by
 *  @c std::prefix_sum().
 *
 *  When the type @c T has an associated MPI data type, this routine
 *  invokes @c MPI_Scan to perform the reduction. If possible,
 *  built-in MPI operations will be used; otherwise, @c scan() will
 *  create a custom @c MPI_Op for the call to MPI_Scan.
 *
 *    @param comm The communicator over which the prefix reduction
 *    will occur.
 *
 *    @param in_value The local value to be combined with the local
 *    values of other processes. For the array variant, the @c
 *    in_values parameter points to the @c n local values that will be
 *    combined.
 *
 *    @param out_value If provided, the ith process will receive the
 *    value @c op(in_value[0], op(in_value[1], op(..., in_value[i])
 *    ... )). For the array variant, @c out_values contains a pointer
 *    to storage for the @c n output values. The prefix reduction
 *    occurs independently for each of the @p n values referenced by
 *    @p in_values, e.g., calling scan on an array of @p n values is
 *    like calling @c scan @p n separate times, one for each location
 *    in @p in_values and @p out_values.
 *
 *    @param op The binary operation that combines two values of type
 *    @c T into a third value of type @c T. For types @c T that has
 *    ssociated MPI data types, @c op will either be translated into
 *    an @c MPI_Op (via @c MPI_Op_create) or, if possible, mapped
 *    directly to a built-in MPI operation. See @c is_mpi_op in the @c
 *    operations.hpp header for more details on this mapping. For any
 *    non-built-in operation, commutativity will be determined by the
 *    @c is_commmutative trait (also in @c operations.hpp).
 *
 *    @returns If no @p out_value parameter is provided, returns the
 *    result of prefix reduction.
 */
template<typename T, typename Op>
void
scan(const communicator& comm, const T& in_value, T& out_value, Op op);

/**
 * \overload
 */
template<typename T, typename Op>
T
scan(const communicator& comm, const T& in_value, Op op);

/**
 * \overload
 */
template<typename T, typename Op>
void
scan(const communicator& comm, const T* in_values, int n, T* out_values, Op op);

} } // end namespace boost::mpi
#endif // BOOST_MPI_COLLECTIVES_HPP

#ifndef BOOST_MPI_COLLECTIVES_FORWARD_ONLY
// Include implementations of each of the collectives
#  include <boost/mpi/collectives/all_gather.hpp>
#  include <boost/mpi/collectives/all_reduce.hpp>
#  include <boost/mpi/collectives/all_to_all.hpp>
#  include <boost/mpi/collectives/broadcast.hpp>
#  include <boost/mpi/collectives/gather.hpp>
#  include <boost/mpi/collectives/gatherv.hpp>
#  include <boost/mpi/collectives/scatter.hpp>
#  include <boost/mpi/collectives/scatterv.hpp>
#  include <boost/mpi/collectives/reduce.hpp>
#  include <boost/mpi/collectives/scan.hpp>
#endif

