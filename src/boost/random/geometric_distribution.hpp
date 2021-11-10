/* boost random/geometric_distribution.hpp header file
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

#ifndef BOOST_RANDOM_GEOMETRIC_DISTRIBUTION_HPP
#define BOOST_RANDOM_GEOMETRIC_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>          // std::log
#include <iosfwd>
#include <ios>
#include <boost/assert.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/uniform_01.hpp>

namespace boost {
namespace random {

/**
 * An instantiation of the class template @c geometric_distribution models
 * a \random_distribution.  The distribution produces positive
 * integers which are the number of bernoulli trials
 * with probability @c p required to get one that fails.
 *
 * For the geometric distribution, \f$p(i) = p(1-p)^{i}\f$.
 *
 * @xmlwarning
 * This distribution has been updated to match the C++ standard.
 * Its behavior has changed from the original
 * boost::geometric_distribution.  A backwards compatible
 * wrapper is provided in namespace boost.
 * @endxmlwarning
 */
template<class IntType = int, class RealType = double>
class geometric_distribution
{
public:
    typedef RealType input_type;
    typedef IntType result_type;

    class param_type
    {
    public:

        typedef geometric_distribution distribution_type;

        /** Constructs the parameters with p. */
        explicit param_type(RealType p_arg = RealType(0.5))
          : _p(p_arg)
        {
            BOOST_ASSERT(RealType(0) < _p && _p < RealType(1));
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
            double p_in;
            if(is >> p_in) {
                if(p_in > RealType(0) && p_in < RealType(1)) {
                    parm._p = p_in;
                } else {
                    is.setstate(std::ios_base::failbit);
                }
            }
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
     * Contructs a new geometric_distribution with the paramter @c p.
     *
     * Requires: 0 < p < 1
     */
    explicit geometric_distribution(const RealType& p_arg = RealType(0.5))
      : _p(p_arg)
    {
        BOOST_ASSERT(RealType(0) < _p && _p < RealType(1));
        init();
    }

    /** Constructs a new geometric_distribution from its parameters. */
    explicit geometric_distribution(const param_type& parm)
      : _p(parm.p())
    {
        init();
    }

    // compiler-generated copy ctor and assignment operator are fine

    /** Returns: the distribution parameter @c p  */
    RealType p() const { return _p; }

    /** Returns the smallest value that the distribution can produce. */
    IntType min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return IntType(0); }

    /** Returns the largest value that the distribution can produce. */
    IntType max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return (std::numeric_limits<IntType>::max)(); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_p); }

    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        _p = parm.p();
        init();
    }
  
    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /**
     * Returns a random variate distributed according to the
     * geometric_distribution.
     */
    template<class Engine>
    result_type operator()(Engine& eng) const
    {
        using std::log;
        using std::floor;
        RealType x = RealType(1) - boost::uniform_01<RealType>()(eng);
        return IntType(floor(log(x) / _log_1mp));
    }

    /**
     * Returns a random variate distributed according to the
     * geometric distribution with parameters specified by param.
     */
    template<class Engine>
    result_type operator()(Engine& eng, const param_type& parm) const
    { return geometric_distribution(parm)(eng); }

    /** Writes the distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, geometric_distribution, gd)
    {
        os << gd._p;
        return os;
    }

    /** Reads the distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, geometric_distribution, gd)
    {
        param_type parm;
        if(is >> parm) {
            gd.param(parm);
        }
        return is;
    }

    /**
     * Returns true if the two distributions will produce identical
     * sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(geometric_distribution, lhs, rhs)
    { return lhs._p == rhs._p; }

    /**
     * Returns true if the two distributions may produce different
     * sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(geometric_distribution)

private:

    /// \cond show_private

    void init()
    {
        using std::log;
        _log_1mp = log(1 - _p);
    }

    RealType _p;
    RealType _log_1mp;

    /// \endcond
};

} // namespace random

/// \cond show_deprecated

/**
 * Provided for backwards compatibility.  This class is
 * deprecated.  It provides the old behavior of geometric_distribution
 * with \f$p(i) = (1-p) p^{i-1}\f$.
 */
template<class IntType = int, class RealType = double>
class geometric_distribution
{
public:
    typedef RealType input_type;
    typedef IntType result_type;

    explicit geometric_distribution(RealType p_arg = RealType(0.5))
      : _impl(1 - p_arg) {}

    RealType p() const { return 1 - _impl.p(); }

    void reset() {}

    template<class Engine>
    IntType operator()(Engine& eng) const { return _impl(eng) + IntType(1); }

    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, geometric_distribution, gd)
    {
        os << gd.p();
        return os;
    }

    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, geometric_distribution, gd)
    {
        RealType val;
        if(is >> val) {
            typename impl_type::param_type impl_param(1 - val);
            gd._impl.param(impl_param);
        }
        return is;
    }

private:
    typedef random::geometric_distribution<IntType, RealType> impl_type;
    impl_type _impl;
};

/// \endcond

} // namespace boost

#endif // BOOST_RANDOM_GEOMETRIC_DISTRIBUTION_HPP
