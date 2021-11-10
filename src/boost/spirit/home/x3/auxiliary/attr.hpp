/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2013 Agustin Berge

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_X3_ATTR_JUL_23_2008_0956AM
#define BOOST_SPIRIT_X3_ATTR_JUL_23_2008_0956AM

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/support/traits/container_traits.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <cstddef>
#include <string>
#include <utility>

namespace boost { namespace spirit { namespace x3
{

namespace detail
{
    template <typename Value, std::size_t N
      , typename = std::make_index_sequence<N>>
    struct array_helper;

    template <typename Value, std::size_t N, std::size_t... Is>
    struct array_helper<Value, N, std::index_sequence<Is...>>
    {
        constexpr array_helper(Value const (&value)[N])
            : value_{ value[Is]... } {}

        constexpr array_helper(Value (&&value)[N])
            : value_{ static_cast<Value&&>(value[Is])... } {}

        // silence MSVC warning C4512: assignment operator could not be generated
        array_helper& operator= (array_helper const&) = delete;

        Value value_[N];
    };
}

    template <typename Value>
    struct attr_parser : parser<attr_parser<Value>>
    {
        typedef Value attribute_type;

        static bool const has_attribute =
            !is_same<unused_type, attribute_type>::value;
        static bool const handles_container =
            traits::is_container<attribute_type>::value;
        
        constexpr attr_parser(Value const& value)
          : value_(value) {}
        constexpr attr_parser(Value&& value)
          : value_(std::move(value)) {}

        template <typename Iterator, typename Context
          , typename RuleContext, typename Attribute>
        bool parse(Iterator& /* first */, Iterator const& /* last */
          , Context const& /* context */, RuleContext&, Attribute& attr_) const
        {
            // $$$ Change to copy_to once we have it $$$
            traits::move_to(value_, attr_);
            return true;
        }

        Value value_;

        // silence MSVC warning C4512: assignment operator could not be generated
        attr_parser& operator= (attr_parser const&) = delete;
    };
    
    template <typename Value, std::size_t N>
    struct attr_parser<Value[N]> : parser<attr_parser<Value[N]>>
      , detail::array_helper<Value, N>
    {
        using detail::array_helper<Value, N>::array_helper;

        typedef Value attribute_type[N];

        static bool const has_attribute =
            !is_same<unused_type, attribute_type>::value;
        static bool const handles_container = true;

        template <typename Iterator, typename Context
          , typename RuleContext, typename Attribute>
        bool parse(Iterator& /* first */, Iterator const& /* last */
          , Context const& /* context */, RuleContext&, Attribute& attr_) const
        {
            // $$$ Change to copy_to once we have it $$$
            traits::move_to(this->value_ + 0, this->value_ + N, attr_);
            return true;
        }

        // silence MSVC warning C4512: assignment operator could not be generated
        attr_parser& operator= (attr_parser const&) = delete;
    };
    
    template <typename Value>
    struct get_info<attr_parser<Value>>
    {
        typedef std::string result_type;
        std::string operator()(attr_parser<Value> const& /*p*/) const
        {
            return "attr";
        }
    };

    struct attr_gen
    {
        template <typename Value>
        constexpr attr_parser<typename remove_cv<
            typename remove_reference<Value>::type>::type>
        operator()(Value&& value) const
        {
            return { std::forward<Value>(value) };
        }
    };

    constexpr auto attr = attr_gen{};
}}}

#endif
