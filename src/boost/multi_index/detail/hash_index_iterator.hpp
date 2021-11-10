/* Copyright 2003-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_HASH_INDEX_ITERATOR_HPP
#define BOOST_MULTI_INDEX_DETAIL_HASH_INDEX_ITERATOR_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/operators.hpp>

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#endif

namespace boost{

namespace multi_index{

namespace detail{

/* Iterator class for hashed indices.
 */

struct hashed_index_global_iterator_tag{};
struct hashed_index_local_iterator_tag{};

template<
  typename Node,typename BucketArray,
  typename IndexCategory,typename IteratorCategory
>
class hashed_index_iterator:
  public forward_iterator_helper<
    hashed_index_iterator<Node,BucketArray,IndexCategory,IteratorCategory>,
    typename Node::value_type,
    typename Node::difference_type,
    const typename Node::value_type*,
    const typename Node::value_type&>
{
public:
  /* coverity[uninit_ctor]: suppress warning */
  hashed_index_iterator(){}
  hashed_index_iterator(Node* node_):node(node_){}

  const typename Node::value_type& operator*()const
  {
    return node->value();
  }

  hashed_index_iterator& operator++()
  {
    this->increment(IteratorCategory());
    return *this;
  }

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
  /* Serialization. As for why the following is public,
   * see explanation in safe_mode_iterator notes in safe_mode.hpp.
   */
  
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  typedef typename Node::base_type node_base_type;

  template<class Archive>
  void save(Archive& ar,const unsigned int)const
  {
    node_base_type* bnode=node;
    ar<<serialization::make_nvp("pointer",bnode);
  }

  template<class Archive>
  void load(Archive& ar,const unsigned int version)
  {
    load(ar,version,IteratorCategory());
  }

  template<class Archive>
  void load(
    Archive& ar,const unsigned int version,hashed_index_global_iterator_tag)
  {
    node_base_type* bnode;
    ar>>serialization::make_nvp("pointer",bnode);
    node=static_cast<Node*>(bnode);
    if(version<1){
      BucketArray* throw_away; /* consume unused ptr */
      ar>>serialization::make_nvp("pointer",throw_away);
    }
  }

  template<class Archive>
  void load(
    Archive& ar,const unsigned int version,hashed_index_local_iterator_tag)
  {
    node_base_type* bnode;
    ar>>serialization::make_nvp("pointer",bnode);
    node=static_cast<Node*>(bnode);
    if(version<1){
      BucketArray* buckets;
      ar>>serialization::make_nvp("pointer",buckets);
      if(buckets&&node&&node->impl()==buckets->end()->prior()){
        /* end local_iterators used to point to end node, now they are null */
        node=0;
      }
    }
  }
#endif

  /* get_node is not to be used by the user */

  typedef Node node_type;

  Node* get_node()const{return node;}

private:

  void increment(hashed_index_global_iterator_tag)
  {
    Node::template increment<IndexCategory>(node);
  }

  void increment(hashed_index_local_iterator_tag)
  {
    Node::template increment_local<IndexCategory>(node);
  }

  Node* node;
};

template<
  typename Node,typename BucketArray,
  typename IndexCategory,typename IteratorCategory
>
bool operator==(
  const hashed_index_iterator<
    Node,BucketArray,IndexCategory,IteratorCategory>& x,
  const hashed_index_iterator<
    Node,BucketArray,IndexCategory,IteratorCategory>& y)
{
  return x.get_node()==y.get_node();
}

} /* namespace multi_index::detail */

} /* namespace multi_index */

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
/* class version = 1 : hashed_index_iterator does no longer serialize a bucket
 * array pointer.
 */

namespace serialization {
template<
  typename Node,typename BucketArray,
  typename IndexCategory,typename IteratorCategory
>
struct version<
  boost::multi_index::detail::hashed_index_iterator<
    Node,BucketArray,IndexCategory,IteratorCategory
  >
>
{
  BOOST_STATIC_CONSTANT(int,value=1);
};
} /* namespace serialization */
#endif

} /* namespace boost */

#endif
