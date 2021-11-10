/* boost random/triangle_distribution.hpp header file
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

#ifndef BOOST_RANDOM_TRIANGLE_DISTRIBUTION_HPP
#define BOOST_RANDOM_TRIANGLE_DISTRIBUTION_HPP

#include <boost/config/no_tr1/cmath.hpp>
#include <iosfwd>
#include <ios>
#include <istream>
#include <boost/assert.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/uniform_01.hpp>

namespace boost {
namespace random {

/**
 * Instantiations of @c triangle_distribution model a \random_distribution.
 * A @c triangle_distribution has three parameters, @c a, @c b, and @c c,
 * which are the smallest, the most probable and the largest values of
 * the distribution respectively.
 */
template<class RealType = double>
class triangle_distribution
{
public:
    typedef RealType input_type;
    typedef RealType result_type;

    class param_type
    {
    public:

        typedef triangle_distribution distribution_type;

        /** Constructs the parameters of a @c triangle_distribution. */
        explicit param_type(RealType a_arg = RealType(0.0),
                            RealType b_arg = RealType(0.5),
                            RealType c_arg = RealType(1.0))
          : _a(a_arg), _b(b_arg), _c(c_arg)
        {
            BOOST_ASSERT(_a <= _b && _b <= _c);
        }

        /** Returns the minimum value of the distribution. */
        RealType a() const { return _a; }
        /** Returns the mode of the distribution. */
        RealType b() const { return _b; }
        /** Returns the maximum value of the distribution. */
        RealType c() const { return _c; }

        /** Writes the parameters to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        {
            os << parm._a << " " << parm._b << " " << parm._c;
            return os;
        }

        /** Reads the parameters from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        {
            double a_in, b_in, c_in;
            if(is >> a_in >> std::ws >> b_in >> std::ws >> c_in) {
                if(a_in <= b_in && b_in <= c_in) {
                    parm._a = a_in;
                    parm._b = b_in;
                    parm._c = c_in;
                } else {
                    is.setstate(std::ios_base::failbit);
                }
            }
            return is;
        }

        /** Returns true if the two sets of parameters are equal. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._a == rhs._a && lhs._b == rhs._b && lhs._c == rhs._c; }

        /** Returns true if the two sets of parameters are different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        RealType _a;
        RealType _b;
        RealType _c;
    };

    /**
     * Constructs a @c triangle_distribution with the parameters
     * @c a, @c b, and @c c.
     *
     * Preconditions: a <= b <= c.
     */
    explicit triangle_distribution(RealType a_arg = RealType(0.0),
                                   RealType b_arg = RealType(0.5),
                                   RealType c_arg = RealType(1.0))
      : _a(a_arg), _b(b_arg), _c(c_arg)
    {
        BOOST_ASSERT(_a <= _b && _b <= _c);
        init();
    }

    /** Constructs a @c triangle_distribution from its parameters. */
    explicit triangle_distribution(const param_type& parm)
      : _a(parm.a()), _b(parm.b()), _c(parm.c())
    {
        init();
    }

    // compiler-generated copy ctor and assignment operator are fine

    /** Returns the @c a parameter of the distribution */
    result_type a() const { return _a; }
    /** Returns the @c b parameter of the distribution */
    result_type b() const { return _b; }
    /** Returns the @c c parameter of the distribution */
    result_type c() const { return _c; }

    /** Returns the smallest value that the distribution can produce. */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _a; }
    /** Returns the largest value that the distribution can produce. */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _c; }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_a, _b, _c); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        _a = parm.a();
        _b = parm.b();
        _c = parm.c();
        init();
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /**
     * Returns a random variate distributed according to the
     * triangle distribution.
     */
    template<class Engine>
    result_type operator()(Engine& eng)
    {
        using std::sqrt;
        result_type u = uniform_01<result_type>()(eng);
        if( u <= q1 )
            return _a + p1*sqrt(u);
        else
        return _c - d3*sqrt(d2*u-d1);
    }

    /**
     * Returns a random variate distributed according to the
     * triangle distribution with parameters specified by param.
     */
    template<class Engine>
    result_type operator()(Engine& eng, const param_type& parm)
    { return triangle_distribution(parm)(eng); }

    /** Writes the distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, triangle_distribution, td)
    {
        os << td.param();
        return os;
    }

    /** Reads the distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, triangle_distribution, td)
    {
        param_type parm;
        if(is >> parm) {
            td.param(parm);
        }
        return is;
    }

    /**
     * Returns true if the two distributions will produce identical
     * sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(triangle_distribution, lhs, rhs)
    { return lhs._a == rhs._a && lhs._b == rhs._b && lhs._c == rhs._c; }

    /**
     * Returns true if the two distributions may produce different
     * sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(triangle_distribution)

private:
    /// \cond show_private
    void init()
    {
        using std::sqrt;
        d1 = _b - _a;
        d2 = _c - _a;
        d3 = sqrt(_c - _b);
        q1 = d1 / d2;
        p1 = sqrt(d1 * d2);
    }
    /// \endcond

    RealType _a, _b, _c;
    RealType d1, d2, d3, q1, p1;
};

} // namespace random

using random::triangle_distribution;

} // namespace boost

#endif // BOOST_RANDOM_TRIANGLE_DISTRIBUTION_HPP
