/* Copyright 2003-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_BUCKET_ARRAY_HPP
#define BOOST_MULTI_INDEX_DETAIL_BUCKET_ARRAY_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/multi_index/detail/allocator_traits.hpp>
#include <boost/multi_index/detail/auto_space.hpp>
#include <boost/multi_index/detail/hash_index_node.hpp>
#include <boost/noncopyable.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <cstddef>
#include <limits.h>

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
#include <boost/archive/archive_exception.hpp>
#include <boost/serialization/access.hpp>
#include <boost/throw_exception.hpp> 
#endif

namespace boost{

namespace multi_index{

namespace detail{

/* bucket structure for use by hashed indices */

#define BOOST_MULTI_INDEX_BA_SIZES_32BIT                                  \
(53ul)(97ul)(193ul)(389ul)(769ul)                                         \
(1543ul)(3079ul)(6151ul)(12289ul)(24593ul)                                \
(49157ul)(98317ul)(196613ul)(393241ul)(786433ul)                          \
(1572869ul)(3145739ul)(6291469ul)(12582917ul)(25165843ul)                 \
(50331653ul)(100663319ul)(201326611ul)(402653189ul)(805306457ul)          \
(1610612741ul)(3221225473ul)

#if ((((ULONG_MAX>>16)>>16)>>16)>>15)==0 /* unsigned long less than 64 bits */
#define BOOST_MULTI_INDEX_BA_SIZES                                         \
BOOST_MULTI_INDEX_BA_SIZES_32BIT                                           \
(4294967291ul)
#else
  /* obtained with aid from
   *   http://javaboutique.internet.com/prime_numb/
   *   http://www.rsok.com/~jrm/next_ten_primes.html
   * and verified with
   *   http://www.alpertron.com.ar/ECM.HTM
   */

#define BOOST_MULTI_INDEX_BA_SIZES                                         \
BOOST_MULTI_INDEX_BA_SIZES_32BIT                                           \
(6442450939ul)(12884901893ul)(25769803751ul)(51539607551ul)                \
(103079215111ul)(206158430209ul)(412316860441ul)(824633720831ul)           \
(1649267441651ul)(3298534883309ul)(6597069766657ul)(13194139533299ul)      \
(26388279066623ul)(52776558133303ul)(105553116266489ul)(211106232532969ul) \
(422212465066001ul)(844424930131963ul)(1688849860263953ul)                 \
(3377699720527861ul)(6755399441055731ul)(13510798882111483ul)              \
(27021597764222939ul)(54043195528445957ul)(108086391056891903ul)           \
(216172782113783843ul)(432345564227567621ul)(864691128455135207ul)         \
(1729382256910270481ul)(3458764513820540933ul)(6917529027641081903ul)      \
(13835058055282163729ul)(18446744073709551557ul)
#endif

template<bool _=true> /* templatized to have in-header static var defs */
class bucket_array_base:private noncopyable
{
protected:
  static const std::size_t sizes[
    BOOST_PP_SEQ_SIZE(BOOST_MULTI_INDEX_BA_SIZES)];

  static std::size_t size_index(std::size_t n)
  {
    const std::size_t *bound=std::lower_bound(sizes,sizes+sizes_length,n);
    if(bound==sizes+sizes_length)--bound;
    return bound-sizes;
  }

#define BOOST_MULTI_INDEX_BA_POSITION_CASE(z,n,_)                    \
  case n:return hash%BOOST_PP_SEQ_ELEM(n,BOOST_MULTI_INDEX_BA_SIZES);

  static std::size_t position(std::size_t hash,std::size_t size_index_)
  {
    /* Accelerate hash%sizes[size_index_] by replacing with a switch on
     * hash%Ci expressions, each Ci a compile-time constant, which the
     * compiler can implement without using integer division.
     */

    switch(size_index_){
      default: /* never used */
      BOOST_PP_REPEAT(
        BOOST_PP_SEQ_SIZE(BOOST_MULTI_INDEX_BA_SIZES),
        BOOST_MULTI_INDEX_BA_POSITION_CASE,~)
    }
  }

private:
  static const std::size_t sizes_length;
};

