//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_PP_IS_ITERATING)

#if !defined(BOOST_SPIRIT_KARMA_FORMAT_MANIP_ATTR_APR_24_2009_0734AM)
#define BOOST_SPIRIT_KARMA_FORMAT_MANIP_ATTR_APR_24_2009_0734AM

#include <boost/spirit/home/karma/stream/format_manip.hpp>

#include <boost/fusion/include/vector.hpp>
#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_FILENAME_1                                                   \
    <boost/spirit/home/karma/stream/format_manip_attr.hpp>
#define BOOST_PP_ITERATION_LIMITS (2, SPIRIT_ARGUMENTS_LIMIT)
#include BOOST_PP_ITERATE()

#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()
#define BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE(z, n, A)                       \
    BOOST_PP_CAT(A, n) const&

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma 
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr, BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline detail::format_manip<Expr, mpl::false_, mpl::true_, unused_type
      , fusion::vector<
            BOOST_PP_ENUM(N, BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE, A)
        > > 
    format(Expr const& xpr, BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        using karma::detail::format_manip;

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);

        typedef fusion::vector<
            BOOST_PP_ENUM(N, BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE, A)
        > vector_type;

        vector_type attr (BOOST_PP_ENUM_PARAMS(N, attr));
        return format_manip<Expr, mpl::false_, mpl::true_, unused_type, vector_type>(
            xpr, unused, attr);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr, typename Delimiter
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline detail::format_manip<Expr, mpl::false_, mpl::true_, Delimiter
      , fusion::vector<
            BOOST_PP_ENUM(N, BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE, A)
        > > 
    format_delimited(Expr const& xpr, Delimiter const& d
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        using karma::detail::format_manip;

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Delimiter);

        typedef fusion::vector<
            BOOST_PP_ENUM(N, BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE, A)
        > vector_type;

        vector_type attr (BOOST_PP_ENUM_PARAMS(N, attr));
        return format_manip<Expr, mpl::false_, mpl::true_, Delimiter, vector_type>(
            xpr, d, pre_delimit, attr);
    }

    template <typename Expr, typename Delimiter
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline detail::format_manip<Expr, mpl::false_, mpl::true_, Delimiter
      , fusion::vector<
            BOOST_PP_ENUM(N, BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE, A)
        > > 
    format_delimited(Expr const& xpr, Delimiter const& d
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        using karma::detail::format_manip;

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Delimiter);

        typedef fusion::vector<
            BOOST_PP_ENUM(N, BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE, A)
        > vector_type;

        vector_type attr (BOOST_PP_ENUM_PARAMS(N, attr));
        return format_manip<Expr, mpl::false_, mpl::true_, Delimiter, vector_type>(
            xpr, d, delimit_flag::dont_predelimit, attr);
    }
}}}

#undef BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE
#undef N

#endif 

