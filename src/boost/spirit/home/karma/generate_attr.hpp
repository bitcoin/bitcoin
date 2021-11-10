//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_PP_IS_ITERATING)

#if !defined(BOOST_SPIRIT_KARMA_GENERATE_ATTR_APR_23_2009_0541PM)
#define BOOST_SPIRIT_KARMA_GENERATE_ATTR_APR_23_2009_0541PM

#include <boost/spirit/home/karma/generate.hpp>

#include <boost/fusion/include/vector.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_FILENAME_1 <boost/spirit/home/karma/generate_attr.hpp>
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

namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Properties, typename Expr
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool
    generate(
        detail::output_iterator<OutputIterator, Properties>& sink
      , Expr const& expr
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);

        typedef fusion::vector<
            BOOST_PP_ENUM(N, BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE, A)
        > vector_type;

        vector_type attr (BOOST_PP_ENUM_PARAMS(N, attr));
        return compile<karma::domain>(expr).generate(sink, unused, unused, attr);
    }

    template <typename OutputIterator, typename Expr
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool
    generate(
        OutputIterator& sink_
      , Expr const& expr
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        typedef traits::properties_of<
            typename result_of::compile<karma::domain, Expr>::type
        > properties;

        // wrap user supplied iterator into our own output iterator
        detail::output_iterator<OutputIterator
          , mpl::int_<properties::value> > sink(sink_);
        return karma::generate(sink, expr, BOOST_PP_ENUM_PARAMS(N, attr));
    }

    template <typename OutputIterator, typename Expr
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool
    generate(
        OutputIterator const& sink_
      , Expr const& expr
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        OutputIterator sink = sink_;
        return karma::generate(sink, expr, BOOST_PP_ENUM_PARAMS(N, attr));
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Properties, typename Expr
      , typename Delimiter, BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool
    generate_delimited(
        detail::output_iterator<OutputIterator, Properties>& sink
      , Expr const& expr
      , Delimiter const& delimiter
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then either the expression (expr) or skipper is not a valid
        // spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Delimiter);

        typename result_of::compile<karma::domain, Delimiter>::type const 
            delimiter_ = compile<karma::domain>(delimiter);

        if (pre_delimit == delimit_flag::predelimit &&
            !karma::delimit_out(sink, delimiter_))
        {
            return false;
        }

        typedef fusion::vector<
            BOOST_PP_ENUM(N, BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE, A)
        > vector_type;

        vector_type attr (BOOST_PP_ENUM_PARAMS(N, attr));
        return compile<karma::domain>(expr).
            generate(sink, unused, delimiter_, attr);
    }

    template <typename OutputIterator, typename Expr, typename Delimiter
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool
    generate_delimited(
        OutputIterator& sink_
      , Expr const& expr
      , Delimiter const& delimiter
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        typedef traits::properties_of<
            typename result_of::compile<karma::domain, Expr>::type
        > properties;
        typedef traits::properties_of<
            typename result_of::compile<karma::domain, Delimiter>::type
        > delimiter_properties;

        // wrap user supplied iterator into our own output iterator
        detail::output_iterator<OutputIterator
          , mpl::int_<properties::value | delimiter_properties::value>
        > sink(sink_);
        return karma::generate_delimited(sink, expr, delimiter, pre_delimit
          , BOOST_PP_ENUM_PARAMS(N, attr));
    }

    template <typename OutputIterator, typename Expr, typename Delimiter
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool
    generate_delimited(
        OutputIterator const& sink_
      , Expr const& expr
      , Delimiter const& delimiter
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        OutputIterator sink = sink_;
        return karma::generate_delimited(sink, expr, delimiter, pre_delimit
          , BOOST_PP_ENUM_PARAMS(N, attr));
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Expr, typename Delimiter
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool
    generate_delimited(
        OutputIterator& sink_
      , Expr const& expr
      , Delimiter const& delimiter
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        typedef traits::properties_of<
            typename result_of::compile<karma::domain, Expr>::type
        > properties;
        typedef traits::properties_of<
            typename result_of::compile<karma::domain, Delimiter>::type
        > delimiter_properties;

        // wrap user supplied iterator into our own output iterator
        detail::output_iterator<OutputIterator
          , mpl::int_<properties::value | delimiter_properties::value>
        > sink(sink_);
        return karma::generate_delimited(sink, expr, delimiter
          , delimit_flag::dont_predelimit, BOOST_PP_ENUM_PARAMS(N, attr));
    }

    template <typename OutputIterator, typename Expr, typename Delimiter
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool
    generate_delimited(
        OutputIterator const& sink_
      , Expr const& expr
      , Delimiter const& delimiter
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        OutputIterator sink = sink_;
        return karma::generate_delimited(sink, expr, delimiter 
          , delimit_flag::dont_predelimit, BOOST_PP_ENUM_PARAMS(N, attr));
    }

}}}

#undef BOOST_SPIRIT_KARMA_ATTRIBUTE_REFERENCE
#undef N

#endif // defined(BOOST_PP_IS_ITERATING)

