/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_EXCEPTIONS_HPP
#define BOOST_SPIRIT_EXCEPTIONS_HPP

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <exception>

#include <boost/spirit/home/classic/error_handling/exceptions_fwd.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  parser_error_base class
    //
    //      This is the base class of parser_error (see below). This may be
    //      used to catch any type of parser error.
    //
    //      This exception shouldn't propagate outside the parser. However to
    //      avoid quirks of many platforms/implementations which fall outside
    //      the C++ standard, we derive parser_error_base from std::exception
    //      to allow a single catch handler to catch all exceptions.
    //
    ///////////////////////////////////////////////////////////////////////////
    class BOOST_SYMBOL_VISIBLE parser_error_base : public std::exception
    {
    protected:

        parser_error_base() {}
        virtual ~parser_error_base() BOOST_NOEXCEPT_OR_NOTHROW {}

    public:

        parser_error_base(parser_error_base const& rhs)
            : std::exception(rhs) {}
        parser_error_base& operator=(parser_error_base const&)
        {
            return *this;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  parser_error class
    //
    //      Generic parser exception class. This is the base class for all
    //      parser exceptions. The exception holds the iterator position
    //      where the error was encountered in its member variable "where".
    //      The parser_error also holds information regarding the error
    //      (error descriptor) in its member variable "descriptor".
    //
    //      The throw_ function creates and throws a parser_error given
    //      an iterator and an error descriptor.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ErrorDescrT, typename IteratorT>
    struct parser_error : public parser_error_base
    {
        typedef ErrorDescrT error_descr_t;
        typedef IteratorT iterator_t;

        parser_error(IteratorT where_, ErrorDescrT descriptor_)
        : where(where_), descriptor(descriptor_) {}

        parser_error(parser_error const& rhs)
        : parser_error_base(rhs)
        , where(rhs.where), descriptor(rhs.descriptor) {}

        parser_error&
        operator=(parser_error const& rhs)
        {
            where = rhs.where;
            descriptor = rhs.descriptor;
            return *this;
        }

        virtual
        ~parser_error() BOOST_NOEXCEPT_OR_NOTHROW {}

        virtual const char*
        what() const BOOST_NOEXCEPT_OR_NOTHROW
        {
            return "BOOST_SPIRIT_CLASSIC_NS::parser_error";
        }

        IteratorT where;
        ErrorDescrT descriptor;
    };

    //////////////////////////////////
    template <typename ErrorDescrT, typename IteratorT>
    inline void
    throw_(IteratorT where, ErrorDescrT descriptor)
    {
         boost::throw_exception(
            parser_error<ErrorDescrT, IteratorT>(where, descriptor));
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  assertive_parser class
    //
    //      An assertive_parser class is a parser that throws an exception
    //      in response to a parsing failure. The assertive_parser throws a
    //      parser_error exception rather than returning an unsuccessful
    //      match to signal that the parser failed to match the input.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ErrorDescrT, typename ParserT>
    struct assertive_parser
    :   public unary<ParserT, parser<assertive_parser<ErrorDescrT, ParserT> > >
    {
        typedef assertive_parser<ErrorDescrT, ParserT>  self_t;
        typedef unary<ParserT, parser<self_t> >         base_t;
        typedef unary_parser_category                   parser_category_t;

        assertive_parser(ParserT const& parser, ErrorDescrT descriptor_)
        : base_t(parser), descriptor(descriptor_) {}

        template <typename ScannerT>
        struct result
        {
            typedef typename parser_result<ParserT, ScannerT>::type type;
        };

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<ParserT, ScannerT>::type result_t;

            result_t hit = this->subject().parse(scan);
            if (!hit)
            {
                throw_(scan.first, descriptor);
            }
            return hit;
        }

        ErrorDescrT descriptor;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  assertion class
    //
    //      assertive_parsers are never instantiated directly. The assertion
    //      class is used to indirectly create an assertive_parser object.
    //      Before declaring the grammar, we declare some assertion objects.
    //      Examples:
    //
    //          enum Errors
    //          {
    //              program_expected, begin_expected, end_expected
    //          };
    //
    //          assertion<Errors>   expect_program(program_expected);
    //          assertion<Errors>   expect_begin(begin_expected);
    //          assertion<Errors>   expect_end(end_expected);
    //
    //      Now, we can use these assertions as wrappers around parsers:
    //
    //          expect_end(str_p("end"))
    //
    //      Take note that although the example uses enums to hold the
    //      information regarding the error (error desccriptor), we are free
    //      to use other types such as integers and strings. Enums are
    //      convenient for error handlers to easily catch since C++ treats
    //      enums as unique types.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ErrorDescrT>
    struct assertion
    {
        assertion(ErrorDescrT descriptor_)
        : descriptor(descriptor_) {}

        template <typename ParserT>
        assertive_parser<ErrorDescrT, ParserT>
        operator()(ParserT const& parser) const
        {
            return assertive_parser<ErrorDescrT, ParserT>(parser, descriptor);
        }

        ErrorDescrT descriptor;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  error_status<T>
    //
    //      Where T is an attribute type compatible with the match attribute
    //      of the fallback_parser's subject (defaults to nil_t). The class
    //      error_status reports the result of an error handler (see
    //      fallback_parser). result can be one of:
    //
    //          fail:       quit and fail (return a no_match)
    //          retry:      attempt error recovery, possibly moving the scanner
    //          accept:     force success returning a matching length, moving
    //                      the scanner appropriately and returning an attribute
    //                      value
    //          rethrow:    rethrows the error.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct error_status
    {
        enum result_t { fail, retry, accept, rethrow };

        error_status(
            result_t result_ = fail,
            std::ptrdiff_t length_ = -1,
            T const& value_ = T())
        : result(result_), length(length_), value(value_) {}

        result_t        result;
        std::ptrdiff_t  length;
        T               value;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  fallback_parser class
    //
    //      Handles exceptions of type parser_error<ErrorDescrT, IteratorT>
    //      thrown somewhere inside its embedded ParserT object. The class
    //      sets up a try block before delegating parsing to its subject.
    //      When an exception is caught, the catch block then calls the
    //      HandlerT object. HandlerT may be a function or a functor (with
    //      an operator() member function) compatible with the interface:
    //
    //          error_status<T>
    //          handler(ScannerT const& scan, ErrorT error);
    //
    //      Where scan points to the scanner state prior to parsing and error
    //      is the error that arose (see parser_error). The handler must
    //      return an error_status<T> object (see above).
    //
    ///////////////////////////////////////////////////////////////////////////
    namespace impl
    {
        template <typename RT, typename ParserT, typename ScannerT>
        RT fallback_parser_parse(ParserT const& p, ScannerT const& scan);
    }

    template <typename ErrorDescrT, typename ParserT, typename HandlerT>
    struct fallback_parser
    :   public unary<ParserT,
        parser<fallback_parser<ErrorDescrT, ParserT, HandlerT> > >
    {
        typedef fallback_parser<ErrorDescrT, ParserT, HandlerT>
            self_t;
        typedef ErrorDescrT
            error_descr_t;
        typedef unary<ParserT, parser<self_t> >
            base_t;
        typedef unary_parser_category
            parser_category_t;

        fallback_parser(ParserT const& parser, HandlerT const& handler_)
        : base_t(parser), handler(handler_) {}

        template <typename ScannerT>
        struct result
        {
            typedef typename parser_result<ParserT, ScannerT>::type type;
        };

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            return impl::fallback_parser_parse<result_t>(*this, scan);
        }

        HandlerT handler;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  guard class
    //
    //      fallback_parser objects are not instantiated directly. The guard
    //      class is used to indirectly create a fallback_parser object.
    //      guards are typically predeclared just like assertions (see the
    //      assertion class above; the example extends the previous example
    //      introduced in the assertion class above):
    //
    //          guard<Errors>   my_guard;
    //
    //      Errors, in this example is the error descriptor type we want to
    //      detect; This is essentially the ErrorDescrT template parameter
    //      of the fallback_parser class.
    //
    //      my_guard may now be used in a grammar declaration as:
    //
    //          my_guard(p)[h]
    //
    //      where p is a parser, h is a function or functor compatible with
    //      fallback_parser's HandlerT (see above).
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ErrorDescrT, typename ParserT>
    struct guard_gen : public unary<ParserT, nil_t>
    {
        typedef guard<ErrorDescrT>      parser_generator_t;
        typedef unary_parser_category   parser_category_t;

        guard_gen(ParserT const& p)
        : unary<ParserT, nil_t>(p) {}

        template <typename HandlerT>
        fallback_parser<ErrorDescrT, ParserT, HandlerT>
        operator[](HandlerT const& handler) const
        {
            return fallback_parser<ErrorDescrT, ParserT, HandlerT>
                (this->subject(), handler);
        }
    };

    template <typename ErrorDescrT>
    struct guard
    {
        template <typename ParserT>
        struct result
        {
            typedef guard_gen<ErrorDescrT, ParserT> type;
        };

        template <typename ParserT>
        static guard_gen<ErrorDescrT, ParserT>
        generate(ParserT const& parser)
        {
            return guard_gen<ErrorDescrT, ParserT>(parser);
        }

        template <typename ParserT>
        guard_gen<ErrorDescrT, ParserT>
        operator()(ParserT const& parser) const
        {
            return guard_gen<ErrorDescrT, ParserT>(parser);
        }
    };

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#include <boost/spirit/home/classic/error_handling/impl/exceptions.ipp>
#endif

