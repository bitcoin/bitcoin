/* boost random/taus88.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/random for documentation.
 *
 * $Id$
 *
 */

#ifndef BOOST_RANDOM_TAUS88_HPP
#define BOOST_RANDOM_TAUS88_HPP

#include <boost/random/linear_feedback_shift.hpp>
#include <boost/random/xor_combine.hpp>

namespace boost {
namespace random {

/** 
 * The specialization taus88 was suggested in
 *
 *  @blockquote
 *  "Maximally Equidistributed Combined Tausworthe Generators",
 *  Pierre L'Ecuyer, Mathematics of Computation, Volume 65,
 *  Number 213, January 1996, Pages 203-213
 *  @endblockquote
 */
typedef xor_combine_engine<
    xor_combine_engine<
        linear_feedback_shift_engine<uint32_t, 32, 31, 13, 12>, 0,
        linear_feedback_shift_engine<uint32_t, 32, 29, 2, 4>, 0>, 0,
    linear_feedback_shift_engine<uint32_t, 32, 28, 3, 17>, 0> taus88;

} // namespace random

using random::taus88;

} // namespace boost

#endif // BOOST_RANDOM_TAUS88_HPP
