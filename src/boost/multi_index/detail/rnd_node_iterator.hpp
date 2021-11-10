/* Copyright 2003-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_RND_NODE_ITERATOR_HPP
#define BOOST_MULTI_INDEX_DETAIL_RND_NODE_ITERATOR_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/operators.hpp>

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#endif

namespace boost{

namespace multi_index{

namespace detail{

/* Iterator class for node-based indices with random access iterators. */

template<typename Node>
class rnd_node_iterator:
  public random_access_iterator_helper<
    rnd_node_iterator<Node>,
    typename Node::value_type,
    typename Node::difference_type,
    const typename Node::value_type*,
    const typename Node::value_type&>
{
public:
  /* coverity[uninit_ctor]: suppress warning */
  rnd_node_iterator(){}
  explicit rnd_node_iterator(Node* node_):node(node_){}

  const typename Node::value_type& operator*()const
  {
    return node->value();
  }

  rnd_node_iterator& operator++()
  {
    Node::increment(node);
    return *this;
  }

  rnd_node_iterator& operator--()
  {
    Node::decrement(node);
    return *this;
  }

  rnd_node_iterator& operator+=(typename Node::difference_type n)
  {
    Node::advance(node,n);
    return *this;
  }

  rnd_node_iterator& operator-=(typename Node::difference_type n)
  {
    Node::advance(node,-n);
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
  void load(Archive& ar,const unsigned int)
  {
    node_base_type* bnode;
    ar>>serialization::make_nvp("pointer",bnode);
    node=static_cast<Node*>(bnode);
  }
#endif

  /* get_node is not to be used by the user */

  typedef Node node_type;

  Node* get_node()const{return node;}

private:
  Node* node;
};

template<typename Node>
bool operator==(
  const rnd_node_iterator<Node>& x,
  const rnd_node_iterator<Node>& y)
{
  return x.get_node()==y.get_node();
}

template<typename Node>
bool operator<(
  const rnd_node_iterator<Node>& x,
  const rnd_node_iterator<Node>& y)
{
  return Node::distance(x.get_node(),y.get_node())>0;
}

template<typename Node>
typename Node::difference_type operator-(
  const rnd_node_iterator<Node>& x,
  const rnd_node_iterator<Node>& y)
{
  return Node::distance(y.get_node(),x.get_node());
}

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
