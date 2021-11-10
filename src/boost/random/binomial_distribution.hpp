/* boost random/binomial_distribution.hpp header file
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

#ifndef BOOST_RANDOM_BINOMIAL_DISTRIBUTION_HPP_INCLUDED
#define BOOST_RANDOM_BINOMIAL_DISTRIBUTION_HPP_INCLUDED

#include <boost/config/no_tr1/cmath.hpp>
#include <cstdlib>
#include <iosfwd>

#include <boost/random/detail/config.hpp>
#include <boost/random/uniform_01.hpp>

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {
namespace random {

namespace detail {

template<class RealType>
struct binomial_table {
    static const RealType table[10];
};

template<class RealType>
const RealType binomial_table<RealType>::table[10] = {
    0.08106146679532726,
    0.04134069595540929,
    0.02767792568499834,
    0.02079067210376509,
    0.01664469118982119,
    0.01387612882307075,
    0.01189670994589177,
    0.01041126526197209,
    0.009255462182712733,
    0.008330563433362871
};

}

/**
 * The binomial distribution is an integer valued distribution with
 * two parameters, @c t and @c p.  The values of the distribution
 * are within the range [0,t].
 *
 * The distribution function is
 * \f$\displaystyle P(k) = {t \choose k}p^k(1-p)^{t-k}\f$.
 *
 * The algorithm used is the BTRD algorithm described in
 *
 *  @blockquote
 *  "The generation of binomial random variates", Wolfgang Hormann,
 *  Journal of Statistical Computation and Simulation, Volume 46,
 *  Issue 1 & 2 April 1993 , pages 101 - 110
 *  @endblockquote
 */
template<class IntType = int, class RealType = double>
class binomial_distribution {
public:
    typedef IntType result_type;
    typedef RealType input_type;

    class param_type {
    public:
        typedef binomial_distribution distribution_type;
        /**
         * Construct a param_type object.  @c t and @c p
         * are the parameters of the distribution.
         *
         * Requires: t >=0 && 0 <= p <= 1
         */
        explicit param_type(IntType t_arg = 1, RealType p_arg = RealType (0.5))
          : _t(t_arg), _p(p_arg)
        {}
        /** Returns the @c t parameter of the distribution. */
        IntType t() const { return _t; }
        /** Returns the @c p parameter of the distribution. */
        RealType p() const { return _p; }
#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
        /** Writes the parameters of the distribution to a @c std::ostream. */
        template<class CharT, class Traits>
        friend std::basic_ostream<CharT,Traits>&
        operator<<(std::basic_ostream<CharT,Traits>& os,
                   const param_type& parm)
        {
            os << parm._p << " " << parm._t;
            return os;
        }
    
        /** Reads the parameters of the distribution from a @c std::istream. */
        template<class CharT, class Traits>
        friend std::basic_istream<CharT,Traits>&
        operator>>(std::basic_istream<CharT,Traits>& is, param_type& parm)
        {
            is >> parm._p >> std::ws >> parm._t;
            return is;
        }
#endif
        /** Returns true if the parameters have the same values. */
        friend bool operator==(const param_type& lhs, const param_type& rhs)
        {
            return lhs._t == rhs._t && lhs._p == rhs._p;
        }
        /** Returns true if the parameters have different values. */
        friend bool operator!=(const param_type& lhs, const param_type& rhs)
        {
            return !(lhs == rhs);
        }
    private:
        IntType _t;
        RealType _p;
    };
    
    /**
     * Construct a @c binomial_distribution object. @c t and @c p
     * are the parameters of the distribution.
     *
     * Requires: t >=0 && 0 <= p <= 1
     */
    explicit binomial_distribution(IntType t_arg = 1,
                                   RealType p_arg = RealType(0.5))
      : _t(t_arg), _p(p_arg)
    {
        init();
    }
    
    /**
     * Construct an @c binomial_distribution object from the
     * parameters.
     */
    explicit binomial_distribution(const param_type& parm)
      : _t(parm.t()), _p(parm.p())
    {
        init();
    }
    
