// Copyright (C) 2005-2006 Douglas Gregor <doug.gregor@gmail.com>.
// Copyright (C) 2004 The Trustees of Indiana University

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//   Authors: Douglas Gregor
//            Andrew Lumsdaine

// Message Passing Interface 1.1 -- Section 4.9.1. Scan
#ifndef BOOST_MPI_SCAN_HPP
#define BOOST_MPI_SCAN_HPP

#include <boost/mpi/exception.hpp>
#include <boost/mpi/datatype.hpp>

// For (de-)serializing sends and receives
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/mpi/packed_iarchive.hpp>

// For packed_[io]archive sends and receives
#include <boost/mpi/detail/point_to_point.hpp>

#include <boost/mpi/communicator.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/mpi/detail/computation_tree.hpp>
#include <boost/mpi/operations.hpp>
#include <algorithm>
#include <exception>
#include <boost/assert.hpp>

namespace boost { namespace mpi {


/************************************************************************
 * Implementation details                                               *
 ************************************************************************/
namespace detail {
  /**********************************************************************
   * Simple prefix reduction with MPI_Scan                              *
   **********************************************************************/

  // We are performing prefix reduction for a type that has an
  // associated MPI datatype and operation, so we'll use MPI_Scan
  // directly.
  template<typename T, typename Op>
  void
  scan_impl(const communicator& comm, const T* in_values, int n, T* out_values,
            Op /*op*/, mpl::true_ /*is_mpi_op*/, mpl::true_ /*is_mpi_datatype*/)
  {
    BOOST_MPI_CHECK_RESULT(MPI_Scan,
                           (const_cast<T*>(in_values), out_values, n,
                            boost::mpi::get_mpi_datatype<T>(*in_values),
                            (is_mpi_op<Op, T>::op()), comm));
  }

  /**********************************************************************
   * User-defined prefix reduction with MPI_Scan                        *
   **********************************************************************/

  // We are performing prefix reduction for a type that has an
  // associated MPI datatype but with a custom operation. We'll use
  // MPI_Scan directly, but we'll need to create an MPI_Op manually.
  template<typename T, typename Op>
  void
  scan_impl(const communicator& comm, const T* in_values, int n, T* out_values,
            Op op, mpl::false_ /*is_mpi_op*/, mpl::true_ /*is_mpi_datatype*/)
  {
    user_op<Op, T> mpi_op;
    BOOST_MPI_CHECK_RESULT(MPI_Scan,
                           (const_cast<T*>(in_values), out_values, n,
                            boost::mpi::get_mpi_datatype<T>(*in_values),
                            mpi_op.get_mpi_op(), comm));
  }

  /**********************************************************************
   * User-defined, tree-based reduction for non-MPI data types          *
   **********************************************************************/

  template<typename T, typename Op>
  void
  upper_lower_scan(const communicator& comm, const T* in_values, int n,
                   T* out_values, Op& op, int lower, int upper)
  {
    int tag = environment::collectives_tag();
    int rank = comm.rank();

    if (lower + 1 == upper) {
      std::copy(in_values, in_values + n, out_values);
    } else {
      int middle = (lower + upper) / 2;
      
      if (rank < middle) {
        // Lower half
        upper_lower_scan(comm, in_values, n, out_values, op, lower, middle);

        // If we're the last process in the lower half, send our values
        // to everyone in the upper half.
        if (rank == middle - 1) {
          packed_oarchive oa(comm);
          for (int i = 0; i < n; ++i)
            oa << out_values[i];

          for (int p = middle; p < upper; ++p)
            comm.send(p, tag, oa);
        }
      } else {
        // Upper half
        upper_lower_scan(comm, in_values, n, out_values, op, middle, upper);

        // Receive value from the last process in the lower half.
        packed_iarchive ia(comm);
        comm.recv(middle - 1, tag, ia);

        // Combine value that came from the left with our value
        T left_value;
        for (int i = 0; i < n; ++i)
          {
            ia >> left_value;
            out_values[i] = op(left_value, out_values[i]);
          }
      }
    }
  }

  // We are performing prefix reduction for a type that has no
  // associated MPI datatype and operation, so we'll use a simple
  // upper/lower algorithm.
  template<typename T, typename Op>
  inline void
  scan_impl(const communicator& comm, const T* in_values, int n, T* out_values, 
            Op op, mpl::false_ /*is_mpi_op*/, mpl::false_/*is_mpi_datatype*/)
  {
    upper_lower_scan(comm, in_values, n, out_values, op, 0, comm.size());
  }
} // end namespace detail


template<typename T, typename Op>
inline void
scan(const communicator& comm, const T& in_value, T& out_value, Op op)
{
  detail::scan_impl(comm, &in_value, 1, &out_value, op, 
                    is_mpi_op<Op, T>(), is_mpi_datatype<T>());
}

template<typename T, typename Op>
inline void
scan(const communicator& comm, const T* in_values, int n, T* out_values, Op op)
{
  detail::scan_impl(comm, in_values, n, out_values, op, 
                    is_mpi_op<Op, T>(), is_mpi_datatype<T>());
}

template<typename T, typename Op>
inline T
scan(const communicator& comm, const T& in_value, Op op)
{
  T out_value;
  detail::scan_impl(comm, &in_value, 1, &out_value, op, 
                    is_mpi_op<Op, T>(), is_mpi_datatype<T>());
  return out_value;
}

} } // end namespace boost::mpi

#endif // BOOST_MPI_SCAN_HPP
