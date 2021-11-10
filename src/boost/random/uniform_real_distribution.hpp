/* boost random/uniform_real_distribution.hpp header file
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
 */

#ifndef BOOST_RANDOM_UNIFORM_REAL_DISTRIBUTION_HPP
#define BOOST_RANDOM_UNIFORM_REAL_DISTRIBUTION_HPP

#include <iosfwd>
#include <ios>
#include <istream>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/signed_unsigned_tools.hpp>
#include <boost/type_traits/is_integral.hpp>

namespace boost {
namespace random {
namespace detail {

template<class Engine, class T>
T generate_uniform_real(
    Engine& eng, T min_value, T max_value,
    boost::false_type  /** is_integral<Engine::result_type> */)
{
    for(;;) {
        typedef T result_type;
        result_type numerator = static_cast<T>(eng() - (eng.min)());
        result_type divisor = static_cast<T>((eng.max)() - (eng.min)());
        BOOST_ASSERT(divisor > 0);
        BOOST_ASSERT(numerator >= 0 && numerator <= divisor);
        T result = numerator / divisor * (max_value - min_value) + min_value;
        if(result < max_value) return result;
    }
}

template<class Engine, class T>
T generate_uniform_real(
    Engine& eng, T min_value, T max_value,
    boost::true_type  /** is_integral<Engine::result_type> */)
{
    for(;;) {
        typedef T result_type;
        typedef typename Engine::result_type base_result;
        result_type numerator = static_cast<T>(subtract<base_result>()(eng(), (eng.min)()));
        result_type divisor = static_cast<T>(subtract<base_result>()((eng.max)(), (eng.min)())) + 1;
        BOOST_ASSERT(divisor > 0);
        BOOST_ASSERT(numerator >= 0 && numerator <= divisor);
        T result = numerator / divisor * (max_value - min_value) + min_value;
        if(result < max_value) return result;
    }
}

template<class Engine, class T>
inline T generate_uniform_real(Engine& eng, T min_value, T max_value)
{
    if(max_value / 2 - min_value / 2 > (std::numeric_limits<T>::max)() / 2)
        return 2 * generate_uniform_real(eng, T(min_value / 2), T(max_value / 2));
    typedef typename Engine::result_type base_result;
    return generate_uniform_real(eng, min_value, max_value,
        boost::is_integral<base_result>());
}

}

/**
 * The class template uniform_real_distribution models a \random_distribution.
 * On each invocation, it returns a random floating-point value uniformly
 * distributed in the range [min..max).
 */
template<class RealType = double>
class uniform_real_distribution
{
public:
    typedef RealType input_type;
    typedef RealType result_type;

    class param_type
    {
    public:

        typedef uniform_real_distribution distribution_type;

        /**
         * Constructs the parameters of a uniform_real_distribution.
         *
         * Requires min <= max
         */
        explicit param_type(RealType min_arg = RealType(0.0),
                            RealType max_arg = RealType(1.0))
          : _min(min_arg), _max(max_arg)
        {
            BOOST_ASSERT(_min < _max);
        }

        /** Returns the minimum value of the distribution. */
        RealType a() const { return _min; }
        /** Returns the maximum value of the distribution. */
        RealType b() const { return _max; }

        /** Writes the parameters to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        {
            os << parm._min << " " << parm._max;
            return os;
        }

        /** Reads the parameters from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        {
            RealType min_in, max_in;
            if(is >> min_in >> std::ws >> max_in) {
                if(min_in <= max_in) {
                    parm._min = min_in;
                    parm._max = max_in;
                } else {
                    is.setstate(std::ios_base::failbit);
                }
            }
            return is;
        }

        /** Returns true if the two sets of parameters are equal. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._min == rhs._min && lhs._max == rhs._max; }

        /** Returns true if the two sets of parameters are different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:

        RealType _min;
        RealType _max;
    };

    /**
     * Constructs a uniform_real_distribution. @c min and @c max are
     * the parameters of the distribution.
     *
     * Requires: min <= max
     */
    explicit uniform_real_distribution(
        RealType min_arg = RealType(0.0),
        RealType max_arg = RealType(1.0))
      : _min(min_arg), _max(max_arg)
    {
        BOOST_ASSERT(min_arg < max_arg);
    }
    /** Constructs a uniform_real_distribution from its parameters. */
    explicit uniform_real_distribution(const param_type& parm)
      : _min(parm.a()), _max(parm.b()) {}

    /**  Returns the minimum value of the distribution */
    RealType min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _min; }
    /**  Returns the maximum value of the distribution */
    RealType max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _max; }

    /**  Returns the minimum value of the distribution */
    RealType a() const { return _min; }
    /**  Returns the maximum value of the distribution */
    RealType b() const { return _max; }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_min, _max); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        _min = parm.a();
        _max = parm.b();
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /** Returns a value uniformly distributed in the range [min, max). */
    template<class Engine>
    result_type operator()(Engine& eng) const
    { return detail::generate_uniform_real(eng, _min, _max); }

    /**
     * Returns a value uniformly distributed in the range
     * [param.a(), param.b()).
     */
    template<class Engine>
    result_type operator()(Engine& eng, const param_type& parm) const
    { return detail::generate_uniform_real(eng, parm.a(), parm.b()); }

    /** Writes the distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, uniform_real_distribution, ud)
    {
        os << ud.param();
        return os;
    }

    /** Reads the distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, uniform_real_distribution, ud)
    {
        param_type parm;
        if(is >> parm) {
            ud.param(parm);
        }
        return is;
    }

    /**
     * Returns true if the two distributions will produce identical sequences
     * of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(uniform_real_distribution, lhs, rhs)
    { return lhs._min == rhs._min && lhs._max == rhs._max; }
    
    /**
     * Returns true if the two distributions may produce different sequences
     * of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(uniform_real_distribution)

private:
    RealType _min;
    RealType _max;
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_INT_HPP