    /**
     * Returns a random variate distributed according to the
     * binomial distribution.
     */
    template<class URNG>
    IntType operator()(URNG& urng) const
    {
        if(use_inversion()) {
            if(0.5 < _p) {
                return _t - invert(_t, 1-_p, urng);
            } else {
                return invert(_t, _p, urng);
            }
        } else if(0.5 < _p) {
            return _t - generate(urng);
        } else {
            return generate(urng);
        }
    }
    
    /**
     * Returns a random variate distributed according to the
     * binomial distribution with parameters specified by @c param.
     */
    template<class URNG>
    IntType operator()(URNG& urng, const param_type& parm) const
    {
        return binomial_distribution(parm)(urng);
    }

    /** Returns the @c t parameter of the distribution. */
    IntType t() const { return _t; }
    /** Returns the @c p parameter of the distribution. */
    RealType p() const { return _p; }

    /** Returns the smallest value that the distribution can produce. */
    IntType min BOOST_PREVENT_MACRO_SUBSTITUTION() const { return 0; }
    /** Returns the largest value that the distribution can produce. */
    IntType max BOOST_PREVENT_MACRO_SUBSTITUTION() const { return _t; }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_t, _p); }
    /** Sets parameters of the distribution. */
    void param(const param_type& parm)
    {
        _t = parm.t();
        _p = parm.p();
        init();
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
               const binomial_distribution& bd)
    {
        os << bd.param();
        return os;
    }
    
    /** Reads the parameters of the distribution from a @c std::istream. */
    template<class CharT, class Traits>
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& is, binomial_distribution& bd)
    {
        bd.read(is);
        return is;
    }
#endif

    /** Returns true if the two distributions will produce the same
        sequence of values, given equal generators. */
    friend bool operator==(const binomial_distribution& lhs,
                           const binomial_distribution& rhs)
    {
        return lhs._t == rhs._t && lhs._p == rhs._p;
    }
    /** Returns true if the two distributions could produce different
        sequences of values, given equal generators. */
    friend bool operator!=(const binomial_distribution& lhs,
                           const binomial_distribution& rhs)
    {
        return !(lhs == rhs);
    }

