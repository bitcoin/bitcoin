// Copyright (C) 2005, 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Allows broadcast of skeletons via proxy.

// This header may only be included after both the broadcast.hpp and
// and skeleton_and_content.hpp headers have been included.
#ifndef BOOST_MPI_BROADCAST_SC_HPP
#define BOOST_MPI_BROADCAST_SC_HPP

namespace boost { namespace mpi {

template<typename T>
void
broadcast(const communicator& comm, const skeleton_proxy<T>& proxy, int root)
{
  if (comm.rank() == root) {
    packed_skeleton_oarchive oa(comm);
    oa << proxy.object;
    broadcast(comm, oa, root);
  } else {
    packed_skeleton_iarchive ia(comm);
    broadcast(comm, ia, root);
    ia >> proxy.object;
  }
}

template<typename T>
inline void
broadcast(const communicator& comm, skeleton_proxy<T>& proxy, int root)
{
  const skeleton_proxy<T>& const_proxy(proxy);
  broadcast(comm, const_proxy, root);
}


} } // end namespace boost::mpi

#endif // BOOST_MPI_BROADCAST_SC_HPP
