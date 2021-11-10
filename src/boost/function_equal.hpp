//  Copyright Douglas Gregor 2004.
//  Copyright 2005 Peter Dimov

//  Use, modification and distribution is subject to
//  the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org
#ifndef BOOST_FUNCTION_EQUAL_HPP
#define BOOST_FUNCTION_EQUAL_HPP

namespace boost {

template<typename F, typename G>
  bool function_equal_impl(const F& f, const G& g, long)
  { return f == g; }

// function_equal_impl needs to be unqualified to pick
// user overloads on two-phase compilers

template<typename F, typename G>
  bool function_equal(const F& f, const G& g)
  { return function_equal_impl(f, g, 0); }

} // end namespace boost

#endif // BOOST_FUNCTION_EQUAL_HPP
