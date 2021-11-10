//          Copyright Alain Miniussi 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Authors: Alain Miniussi

#ifndef BOOST_MPI_ANTIQUES_HPP
#define BOOST_MPI_ANTIQUES_HPP

#include <vector>

// Support for some obsolette compilers

namespace boost { namespace mpi {
namespace detail {
  // Some old gnu compiler have no support for vector<>::data
  // Use this in the mean time, the cumbersome syntax should 
  // serve as an incentive to get rid of this when those compilers 
  // are dropped.
  template <typename T, typename A>
  T* c_data(std::vector<T,A>& v) { return v.empty() ? static_cast<T*>(0) : &(v[0]); }

  template <typename T, typename A>
  T const* c_data(std::vector<T,A> const& v) { return v.empty() ? static_cast<T const*>(0) : &(v[0]); }

  // Some old MPI implementation (OpenMPI 1.6 for example) have non 
  // conforming API w.r.t. constness.
  // We choose to fix this trhough this converter in order to 
  // explain/remember why we're doing this and remove it easilly 
  // when support for those MPI is dropped.
  // The fix is as specific (un templatized, for one) as possible 
  // in order to encourage it usage for the probleme at hand.
  // Problematic API include MPI_Send
  inline
  void *unconst(void const* addr) { return const_cast<void*>(addr); }

} } }

#endif
