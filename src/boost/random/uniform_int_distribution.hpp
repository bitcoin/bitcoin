/* boost random/uniform_int_distribution.hpp header file
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
 *  2001-04-08  added min<max assertion (N. Becker)
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_UNIFORM_INT_DISTRIBUTION_HPP
#define BOOST_RANDOM_UNIFORM_INT_DISTRIBUTION_HPP

#include <iosfwd>
#include <ios>
#include <istream>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/assert.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/uniform_int_float.hpp>
#include <boost/random/detail/signed_unsigned_tools.hpp>
#include <boost/random/traits.hpp>
#include <boost/type_traits/integral_constant.hpp>
#ifdef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
#include <boost/type_traits/conditional.hpp>
#endif

namespace boost {
namespace random {
namespace detail {
    

#ifdef BOOST_MSVC
#pragma warning(push)
// disable division by zero warning, since we can't
// actually divide by zero.
#pragma warning(disable:4723)
#endif

template<class Engine, class T>
T generate_uniform_int(
    Engine& eng, T min_value, T max_value,
    boost::true_type /** is_integral<Engine::result_type> */)
{
    typedef T result_type;
    typedef typename boost::random::traits::make_unsigned_or_unbounded<T>::type range_type;
    typedef typename Engine::result_type base_result;
    // ranges are always unsigned or unbounded
    typedef typename boost::random::traits::make_unsigned_or_unbounded<base_result>::type base_unsigned;
    const range_type range = random::detail::subtract<result_type>()(max_value, min_value);
    const base_result bmin = (eng.min)();
    const base_unsigned brange =
      random::detail::subtract<base_result>()((eng.max)(), (eng.min)());

    if(range == 0) {
      return min_value;    
    } else if(brange == range) {
      // this will probably never happen in real life
      // basically nothing to do; just take care we don't overflow / underflow
      base_unsigned v = random::detail::subtract<base_result>()(eng(), bmin);
      return random::detail::add<base_unsigned, result_type>()(v, min_value);
    } else if(brange < range) {
      // use rejection method to handle things like 0..3 --> 0..4
      for(;;) {
        // concatenate several invocations of the base RNG
        // take extra care to avoid overflows

        //  limit == floor((range+1)/(brange+1))
        //  Therefore limit*(brange+1) <= range+1
        range_type limit;
        if(range == (std::numeric_limits<range_type>::max)()) {
          limit = range/(range_type(brange)+1);
          if(range % (range_type(brange)+1) == range_type(brange))
            ++limit;
        } else {
          limit = (range+1)/(range_type(brange)+1);
        }

        // We consider "result" as expressed to base (brange+1):
        // For every power of (brange+1), we determine a random factor
        range_type result = range_type(0);
        range_type mult = range_type(1);

        // loop invariants:
        //  result < mult
        //  mult <= range
        while(mult <= limit) {
          // Postcondition: result <= range, thus no overflow
          //
          // limit*(brange+1)<=range+1                   def. of limit       (1)
          // eng()-bmin<=brange                          eng() post.         (2)
          // and mult<=limit.                            loop condition      (3)
          // Therefore mult*(eng()-bmin+1)<=range+1      by (1),(2),(3)      (4)
          // Therefore mult*(eng()-bmin)+mult<=range+1   rearranging (4)     (5)
          // result<mult                                 loop invariant      (6)
          // Therefore result+mult*(eng()-bmin)<range+1  by (5), (6)         (7)
          //
          // Postcondition: result < mult*(brange+1)
          //
          // result<mult                                 loop invariant      (1)
          // eng()-bmin<=brange                          eng() post.         (2)
          // Therefore result+mult*(eng()-bmin) <
          //           mult+mult*(eng()-bmin)            by (1)              (3)
          // Therefore result+(eng()-bmin)*mult <
          //           mult+mult*brange                  by (2), (3)         (4)
          // Therefore result+(eng()-bmin)*mult <
          //           mult*(brange+1)                   by (4)
          result += static_cast<range_type>(static_cast<range_type>(random::detail::subtract<base_result>()(eng(), bmin)) * mult);

          // equivalent to (mult * (brange+1)) == range+1, but avoids overflow.
          if(mult * range_type(brange) == range - mult + 1) {
              // The destination range is an integer power of
              // the generator's range.
              return(result);
          }

          // Postcondition: mult <= range
          // 
          // limit*(brange+1)<=range+1                   def. of limit       (1)
          // mult<=limit                                 loop condition      (2)
          // Therefore mult*(brange+1)<=range+1          by (1), (2)         (3)
          // mult*(brange+1)!=range+1                    preceding if        (4)
          // Therefore mult*(brange+1)<range+1           by (3), (4)         (5)
          // 
          // Postcondition: result < mult
          //
          // See the second postcondition on the change to result. 
          mult *= range_type(brange)+range_type(1);
        }
        // loop postcondition: range/mult < brange+1
        //
        // mult > limit                                  loop condition      (1)
        // Suppose range/mult >= brange+1                Assumption          (2)
        // range >= mult*(brange+1)                      by (2)              (3)
        // range+1 > mult*(brange+1)                     by (3)              (4)
        // range+1 > (limit+1)*(brange+1)                by (1), (4)         (5)
        // (range+1)/(brange+1) > limit+1                by (5)              (6)
        // limit < floor((range+1)/(brange+1))           by (6)              (7)
        // limit==floor((range+1)/(brange+1))            def. of limit       (8)
        // not (2)                                       reductio            (9)
        //
        // loop postcondition: (range/mult)*mult+(mult-1) >= range
        //
        // (range/mult)*mult + range%mult == range       identity            (1)
        // range%mult < mult                             def. of %           (2)
        // (range/mult)*mult+mult > range                by (1), (2)         (3)
        // (range/mult)*mult+(mult-1) >= range           by (3)              (4)
        //
        // Note that the maximum value of result at this point is (mult-1),
        // so after this final step, we generate numbers that can be
        // at least as large as range.  We have to really careful to avoid
        // overflow in this final addition and in the rejection.  Anything
        // that overflows is larger than range and can thus be rejected.

        // range/mult < brange+1  -> no endless loop
        range_type result_increment =
            generate_uniform_int(
                eng,
                static_cast<range_type>(0),
                static_cast<range_type>(range/mult),
                boost::true_type());
        if(std::numeric_limits<range_type>::is_bounded && ((std::numeric_limits<range_type>::max)() / mult < result_increment)) {
          // The multiplcation would overflow.  Reject immediately.
          continue;
        }
        result_increment *= mult;
        // unsigned integers are guaranteed to wrap on overflow.
        result += result_increment;
        if(result < result_increment) {
          // The addition overflowed.  Reject.
          continue;
        }
        if(result > range) {
          // Too big.  Reject.
          continue;
        }
        return random::detail::add<range_type, result_type>()(result, min_value);
      }
    } else {                   // brange > range
#ifdef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
      typedef typename conditional<
         std::numeric_limits<range_type>::is_specialized && std::numeric_limits<base_unsigned>::is_specialized
         && (std::numeric_limits<range_type>::digits >= std::numeric_limits<base_unsigned>::digits),
         range_type, base_unsigned>::type mixed_range_type;
#else
      typedef base_unsigned mixed_range_type;
#endif

      mixed_range_type bucket_size;
      // it's safe to add 1 to range, as long as we cast it first,
      // because we know that it is less than brange.  However,
      // we do need to be careful not to cause overflow by adding 1
      // to brange.  We use mixed_range_type throughout for mixed
      // arithmetic between base_unsigned and range_type - in the case
      // that range_type has more bits than base_unsigned it is always
      // safe to use range_type for this albeit it may be more effient
      // to use base_unsigned.  The latter is a narrowing conversion though
      // which may be disallowed if range_type is a multiprecision type
      // and there are no explicit converison operators.

      if(brange == (std::numeric_limits<base_unsigned>::max)()) {
        bucket_size = static_cast<mixed_range_type>(brange) / (static_cast<mixed_range_type>(range)+1);
        if(static_cast<mixed_range_type>(brange) % (static_cast<mixed_range_type>(range)+1) == static_cast<mixed_range_type>(range)) {
          ++bucket_size;
        }
      } else {
        bucket_size = static_cast<mixed_range_type>(brange + 1) / (static_cast<mixed_range_type>(range)+1);
      }
      for(;;) {
        mixed_range_type result =
          random::detail::subtract<base_result>()(eng(), bmin);
        result /= bucket_size;
        // result and range are non-negative, and result is possibly larger
        // than range, so the cast is safe
        if(result <= static_cast<mixed_range_type>(range))
          return random::detail::add<mixed_range_type, result_type>()(result, min_value);
      }
    }
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

template<class Engine, class T>
inline T generate_uniform_int(
    Engine& eng, T min_value, T max_value,
    boost::false_type /** is_integral<Engine::result_type> */)
{
    uniform_int_float<Engine> wrapper(eng);
    return generate_uniform_int(wrapper, min_value, max_value, boost::true_type());
}

template<class Engine, class T>
inline T generate_uniform_int(Engine& eng, T min_value, T max_value)
{
    typedef typename Engine::result_type base_result;
    return generate_uniform_int(eng, min_value, max_value,
        boost::random::traits::is_integral<base_result>());
}

}