private:

    /// @cond show_private

    template<class CharT, class Traits>
    void read(std::basic_istream<CharT, Traits>& is) {
        param_type parm;
        if(is >> parm) {
            param(parm);
        }
    }

    bool use_inversion() const
    {
        // BTRD is safe when np >= 10
        return m < 11;
    }

    // computes the correction factor for the Stirling approximation
    // for log(k!)
    static RealType fc(IntType k)
    {
        if(k < 10) return detail::binomial_table<RealType>::table[k];
        else {
            RealType ikp1 = RealType(1) / (k + 1);
            return (RealType(1)/12
                 - (RealType(1)/360
                 - (RealType(1)/1260)*(ikp1*ikp1))*(ikp1*ikp1))*ikp1;
        }
    }

    void init()
    {
        using std::sqrt;
        using std::pow;

        RealType p = (0.5 < _p)? (1 - _p) : _p;
        IntType t = _t;
        
        m = static_cast<IntType>((t+1)*p);

        if(use_inversion()) {
            _u.q_n = pow((1 - p), static_cast<RealType>(t));
        } else {
            _u.btrd.r = p/(1-p);
            _u.btrd.nr = (t+1)*_u.btrd.r;
            _u.btrd.npq = t*p*(1-p);
            RealType sqrt_npq = sqrt(_u.btrd.npq);
            _u.btrd.b = 1.15 + 2.53 * sqrt_npq;
            _u.btrd.a = -0.0873 + 0.0248*_u.btrd.b + 0.01*p;
            _u.btrd.c = t*p + 0.5;
            _u.btrd.alpha = (2.83 + 5.1/_u.btrd.b) * sqrt_npq;
            _u.btrd.v_r = 0.92 - 4.2/_u.btrd.b;
            _u.btrd.u_rv_r = 0.86*_u.btrd.v_r;
        }
    }

    template<class URNG>
    result_type generate(URNG& urng) const
    {
        using std::floor;
        using std::abs;
        using std::log;

        while(true) {
            RealType u;
            RealType v = uniform_01<RealType>()(urng);
            if(v <= _u.btrd.u_rv_r) {
                u = v/_u.btrd.v_r - 0.43;
                return static_cast<IntType>(floor(
                    (2*_u.btrd.a/(0.5 - abs(u)) + _u.btrd.b)*u + _u.btrd.c));
            }

            if(v >= _u.btrd.v_r) {
                u = uniform_01<RealType>()(urng) - 0.5;
            } else {
                u = v/_u.btrd.v_r - 0.93;
                u = ((u < 0)? -0.5 : 0.5) - u;
                v = uniform_01<RealType>()(urng) * _u.btrd.v_r;
            }

            RealType us = 0.5 - abs(u);
            IntType k = static_cast<IntType>(floor((2*_u.btrd.a/us + _u.btrd.b)*u + _u.btrd.c));
            if(k < 0 || k > _t) continue;
            v = v*_u.btrd.alpha/(_u.btrd.a/(us*us) + _u.btrd.b);
            RealType km = abs(k - m);
            if(km <= 15) {
                RealType f = 1;
                if(m < k) {
                    IntType i = m;
                    do {
                        ++i;
                        f = f*(_u.btrd.nr/i - _u.btrd.r);
                    } while(i != k);
                } else if(m > k) {
                    IntType i = k;
                    do {
                        ++i;
                        v = v*(_u.btrd.nr/i - _u.btrd.r);
                    } while(i != m);
                }
                if(v <= f) return k;
                else continue;
            } else {
                // final acceptance/rejection
                v = log(v);
                RealType rho =
                    (km/_u.btrd.npq)*(((km/3. + 0.625)*km + 1./6)/_u.btrd.npq + 0.5);
                RealType t = -km*km/(2*_u.btrd.npq);
                if(v < t - rho) return k;
                if(v > t + rho) continue;

                IntType nm = _t - m + 1;
                RealType h = (m + 0.5)*log((m + 1)/(_u.btrd.r*nm))
                           + fc(m) + fc(_t - m);

                IntType nk = _t - k + 1;
                if(v <= h + (_t+1)*log(static_cast<RealType>(nm)/nk)
                          + (k + 0.5)*log(nk*_u.btrd.r/(k+1))
                          - fc(k)
                          - fc(_t - k))
                {
                    return k;
                } else {
                    continue;
                }
            }
        }
    }

    template<class URNG>
    IntType invert(IntType t, RealType p, URNG& urng) const
    {
        RealType q = 1 - p;
        RealType s = p / q;
        RealType a = (t + 1) * s;
        RealType r = _u.q_n;
        RealType u = uniform_01<RealType>()(urng);
        IntType x = 0;
        while(u > r) {
            u = u - r;
            ++x;
            RealType r1 = ((a/x) - s) * r;
            // If r gets too small then the round-off error
            // becomes a problem.  At this point, p(i) is
            // decreasing exponentially, so if we just call
            // it 0, it's close enough.  Note that the
            // minimum value of q_n is about 1e-7, so we
            // may need to be a little careful to make sure that
            // we don't terminate the first time through the loop
            // for float.  (Hence the test that r is decreasing)
            if(r1 < std::numeric_limits<RealType>::epsilon() && r1 < r) {
                break;
            }
            r = r1;
        }
        return x;
    }

    // parameters
    IntType _t;
    RealType _p;

    // common data
    IntType m;

    union {
        // for btrd
        struct {
            RealType r;
            RealType nr;
            RealType npq;
            RealType b;
            RealType a;
            RealType c;
            RealType alpha;
            RealType v_r;
            RealType u_rv_r;
        } btrd;
        // for inversion
        RealType q_n;
    } _u;

    /// @endcond
};

}

// backwards compatibility
using random::binomial_distribution;

}

#include <boost/random/detail/enable_warnings.hpp>

#endif
