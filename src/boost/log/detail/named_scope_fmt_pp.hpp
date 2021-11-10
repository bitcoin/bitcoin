/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

template< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT) >
BOOST_FORCEINLINE format_named_scope_actor<
    fallback_to_none,
    typename boost::log::aux::deduce_char_type<
        typename parameter::binding<
            typename boost::log::aux::make_arg_list< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), ArgT) >::type,
            keywords::tag::format,
            void
        >::type
    >::type
> format_named_scope(attribute_name const& name, BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg))
{
    typedef typename boost::log::aux::deduce_char_type<
        typename parameter::binding<
            typename boost::log::aux::make_arg_list< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), ArgT) >::type,
            keywords::tag::format,
            void
        >::type
    >::type char_type;
    return aux::format_named_scope< char_type, phoenix::actor >(name, fallback_to_none(), (BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg)));
}

template< typename DescriptorT, template< typename > class ActorT, BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT) >
BOOST_FORCEINLINE format_named_scope_actor<
    fallback_to_none,
    typename boost::log::aux::deduce_char_type<
        typename parameter::binding<
            typename boost::log::aux::make_arg_list< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), ArgT) >::type,
            keywords::tag::format,
            void
        >::type
    >::type,
    ActorT
>
format_named_scope(attribute_keyword< DescriptorT, ActorT > const& keyword, BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg))
{
    BOOST_STATIC_ASSERT_MSG((is_same< typename DescriptorT::value_type, attributes::named_scope::value_type >::value),\
        "Boost.Log: Named scope formatter only accepts attribute values of type attributes::named_scope::value_type.");

    typedef typename boost::log::aux::deduce_char_type<
        typename parameter::binding<
            typename boost::log::aux::make_arg_list< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), ArgT) >::type,
            keywords::tag::format,
            void
        >::type
    >::type char_type;
    return aux::format_named_scope< char_type, ActorT >(keyword.get_name(), fallback_to_none(), (BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg)));
}

template< typename T, typename FallbackPolicyT, typename TagT, template< typename > class ActorT, BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT) >
BOOST_FORCEINLINE format_named_scope_actor<
    FallbackPolicyT,
    typename boost::log::aux::deduce_char_type<
        typename parameter::binding<
            typename boost::log::aux::make_arg_list< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), ArgT) >::type,
            keywords::tag::format,
            void
        >::type
    >::type,
    ActorT
>
format_named_scope(attribute_actor< T, FallbackPolicyT, TagT, ActorT > const& placeholder, BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg))
{
    BOOST_STATIC_ASSERT_MSG((is_same< T, attributes::named_scope::value_type >::value),\
        "Boost.Log: Named scope formatter only accepts attribute values of type attributes::named_scope::value_type.");

    typedef typename boost::log::aux::deduce_char_type<
        typename parameter::binding<
            typename boost::log::aux::make_arg_list< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), ArgT) >::type,
            keywords::tag::format,
            void
        >::type
    >::type char_type;
    return aux::format_named_scope< char_type, ActorT >(placeholder.get_name(), placeholder.get_fallback_policy(), (BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg)));
}
