//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_NUMERIC_UTILS_FEB_23_2007_0841PM)
#define BOOST_SPIRIT_KARMA_NUMERIC_UTILS_FEB_23_2007_0841PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/config/no_tr1/cmath.hpp>
#include <boost/limits.hpp>

#include <boost/type_traits/is_integral.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/numeric_traits.hpp>
#include <boost/spirit/home/support/detail/pow10.hpp>
#include <boost/spirit/home/karma/detail/generate_to.hpp>
#include <boost/spirit/home/karma/detail/string_generate.hpp>

#include <boost/core/cmath.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  The value BOOST_KARMA_NUMERICS_LOOP_UNROLL specifies, how to unroll the
//  integer string generation loop (see below).
//
//      Set the value to some integer in between 0 (no unrolling) and the
//      largest expected generated integer string length (complete unrolling).
//      If not specified, this value defaults to 6.
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_KARMA_NUMERICS_LOOP_UNROLL)
#define BOOST_KARMA_NUMERICS_LOOP_UNROLL 6
#endif

#if BOOST_KARMA_NUMERICS_LOOP_UNROLL < 0
#error "Please set the BOOST_KARMA_NUMERICS_LOOP_UNROLL to a non-negative value!"
#endif

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////
    //
    //  return the absolute value from a given number, avoiding over- and
    //  underflow
    //
    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable/* = void*/>
    struct absolute_value
    {
        typedef T type;
        static T call (T n)
        {
            // allow for ADL to find the correct overloads for fabs
            using namespace std;
            return fabs(n);
        }
    };

#define BOOST_SPIRIT_ABSOLUTE_VALUE(signedtype, unsignedtype)                 \
        template <>                                                           \
        struct absolute_value<signedtype>                                     \
        {                                                                     \
            typedef unsignedtype type;                                        \
            static type call(signedtype n)                                    \
            {                                                                 \
                /* implementation is well-defined for one's complement, */    \
                /* two's complement, and signed magnitude architectures */    \
                /* by the C++ Standard. [conv.integral] [expr.unary.op] */    \
                return (n >= 0) ?  static_cast<type>(n)                       \
                                : -static_cast<type>(n);                      \
            }                                                                 \
        }                                                                     \
    /**/
#define BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(unsignedtype)                    \
        template <>                                                           \
        struct absolute_value<unsignedtype>                                   \
        {                                                                     \
            typedef unsignedtype type;                                        \
            static type call(unsignedtype n)                                  \
            {                                                                 \
                return n;                                                     \
            }                                                                 \
        }                                                                     \
    /**/

#if defined(BOOST_MSVC)
# pragma warning(push)
// unary minus operator applied to unsigned type, result still unsigned
# pragma warning(disable: 4146)
#endif
    BOOST_SPIRIT_ABSOLUTE_VALUE(signed char, unsigned char);
    BOOST_SPIRIT_ABSOLUTE_VALUE(char, unsigned char);
    BOOST_SPIRIT_ABSOLUTE_VALUE(short, unsigned short);
    BOOST_SPIRIT_ABSOLUTE_VALUE(int, unsigned int);
    BOOST_SPIRIT_ABSOLUTE_VALUE(long, unsigned long);
    BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(unsigned char);
    BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(unsigned short);
    BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(unsigned int);
    BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(unsigned long);
#ifdef BOOST_HAS_LONG_LONG
    BOOST_SPIRIT_ABSOLUTE_VALUE(boost::long_long_type, boost::ulong_long_type);
    BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(boost::ulong_long_type);
