/* boost random/student_t_distribution.hpp header file
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

#ifndef BOOST_RANDOM_STUDENT_T_DISTRIBUTION_HPP
#define BOOST_RANDOM_STUDENT_T_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <iosfwd>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/chi_squared_distribution.hpp>
#include <boost/random/normal_distribution.hpp>

namespace boost {
namespace random {

/**
 * The Student t distribution is a real valued distribution with one
 * parameter n, the number of degrees of freedom.
 *
 * It has \f$\displaystyle p(x) =
 *   \frac{1}{\sqrt{n\pi}}
 *   \frac{\Gamma((n+1)/2)}{\Gamma(n/2)}
 *   \left(1+\frac{x^2}{n}\right)^{-(n+1)/2}
 * \f$.
 */
template<class RealType = double>
class student_t_distribution {
public:
    typedef RealType result_type;
    typedef RealType input_type;

    class param_type {
    public:
        typedef student_t_distribution distribution_type;

        /**
         * Constructs a @c param_type with "n" degrees of freedom.
         *
         * Requires: n > 0
         */
        explicit param_type(RealType n_arg = RealType(1.0))
          : _n(n_arg)
        {}

        /** Returns the number of degrees of freedom of the distribution. */
        RealType n() const { return _n; }

        /** Writes a @c param_type to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        { os << parm._n; return os; }

        /** Reads a @c param_type from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        { is >> parm._n; return is; }

        /** Returns true if the two sets of parameters are the same. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._n == rhs._n; }
        
        /** Returns true if the two sets of parameters are the different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        RealType _n;
    };

    /**
     * Constructs an @c student_t_distribution with "n" degrees of freedom.
     *
     * Requires: n > 0
     */
    explicit student_t_distribution(RealType n_arg = RealType(1.0))
      : _normal(), _chi_squared(n_arg)
    {}
    /** Constructs an @c student_t_distribution from its parameters. */
    explicit student_t_distribution(const param_type& parm)
      : _normal(), _chi_squared(parm.n())
    {}

    /**
     * Returns a random variate distributed according to the
     * Student t distribution.
     */
    template<class URNG>
    RealType operator()(URNG& urng)
    {
        using std::sqrt;
        return _normal(urng) / sqrt(_chi_squared(urng) / n());
    }

    /**
     * Returns a random variate distributed accordint to the Student
     * t distribution with parameters specified by @c param.
     */
    template<class URNG>
    RealType operator()(URNG& urng, const param_type& parm) const
    {
        return student_t_distribution(parm)(urng);
    }

    /** Returns the number of degrees of freedom. */
    RealType n() const { return _chi_squared.n(); }

    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return -std::numeric_limits<RealType>::infinity(); }
    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return std::numeric_limits<RealType>::infinity(); }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(n()); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        typedef chi_squared_distribution<RealType> chi_squared_type;
        typename chi_squared_type::param_type chi_squared_param(parm.n());
        _chi_squared.param(chi_squared_param);
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset()
    {
        _normal.reset();
        _chi_squared.reset();
    }

    /** Writes a @c student_t_distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, student_t_distribution, td)
    {
        os << td.param();
        return os;
    }

    /** Reads a @c student_t_distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, student_t_distribution, td)
    {
        param_type parm;
        if(is >> parm) {
            td.param(parm);
        }
        return is;
    }

    /**
     * Returns true if the two instances of @c student_t_distribution will
     * return identical sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(student_t_distribution, lhs, rhs)
    { return lhs._normal == rhs._normal && lhs._chi_squared == rhs._chi_squared; }
    
    /**
     * Returns true if the two instances of @c student_t_distribution will
     * return different sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(student_t_distribution)

private:
    normal_distribution<RealType> _normal;
    chi_squared_distribution<RealType> _chi_squared;
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_STUDENT_T_DISTRIBUTION_HPP
