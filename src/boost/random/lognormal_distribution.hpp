/* boost random/lognormal_distribution.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_LOGNORMAL_DISTRIBUTION_HPP
#define BOOST_RANDOM_LOGNORMAL_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>      // std::exp, std::sqrt
#include <cassert>
#include <iosfwd>
#include <istream>
#include <boost/limits.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/normal_distribution.hpp>

namespace boost {
namespace random {

/**
 * Instantiations of class template lognormal_distribution model a
 * \random_distribution. Such a distribution produces random numbers
 * with \f$\displaystyle p(x) = \frac{1}{x s \sqrt{2\pi}} e^{\frac{-\left(\log(x)-m\right)^2}{2s^2}}\f$
 * for x > 0.
 *
 * @xmlwarning
 * This distribution has been updated to match the C++ standard.
 * Its behavior has changed from the original
 * boost::lognormal_distribution.  A backwards compatible
 * version is provided in namespace boost.
 * @endxmlwarning
 */
template<class RealType = double>
class lognormal_distribution
{
public:
    typedef typename normal_distribution<RealType>::input_type input_type;
    typedef RealType result_type;

    class param_type
    {
    public:

        typedef lognormal_distribution distribution_type;

        /** Constructs the parameters of a lognormal_distribution. */
        explicit param_type(RealType m_arg = RealType(0.0),
                            RealType s_arg = RealType(1.0))
          : _m(m_arg), _s(s_arg) {}

        /** Returns the "m" parameter of the distribution. */
        RealType m() const { return _m; }

        /** Returns the "s" parameter of the distribution. */
        RealType s() const { return _s; }

        /** Writes the parameters to a std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        {
            os << parm._m << " " << parm._s;
            return os;
        }

        /** Reads the parameters from a std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        {
            is >> parm._m >> std::ws >> parm._s;
            return is;
        }

        /** Returns true if the two sets of parameters are equal. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._m == rhs._m && lhs._s == rhs._s; }

        /** Returns true if the two sets of parameters are different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        RealType _m;
        RealType _s;
    };

    /**
     * Constructs a lognormal_distribution. @c m and @c s are the
     * parameters of the distribution.
     */
    explicit lognormal_distribution(RealType m_arg = RealType(0.0),
                                    RealType s_arg = RealType(1.0))
      : _normal(m_arg, s_arg) {}

    /**
     * Constructs a lognormal_distribution from its parameters.
     */
    explicit lognormal_distribution(const param_type& parm)
      : _normal(parm.m(), parm.s()) {}

    // compiler-generated copy ctor and assignment operator are fine

    /** Returns the m parameter of the distribution. */
    RealType m() const { return _normal.mean(); }
    /** Returns the s parameter of the distribution. */
    RealType s() const { return _normal.sigma(); }

    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return RealType(0); }
    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return (std::numeric_limits<RealType>::infinity)(); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(m(), s()); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        typedef normal_distribution<RealType> normal_type;
        typename normal_type::param_type normal_param(parm.m(), parm.s());
        _normal.param(normal_param);
    }
    
    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { _normal.reset(); }

    /**
     * Returns a random variate distributed according to the
     * lognormal distribution.
     */
    template<class Engine>
    result_type operator()(Engine& eng)
    {
        using std::exp;
        return exp(_normal(eng));
    }

    /**
     * Returns a random variate distributed according to the
     * lognormal distribution with parameters specified by param.
     */
    template<class Engine>
    result_type operator()(Engine& eng, const param_type& parm)
    { return lognormal_distribution(parm)(eng); }

    /** Writes the distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, lognormal_distribution, ld)
    {
        os << ld._normal;
        return os;
    }

    /** Reads the distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, lognormal_distribution, ld)
    {
        is >> ld._normal;
        return is;
    }

    /**
     * Returns true if the two distributions will produce identical
     * sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(lognormal_distribution, lhs, rhs)
    { return lhs._normal == rhs._normal; }

    /**
     * Returns true if the two distributions may produce different
     * sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(lognormal_distribution)

private:
    normal_distribution<result_type> _normal;
};

} // namespace random

/// \cond show_deprecated

/**
 * Provided for backwards compatibility.  This class is
 * deprecated.  It provides the old behavior of lognormal_distribution with
 * \f$\displaystyle p(x) = \frac{1}{x \sigma_N \sqrt{2\pi}} e^{\frac{-\left(\log(x)-\mu_N\right)^2}{2\sigma_N^2}}\f$
 * for x > 0, where \f$\displaystyle \mu_N = \log\left(\frac{\mu^2}{\sqrt{\sigma^2 + \mu^2}}\right)\f$ and
 * \f$\displaystyle \sigma_N = \sqrt{\log\left(1 + \frac{\sigma^2}{\mu^2}\right)}\f$.
 */
template<class RealType = double>
class lognormal_distribution
{
public:
    typedef typename normal_distribution<RealType>::input_type input_type;
    typedef RealType result_type;

    lognormal_distribution(RealType mean_arg = RealType(1.0),
                           RealType sigma_arg = RealType(1.0))
      : _mean(mean_arg), _sigma(sigma_arg)
    {
        init();
    }
    RealType mean() const { return _mean; }
    RealType sigma() const { return _sigma; }
    void reset() { _normal.reset(); }
    template<class Engine>
    RealType operator()(Engine& eng)
    {
        using std::exp;
        return exp(_normal(eng) * _nsigma + _nmean);
    }
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, lognormal_distribution, ld)
    {
        os << ld._normal << " " << ld._mean << " " << ld._sigma;
        return os;
    }
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, lognormal_distribution, ld)
    {
        is >> ld._normal >> std::ws >> ld._mean >> std::ws >> ld._sigma;
        ld.init();
        return is;
    }
private:
    /// \cond show_private
    void init()
    {
        using std::log;
        using std::sqrt;
        _nmean = log(_mean*_mean/sqrt(_sigma*_sigma + _mean*_mean));
        _nsigma = sqrt(log(_sigma*_sigma/_mean/_mean+result_type(1)));
    }
    RealType _mean;
    RealType _sigma;
    RealType _nmean;
    RealType _nsigma;
    normal_distribution<RealType> _normal;
    /// \endcond
};

/// \endcond

} // namespace boost

#endif // BOOST_RANDOM_LOGNORMAL_DISTRIBUTION_HPP
