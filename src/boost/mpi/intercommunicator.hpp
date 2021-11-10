// Copyright (C) 2007 The Trustees of Indiana University.

// Authors: Douglas Gregor
//          Andrew Lumsdaine

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** @file intercommunicator.hpp
 *
 *  This header defines the @c intercommunicator class, which permits
 *  communication between different process groups.
 */
#ifndef BOOST_MPI_INTERCOMMUNICATOR_HPP
#define BOOST_MPI_INTERCOMMUNICATOR_HPP

#include <boost/mpi/communicator.hpp>

namespace boost { namespace mpi {

/**
 * INTERNAL ONLY
 *
 * Forward declaration of the MPI "group" representation, for use in
 * the description of the @c intercommunicator class.
 */ 
class group;

/**
 * @brief Communication facilities among processes in different
 * groups.
 *
 * The @c intercommunicator class provides communication facilities
 * among processes from different groups. An intercommunicator is
 * always associated with two process groups: one "local" process
 * group, containing the process that initiates an MPI operation
 * (e.g., the sender in a @c send operation), and one "remote" process
 * group, containing the process that is the target of the MPI
 * operation.
 *
 * While intercommunicators have essentially the same point-to-point
 * operations as intracommunicators (the latter communicate only
 * within a single process group), all communication with
 * intercommunicators occurs between the processes in the local group
 * and the processes in the remote group; communication within a group
 * must use a different (intra-)communicator.
 * 
 */   
class BOOST_MPI_DECL intercommunicator : public communicator
{
private:
  friend class communicator;

  /**
   * INTERNAL ONLY
   *
   * Construct an intercommunicator given a shared pointer to the
   * underlying MPI_Comm. This operation is used for "casting" from a
   * communicator to an intercommunicator.
   */
  explicit intercommunicator(const shared_ptr<MPI_Comm>& cp)
  {
    this->comm_ptr = cp;
  }

public:
  /**
   * Build a new Boost.MPI intercommunicator based on the MPI
   * intercommunicator @p comm.
   *
   * @p comm may be any valid MPI intercommunicator. If @p comm is
   * MPI_COMM_NULL, an empty communicator (that cannot be used for
   * communication) is created and the @p kind parameter is
   * ignored. Otherwise, the @p kind parameter determines how the
   * Boost.MPI communicator will be related to @p comm:
   *
   *   - If @p kind is @c comm_duplicate, duplicate @c comm to create
   *   a new communicator. This new communicator will be freed when
   *   the Boost.MPI communicator (and all copies of it) is
   *   destroyed. This option is only permitted if the underlying MPI
   *   implementation supports MPI 2.0; duplication of
   *   intercommunicators is not available in MPI 1.x.
   *
   *   - If @p kind is @c comm_take_ownership, take ownership of @c
   *   comm. It will be freed automatically when all of the Boost.MPI
   *   communicators go out of scope.
   *
   *   - If @p kind is @c comm_attach, this Boost.MPI communicator
   *   will reference the existing MPI communicator @p comm but will
   *   not free @p comm when the Boost.MPI communicator goes out of
   *   scope. This option should only be used when the communicator is
   *   managed by the user.
   */
  intercommunicator(const MPI_Comm& comm, comm_create_kind kind)
    : communicator(comm, kind) { }

  /**
   * Constructs a new intercommunicator whose local group is @p local
   * and whose remote group is @p peer. The intercommunicator can then
   * be used to communicate between processes in the two groups. This
   * constructor is equivalent to a call to @c MPI_Intercomm_create.
   *
   * @param local The intracommunicator containing all of the
   * processes that will go into the local group.
   *
   * @param local_leader The rank within the @p local
   * intracommunicator that will serve as its leader.
   *
   * @param peer The intracommunicator containing all of the processes
   * that will go into the remote group.
   *
   * @param remote_leader The rank within the @p peer group that will
   * serve as its leader.
   */
  intercommunicator(const communicator& local, int local_leader,
                    const communicator& peer, int remote_leader);

  /**
   * Returns the size of the local group, i.e., the number of local
   * processes that are part of the group.
   */
  int local_size() const { return this->size(); }

  /**
   * Returns the local group, containing all of the local processes in
   * this intercommunicator.
   */
  boost::mpi::group local_group() const;

  /**
   * Returns the rank of this process within the local group.
   */
  int local_rank() const { return this->rank(); }

  /**
   * Returns the size of the remote group, i.e., the number of
   * processes that are part of the remote group.
   */
  int remote_size() const;

  /**
   * Returns the remote group, containing all of the remote processes
   * in this intercommunicator.
   */
  boost::mpi::group remote_group() const;

  /**
   * Merge the local and remote groups in this intercommunicator into
   * a new intracommunicator containing the union of the processes in
   * both groups. This method is equivalent to @c MPI_Intercomm_merge.
   *  
   * @param high Whether the processes in this group should have the
   * higher rank numbers than the processes in the other group. Each
   * of the processes within a particular group shall have the same
   * "high" value.
   *
   * @returns the new, merged intracommunicator
   */
  communicator merge(bool high) const;
};

} } // end namespace boost::mpi

#endif // BOOST_MPI_INTERCOMMUNICATOR_HPP
