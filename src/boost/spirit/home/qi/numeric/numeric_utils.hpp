/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Jan Frederick Eick

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_NUMERIC_UTILS_APRIL_17_2006_0830AM)
#define BOOST_SPIRIT_NUMERIC_UTILS_APRIL_17_2006_0830AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/assert_msg.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include <boost/spirit/home/qi/numeric/detail/numeric_utils.hpp>
#include <boost/assert.hpp>
#include <boost/mpl/assert.hpp>

namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    //  Extract the prefix sign (- or +), return true if a '-' was found
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    inline bool
    extract_sign(Iterator& first, Iterator const& last)
    {
        (void)last;                  // silence unused warnings
        BOOST_ASSERT(first != last); // precondition

        // Extract the sign
        bool neg = *first == '-';
        if (neg || (*first == '+'))
        {
            ++first;
            return neg;
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Low level unsigned integer parser
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, unsigned Radix, unsigned MinDigits, int MaxDigits
      , bool Accumulate = false, bool IgnoreOverflowDigits = false>
    struct extract_uint
    {
        // check template parameter 'Radix' for validity
        BOOST_SPIRIT_ASSERT_MSG(
            Radix >= 2 && Radix <= 36,
            not_supported_radix, ());

        template <typename Iterator>
        inline static bool call(Iterator& first, Iterator const& last, T& attr_)
        {
            if (first == last)
                return false;

            typedef detail::extract_int<
                T
              , Radix
              , MinDigits
              , MaxDigits
              , detail::positive_accumulator<Radix>
              , Accumulate
              , IgnoreOverflowDigits>
            extract_type;

            Iterator save = first;
            if (!extract_type::parse(first, last, attr_))
            {
                first = save;
                return false;
            }
            return true;
        }

        template <typename Iterator, typename Attribute>
        inline static bool call(Iterator& first, Iterator const& last, Attribute& attr_)
        {
            // this case is called when Attribute is not T
            T attr_local;
            if (call(first, last, attr_local))
            {
                traits::assign_to(attr_local, attr_);
                return true;
            }
            return false;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Low level signed integer parser
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, unsigned Radix, unsigned MinDigits, int MaxDigits>
    struct extract_int
    {
        // check template parameter 'Radix' for validity
        BOOST_SPIRIT_ASSERT_MSG(
            Radix == 2 || Radix == 8 || Radix == 10 || Radix == 16,
            not_supported_radix, ());

        template <typename Iterator>
        inline static bool call(Iterator& first, Iterator const& last, T& attr_)
        {
            if (first == last)
                return false;

            typedef detail::extract_int<
                T, Radix, MinDigits, MaxDigits>
            extract_pos_type;

            typedef detail::extract_int<
                T, Radix, MinDigits, MaxDigits, detail::negative_accumulator<Radix> >
            extract_neg_type;

            Iterator save = first;
            bool hit = extract_sign(first, last);
            if (hit)
                hit = extract_neg_type::parse(first, last, attr_);
            else
                hit = extract_pos_type::parse(first, last, attr_);

            if (!hit)
            {
                first = save;
                return false;
            }
            return true;
        }

        template <typename Iterator, typename Attribute>
        inline static bool call(Iterator& first, Iterator const& last, Attribute& attr_)
        {
            // this case is called when Attribute is not T
            T attr_local;
            if (call(first, last, attr_local))
            {
                traits::assign_to(attr_local, attr_);
                return true;
            }
            return false;
        }
    };
}}}

#endif