/**
 * The class template uniform_int_distribution models a \random_distribution.
 * On each invocation, it returns a random integer value uniformly
 * distributed in the set of integers {min, min+1, min+2, ..., max}.
 *
 * The template parameter IntType shall denote an integer-like value type.
 */
template<class IntType = int>
class uniform_int_distribution
{
public:
    typedef IntType input_type;
    typedef IntType result_type;

    class param_type
    {
    public:

        typedef uniform_int_distribution distribution_type;

        /**
         * Constructs the parameters of a uniform_int_distribution.
         *
         * Requires min <= max
         */
        explicit param_type(
            IntType min_arg = 0,
            IntType max_arg = (std::numeric_limits<IntType>::max)())
          : _min(min_arg), _max(max_arg)
        {
            BOOST_ASSERT(_min <= _max);
        }

        /** Returns the minimum value of the distribution. */
        IntType a() const { return _min; }
        /** Returns the maximum value of the distribution. */
        IntType b() const { return _max; }

        /** Writes the parameters to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        {
            os << parm._min << " " << parm._max;
            return os;
        }

        /** Reads the parameters from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        {
            IntType min_in, max_in;
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

        IntType _min;
        IntType _max;
    };

    /**
     * Constructs a uniform_int_distribution. @c min and @c max are
     * the parameters of the distribution.
     *
     * Requires: min <= max
     */
    explicit uniform_int_distribution(
        IntType min_arg = 0,
        IntType max_arg = (std::numeric_limits<IntType>::max)())
      : _min(min_arg), _max(max_arg)
    {
        BOOST_ASSERT(min_arg <= max_arg);
    }
    /** Constructs a uniform_int_distribution from its parameters. */
    explicit uniform_int_distribution(const param_type& parm)
      : _min(parm.a()), _max(parm.b()) {}

