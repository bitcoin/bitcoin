/* boost random/detail/uniform_int_float.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 */

#ifndef BOOST_RANDOM_DETAIL_UNIFORM_INT_FLOAT_HPP
#define BOOST_RANDOM_DETAIL_UNIFORM_INT_FLOAT_HPP

#include <boost/limits.hpp>
#include <boost/config.hpp>
#include <boost/integer.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/generator_bits.hpp>

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {
namespace random {
namespace detail {

template<class URNG>
class uniform_int_float
{
public:
    typedef URNG base_type;
    typedef typename base_type::result_type base_result;

    typedef typename boost::uint_t<
        (std::numeric_limits<boost::uintmax_t>::digits <
            std::numeric_limits<base_result>::digits)?
        std::numeric_limits<boost::uintmax_t>::digits :
        std::numeric_limits<base_result>::digits
    >::fast result_type;

    uniform_int_float(base_type& rng)
      : _rng(rng) {}

    static result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return 0; }
    static result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    {
        std::size_t digits = std::numeric_limits<result_type>::digits;
        if(detail::generator_bits<URNG>::value() < digits) {
            digits = detail::generator_bits<URNG>::value();
        }
        return (result_type(2) << (digits - 1)) - 1;
    }
    base_type& base() { return _rng; }
    const base_type& base() const { return _rng; }

    result_type operator()()
    {
        base_result range = static_cast<base_result>((max)())+1;
        return static_cast<result_type>(_rng() * range);
    }

private:
    base_type& _rng;
};

} // namespace detail
} // namespace random
} // namespace boost

#include <boost/random/detail/enable_warnings.hpp>

#endif // BOOST_RANDOM_DETAIL_UNIFORM_INT_FLOAT_HPP
