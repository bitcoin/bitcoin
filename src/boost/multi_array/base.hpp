// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

#ifndef BOOST_MULTI_ARRAY_BASE_HPP
#define BOOST_MULTI_ARRAY_BASE_HPP

//
// base.hpp - some implementation base classes for from which
// functionality is acquired
//

#include "boost/multi_array/extent_range.hpp"
#include "boost/multi_array/extent_gen.hpp"
#include "boost/multi_array/index_range.hpp"
#include "boost/multi_array/index_gen.hpp"
#include "boost/multi_array/storage_order.hpp"
#include "boost/multi_array/types.hpp"
#include "boost/config.hpp"
#include "boost/multi_array/concept_checks.hpp" //for ignore_unused_...
#include "boost/mpl/eval_if.hpp"
#include "boost/mpl/if.hpp"
#include "boost/mpl/size_t.hpp"
#include "boost/iterator/reverse_iterator.hpp"
#include "boost/static_assert.hpp"
#include "boost/type.hpp"
#include "boost/assert.hpp"
#include <cstddef>
#include <memory>

namespace boost {

/////////////////////////////////////////////////////////////////////////
// class declarations
/////////////////////////////////////////////////////////////////////////

template<typename T, std::size_t NumDims,
  typename Allocator = std::allocator<T> >
class multi_array;

// This is a public interface for use by end users!
namespace multi_array_types {
  typedef boost::detail::multi_array::size_type size_type;
  typedef std::ptrdiff_t difference_type;
  typedef boost::detail::multi_array::index index;
  typedef detail::multi_array::index_range<index,size_type> index_range;
  typedef detail::multi_array::extent_range<index,size_type> extent_range;
  typedef detail::multi_array::index_gen<0,0> index_gen;
  typedef detail::multi_array::extent_gen<0> extent_gen;
}


// boost::extents and boost::indices are now a part of the public
// interface.  That way users don't necessarily have to create their 
// own objects.  On the other hand, one may not want the overhead of 
// object creation in small-memory environments.  Thus, the objects
// can be left undefined by defining BOOST_MULTI_ARRAY_NO_GENERATORS 
// before loading multi_array.hpp.
#ifndef BOOST_MULTI_ARRAY_NO_GENERATORS
namespace {
  multi_array_types::extent_gen extents;
  multi_array_types::index_gen indices;
}
#endif // BOOST_MULTI_ARRAY_NO_GENERATORS

namespace detail {
namespace multi_array {

template <typename T, std::size_t NumDims>
class sub_array;

template <typename T, std::size_t NumDims, typename TPtr = const T*>
class const_sub_array;

  template <typename T, typename TPtr, typename NumDims, typename Reference,
            typename IteratorCategory>
class array_iterator;

template <typename T, std::size_t NumDims, typename TPtr = const T*>
class const_multi_array_view;

template <typename T, std::size_t NumDims>
class multi_array_view;

/////////////////////////////////////////////////////////////////////////
// class interfaces
/////////////////////////////////////////////////////////////////////////

class multi_array_base {
public:
  typedef multi_array_types::size_type size_type;
  typedef multi_array_types::difference_type difference_type;
  typedef multi_array_types::index index;
  typedef multi_array_types::index_range index_range;
  typedef multi_array_types::extent_range extent_range;
  typedef multi_array_types::index_gen index_gen;
  typedef multi_array_types::extent_gen extent_gen;
};

//
// value_accessor_n
//  contains the routines for accessing elements from
//  N-dimensional views.
//
template<typename T, std::size_t NumDims>
class value_accessor_n : public multi_array_base {
  typedef multi_array_base super_type;
public:
  typedef typename super_type::index index;

