/* boost random/extreme_value_distribution.hpp header file
 *
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 */

#ifndef BOOST_RANDOM_EXTREME_VALUE_DISTRIBUTION_HPP
#define BOOST_RANDOM_EXTREME_VALUE_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <iosfwd>
#include <istream>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/uniform_01.hpp>

namespace boost {
namespace random {

/**
 * The extreme value distribution is a real valued distribution with two
 * parameters a and b.
 *
 * It has \f$\displaystyle p(x) = \frac{1}{b}e^{\frac{a-x}{b} - e^\frac{a-x}{b}}\f$.
 */
template<class RealType = double>
class extreme_value_distribution {
public:
    typedef RealType result_type;
    typedef RealType input_type;

    class param_type {
    public:
        typedef extreme_value_distribution distribution_type;

        /**
         * Constructs a @c param_type from the "a" and "b" parameters
         * of the distribution.
         *
         * Requires: b > 0
         */
        explicit param_type(RealType a_arg = 1.0, RealType b_arg = 1.0)
          : _a(a_arg), _b(b_arg)
        {}

        /** Returns the "a" parameter of the distribtuion. */
        RealType a() const { return _a; }
        /** Returns the "b" parameter of the distribution. */
        RealType b() const { return _b; }

        /** Writes a @c param_type to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        { os << parm._a << ' ' << parm._b; return os; }

        /** Reads a @c param_type from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        { is >> parm._a >> std::ws >> parm._b; return is; }

        /** Returns true if the two sets of parameters are the same. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._a == rhs._a && lhs._b == rhs._b; }
        
        /** Returns true if the two sets of parameters are the different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        RealType _a;
        RealType _b;
    };

    /**
     * Constructs an @c extreme_value_distribution from its "a" and "b" parameters.
     *
     * Requires: b > 0
     */
    explicit extreme_value_distribution(RealType a_arg = 1.0, RealType b_arg = 1.0)
      : _a(a_arg), _b(b_arg)
    {}
    /** Constructs an @c extreme_value_distribution from its parameters. */
    explicit extreme_value_distribution(const param_type& parm)
      : _a(parm.a()), _b(parm.b())
    {}

    /**
     * Returns a random variate distributed according to the
     * @c extreme_value_distribution.
     */
    template<class URNG>
    RealType operator()(URNG& urng) const
    {
        using std::log;
        return _a - log(-log(uniform_01<RealType>()(urng))) * _b;
    }

    /**
     * Returns a random variate distributed accordint to the extreme
     * value distribution with parameters specified by @c param.
     */
    template<class URNG>
    RealType operator()(URNG& urng, const param_type& parm) const
    {
        return extreme_value_distribution(parm)(urng);
    }

    /** Returns the "a" parameter of the distribution. */
    RealType a() const { return _a; }
    /** Returns the "b" parameter of the distribution. */
    RealType b() const { return _b; }

    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return -std::numeric_limits<RealType>::infinity(); }
    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return std::numeric_limits<RealType>::infinity(); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_a, _b); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        _a = parm.a();
        _b = parm.b();
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /** Writes an @c extreme_value_distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, extreme_value_distribution, wd)
    {
        os << wd.param();
        return os;
    }

    /** Reads an @c extreme_value_distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, extreme_value_distribution, wd)
    {
        param_type parm;
        if(is >> parm) {
            wd.param(parm);
        }
        return is;
    }

    /**
     * Returns true if the two instances of @c extreme_value_distribution will
     * return identical sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(extreme_value_distribution, lhs, rhs)
    { return lhs._a == rhs._a && lhs._b == rhs._b; }
    
    /**
     * Returns true if the two instances of @c extreme_value_distribution will
     * return different sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(extreme_value_distribution)

private:
    RealType _a;
    RealType _b;
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_EXTREME_VALUE_DISTRIBUTION_HPP
