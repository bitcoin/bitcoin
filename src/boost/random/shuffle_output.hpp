/* boost random/shuffle_output.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_SHUFFLE_OUTPUT_HPP
#define BOOST_RANDOM_SHUFFLE_OUTPUT_HPP

#include <boost/random/shuffle_order.hpp>

namespace boost {
namespace random {

/// \cond

template<typename URNG, int k, 
         typename URNG::result_type val = 0> 
class shuffle_output : public shuffle_order_engine<URNG, k>
{
    typedef shuffle_order_engine<URNG, k> base_t;
public:
    typedef typename base_t::result_type result_type;
    shuffle_output() {}
    template<class T>
    explicit shuffle_output(T& arg) : base_t(arg) {}
    template<class T>
    explicit shuffle_output(const T& arg) : base_t(arg) {}
    template<class It>
    shuffle_output(It& first, It last) : base_t(first, last) {}
    result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return (this->base().min)(); }
    result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return (this->base().max)(); }
};

/// \endcond

}
}

#endif // BOOST_RANDOM_SHUFFLE_OUTPUT_HPP
