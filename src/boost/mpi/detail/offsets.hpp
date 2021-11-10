//          Copyright Alain Miniussi 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Authors: Alain Miniussi

#ifndef BOOST_MPI_OFFSETS_HPP
#define BOOST_MPI_OFFSETS_HPP

#include <vector>
#include <boost/mpi/config.hpp>
#include <boost/mpi/communicator.hpp>

namespace boost { namespace mpi {
namespace detail {

// Convert a sequence of sizes [S0..Sn] to a sequence displacement 
// [O0..On] where O[0] = 0 and O[k+1] = O[k]+S[k].
void BOOST_MPI_DECL sizes2offsets(int const* sizes, int* offsets, int n);

// Same as size2offset(sizes.data(), offsets.data(), sizes.size())
void BOOST_MPI_DECL sizes2offsets(std::vector<int> const& sizes, std::vector<int>& offsets);

// Given a sequence of sizes (typically the number of records dispatched
// to each process in a scater) and a sequence of displacements (typically the
// slot index at with those record starts), convert the later to a number 
// of skipped slots.
void offsets2skipped(int const* sizes, int const* offsets, int* skipped, int n);

// Reconstruct offsets from sizes assuming no padding.
// Only takes place if on the root process and if 
// displs are not already provided.
// If memory was allocated, returns a pointer to it
// otherwise null.
int* make_offsets(communicator const& comm, int const* sizes, int const* displs, int root = -1);

// Reconstruct skip slots from sizes and offsets.
// Only takes place if on the root process and if 
// displs are provided.
// If memory was allocated, returns a pointer to it
// otherwise null.
int* make_skipped_slots(communicator const& comm, int const* sizes, int const* displs, int root = -1);

}
}}// end namespace boost::mpi

#endif // BOOST_MPI_OFFSETS_HPP
