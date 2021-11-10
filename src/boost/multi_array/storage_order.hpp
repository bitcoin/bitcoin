// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

#ifndef BOOST_MULTI_ARRAY_STORAGE_ORDER_HPP
#define BOOST_MULTI_ARRAY_STORAGE_ORDER_HPP

#include "boost/multi_array/types.hpp"
#include "boost/array.hpp"
#include "boost/multi_array/algorithm.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>

namespace boost {

  // RG - This is to make things work with VC++. So sad, so sad.
  class c_storage_order; 
  class fortran_storage_order;

  template <std::size_t NumDims>
  class general_storage_order
  {
  public:
    typedef detail::multi_array::size_type size_type;
    template <typename OrderingIter, typename AscendingIter>
    general_storage_order(OrderingIter ordering,
                          AscendingIter ascending) {
      boost::detail::multi_array::copy_n(ordering,NumDims,ordering_.begin());
      boost::detail::multi_array::copy_n(ascending,NumDims,ascending_.begin());
    }

    // RG - ideally these would not be necessary, but some compilers
    // don't like template conversion operators.  I suspect that not
    // too many folk will feel the need to use customized
    // storage_order objects, I sacrifice that feature for compiler support.
    general_storage_order(const c_storage_order&) {
      for (size_type i=0; i != NumDims; ++i) {
        ordering_[i] = NumDims - 1 - i;
      }
      ascending_.assign(true);
    }

    general_storage_order(const fortran_storage_order&) {
      for (size_type i=0; i != NumDims; ++i) {
        ordering_[i] = i;
      }
      ascending_.assign(true);
    }

    size_type ordering(size_type dim) const { return ordering_[dim]; }
    bool ascending(size_type dim) const { return ascending_[dim]; }

    bool all_dims_ascending() const {
      return std::accumulate(ascending_.begin(),ascending_.end(),true,
                      std::logical_and<bool>());
    }

    bool operator==(general_storage_order const& rhs) const {
      return (ordering_ == rhs.ordering_) &&
        (ascending_ == rhs.ascending_);
    }

  protected:
    boost::array<size_type,NumDims> ordering_;
    boost::array<bool,NumDims> ascending_;
  };

  class c_storage_order 
  {
    typedef detail::multi_array::size_type size_type;
  public:
    // This is the idiom for creating your own custom storage orders.
    // Not supported by all compilers though!
#ifndef __MWERKS__ // Metrowerks screams "ambiguity!"
    template <std::size_t NumDims>
    operator general_storage_order<NumDims>() const {
      boost::array<size_type,NumDims> ordering;
      boost::array<bool,NumDims> ascending;

      for (size_type i=0; i != NumDims; ++i) {
        ordering[i] = NumDims - 1 - i;
        ascending[i] = true;
      }
      return general_storage_order<NumDims>(ordering.begin(),
                                            ascending.begin());
    }
#endif
  };

  class fortran_storage_order
  {
    typedef detail::multi_array::size_type size_type;
  public:
    // This is the idiom for creating your own custom storage orders.
    // Not supported by all compilers though! 
#ifndef __MWERKS__ // Metrowerks screams "ambiguity!"
    template <std::size_t NumDims>
    operator general_storage_order<NumDims>() const {
      boost::array<size_type,NumDims> ordering;
      boost::array<bool,NumDims> ascending;

      for (size_type i=0; i != NumDims; ++i) {
        ordering[i] = i;
        ascending[i] = true;
      }
      return general_storage_order<NumDims>(ordering.begin(),
                                            ascending.begin());
    }
#endif
  };

} // namespace boost

#endif
