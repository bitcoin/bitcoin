// Copyright 2002 The Trustees of Indiana University.

// Copyright 2018 Glen Joseph Fernandes
// (glenjofe@gmail.com)

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

#ifndef BOOST_MULTI_ARRAY_HPP
#define BOOST_MULTI_ARRAY_HPP

//
// multi_array.hpp - contains the multi_array class template
// declaration and definition
//

#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 406)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wshadow"
#endif

#include "boost/multi_array/base.hpp"
#include "boost/multi_array/collection_concept.hpp"
#include "boost/multi_array/copy_array.hpp"
#include "boost/multi_array/iterator.hpp"
#include "boost/multi_array/subarray.hpp"
#include "boost/multi_array/multi_array_ref.hpp"
#include "boost/multi_array/algorithm.hpp"
#include "boost/core/alloc_construct.hpp"
#include "boost/core/empty_value.hpp"
#include "boost/array.hpp"
#include "boost/mpl/if.hpp"
#include "boost/type_traits.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>



namespace boost {
  namespace detail {
    namespace multi_array {

      struct populate_index_ranges {
        multi_array_types::index_range
        // RG: underscore on extent_ to stifle strange MSVC warning.
        operator()(multi_array_types::index base,
                   multi_array_types::size_type extent_) {
          return multi_array_types::index_range(base,base+extent_);
        }
      };

#ifdef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
//
// Compilers that don't support partial ordering may need help to
// disambiguate multi_array's templated constructors.  Even vc6/7 are
// capable of some limited SFINAE, so we take the most-general version
// out of the overload set with disable_multi_array_impl.
//
template <typename T, std::size_t NumDims, typename TPtr>
char is_multi_array_impl_help(const_multi_array_view<T,NumDims,TPtr>&);
template <typename T, std::size_t NumDims, typename TPtr>
char is_multi_array_impl_help(const_sub_array<T,NumDims,TPtr>&);
template <typename T, std::size_t NumDims, typename TPtr>
char is_multi_array_impl_help(const_multi_array_ref<T,NumDims,TPtr>&);

char ( &is_multi_array_impl_help(...) )[2];

template <class T>
struct is_multi_array_impl
{
    static T x;
    BOOST_STATIC_CONSTANT(bool, value = sizeof((is_multi_array_impl_help)(x)) == 1);

