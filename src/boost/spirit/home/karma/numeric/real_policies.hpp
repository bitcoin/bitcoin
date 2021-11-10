//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_REAL_POLICIES_MAR_02_2007_0936AM)
#define BOOST_SPIRIT_KARMA_REAL_POLICIES_MAR_02_2007_0936AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config/no_tr1/cmath.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/char.hpp>
#include <boost/spirit/home/karma/numeric/int.hpp>
#include <boost/spirit/home/karma/numeric/detail/real_utils.hpp>

#include <boost/mpl/bool.hpp>

namespace boost { namespace spirit { namespace karma 
{
    ///////////////////////////////////////////////////////////////////////////
    //
    //  real_policies, if you need special handling of your floating
    //  point numbers, just overload this policy class and use it as a template
    //  parameter to the karma::real_generator floating point specifier:
    //
    //      template <typename T>
    //      struct scientific_policy : karma::real_policies<T>
    //      {
    //          //  we want the numbers always to be in scientific format
    //          static int floatfield(T n) { return fmtflags::scientific; }
    //      };
    //
    //      typedef 
    //          karma::real_generator<double, scientific_policy<double> > 
    //      science_type;
    //
    //      karma::generate(sink, science_type(), 1.0); // will output: 1.0e00
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct real_policies
    {
        ///////////////////////////////////////////////////////////////////////
        // Expose the data type the generator is targeted at
        ///////////////////////////////////////////////////////////////////////
        typedef T value_type;

        ///////////////////////////////////////////////////////////////////////
        //  By default the policy doesn't require any special iterator 
        //  functionality. The floating point generator exposes its properties
        //  from here, so this needs to be updated in case other properties
        //  need to be implemented.
        ///////////////////////////////////////////////////////////////////////
        typedef mpl::int_<generator_properties::no_properties> properties;

        ///////////////////////////////////////////////////////////////////////
        //  Specifies, which representation type to use during output 
        //  generation.
        ///////////////////////////////////////////////////////////////////////
        struct fmtflags
        {
            enum {
                scientific = 0,   // Generate floating-point values in scientific 
                                  // format (with an exponent field).
                fixed = 1         // Generate floating-point values in fixed-point 
                                  // format (with no exponent field). 
            };
        };

        ///////////////////////////////////////////////////////////////////////
        //  This is the main function used to generate the output for a 
        //  floating point number. It is called by the real generator in order 
        //  to perform the conversion. In theory all of the work can be 
        //  implemented here, but it is the easiest to use existing 
        //  functionality provided by the type specified by the template 
        //  parameter `Inserter`. 
        //
        //      sink: the output iterator to use for generation
        //      n:    the floating point number to convert 
        //      p:    the instance of the policy type used to instantiate this 
        //            floating point generator.
        ///////////////////////////////////////////////////////////////////////
        template <typename Inserter, typename OutputIterator, typename Policies>
        static bool
        call (OutputIterator& sink, T n, Policies const& p)
        {
            return Inserter::call_n(sink, n, p);
        }

        ///////////////////////////////////////////////////////////////////////
        //  The default behavior is to not to require generating a sign. If 
        //  'force_sign()' returns true, then all generated numbers will 
        //  have a sign ('+' or '-', zeros will have a space instead of a sign)
        // 
        //      n     The floating point number to output. This can be used to 
        //            adjust the required behavior depending on the value of 
        //            this number.
        ///////////////////////////////////////////////////////////////////////
        static bool force_sign(T)
        {
            return false;
        }

        ///////////////////////////////////////////////////////////////////////
        //  Return whether trailing zero digits have to be emitted in the 
        //  fractional part of the output. If set, this flag instructs the 
        //  floating point generator to emit trailing zeros up to the required 
        //  precision digits (as returned by the precision() function).
        // 
        //      n     The floating point number to output. This can be used to 
        //            adjust the required behavior depending on the value of 
        //            this number.
        ///////////////////////////////////////////////////////////////////////
        static bool trailing_zeros(T)
        {
            // the default behavior is not to generate trailing zeros
            return false;
        }

        ///////////////////////////////////////////////////////////////////////
        //  Decide, which representation type to use in the generated output.
        //
        //  By default all numbers having an absolute value of zero or in 
        //  between 0.001 and 100000 will be generated using the fixed format, 
        //  all others will be generated using the scientific representation.
        //
        //  The function trailing_zeros() can be used to force the output of 
        //  trailing zeros in the fractional part up to the number of digits 
        //  returned by the precision() member function. The default is not to 
        //  generate the trailing zeros.
        //  
        //      n     The floating point number to output. This can be used to 
        //            adjust the formatting flags depending on the value of 
        //            this number.
        ///////////////////////////////////////////////////////////////////////
        static int floatfield(T n)
        {
            if (traits::test_zero(n))
                return fmtflags::fixed;

            T abs_n = traits::get_absolute_value(n);
            return (abs_n >= 1e5 || abs_n < 1e-3) 
              ? fmtflags::scientific : fmtflags::fixed;
        }

        ///////////////////////////////////////////////////////////////////////
        //  Return the maximum number of decimal digits to generate in the 
        //  fractional part of the output.
        //  
        //      n     The floating point number to output. This can be used to 
        //            adjust the required precision depending on the value of 
        //            this number. If the trailing zeros flag is specified the
        //            fractional part of the output will be 'filled' with 
        //            zeros, if appropriate
        //
        //  Note:     If the trailing_zeros flag is not in effect additional
        //            comments apply. See the comment for the fraction_part()
        //            function below.
        ///////////////////////////////////////////////////////////////////////
        static unsigned precision(T)
        {
            // by default, generate max. 3 fractional digits
            return 3;
        }

        ///////////////////////////////////////////////////////////////////////
        //  Generate the integer part of the number.
        //
        //      sink       The output iterator to use for generation
        //      n          The absolute value of the integer part of the floating
        //                 point number to convert (always non-negative).
        //      sign       The sign of the overall floating point number to
        //                 convert.
        //      force_sign Whether a sign has to be generated even for
        //                 non-negative numbers. Note, that force_sign will be
        //                 set to false for zero floating point values.
        ///////////////////////////////////////////////////////////////////////
        template <typename OutputIterator>
        static bool integer_part (OutputIterator& sink, T n, bool sign
          , bool force_sign)
        {
            return sign_inserter::call(
                      sink, traits::test_zero(n), sign, force_sign, force_sign) &&
                   int_inserter<10>::call(sink, n);
        }

        ///////////////////////////////////////////////////////////////////////
        //  Generate the decimal point.
        //
        //      sink  The output iterator to use for generation
        //      n     The fractional part of the floating point number to 
        //            convert. Note that this number is scaled such, that 
        //            it represents the number of units which correspond
        //            to the value returned from the precision() function 
        //            earlier. I.e. a fractional part of 0.01234 is
        //            represented as 1234 when the 'Precision' is 5.
        //      precision   The number of digits to emit as returned by the 
        //                  function 'precision()' above
        //
        //            This is given to allow to decide, whether a decimal point
        //            has to be generated at all.
        //
        //  Note:     If the trailing_zeros flag is not in effect additional
        //            comments apply. See the comment for the fraction_part()
        //            function below.
        ///////////////////////////////////////////////////////////////////////
        template <typename OutputIterator>
        static bool dot (OutputIterator& sink, T /*n*/, unsigned /*precision*/)
        {
            return char_inserter<>::call(sink, '.');  // generate the dot by default 
        }

        ///////////////////////////////////////////////////////////////////////
        //  Generate the fractional part of the number.
        //
        //      sink  The output iterator to use for generation
        //      n     The fractional part of the floating point number to 
        //            convert. This number is scaled such, that it represents 
        //            the number of units which correspond to the 'Precision'. 
        //            I.e. a fractional part of 0.01234 is represented as 1234 
        //            when the 'precision_' parameter is 5.
        //      precision_  The corrected number of digits to emit (see note 
        //                  below)
        //      precision   The number of digits to emit as returned by the 
        //                  function 'precision()' above
        //
        //  Note: If trailing_zeros() does not return true the 'precision_' 
        //        parameter will have been corrected from the value the 
        //        precision() function returned earlier (defining the maximal 
        //        number of fractional digits) in the sense, that it takes into 
        //        account trailing zeros. I.e. a floating point number 0.0123 
        //        and a value of 5 returned from precision() will result in:
        //
        //        trailing_zeros is not specified:
        //            n           123
        //            precision_  4
        //
        //        trailing_zeros is specified:
        //            n           1230
        //            precision_  5
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename OutputIterator>
        static bool fraction_part (OutputIterator& sink, T n
          , unsigned precision_, unsigned precision)
        {
            // allow for ADL to find the correct overload for floor and log10
            using namespace std;

            // The following is equivalent to:
            //    generate(sink, right_align(precision, '0')[ulong], n);
            // but it's spelled out to avoid inter-modular dependencies.

            typename remove_const<T>::type digits = 
                (traits::test_zero(n) ? 0 : floor(log10(n))) + 1;
            bool r = true;
            for (/**/; r && digits < precision_; digits = digits + 1)
                r = char_inserter<>::call(sink, '0');
            if (precision && r)
                r = int_inserter<10>::call(sink, n);
            return r;
        }

        ///////////////////////////////////////////////////////////////////////
        //  Generate the exponential part of the number (this is called only 
        //  if the floatfield() function returned the 'scientific' flag).
        //
        //      sink  The output iterator to use for generation
        //      n     The (signed) exponential part of the floating point 
        //            number to convert. 
        //
        //  The Tag template parameter is either of the type unused_type or
        //  describes the character class and conversion to be applied to any 
        //  output possibly influenced by either the lower[...] or upper[...] 
        //  directives.
        ///////////////////////////////////////////////////////////////////////
        template <typename CharEncoding, typename Tag, typename OutputIterator>
        static bool exponent (OutputIterator& sink, long n)
        {
            long abs_n = traits::get_absolute_value(n);
            bool r = char_inserter<CharEncoding, Tag>::call(sink, 'e') &&
                     sign_inserter::call(sink, traits::test_zero(n)
                        , traits::test_negative(n), false);

            // the C99 Standard requires at least two digits in the exponent
            if (r && abs_n < 10)
                r = char_inserter<CharEncoding, Tag>::call(sink, '0');
            return r && int_inserter<10>::call(sink, abs_n);
        }

        ///////////////////////////////////////////////////////////////////////
        //  Print the textual representations for non-normal floats (NaN and 
        //  Inf)
        //
        //      sink       The output iterator to use for generation
        //      n          The (signed) floating point number to convert. 
        //      force_sign Whether a sign has to be generated even for 
        //                 non-negative numbers
        //
        //  The Tag template parameter is either of the type unused_type or
        //  describes the character class and conversion to be applied to any 
        //  output possibly influenced by either the lower[...] or upper[...] 
        //  directives.
        //
        //  Note: These functions get called only if fpclassify() returned 
        //        FP_INFINITY or FP_NAN.
        ///////////////////////////////////////////////////////////////////////
        template <typename CharEncoding, typename Tag, typename OutputIterator>
        static bool nan (OutputIterator& sink, T n, bool force_sign)
        {
            return sign_inserter::call(
                        sink, false, traits::test_negative(n), force_sign) &&
                   string_inserter<CharEncoding, Tag>::call(sink, "nan");
        }

        template <typename CharEncoding, typename Tag, typename OutputIterator>
        static bool inf (OutputIterator& sink, T n, bool force_sign)
        {
            return sign_inserter::call(
                        sink, false, traits::test_negative(n), force_sign) &&
                   string_inserter<CharEncoding, Tag>::call(sink, "inf");
        }
    };
}}}

#endif // defined(BOOST_SPIRIT_KARMA_REAL_POLICIES_MAR_02_2007_0936AM)
