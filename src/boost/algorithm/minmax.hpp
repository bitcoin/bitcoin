//  (C) Copyright Herve Bronnimann 2004.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
 Revision history:
   1 July 2004
      Split the code into two headers to lessen dependence on
      Boost.tuple. (Herve)
   26 June 2004
      Added the code for the boost minmax library. (Herve)
*/

#ifndef BOOST_ALGORITHM_MINMAX_HPP
#define BOOST_ALGORITHM_MINMAX_HPP

/* PROPOSED STANDARD EXTENSIONS:
 *
 * minmax(a, b)
 * Effect: (b<a) ? std::make_pair(b,a) : std::make_pair(a,b);
 *
 * minmax(a, b, comp)
 * Effect: comp(b,a) ? std::make_pair(b,a) : std::make_pair(a,b);
 *
 */

#include <boost/config.hpp>
#include <boost/tuple/tuple.hpp> // for using pairs with boost::cref
#include <boost/ref.hpp>

namespace boost {

  template <typename T>
  tuple< T const&, T const& >
  minmax(T const& a, T const& b) {
    return (b<a) ? make_tuple(cref(b),cref(a)) : make_tuple(cref(a),cref(b));
  }

  template <typename T, class BinaryPredicate>
  tuple< T const&, T const& >
  minmax(T const& a, T const& b, BinaryPredicate comp) {
    return comp(b,a) ? make_tuple(cref(b),cref(a)) : make_tuple(cref(a),cref(b));
  }

} // namespace boost

#endif // BOOST_ALGORITHM_MINMAX_HPP