  // 
  // public typedefs used by classes that inherit from this base
  //
  typedef T element;
  typedef boost::multi_array<T,NumDims-1> value_type;
  typedef sub_array<T,NumDims-1> reference;
  typedef const_sub_array<T,NumDims-1> const_reference;

protected:
  // used by array operator[] and iterators to get reference types.
  template <typename Reference, typename TPtr>
  Reference access(boost::type<Reference>,index idx,TPtr base,
                   const size_type* extents,
                   const index* strides,
                   const index* index_bases) const {

    BOOST_ASSERT(idx - index_bases[0] >= 0);
    BOOST_ASSERT(size_type(idx - index_bases[0]) < extents[0]);
    // return a sub_array<T,NDims-1> proxy object
    TPtr newbase = base + idx * strides[0];
    return Reference(newbase,extents+1,strides+1,index_bases+1);

  }

  value_accessor_n() { }
  ~value_accessor_n() { }
};



//
// value_accessor_one
//  contains the routines for accessing reference elements from
//  1-dimensional views.
//
template<typename T>
class value_accessor_one : public multi_array_base {
  typedef multi_array_base super_type;
public:
  typedef typename super_type::index index;
  //
  // public typedefs for use by classes that inherit it.
  //
  typedef T element;
  typedef T value_type;
  typedef T& reference;
  typedef T const& const_reference;

protected:
  // used by array operator[] and iterators to get reference types.
  template <typename Reference, typename TPtr>
  Reference access(boost::type<Reference>,index idx,TPtr base,
                   const size_type* extents,
                   const index* strides,
                   const index* index_bases) const {

    ignore_unused_variable_warning(index_bases);
    ignore_unused_variable_warning(extents);
    BOOST_ASSERT(idx - index_bases[0] >= 0);
    BOOST_ASSERT(size_type(idx - index_bases[0]) < extents[0]);
    return *(base + idx * strides[0]);
  }

  value_accessor_one() { }
  ~value_accessor_one() { }
};


/////////////////////////////////////////////////////////////////////////
// choose value accessor begins
//

template <typename T, std::size_t NumDims>
struct choose_value_accessor_n {
  typedef value_accessor_n<T,NumDims> type;
};

template <typename T>
struct choose_value_accessor_one {
  typedef value_accessor_one<T> type;
};

template <typename T, typename NumDims>
struct value_accessor_generator {
    BOOST_STATIC_CONSTANT(std::size_t, dimensionality = NumDims::value);
    
  typedef typename
  mpl::eval_if_c<(dimensionality == 1),
                  choose_value_accessor_one<T>,
                  choose_value_accessor_n<T,dimensionality>
  >::type type;
};

template <class T, class NumDims>
struct associated_types
  : value_accessor_generator<T,NumDims>::type
{};

//
// choose value accessor ends
/////////////////////////////////////////////////////////////////////////

// Due to some imprecision in the C++ Standard, 
// MSVC 2010 is broken in debug mode: it requires
// that an Output Iterator have output_iterator_tag in its iterator_category if 
// that iterator is not bidirectional_iterator or random_access_iterator.
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1600)
struct mutable_iterator_tag
 : boost::random_access_traversal_tag, std::input_iterator_tag
{
  operator std::output_iterator_tag() const {
    return std::output_iterator_tag();
  }
};
#endif

////////////////////////////////////////////////////////////////////////
// multi_array_base
////////////////////////////////////////////////////////////////////////
template <typename T, std::size_t NumDims>
class multi_array_impl_base
  :
      public value_accessor_generator<T,mpl::size_t<NumDims> >::type
{
  typedef associated_types<T,mpl::size_t<NumDims> > types;
public:
  typedef typename types::index index;
  typedef typename types::size_type size_type;
  typedef typename types::element element;
  typedef typename types::index_range index_range;
  typedef typename types::value_type value_type;
  typedef typename types::reference reference;
  typedef typename types::const_reference const_reference;

  template <std::size_t NDims>
  struct subarray {
    typedef boost::detail::multi_array::sub_array<T,NDims> type;
  };

  template <std::size_t NDims>
  struct const_subarray {
    typedef boost::detail::multi_array::const_sub_array<T,NDims> type;
  };

  template <std::size_t NDims>
  struct array_view {
    typedef boost::detail::multi_array::multi_array_view<T,NDims> type;
  };

  template <std::size_t NDims>
  struct const_array_view {
  public:
    typedef boost::detail::multi_array::const_multi_array_view<T,NDims> type;
  };

  //
  // iterator support
  //
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1600)
  // Deal with VC 2010 output_iterator_tag requirement
  typedef array_iterator<T,T*,mpl::size_t<NumDims>,reference,
                         mutable_iterator_tag> iterator;
#else
  typedef array_iterator<T,T*,mpl::size_t<NumDims>,reference,
                         boost::random_access_traversal_tag> iterator;
#endif
  typedef array_iterator<T,T const*,mpl::size_t<NumDims>,const_reference,
                         boost::random_access_traversal_tag> const_iterator;

