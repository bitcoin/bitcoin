/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_RULE_JAN_08_2012_0326PM)
#define BOOST_SPIRIT_X3_RULE_JAN_08_2012_0326PM

#include <boost/spirit/home/x3/nonterminal/detail/rule.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <type_traits>

#if !defined(BOOST_SPIRIT_X3_NO_RTTI)
#include <typeinfo>
#endif

namespace boost { namespace spirit { namespace x3
{
    // default parse_rule implementation
    template <typename ID, typename Iterator
      , typename Context, typename ActualAttribute>
    inline detail::default_parse_rule_result
    parse_rule(
        detail::rule_id<ID>
      , Iterator& first, Iterator const& last
      , Context const& context, ActualAttribute& attr)
    {
        static_assert(!is_same<decltype(x3::get<ID>(context)), unused_type>::value,
            "BOOST_SPIRIT_DEFINE undefined for this rule.");
        return x3::get<ID>(context).parse(first, last, context, unused, attr);
    }

    template <typename ID, typename RHS, typename Attribute, bool force_attribute_, bool skip_definition_injection = false>
    struct rule_definition : parser<rule_definition<ID, RHS, Attribute, force_attribute_>>
    {
        typedef rule_definition<ID, RHS, Attribute, force_attribute_, skip_definition_injection> this_type;
        typedef ID id;
        typedef RHS rhs_type;
        typedef rule<ID, Attribute, force_attribute_> lhs_type;
        typedef Attribute attribute_type;

        static bool const has_attribute =
            !is_same<Attribute, unused_type>::value;
        static bool const handles_container =
            traits::is_container<Attribute>::value;
        static bool const force_attribute =
            force_attribute_;

        constexpr rule_definition(RHS const& rhs, char const* name)
          : rhs(rhs), name(name) {}

        template <typename Iterator, typename Context, typename Attribute_>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute_& attr) const
        {
            return detail::rule_parser<attribute_type, ID, skip_definition_injection>
                ::call_rule_definition(
                    rhs, name, first, last
                  , context
                  , attr
                  , mpl::bool_<force_attribute>());
        }

        RHS rhs;
        char const* name;
    };

    template <typename ID, typename Attribute, bool force_attribute_>
    struct rule : parser<rule<ID, Attribute, force_attribute_>>
    {
        static_assert(!std::is_reference<Attribute>::value,
                      "Reference qualifier on rule attribute type is meaningless");

        typedef ID id;
        typedef Attribute attribute_type;
        static bool const has_attribute =
            !std::is_same<std::remove_const_t<Attribute>, unused_type>::value;
        static bool const handles_container =
            traits::is_container<Attribute>::value;
        static bool const force_attribute = force_attribute_;

#if !defined(BOOST_SPIRIT_X3_NO_RTTI)
        rule() : name(typeid(rule).name()) {}
#else
        constexpr rule() : name("unnamed") {}
#endif

        constexpr rule(char const* name)
          : name(name) {}

        constexpr rule(rule const& r)
          : name(r.name)
        {
            // Assert that we are not copying an unitialized static rule. If
            // the static is in another TU, it may be initialized after we copy
            // it. If so, its name member will be nullptr.
            BOOST_ASSERT_MSG(r.name, "uninitialized rule"); // static initialization order fiasco
        }

        template <typename RHS>
        constexpr rule_definition<
            ID, typename extension::as_parser<RHS>::value_type, Attribute, force_attribute_>
        operator=(RHS const& rhs) const&
        {
            return { as_parser(rhs), name };
        }

        template <typename RHS>
        constexpr rule_definition<
            ID, typename extension::as_parser<RHS>::value_type, Attribute, true>
        operator%=(RHS const& rhs) const&
        {
            return { as_parser(rhs), name };
        }

        // When a rule placeholder constructed and immediately consumed it cannot be used recursively,
        // that's why the rule definition injection into a parser context can be skipped.
        // This optimization has a huge impact on compile times because immediate rules are commonly
        // used to cast an attribute like `as`/`attr_cast` does in Qi.
        template <typename RHS>
        constexpr rule_definition<
            ID, typename extension::as_parser<RHS>::value_type, Attribute, force_attribute_, true>
        operator=(RHS const& rhs) const&&
        {
            return { as_parser(rhs), name };
        }

        template <typename RHS>
        constexpr rule_definition<
            ID, typename extension::as_parser<RHS>::value_type, Attribute, true, true>
        operator%=(RHS const& rhs) const&&
        {
            return { as_parser(rhs), name };
        }


