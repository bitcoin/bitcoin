/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_OPERATOR_SEQUENCE_HPP
#define BOOST_SPIRIT_QI_OPERATOR_SEQUENCE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/operator/sequence_base.hpp>
#include <boost/spirit/home/qi/detail/fail_function.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/proto/operators.hpp>
#include <boost/proto/tags.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<qi::domain, proto::tag::shift_right> // enables >>
      : mpl::true_ {};

    template <>
    struct flatten_tree<qi::domain, proto::tag::shift_right> // flattens >>
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
    template <typename Elements>
    struct sequence : sequence_base<sequence<Elements>, Elements>
    {
        friend struct sequence_base<sequence<Elements>, Elements>;

        sequence(Elements const& elements)
          : sequence_base<sequence<Elements>, Elements>(elements) {}

    private:

        template <typename Iterator, typename Context, typename Skipper>
        static detail::fail_function<Iterator, Context, Skipper>
        fail_function(
            Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper)
        {
            return detail::fail_function<Iterator, Context, Skipper>
                (first, last, context, skipper);
        }

        std::string id() const { return "sequence"; }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::shift_right, Elements, Modifiers>
      : make_nary_composite<Elements, sequence>
    {};

//     ///////////////////////////////////////////////////////////////////////////
//     // Define what attributes are compatible with a sequence
//     template <typename Attribute, typename Elements, typename Context, typename Iterator>
//     struct is_attribute_compatible<Attribute, sequence<Elements>, Context, Iterator>
//       : mpl::or_<
//             is_convertible<Attribute
//               , typename traits::attribute_of<sequence<Elements>, Context, Iterator>::type> 
//           , traits::is_fusion_sequence_compatible<qi::domain, Attribute
//               , sequence<Elements>, Context, Iterator> 
//           , traits::is_container_compatible<qi::domain, Attribute
//               , sequence<Elements>, Context, Iterator>
//         >
//     {};
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements>
    struct has_semantic_action<qi::sequence<Elements> >
      : nary_has_semantic_action<Elements> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<qi::sequence<Elements>, Attribute, Context
          , Iterator>
      : mpl::true_ {};
}}}

#endif
