// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

#ifndef BOOST_MULTI_ARRAY_MULTI_ARRAY_REF_HPP
#define BOOST_MULTI_ARRAY_MULTI_ARRAY_REF_HPP

//
// multi_array_ref.hpp - code for creating "views" of array data.
//

#include "boost/multi_array/base.hpp"
#include "boost/multi_array/collection_concept.hpp"
#include "boost/multi_array/concept_checks.hpp"
#include "boost/multi_array/iterator.hpp"
#include "boost/multi_array/storage_order.hpp"
#include "boost/multi_array/subarray.hpp"
#include "boost/multi_array/view.hpp"
#include "boost/multi_array/algorithm.hpp"
#include "boost/type_traits/is_integral.hpp"
#include "boost/utility/enable_if.hpp"
#include "boost/array.hpp"
#include "boost/concept_check.hpp"
#include "boost/functional.hpp"
#include "boost/limits.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <numeric>

namespace boost {

template <typename T, std::size_t NumDims,
  typename TPtr = const T*
>
class const_multi_array_ref :
    public detail::multi_array::multi_array_impl_base<T,NumDims>
{
  typedef detail::multi_array::multi_array_impl_base<T,NumDims> super_type;
public: 
  typedef typename super_type::value_type value_type;
  typedef typename super_type::const_reference const_reference;
  typedef typename super_type::const_iterator const_iterator;
  typedef typename super_type::const_reverse_iterator const_reverse_iterator;
  typedef typename super_type::element element;
  typedef typename super_type::size_type size_type;
  typedef typename super_type::difference_type difference_type;
  typedef typename super_type::index index;
  typedef typename super_type::extent_range extent_range;
  typedef general_storage_order<NumDims> storage_order_type;

  // template typedefs
  template <std::size_t NDims>
  struct const_array_view {
    typedef boost::detail::multi_array::const_multi_array_view<T,NDims> type;
  };

  template <std::size_t NDims>
  struct array_view {
    typedef boost::detail::multi_array::multi_array_view<T,NDims> type;
  };

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
  // make const_multi_array_ref a friend of itself
  template <typename,std::size_t,typename>
  friend class const_multi_array_ref;
#endif

  // This ensures that const_multi_array_ref types with different TPtr 
  // types can convert to each other
  template <typename OPtr>
  const_multi_array_ref(const const_multi_array_ref<T,NumDims,OPtr>& other)
    : base_(other.base_), storage_(other.storage_),
      extent_list_(other.extent_list_),
      stride_list_(other.stride_list_),
      index_base_list_(other.index_base_list_),
      origin_offset_(other.origin_offset_),
      directional_offset_(other.directional_offset_),
      num_elements_(other.num_elements_)  {  }

  template <typename ExtentList>
  explicit const_multi_array_ref(TPtr base, const ExtentList& extents) :
    base_(base), storage_(c_storage_order()) {
    boost::function_requires<
      CollectionConcept<ExtentList> >();

    index_base_list_.assign(0);
    init_multi_array_ref(extents.begin());
  }
  
  template <typename ExtentList>
  explicit const_multi_array_ref(TPtr base, const ExtentList& extents,
                       const general_storage_order<NumDims>& so) : 
    base_(base), storage_(so) {
    boost::function_requires<
      CollectionConcept<ExtentList> >();

    index_base_list_.assign(0);
    init_multi_array_ref(extents.begin());
  }
  
  explicit const_multi_array_ref(TPtr base,
                         const detail::multi_array::
                         extent_gen<NumDims>& ranges) :
    base_(base), storage_(c_storage_order()) {

    init_from_extent_gen(ranges);
  }
  
  explicit const_multi_array_ref(TPtr base,
                           const detail::multi_array::
                           extent_gen<NumDims>& ranges,
                           const general_storage_order<NumDims>& so) :
    base_(base), storage_(so) {

    init_from_extent_gen(ranges);
  }
  
  template <class InputIterator>
  void assign(InputIterator begin, InputIterator end) {
    boost::function_requires<InputIteratorConcept<InputIterator> >();

    InputIterator in_iter = begin;
    T* out_iter = base_;
    std::size_t copy_count=0;
    while (in_iter != end && copy_count < num_elements_) {
      *out_iter++ = *in_iter++;
      copy_count++;      
    }
  }

  template <class BaseList>
#ifdef BOOST_NO_SFINAE
  void
#else
  typename
  disable_if<typename boost::is_integral<BaseList>::type,void >::type
#endif // BOOST_NO_SFINAE
  reindex(const BaseList& values) {
    boost::function_requires<
      CollectionConcept<BaseList> >();
    boost::detail::multi_array::
      copy_n(values.begin(),num_dimensions(),index_base_list_.begin());
    origin_offset_ =
      this->calculate_origin_offset(stride_list_,extent_list_,
                              storage_,index_base_list_);
  }

  void reindex(index value) {
    index_base_list_.assign(value);
    origin_offset_ =
      this->calculate_origin_offset(stride_list_,extent_list_,
                              storage_,index_base_list_);
  }

  template <typename SizeList>
  void reshape(const SizeList& extents) {
    boost::function_requires<
      CollectionConcept<SizeList> >();
    BOOST_ASSERT(num_elements_ ==
                 std::accumulate(extents.begin(),extents.end(),
                                 size_type(1),std::multiplies<size_type>()));

    std::copy(extents.begin(),extents.end(),extent_list_.begin());
    this->compute_strides(stride_list_,extent_list_,storage_);

    origin_offset_ =
      this->calculate_origin_offset(stride_list_,extent_list_,
                              storage_,index_base_list_);
  }

  size_type num_dimensions() const { return NumDims; }

  size_type size() const { return extent_list_.front(); }

  // given reshaping functionality, this is the max possible size.
  size_type max_size() const { return num_elements(); }

  bool empty() const { return size() == 0; }

  const size_type* shape() const {
    return extent_list_.data();
  }

  const index* strides() const {
    return stride_list_.data();
  }

  const element* origin() const { return base_+origin_offset_; }
  const element* data() const { return base_; }

  size_type num_elements() const { return num_elements_; }

  const index* index_bases() const {
    return index_base_list_.data();
  }


  const storage_order_type& storage_order() const {
    return storage_;
  }

  template <typename IndexList>
  const element& operator()(IndexList indices) const {
    boost::function_requires<
      CollectionConcept<IndexList> >();
    return super_type::access_element(boost::type<const element&>(),
                                      indices,origin(),
                                      shape(),strides(),index_bases());
  }

  // Only allow const element access
  const_reference operator[](index idx) const {
    return super_type::access(boost::type<const_reference>(),
                              idx,origin(),
                              shape(),strides(),index_bases());
  }

  // see generate_array_view in base.hpp
  template <int NDims>
  typename const_array_view<NDims>::type 
  operator[](const detail::multi_array::
             index_gen<NumDims,NDims>& indices)
    const {
    typedef typename const_array_view<NDims>::type return_type;
    return
      super_type::generate_array_view(boost::type<return_type>(),
                                      indices,
                                      shape(),
                                      strides(),
                                      index_bases(),
                                      origin());
  }
  
  const_iterator begin() const {
    return const_iterator(*index_bases(),origin(),
                          shape(),strides(),index_bases());
  }

  const_iterator end() const {
    return const_iterator(*index_bases()+(index)*shape(),origin(),
                          shape(),strides(),index_bases());
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }


  template <typename OPtr>
  bool operator==(const
                  const_multi_array_ref<T,NumDims,OPtr>& rhs)
    const {
    if(std::equal(extent_list_.begin(),
                  extent_list_.end(),
                  rhs.extent_list_.begin()))
      return std::equal(begin(),end(),rhs.begin());
    else return false;
  }

  template <typename OPtr>
  bool operator<(const
                 const_multi_array_ref<T,NumDims,OPtr>& rhs)
    const {
    return std::lexicographical_compare(begin(),end(),rhs.begin(),rhs.end());
  }

  template <typename OPtr>
  bool operator!=(const
                  const_multi_array_ref<T,NumDims,OPtr>& rhs)
    const {
    return !(*this == rhs);
  }

  template <typename OPtr>
  bool operator>(const
                 const_multi_array_ref<T,NumDims,OPtr>& rhs)
    const {
    return rhs < *this;
  }

  template <typename OPtr>
  bool operator<=(const
                 const_multi_array_ref<T,NumDims,OPtr>& rhs)
    const {
    return !(*this > rhs);
  }

  template <typename OPtr>
  bool operator>=(const
                 const_multi_array_ref<T,NumDims,OPtr>& rhs)
    const {
    return !(*this < rhs);
  }


#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
protected:
#else
public:
#endif

  typedef boost::array<size_type,NumDims> size_list;
  typedef boost::array<index,NumDims> index_list;

  // This is used by multi_array, which is a subclass of this
  void set_base_ptr(TPtr new_base) { base_ = new_base; }


  // This constructor supports multi_array's default constructor
  // and constructors from multi_array_ref, subarray, and array_view
  explicit
  const_multi_array_ref(TPtr base,
                        const storage_order_type& so,
                        const index * index_bases,
                        const size_type* extents) :
    base_(base), storage_(so), origin_offset_(0), directional_offset_(0)
 {
   // If index_bases or extents is null, then initialize the corresponding
   // private data to zeroed lists.
   if(index_bases) {
     boost::detail::multi_array::
       copy_n(index_bases,NumDims,index_base_list_.begin());
   } else {
     std::fill_n(index_base_list_.begin(),NumDims,0);
   }
   if(extents) {
     init_multi_array_ref(extents);
   } else {
     boost::array<index,NumDims> extent_list;
     extent_list.assign(0);
     init_multi_array_ref(extent_list.begin());
   }
 }


  TPtr base_;
  storage_order_type storage_;
  size_list extent_list_;
  index_list stride_list_;
  index_list index_base_list_;
  index origin_offset_;
  index directional_offset_;
  size_type num_elements_;

private:
  // const_multi_array_ref cannot be assigned to (no deep copies!)
  const_multi_array_ref& operator=(const const_multi_array_ref& other);

  void init_from_extent_gen(const
                        detail::multi_array::
                        extent_gen<NumDims>& ranges) { 
    
    typedef boost::array<index,NumDims> extent_list;

    // get the index_base values
    std::transform(ranges.ranges_.begin(),ranges.ranges_.end(),
              index_base_list_.begin(),
              boost::mem_fun_ref(&extent_range::start));

    // calculate the extents
    extent_list extents;
    std::transform(ranges.ranges_.begin(),ranges.ranges_.end(),
              extents.begin(),
              boost::mem_fun_ref(&extent_range::size));

    init_multi_array_ref(extents.begin());
  }


#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
protected:
#else
public:
#endif
  // RG - move me!
  template <class InputIterator>
  void init_multi_array_ref(InputIterator extents_iter) {
    boost::function_requires<InputIteratorConcept<InputIterator> >();

    boost::detail::multi_array::
      copy_n(extents_iter,num_dimensions(),extent_list_.begin());

    // Calculate the array size
    num_elements_ = std::accumulate(extent_list_.begin(),extent_list_.end(),
                            size_type(1),std::multiplies<size_type>());

    this->compute_strides(stride_list_,extent_list_,storage_);

    origin_offset_ =
      this->calculate_origin_offset(stride_list_,extent_list_,
                              storage_,index_base_list_);
    directional_offset_ =
      this->calculate_descending_dimension_offset(stride_list_,extent_list_,
                                            storage_);
  }
};

template <typename T, std::size_t NumDims>
class multi_array_ref :
  public const_multi_array_ref<T,NumDims,T*>
{
  typedef const_multi_array_ref<T,NumDims,T*> super_type;
public: 
  typedef typename super_type::value_type value_type;
  typedef typename super_type::reference reference;
  typedef typename super_type::iterator iterator;
  typedef typename super_type::reverse_iterator reverse_iterator;
  typedef typename super_type::const_reference const_reference;
  typedef typename super_type::const_iterator const_iterator;
  typedef typename super_type::const_reverse_iterator const_reverse_iterator;
  typedef typename super_type::element element;
  typedef typename super_type::size_type size_type;
  typedef typename super_type::difference_type difference_type;
  typedef typename super_type::index index;
  typedef typename super_type::extent_range extent_range;

  typedef typename super_type::storage_order_type storage_order_type;
  typedef typename super_type::index_list index_list;
  typedef typename super_type::size_list size_list;

  template <std::size_t NDims>
  struct const_array_view {
    typedef boost::detail::multi_array::const_multi_array_view<T,NDims> type;
  };

  template <std::size_t NDims>
  struct array_view {
    typedef boost::detail::multi_array::multi_array_view<T,NDims> type;
  };

  template <class ExtentList>
  explicit multi_array_ref(T* base, const ExtentList& extents) :
    super_type(base,extents) {
    boost::function_requires<
      CollectionConcept<ExtentList> >();
  }

  template <class ExtentList>
  explicit multi_array_ref(T* base, const ExtentList& extents,
                           const general_storage_order<NumDims>& so) :
    super_type(base,extents,so) {
    boost::function_requires<
      CollectionConcept<ExtentList> >();
  }


  explicit multi_array_ref(T* base,
                           const detail::multi_array::
                           extent_gen<NumDims>& ranges) :
    super_type(base,ranges) { }


  explicit multi_array_ref(T* base,
                           const detail::multi_array::
                           extent_gen<NumDims>&
                             ranges,
                           const general_storage_order<NumDims>& so) :
    super_type(base,ranges,so) { }


  // Assignment from other ConstMultiArray types.
  template <typename ConstMultiArray>
  multi_array_ref& operator=(const ConstMultiArray& other) {
    function_requires< 
      multi_array_concepts::
      ConstMultiArrayConcept<ConstMultiArray,NumDims> >();

    // make sure the dimensions agree
    BOOST_ASSERT(other.num_dimensions() == this->num_dimensions());
    BOOST_ASSERT(std::equal(other.shape(),other.shape()+this->num_dimensions(),
                            this->shape()));
    // iterator-based copy
    std::copy(other.begin(),other.end(),this->begin());
    return *this;
  }

  multi_array_ref& operator=(const multi_array_ref& other) {
    if (&other != this) {
      // make sure the dimensions agree
      
      BOOST_ASSERT(other.num_dimensions() == this->num_dimensions());
      BOOST_ASSERT(std::equal(other.shape(),
                              other.shape()+this->num_dimensions(),
                              this->shape()));
      // iterator-based copy
      std::copy(other.begin(),other.end(),this->begin());
    }
    return *this;
  }

  element* origin() { return super_type::base_+super_type::origin_offset_; }

  element* data() { return super_type::base_; }

  template <class IndexList>
  element& operator()(const IndexList& indices) {
    boost::function_requires<
      CollectionConcept<IndexList> >();
    return super_type::access_element(boost::type<element&>(),
                                      indices,origin(),
                                      this->shape(),this->strides(),
                                      this->index_bases());
  }


  reference operator[](index idx) {
    return super_type::access(boost::type<reference>(),
                              idx,origin(),
                              this->shape(),this->strides(),
                              this->index_bases());
  }


  // See note attached to generate_array_view in base.hpp
  template <int NDims>
  typename array_view<NDims>::type 
  operator[](const detail::multi_array::
             index_gen<NumDims,NDims>& indices) {
    typedef typename array_view<NDims>::type return_type;
    return
      super_type::generate_array_view(boost::type<return_type>(),
                                      indices,
                                      this->shape(),
                                      this->strides(),
                                      this->index_bases(),
                                      origin());
  }
  
  
  iterator begin() {
    return iterator(*this->index_bases(),origin(),this->shape(),
                    this->strides(),this->index_bases());
  }

  iterator end() {
    return iterator(*this->index_bases()+(index)*this->shape(),origin(),
                    this->shape(),this->strides(),
                    this->index_bases());
  }

  // rbegin() and rend() written naively to thwart MSVC ICE.
  reverse_iterator rbegin() {
    reverse_iterator ri(end());
    return ri;
  }

  reverse_iterator rend() {
    reverse_iterator ri(begin());
    return ri;
  }

  // Using declarations don't seem to work for g++
  // These are the proxies to work around this.

  const element* origin() const { return super_type::origin(); }
  const element* data() const { return super_type::data(); }

  template <class IndexList>
  const element& operator()(const IndexList& indices) const {
    boost::function_requires<
      CollectionConcept<IndexList> >();
    return super_type::operator()(indices);
  }

  const_reference operator[](index idx) const {
    return super_type::access(boost::type<const_reference>(),
                              idx,origin(),
                              this->shape(),this->strides(),
                              this->index_bases());
  }

  // See note attached to generate_array_view in base.hpp
  template <int NDims>
  typename const_array_view<NDims>::type 
  operator[](const detail::multi_array::
             index_gen<NumDims,NDims>& indices)
    const {
    return super_type::operator[](indices);
  }
  
  const_iterator begin() const {
    return super_type::begin();
  }

  const_iterator end() const {
    return super_type::end();
  }

  const_reverse_iterator rbegin() const {
    return super_type::rbegin();
  }

  const_reverse_iterator rend() const {
    return super_type::rend();
  }

protected:
  // This is only supplied to support multi_array's default constructor
  explicit multi_array_ref(T* base,
                           const storage_order_type& so,
                           const index* index_bases,
                           const size_type* extents) :
    super_type(base,so,index_bases,extents) { }

};

} // namespace boost

#endif