template<bool _>
const std::size_t bucket_array_base<_>::sizes[]={
  BOOST_PP_SEQ_ENUM(BOOST_MULTI_INDEX_BA_SIZES)
};

template<bool _>
const std::size_t bucket_array_base<_>::sizes_length=
  sizeof(bucket_array_base<_>::sizes)/
  sizeof(bucket_array_base<_>::sizes[0]);

#undef BOOST_MULTI_INDEX_BA_POSITION_CASE
#undef BOOST_MULTI_INDEX_BA_SIZES
#undef BOOST_MULTI_INDEX_BA_SIZES_32BIT

template<typename Allocator>
class bucket_array:bucket_array_base<>
{
  typedef bucket_array_base<>                        super;
  typedef hashed_index_base_node_impl<
    typename rebind_alloc_for<
      Allocator,
      char
    >::type
  >                                                  base_node_impl_type;

public:
  typedef typename base_node_impl_type::base_pointer base_pointer;
  typedef typename base_node_impl_type::pointer      pointer;

  bucket_array(const Allocator& al,pointer end_,std::size_t size_):
    size_index_(super::size_index(size_)),
    spc(al,static_cast<auto_space_size_type>(super::sizes[size_index_]+1))
  {
    clear(end_);
  }

  std::size_t size()const
  {
    return super::sizes[size_index_];
  }

  std::size_t position(std::size_t hash)const
  {
    return super::position(hash,size_index_);
  }

  base_pointer begin()const{return buckets();}
  base_pointer end()const{return buckets()+size();}
  base_pointer at(std::size_t n)const{return buckets()+n;}

  void clear(pointer end_)
  {
    for(base_pointer x=begin(),y=end();x!=y;++x)x->prior()=pointer(0);
    end()->prior()=end_->prior()=end_;
    end_->next()=end();
 }

  void swap(bucket_array& x)
  {
    std::swap(size_index_,x.size_index_);
    spc.swap(x.spc);
  }

  template<typename BoolConstant>
  void swap(bucket_array& x,BoolConstant swap_allocators)
  {
    std::swap(size_index_,x.size_index_);
    spc.swap(x.spc,swap_allocators);
  }

private:
  typedef auto_space<base_node_impl_type,Allocator> auto_space_type;
  typedef typename auto_space_type::size_type       auto_space_size_type;

  std::size_t      size_index_;
  auto_space_type  spc;

  base_pointer buckets()const
  {
    return spc.data();
  }

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
  friend class boost::serialization::access;
  
  /* bucket_arrays do not emit any kind of serialization info. They are
   * fed to Boost.Serialization as hashed index iterators need to track
   * them during serialization.
   */

  template<class Archive>
  void serialize(Archive&,const unsigned int)
  {
  }
#endif
};

template<typename Allocator>
void swap(bucket_array<Allocator>& x,bucket_array<Allocator>& y)
{
  x.swap(y);
}

} /* namespace multi_index::detail */

} /* namespace multi_index */

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
/* bucket_arrays never get constructed directly by Boost.Serialization,
 * as archives are always fed pointers to previously existent
 * arrays. So, if this is called it means we are dealing with a
 * somehow invalid archive.
 */

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace serialization{
#else
namespace multi_index{
namespace detail{
#endif

template<class Archive,typename Allocator>
inline void load_construct_data(
  Archive&,boost::multi_index::detail::bucket_array<Allocator>*,
  const unsigned int)
{
  throw_exception(
    archive::archive_exception(archive::archive_exception::other_exception));
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
} /* namespace serialization */
#else
} /* namespace multi_index::detail */
} /* namespace multi_index */
#endif

#endif

} /* namespace boost */

#endif