  typedef mpl::bool_<value> type;
};

template <bool multi_array = false>
struct disable_multi_array_impl_impl
{
    typedef int type;
};

template <>
struct disable_multi_array_impl_impl<true>
{
    // forming a pointer to a reference triggers SFINAE
    typedef int& type; 
};


template <class T>
struct disable_multi_array_impl :
  disable_multi_array_impl_impl<is_multi_array_impl<T>::value>
{ };


template <>
struct disable_multi_array_impl<int>
{
  typedef int type;
};


#endif

    } //namespace multi_array
  } // namespace detail

template<typename T, std::size_t NumDims,
  typename Allocator>
class multi_array :
  public multi_array_ref<T,NumDims>,
  private boost::empty_value<Allocator>
{
  typedef boost::empty_value<Allocator> alloc_base;
  typedef multi_array_ref<T,NumDims> super_type;
public:
  typedef typename super_type::value_type value_type;
  typedef typename super_type::reference reference;
  typedef typename super_type::const_reference const_reference;
  typedef typename super_type::iterator iterator;
  typedef typename super_type::const_iterator const_iterator;
  typedef typename super_type::reverse_iterator reverse_iterator;
  typedef typename super_type::const_reverse_iterator const_reverse_iterator;
  typedef typename super_type::element element;
  typedef typename super_type::size_type size_type;
  typedef typename super_type::difference_type difference_type;
  typedef typename super_type::index index;
  typedef typename super_type::extent_range extent_range;


  template <std::size_t NDims>
  struct const_array_view {
    typedef boost::detail::multi_array::const_multi_array_view<T,NDims> type;
  };

  template <std::size_t NDims>
  struct array_view {
    typedef boost::detail::multi_array::multi_array_view<T,NDims> type;
  };

  explicit multi_array(const Allocator& alloc = Allocator()) :
    super_type((T*)initial_base_,c_storage_order(),
               /*index_bases=*/0, /*extents=*/0),
    alloc_base(boost::empty_init_t(),alloc) {
    allocate_space(); 
  }

  template <class ExtentList>
  explicit multi_array(
      ExtentList const& extents,
      const Allocator& alloc = Allocator()
#ifdef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
      , typename mpl::if_<
      detail::multi_array::is_multi_array_impl<ExtentList>,
      int&,int>::type* = 0
#endif
      ) :
    super_type((T*)initial_base_,extents),
    alloc_base(boost::empty_init_t(),alloc) {
    boost::function_requires<
      detail::multi_array::CollectionConcept<ExtentList> >();
    allocate_space();
  }

    
  template <class ExtentList>
  explicit multi_array(ExtentList const& extents,
                       const general_storage_order<NumDims>& so) :
    super_type((T*)initial_base_,extents,so),
    alloc_base(boost::empty_init_t()) {
    boost::function_requires<
      detail::multi_array::CollectionConcept<ExtentList> >();
    allocate_space();
  }

  template <class ExtentList>
  explicit multi_array(ExtentList const& extents,
                       const general_storage_order<NumDims>& so,
                       Allocator const& alloc) :
    super_type((T*)initial_base_,extents,so),
    alloc_base(boost::empty_init_t(),alloc) {
    boost::function_requires<
      detail::multi_array::CollectionConcept<ExtentList> >();
    allocate_space();
  }


  explicit multi_array(const detail::multi_array
                       ::extent_gen<NumDims>& ranges,
                       const Allocator& alloc = Allocator()) :
    super_type((T*)initial_base_,ranges),
    alloc_base(boost::empty_init_t(),alloc) {

    allocate_space();
  }


  explicit multi_array(const detail::multi_array
                       ::extent_gen<NumDims>& ranges,
                       const general_storage_order<NumDims>& so) :
    super_type((T*)initial_base_,ranges,so),
    alloc_base(boost::empty_init_t()) {

    allocate_space();
  }


  explicit multi_array(const detail::multi_array
                       ::extent_gen<NumDims>& ranges,
                       const general_storage_order<NumDims>& so,
                       Allocator const& alloc) :
    super_type((T*)initial_base_,ranges,so),
    alloc_base(boost::empty_init_t(),alloc) {

    allocate_space();
  }

  multi_array(const multi_array& rhs) :
  super_type(rhs),
  alloc_base(static_cast<const alloc_base&>(rhs)) {
    allocate_space();
    boost::detail::multi_array::copy_n(rhs.base_,rhs.num_elements(),base_);
  }


  //
  // A multi_array is constructible from any multi_array_ref, subarray, or
  // array_view object.  The following constructors ensure that.
  //

  // Due to limited support for partial template ordering, 
  // MSVC 6&7 confuse the following with the most basic ExtentList 
  // constructor.
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  template <typename OPtr>
  multi_array(const const_multi_array_ref<T,NumDims,OPtr>& rhs,
              const general_storage_order<NumDims>& so = c_storage_order(),
              const Allocator& alloc = Allocator())
    : super_type(0,so,rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    // Warning! storage order may change, hence the following copy technique.
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }

  template <typename OPtr>
  multi_array(const detail::multi_array::
              const_sub_array<T,NumDims,OPtr>& rhs,
              const general_storage_order<NumDims>& so = c_storage_order(),
              const Allocator& alloc = Allocator())
    : super_type(0,so,rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }


  template <typename OPtr>
  multi_array(const detail::multi_array::
              const_multi_array_view<T,NumDims,OPtr>& rhs,
              const general_storage_order<NumDims>& so = c_storage_order(),
              const Allocator& alloc = Allocator())
    : super_type(0,so,rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }

#else // BOOST_NO_FUNCTION_TEMPLATE_ORDERING
  // More limited support for MSVC


  multi_array(const const_multi_array_ref<T,NumDims>& rhs,
              const Allocator& alloc = Allocator())
    : super_type(0,c_storage_order(),rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    // Warning! storage order may change, hence the following copy technique.
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }

  multi_array(const const_multi_array_ref<T,NumDims>& rhs,
              const general_storage_order<NumDims>& so,
              const Allocator& alloc = Allocator())
    : super_type(0,so,rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    // Warning! storage order may change, hence the following copy technique.
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }

  multi_array(const detail::multi_array::
              const_sub_array<T,NumDims>& rhs,
              const Allocator& alloc = Allocator())
    : super_type(0,c_storage_order(),rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }

  multi_array(const detail::multi_array::
              const_sub_array<T,NumDims>& rhs,
              const general_storage_order<NumDims>& so,
              const Allocator& alloc = Allocator())
    : super_type(0,so,rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }


  multi_array(const detail::multi_array::
              const_multi_array_view<T,NumDims>& rhs,
              const Allocator& alloc = Allocator())
    : super_type(0,c_storage_order(),rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }

  multi_array(const detail::multi_array::
              const_multi_array_view<T,NumDims>& rhs,
              const general_storage_order<NumDims>& so,
              const Allocator& alloc = Allocator())
    : super_type(0,so,rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }

#endif // !BOOST_NO_FUNCTION_TEMPLATE_ORDERING

  // Thes constructors are necessary because of more exact template matches.
  multi_array(const multi_array_ref<T,NumDims>& rhs,
              const Allocator& alloc = Allocator())
    : super_type(0,c_storage_order(),rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    // Warning! storage order may change, hence the following copy technique.
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }

  multi_array(const multi_array_ref<T,NumDims>& rhs,
              const general_storage_order<NumDims>& so,
              const Allocator& alloc = Allocator())
    : super_type(0,so,rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    // Warning! storage order may change, hence the following copy technique.
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }


  multi_array(const detail::multi_array::
              sub_array<T,NumDims>& rhs,
              const Allocator& alloc = Allocator())
    : super_type(0,c_storage_order(),rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }

  multi_array(const detail::multi_array::
              sub_array<T,NumDims>& rhs,
              const general_storage_order<NumDims>& so,
              const Allocator& alloc = Allocator())
    : super_type(0,so,rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }


  multi_array(const detail::multi_array::
              multi_array_view<T,NumDims>& rhs,
              const Allocator& alloc = Allocator())
    : super_type(0,c_storage_order(),rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }
    
  multi_array(const detail::multi_array::
              multi_array_view<T,NumDims>& rhs,
              const general_storage_order<NumDims>& so,
              const Allocator& alloc = Allocator())
    : super_type(0,so,rhs.index_bases(),rhs.shape()),
      alloc_base(boost::empty_init_t(),alloc)
  {
    allocate_space();
    std::copy(rhs.begin(),rhs.end(),this->begin());
  }
    
  // Since assignment is a deep copy, multi_array_ref
  // contains all the necessary code.
  template <typename ConstMultiArray>
  multi_array& operator=(const ConstMultiArray& other) {
    super_type::operator=(other);
    return *this;
  }

  multi_array& operator=(const multi_array& other) {
    if (&other != this) {
      super_type::operator=(other);
    }
    return *this;
  }


  template <typename ExtentList>
  multi_array& resize(const ExtentList& extents) {
    boost::function_requires<
      detail::multi_array::CollectionConcept<ExtentList> >();

    typedef detail::multi_array::extent_gen<NumDims> gen_type;
    gen_type ranges;

    for (int i=0; i != NumDims; ++i) {
      typedef typename gen_type::range range_type;
      ranges.ranges_[i] = range_type(0,extents[i]);
    }
    
    return this->resize(ranges);
  }



  multi_array& resize(const detail::multi_array
                      ::extent_gen<NumDims>& ranges) {


    // build a multi_array with the specs given
    multi_array new_array(ranges,this->storage_order(),allocator());


    // build a view of tmp with the minimum extents

    // Get the minimum extents of the arrays.
    boost::array<size_type,NumDims> min_extents;

    const size_type& (*min)(const size_type&, const size_type&) =
      std::min;
    std::transform(new_array.extent_list_.begin(),new_array.extent_list_.end(),
                   this->extent_list_.begin(),
                   min_extents.begin(),
                   min);


    // typedef boost::array<index,NumDims> index_list;
    // Build index_gen objects to create views with the same shape

    // these need to be separate to handle non-zero index bases
    typedef detail::multi_array::index_gen<NumDims,NumDims> index_gen;
    index_gen old_idxes;
    index_gen new_idxes;

    std::transform(new_array.index_base_list_.begin(),
                   new_array.index_base_list_.end(),
                   min_extents.begin(),new_idxes.ranges_.begin(),
                   detail::multi_array::populate_index_ranges());

    std::transform(this->index_base_list_.begin(),
                   this->index_base_list_.end(),
                   min_extents.begin(),old_idxes.ranges_.begin(),
                   detail::multi_array::populate_index_ranges());

    // Build same-shape views of the two arrays
    typename
      multi_array::BOOST_NESTED_TEMPLATE array_view<NumDims>::type view_old = (*this)[old_idxes];
    typename
      multi_array::BOOST_NESTED_TEMPLATE array_view<NumDims>::type view_new = new_array[new_idxes];

    // Set the right portion of the new array
    view_new = view_old;

    using std::swap;
    // Swap the internals of these arrays.
    swap(this->super_type::base_,new_array.super_type::base_);
    swap(this->allocator(),new_array.allocator());
    swap(this->storage_,new_array.storage_);
    swap(this->extent_list_,new_array.extent_list_);
    swap(this->stride_list_,new_array.stride_list_);
    swap(this->index_base_list_,new_array.index_base_list_);
    swap(this->origin_offset_,new_array.origin_offset_);
    swap(this->directional_offset_,new_array.directional_offset_);
    swap(this->num_elements_,new_array.num_elements_);
    swap(this->base_,new_array.base_);
    swap(this->allocated_elements_,new_array.allocated_elements_);

    return *this;
  }


  ~multi_array() {
    deallocate_space();
  }

private:
  friend inline bool operator==(const multi_array& a, const multi_array& b) {
    return a.base() == b.base();
  }

  friend inline bool operator!=(const multi_array& a, const multi_array& b) {
    return !(a == b);
  }

  const super_type& base() const {
    return *this;
  }

  const Allocator& allocator() const {
    return alloc_base::get();
  }

  Allocator& allocator() {
    return alloc_base::get();
  }

  void allocate_space() {
    base_ = allocator().allocate(this->num_elements());
    this->set_base_ptr(base_);
    allocated_elements_ = this->num_elements();
    boost::alloc_construct_n(allocator(),base_,allocated_elements_);
  }

  void deallocate_space() {
    if(base_) {
      boost::alloc_destroy_n(allocator(),base_,allocated_elements_);
      allocator().deallocate(base_,allocated_elements_);
    }
  }

  typedef boost::array<size_type,NumDims> size_list;
  typedef boost::array<index,NumDims> index_list;

  T* base_;
  size_type allocated_elements_;
  enum {initial_base_ = 0};
};

} // namespace boost

#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 406)
#  pragma GCC diagnostic pop
#endif

#endif
