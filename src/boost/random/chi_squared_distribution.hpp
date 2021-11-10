/* boost random/chi_squared_distribution.hpp header file
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 */

#ifndef BOOST_RANDOM_CHI_SQUARED_DISTRIBUTION_HPP_INCLUDED
#define BOOST_RANDOM_CHI_SQUARED_DISTRIBUTION_HPP_INCLUDED

#include <iosfwd>
#include <boost/limits.hpp>

#include <boost/random/detail/config.hpp>
#include <boost/random/gamma_distribution.hpp>

namespace boost {
namespace random {

/**
 * The chi squared distribution is a real valued distribution with
 * one parameter, @c n.  The distribution produces values > 0.
 *
 * The distribution function is
 * \f$\displaystyle P(x) = \frac{x^{(n/2)-1}e^{-x/2}}{\Gamma(n/2)2^{n/2}}\f$.
 */
template<class RealType = double>
class chi_squared_distribution {
public:
    typedef RealType result_type;
    typedef RealType input_type;

    class param_type {
    public:
        typedef chi_squared_distribution distribution_type;
        /**
         * Construct a param_type object.  @c n
         * is the parameter of the distribution.
         *
         * Requires: t >=0 && 0 <= p <= 1
         */
        explicit param_type(RealType n_arg = RealType(1))
          : _n(n_arg)
        {}
        /** Returns the @c n parameter of the distribution. */
        RealType n() const { return _n; }
#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
        /** Writes the parameters of the distribution to a @c std::ostream. */
        template<class CharT, class Traits>
        friend std::basic_ostream<CharT,Traits>&
        operator<<(std::basic_ostream<CharT,Traits>& os,
                   const param_type& parm)
        {
            os << parm._n;
            return os;
        }
    
        /** Reads the parameters of the distribution from a @c std::istream. */
        template<class CharT, class Traits>
        friend std::basic_istream<CharT,Traits>&
        operator>>(std::basic_istream<CharT,Traits>& is, param_type& parm)
        {
            is >> parm._n;
            return is;
        }
#endif
        /** Returns true if the parameters have the same values. */
        friend bool operator==(const param_type& lhs, const param_type& rhs)
        {
            return lhs._n == rhs._n;
        }
        /** Returns true if the parameters have different values. */
        friend bool operator!=(const param_type& lhs, const param_type& rhs)
        {
            return !(lhs == rhs);
        }
    private:
        RealType _n;
    };
    
    /**
     * Construct a @c chi_squared_distribution object. @c n
     * is the parameter of the distribution.
     *
     * Requires: t >=0 && 0 <= p <= 1
     */
    explicit chi_squared_distribution(RealType n_arg = RealType(1))
      : _impl(static_cast<RealType>(n_arg / 2))
    {
    }
    
    /**
     * Construct an @c chi_squared_distribution object from the
     * parameters.
     */
    explicit chi_squared_distribution(const param_type& parm)
      : _impl(static_cast<RealType>(parm.n() / 2))
    {
    }
    
    /**
     * Returns a random variate distributed according to the
     * chi squared distribution.
     */
    template<class URNG>
    RealType operator()(URNG& urng)
    {
        return 2 * _impl(urng);
    }
    
    /**
     * Returns a random variate distributed according to the
     * chi squared distribution with parameters specified by @c param.
     */
    template<class URNG>
    RealType operator()(URNG& urng, const param_type& parm) const
    {
        return chi_squared_distribution(parm)(urng);
    }

    /** Returns the @c n parameter of the distribution. */
    RealType n() const { return 2 * _impl.alpha(); }

    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION() const { return 0; }
    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION() const
    { return (std::numeric_limits<RealType>::infinity)(); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(n()); }
    /** Sets parameters of the distribution. */
    void param(const param_type& parm)
    {
        typedef gamma_distribution<RealType> impl_type;
        typename impl_type::param_type impl_parm(static_cast<RealType>(parm.n() / 2));
        _impl.param(impl_parm);
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { _impl.reset(); }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
    /** Writes the parameters of the distribution to a @c std::ostream. */
    template<class CharT, class Traits>
    friend std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& os,
               const chi_squared_distribution& c2d)
    {
        os << c2d.param();
        return os;
    }
    
    /** Reads the parameters of the distribution from a @c std::istream. */
    template<class CharT, class Traits>
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& is,
               chi_squared_distribution& c2d)
    {
        c2d.read(is);
        return is;
    }
#endif

    /** Returns true if the two distributions will produce the same
        sequence of values, given equal generators. */
    friend bool operator==(const chi_squared_distribution& lhs,
                           const chi_squared_distribution& rhs)
    {
        return lhs._impl == rhs._impl;
    }
    /** Returns true if the two distributions could produce different
        sequences of values, given equal generators. */
    friend bool operator!=(const chi_squared_distribution& lhs,
                           const chi_squared_distribution& rhs)
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

    gamma_distribution<RealType> _impl;

    /// @endcond
};

}

}

#endif
