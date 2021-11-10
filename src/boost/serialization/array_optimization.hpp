#ifndef BOOST_SERIALIZATION_ARRAY_OPTIMIZATION_HPP
#define BOOST_SERIALIZATION_ARRAY_OPTIMIZATION_HPP

// (C) Copyright 2005 Matthias Troyer and Dave Abrahams
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp> // msvc 6.0 needs this for warning suppression

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::size_t;
} // namespace std
#endif

#include <boost/mpl/always.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/type_traits/remove_const.hpp>

namespace boost { namespace serialization {

template <class Archive>
struct use_array_optimization : boost::mpl::always<boost::mpl::false_> {};

} } // end namespace boost::serialization

#define BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(Archive)           \
namespace boost { namespace serialization {                           \
template <> struct use_array_optimization<Archive> {                  \
  template <class ValueType>                                          \
  struct apply : boost::mpl::apply1<Archive::use_array_optimization   \
      , typename boost::remove_const<ValueType>::type                 \
    >::type {};                                                       \
}; }}

#endif //BOOST_SERIALIZATION_ARRAY_OPTIMIZATION_HPP
