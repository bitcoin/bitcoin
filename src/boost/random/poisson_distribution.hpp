/* boost random/poisson_distribution.hpp header file
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

#ifndef BOOST_RANDOM_POISSON_DISTRIBUTION_HPP
#define BOOST_RANDOM_POISSON_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <cstdlib>
#include <iosfwd>
#include <boost/assert.hpp>
#include <boost/limits.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/detail/config.hpp>

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {
namespace random {

namespace detail {

template<class RealType>
struct poisson_table {
    static RealType value[10];
};

template<class RealType>
RealType poisson_table<RealType>::value[10] = {
    0.0,
    0.0,
    0.69314718055994529,
    1.7917594692280550,
    3.1780538303479458,
    4.7874917427820458,
    6.5792512120101012,
    8.5251613610654147,
    10.604602902745251,
    12.801827480081469
};

}

/**
 * An instantiation of the class template @c poisson_distribution is a
 * model of \random_distribution.  The poisson distribution has
 * \f$p(i) = \frac{e^{-\lambda}\lambda^i}{i!}\f$
 *
 * This implementation is based on the PTRD algorithm described
 * 
 *  @blockquote
 *  "The transformed rejection method for generating Poisson random variables",
 *  Wolfgang Hormann, Insurance: Mathematics and Economics
 *  Volume 12, Issue 1, February 1993, Pages 39-45
 *  @endblockquote
 */
template<class IntType = int, class RealType = double>
class poisson_distribution {
public:
    typedef IntType result_type;
    typedef RealType input_type;

    class param_type {
    public:
        typedef poisson_distribution distribution_type;
        /**
         * Construct a param_type object with the parameter "mean"
         *
         * Requires: mean > 0
         */
        explicit param_type(RealType mean_arg = RealType(1))
          : _mean(mean_arg)
        {
            BOOST_ASSERT(_mean > 0);
        }
        /* Returns the "mean" parameter of the distribution. */
        RealType mean() const { return _mean; }
#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
        /** Writes the parameters of the distribution to a @c std::ostream. */
        template<class CharT, class Traits>
        friend std::basic_ostream<CharT, Traits>&
        operator<<(std::basic_ostream<CharT, Traits>& os,
                   const param_type& parm)
        {
            os << parm._mean;
            return os;
        }

        /** Reads the parameters of the distribution from a @c std::istream. */
        template<class CharT, class Traits>
        friend std::basic_istream<CharT, Traits>&
        operator>>(std::basic_istream<CharT, Traits>& is, param_type& parm)
        {
            is >> parm._mean;
            return is;
        }
#endif
        /** Returns true if the parameters have the same values. */
        friend bool operator==(const param_type& lhs, const param_type& rhs)
        {
            return lhs._mean == rhs._mean;
        }
        /** Returns true if the parameters have different values. */
        friend bool operator!=(const param_type& lhs, const param_type& rhs)
        {
            return !(lhs == rhs);
        }
    private:
        RealType _mean;
    };
    
    /**
     * Constructs a @c poisson_distribution with the parameter @c mean.
     *
     * Requires: mean > 0
     */
    explicit poisson_distribution(RealType mean_arg = RealType(1))
      : _mean(mean_arg)
    {
        BOOST_ASSERT(_mean > 0);
        init();
    }
    
    /**
     * Construct an @c poisson_distribution object from the
     * parameters.
     */
    explicit poisson_distribution(const param_type& parm)
      : _mean(parm.mean())
    {
        init();
    }
    
    /**
     * Returns a random variate distributed according to the
     * poisson distribution.
     */
    template<class URNG>
    IntType operator()(URNG& urng) const
    {
        if(use_inversion()) {
            return invert(urng);
        } else {
            return generate(urng);
        }
    }

    /**
     * Returns a random variate distributed according to the
     * poisson distribution with parameters specified by param.
     */
    template<class URNG>
    IntType operator()(URNG& urng, const param_type& parm) const
    {
        return poisson_distribution(parm)(urng);
    }

    /** Returns the "mean" parameter of the distribution. */
    RealType mean() const { return _mean; }
    
