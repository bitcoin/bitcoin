/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_NUMERICS_IPP
#define BOOST_SPIRIT_NUMERICS_IPP

#include <boost/config/no_tr1/cmath.hpp>
#include <limits>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    struct sign_parser; // forward declaration only

    namespace impl
    {
        ///////////////////////////////////////////////////////////////////////
        //
        //  Extract the prefix sign (- or +)
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename ScannerT>
        bool
        extract_sign(ScannerT const& scan, std::size_t& count)
        {
            //  Extract the sign
            count = 0;
            bool neg = *scan == '-';
            if (neg || (*scan == '+'))
            {
                ++scan;
                ++count;
                return neg;
            }

            return false;
        }

        ///////////////////////////////////////////////////////////////////////
        //
        //  Traits class for radix specific number conversion
        //
        //      Convert a digit from character representation, ch, to binary
        //      representation, returned in val.
        //      Returns whether the conversion was successful.
        //
        //        template<typename CharT> static bool digit(CharT ch, T& val);
        //
        ///////////////////////////////////////////////////////////////////////
        template<const int Radix>
        struct radix_traits;

        ////////////////////////////////// Binary
        template<>
        struct radix_traits<2>
        {
            template<typename CharT, typename T>
            static bool digit(CharT ch, T& val)
            {
                val = ch - '0';
                return ('0' == ch || '1' == ch);
            }
        };

        ////////////////////////////////// Octal
        template<>
        struct radix_traits<8>
        {
            template<typename CharT, typename T>
            static bool digit(CharT ch, T& val)
            {
                val = ch - '0';
                return ('0' <= ch && ch <= '7');
            }
        };

        ////////////////////////////////// Decimal
        template<>
        struct radix_traits<10>
        {
            template<typename CharT, typename T>
            static bool digit(CharT ch, T& val)
            {
                val = ch - '0';
                return impl::isdigit_(ch);
            }
        };

        ////////////////////////////////// Hexadecimal
        template<>
        struct radix_traits<16>
        {
            template<typename CharT, typename T>
            static bool digit(CharT ch, T& val)
            {
                if (radix_traits<10>::digit(ch, val))
                    return true;

                CharT lc = impl::tolower_(ch);
                if ('a' <= lc && lc <= 'f')
                {
                    val = lc - 'a' + 10;
                    return true;
                }
                return false;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //      Helper templates for encapsulation of radix specific
        //      conversion of an input string to an integral value.
        //
        //      main entry point:
        //
        //          extract_int<Radix, MinDigits, MaxDigits, Accumulate>
        //              ::f(first, last, n, count);
        //
        //          The template parameter Radix represents the radix of the
        //          number contained in the parsed string. The template
        //          parameter MinDigits specifies the minimum digits to
        //          accept. The template parameter MaxDigits specifies the
        //          maximum digits to parse. A -1 value for MaxDigits will
        //          make it parse an arbitrarilly large number as long as the
        //          numeric type can hold it. Accumulate is either
        //          positive_accumulate<Radix> (default) for parsing positive
        //          numbers or negative_accumulate<Radix> otherwise.
        //          Checking is only performed when std::numeric_limits<T>::
        //          is_specialized is true. Otherwise, there's no way to
        //          do the check.
        //
        //          scan.first and scan.last are iterators as usual (i.e.
        //          first is mutable and is moved forward when a match is
        //          found), n is a variable that holds the number (passed by
        //          reference). The number of parsed characters is added to
        //          count (also passed by reference)
        //
        //      NOTE:
        //              Returns a non-match, if the number to parse
        //              overflows (or underflows) the used type.
        //
        //      BEWARE:
        //              the parameters 'n' and 'count' should be properly
        //              initialized before calling this function.
        //
        ///////////////////////////////////////////////////////////////////////
#if defined(BOOST_MSVC)
#pragma warning(push) 
#pragma warning(disable:4127) //conditional expression is constant
#endif
        
        template <typename T, int Radix>
        struct positive_accumulate
        {
            //  Use this accumulator if number is positive
            static bool add(T& n, T digit)
            {
                if (std::numeric_limits<T>::is_specialized)
                {
                    static T const max = (std::numeric_limits<T>::max)();
                    static T const max_div_radix = max/Radix;

                    if (n > max_div_radix)
                        return false;
                    n *= Radix;

                    if (n > max - digit)
                        return false;
                    n += digit;

                    return true;
                }
                else
                {
                    n *= Radix;
                    n += digit;
                    return true;
                }
            }
        };

        template <typename T, int Radix>
        struct negative_accumulate
        {
            //  Use this accumulator if number is negative
            static bool add(T& n, T digit)
            {
                if (std::numeric_limits<T>::is_specialized)
                {
                    typedef std::numeric_limits<T> num_limits;
                    static T const min =
                        (!num_limits::is_integer && num_limits::is_signed && num_limits::has_denorm) ?
                        -(num_limits::max)() : (num_limits::min)();
                    static T const min_div_radix = min/Radix;

                    if (n < min_div_radix)
                        return false;
                    n *= Radix;

                    if (n < min + digit)
                        return false;
                    n -= digit;

                    return true;
                }
                else
                {
                    n *= Radix;
                    n -= digit;
                    return true;
                }
            }
        };

        template <int MaxDigits>
        inline bool allow_more_digits(std::size_t i)
        {
            return i < MaxDigits;
        }

        template <>
        inline bool allow_more_digits<-1>(std::size_t)
        {
            return true;
        }

        //////////////////////////////////
        template <
            int Radix, unsigned MinDigits, int MaxDigits,
            typename Accumulate
        >
        struct extract_int
        {
            template <typename ScannerT, typename T>
            static bool
            f(ScannerT& scan, T& n, std::size_t& count)
            {
                std::size_t i = 0;
                T digit;
                while( allow_more_digits<MaxDigits>(i) && !scan.at_end() &&
                    radix_traits<Radix>::digit(*scan, digit) )
                {
                    if (!Accumulate::add(n, digit))
                        return false; // Overflow
                    ++i, ++scan, ++count;
                }
                return i >= MinDigits;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  uint_parser_impl class
        //
        ///////////////////////////////////////////////////////////////////////
        template <
            typename T = unsigned,
            int Radix = 10,
            unsigned MinDigits = 1,
            int MaxDigits = -1
        >
        struct uint_parser_impl
            : parser<uint_parser_impl<T, Radix, MinDigits, MaxDigits> >
        {
            typedef uint_parser_impl<T, Radix, MinDigits, MaxDigits> self_t;

            template <typename ScannerT>
            struct result
            {
                typedef typename match_result<ScannerT, T>::type type;
            };

            template <typename ScannerT>
            typename parser_result<self_t, ScannerT>::type
            parse(ScannerT const& scan) const
            {
                if (!scan.at_end())
                {
                    T n = 0;
                    std::size_t count = 0;
                    typename ScannerT::iterator_t save = scan.first;
                    if (extract_int<Radix, MinDigits, MaxDigits,
                        positive_accumulate<T, Radix> >::f(scan, n, count))
                    {
                        return scan.create_match(count, n, save, scan.first);
                    }
                    // return no-match if number overflows
                }
                return scan.no_match();
            }
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  int_parser_impl class
        //
        ///////////////////////////////////////////////////////////////////////
        template <
            typename T = unsigned,
            int Radix = 10,
            unsigned MinDigits = 1,
            int MaxDigits = -1
        >
        struct int_parser_impl
            : parser<int_parser_impl<T, Radix, MinDigits, MaxDigits> >
        {
            typedef int_parser_impl<T, Radix, MinDigits, MaxDigits> self_t;

            template <typename ScannerT>
            struct result
            {
                typedef typename match_result<ScannerT, T>::type type;
            };

            template <typename ScannerT>
            typename parser_result<self_t, ScannerT>::type
            parse(ScannerT const& scan) const
            {
                typedef extract_int<Radix, MinDigits, MaxDigits,
                    negative_accumulate<T, Radix> > extract_int_neg_t;
                typedef extract_int<Radix, MinDigits, MaxDigits,
                    positive_accumulate<T, Radix> > extract_int_pos_t;

                if (!scan.at_end())
                {
                    T n = 0;
                    std::size_t count = 0;
                    typename ScannerT::iterator_t save = scan.first;

                    bool hit = impl::extract_sign(scan, count);

                    if (hit)
                        hit = extract_int_neg_t::f(scan, n, count);
                    else
                        hit = extract_int_pos_t::f(scan, n, count);

                    if (hit)
                        return scan.create_match(count, n, save, scan.first);
                    else
                        scan.first = save;
                    // return no-match if number overflows or underflows
                }
                return scan.no_match();
            }
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  real_parser_impl class
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename RT, typename T, typename RealPoliciesT>
        struct real_parser_impl
        {
            typedef real_parser_impl<RT, T, RealPoliciesT> self_t;

            template <typename ScannerT>
            RT parse_main(ScannerT const& scan) const
            {
                if (scan.at_end())
                    return scan.no_match();
                typename ScannerT::iterator_t save = scan.first;

                typedef typename parser_result<sign_parser, ScannerT>::type
                    sign_match_t;
                typedef typename parser_result<chlit<>, ScannerT>::type
                    exp_match_t;

                sign_match_t    sign_match = RealPoliciesT::parse_sign(scan);
                std::size_t     count = sign_match ? sign_match.length() : 0;
                bool            neg = sign_match.has_valid_attribute() ?
                                    sign_match.value() : false;

                RT              n_match = RealPoliciesT::parse_n(scan);
                T               n = n_match.has_valid_attribute() ?
                                    n_match.value() : T(0);
                bool            got_a_number = n_match;
                exp_match_t     e_hit;

                if (!got_a_number && !RealPoliciesT::allow_leading_dot)
                     return scan.no_match();
                else
                    count += n_match.length();

                if (neg)
                    n = -n;

                if (RealPoliciesT::parse_dot(scan))
                {
                    //  We got the decimal point. Now we will try to parse
                    //  the fraction if it is there. If not, it defaults
                    //  to zero (0) only if we already got a number.

                    if (RT hit = RealPoliciesT::parse_frac_n(scan))
                    {
#if !defined(BOOST_NO_STDC_NAMESPACE)
                        using namespace std;  // allow for ADL to find pow()
#endif
                        hit.value(hit.value()
                            * pow(T(10), T(-hit.length())));
                        if (neg)
                            n -= hit.value();
                        else
                            n += hit.value();
                        count += hit.length() + 1;

                    }

                    else if (!got_a_number ||
                        !RealPoliciesT::allow_trailing_dot)
                        return scan.no_match();

                    e_hit = RealPoliciesT::parse_exp(scan);
                }
                else
                {
                    //  We have reached a point where we
                    //  still haven't seen a number at all.
                    //  We return early with a no-match.
                    if (!got_a_number)
                        return scan.no_match();

                    //  If we must expect a dot and we didn't see
                    //  an exponent, return early with a no-match.
                    e_hit = RealPoliciesT::parse_exp(scan);
                    if (RealPoliciesT::expect_dot && !e_hit)
                        return scan.no_match();
                }

                if (e_hit)
                {
                    //  We got the exponent prefix. Now we will try to parse the
                    //  actual exponent. It is an error if it is not there.
                    if (RT e_n_hit = RealPoliciesT::parse_exp_n(scan))
                    {
#if !defined(BOOST_NO_STDC_NAMESPACE)
                        using namespace std;    // allow for ADL to find pow()
#endif
                        n *= pow(T(10), T(e_n_hit.value()));
                        count += e_n_hit.length() + e_hit.length();
                    }
                    else
                    {
                        //  Oops, no exponent, return a no-match
                        return scan.no_match();
                    }
                }

                return scan.create_match(count, n, save, scan.first);
            }

            template <typename ScannerT>
            static RT parse(ScannerT const& scan)
            {
                static self_t this_;
                return impl::implicit_lexeme_parse<RT>(this_, scan, scan);
            }
        };

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

    }   //  namespace impl

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif
