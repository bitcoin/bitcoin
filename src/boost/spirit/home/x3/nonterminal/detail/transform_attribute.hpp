/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_X3_NONTERMINAL_DETAIL_TRANSFORM_ATTRIBUTE_HPP
#define BOOST_SPIRIT_X3_NONTERMINAL_DETAIL_TRANSFORM_ATTRIBUTE_HPP

#include <boost/spirit/home/x3/support/traits/transform_attribute.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace x3
{
    struct parser_id;

    template <typename Exposed, typename Transformed>
    struct default_transform_attribute
    {
        typedef Transformed type;

        static Transformed pre(Exposed&) { return Transformed(); }

        static void post(Exposed& val, Transformed&& attribute)
        {
            traits::move_to(std::forward<Transformed>(attribute), val);
        }
    };

    // handle case where no transformation is required as the types are the same
    template <typename Attribute>
    struct default_transform_attribute<Attribute, Attribute>
    {
        typedef Attribute& type;
        static Attribute& pre(Attribute& val) { return val; }
        static void post(Attribute&, Attribute const&) {}
    };

    // main specialization for x3
    template <typename Exposed, typename Transformed, typename Enable = void>
    struct transform_attribute
      : default_transform_attribute<Exposed, Transformed> {};

    // unused_type needs some special handling as well
    template <>
    struct transform_attribute<unused_type, unused_type>
    {
        typedef unused_type type;
        static unused_type pre(unused_type) { return unused; }
        static void post(unused_type, unused_type) {}
    };

    template <>
    struct transform_attribute<unused_type const, unused_type>
      : transform_attribute<unused_type, unused_type> {};

    template <typename Attribute>
    struct transform_attribute<unused_type, Attribute>
      : transform_attribute<unused_type, unused_type> {};

    template <typename Attribute>
    struct transform_attribute<unused_type const, Attribute>
      : transform_attribute<unused_type, unused_type> {};

    template <typename Attribute>
    struct transform_attribute<Attribute, unused_type>
      : transform_attribute<unused_type, unused_type> {};

    template <typename Attribute>
    struct transform_attribute<Attribute const, unused_type>
      : transform_attribute<unused_type, unused_type> {};
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Exposed, typename Transformed>
    struct transform_attribute<Exposed, Transformed, x3::parser_id>
      : x3::transform_attribute<Exposed, Transformed>
    {
        static_assert(!std::is_reference<Exposed>::value,
            "Exposed cannot be a reference type");
        static_assert(!std::is_reference<Transformed>::value,
            "Transformed cannot be a reference type");
    };
}}}}

#endif
