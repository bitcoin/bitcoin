//  (c) Copyright Fernando Luis Cacciola Carballal 2000-2004
//  Use, modification, and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/numeric/conversion
//
// Contact the author at: fernando_cacciola@hotmail.com
//
#ifndef BOOST_NUMERIC_CONVERSION_CONVERTER_POLICIES_FLC_12NOV2002_HPP
#define BOOST_NUMERIC_CONVERSION_CONVERTER_POLICIES_FLC_12NOV2002_HPP

#include <functional>
#include <typeinfo> // for std::bad_cast

#include <boost/config.hpp>
#include <boost/config/no_tr1/cmath.hpp> // for std::floor and std::ceil
#include <boost/throw_exception.hpp>

#include "boost/type_traits/is_arithmetic.hpp"

#include "boost/mpl/if.hpp"
#include "boost/mpl/integral_c.hpp"

namespace boost { namespace numeric
{

template<class S>
struct Trunc
{
  typedef S source_type ;

  typedef typename mpl::if_< is_arithmetic<S>,S,S const&>::type argument_type ;

  static source_type nearbyint ( argument_type s )
  {
#if !defined(BOOST_NO_STDC_NAMESPACE)
    using std::floor ;
    using std::ceil  ;
#endif

    return s < static_cast<S>(0) ? ceil(s) : floor(s) ;
  }

  typedef mpl::integral_c< std::float_round_style, std::round_toward_zero> round_style ;
} ;



template<class S>
struct Floor
{
  typedef S source_type ;

  typedef typename mpl::if_< is_arithmetic<S>,S,S const&>::type argument_type ;

  static source_type nearbyint ( argument_type s )
  {
#if !defined(BOOST_NO_STDC_NAMESPACE)
    using std::floor ;
#endif

    return floor(s) ;
  }

  typedef mpl::integral_c< std::float_round_style, std::round_toward_neg_infinity> round_style ;
} ;

template<class S>
struct Ceil
{
  typedef S source_type ;

  typedef typename mpl::if_< is_arithmetic<S>,S,S const&>::type argument_type ;

  static source_type nearbyint ( argument_type s )
  {
#if !defined(BOOST_NO_STDC_NAMESPACE)
    using std::ceil ;
#endif

    return ceil(s) ;
  }

  typedef mpl::integral_c< std::float_round_style, std::round_toward_infinity> round_style ;
} ;

template<class S>
struct RoundEven
{
  typedef S source_type ;

  typedef typename mpl::if_< is_arithmetic<S>,S,S const&>::type argument_type ;

  static source_type nearbyint ( argument_type s )
  {
    // Algorithm contributed by Guillaume Melquiond

#if !defined(BOOST_NO_STDC_NAMESPACE)
    using std::floor ;
    using std::ceil  ;
#endif

    // only works inside the range not at the boundaries
    S prev = floor(s);
    S next = ceil(s);

    S rt = (s - prev) - (next - s); // remainder type

    S const zero(0.0);
    S const two(2.0);

    if ( rt < zero )
      return prev;
    else if ( rt > zero )
      return next;
    else
    {
      bool is_prev_even = two * floor(prev / two) == prev ;
      return ( is_prev_even ? prev : next ) ;
    }
  }

  typedef mpl::integral_c< std::float_round_style, std::round_to_nearest> round_style ;
} ;


enum range_check_result
{
  cInRange     = 0 ,
  cNegOverflow = 1 ,
  cPosOverflow = 2
} ;

class bad_numeric_cast : public std::bad_cast
{
  public:

    const char * what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE
      {  return "bad numeric conversion: overflow"; }
};

class negative_overflow : public bad_numeric_cast
{
  public:

    const char * what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE
      {  return "bad numeric conversion: negative overflow"; }
};
class positive_overflow : public bad_numeric_cast
{
  public:

    const char * what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE
      { return "bad numeric conversion: positive overflow"; }
};

struct def_overflow_handler
{
  void operator() ( range_check_result r ) // throw(negative_overflow,positive_overflow)
  {
#ifndef BOOST_NO_EXCEPTIONS
    if ( r == cNegOverflow )
      throw negative_overflow() ;
    else if ( r == cPosOverflow )
           throw positive_overflow() ;
#else
    if ( r == cNegOverflow )
      ::boost::throw_exception(negative_overflow()) ;
    else if ( r == cPosOverflow )
           ::boost::throw_exception(positive_overflow()) ;
#endif
  }
} ;

struct silent_overflow_handler
{
  void operator() ( range_check_result ) {} // throw()
} ;

template<class Traits>
struct raw_converter
{
  typedef typename Traits::result_type   result_type   ;
  typedef typename Traits::argument_type argument_type ;

  static result_type low_level_convert ( argument_type s ) { return static_cast<result_type>(s) ; }
} ;

struct UseInternalRangeChecker {} ;

} } // namespace boost::numeric

#endif
