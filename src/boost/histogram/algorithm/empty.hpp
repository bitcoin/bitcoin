// Copyright 2019 Henry Schreiner
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_ALGORITHM_EMPTY_HPP
#define BOOST_HISTOGRAM_ALGORITHM_EMPTY_HPP

#include <boost/histogram/fwd.hpp>
#include <boost/histogram/indexed.hpp>

namespace boost {
namespace histogram {
namespace algorithm {
/** Check to see if all histogram cells are empty. Use coverage to include or
  exclude the underflow/overflow bins.

  This algorithm has O(N) complexity, where N is the number of cells.

  Returns true if all cells are empty, and false otherwise.
 */
template <class A, class S>
auto empty(const histogram<A, S>& h, coverage cov) {
  using value_type = typename histogram<A, S>::value_type;
  const value_type default_value = value_type();
  for (auto&& ind : indexed(h, cov)) {
    if (*ind != default_value) { return false; }
  }
  return true;
}
} // namespace algorithm
} // namespace histogram
} // namespace boost

#endif