  typedef ::boost::reverse_iterator<iterator> reverse_iterator;
  typedef ::boost::reverse_iterator<const_iterator> const_reverse_iterator;

  BOOST_STATIC_CONSTANT(std::size_t, dimensionality = NumDims);
protected:

  multi_array_impl_base() { }
  ~multi_array_impl_base() { }

  // Used by operator() in our array classes
  template <typename Reference, typename IndexList, typename TPtr>
  Reference access_element(boost::type<Reference>,
                           const IndexList& indices,
                           TPtr base,
                           const size_type* extents,
                           const index* strides,
                           const index* index_bases) const {
    boost::function_requires<
      CollectionConcept<IndexList> >();
    ignore_unused_variable_warning(index_bases);
    ignore_unused_variable_warning(extents);
#if !defined(NDEBUG) && !defined(BOOST_DISABLE_ASSERTS)
    for (size_type i = 0; i != NumDims; ++i) {
      BOOST_ASSERT(indices[i] - index_bases[i] >= 0);
      BOOST_ASSERT(size_type(indices[i] - index_bases[i]) < extents[i]);
    }
#endif

    index offset = 0;
    {
      typename IndexList::const_iterator i = indices.begin();
      size_type n = 0; 
      while (n != NumDims) {
        offset += (*i) * strides[n];
        ++n;
        ++i;
      }
    }
    return base[offset];
  }

  template <typename StrideList, typename ExtentList>
  void compute_strides(StrideList& stride_list, ExtentList& extent_list,
                       const general_storage_order<NumDims>& storage)
  {
    // invariant: stride = the stride for dimension n
    index stride = 1;
    for (size_type n = 0; n != NumDims; ++n) {
      index stride_sign = +1;
      
      if (!storage.ascending(storage.ordering(n)))
        stride_sign = -1;
      
      // The stride for this dimension is the product of the
      // lengths of the ranks minor to it.
      stride_list[storage.ordering(n)] = stride * stride_sign;
      
      stride *= extent_list[storage.ordering(n)];
    } 
  }

  // This calculates the offset to the array base pointer due to:
  // 1. dimensions stored in descending order
  // 2. non-zero dimension index bases
  template <typename StrideList, typename ExtentList, typename BaseList>
  index
  calculate_origin_offset(const StrideList& stride_list,
                          const ExtentList& extent_list,
                          const general_storage_order<NumDims>& storage,
                          const BaseList& index_base_list)
  {
    return
      calculate_descending_dimension_offset(stride_list,extent_list,
                                            storage) +
      calculate_indexing_offset(stride_list,index_base_list);
  }

  // This calculates the offset added to the base pointer that are
  // caused by descending dimensions
  template <typename StrideList, typename ExtentList>
  index
  calculate_descending_dimension_offset(const StrideList& stride_list,
                                const ExtentList& extent_list,
                                const general_storage_order<NumDims>& storage)
  {
    index offset = 0;
    if (!storage.all_dims_ascending()) 
      for (size_type n = 0; n != NumDims; ++n)
        if (!storage.ascending(n))
          offset -= (extent_list[n] - 1) * stride_list[n];

    return offset;
  }

