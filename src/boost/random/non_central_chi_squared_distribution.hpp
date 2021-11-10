/* boost random/non_central_chi_squared_distribution.hpp header file
 *
 * Copyright Thijs van den Berg 2014
 * 
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 */

#ifndef BOOST_RANDOM_NON_CENTRAL_CHI_SQUARED_DISTRIBUTION_HPP
#define BOOST_RANDOM_NON_CENTRAL_CHI_SQUARED_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <iosfwd>
#include <istream>
#include <boost/limits.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/chi_squared_distribution.hpp>
#include <boost/random/poisson_distribution.hpp>

namespace boost {
namespace random {

/**
 * The noncentral chi-squared distribution is a real valued distribution with
 * two parameter, @c k and @c lambda.  The distribution produces values > 0.
 *
 * This is the distribution of the sum of squares of k Normal distributed
 * variates each with variance one and \f$\lambda\f$ the sum of squares of the
 * normal means.
 *
 * The distribution function is
 * \f$\displaystyle P(x) = \frac{1}{2} e^{-(x+\lambda)/2} \left( \frac{x}{\lambda} \right)^{k/4-1/2} I_{k/2-1}( \sqrt{\lambda x} )\f$.
 *  where  \f$\displaystyle I_\nu(z)\f$ is a modified Bessel function of the
 * first kind.
 *
 * The algorithm is taken from
 *
 *  @blockquote
 *  "Monte Carlo Methods in Financial Engineering", Paul Glasserman,
 *  2003, XIII, 596 p, Stochastic Modelling and Applied Probability, Vol. 53,
 *  ISBN 978-0-387-21617-1, p 124, Fig. 3.5.
 *  @endblockquote
 */
template <typename RealType = double>
class non_central_chi_squared_distribution {
public:
    typedef RealType result_type;
    typedef RealType input_type;
    
    class param_type {
    public:
        typedef non_central_chi_squared_distribution distribution_type;
        
        /**
         * Constructs the parameters of a non_central_chi_squared_distribution.
         * @c k and @c lambda are the parameter of the distribution.
         *
         * Requires: k > 0 && lambda > 0
         */
        explicit
        param_type(RealType k_arg = RealType(1), RealType lambda_arg = RealType(1))
        : _k(k_arg), _lambda(lambda_arg)
        {
            BOOST_ASSERT(k_arg > RealType(0));
            BOOST_ASSERT(lambda_arg > RealType(0));
        }
        
        /** Returns the @c k parameter of the distribution */
        RealType k() const { return _k; }
        
        /** Returns the @c lambda parameter of the distribution */
        RealType lambda() const { return _lambda; }

        /** Writes the parameters of the distribution to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        {
            os << parm._k << ' ' << parm._lambda;
            return os;
        }
        
        /** Reads the parameters of the distribution from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        {
            is >> parm._k >> std::ws >> parm._lambda;
            return is;
        }

        /** Returns true if the parameters have the same values. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._k == rhs._k && lhs._lambda == rhs._lambda; }
        
        /** Returns true if the parameters have different values. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)
        
    private:
        RealType _k;
        RealType _lambda;
    };

    /**
     * Construct a @c non_central_chi_squared_distribution object. @c k and
     * @c lambda are the parameter of the distribution.
     *
     * Requires: k > 0 && lambda > 0
     */
    explicit
    non_central_chi_squared_distribution(RealType k_arg = RealType(1), RealType lambda_arg = RealType(1))
      : _param(k_arg, lambda_arg)
    {
        BOOST_ASSERT(k_arg > RealType(0));
        BOOST_ASSERT(lambda_arg > RealType(0));
    }

    /**
     * Construct a @c non_central_chi_squared_distribution object from the parameter.
     */
    explicit
    non_central_chi_squared_distribution(const param_type& parm)
      : _param( parm )
    { }
    
    /**
     * Returns a random variate distributed according to the
     * non central chi squared distribution specified by @c param.
     */
    template<typename URNG>
    RealType operator()(URNG& eng, const param_type& parm) const
    { return non_central_chi_squared_distribution(parm)(eng); }
    
    /**
     * Returns a random variate distributed according to the
     * non central chi squared distribution.
     */
    template<typename URNG> 
    RealType operator()(URNG& eng) 
    {
        using std::sqrt;
        if (_param.k() > 1) {
            boost::random::normal_distribution<RealType> n_dist;
            boost::random::chi_squared_distribution<RealType> c_dist(_param.k() - RealType(1));
            RealType _z = n_dist(eng);
            RealType _x = c_dist(eng);
            RealType term1 = _z + sqrt(_param.lambda());
            return term1*term1 + _x;
        }
        else {
            boost::random::poisson_distribution<> p_dist(_param.lambda()/RealType(2));
            boost::random::poisson_distribution<>::result_type _p = p_dist(eng);
            boost::random::chi_squared_distribution<RealType> c_dist(_param.k() + RealType(2)*_p);
            return c_dist(eng);
        }
    }

    /** Returns the @c k parameter of the distribution. */
    RealType k() const { return _param.k(); }
    
    /** Returns the @c lambda parameter of the distribution. */
    RealType lambda() const { return _param.lambda(); }
    
    /** Returns the parameters of the distribution. */
    param_type param() const { return _param; }
    
    /** Sets parameters of the distribution. */
    void param(const param_type& parm) { _param = parm; }
    
    /** Resets the distribution, so that subsequent uses does not depend on values already produced by it.*/
    void reset() {}
    
    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION() const
    { return RealType(0); }
    
    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION() const
    { return (std::numeric_limits<RealType>::infinity)(); }

    /** Writes the parameters of the distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, non_central_chi_squared_distribution, dist)
    {
        os << dist.param();
        return os;
    }
    
    /** reads the parameters of the distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, non_central_chi_squared_distribution, dist)
    {
        param_type parm;
        if(is >> parm) {
            dist.param(parm);
        }
        return is;
    }

    /** Returns true if two distributions have the same parameters and produce 
        the same sequence of random numbers given equal generators.*/
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(non_central_chi_squared_distribution, lhs, rhs)
    { return lhs.param() == rhs.param(); }
    
    /** Returns true if two distributions have different parameters and/or can produce 
       different sequences of random numbers given equal generators.*/
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(non_central_chi_squared_distribution)
    
private:

    /// @cond show_private
    param_type  _param;
    /// @endcond
};

} // namespace random
} // namespace boost

#endif
