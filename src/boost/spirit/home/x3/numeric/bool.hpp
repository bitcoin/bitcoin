/*=============================================================================
    Copyright (c) 2009  Hartmut Kaiser
    Copyright (c) 2014  Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_X3_NUMERIC_BOOL_HPP
#define BOOST_SPIRIT_X3_NUMERIC_BOOL_HPP

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/numeric/bool_policies.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename T, typename Encoding, typename BoolPolicies = bool_policies<T>>
    struct bool_parser : parser<bool_parser<T, Encoding, BoolPolicies>>
    {
        typedef Encoding encoding;
        typedef T attribute_type;
        static bool const has_attribute = true;

        constexpr bool_parser()
        	: policies() {}

        constexpr bool_parser(BoolPolicies const& policies)
        	: policies(policies) {}

        template <typename Iterator, typename Context>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, T& attr) const
        {
            x3::skip_over(first, last, context);
            return policies.parse_true(first, last, attr, get_case_compare<encoding>(context))
                || policies.parse_false(first, last, attr, get_case_compare<encoding>(context));
        }

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute& attr_param) const
        {
            // this case is called when Attribute is not T
            T attr_;
            if (parse(first, last, context, unused, attr_))
            {
                traits::move_to(attr_, attr_param);
                return true;
            }
            return false;
        }

        BoolPolicies policies;
    };

    template <typename T, typename Encoding, typename BoolPolicies = bool_policies<T>>
    struct literal_bool_parser : parser<bool_parser<T, Encoding, BoolPolicies>>
    {
        typedef Encoding encoding;
        typedef T attribute_type;
        static bool const has_attribute = true;

        template <typename Value>
        constexpr literal_bool_parser(Value const& n)
        	: policies(), n_(n) {}

        template <typename Value>
        constexpr literal_bool_parser(Value const& n, BoolPolicies const& policies)
        	: policies(policies), n_(n) {}

        template <typename Iterator, typename Context>
        bool parse_main(Iterator& first, Iterator const& last
          , Context const& context, T& attr) const
        {
            x3::skip_over(first, last, context);
            return (n_ && policies.parse_true(first, last, attr, get_case_compare<encoding>(context)))
                || (!n_ && policies.parse_false(first, last, attr, get_case_compare<encoding>(context)));
        }

        template <typename Iterator, typename Context>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, T& attr) const
        {
            return parse_main(first, last, context, attr);
        }

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute& attr_param) const
        {
            // this case is called when Attribute is not T
            T attr_;
            if (parse_main(first, last, context, attr_))
            {
                traits::move_to(attr_, attr_param);
                return true;
            }
            return false;
        }

        BoolPolicies policies;
        T n_;
    };

    namespace standard
    {
        typedef bool_parser<bool, char_encoding::standard> bool_type;
        constexpr bool_type bool_ = {};

        typedef literal_bool_parser<bool, char_encoding::standard> true_type;
        constexpr true_type true_ = { true };

        typedef literal_bool_parser<bool, char_encoding::standard> false_type;
        constexpr false_type false_ = { false };
    }

#ifndef BOOST_SPIRIT_NO_STANDARD_WIDE
    namespace standard_wide
    {
        typedef bool_parser<bool, char_encoding::standard_wide> bool_type;
        constexpr bool_type bool_ = {};

        typedef literal_bool_parser<bool, char_encoding::standard_wide> true_type;
        constexpr true_type true_ = { true };

        typedef literal_bool_parser<bool, char_encoding::standard_wide> false_type;
        constexpr false_type false_ = { false };
    }
#endif

    namespace ascii
    {
        typedef bool_parser<bool, char_encoding::ascii> bool_type;
        constexpr bool_type bool_ = {};

        typedef literal_bool_parser<bool, char_encoding::ascii> true_type;
        constexpr true_type true_ = { true };

        typedef literal_bool_parser<bool, char_encoding::ascii> false_type;
        constexpr false_type false_ = { false };
    }

    namespace iso8859_1
    {
        typedef bool_parser<bool, char_encoding::iso8859_1> bool_type;
        constexpr bool_type bool_ = {};

        typedef literal_bool_parser<bool, char_encoding::iso8859_1> true_type;
        constexpr true_type true_ = { true };

        typedef literal_bool_parser<bool, char_encoding::iso8859_1> false_type;
        constexpr false_type false_ = { false };
    }

    using standard::bool_;
    using standard::true_;
    using standard::false_;

    }}}

#endif