#endif
#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#undef BOOST_SPIRIT_ABSOLUTE_VALUE
#undef BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED

    template <>
    struct absolute_value<float>
    {
        typedef float type;
        static type call(float n)
        {
            return (std::fabs)(n);
        }
    };

    template <>
    struct absolute_value<double>
    {
        typedef double type;
        static type call(double n)
        {
            return (std::fabs)(n);
        }
    };

    template <>
    struct absolute_value<long double>
    {
        typedef long double type;
        static type call(long double n)
        {
            return (std::fabs)(n);
        }
    };

    // specialization for pointers
    template <typename T>
    struct absolute_value<T*>
    {
        typedef std::size_t type;
        static type call (T* p)
        {
            return std::size_t(p);
        }
    };

    template <typename T>
    inline typename absolute_value<T>::type
    get_absolute_value(T n)
    {
        return absolute_value<T>::call(n);
    }

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable/* = void*/>
    struct is_negative
    {
        static bool call(T n)
        {
            return (n < 0) ? true : false;
        }
    };

    template <>
    struct is_negative<float>
    {
        static bool call(float n)
        {
            return (core::signbit)(n) ? true : false;
        }
    };

    template <>
    struct is_negative<double>
    {
        static bool call(double n)
        {
            return (core::signbit)(n) ? true : false;
        }
    };

    template <>
    struct is_negative<long double>
    {
        static bool call(long double n)
        {
            return (core::signbit)(n) ? true : false;
        }
    };

    template <typename T>
    inline bool test_negative(T n)
    {
        return is_negative<T>::call(n);
    }

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable/* = void*/>
    struct is_zero
    {
        static bool call(T n)
        {
            return (n == 0) ? true : false;
        }
    };

    template <>
    struct is_zero<float>
    {
        static bool call(float n)
        {
            return (core::fpclassify)(n) == core::fp_zero;
        }
    };

    template <>
    struct is_zero<double>
    {
        static bool call(double n)
        {
            return (core::fpclassify)(n) == core::fp_zero;
        }
    };

    template <>
    struct is_zero<long double>
    {
        static bool call(long double n)
        {
            return (core::fpclassify)(n) == core::fp_zero;
        }
    };

    template <typename T>
    inline bool test_zero(T n)
    {
        return is_zero<T>::call(n);
    }

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable/* = void*/>
    struct is_nan
    {
        static bool call(T n)
        {
            // NaN numbers are not equal to anything
            return (n != n) ? true : false;
        }
    };

    template <>
    struct is_nan<float>
    {
        static bool call(float n)
        {
            return (core::fpclassify)(n) == core::fp_nan;
        }
    };

    template <>
    struct is_nan<double>
    {
        static bool call(double n)
        {
            return (core::fpclassify)(n) == core::fp_nan;
        }
    };

    template <>
    struct is_nan<long double>
    {
        static bool call(long double n)
        {
            return (core::fpclassify)(n) == core::fp_nan;
        }
    };

    template <typename T>
    inline bool test_nan(T n)
    {
        return is_nan<T>::call(n);
    }

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable/* = void*/>
    struct is_infinite
    {
        static bool call(T n)
        {
            return std::numeric_limits<T>::has_infinity
                && n == std::numeric_limits<T>::infinity();
        }
    };

    template <>
    struct is_infinite<float>
    {
        static bool call(float n)
        {
            return (core::fpclassify)(n) == core::fp_infinite;
        }
    };

    template <>
    struct is_infinite<double>
    {
        static bool call(double n)
        {
            return (core::fpclassify)(n) == core::fp_infinite;
        }
    };

    template <>
    struct is_infinite<long double>
    {
        static bool call(long double n)
        {
            return (core::fpclassify)(n) == core::fp_infinite;
        }
    };

    template <typename T>
    inline bool test_infinite(T n)
    {
        return is_infinite<T>::call(n);
    }

    ///////////////////////////////////////////////////////////////////////
    struct cast_to_long
    {
        static long call(float n, mpl::false_)
        {
            return static_cast<long>(std::floor(n));
        }

        static long call(double n, mpl::false_)
        {
            return static_cast<long>(std::floor(n));
        }

        static long call(long double n, mpl::false_)
        {
            return static_cast<long>(std::floor(n));
        }

        template <typename T>
        static long call(T n, mpl::false_)
        {
            // allow for ADL to find the correct overload for floor and
            // lround
            using namespace std;
            return lround(floor(n));
        }

        template <typename T>
        static long call(T n, mpl::true_)
        {
            return static_cast<long>(n);
        }

        template <typename T>
        static long call(T n)
        {
            return call(n, mpl::bool_<is_integral<T>::value>());
        }
    };

    ///////////////////////////////////////////////////////////////////////
    struct truncate_to_long
    {
        static long call(float n, mpl::false_)
        {
            return test_negative(n) ? static_cast<long>(std::ceil(n)) :
                static_cast<long>(std::floor(n));
        }

        static long call(double n, mpl::false_)
        {
            return test_negative(n) ? static_cast<long>(std::ceil(n)) :
                static_cast<long>(std::floor(n));
        }

        static long call(long double n, mpl::false_)
        {
            return test_negative(n) ? static_cast<long>(std::ceil(n)) :
                static_cast<long>(std::floor(n));
        }

        template <typename T>
        static long call(T n, mpl::false_)
        {
            // allow for ADL to find the correct overloads for ltrunc
            using namespace std;
            return ltrunc(n);
        }

        template <typename T>
        static long call(T n, mpl::true_)
        {
            return static_cast<long>(n);
        }

        template <typename T>
        static long call(T n)
        {
            return call(n, mpl::bool_<is_integral<T>::value>());
        }
    };

    ///////////////////////////////////////////////////////////////////////
    //
    //  Traits class for radix specific number conversion
    //
    //      Convert a digit from binary representation to character
    //      representation:
    //
    //          static int call(unsigned n);
    //
    ///////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename CharEncoding, typename Tag, bool radix_less_than_10>
        struct convert_digit
        {
            static int call(unsigned n)
            {
                if (n <= 9)
                    return n + '0';

                using spirit::char_class::convert;
                return convert<CharEncoding>::to(Tag(), n - 10 + 'a');
            }
        };

        template <>
        struct convert_digit<unused_type, unused_type, false>
        {
            static int call(unsigned n)
            {
                if (n <= 9)
                    return n + '0';
                return n - 10 + 'a';
            }
        };

        template <typename CharEncoding, typename Tag>
        struct convert_digit<CharEncoding, Tag, true>
        {
            static int call(unsigned n)
            {
                return n + '0';
            }
        };
    }

    template <unsigned Radix, typename CharEncoding, typename Tag>
    struct convert_digit
      : detail::convert_digit<CharEncoding, Tag, (Radix <= 10) ? true : false>
    {};

    ///////////////////////////////////////////////////////////////////////
    template <unsigned Radix>
    struct divide
    {
        template <typename T>
        static T call(T& n, mpl::true_)
        {
            return n / Radix;
        }

        template <typename T>
        static T call(T& n, mpl::false_)
        {
            // Allow ADL to find the correct overload for floor
            using namespace std;
            return floor(n / Radix);
        }

        template <typename T>
        static T call(T& n, T const&, int)
        {
            return call(n, mpl::bool_<is_integral<T>::value>());
        }

        template <typename T>
        static T call(T& n)
        {
            return call(n, mpl::bool_<is_integral<T>::value>());
        }
    };

    // specialization for division by 10
    template <>
    struct divide<10>
    {
        template <typename T>
        static T call(T& n, T, int, mpl::true_)
        {
            return n / 10;
        }

        template <typename T>
        static T call(T, T& num, int exp, mpl::false_)
        {
            // Allow ADL to find the correct overload for floor
            using namespace std;
            return floor(num / spirit::traits::pow10<T>(exp));
        }

        template <typename T>
        static T call(T& n, T& num, int exp)
        {
            return call(n, num, exp, mpl::bool_<is_integral<T>::value>());
        }

        template <typename T>
        static T call(T& n)
        {
            return call(n, n, 1, mpl::bool_<is_integral<T>::value>());
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <unsigned Radix>
    struct remainder
    {
        template <typename T>
        static long call(T n, mpl::true_)
        {
            // this cast is safe since we know the result is not larger
            // than Radix
            return static_cast<long>(n % Radix);
        }

        template <typename T>
        static long call(T n, mpl::false_)
        {
            // Allow ADL to find the correct overload for fmod
            using namespace std;
            return cast_to_long::call(fmod(n, T(Radix)));
        }

        template <typename T>
        static long call(T n)
        {
            return call(n, mpl::bool_<is_integral<T>::value>());
        }
    };
}}}

namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    //
    //  The int_inserter template takes care of the integer to string
    //  conversion. If specified, the loop is unrolled for better performance.
    //
    //      Set the value BOOST_KARMA_NUMERICS_LOOP_UNROLL to some integer in
    //      between 0 (no unrolling) and the largest expected generated integer
    //      string length (complete unrolling).
    //      If not specified, this value defaults to 6.
    //
    ///////////////////////////////////////////////////////////////////////////
#define BOOST_KARMA_NUMERICS_INNER_LOOP_PREFIX(z, x, data)                    \
        if (!traits::test_zero(n)) {                                          \
            int ch_##x = radix_type::call(remainder_type::call(n));           \
            n = divide_type::call(n, num, ++exp);                             \
    /**/

#define BOOST_KARMA_NUMERICS_INNER_LOOP_SUFFIX(z, x, n_rolls_sub1)            \
            *sink = char(BOOST_PP_CAT(ch_, BOOST_PP_SUB(n_rolls_sub1, x)));   \
            ++sink;                                                           \
        }                                                                     \
    /**/

    template <
        unsigned Radix, typename CharEncoding = unused_type
      , typename Tag = unused_type>
    struct int_inserter
    {
        typedef traits::convert_digit<Radix, CharEncoding, Tag> radix_type;
        typedef traits::divide<Radix> divide_type;
        typedef traits::remainder<Radix> remainder_type;

        template <typename OutputIterator, typename T>
        static bool
        call(OutputIterator& sink, T n, T& num, int exp)
        {
            // remainder_type::call returns n % Radix
            int ch = radix_type::call(remainder_type::call(n));
            n = divide_type::call(n, num, ++exp);

            BOOST_PP_REPEAT(
                BOOST_KARMA_NUMERICS_LOOP_UNROLL,
                BOOST_KARMA_NUMERICS_INNER_LOOP_PREFIX, _);

            if (!traits::test_zero(n))
                call(sink, n, num, exp);

            BOOST_PP_REPEAT(
                BOOST_KARMA_NUMERICS_LOOP_UNROLL,
                BOOST_KARMA_NUMERICS_INNER_LOOP_SUFFIX,
                BOOST_PP_DEC(BOOST_KARMA_NUMERICS_LOOP_UNROLL));

            *sink = char(ch);
            ++sink;
            return true;
        }

        //  Common code for integer string representations
        template <typename OutputIterator, typename T>
        static bool
        call(OutputIterator& sink, T n)
        {
            return call(sink, n, n, 0);
        }

    private:
        // helper function returning the biggest number representable either in
        // a boost::long_long_type (if this does exist) or in a plain long
        // otherwise
#if defined(BOOST_HAS_LONG_LONG)
        typedef boost::long_long_type biggest_long_type;
#else
        typedef long biggest_long_type;
#endif

        static biggest_long_type max_long()
        {
            return (std::numeric_limits<biggest_long_type>::max)();
        }

    public:
        // Specialization for doubles and floats, falling back to long integers
        // for representable values. These specializations speed up formatting
        // of floating point numbers considerably as all the required
        // arithmetics will be executed using integral data types.
        template <typename OutputIterator>
        static bool
        call(OutputIterator& sink, long double n)
        {
            if (std::fabs(n) < max_long())
            {
                biggest_long_type l((biggest_long_type)n);
                return call(sink, l, l, 0);
            }
            return call(sink, n, n, 0);
        }
        template <typename OutputIterator>
        static bool
        call(OutputIterator& sink, double n)
        {
            if (std::fabs(n) < max_long())
            {
                biggest_long_type l((biggest_long_type)n);
                return call(sink, l, l, 0);
            }
            return call(sink, n, n, 0);
        }
        template <typename OutputIterator>
        static bool
        call(OutputIterator& sink, float n)
        {
            if (std::fabs(n) < max_long())
            {
                biggest_long_type l((biggest_long_type)n);
                return call(sink, l, l, 0);
            }
            return call(sink, n, n, 0);
        }
    };

