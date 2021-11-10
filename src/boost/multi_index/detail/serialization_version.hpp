/* Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_SERIALIZATION_VERSION_HPP
#define BOOST_MULTI_INDEX_DETAIL_SERIALIZATION_VERSION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>

namespace boost{

namespace multi_index{

namespace detail{

/* Helper class for storing and retrieving a given type serialization class
 * version while avoiding saving the number multiple times in the same
 * archive.
 * Behavior undefined if template partial specialization is not supported.
 */

template<typename T>
struct serialization_version
{
  serialization_version():
    value(boost::serialization::version<serialization_version>::value){}

  serialization_version& operator=(unsigned int x){value=x;return *this;};

  operator unsigned int()const{return value;}

private:
  friend class boost::serialization::access;

  BOOST_SERIALIZATION_SPLIT_MEMBER()

  template<class Archive>
  void save(Archive&,const unsigned int)const{}

  template<class Archive>
  void load(Archive&,const unsigned int version)
  {
    this->value=version;
  }

  unsigned int value;
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

namespace serialization {
template<typename T>
struct version<boost::multi_index::detail::serialization_version<T> >
{
  BOOST_STATIC_CONSTANT(int,value=version<T>::value);
};
} /* namespace serialization */

} /* namespace boost */

#endif
