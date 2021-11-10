/* boost random/uniform_int.hpp header file
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

#ifndef BOOST_RANDOM_UNIFORM_INT_HPP
#define BOOST_RANDOM_UNIFORM_INT_HPP

#include <boost/assert.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace boost {

/**
 * The distribution function uniform_int models a \random_distribution.
 * On each invocation, it returns a random integer value uniformly
 * distributed in the set of integer numbers {min, min+1, min+2, ..., max}.
 *
 * The template parameter IntType shall denote an integer-like value type.
 *
 * This class is deprecated.  Please use @c uniform_int_distribution in
 * new code.
 */
template<class IntType = int>
class uniform_int : public random::uniform_int_distribution<IntType>
{
    typedef random::uniform_int_distribution<IntType> base_type;
public:

    class param_type : public base_type::param_type
    {
    public:
        typedef uniform_int distribution_type;
        /**
         * Constructs the parameters of a uniform_int distribution.
         *
         * Requires: min <= max
         */
        explicit param_type(IntType min_arg = 0, IntType max_arg = 9)
          : base_type::param_type(min_arg, max_arg)
        {}
    };

    /**
     * Constructs a uniform_int object. @c min and @c max are
     * the parameters of the distribution.
     *
     * Requires: min <= max
     */
    explicit uniform_int(IntType min_arg = 0, IntType max_arg = 9)
      : base_type(min_arg, max_arg)
    {}

    /** Constructs a uniform_int distribution from its parameters. */
    explicit uniform_int(const param_type& parm)
      : base_type(parm)
    {}

    /** Returns the parameters of the distribution */
    param_type param() const { return param_type(this->a(), this->b()); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm) { this->base_type::param(parm); }

    // Codergear seems to have trouble with a using declaration here

    template<class Engine>
    IntType operator()(Engine& eng) const
    {
        return static_cast<const base_type&>(*this)(eng);
    }

    template<class Engine>
    IntType operator()(Engine& eng, const param_type& parm) const
    {
        return static_cast<const base_type&>(*this)(eng, parm);
    }

    template<class Engine>
    IntType operator()(Engine& eng, IntType n) const
    {
        BOOST_ASSERT(n > 0);
        return static_cast<const base_type&>(*this)(eng, param_type(0, n - 1));
    }
};

} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_INT_HPP
