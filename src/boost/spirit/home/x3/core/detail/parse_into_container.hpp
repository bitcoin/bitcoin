/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_PARSE_INTO_CONTAINER_JAN_15_2013_0957PM)
#define BOOST_SPIRIT_X3_PARSE_INTO_CONTAINER_JAN_15_2013_0957PM

#include <type_traits>

#include <boost/spirit/home/x3/support/traits/container_traits.hpp>
#include <boost/spirit/home/x3/support/traits/attribute_of.hpp>
#include <boost/spirit/home/x3/support/traits/pseudo_attribute.hpp>
#include <boost/spirit/home/x3/support/traits/handles_container.hpp>
#include <boost/spirit/home/x3/support/traits/has_attribute.hpp>
#include <boost/spirit/home/x3/support/traits/is_substitute.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <boost/mpl/and.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/front.hpp>
#include <boost/fusion/include/back.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <iterator> // for std::make_move_iterator

namespace boost { namespace spirit { namespace x3 { namespace detail
{
    template <typename Attribute, typename Value>
    struct saver_visitor;

    // save to associative fusion container where Key is simple type
    template <typename Key, typename Enable = void>
    struct save_to_assoc_attr
    {
        template <typename Value, typename Attribute>
        static void call(const Key, Value& value, Attribute& attr)
        {
            traits::move_to(value, fusion::at_key<Key>(attr));
        }
    };

    // save to associative fusion container where Key
    // is variant over possible keys
    template <typename ...T>
    struct save_to_assoc_attr<variant<T...> >
    {
        typedef variant<T...> variant_t;

        template <typename Value, typename Attribute>
        static void call(const variant_t key, Value& value, Attribute& attr)
        {
            apply_visitor(saver_visitor<Attribute, Value>(attr, value), key);
        }
    };

    template <typename Attribute, typename Value>
    struct saver_visitor  : boost::static_visitor<void>
    {
        saver_visitor(Attribute& attr, Value& value)
            : attr(attr), value(value) {};

        Attribute& attr;
        Value& value;

        template <typename Key>
        void operator()(Key) const
        {
            save_to_assoc_attr<Key>::call(Key(), value,attr);
        }
    };

    template <typename Parser, typename Container, typename Context>
    struct parser_accepts_container
        : traits::is_substitute<
                typename traits::attribute_of<Parser, Context>::type
              , Container
            >
    {};

    template <typename Parser>
    struct parse_into_container_base_impl
    {
    private:

        // Parser has attribute (synthesize; Attribute is a container)
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        static bool call_synthesize_x(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr, mpl::false_)
        {
            // synthesized attribute needs to be value initialized
            using value_type = typename traits::container_value<Attribute>::type;
            value_type val{};

            if (!parser.parse(first, last, context, rcontext, val))
                return false;

            // push the parsed value into our attribute
            traits::push_back(attr, static_cast<value_type&&>(val));
            return true;
        }

        // Parser has attribute (synthesize; Attribute is a container)
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        static bool call_synthesize_x(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr, mpl::true_)
        {
            return parser.parse(first, last, context, rcontext, attr);
        }

        // Parser has attribute (synthesize; Attribute is a container)
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        static bool call_synthesize(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr)
        {
            typedef
                parser_accepts_container<Parser, Attribute, Context>
            parser_accepts_container;

            return call_synthesize_x(parser, first, last, context, rcontext, attr
                , parser_accepts_container());
        }

        // Parser has attribute (synthesize; Attribute is a single element fusion sequence)
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        static bool call_synthesize_into_fusion_seq(Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr, mpl::false_ /* is_associative */)
        {
            static_assert(traits::has_size<Attribute, 1>::value,
                "Expecting a single element fusion sequence");
            return call_synthesize(parser, first, last, context, rcontext,
                fusion::front(attr));
        }

