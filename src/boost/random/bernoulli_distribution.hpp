/* boost random/bernoulli_distribution.hpp header file
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

#ifndef BOOST_RANDOM_BERNOULLI_DISTRIBUTION_HPP
#define BOOST_RANDOM_BERNOULLI_DISTRIBUTION_HPP

#include <iosfwd>
#include <boost/assert.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>

namespace boost {
namespace random {

/**
 * Instantiations of class template \bernoulli_distribution model a
 * \random_distribution. Such a random distribution produces bool values
 * distributed with probabilities P(true) = p and P(false) = 1-p. p is
 * the parameter of the distribution.
 */
template<class RealType = double>
class bernoulli_distribution
{
public:
    // In principle, this could work with both integer and floating-point
    // types.  Generating floating-point random numbers in the first
    // place is probably more expensive, so use integer as input.
    typedef int input_type;
    typedef bool result_type;

    class param_type
    {
    public:

        typedef bernoulli_distribution distribution_type;

        /** 
         * Constructs the parameters of the distribution.
         *
         * Requires: 0 <= p <= 1
         */
        explicit param_type(RealType p_arg = RealType(0.5))
          : _p(p_arg)
        {
            BOOST_ASSERT(_p >= 0);
            BOOST_ASSERT(_p <= 1);
        }

        /** Returns the p parameter of the distribution. */
        RealType p() const { return _p; }

        /** Writes the parameters to a std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        {
            os << parm._p;
            return os;
        }

        /** Reads the parameters from a std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        {
            is >> parm._p;
            return is;
        }

        /** Returns true if the two sets of parameters are equal. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._p == rhs._p; }

        /** Returns true if the two sets of parameters are different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        RealType _p;
    };

    /** 
     * Constructs a \bernoulli_distribution object.
     * p is the parameter of the distribution.
     *
     * Requires: 0 <= p <= 1
     */
    explicit bernoulli_distribution(const RealType& p_arg = RealType(0.5)) 
      : _p(p_arg)
    {
        BOOST_ASSERT(_p >= 0);
        BOOST_ASSERT(_p <= 1);
    }
    /**
     * Constructs \bernoulli_distribution from its parameters
     */
    explicit bernoulli_distribution(const param_type& parm)
      : _p(parm.p()) {}

    // compiler-generated copy ctor and assignment operator are fine

    /**
     * Returns: The "p" parameter of the distribution.
     */
    RealType p() const { return _p; }

    /** Returns the smallest value that the distribution can produce. */
    bool min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return false; }
    /** Returns the largest value that the distribution can produce. */
    bool max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return true; }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_p); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm) { _p = parm.p(); }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /**
     * Returns: a random variate distributed according to the
     * \bernoulli_distribution.
     */
    template<class Engine>
    bool operator()(Engine& eng) const
    {
        if(_p == RealType(0))
            return false;
        else
            return RealType(eng() - (eng.min)()) <= _p * RealType((eng.max)()-(eng.min)());
    }

    /**
     * Returns: a random variate distributed according to the
     * \bernoulli_distribution with parameters specified by param.
     */
    template<class Engine>
    bool operator()(Engine& eng, const param_type& parm) const
    {
        return bernoulli_distribution(parm)(eng);
    }

    /**
     * Writes the parameters of the distribution to a @c std::ostream.
     */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, bernoulli_distribution, bd)
    {
        os << bd._p;
        return os;
    }

    /**
     * Reads the parameters of the distribution from a @c std::istream.
     */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, bernoulli_distribution, bd)
    {
        is >> bd._p;
        return is;
    }

    /**
     * Returns true iff the two distributions will produce identical
     * sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(bernoulli_distribution, lhs, rhs)
    { return lhs._p == rhs._p; }
    
    /**
     * Returns true iff the two distributions will produce different
     * sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(bernoulli_distribution)

private:
    RealType _p;
};

} // namespace random

using random::bernoulli_distribution;

} // namespace boost

#endif // BOOST_RANDOM_BERNOULLI_DISTRIBUTION_HPP
