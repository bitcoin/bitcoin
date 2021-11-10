// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_PARALLEL_ALGORITHM_HPP
#define BOOST_PARALLEL_ALGORITHM_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/optional.hpp>
#include <boost/config.hpp> // for BOOST_STATIC_CONSTANT
#include <vector>

namespace boost { namespace parallel {
  template<typename BinaryOp>
  struct is_commutative
  {
    BOOST_STATIC_CONSTANT(bool, value = false);
  };

  template<typename T>
  struct minimum
  {
    typedef T first_argument_type;
    typedef T second_argument_type;
    typedef T result_type;
    const T& operator()(const T& x, const T& y) const { return x < y? x : y; }
  };

  template<typename T>
  struct maximum
  {
    typedef T first_argument_type;
    typedef T second_argument_type;
    typedef T result_type;
    const T& operator()(const T& x, const T& y) const { return x < y? y : x; }
  };

  template<typename T>
  struct sum
  {
    typedef T first_argument_type;
    typedef T second_argument_type;
    typedef T result_type;
    const T operator()(const T& x, const T& y) const { return x + y; }
  };

  template<typename ProcessGroup, typename InputIterator,
           typename OutputIterator, typename BinaryOperation>
  OutputIterator
  reduce(ProcessGroup pg, typename ProcessGroup::process_id_type root,
         InputIterator first, InputIterator last, OutputIterator out,
         BinaryOperation bin_op);

  template<typename ProcessGroup, typename T, typename BinaryOperation>
  inline T
  all_reduce(ProcessGroup pg, const T& value, BinaryOperation bin_op)
  {
    T result;
    all_reduce(pg,
               const_cast<T*>(&value), const_cast<T*>(&value+1),
               &result, bin_op);
    return result;
  }

  template<typename ProcessGroup, typename T, typename BinaryOperation>
  inline T
  scan(ProcessGroup pg, const T& value, BinaryOperation bin_op)
  {
    T result;
    scan(pg,
         const_cast<T*>(&value), const_cast<T*>(&value+1),
         &result, bin_op);
    return result;
  }


  template<typename ProcessGroup, typename InputIterator, typename T>
  void
  all_gather(ProcessGroup pg, InputIterator first, InputIterator last,
             std::vector<T>& out);
} } // end namespace boost::parallel

#include <boost/graph/parallel/detail/inplace_all_to_all.hpp>

#endif // BOOST_PARALLEL_ALGORITHM_HPP
