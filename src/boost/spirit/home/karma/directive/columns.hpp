//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_COLUMNS_DEC_03_2009_0736AM)
#define BOOST_SPIRIT_KARMA_COLUMNS_DEC_03_2009_0736AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/detail/default_width.hpp>
#include <boost/spirit/home/karma/auxiliary/eol.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/integer_traits.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<karma::domain, tag::columns>   // enables columns[]
      : mpl::true_ {};

    // enables columns(c)[g], where c provides the number of require columns
    template <typename T>
    struct use_directive<karma::domain
          , terminal_ex<tag::columns, fusion::vector1<T> > > 
      : mpl::true_ {};

    // enables *lazy* columns(c)[g]
    template <>
    struct use_lazy_directive<karma::domain, tag::columns, 1> 
      : mpl::true_ {};

    // enables columns(c, d)[g], where c provides the number of require columns
    // and d is the custom column-delimiter (default is karma::endl)
    template <typename T1, typename T2>
    struct use_directive<karma::domain
          , terminal_ex<tag::columns, fusion::vector2<T1, T2> > > 
      : boost::spirit::traits::matches<karma::domain, T2> {};

    // enables *lazy* columns(c, d)[g]
    template <>
    struct use_lazy_directive<karma::domain, tag::columns, 2> 
      : mpl::true_ {};

}}

namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::columns;
#endif
    using spirit::columns_type;

    namespace detail
    {
        template <typename Delimiter, typename ColumnDelimiter>
        struct columns_delimiter 
        {
            columns_delimiter(Delimiter const& delim
                  , ColumnDelimiter const& cdelim, unsigned int const numcols)
              : delimiter(delim), column_delimiter(cdelim)
              , numcolumns(numcols), count(0) {}

            template <typename OutputIterator, typename Context
              , typename Delimiter_, typename Attribute>
            bool generate(OutputIterator& sink, Context&, Delimiter_ const&
              , Attribute const&) const
            {
                // first invoke the embedded delimiter
                if (!karma::delimit_out(sink, delimiter))
                    return false;

                // now we count the number of invocations and emit the column 
                // delimiter if needed
                if ((++count % numcolumns) == 0)
                    return karma::delimit_out(sink, column_delimiter);
                return true;
            }

            // generate a final column delimiter if the last invocation didn't 
            // emit one
            template <typename OutputIterator>
            bool delimit_out(OutputIterator& sink) const
            {
                if (count % numcolumns)
                    return karma::delimit_out(sink, column_delimiter);
                return true;
            }

            Delimiter const& delimiter;
            ColumnDelimiter const& column_delimiter;
            unsigned int const numcolumns;
            mutable unsigned int count;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(columns_delimiter& operator= (columns_delimiter const&))
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    //  The columns_generator is used for columns(c, d)[...] directives.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename NumColumns, typename ColumnsDelimiter>
    struct columns_generator 
      : unary_generator<columns_generator<Subject, NumColumns, ColumnsDelimiter> >
    {
        typedef Subject subject_type;
        typedef ColumnsDelimiter delimiter_type;

        typedef mpl::int_<
            subject_type::properties::value | delimiter_type::properties::value 
        > properties;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        columns_generator(Subject const& subject, NumColumns const& cols
              , ColumnsDelimiter const& cdelimiter)
          : subject(subject), numcolumns(cols), column_delimiter(cdelimiter) 
        {
            // having zero number of columns doesn't make any sense
            BOOST_ASSERT(numcolumns > 0);
        }

        template <typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx
          , Delimiter const& delimiter, Attribute const& attr) const
        {
            //  The columns generator dispatches to the embedded generator 
            //  while supplying a new delimiter to use, wrapping the outer 
            //  delimiter.
            typedef detail::columns_delimiter<
                Delimiter, ColumnsDelimiter
            > columns_delimiter_type;

            columns_delimiter_type d(delimiter, column_delimiter, numcolumns);
            return subject.generate(sink, ctx, d, attr) && d.delimit_out(sink);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("columns", subject.what(context));
        }

        Subject subject;
        NumColumns numcolumns;
        ColumnsDelimiter column_delimiter;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////

    // creates columns[] directive
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::columns, Subject, Modifiers>
    {
        typedef typename
            result_of::compile<karma::domain, eol_type, Modifiers>::type
        columns_delimiter_type;
        typedef columns_generator<
            Subject, detail::default_columns, columns_delimiter_type> 
        result_type;

        result_type operator()(unused_type, Subject const& subject
          , unused_type) const
        {
#if defined(BOOST_SPIRIT_NO_PREDEFINED_TERMINALS)
            eol_type const eol = eol_type();
#endif
            return result_type(subject, detail::default_columns()
              , compile<karma::domain>(eol));
        }
    };

    // creates columns(c)[] directive generator (c is the number of columns)
    template <typename T, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::columns, fusion::vector1<T> >
      , Subject, Modifiers
      , typename enable_if_c<integer_traits<T>::is_integral>::type>
    {
        typedef typename
            result_of::compile<karma::domain, eol_type, Modifiers>::type
        columns_delimiter_type;
        typedef columns_generator<
            Subject, T, columns_delimiter_type
        > result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , unused_type) const
        {
#if defined(BOOST_SPIRIT_NO_PREDEFINED_TERMINALS)
            eol_type const eol = eol_type();
#endif
            return result_type(subject, fusion::at_c<0>(term.args)
              , compile<karma::domain>(eol));
        }
    };

    // creates columns(d)[] directive generator (d is the column delimiter)
    template <typename T, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::columns, fusion::vector1<T> >
      , Subject, Modifiers
      , typename enable_if<
            mpl::and_<
                spirit::traits::matches<karma::domain, T>,
                mpl::not_<mpl::bool_<integer_traits<T>::is_integral> >
            >
        >::type>
    {
        typedef typename
            result_of::compile<karma::domain, T, Modifiers>::type
        columns_delimiter_type;
        typedef columns_generator<
            Subject, detail::default_columns, columns_delimiter_type
        > result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , unused_type) const
        {
            return result_type(subject, detail::default_columns()
              , compile<karma::domain>(fusion::at_c<0>(term.args)));
        }
    };

    // creates columns(c, d)[] directive generator (c is the number of columns
    // and d is the column delimiter)
    template <typename T1, typename T2, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::columns, fusion::vector2<T1, T2> >
      , Subject, Modifiers>
    {
        typedef typename
            result_of::compile<karma::domain, T2, Modifiers>::type
        columns_delimiter_type;
        typedef columns_generator<
            Subject, T1, columns_delimiter_type
        > result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , unused_type) const
        {
            return result_type (subject, fusion::at_c<0>(term.args)
              , compile<karma::domain>(fusion::at_c<1>(term.args)));
        }
    };

}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename T1, typename T2>
    struct has_semantic_action<karma::columns_generator<Subject, T1, T2> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename T1, typename T2, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<
            karma::columns_generator<Subject, T1, T2>, Attribute
          , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
