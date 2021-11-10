/* boost random/negative_binomial_distribution.hpp header file
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

#ifndef BOOST_RANDOM_NEGATIVE_BINOMIAL_DISTRIBUTION_HPP_INCLUDED
#define BOOST_RANDOM_NEGATIVE_BINOMIAL_DISTRIBUTION_HPP_INCLUDED

#include <iosfwd>

#include <boost/limits.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/gamma_distribution.hpp>
#include <boost/random/poisson_distribution.hpp>

namespace boost {
namespace random {

/**
 * The negative binomial distribution is an integer valued
 * distribution with two parameters, @c k and @c p.  The
 * distribution produces non-negative values.
 *
 * The distribution function is
 * \f$\displaystyle P(i) = {k+i-1\choose i}p^k(1-p)^i\f$.
 *
 * This implementation uses a gamma-poisson mixture.
 */
template<class IntType = int, class RealType = double>
class negative_binomial_distribution {
public:
    typedef IntType result_type;
    typedef RealType input_type;

    class param_type {
    public:
        typedef negative_binomial_distribution distribution_type;
        /**
         * Construct a param_type object.  @c k and @c p
         * are the parameters of the distribution.
         *
         * Requires: k >=0 && 0 <= p <= 1
         */
        explicit param_type(IntType k_arg = 1, RealType p_arg = RealType (0.5))
          : _k(k_arg), _p(p_arg)
        {}
        /** Returns the @c k parameter of the distribution. */
        IntType k() const { return _k; }
        /** Returns the @c p parameter of the distribution. */
        RealType p() const { return _p; }
#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
        /** Writes the parameters of the distribution to a @c std::ostream. */
        template<class CharT, class Traits>
        friend std::basic_ostream<CharT,Traits>&
        operator<<(std::basic_ostream<CharT,Traits>& os,
                   const param_type& parm)
        {
            os << parm._p << " " << parm._k;
            return os;
        }
    
        /** Reads the parameters of the distribution from a @c std::istream. */
        template<class CharT, class Traits>
        friend std::basic_istream<CharT,Traits>&
        operator>>(std::basic_istream<CharT,Traits>& is, param_type& parm)
        {
            is >> parm._p >> std::ws >> parm._k;
            return is;
        }
#endif
        /** Returns true if the parameters have the same values. */
        friend bool operator==(const param_type& lhs, const param_type& rhs)
        {
            return lhs._k == rhs._k && lhs._p == rhs._p;
        }
        /** Returns true if the parameters have different values. */
        friend bool operator!=(const param_type& lhs, const param_type& rhs)
        {
            return !(lhs == rhs);
        }
    private:
        IntType _k;
        RealType _p;
    };
    
    /**
     * Construct a @c negative_binomial_distribution object. @c k and @c p
     * are the parameters of the distribution.
     *
     * Requires: k >=0 && 0 <= p <= 1
     */
    explicit negative_binomial_distribution(IntType k_arg = 1,
                                            RealType p_arg = RealType(0.5))
      : _k(k_arg), _p(p_arg)
    {}
    
    /**
     * Construct an @c negative_binomial_distribution object from the
     * parameters.
     */
    explicit negative_binomial_distribution(const param_type& parm)
      : _k(parm.k()), _p(parm.p())
    {}
    
    /**
     * Returns a random variate distributed according to the
     * negative binomial distribution.
     */
    template<class URNG>
    IntType operator()(URNG& urng) const
    {
        gamma_distribution<RealType> gamma(_k, (1-_p)/_p);
        poisson_distribution<IntType, RealType> poisson(gamma(urng));
        return poisson(urng);
    }
    
    /**
     * Returns a random variate distributed according to the negative
     * binomial distribution with parameters specified by @c param.
     */
    template<class URNG>
    IntType operator()(URNG& urng, const param_type& parm) const
    {
        return negative_binomial_distribution(parm)(urng);
    }

    /** Returns the @c k parameter of the distribution. */
    IntType k() const { return _k; }
    /** Returns the @c p parameter of the distribution. */
    RealType p() const { return _p; }

    /** Returns the smallest value that the distribution can produce. */
    IntType min BOOST_PREVENT_MACRO_SUBSTITUTION() const { return 0; }
    /** Returns the largest value that the distribution can produce. */
    IntType max BOOST_PREVENT_MACRO_SUBSTITUTION() const
    { return (std::numeric_limits<IntType>::max)(); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_k, _p); }
    /** Sets parameters of the distribution. */
    void param(const param_type& parm)
    {
        _k = parm.k();
        _p = parm.p();
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
    /** Writes the parameters of the distribution to a @c std::ostream. */
    template<class CharT, class Traits>
    friend std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& os,
               const negative_binomial_distribution& bd)
    {
        os << bd.param();
        return os;
    }
    
    /** Reads the parameters of the distribution from a @c std::istream. */
    template<class CharT, class Traits>
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& is,
               negative_binomial_distribution& bd)
    {
        bd.read(is);
        return is;
    }
#endif

    /** Returns true if the two distributions will produce the same
        sequence of values, given equal generators. */
    friend bool operator==(const negative_binomial_distribution& lhs,
                           const negative_binomial_distribution& rhs)
    {
        return lhs._k == rhs._k && lhs._p == rhs._p;
    }
    /** Returns true if the two distributions could produce different
        sequences of values, given equal generators. */
    friend bool operator!=(const negative_binomial_distribution& lhs,
                           const negative_binomial_distribution& rhs)
    {
        return !(lhs == rhs);
    }

private:

    /// @cond \show_private

    template<class CharT, class Traits>
    void read(std::basic_istream<CharT, Traits>& is) {
        param_type parm;
        if(is >> parm) {
            param(parm);
        }
    }

    // parameters
    IntType _k;
    RealType _p;

    /// @endcond
};

}

}

#endif
