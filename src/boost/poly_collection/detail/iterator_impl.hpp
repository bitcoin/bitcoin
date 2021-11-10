/* Copyright 2016-2021 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_ITERATOR_IMPL_HPP
#define BOOST_POLY_COLLECTION_DETAIL_ITERATOR_IMPL_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/detail/workaround.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/poly_collection/detail/is_constructible.hpp>
#include <boost/poly_collection/detail/iterator_traits.hpp>
#include <type_traits>
#include <typeinfo>

namespace boost{

namespace poly_collection{

namespace detail{

/* Implementations of poly_collection::[const_][local_[base_]]iterator moved
 * out of class to allow for use in deduced contexts.
 */

template<typename PolyCollection,bool Const>
using iterator_impl_value_type=typename std::conditional<
  Const,
  const typename PolyCollection::value_type,
  typename PolyCollection::value_type
>::type;

template<typename PolyCollection,bool Const>
class iterator_impl:
  public boost::iterator_facade<
    iterator_impl<PolyCollection,Const>,
    iterator_impl_value_type<PolyCollection,Const>,
    boost::forward_traversal_tag
  >
{
  using segment_type=typename PolyCollection::segment_type;
  using const_segment_base_iterator=
    typename PolyCollection::const_segment_base_iterator;
  using const_segment_base_sentinel=
    typename PolyCollection::const_segment_base_sentinel;
  using const_segment_map_iterator=
    typename PolyCollection::const_segment_map_iterator;

public:
  using value_type=iterator_impl_value_type<PolyCollection,Const>;

private:
  iterator_impl(
    const_segment_map_iterator mapit,
    const_segment_map_iterator mapend)noexcept:
    mapit{mapit},mapend{mapend}
  {
    next_segment_position();
  }

  iterator_impl(
    const_segment_map_iterator mapit_,const_segment_map_iterator mapend_,
    const_segment_base_iterator segpos_)noexcept:
    mapit{mapit_},mapend{mapend_},segpos{segpos_}
  {
    if(mapit!=mapend&&segpos==sentinel()){
      ++mapit;
      next_segment_position();
    }
  }

public:
  iterator_impl()=default;
  iterator_impl(const iterator_impl&)=default;
  iterator_impl& operator=(const iterator_impl&)=default;

  template<bool Const2,typename std::enable_if<!Const2>::type* =nullptr>
  iterator_impl(const iterator_impl<PolyCollection,Const2>& x):
    mapit{x.mapit},mapend{x.mapend},segpos{x.segpos}{}
      
private:
  template<typename,bool>
  friend class iterator_impl;
  friend PolyCollection;
  friend class boost::iterator_core_access;
  template<typename>
  friend struct iterator_traits;

  value_type& dereference()const noexcept
    {return const_cast<value_type&>(*segpos);}
  bool equal(const iterator_impl& x)const noexcept{return segpos==x.segpos;}

  void increment()noexcept
  {
    if(++segpos==sentinel()){
      ++mapit;
      next_segment_position();
    }
  }

  void next_segment_position()noexcept
  {
    for(;mapit!=mapend;++mapit){
      segpos=segment().begin();
      if(segpos!=sentinel())return;
    }
    segpos=nullptr;
  }

  segment_type&       segment()noexcept
    {return const_cast<segment_type&>(mapit->second);}
  const segment_type& segment()const noexcept{return mapit->second;}

  const_segment_base_sentinel sentinel()const noexcept
    {return segment().sentinel();}

  const_segment_map_iterator  mapit,mapend;
  const_segment_base_iterator segpos;
};

template<typename PolyCollection,bool Const>
struct poly_collection_of<iterator_impl<PolyCollection,Const>>
{
  using type=PolyCollection;
};

template<typename PolyCollection,typename BaseIterator>
class local_iterator_impl:
  public boost::iterator_adaptor<
    local_iterator_impl<PolyCollection,BaseIterator>,
    BaseIterator
  >
{
  using segment_type=typename PolyCollection::segment_type;
  using segment_base_iterator=typename PolyCollection::segment_base_iterator;
  using const_segment_map_iterator=
    typename PolyCollection::const_segment_map_iterator;

#if BOOST_WORKAROUND(BOOST_GCC_VERSION,>=90300)&&\
    BOOST_WORKAROUND(BOOST_GCC_VERSION,<100300)
/* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95888 */

public:
#else
private:
#endif

  template<typename Iterator>
  local_iterator_impl(
    const_segment_map_iterator mapit,
    Iterator it):
    local_iterator_impl::iterator_adaptor_{BaseIterator(it)},
    mapit{mapit}
  {}

public:
  using base_iterator=BaseIterator;

  local_iterator_impl()=default;
  local_iterator_impl(const local_iterator_impl&)=default;
  local_iterator_impl& operator=(const local_iterator_impl&)=default;

  template<
    typename BaseIterator2,
    typename std::enable_if<
      std::is_convertible<BaseIterator2,BaseIterator>::value
    >::type* =nullptr
  >
  local_iterator_impl(
    const local_iterator_impl<PolyCollection,BaseIterator2>& x):
    local_iterator_impl::iterator_adaptor_{x.base()},
    mapit{x.mapit}{}

  template<
    typename BaseIterator2,
    typename std::enable_if<
      !std::is_convertible<BaseIterator2,BaseIterator>::value&&
      is_constructible<BaseIterator,BaseIterator2>::value
    >::type* =nullptr
  >
  explicit local_iterator_impl(
    const local_iterator_impl<PolyCollection,BaseIterator2>& x):
    local_iterator_impl::iterator_adaptor_{BaseIterator(x.base())},
    mapit{x.mapit}{}

  template<
    typename BaseIterator2,
    typename std::enable_if<
      !is_constructible<BaseIterator,BaseIterator2>::value&&
      is_constructible<BaseIterator,segment_base_iterator>::value&&
      is_constructible<BaseIterator2,segment_base_iterator>::value
    >::type* =nullptr
  >
  explicit local_iterator_impl(
    const local_iterator_impl<PolyCollection,BaseIterator2>& x):
    local_iterator_impl::iterator_adaptor_{
      base_iterator_from(x.segment(),x.base())},
    mapit{x.mapit}{}

  /* define [] to avoid Boost.Iterator operator_brackets_proxy mess */

  template<typename DifferenceType>
  typename std::iterator_traits<BaseIterator>::reference
  operator[](DifferenceType n)const{return *(*this+n);}

private:
  template<typename,typename>
  friend class local_iterator_impl;
  friend PolyCollection;
  template<typename>
  friend struct iterator_traits;

  template<typename BaseIterator2>
  static BaseIterator base_iterator_from(
    const segment_type& s,BaseIterator2 it)
  {
    segment_base_iterator bit=s.begin();
    return BaseIterator{bit+(it-static_cast<BaseIterator2>(bit))};
  } 

  base_iterator              base()const noexcept
    {return local_iterator_impl::iterator_adaptor_::base();}
  const std::type_info&      type_info()const{return *mapit->first;}
  segment_type&              segment()noexcept
    {return const_cast<segment_type&>(mapit->second);}
  const segment_type&        segment()const noexcept{return mapit->second;}

  const_segment_map_iterator mapit;
};

template<typename PolyCollection,typename BaseIterator>
struct poly_collection_of<local_iterator_impl<PolyCollection,BaseIterator>>
{
  using type=PolyCollection;
};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
