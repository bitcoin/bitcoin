/* boost random/gamma_distribution.hpp header file
 *
 * Copyright Jens Maurer 2002
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 */

#ifndef BOOST_RANDOM_GAMMA_DISTRIBUTION_HPP
#define BOOST_RANDOM_GAMMA_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <istream>
#include <iosfwd>
#include <boost/assert.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/exponential_distribution.hpp>

namespace boost {
namespace random {

// The algorithm is taken from Knuth

/**
 * The gamma distribution is a continuous distribution with two
 * parameters alpha and beta.  It produces values > 0.
 *
 * It has
 * \f$\displaystyle p(x) = x^{\alpha-1}\frac{e^{-x/\beta}}{\beta^\alpha\Gamma(\alpha)}\f$.
 */
template<class RealType = double>
class gamma_distribution
{
public:
    typedef RealType input_type;
    typedef RealType result_type;

    class param_type
    {
    public:
        typedef gamma_distribution distribution_type;

        /**
         * Constructs a @c param_type object from the "alpha" and "beta"
         * parameters.
         *
         * Requires: alpha > 0 && beta > 0
         */
        param_type(const RealType& alpha_arg = RealType(1.0),
                   const RealType& beta_arg = RealType(1.0))
          : _alpha(alpha_arg), _beta(beta_arg)
        {
        }

        /** Returns the "alpha" parameter of the distribution. */
        RealType alpha() const { return _alpha; }
        /** Returns the "beta" parameter of the distribution. */
        RealType beta() const { return _beta; }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
        /** Writes the parameters to a @c std::ostream. */
        template<class CharT, class Traits>
        friend std::basic_ostream<CharT, Traits>&
        operator<<(std::basic_ostream<CharT, Traits>& os,
                   const param_type& parm)
        {
            os << parm._alpha << ' ' << parm._beta;
            return os;
        }
        
        /** Reads the parameters from a @c std::istream. */
        template<class CharT, class Traits>
        friend std::basic_istream<CharT, Traits>&
        operator>>(std::basic_istream<CharT, Traits>& is, param_type& parm)
        {
            is >> parm._alpha >> std::ws >> parm._beta;
            return is;
        }
#endif

        /** Returns true if the two sets of parameters are the same. */
        friend bool operator==(const param_type& lhs, const param_type& rhs)
        {
            return lhs._alpha == rhs._alpha && lhs._beta == rhs._beta;
        }
        /** Returns true if the two sets fo parameters are different. */
        friend bool operator!=(const param_type& lhs, const param_type& rhs)
        {
            return !(lhs == rhs);
        }
    private:
        RealType _alpha;
        RealType _beta;
    };

#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
    BOOST_STATIC_ASSERT(!std::numeric_limits<RealType>::is_integer);
#endif

    /**
     * Creates a new gamma_distribution with parameters "alpha" and "beta".
     *
     * Requires: alpha > 0 && beta > 0
     */
    explicit gamma_distribution(const result_type& alpha_arg = result_type(1.0),
                                const result_type& beta_arg = result_type(1.0))
      : _exp(result_type(1)), _alpha(alpha_arg), _beta(beta_arg)
    {
        BOOST_ASSERT(_alpha > result_type(0));
        BOOST_ASSERT(_beta > result_type(0));
        init();
    }

    /** Constructs a @c gamma_distribution from its parameters. */
    explicit gamma_distribution(const param_type& parm)
      : _exp(result_type(1)), _alpha(parm.alpha()), _beta(parm.beta())
    {
        init();
    }

    // compiler-generated copy ctor and assignment operator are fine

    /** Returns the "alpha" paramter of the distribution. */
    RealType alpha() const { return _alpha; }
    /** Returns the "beta" parameter of the distribution. */
    RealType beta() const { return _beta; }
    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return 0; }
    /* Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return (std::numeric_limits<RealType>::infinity)(); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_alpha, _beta); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        _alpha = parm.alpha();
        _beta = parm.beta();
        init();
    }
    
    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { _exp.reset(); }

    /**
     * Returns a random variate distributed according to
     * the gamma distribution.
     */
    template<class Engine>
    result_type operator()(Engine& eng)
    {
#ifndef BOOST_NO_STDC_NAMESPACE
        // allow for Koenig lookup
        using std::tan; using std::sqrt; using std::exp; using std::log;
        using std::pow;
#endif
        if(_alpha == result_type(1)) {
            return _exp(eng) * _beta;
        } else if(_alpha > result_type(1)) {
            // Can we have a boost::mathconst please?
            const result_type pi = result_type(3.14159265358979323846);
            for(;;) {
                result_type y = tan(pi * uniform_01<RealType>()(eng));
                result_type x = sqrt(result_type(2)*_alpha-result_type(1))*y
                    + _alpha-result_type(1);
                if(x <= result_type(0))
                    continue;
                if(uniform_01<RealType>()(eng) >
                    (result_type(1)+y*y) * exp((_alpha-result_type(1))
                                               *log(x/(_alpha-result_type(1)))
                                               - sqrt(result_type(2)*_alpha
                                                      -result_type(1))*y))
                    continue;
                return x * _beta;
            }
        } else /* alpha < 1.0 */ {
            for(;;) {
                result_type u = uniform_01<RealType>()(eng);
                result_type y = _exp(eng);
                result_type x, q;
                if(u < _p) {
                    x = exp(-y/_alpha);
                    q = _p*exp(-x);
                } else {
                    x = result_type(1)+y;
                    q = _p + (result_type(1)-_p) * pow(x,_alpha-result_type(1));
                }
                if(u >= q)
                    continue;
                return x * _beta;
            }
        }
    }

    template<class URNG>
    RealType operator()(URNG& urng, const param_type& parm) const
    {
        return gamma_distribution(parm)(urng);
    }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
    /** Writes a @c gamma_distribution to a @c std::ostream. */
    template<class CharT, class Traits>
    friend std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& os,
               const gamma_distribution& gd)
    {
        os << gd.param();
        return os;
    }
    
    /** Reads a @c gamma_distribution from a @c std::istream. */
    template<class CharT, class Traits>
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& is, gamma_distribution& gd)
    {
        gd.read(is);
        return is;
    }
#endif

    /**
     * Returns true if the two distributions will produce identical
     * sequences of random variates given equal generators.
     */
    friend bool operator==(const gamma_distribution& lhs,
                           const gamma_distribution& rhs)
    {
        return lhs._alpha == rhs._alpha
            && lhs._beta == rhs._beta
            && lhs._exp == rhs._exp;
    }

    /**
     * Returns true if the two distributions can produce different
     * sequences of random variates, given equal generators.
     */
    friend bool operator!=(const gamma_distribution& lhs,
                           const gamma_distribution& rhs)
    {
        return !(lhs == rhs);
    }

private:
    /// \cond hide_private_members

    template<class CharT, class Traits>
    void read(std::basic_istream<CharT, Traits>& is)
    {
        param_type parm;
        if(is >> parm) {
            param(parm);
        }
    }

    void init()
    {
#ifndef BOOST_NO_STDC_NAMESPACE
        // allow for Koenig lookup
        using std::exp;
#endif
        _p = exp(result_type(1)) / (_alpha + exp(result_type(1)));
    }
    /// \endcond

    exponential_distribution<RealType> _exp;
    result_type _alpha;
    result_type _beta;
    // some data precomputed from the parameters
    result_type _p;
};


} // namespace random

using random::gamma_distribution;

} // namespace boost

#endif // BOOST_RANDOM_GAMMA_DISTRIBUTION_HPP
