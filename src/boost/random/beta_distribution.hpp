/* boost random/beta_distribution.hpp header file
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

#ifndef BOOST_RANDOM_BETA_DISTRIBUTION_HPP
#define BOOST_RANDOM_BETA_DISTRIBUTION_HPP

#include <cassert>
#include <istream>
#include <iosfwd>
#include <boost/random/detail/operators.hpp>
#include <boost/random/gamma_distribution.hpp>

namespace boost {
namespace random {

/**
 * The beta distribution is a real-valued distribution which produces
 * values in the range [0, 1].  It has two parameters, alpha and beta.
 *
 * It has \f$\displaystyle p(x) = \frac{x^{\alpha-1}(1-x)^{\beta-1}}{B(\alpha, \beta)}\f$.
 */
template<class RealType = double>
class beta_distribution {
public:
    typedef RealType result_type;
    typedef RealType input_type;

    class param_type {
    public:
        typedef beta_distribution distribution_type;

        /**
         * Constructs a @c param_type from the "alpha" and "beta" parameters
         * of the distribution.
         *
         * Requires: alpha > 0, beta > 0
         */
        explicit param_type(RealType alpha_arg = RealType(1.0),
                            RealType beta_arg = RealType(1.0))
          : _alpha(alpha_arg), _beta(beta_arg)
        {
            assert(alpha_arg > 0);
            assert(beta_arg > 0);
        }

        /** Returns the "alpha" parameter of the distribtuion. */
        RealType alpha() const { return _alpha; }
        /** Returns the "beta" parameter of the distribution. */
        RealType beta() const { return _beta; }

        /** Writes a @c param_type to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        { os << parm._alpha << ' ' << parm._beta; return os; }

        /** Reads a @c param_type from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        { is >> parm._alpha >> std::ws >> parm._beta; return is; }

        /** Returns true if the two sets of parameters are the same. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._alpha == rhs._alpha && lhs._beta == rhs._beta; }
        
        /** Returns true if the two sets of parameters are the different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        RealType _alpha;
        RealType _beta;
    };

    /**
     * Constructs an @c beta_distribution from its "alpha" and "beta" parameters.
     *
     * Requires: alpha > 0, beta > 0
     */
    explicit beta_distribution(RealType alpha_arg = RealType(1.0),
                               RealType beta_arg = RealType(1.0))
      : _alpha(alpha_arg), _beta(beta_arg)
    {
        assert(alpha_arg > 0);
        assert(beta_arg > 0);
    }
    /** Constructs an @c beta_distribution from its parameters. */
    explicit beta_distribution(const param_type& parm)
      : _alpha(parm.alpha()), _beta(parm.beta())
    {}

    /**
     * Returns a random variate distributed according to the
     * beta distribution.
     */
    template<class URNG>
    RealType operator()(URNG& urng) const
    {
        RealType a = gamma_distribution<RealType>(_alpha, RealType(1.0))(urng);
        RealType b = gamma_distribution<RealType>(_beta, RealType(1.0))(urng);
        return a / (a + b);
    }

    /**
     * Returns a random variate distributed accordint to the beta
     * distribution with parameters specified by @c param.
     */
    template<class URNG>
    RealType operator()(URNG& urng, const param_type& parm) const
    {
        return beta_distribution(parm)(urng);
    }

    /** Returns the "alpha" parameter of the distribution. */
    RealType alpha() const { return _alpha; }
    /** Returns the "beta" parameter of the distribution. */
    RealType beta() const { return _beta; }

    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return RealType(0.0); }
    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return RealType(1.0); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_alpha, _beta); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        _alpha = parm.alpha();
        _beta = parm.beta();
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /** Writes an @c beta_distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, beta_distribution, wd)
    {
        os << wd.param();
        return os;
    }

    /** Reads an @c beta_distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, beta_distribution, wd)
    {
        param_type parm;
        if(is >> parm) {
            wd.param(parm);
        }
        return is;
    }

    /**
     * Returns true if the two instances of @c beta_distribution will
     * return identical sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(beta_distribution, lhs, rhs)
    { return lhs._alpha == rhs._alpha && lhs._beta == rhs._beta; }
    
    /**
     * Returns true if the two instances of @c beta_distribution will
     * return different sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(beta_distribution)

private:
    RealType _alpha;
    RealType _beta;
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_BETA_DISTRIBUTION_HPP
