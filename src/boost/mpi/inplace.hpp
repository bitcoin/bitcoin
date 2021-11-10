// Copyright (C) 2005-2006 Alain Miniussi <alain.miniussi -at- oca.eu>.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Message Passing Interface 1.1 -- Section 4. MPI Collectives

/** @file inplace.hpp
 *
 *  This header provides helpers to indicate to MPI collective operation
 *  that a buffer can be use both as an input and output.
 */
#ifndef BOOST_MPI_INPLACE_HPP
#define BOOST_MPI_INPLACE_HPP

#include <boost/mpi/communicator.hpp>
#include <vector>

namespace boost { namespace mpi {

/**
 *  @brief Wrapper type to explicitly indicate that a input data 
 * can be overriden with an output value.
 */
template <typename T>
struct inplace_t {
  inplace_t(T& inout) : buffer(inout) {}
  T& buffer;
};

template <typename T>
struct inplace_t<T*> {
  inplace_t(T* inout) : buffer(inout) {}
  T* buffer;
};


/**
 *  @brief Wrapp a input data to indicate that it can be overriden 
 *  with an ouput value.
 *  @param inout the contributing input value, it will be overriden 
 *  with the output value where one is expected. If it is a pointer, 
 *  the number of elements will be provided separatly.
 *  @returns The wrapped value or pointer.
 */
template<typename T>
inplace_t<T>
inplace(T& inout) { 
  return inplace_t<T>(inout);
}
/**
 * \overload
 */
template<typename T>
inplace_t<T*>
inplace(T* inout) { 
  return inplace_t<T*>(inout);
}
} }  // end namespace boost::mpi

#endif // BOOST_MPI_INPLACE_HPP