        template <typename Iterator, typename Context, typename Attribute_>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute_& attr) const
        {
            static_assert(has_attribute,
                "The rule does not have an attribute. Check your parser.");

            using transform = traits::transform_attribute<
                Attribute_, attribute_type, parser_id>;

            using transform_attr = typename transform::type;
            transform_attr attr_ = transform::pre(attr);

            if (parse_rule(detail::rule_id<ID>{}, first, last, context, attr_)) {
                transform::post(attr, std::forward<transform_attr>(attr_));
                return true;
            }
            return false;
        }

        template <typename Iterator, typename Context>
        bool parse(Iterator& first, Iterator const& last
            , Context const& context, unused_type, unused_type) const
        {
            // make sure we pass exactly the rule attribute type
            attribute_type no_attr{};
            return parse_rule(detail::rule_id<ID>{}, first, last, context, no_attr);
        }

        char const* name;
    };

    namespace traits
    {
        template <typename T, typename Enable = void>
        struct is_rule : mpl::false_ {};

        template <typename ID, typename Attribute, bool force_attribute>
        struct is_rule<rule<ID, Attribute, force_attribute>> : mpl::true_ {};

        template <typename ID, typename Attribute, typename RHS, bool force_attribute, bool skip_definition_injection>
        struct is_rule<rule_definition<ID, RHS, Attribute, force_attribute, skip_definition_injection>> : mpl::true_ {};
    }

    template <typename T>
    struct get_info<T, typename enable_if<traits::is_rule<T>>::type>
    {
        typedef std::string result_type;
        std::string operator()(T const& r) const
        {
            BOOST_ASSERT_MSG(r.name, "uninitialized rule"); // static initialization order fiasco
            return r.name? r.name : "uninitialized";
        }
    };

#define BOOST_SPIRIT_DECLARE_(r, data, rule_type)                               \
    template <typename Iterator, typename Context>                              \
    bool parse_rule(                                                            \
        ::boost::spirit::x3::detail::rule_id<rule_type::id>                     \
      , Iterator& first, Iterator const& last                                   \
      , Context const& context, rule_type::attribute_type& attr);               \
    /***/

#define BOOST_SPIRIT_DECLARE(...) BOOST_PP_SEQ_FOR_EACH(                        \
    BOOST_SPIRIT_DECLARE_, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))            \
    /***/

#if BOOST_WORKAROUND(BOOST_MSVC, < 1910)
#define BOOST_SPIRIT_DEFINE_(r, data, rule_name)                                \
    using BOOST_PP_CAT(rule_name, _synonym) = decltype(rule_name);              \
    template <typename Iterator, typename Context>                              \
    inline bool parse_rule(                                                     \
        ::boost::spirit::x3::detail::rule_id<BOOST_PP_CAT(rule_name, _synonym)::id> \
      , Iterator& first, Iterator const& last                                   \
      , Context const& context, BOOST_PP_CAT(rule_name, _synonym)::attribute_type& attr) \
    {                                                                           \
        using rule_t = BOOST_JOIN(rule_name, _synonym);                         \
        return ::boost::spirit::x3::detail                                      \
            ::rule_parser<typename rule_t::attribute_type, rule_t::id, true>    \
            ::call_rule_definition(                                             \
                BOOST_JOIN(rule_name, _def), rule_name.name                     \
              , first, last, context, attr                                      \
              , ::boost::mpl::bool_<rule_t::force_attribute>());                \
    }                                                                           \
    /***/
#else
#define BOOST_SPIRIT_DEFINE_(r, data, rule_name)                                \
    template <typename Iterator, typename Context>                              \
    inline bool parse_rule(                                                     \
        ::boost::spirit::x3::detail::rule_id<decltype(rule_name)::id>           \
      , Iterator& first, Iterator const& last                                   \
      , Context const& context, decltype(rule_name)::attribute_type& attr)      \
    {                                                                           \
        using rule_t = decltype(rule_name);                                     \
        return ::boost::spirit::x3::detail                                      \
            ::rule_parser<typename rule_t::attribute_type, rule_t::id, true>    \
            ::call_rule_definition(                                             \
                BOOST_JOIN(rule_name, _def), rule_name.name                     \
              , first, last, context, attr                                      \
              , ::boost::mpl::bool_<rule_t::force_attribute>());                \
    }                                                                           \
    /***/
#endif

#define BOOST_SPIRIT_DEFINE(...) BOOST_PP_SEQ_FOR_EACH(                         \
    BOOST_SPIRIT_DEFINE_, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))             \
    /***/

#define BOOST_SPIRIT_INSTANTIATE(rule_type, Iterator, Context)                  \
    template bool parse_rule<Iterator, Context>(                                \
        ::boost::spirit::x3::detail::rule_id<rule_type::id>                     \
      , Iterator& first, Iterator const& last                                   \
      , Context const& context, rule_type::attribute_type&);                    \
    /***/


}}}

#endif
