//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2009 Carl Barron
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_PP_IS_ITERATING)

#if !defined(BOOST_SPIRIT_LEXER_PARSE_ATTR_MAY_27_2009_0926AM)
#define BOOST_SPIRIT_LEXER_PARSE_ATTR_MAY_27_2009_0926AM

#include <boost/spirit/home/lex/tokenize_and_parse.hpp>

#include <boost/fusion/include/vector.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_FILENAME_1 <boost/spirit/home/lex/tokenize_and_parse_attr.hpp>
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
#define BOOST_SPIRIT_QI_ATTRIBUTE_REFERENCE(z, n, A) BOOST_PP_CAT(A, n)&

namespace boost { namespace spirit { namespace lex
{
    template <typename Iterator, typename Lexer, typename ParserExpr
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool
    tokenize_and_parse(Iterator& first, Iterator last, Lexer const& lex
      , ParserExpr const& expr, BOOST_PP_ENUM_BINARY_PARAMS(N, A, & attr))
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(qi::domain, ParserExpr);

        typedef fusion::vector<
            BOOST_PP_ENUM(N, BOOST_SPIRIT_QI_ATTRIBUTE_REFERENCE, A)
        > vector_type;

        vector_type attr (BOOST_PP_ENUM_PARAMS(N, attr));
        typename Lexer::iterator_type iter = lex.begin(first, last);
        return compile<qi::domain>(expr).parse(
            iter, lex.end(), unused, unused, attr);
    }
}}}

#undef BOOST_SPIRIT_QI_ATTRIBUTE_REFERENCE
#undef N

#endif