    /**  Returns the minimum value of the distribution */
    IntType min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _min; }
    /**  Returns the maximum value of the distribution */
    IntType max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _max; }

    /**  Returns the minimum value of the distribution */
    IntType a() const { return _min; }
    /**  Returns the maximum value of the distribution */
    IntType b() const { return _max; }

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

    /** Returns an integer uniformly distributed in the range [min, max]. */
    template<class Engine>
    result_type operator()(Engine& eng) const
    { return detail::generate_uniform_int(eng, _min, _max); }

    /**
     * Returns an integer uniformly distributed in the range
     * [param.a(), param.b()].
     */
    template<class Engine>
    result_type operator()(Engine& eng, const param_type& parm) const
    { return detail::generate_uniform_int(eng, parm.a(), parm.b()); }

    /** Writes the distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, uniform_int_distribution, ud)
    {
        os << ud.param();
        return os;
    }

    /** Reads the distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, uniform_int_distribution, ud)
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
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(uniform_int_distribution, lhs, rhs)
    { return lhs._min == rhs._min && lhs._max == rhs._max; }
    
    /**
     * Returns true if the two distributions may produce different sequences
     * of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(uniform_int_distribution)

private:
    IntType _min;
    IntType _max;
};

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_UNIFORM_INT_HPP
