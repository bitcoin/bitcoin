/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_ALTERNATIVE_DETAIL_JAN_07_2013_1245PM)
#define BOOST_SPIRIT_X3_ALTERNATIVE_DETAIL_JAN_07_2013_1245PM

#include <boost/spirit/home/x3/support/traits/attribute_of.hpp>
#include <boost/spirit/home/x3/support/traits/pseudo_attribute.hpp>
#include <boost/spirit/home/x3/support/traits/is_variant.hpp>
#include <boost/spirit/home/x3/support/traits/tuple_traits.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <boost/spirit/home/x3/support/traits/variant_has_substitute.hpp>
#include <boost/spirit/home/x3/support/traits/variant_find_substitute.hpp>
#include <boost/spirit/home/x3/core/detail/parse_into_container.hpp>

#include <boost/mpl/if.hpp>

#include <boost/fusion/include/front.hpp>

#include <boost/type_traits/is_same.hpp>
#include <type_traits>

namespace boost { namespace spirit { namespace x3
{
    template <typename Left, typename Right>
    struct alternative;
}}}

namespace boost { namespace spirit { namespace x3 { namespace detail
{
    struct pass_variant_unused
    {
        typedef unused_type type;

        template <typename T>
        static unused_type
        call(T&)
        {
            return unused_type();
        }
    };

    template <typename Attribute>
    struct pass_variant_used
    {
        typedef Attribute& type;

        static Attribute&
        call(Attribute& v)
        {
            return v;
        }
    };

    template <>
    struct pass_variant_used<unused_type> : pass_variant_unused {};

    template <typename Parser, typename Attribute, typename Context
      , typename Enable = void>
    struct pass_parser_attribute
    {
        typedef typename
            traits::attribute_of<Parser, Context>::type
        attribute_type;
        typedef typename
            traits::variant_find_substitute<Attribute, attribute_type>::type
        substitute_type;

        typedef typename
            mpl::if_<
                is_same<Attribute, substitute_type>
              , Attribute&
              , substitute_type
            >::type
        type;

        template <typename Attribute_>
        static Attribute_&
        call(Attribute_& attribute, mpl::true_)
        {
            return attribute;
        }

        template <typename Attribute_>
        static type
        call(Attribute_&, mpl::false_)
        {
            return type();
        }

        template <typename Attribute_>
        static type
        call(Attribute_& attribute)
        {
            return call(attribute, is_same<Attribute_, typename remove_reference<type>::type>());
        }
    };

    // Pass non-variant attributes as-is
    template <typename Parser, typename Attribute, typename Context
      , typename Enable = void>
    struct pass_non_variant_attribute
    {
        typedef Attribute& type;

        static Attribute&
        call(Attribute& attribute)
        {
            return attribute;
        }
    };

    // Unwrap single element sequences
    template <typename Parser, typename Attribute, typename Context>
    struct pass_non_variant_attribute<Parser, Attribute, Context,
        typename enable_if<traits::is_size_one_sequence<Attribute>>::type>
    {
        typedef typename remove_reference<
            typename fusion::result_of::front<Attribute>::type>::type
        attr_type;

        typedef pass_parser_attribute<Parser, attr_type, Context> pass;
        typedef typename pass::type type;

        template <typename Attribute_>
        static type
        call(Attribute_& attr)
        {
            return pass::call(fusion::front(attr));
        }
    };

    template <typename Parser, typename Attribute, typename Context>
    struct pass_parser_attribute<Parser, Attribute, Context,
        typename enable_if_c<(!traits::is_variant<Attribute>::value)>::type>
        : pass_non_variant_attribute<Parser, Attribute, Context>
    {};

    template <typename Parser, typename Context>
    struct pass_parser_attribute<Parser, unused_type, Context>
        : pass_variant_unused {};

    template <typename Parser, typename Attribute, typename Context>
    struct pass_variant_attribute :
        mpl::if_c<traits::has_attribute<Parser, Context>::value
          , pass_parser_attribute<Parser, Attribute, Context>
          , pass_variant_unused>::type
    {
    };

    template <typename L, typename R, typename Attribute, typename Context>
    struct pass_variant_attribute<alternative<L, R>, Attribute, Context> :
        mpl::if_c<traits::has_attribute<alternative<L, R>, Context>::value
          , pass_variant_used<Attribute>
          , pass_variant_unused>::type
    {
    };

    template <bool Condition>
    struct move_if
    {
        template<typename T1, typename T2>
        static void call(T1& /* attr_ */, T2& /* attr */) {}
    };

    template <>
    struct move_if<true>
    {
        template<typename T1, typename T2>
        static void call(T1& attr_, T2& attribute)
        {
            traits::move_to(attr_, attribute);
        }
    };

    template <typename Parser, typename Iterator, typename Context
      , typename RContext, typename Attribute>
    bool parse_alternative(Parser const& p, Iterator& first, Iterator const& last
      , Context const& context, RContext& rcontext, Attribute& attribute)
    {
        using pass = detail::pass_variant_attribute<Parser, Attribute, Context>;
        using pseudo = traits::pseudo_attribute<Context, typename pass::type, Iterator>;

        typename pseudo::type attr_ = pseudo::call(first, last, pass::call(attribute));

        if (p.parse(first, last, context, rcontext, attr_))
        {
            move_if<!std::is_reference<decltype(attr_)>::value>::call(attr_, attribute);
            return true;
        }
        return false;
    }

    template <typename Subject>
    struct alternative_helper : unary_parser<Subject, alternative_helper<Subject>>
    {
        static bool const is_pass_through_unary = true;

        using unary_parser<Subject, alternative_helper<Subject>>::unary_parser;

        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            return detail::parse_alternative(this->subject, first, last, context, rcontext, attr);
        }
    };

    template <typename Left, typename Right, typename Context, typename RContext>
    struct parse_into_container_impl<alternative<Left, Right>, Context, RContext>
    {
        typedef alternative<Left, Right> parser_type;

        template <typename Iterator, typename Attribute>
        static bool call(
            parser_type const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attribute, mpl::false_)
        {
            return detail::parse_into_container(parser.left, first, last, context, rcontext, attribute)
                || detail::parse_into_container(parser.right, first, last, context, rcontext, attribute);
        }

        template <typename Iterator, typename Attribute>
        static bool call(
            parser_type const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attribute, mpl::true_)
        {
            return detail::parse_into_container(alternative_helper<Left>{parser.left}, first, last, context, rcontext, attribute)
                || detail::parse_into_container(alternative_helper<Right>{parser.right}, first, last, context, rcontext, attribute);
        }

        template <typename Iterator, typename Attribute>
        static bool call(
            parser_type const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attribute)
        {
            return call(parser, first, last, context, rcontext, attribute,
                typename traits::is_variant<typename traits::container_value<Attribute>::type>::type{});
        }
    };

}}}}

#endif
