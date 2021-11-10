// Copyright (C) 2005-2006 Douglas Gregor <doug.gregor -at- gmail.com>
// Copyright (C) 2004 The Trustees of Indiana University

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//   Authors: Douglas Gregor
//            Andrew Lumsdaine

// Message Passing Interface 1.1 -- Section 4.9.1. Reduce
#ifndef BOOST_MPI_ALL_REDUCE_HPP
#define BOOST_MPI_ALL_REDUCE_HPP

#include <vector>

#include <boost/mpi/inplace.hpp>

// All-reduce falls back to reduce() + broadcast() in some cases.
#include <boost/mpi/collectives/broadcast.hpp>
#include <boost/mpi/collectives/reduce.hpp>

namespace boost { namespace mpi {
namespace detail {
  /**********************************************************************
   * Simple reduction with MPI_Allreduce                                *
   **********************************************************************/
  // We are reducing for a type that has an associated MPI
  // datatype and operation, so we'll use MPI_Allreduce directly.
  template<typename T, typename Op>
  void
  all_reduce_impl(const communicator& comm, const T* in_values, int n,
                  T* out_values, Op /*op*/, mpl::true_ /*is_mpi_op*/,
                  mpl::true_ /*is_mpi_datatype*/)
  {
    BOOST_MPI_CHECK_RESULT(MPI_Allreduce,
                           (const_cast<T*>(in_values), out_values, n,
                            boost::mpi::get_mpi_datatype<T>(*in_values),
                            (is_mpi_op<Op, T>::op()), comm));
  }

  /**********************************************************************
   * User-defined reduction with MPI_Allreduce                          *
   **********************************************************************/
  // We are reducing at the root for a type that has an associated MPI
  // datatype but with a custom operation. We'll use MPI_Reduce
  // directly, but we'll need to create an MPI_Op manually.
  template<typename T, typename Op>
  void
  all_reduce_impl(const communicator& comm, const T* in_values, int n,
                  T* out_values, Op /* op */, mpl::false_ /*is_mpi_op*/,
                  mpl::true_ /*is_mpi_datatype*/)
  {
    user_op<Op, T> mpi_op;
    BOOST_MPI_CHECK_RESULT(MPI_Allreduce,
                           (const_cast<T*>(in_values), out_values, n,
                            boost::mpi::get_mpi_datatype<T>(*in_values),
                            mpi_op.get_mpi_op(), comm));
  }

  /**********************************************************************
   * User-defined, tree-based reduction for non-MPI data types          *
   **********************************************************************/
  // We are reducing at the root for a type that has no associated MPI
  // datatype and operation, so we'll use a simple tree-based
  // algorithm.
  template<typename T, typename Op>
  void
  all_reduce_impl(const communicator& comm, const T* in_values, int n,
                  T* out_values, Op op, mpl::false_ /*is_mpi_op*/,
                  mpl::false_ /*is_mpi_datatype*/)
  {
    if (in_values == MPI_IN_PLACE) {
      // if in_values matches the in place tag, then the output
      // buffer actually contains the input data.
      // But we can just go back to the out of place 
      // implementation in this case.
      // it's not clear how/if we can avoid the copy.
      std::vector<T> tmp_in( out_values, out_values + n);
      reduce(comm, detail::c_data(tmp_in), n, out_values, op, 0);
    } else {
      reduce(comm, in_values, n, out_values, op, 0);
    }
    broadcast(comm, out_values, n, 0);
  }
} // end namespace detail

template<typename T, typename Op>
inline void
all_reduce(const communicator& comm, const T* in_values, int n, T* out_values,
           Op op)
{
  detail::all_reduce_impl(comm, in_values, n, out_values, op,
                          is_mpi_op<Op, T>(), is_mpi_datatype<T>());
}

template<typename T, typename Op>
inline void
all_reduce(const communicator& comm, inplace_t<T*> inout_values, int n, Op op)
{
  all_reduce(comm, static_cast<const T*>(MPI_IN_PLACE), n, inout_values.buffer, op);
}

template<typename T, typename Op>
inline void
all_reduce(const communicator& comm, inplace_t<T> inout_values, Op op)
{
  all_reduce(comm, static_cast<const T*>(MPI_IN_PLACE), 1, &(inout_values.buffer), op);
}

template<typename T, typename Op>
inline void
all_reduce(const communicator& comm, const T& in_value, T& out_value, Op op)
{
  detail::all_reduce_impl(comm, &in_value, 1, &out_value, op,
                          is_mpi_op<Op, T>(), is_mpi_datatype<T>());
}

template<typename T, typename Op>
T all_reduce(const communicator& comm, const T& in_value, Op op)
{
  T result;
  ::boost::mpi::all_reduce(comm, in_value, result, op);
  return result;
}

} } // end namespace boost::mpi

#endif // BOOST_MPI_ALL_REDUCE_HPP
