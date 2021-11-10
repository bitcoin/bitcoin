/* boost random/fisher_f_distribution.hpp header file
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

#ifndef BOOST_RANDOM_FISHER_F_DISTRIBUTION_HPP
#define BOOST_RANDOM_FISHER_F_DISTRIBUTION_HPP

#include <iosfwd>
#include <istream>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/chi_squared_distribution.hpp>

namespace boost {
namespace random {

/**
 * The Fisher F distribution is a real valued distribution with two
 * parameters m and n.
 *
 * It has \f$\displaystyle p(x) =
 *   \frac{\Gamma((m+n)/2)}{\Gamma(m/2)\Gamma(n/2)}
 *   \left(\frac{m}{n}\right)^{m/2}
 *   x^{(m/2)-1} \left(1+\frac{mx}{n}\right)^{-(m+n)/2}
 * \f$.
 */
template<class RealType = double>
class fisher_f_distribution {
public:
    typedef RealType result_type;
    typedef RealType input_type;

    class param_type {
    public:
        typedef fisher_f_distribution distribution_type;

        /**
         * Constructs a @c param_type from the "m" and "n" parameters
         * of the distribution.
         *
         * Requires: m > 0 and n > 0
         */
        explicit param_type(RealType m_arg = RealType(1.0),
                            RealType n_arg = RealType(1.0))
          : _m(m_arg), _n(n_arg)
        {}

        /** Returns the "m" parameter of the distribtuion. */
        RealType m() const { return _m; }
        /** Returns the "n" parameter of the distribution. */
        RealType n() const { return _n; }

        /** Writes a @c param_type to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        { os << parm._m << ' ' << parm._n; return os; }

        /** Reads a @c param_type from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        { is >> parm._m >> std::ws >> parm._n; return is; }

        /** Returns true if the two sets of parameters are the same. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._m == rhs._m && lhs._n == rhs._n; }
        
        /** Returns true if the two sets of parameters are the different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        RealType _m;
        RealType _n;
    };

    /**
     * Constructs a @c fisher_f_distribution from its "m" and "n" parameters.
     *
     * Requires: m > 0 and n > 0
     */
    explicit fisher_f_distribution(RealType m_arg = RealType(1.0),
                                   RealType n_arg = RealType(1.0))
      : _impl_m(m_arg), _impl_n(n_arg)
    {}
    /** Constructs an @c fisher_f_distribution from its parameters. */
    explicit fisher_f_distribution(const param_type& parm)
      : _impl_m(parm.m()), _impl_n(parm.n())
    {}

    /**
     * Returns a random variate distributed according to the
     * F distribution.
     */
    template<class URNG>
    RealType operator()(URNG& urng)
    {
        return (_impl_m(urng) * n()) / (_impl_n(urng) * m());
    }

    /**
     * Returns a random variate distributed according to the
     * F distribution with parameters specified by @c param.
     */
    template<class URNG>
    RealType operator()(URNG& urng, const param_type& parm) const
    {
        return fisher_f_distribution(parm)(urng);
    }

    /** Returns the "m" parameter of the distribution. */
    RealType m() const { return _impl_m.n(); }
    /** Returns the "n" parameter of the distribution. */
    RealType n() const { return _impl_n.n(); }

    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return 0; }
    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return std::numeric_limits<RealType>::infinity(); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(m(), n()); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        typedef chi_squared_distribution<RealType> impl_type;
        typename impl_type::param_type m_param(parm.m());
        _impl_m.param(m_param);
        typename impl_type::param_type n_param(parm.n());
        _impl_n.param(n_param);
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /** Writes an @c fisher_f_distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, fisher_f_distribution, fd)
    {
        os << fd.param();
        return os;
    }

    /** Reads an @c fisher_f_distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, fisher_f_distribution, fd)
    {
        param_type parm;
        if(is >> parm) {
            fd.param(parm);
        }
        return is;
    }

    /**
     * Returns true if the two instances of @c fisher_f_distribution will
     * return identical sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(fisher_f_distribution, lhs, rhs)
    { return lhs._impl_m == rhs._impl_m && lhs._impl_n == rhs._impl_n; }
    
    /**
     * Returns true if the two instances of @c fisher_f_distribution will
     * return different sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(fisher_f_distribution)

private:
    chi_squared_distribution<RealType> _impl_m;
    chi_squared_distribution<RealType> _impl_n;
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_EXTREME_VALUE_DISTRIBUTION_HPP
