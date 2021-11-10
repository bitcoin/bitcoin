//  boost cast.hpp header file  ----------------------------------------------//

//  (C) Copyright Kevlin Henney and Dave Abrahams 1999.
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/conversion for Documentation.

//  Revision History
//  02 Jun 14  Remove VC6 workarounds.
//  16 Jul 11  Bugfixes for VC6.
//  23 JUN 05  Code extracted from /boost/cast.hpp into this new header.
//             Keeps this legacy version of numeric_cast<> for old compilers
//             wich can't compile the new version in /boost/numeric/conversion/cast.hpp
//             (Fernando Cacciola)
//  02 Apr 01  Removed BOOST_NO_LIMITS workarounds and included
//             <boost/limits.hpp> instead (the workaround did not
//             actually compile when BOOST_NO_LIMITS was defined in
//             any case, so we loose nothing). (John Maddock)
//  21 Jan 01  Undid a bug I introduced yesterday. numeric_cast<> never
//             worked with stock GCC; trying to get it to do that broke
//             vc-stlport.
//  20 Jan 01  Moved BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS to config.hpp.
//             Removed unused BOOST_EXPLICIT_TARGET macro. Moved
//             boost::detail::type to boost/type.hpp. Made it compile with
//             stock gcc again (Dave Abrahams)
//  29 Nov 00  Remove nested namespace cast, cleanup spacing before Formal
//             Review (Beman Dawes)
//  19 Oct 00  Fix numeric_cast for floating-point types (Dave Abrahams)
//  15 Jul 00  Suppress numeric_cast warnings for GCC, Borland and MSVC
//             (Dave Abrahams)
//  30 Jun 00  More MSVC6 wordarounds.  See comments below.  (Dave Abrahams)
//  28 Jun 00  Removed implicit_cast<>.  See comment below. (Beman Dawes)
//  27 Jun 00  More MSVC6 workarounds
//  15 Jun 00  Add workarounds for MSVC6
//   2 Feb 00  Remove bad_numeric_cast ";" syntax error (Doncho Angelov)
//  26 Jan 00  Add missing throw() to bad_numeric_cast::what(0 (Adam Levar)
//  29 Dec 99  Change using declarations so usages in other namespaces work
//             correctly (Dave Abrahams)
//  23 Sep 99  Change polymorphic_downcast assert to also detect M.I. errors
//             as suggested Darin Adler and improved by Valentin Bonnard.
//   2 Sep 99  Remove controversial asserts, simplify, rename.
//  30 Aug 99  Move to cast.hpp, replace value_cast with numeric_cast,
//             place in nested namespace.
//   3 Aug 99  Initial version

#ifndef BOOST_OLD_NUMERIC_CAST_HPP
#define BOOST_OLD_NUMERIC_CAST_HPP

# include <boost/config.hpp>
# include <cassert>
# include <typeinfo>
# include <boost/type.hpp>
# include <boost/limits.hpp>
# include <boost/numeric/conversion/converter_policies.hpp>

namespace boost
{
  using numeric::bad_numeric_cast;

//  LEGACY numeric_cast [only for some old broken compilers] --------------------------------------//

//  Contributed by Kevlin Henney

//  numeric_cast  ------------------------------------------------------------//

#if !defined(BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS) || defined(BOOST_SGI_CPP_LIMITS)

    namespace detail
    {
      template <class T>
      struct signed_numeric_limits : std::numeric_limits<T>
      {
             static inline T min BOOST_PREVENT_MACRO_SUBSTITUTION ()
         {
             return (std::numeric_limits<T>::min)() >= 0
                     // unary minus causes integral promotion, thus the static_cast<>
                     ? static_cast<T>(-(std::numeric_limits<T>::max)())
                     : (std::numeric_limits<T>::min)();
         };
      };

      // Move to namespace boost in utility.hpp?
      template <class T, bool specialized>
      struct fixed_numeric_limits_base
          : public if_true< std::numeric_limits<T>::is_signed >
           ::BOOST_NESTED_TEMPLATE then< signed_numeric_limits<T>,
                            std::numeric_limits<T>
                   >::type
      {};

      template <class T>
      struct fixed_numeric_limits
          : fixed_numeric_limits_base<T,(std::numeric_limits<T>::is_specialized)>
      {};

# ifdef BOOST_HAS_LONG_LONG
      // cover implementations which supply no specialization for long
      // long / unsigned long long. Not intended to be full
      // numeric_limits replacements, but good enough for numeric_cast<>
      template <>
      struct fixed_numeric_limits_base< ::boost::long_long_type, false>
      {
          BOOST_STATIC_CONSTANT(bool, is_specialized = true);
          BOOST_STATIC_CONSTANT(bool, is_signed = true);
          static  ::boost::long_long_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
          {
#  ifdef LONGLONG_MAX
              return LONGLONG_MAX;
#  else
              return 9223372036854775807LL; // hope this is portable
#  endif
          }

          static  ::boost::long_long_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
          {
#  ifdef LONGLONG_MIN
              return LONGLONG_MIN;
#  else
               return -( 9223372036854775807LL )-1; // hope this is portable
#  endif
          }
      };