  // This is used to reindex array_views, which are no longer
  // concerned about storage order (specifically, whether dimensions
  // are ascending or descending) since the viewed array handled it.

  template <typename StrideList, typename BaseList>
  index
  calculate_indexing_offset(const StrideList& stride_list,
                          const BaseList& index_base_list)
  {
    index offset = 0;
    for (size_type n = 0; n != NumDims; ++n)
        offset -= stride_list[n] * index_base_list[n];
    return offset;
  }

  // Slicing using an index_gen.
  // Note that populating an index_gen creates a type that encodes
  // both the number of dimensions in the current Array (NumDims), and 
  // the Number of dimensions for the resulting view.  This allows the 
  // compiler to fail if the dimensions aren't completely accounted
  // for.  For reasons unbeknownst to me, a BOOST_STATIC_ASSERT
  // within the member function template does not work. I should add a 
  // note to the documentation specifying that you get a damn ugly
  // error message if you screw up in your slicing code.
  template <typename ArrayRef, int NDims, typename TPtr>
  ArrayRef
  generate_array_view(boost::type<ArrayRef>,
                      const boost::detail::multi_array::
                      index_gen<NumDims,NDims>& indices,
                      const size_type* extents,
                      const index* strides,
                      const index* index_bases,
                      TPtr base) const {

    boost::array<index,NDims> new_strides;
    boost::array<index,NDims> new_extents;

    index offset = 0;
    size_type dim = 0;
    for (size_type n = 0; n != NumDims; ++n) {

      // Use array specs and input specs to produce real specs.
      const index default_start = index_bases[n];
      const index default_finish = default_start+extents[n];
      const index_range& current_range = indices.ranges_[n];
      index start = current_range.get_start(default_start);
      index finish = current_range.get_finish(default_finish);
      index stride = current_range.stride();
      BOOST_ASSERT(stride != 0);

      // An index range indicates a half-open strided interval 
      // [start,finish) (with stride) which faces upward when stride 
      // is positive and downward when stride is negative, 

      // RG: The following code for calculating length suffers from 
      // some representation issues: if finish-start cannot be represented as
      // by type index, then overflow may result.

      index len;
      if ((finish - start) / stride < 0) {
        // [start,finish) is empty according to the direction imposed by 
        // the stride.
        len = 0;
      } else {
        // integral trick for ceiling((finish-start) / stride) 
        // taking into account signs.
        index shrinkage = stride > 0 ? 1 : -1;
        len = (finish - start + (stride - shrinkage)) / stride;
      }

      // start marks the closed side of the range, so it must lie
      // exactly in the set of legal indices
      // with a special case for empty arrays
      BOOST_ASSERT(index_bases[n] <= start &&
                   ((start <= index_bases[n]+index(extents[n])) ||
                     (start == index_bases[n] && extents[n] == 0)));

#ifndef BOOST_DISABLE_ASSERTS
      // finish marks the open side of the range, so it can go one past
      // the "far side" of the range (the top if stride is positive, the bottom
      // if stride is negative).
      index bound_adjustment = stride < 0 ? 1 : 0;
      BOOST_ASSERT(((index_bases[n] - bound_adjustment) <= finish) &&
        (finish <= (index_bases[n] + index(extents[n]) - bound_adjustment)));
      ignore_unused_variable_warning(bound_adjustment);
#endif // BOOST_DISABLE_ASSERTS


      // the array data pointer is modified to account for non-zero
      // bases during slicing (see [Garcia] for the math involved)
      offset += start * strides[n];

      if (!current_range.is_degenerate()) {

        // The stride for each dimension is included into the
        // strides for the array_view (see [Garcia] for the math involved).
        new_strides[dim] = stride * strides[n];
        
        // calculate new extents
        new_extents[dim] = len;
        ++dim;
      }
    }
    BOOST_ASSERT(dim == NDims);

    return
      ArrayRef(base+offset,
               new_extents,
               new_strides);
  }
                     

};

} // namespace multi_array
} // namespace detail

} // namespace boost

#endif
