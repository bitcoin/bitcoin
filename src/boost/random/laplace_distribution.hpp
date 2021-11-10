/* boost random/laplace_distribution.hpp header file
 *
 * Copyright Steven Watanabe 2014
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 */

#ifndef BOOST_RANDOM_LAPLACE_DISTRIBUTION_HPP
#define BOOST_RANDOM_LAPLACE_DISTRIBUTION_HPP

#include <cassert>
#include <istream>
#include <iosfwd>
#include <boost/random/detail/operators.hpp>
#include <boost/random/exponential_distribution.hpp>

namespace boost {
namespace random {

/**
 * The laplace distribution is a real-valued distribution with
 * two parameters, mean and beta.
 *
 * It has \f$\displaystyle p(x) = \frac{e^-{\frac{|x-\mu|}{\beta}}}{2\beta}\f$.
 */
template<class RealType = double>
class laplace_distribution {
public:
    typedef RealType result_type;
    typedef RealType input_type;

    class param_type {
    public:
        typedef laplace_distribution distribution_type;

        /**
         * Constructs a @c param_type from the "mean" and "beta" parameters
         * of the distribution.
         */
        explicit param_type(RealType mean_arg = RealType(0.0),
                            RealType beta_arg = RealType(1.0))
          : _mean(mean_arg), _beta(beta_arg)
        {}

        /** Returns the "mean" parameter of the distribtuion. */
        RealType mean() const { return _mean; }
        /** Returns the "beta" parameter of the distribution. */
        RealType beta() const { return _beta; }

        /** Writes a @c param_type to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        { os << parm._mean << ' ' << parm._beta; return os; }

        /** Reads a @c param_type from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        { is >> parm._mean >> std::ws >> parm._beta; return is; }

        /** Returns true if the two sets of parameters are the same. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._mean == rhs._mean && lhs._beta == rhs._beta; }
        
        /** Returns true if the two sets of parameters are the different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        RealType _mean;
        RealType _beta;
    };

    /**
     * Constructs an @c laplace_distribution from its "mean" and "beta" parameters.
     */
    explicit laplace_distribution(RealType mean_arg = RealType(0.0),
                               RealType beta_arg = RealType(1.0))
      : _mean(mean_arg), _beta(beta_arg)
    {}
    /** Constructs an @c laplace_distribution from its parameters. */
    explicit laplace_distribution(const param_type& parm)
      : _mean(parm.mean()), _beta(parm.beta())
    {}

    /**
     * Returns a random variate distributed according to the
     * laplace distribution.
     */
    template<class URNG>
    RealType operator()(URNG& urng) const
    {
        RealType exponential = exponential_distribution<RealType>()(urng);
        if(uniform_01<RealType>()(urng) < 0.5)
            exponential = -exponential;
        return _mean + _beta * exponential;
    }

    /**
     * Returns a random variate distributed accordint to the laplace
     * distribution with parameters specified by @c param.
     */
    template<class URNG>
    RealType operator()(URNG& urng, const param_type& parm) const
    {
        return laplace_distribution(parm)(urng);
    }

    /** Returns the "mean" parameter of the distribution. */
    RealType mean() const { return _mean; }
    /** Returns the "beta" parameter of the distribution. */
    RealType beta() const { return _beta; }

    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return RealType(-std::numeric_limits<RealType>::infinity()); }
    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return RealType(std::numeric_limits<RealType>::infinity()); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_mean, _beta); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        _mean = parm.mean();
        _beta = parm.beta();
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /** Writes an @c laplace_distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, laplace_distribution, wd)
    {
        os << wd.param();
        return os;
    }

    /** Reads an @c laplace_distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, laplace_distribution, wd)
    {
        param_type parm;
        if(is >> parm) {
            wd.param(parm);
        }
        return is;
    }

    /**
     * Returns true if the two instances of @c laplace_distribution will
     * return identical sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(laplace_distribution, lhs, rhs)
    { return lhs._mean == rhs._mean && lhs._beta == rhs._beta; }
    
    /**
     * Returns true if the two instances of @c laplace_distribution will
     * return different sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(laplace_distribution)

private:
    RealType _mean;
    RealType _beta;
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_LAPLACE_DISTRIBUTION_HPP