      template <>
      struct fixed_numeric_limits_base< ::boost::ulong_long_type, false>
      {
          BOOST_STATIC_CONSTANT(bool, is_specialized = true);
          BOOST_STATIC_CONSTANT(bool, is_signed = false);
          static  ::boost::ulong_long_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
          {
#  ifdef ULONGLONG_MAX
              return ULONGLONG_MAX;
#  else
              return 0xffffffffffffffffULL; // hope this is portable
#  endif
          }

          static  ::boost::ulong_long_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
      };
# endif
    } // namespace detail

// less_than_type_min -
  //    x_is_signed should be numeric_limits<X>::is_signed
  //    y_is_signed should be numeric_limits<Y>::is_signed
  //    y_min should be numeric_limits<Y>::min()
  //
  //    check(x, y_min) returns true iff x < y_min without invoking comparisons
  //    between signed and unsigned values.
  //
  //    "poor man's partial specialization" is in use here.
    template <bool x_is_signed, bool y_is_signed>
    struct less_than_type_min
    {
        template <class X, class Y>
        static bool check(X x, Y y_min)
            { return x < y_min; }
    };

    template <>
    struct less_than_type_min<false, true>
    {
        template <class X, class Y>
        static bool check(X, Y)
            { return false; }
    };

    template <>
    struct less_than_type_min<true, false>
    {
        template <class X, class Y>
        static bool check(X x, Y)
            { return x < 0; }
    };

  // greater_than_type_max -
  //    same_sign should be:
  //            numeric_limits<X>::is_signed == numeric_limits<Y>::is_signed
  //    y_max should be numeric_limits<Y>::max()
  //
  //    check(x, y_max) returns true iff x > y_max without invoking comparisons
  //    between signed and unsigned values.
  //
  //    "poor man's partial specialization" is in use here.
    template <bool same_sign, bool x_is_signed>
    struct greater_than_type_max;

    template<>
    struct greater_than_type_max<true, true>
    {
        template <class X, class Y>
        static inline bool check(X x, Y y_max)
            { return x > y_max; }
    };

    template <>
    struct greater_than_type_max<false, true>
    {
        // What does the standard say about this? I think it's right, and it
        // will work with every compiler I know of.
        template <class X, class Y>
        static inline bool check(X x, Y)
            { return x >= 0 && static_cast<X>(static_cast<Y>(x)) != x; }
    };

    template<>
    struct greater_than_type_max<true, false>
    {
        template <class X, class Y>
        static inline bool check(X x, Y y_max)
            { return x > y_max; }
    };

    template <>
    struct greater_than_type_max<false, false>
    {
        // What does the standard say about this? I think it's right, and it
        // will work with every compiler I know of.
        template <class X, class Y>
        static inline bool check(X x, Y)
            { return static_cast<X>(static_cast<Y>(x)) != x; }
    };

#else // use #pragma hacks if available

  namespace detail
  {
# if BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4018)
#  pragma warning(disable : 4146)
#elif defined(BOOST_BORLANDC)
#  pragma option push -w-8041
# endif

       // Move to namespace boost in utility.hpp?
       template <class T>
       struct fixed_numeric_limits : public std::numeric_limits<T>
       {
           static inline T min BOOST_PREVENT_MACRO_SUBSTITUTION ()
           {
               return std::numeric_limits<T>::is_signed && (std::numeric_limits<T>::min)() >= 0
                   ? T(-(std::numeric_limits<T>::max)()) : (std::numeric_limits<T>::min)();
           }
       };

# if BOOST_MSVC
#  pragma warning(pop)
#elif defined(BOOST_BORLANDC)
#  pragma option pop
# endif
  } // namespace detail

#endif

    template<typename Target, typename Source>
    inline Target numeric_cast(Source arg)
    {
        // typedefs abbreviating respective trait classes
        typedef detail::fixed_numeric_limits<Source> arg_traits;
        typedef detail::fixed_numeric_limits<Target> result_traits;

#if defined(BOOST_STRICT_CONFIG) \
    || (!defined(__HP_aCC) || __HP_aCC > 33900) \
         && (!defined(BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS) \
             || defined(BOOST_SGI_CPP_LIMITS))
        // typedefs that act as compile time assertions
        // (to be replaced by boost compile time assertions
        // as and when they become available and are stable)
        typedef bool argument_must_be_numeric[arg_traits::is_specialized];
        typedef bool result_must_be_numeric[result_traits::is_specialized];

        const bool arg_is_signed = arg_traits::is_signed;
        const bool result_is_signed = result_traits::is_signed;
        const bool same_sign = arg_is_signed == result_is_signed;

        if (less_than_type_min<arg_is_signed, result_is_signed>::check(arg, (result_traits::min)())
            || greater_than_type_max<same_sign, arg_is_signed>::check(arg, (result_traits::max)())
            )

#else // We need to use #pragma hacks if available

# if BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4018)
#elif defined(BOOST_BORLANDC)
#pragma option push -w-8012
# endif
        if ((arg < 0 && !result_traits::is_signed)  // loss of negative range
             || (arg_traits::is_signed && arg < (result_traits::min)())  // underflow
             || arg > (result_traits::max)())            // overflow
# if BOOST_MSVC
#  pragma warning(pop)
#elif defined(BOOST_BORLANDC)
#pragma option pop
# endif
#endif
        {
            throw bad_numeric_cast();
        }
        return static_cast<Target>(arg);
    } // numeric_cast

} // namespace boost

#endif  // BOOST_OLD_NUMERIC_CAST_HPP
