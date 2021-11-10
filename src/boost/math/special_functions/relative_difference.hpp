//  (C) Copyright John Maddock 2006, 2015
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_RELATIVE_ERROR
#define BOOST_MATH_RELATIVE_ERROR

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/tools/promotion.hpp>
#include <boost/math/tools/precision.hpp>

namespace boost{
   namespace math{

      template <class T, class U>
      typename boost::math::tools::promote_args<T,U>::type relative_difference(const T& arg_a, const U& arg_b)
      {
         typedef typename boost::math::tools::promote_args<T, U>::type result_type;
         result_type a = arg_a;
         result_type b = arg_b;
         BOOST_MATH_STD_USING
#ifdef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
         //
         // If math.h has no long double support we can't rely
         // on the math functions generating exponents outside
         // the range of a double:
         //
         result_type min_val = (std::max)(
         tools::min_value<result_type>(),
         static_cast<result_type>((std::numeric_limits<double>::min)()));
         result_type max_val = (std::min)(
            tools::max_value<result_type>(),
            static_cast<result_type>((std::numeric_limits<double>::max)()));
#else
         result_type min_val = tools::min_value<result_type>();
         result_type max_val = tools::max_value<result_type>();
#endif
         // Screen out NaN's first, if either value is a NaN then the distance is "infinite":
         if((boost::math::isnan)(a) || (boost::math::isnan)(b))
            return max_val;
         // Screen out infinities:
         if(fabs(b) > max_val)
         {
            if(fabs(a) > max_val)
               return (a < 0) == (b < 0) ? 0 : max_val;  // one infinity is as good as another!
            else
               return max_val;  // one infinity and one finite value implies infinite difference
         }
         else if(fabs(a) > max_val)
            return max_val;    // one infinity and one finite value implies infinite difference

         //
         // If the values have different signs, treat as infinite difference:
         //
         if(((a < 0) != (b < 0)) && (a != 0) && (b != 0))
            return max_val;
         a = fabs(a);
         b = fabs(b);
         //
         // Now deal with zero's, if one value is zero (or denorm) then treat it the same as
         // min_val for the purposes of the calculation that follows:
         //
         if(a < min_val)
            a = min_val;
         if(b < min_val)
            b = min_val;

         return (std::max)(fabs((a - b) / a), fabs((a - b) / b));
      }

#if (defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)) && (LDBL_MAX_EXP <= DBL_MAX_EXP)
      template <>
      inline boost::math::tools::promote_args<double, double>::type relative_difference(const double& arg_a, const double& arg_b)
      {
         BOOST_MATH_STD_USING
         double a = arg_a;
         double b = arg_b;
         //
         // On Mac OS X we evaluate "double" functions at "long double" precision,
         // but "long double" actually has a very slightly narrower range than "double"!  
         // Therefore use the range of "long double" as our limits since results outside
         // that range may have been truncated to 0 or INF:
         //
         double min_val = (std::max)((double)tools::min_value<long double>(), tools::min_value<double>());
         double max_val = (std::min)((double)tools::max_value<long double>(), tools::max_value<double>());

         // Screen out NaN's first, if either value is a NaN then the distance is "infinite":
         if((boost::math::isnan)(a) || (boost::math::isnan)(b))
            return max_val;
         // Screen out infinities:
         if(fabs(b) > max_val)
         {
            if(fabs(a) > max_val)
               return 0;  // one infinity is as good as another!
            else
               return max_val;  // one infinity and one finite value implies infinite difference
         }
         else if(fabs(a) > max_val)
            return max_val;    // one infinity and one finite value implies infinite difference

         //
         // If the values have different signs, treat as infinite difference:
         //
         if(((a < 0) != (b < 0)) && (a != 0) && (b != 0))
            return max_val;
         a = fabs(a);
         b = fabs(b);
         //
         // Now deal with zero's, if one value is zero (or denorm) then treat it the same as
         // min_val for the purposes of the calculation that follows:
         //
         if(a < min_val)
            a = min_val;
         if(b < min_val)
            b = min_val;

         return (std::max)(fabs((a - b) / a), fabs((a - b) / b));
      }
#endif

      template <class T, class U>
      inline typename boost::math::tools::promote_args<T, U>::type epsilon_difference(const T& arg_a, const U& arg_b)
      {
         typedef typename boost::math::tools::promote_args<T, U>::type result_type;
         result_type r = relative_difference(arg_a, arg_b);
         if(tools::max_value<result_type>() * boost::math::tools::epsilon<result_type>() < r)
            return tools::max_value<result_type>();
         return r / boost::math::tools::epsilon<result_type>();
      }
} // namespace math
} // namespace boost

#endif
