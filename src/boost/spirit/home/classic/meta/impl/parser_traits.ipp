/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    Copyright (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_PARSER_TRAITS_IPP)
#define BOOST_SPIRIT_PARSER_TRAITS_IPP

#include <boost/spirit/home/classic/core/composite/operators.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

namespace impl
{


    ///////////////////////////////////////////////////////////////////////////
    struct parser_type_traits_base {

        BOOST_STATIC_CONSTANT(bool, is_alternative = false);
        BOOST_STATIC_CONSTANT(bool, is_sequence = false);
        BOOST_STATIC_CONSTANT(bool, is_sequential_or = false);
        BOOST_STATIC_CONSTANT(bool, is_intersection = false);
        BOOST_STATIC_CONSTANT(bool, is_difference = false);
        BOOST_STATIC_CONSTANT(bool, is_exclusive_or = false);
        BOOST_STATIC_CONSTANT(bool, is_optional = false);
        BOOST_STATIC_CONSTANT(bool, is_kleene_star = false);
        BOOST_STATIC_CONSTANT(bool, is_positive = false);
    };

    template <typename ParserT>
    struct parser_type_traits : public parser_type_traits_base {

    //  no definition here, fallback for all not explicitly mentioned parser
    //  types
    };

    template <typename A, typename B>
    struct parser_type_traits<alternative<A, B> >
    :   public parser_type_traits_base {

        BOOST_STATIC_CONSTANT(bool, is_alternative = true);
    };

    template <typename A, typename B>
    struct parser_type_traits<sequence<A, B> >
    :   public parser_type_traits_base {

        BOOST_STATIC_CONSTANT(bool, is_sequence = true);
    };

    template <typename A, typename B>
    struct parser_type_traits<sequential_or<A, B> >
    :   public parser_type_traits_base {

        BOOST_STATIC_CONSTANT(bool, is_sequential_or = true);
    };

    template <typename A, typename B>
    struct parser_type_traits<intersection<A, B> >
    :   public parser_type_traits_base {

        BOOST_STATIC_CONSTANT(bool, is_intersection = true);
    };

    template <typename A, typename B>
    struct parser_type_traits<difference<A, B> >
    :   public parser_type_traits_base {

        BOOST_STATIC_CONSTANT(bool, is_difference = true);
    };

    template <typename A, typename B>
    struct parser_type_traits<exclusive_or<A, B> >
    :   public parser_type_traits_base {

        BOOST_STATIC_CONSTANT(bool, is_exclusive_or = true);
    };

    template <typename S>
    struct parser_type_traits<optional<S> >
    :   public parser_type_traits_base {

        BOOST_STATIC_CONSTANT(bool, is_optional = true);
    };

    template <typename S>
    struct parser_type_traits<kleene_star<S> >
    :   public parser_type_traits_base {

        BOOST_STATIC_CONSTANT(bool, is_kleene_star = true);
    };

    template <typename S>
    struct parser_type_traits<positive<S> >
    :   public parser_type_traits_base {

        BOOST_STATIC_CONSTANT(bool, is_positive = true);
    };

}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif // !defined(BOOST_SPIRIT_PARSER_TRAITS_IPP)
