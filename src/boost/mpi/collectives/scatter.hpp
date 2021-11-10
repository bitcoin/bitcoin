// Copyright (C) 2005, 2006 Douglas Gregor.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Message Passing Interface 1.1 -- Section 4.6. Scatter
#ifndef BOOST_MPI_SCATTER_HPP
#define BOOST_MPI_SCATTER_HPP

#include <boost/mpi/exception.hpp>
#include <boost/mpi/datatype.hpp>
#include <vector>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/detail/point_to_point.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/mpi/detail/offsets.hpp>
#include <boost/mpi/detail/antiques.hpp>
#include <boost/assert.hpp>

namespace boost { namespace mpi {

namespace detail {
// We're scattering from the root for a type that has an associated MPI
// datatype, so we'll use MPI_Scatter to do all of the work.
template<typename T>
void
scatter_impl(const communicator& comm, const T* in_values, T* out_values, 
             int n, int root, mpl::true_)
{
  MPI_Datatype type = get_mpi_datatype<T>(*in_values);
  BOOST_MPI_CHECK_RESULT(MPI_Scatter,
                         (const_cast<T*>(in_values), n, type,
                          out_values, n, type, root, comm));
}

// We're scattering from a non-root for a type that has an associated MPI
// datatype, so we'll use MPI_Scatter to do all of the work.
template<typename T>
void
scatter_impl(const communicator& comm, T* out_values, int n, int root, 
             mpl::true_)
{
  MPI_Datatype type = get_mpi_datatype<T>(*out_values);
  BOOST_MPI_CHECK_RESULT(MPI_Scatter,
                         (0, n, type,
                          out_values, n, type,
                          root, comm));
}

// Fill the sendbuf while keeping trac of the slot's footprints
// Used in the first steps of both scatter and scatterv
// Nslots contains the number of slots being sent 
// to each process (identical values for scatter).
// skiped_slots, if present, is deduced from the 
// displacement array authorised be the MPI API, 
// for some yet to be determined reason.
template<typename T>
void
fill_scatter_sendbuf(const communicator& comm, T const* values, 
                     int const* nslots, int const* skipped_slots,
                     packed_oarchive::buffer_type& sendbuf, std::vector<int>& archsizes) {
  int nproc = comm.size();
  archsizes.resize(nproc);
  
  for (int dest = 0; dest < nproc; ++dest) {
    if (skipped_slots) { // wee need to keep this for backward compatibility
      for(int k= 0; k < skipped_slots[dest]; ++k) ++values;
    }
    packed_oarchive procarchive(comm);
    for (int i = 0; i < nslots[dest]; ++i) {
      procarchive << *values++;
    }
    int archsize = procarchive.size();
    sendbuf.resize(sendbuf.size() + archsize);
    archsizes[dest] = archsize;
    char const* aptr = static_cast<char const*>(procarchive.address());
    std::copy(aptr, aptr+archsize, sendbuf.end()-archsize);
  }
}

template<typename T, class A>
T*
non_const_data(std::vector<T,A> const& v) {
  using detail::c_data;
  return const_cast<T*>(c_data(v));
}

// Dispatch the sendbuf among proc.
// Used in the second steps of both scatter and scatterv
// in_value is only provide in the non variadic case.
template<typename T>
void
dispatch_scatter_sendbuf(const communicator& comm, 
                         packed_oarchive::buffer_type const& sendbuf, std::vector<int> const& archsizes,
                         T const* in_values,
                         T* out_values, int n, int root) {
  // Distribute the sizes
  int myarchsize;
  BOOST_MPI_CHECK_RESULT(MPI_Scatter,
                         (non_const_data(archsizes), 1, MPI_INT,
                          &myarchsize, 1, MPI_INT, root, comm));
  std::vector<int> offsets;
  if (root == comm.rank()) {
    sizes2offsets(archsizes, offsets);
  }
  // Get my proc archive
  packed_iarchive::buffer_type recvbuf;
  recvbuf.resize(myarchsize);
  BOOST_MPI_CHECK_RESULT(MPI_Scatterv,
                         (non_const_data(sendbuf), non_const_data(archsizes), c_data(offsets), MPI_BYTE,
                          c_data(recvbuf), recvbuf.size(), MPI_BYTE,
                          root, MPI_Comm(comm)));
  // Unserialize
  if ( in_values != 0 && root == comm.rank()) {
    // Our own local values are already here: just copy them.
    std::copy(in_values + root * n, in_values + (root + 1) * n, out_values);
  } else {
    // Otherwise deserialize:
    packed_iarchive iarchv(comm, recvbuf);
    for (int i = 0; i < n; ++i) {
      iarchv >> out_values[i];
    }
  }
}

// We're scattering from the root for a type that does not have an
// associated MPI datatype, so we'll need to serialize it.
template<typename T>
void
scatter_impl(const communicator& comm, const T* in_values, T* out_values, 
             int n, int root, mpl::false_)
{
  packed_oarchive::buffer_type sendbuf;
  std::vector<int> archsizes;
  
  if (root == comm.rank()) {
    std::vector<int> nslots(comm.size(), n);
    fill_scatter_sendbuf(comm, in_values, c_data(nslots), (int const*)0, sendbuf, archsizes);
  }
  dispatch_scatter_sendbuf(comm, sendbuf, archsizes, in_values, out_values, n, root);
}

template<typename T>
void
scatter_impl(const communicator& comm, T* out_values, int n, int root, 
             mpl::false_ is_mpi_type)
{ 
  scatter_impl(comm, (T const*)0, out_values, n, root, is_mpi_type);
}
} // end namespace detail

template<typename T>
void
scatter(const communicator& comm, const T* in_values, T& out_value, int root)
{
  detail::scatter_impl(comm, in_values, &out_value, 1, root, is_mpi_datatype<T>());
}

template<typename T>
void
scatter(const communicator& comm, const std::vector<T>& in_values, T& out_value,
        int root)
{
  using detail::c_data;
  ::boost::mpi::scatter<T>(comm, c_data(in_values), out_value, root);
}

template<typename T>
void scatter(const communicator& comm, T& out_value, int root)
{
  BOOST_ASSERT(comm.rank() != root);
  detail::scatter_impl(comm, &out_value, 1, root, is_mpi_datatype<T>());
}

template<typename T>
void
scatter(const communicator& comm, const T* in_values, T* out_values, int n,
        int root)
{
  detail::scatter_impl(comm, in_values, out_values, n, root, is_mpi_datatype<T>());
}

template<typename T>
void
scatter(const communicator& comm, const std::vector<T>& in_values, 
        T* out_values, int n, int root)
{
  ::boost::mpi::scatter(comm, detail::c_data(in_values), out_values, n, root);
}

template<typename T>
void scatter(const communicator& comm, T* out_values, int n, int root)
{
  BOOST_ASSERT(comm.rank() != root);
  detail::scatter_impl(comm, out_values, n, root, is_mpi_datatype<T>());
}

} } // end namespace boost::mpi

#endif // BOOST_MPI_SCATTER_HPP
