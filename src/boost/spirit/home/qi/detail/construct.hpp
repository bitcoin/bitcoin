/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONSTRUCT_MAR_24_2007_0629PM)
#define BOOST_SPIRIT_CONSTRUCT_MAR_24_2007_0629PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/spirit/home/qi/parse.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/attributes_fwd.hpp>

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    //  We provide overloads for the assign_to_attribute_from_iterators
    //  customization point for all built in types
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct assign_to_attribute_from_iterators<char, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const&, char& attr)
        {
            attr = *first;
        }
    };

    template <typename Iterator>
    struct assign_to_attribute_from_iterators<signed char, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const&, signed char& attr)
        {
            attr = *first;
        }
    };

    template <typename Iterator>
    struct assign_to_attribute_from_iterators<unsigned char, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const&, unsigned char& attr)
        {
            attr = *first;
        }
    };

    // wchar_t is intrinsic
    template <typename Iterator>
    struct assign_to_attribute_from_iterators<wchar_t, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const&, wchar_t& attr)
        {
            attr = *first;
        }
    };

#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
    // wchar_t is intrinsic, have separate overload for unsigned short
    template <typename Iterator>
    struct assign_to_attribute_from_iterators<unsigned short, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const&, unsigned short& attr)
        {
            attr = *first;
        }
    };
#endif

    template <typename Iterator>
    struct assign_to_attribute_from_iterators<bool, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, bool& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, bool_type(), attr);
        }
    };

    template <typename Iterator>
    struct assign_to_attribute_from_iterators<short, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, short& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, short_type(), attr);
        }
    };

    template <typename Iterator>
    struct assign_to_attribute_from_iterators<int, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, int& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, int_type(), attr);
        }
    };
    template <typename Iterator>
    struct assign_to_attribute_from_iterators<unsigned int, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, unsigned int& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, uint_type(), attr);
        }
    };

    template <typename Iterator>
    struct assign_to_attribute_from_iterators<long, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, long& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, long_type(), attr);
        }
    };
    template <typename Iterator>
    struct assign_to_attribute_from_iterators<unsigned long, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, unsigned long& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, ulong_type(), attr);
        }
    };

#ifdef BOOST_HAS_LONG_LONG
    template <typename Iterator>
    struct assign_to_attribute_from_iterators<boost::long_long_type, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, boost::long_long_type& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, long_long_type(), attr);
        }
    };
    template <typename Iterator>
    struct assign_to_attribute_from_iterators<boost::ulong_long_type, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, boost::ulong_long_type& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, ulong_long_type(), attr);
        }
    };
#endif

    template <typename Iterator>
    struct assign_to_attribute_from_iterators<float, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, float& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, float_type(), attr);
        }
    };

    template <typename Iterator>
    struct assign_to_attribute_from_iterators<double, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, double& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, double_type(), attr);
        }
    };

    template <typename Iterator>
    struct assign_to_attribute_from_iterators<long double, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last, long double& attr)
        {
            Iterator first_ = first;
            qi::parse(first_, last, long_double_type(), attr);
        }
    };

}}}

#endif
