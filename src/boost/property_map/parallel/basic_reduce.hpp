// Copyright 2005 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

#ifndef BOOST_PARALLEL_BASIC_REDUCE_HPP
#define BOOST_PARALLEL_BASIC_REDUCE_HPP

namespace boost { namespace parallel {

/** Reduction operation used to reconcile differences between local
 * and remote values for a particular key in a property map.  The
 * type @c T is typically the @c value_type of the property
 * map. This basic reduction returns a default-constructed @c T as
 * the default value and always resolves to the remote value.
 */
template<typename T>
struct basic_reduce
{
  BOOST_STATIC_CONSTANT(bool, non_default_resolver = false);

  /// Returns a default-constructed T object
  template<typename Key>
  T operator()(const Key&) const { return T(); }
  
  /// Returns the remote value
  template<typename Key>
  const T& operator()(const Key&, const T&, const T& remote) const 
  { return remote; }
};

} } // end namespace boost::parallel

#endif // BOOST_PARALLEL_BASIC_REDUCE_HPP
