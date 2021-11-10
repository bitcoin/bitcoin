/* boost random/uniform_real.hpp header file
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
 *  2001-04-08  added min<max assertion (N. Becker)
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_UNIFORM_REAL_HPP
#define BOOST_RANDOM_UNIFORM_REAL_HPP

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/random/uniform_real_distribution.hpp>

namespace boost {

/**
 * The distribution function uniform_real models a random distribution.
 * On each invocation, it returns a random floating-point value uniformly
 * distributed in the range [min..max).
 *
 * This class is deprecated.  Please use @c uniform_real_distribution in
 * new code.
 */
template<class RealType = double>
class uniform_real : public random::uniform_real_distribution<RealType>
{
    typedef random::uniform_real_distribution<RealType> base_type;
public:

    class param_type : public base_type::param_type
    {
    public:
        typedef uniform_real distribution_type;
        /**
         * Constructs the parameters of a uniform_real distribution.
         *
         * Requires: min <= max
         */
        explicit param_type(RealType min_arg = RealType(0.0),
                            RealType max_arg = RealType(1.0))
          : base_type::param_type(min_arg, max_arg)
        {}
    };

    /**
     * Constructs a uniform_real object. @c min and @c max are the
     * parameters of the distribution.
     *
     * Requires: min <= max
     */
    explicit uniform_real(RealType min_arg = RealType(0.0),
                          RealType max_arg = RealType(1.0))
      : base_type(min_arg, max_arg)
    {
        BOOST_ASSERT(min_arg < max_arg);
    }

    /** Constructs a uniform_real distribution from its parameters. */
    explicit uniform_real(const param_type& parm)
      : base_type(parm)
    {}

    /** Returns the parameters of the distribution */
    param_type param() const { return param_type(this->a(), this->b()); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm) { this->base_type::param(parm); }
};

} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_REAL_HPP
