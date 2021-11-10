/* Copyright 2006-2014 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_HASHED_FACTORY_HPP
#define BOOST_FLYWEIGHT_HASHED_FACTORY_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/flyweight/factory_tag.hpp>
#include <boost/flyweight/hashed_factory_fwd.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>
#include <boost/mpl/if.hpp>

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#include <utility>
#endif

/* Flyweight factory based on a hashed container implemented
 * with Boost.MultiIndex.
 */

namespace boost{

namespace flyweights{

template<
  typename Entry,typename Key,
  typename Hash,typename Pred,typename Allocator
>
class hashed_factory_class:public factory_marker
{
  struct index_list:
    boost::mpl::vector1<
      multi_index::hashed_unique<
        multi_index::identity<Entry>,
        typename boost::mpl::if_<
          mpl::is_na<Hash>,
          hash<Key>,
          Hash
        >::type,
        typename boost::mpl::if_<
          mpl::is_na<Pred>,
          std::equal_to<Key>,
          Pred
        >::type
      >
    >
  {};

  typedef multi_index::multi_index_container<
    Entry,
    index_list,
    typename boost::mpl::if_<
      mpl::is_na<Allocator>,
      std::allocator<Entry>,
      Allocator
    >::type
  > container_type;

public:
  typedef const Entry* handle_type;
  
  handle_type insert(const Entry& x)
  {
    return &*cont.insert(x).first;
  }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
  handle_type insert(Entry&& x)
  {
    return &*cont.insert(std::move(x)).first;
  }
#endif

  void erase(handle_type h)
  {
    cont.erase(cont.iterator_to(*h));
  }

  static const Entry& entry(handle_type h){return *h;}

private:  
  container_type cont;

public:
  typedef hashed_factory_class type;
  BOOST_MPL_AUX_LAMBDA_SUPPORT(
    5,hashed_factory_class,(Entry,Key,Hash,Pred,Allocator))
};

/* hashed_factory_class specifier */

template<
  typename Hash,typename Pred,typename Allocator
  BOOST_FLYWEIGHT_NOT_A_PLACEHOLDER_EXPRESSION_DEF
>
struct hashed_factory:factory_marker
{
  template<typename Entry,typename Key>
  struct apply:
    mpl::apply2<
      hashed_factory_class<
        boost::mpl::_1,boost::mpl::_2,Hash,Pred,Allocator
      >,
      Entry,Key
    >
  {};
};

} /* namespace flyweights */

} /* namespace boost */

#endif
