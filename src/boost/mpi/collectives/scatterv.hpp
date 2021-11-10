// Copyright (C) 2011 Júlio Hoffimann.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Message Passing Interface 1.1 -- Section 4.6. Scatterv
#ifndef BOOST_MPI_SCATTERV_HPP
#define BOOST_MPI_SCATTERV_HPP

#include <boost/scoped_array.hpp>
#include <boost/mpi/collectives/scatter.hpp>
#include <boost/mpi/detail/offsets.hpp>
#include <boost/mpi/detail/antiques.hpp>

namespace boost { namespace mpi {

namespace detail {

//////////////////////////////////////////////
/// Implementation for MPI primitive types ///
//////////////////////////////////////////////

// We're scattering from the root for a type that has an associated MPI
// datatype, so we'll use MPI_Scatterv to do all of the work.
template<typename T>
void
scatterv_impl(const communicator& comm, const T* in_values, T* out_values, int out_size,
              const int* sizes, const int* displs, int root, mpl::true_)
{
  assert(!sizes || out_size == sizes[comm.rank()]);
  assert(bool(sizes) == bool(in_values));
  
  scoped_array<int> new_offsets_mem(make_offsets(comm, sizes, displs, root));
  if (new_offsets_mem) displs = new_offsets_mem.get();
  MPI_Datatype type = get_mpi_datatype<T>(*in_values);
  BOOST_MPI_CHECK_RESULT(MPI_Scatterv,
                         (const_cast<T*>(in_values), const_cast<int*>(sizes),
                          const_cast<int*>(displs), type,
                          out_values, out_size, type, root, comm));
}

// We're scattering from a non-root for a type that has an associated MPI
// datatype, so we'll use MPI_Scatterv to do all of the work.
template<typename T>
void
scatterv_impl(const communicator& comm, T* out_values, int out_size, int root, 
              mpl::true_ is_mpi_type)
{
  scatterv_impl(comm, (T const*)0, out_values, out_size, 
                (const int*)0, (const int*)0, root, is_mpi_type);
}

//////////////////////////////////////////////////
/// Implementation for non MPI primitive types ///
//////////////////////////////////////////////////

// We're scattering from the root for a type that does not have an
// associated MPI datatype, so we'll need to serialize it.
template<typename T>
void
scatterv_impl(const communicator& comm, const T* in_values, T* out_values, int out_size,
              int const* sizes, int const* displs, int root, mpl::false_)
{
  packed_oarchive::buffer_type sendbuf;
  bool is_root = comm.rank() == root;
  int nproc = comm.size();
  std::vector<int> archsizes;
  if (is_root) {
    assert(out_size == sizes[comm.rank()]);
    archsizes.resize(nproc);
    std::vector<int> skipped;
    if (displs) {
      skipped.resize(nproc);
      offsets2skipped(sizes, displs, c_data(skipped), nproc);
      displs = c_data(skipped);
    }
    fill_scatter_sendbuf(comm, in_values, sizes, (int const*)0, sendbuf, archsizes);
  }
  dispatch_scatter_sendbuf(comm, sendbuf, archsizes, (T const*)0, out_values, out_size, root);
}

// We're scattering to a non-root for a type that does not have an
// associated MPI datatype. input data not needed.
// it.
template<typename T>
void
scatterv_impl(const communicator& comm, T* out_values, int n, int root, 
              mpl::false_ isnt_mpi_type)
{
  assert(root != comm.rank());
  scatterv_impl(comm, (T const*)0, out_values, n, (int const*)0, (int const*)0, root, isnt_mpi_type);
}

} // end namespace detail

template<typename T>
void
scatterv(const communicator& comm, const T* in_values,
         const std::vector<int>& sizes, const std::vector<int>& displs,
         T* out_values, int out_size, int root)
{
  using detail::c_data;
  detail::scatterv_impl(comm, in_values, out_values, out_size, c_data(sizes), c_data(displs), 
                root, is_mpi_datatype<T>());
}

template<typename T>
void
scatterv(const communicator& comm, const std::vector<T>& in_values, 
         const std::vector<int>& sizes, const std::vector<int>& displs,
         T* out_values, int out_size, int root)
{
  using detail::c_data;
  ::boost::mpi::scatterv(comm, c_data(in_values), sizes, displs,
                         out_values, out_size, root);
}

template<typename T>
void scatterv(const communicator& comm, T* out_values, int out_size, int root)
{
  BOOST_ASSERT(comm.rank() != root);
  detail::scatterv_impl(comm, out_values, out_size, root, is_mpi_datatype<T>());
}

///////////////////////
// common use versions
///////////////////////
template<typename T>
void
scatterv(const communicator& comm, const T* in_values,
         const std::vector<int>& sizes, T* out_values, int root)
{
  using detail::c_data;
  detail::scatterv_impl(comm, in_values, out_values, sizes[comm.rank()], 
                        c_data(sizes), (int const*)0,
                        root, is_mpi_datatype<T>());
}

template<typename T>
void
scatterv(const communicator& comm, const std::vector<T>& in_values,
         const std::vector<int>& sizes, T* out_values, int root)
{
  ::boost::mpi::scatterv(comm, detail::c_data(in_values), sizes, out_values, root);
}

template<typename T>
void
scatterv(const communicator& comm, const T* in_values,
         T* out_values, int n, int root)
{
  detail::scatterv_impl(comm, in_values, out_values, n, (int const*)0, (int const*)0,
                root, is_mpi_datatype<T>());
}

template<typename T>
void
scatterv(const communicator& comm, const std::vector<T>& in_values,
         T* out_values, int out_size, int root)
{
  ::boost::mpi::scatterv(comm, detail::c_data(in_values), out_values, out_size, root);
}

} } // end namespace boost::mpi

#endif // BOOST_MPI_SCATTERV_HPP