    /** Returns the smallest value that the distribution can produce. */
    IntType min BOOST_PREVENT_MACRO_SUBSTITUTION() const { return 0; }
    /** Returns the largest value that the distribution can produce. */
    IntType max BOOST_PREVENT_MACRO_SUBSTITUTION() const
    { return (std::numeric_limits<IntType>::max)(); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_mean); }
    /** Sets parameters of the distribution. */
    void param(const param_type& parm)
    {
        _mean = parm.mean();
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
               const poisson_distribution& pd)
    {
        os << pd.param();
        return os;
    }
    
    /** Reads the parameters of the distribution from a @c std::istream. */
    template<class CharT, class Traits>
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& is, poisson_distribution& pd)
    {
        pd.read(is);
        return is;
    }
#endif
    
    /** Returns true if the two distributions will produce the same
        sequence of values, given equal generators. */
    friend bool operator==(const poisson_distribution& lhs,
                           const poisson_distribution& rhs)
    {
        return lhs._mean == rhs._mean;
    }
    /** Returns true if the two distributions could produce different
        sequences of values, given equal generators. */
    friend bool operator!=(const poisson_distribution& lhs,
                           const poisson_distribution& rhs)
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
        return _mean < 10;
    }

    static RealType log_factorial(IntType k)
    {
        BOOST_ASSERT(k >= 0);
        BOOST_ASSERT(k < 10);
        return detail::poisson_table<RealType>::value[k];
    }

    void init()
    {
        using std::sqrt;
        using std::exp;

        if(use_inversion()) {
            _u._exp_mean = exp(-_mean);
        } else {
            _u._ptrd.smu = sqrt(_mean);
            _u._ptrd.b = 0.931 + 2.53 * _u._ptrd.smu;
            _u._ptrd.a = -0.059 + 0.02483 * _u._ptrd.b;
            _u._ptrd.inv_alpha = 1.1239 + 1.1328 / (_u._ptrd.b - 3.4);
            _u._ptrd.v_r = 0.9277 - 3.6224 / (_u._ptrd.b - 2);
        }
    }
    
    template<class URNG>
    IntType generate(URNG& urng) const
    {
        using std::floor;
        using std::abs;
        using std::log;

        while(true) {
            RealType u;
            RealType v = uniform_01<RealType>()(urng);
            if(v <= 0.86 * _u._ptrd.v_r) {
                u = v / _u._ptrd.v_r - 0.43;
                return static_cast<IntType>(floor(
                    (2*_u._ptrd.a/(0.5-abs(u)) + _u._ptrd.b)*u + _mean + 0.445));
            }

            if(v >= _u._ptrd.v_r) {
                u = uniform_01<RealType>()(urng) - 0.5;
            } else {
                u = v/_u._ptrd.v_r - 0.93;
                u = ((u < 0)? -0.5 : 0.5) - u;
                v = uniform_01<RealType>()(urng) * _u._ptrd.v_r;
            }

            RealType us = 0.5 - abs(u);
            if(us < 0.013 && v > us) {
                continue;
            }

            RealType k = floor((2*_u._ptrd.a/us + _u._ptrd.b)*u+_mean+0.445);
            v = v*_u._ptrd.inv_alpha/(_u._ptrd.a/(us*us) + _u._ptrd.b);

            RealType log_sqrt_2pi = 0.91893853320467267;

            if(k >= 10) {
                if(log(v*_u._ptrd.smu) <= (k + 0.5)*log(_mean/k)
                               - _mean
                               - log_sqrt_2pi
                               + k
                               - (1/12. - (1/360. - 1/(1260.*k*k))/(k*k))/k) {
                    return static_cast<IntType>(k);
                }
            } else if(k >= 0) {
                if(log(v) <= k*log(_mean)
                           - _mean
                           - log_factorial(static_cast<IntType>(k))) {
                    return static_cast<IntType>(k);
                }
            }
        }
    }

    template<class URNG>
    IntType invert(URNG& urng) const
    {
        RealType p = _u._exp_mean;
        IntType x = 0;
        RealType u = uniform_01<RealType>()(urng);
        while(u > p) {
            u = u - p;
            ++x;
            p = _mean * p / x;
        }
        return x;
    }

    RealType _mean;

    union {
        // for ptrd
        struct {
            RealType v_r;
            RealType a;
            RealType b;
            RealType smu;
            RealType inv_alpha;
        } _ptrd;
        // for inversion
        RealType _exp_mean;
    } _u;

    /// @endcond
};

} // namespace random

using random::poisson_distribution;

} // namespace boost

#include <boost/random/detail/enable_warnings.hpp>

#endif // BOOST_RANDOM_POISSON_DISTRIBUTION_HPP
