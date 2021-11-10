/* boost random/cauchy_distribution.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
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

#ifndef BOOST_RANDOM_CAUCHY_DISTRIBUTION_HPP
#define BOOST_RANDOM_CAUCHY_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <iosfwd>
#include <istream>
#include <boost/limits.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/uniform_01.hpp>

namespace boost {
namespace random {

// Cauchy distribution: 

/**
 * The cauchy distribution is a continuous distribution with two
 * parameters, median and sigma.
 *
 * It has \f$\displaystyle p(x) = \frac{\sigma}{\pi(\sigma^2 + (x-m)^2)}\f$
 */
template<class RealType = double>
class cauchy_distribution
{
public:
    typedef RealType input_type;
    typedef RealType result_type;

    class param_type
    {
    public:

        typedef cauchy_distribution distribution_type;

        /** Constructs the parameters of the cauchy distribution. */
        explicit param_type(RealType median_arg = RealType(0.0),
                            RealType sigma_arg = RealType(1.0))
          : _median(median_arg), _sigma(sigma_arg) {}

        // backwards compatibility for Boost.Random

        /** Returns the median of the distribution. */
        RealType median() const { return _median; }
        /** Returns the sigma parameter of the distribution. */
        RealType sigma() const { return _sigma; }

        // The new names in C++0x.

        /** Returns the median of the distribution. */
        RealType a() const { return _median; }
        /** Returns the sigma parameter of the distribution. */
        RealType b() const { return _sigma; }

        /** Writes the parameters to a std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        {
            os << parm._median << " " << parm._sigma;
            return os;
        }

        /** Reads the parameters from a std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        {
            is >> parm._median >> std::ws >> parm._sigma;
            return is;
        }

        /** Returns true if the two sets of parameters are equal. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._median == rhs._median && lhs._sigma == rhs._sigma; }

        /** Returns true if the two sets of parameters are different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        RealType _median;
        RealType _sigma;
    };

    /**
     * Constructs a \cauchy_distribution with the paramters @c median
     * and @c sigma.
     */
    explicit cauchy_distribution(RealType median_arg = RealType(0.0), 
                                 RealType sigma_arg = RealType(1.0))
      : _median(median_arg), _sigma(sigma_arg) { }
    
    /**
     * Constructs a \cauchy_distribution from it's parameters.
     */
    explicit cauchy_distribution(const param_type& parm)
      : _median(parm.median()), _sigma(parm.sigma()) { }

    // compiler-generated copy ctor and assignment operator are fine

    // backwards compatibility for Boost.Random

    /** Returns: the "median" parameter of the distribution */
    RealType median() const { return _median; }
    /** Returns: the "sigma" parameter of the distribution */
    RealType sigma() const { return _sigma; }
    
    // The new names in C++0x

    /** Returns: the "median" parameter of the distribution */
    RealType a() const { return _median; }
    /** Returns: the "sigma" parameter of the distribution */
    RealType b() const { return _sigma; }

    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return -(std::numeric_limits<RealType>::infinity)(); }

    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return (std::numeric_limits<RealType>::infinity)(); }

    param_type param() const { return param_type(_median, _sigma); }

    void param(const param_type& parm)
    {
        _median = parm.median();
        _sigma = parm.sigma();
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /**
     * Returns: A random variate distributed according to the
     * cauchy distribution.
     */
    template<class Engine>
    result_type operator()(Engine& eng)
    {
        // Can we have a boost::mathconst please?
        const result_type pi = result_type(3.14159265358979323846);
        using std::tan;
        RealType val = uniform_01<RealType>()(eng)-result_type(0.5);
        return _median + _sigma * tan(pi*val);
    }

    /**
     * Returns: A random variate distributed according to the
     * cauchy distribution with parameters specified by param.
     */
    template<class Engine>
    result_type operator()(Engine& eng, const param_type& parm)
    {
        return cauchy_distribution(parm)(eng);
    }

    /**
     * Writes the distribution to a @c std::ostream.
     */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, cauchy_distribution, cd)
    {
        os << cd._median << " " << cd._sigma;
        return os;
    }

    /**
     * Reads the distribution from a @c std::istream.
     */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, cauchy_distribution, cd)
    {
        is >> cd._median >> std::ws >> cd._sigma;
        return is;
    }

    /**
     * Returns true if the two distributions will produce
     * identical sequences of values, given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(cauchy_distribution, lhs, rhs)
    { return lhs._median == rhs._median && lhs._sigma == rhs._sigma; }

    /**
     * Returns true if the two distributions may produce
     * different sequences of values, given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(cauchy_distribution)

private:
    RealType _median;
    RealType _sigma;
};

} // namespace random

using random::cauchy_distribution;

} // namespace boost

#endif // BOOST_RANDOM_CAUCHY_DISTRIBUTION_HPP