#undef BOOST_KARMA_NUMERICS_INNER_LOOP_PREFIX
#undef BOOST_KARMA_NUMERICS_INNER_LOOP_SUFFIX

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The uint_inserter template takes care of the conversion of any integer
    //  to a string, while interpreting the number as an unsigned type.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <
        unsigned Radix, typename CharEncoding = unused_type
      , typename Tag = unused_type>
    struct uint_inserter : int_inserter<Radix, CharEncoding, Tag>
    {
        typedef int_inserter<Radix, CharEncoding, Tag> base_type;

        //  Common code for integer string representations
        template <typename OutputIterator, typename T>
        static bool
        call(OutputIterator& sink, T const& n)
        {
            typedef typename traits::absolute_value<T>::type type;
            type un = type(n);
            return base_type::call(sink, un, un, 0);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The sign_inserter template generates a sign for a given numeric value.
    //
    //    The parameter forcesign allows to generate a sign even for positive
    //    numbers.
    //
    ///////////////////////////////////////////////////////////////////////////
    struct sign_inserter
    {
        template <typename OutputIterator>
        static bool
        call_noforce(OutputIterator& sink, bool is_zero, bool is_negative,
            bool sign_if_zero)
        {
            // generate a sign for negative numbers only
            if (is_negative || (is_zero && sign_if_zero)) {
                *sink = '-';
                ++sink;
            }
            return true;
        }

        template <typename OutputIterator>
        static bool
        call_force(OutputIterator& sink, bool is_zero, bool is_negative,
            bool sign_if_zero)
        {
            // generate a sign for all numbers except zero
            if (!is_zero || sign_if_zero)
                *sink = is_negative ? '-' : '+';
            else
                *sink = ' ';

            ++sink;
            return true;
        }

        template <typename OutputIterator>
        static bool
        call(OutputIterator& sink, bool is_zero, bool is_negative
          , bool forcesign, bool sign_if_zero = false)
        {
            return forcesign ?
                call_force(sink, is_zero, is_negative, sign_if_zero) :
                call_noforce(sink, is_zero, is_negative, sign_if_zero);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  These are helper functions for the real policies allowing to generate
    //  a single character and a string
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding = unused_type, typename Tag = unused_type>
    struct char_inserter
    {
        template <typename OutputIterator, typename Char>
        static bool call(OutputIterator& sink, Char c)
        {
            return detail::generate_to(sink, c, CharEncoding(), Tag());
        }
    };

    template <typename CharEncoding = unused_type, typename Tag = unused_type>
    struct string_inserter
    {
        template <typename OutputIterator, typename String>
        static bool call(OutputIterator& sink, String str)
        {
            return detail::string_generate(sink, str, CharEncoding(), Tag());
        }
    };

}}}

#endif