        // Parser has attribute (synthesize; Attribute is fusion map sequence)
        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call_synthesize_into_fusion_seq(
            Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr, mpl::true_ /*is_associative*/)
        {
            using attribute_type = typename traits::attribute_of<Parser, Context>::type;
            static_assert(traits::has_size<attribute_type, 2>::value,
                "To parse directly into fusion map parser must produce 2 element attr");

            // use type of  first element of attribute as key
            using key = typename std::remove_reference<
            typename fusion::result_of::front<attribute_type>::type>::type;

            attribute_type attr_;
            if (!parser.parse(first, last, context, rcontext, attr_))
                return false;

            save_to_assoc_attr<key>::call(fusion::front(attr_), fusion::back(attr_), attr);
            return true;
        }

        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call_synthesize_dispatch_by_seq(Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr, mpl::true_ /*is_sequence*/)
        {
            return call_synthesize_into_fusion_seq(
                parser, first, last, context, rcontext, attr
              , fusion::traits::is_associative<Attribute>());
        }

        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call_synthesize_dispatch_by_seq(Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr, mpl::false_ /*is_sequence*/)
        {
            return call_synthesize(parser, first, last, context, rcontext, attr);
        }

        // Parser has attribute (synthesize)
        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call(Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr, mpl::true_)
        {
            return call_synthesize_dispatch_by_seq(parser, first, last, context, rcontext, attr
                , fusion::traits::is_sequence<Attribute>());
        }

        // Parser has no attribute (pass unused)
        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call(
            Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& /* attr */, mpl::false_)
        {
            return parser.parse(first, last, context, rcontext, unused);
        }


    public:

        template <typename Iterator, typename Context, typename RContext, typename Attribute>
        static bool call(Parser const& parser
          , Iterator& first, Iterator const& last, Context const& context
          , RContext& rcontext, Attribute& attr)
        {
            return call(parser, first, last, context, rcontext, attr
              , mpl::bool_<traits::has_attribute<Parser, Context>::value>());
        }
    };

    template <typename Parser, typename Context, typename RContext, typename Enable = void>
    struct parse_into_container_impl : parse_into_container_base_impl<Parser> {};

    template <typename Parser, typename Iterator, typename Container, typename Context>
    struct parser_attr_is_substitute_for_container_value
        : traits::is_substitute<
            typename traits::pseudo_attribute<
                Context
              , typename traits::attribute_of<Parser, Context>::type
              , Iterator
            >::type
          , typename traits::container_value<Container>::type
        >
    {};

    template <typename Parser, typename Context, typename RContext>
    struct parse_into_container_impl<Parser, Context, RContext,
        typename enable_if<traits::handles_container<Parser, Context>>::type>
    {
        template <typename Iterator, typename Attribute>
        static bool call(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr, mpl::false_)
        {
            return parse_into_container_base_impl<Parser>::call(
                parser, first, last, context, rcontext, attr);
        }

        template <typename Iterator>
        static bool call(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, unused_type attr, mpl::true_)
        {
            return parser.parse(first, last, context, rcontext, attr);
        }

        template <typename Iterator, typename Attribute>
        static bool call(
            Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr, mpl::true_)
        {
            if (traits::is_empty(attr))
                return parser.parse(first, last, context, rcontext, attr);
            Attribute rest;
            bool r = parser.parse(first, last, context, rcontext, rest);
            if (r)
                traits::append(attr, std::make_move_iterator(rest.begin()),
                                     std::make_move_iterator(rest.end()));
            return r;
        }

        template <typename Iterator, typename Attribute>
        static bool call(Parser const& parser
          , Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr)
        {
            typedef parser_accepts_container<
                    Parser, Attribute, Context>
            parser_accepts_container;

            typedef parser_attr_is_substitute_for_container_value<
                    Parser, Iterator, Attribute, Context>
            parser_attr_is_substitute_for_container_value;

            typedef mpl::or_<
                parser_accepts_container
              , mpl::not_<parser_attr_is_substitute_for_container_value>>
            pass_attibute_as_is;

            return call(parser, first, last, context, rcontext, attr,
                pass_attibute_as_is());
        }
    };

    template <typename Parser, typename Iterator, typename Context
      , typename RContext, typename Attribute>
    bool parse_into_container(
        Parser const& parser
      , Iterator& first, Iterator const& last, Context const& context
      , RContext& rcontext, Attribute& attr)
    {
        return parse_into_container_impl<Parser, Context, RContext>::call(
            parser, first, last, context, rcontext, attr);
    }

}}}}

#endif
