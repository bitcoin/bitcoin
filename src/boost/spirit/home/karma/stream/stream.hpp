//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_STREAM_MAY_01_2007_0310PM)
#define BOOST_SPIRIT_KARMA_STREAM_MAY_01_2007_0310PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/detail/hold_any.hpp>
#include <boost/spirit/home/support/detail/get_encoding.hpp>
#include <boost/spirit/home/support/detail/is_spirit_tag.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/karma/stream/detail/format_manip.hpp>
#include <boost/spirit/home/karma/detail/get_casetag.hpp>
#include <boost/spirit/home/karma/detail/extract_from.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <ostream>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    namespace tag
    {
        template <typename Char = char>
        struct stream_tag 
        {
            BOOST_SPIRIT_IS_TAG()
        };
    }

    namespace karma
    {
        ///////////////////////////////////////////////////////////////////////
        // This one is the class that the user can instantiate directly in 
        // order to create a customized int generator
        template <typename Char = char>
        struct stream_generator
          : spirit::terminal<tag::stream_tag<Char> > 
        {};
    }

    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<karma::domain, tag::stream>     // enables stream
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, tag::wstream>    // enables wstream
      : mpl::true_ {};

    template <typename A0>
    struct use_terminal<karma::domain                   // enables stream(...)
      , terminal_ex<tag::stream, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <typename A0>
    struct use_terminal<karma::domain                   // enables wstream(...)
      , terminal_ex<tag::wstream, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <>                                         // enables stream(f)
    struct use_lazy_terminal<
        karma::domain, tag::stream, 1   /*arity*/
    > : mpl::true_ {};

    template <>                                         // enables wstream(f)
    struct use_lazy_terminal<
        karma::domain, tag::wstream, 1  /*arity*/
    > : mpl::true_ {};

    // enables stream_generator<char_type>
    template <typename Char>
    struct use_terminal<karma::domain, tag::stream_tag<Char> >
      : mpl::true_ {};

    template <typename Char, typename A0>
    struct use_terminal<karma::domain
      , terminal_ex<tag::stream_tag<Char>, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <typename Char>
    struct use_lazy_terminal<
        karma::domain, tag::stream_tag<Char>, 1  /*arity*/
    > : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::stream;
    using spirit::wstream;
#endif
    using spirit::stream_type;
    using spirit::wstream_type;

namespace detail
{
    template <typename OutputIterator, typename Char, typename CharEncoding
      , typename Tag>
    struct psbuf : std::basic_streambuf<Char>
    {
        psbuf(OutputIterator& sink) : sink_(sink) {}

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(psbuf& operator=(psbuf const&))

    protected:
        typename psbuf::int_type overflow(typename psbuf::int_type ch) BOOST_OVERRIDE
        {
            if (psbuf::traits_type::eq_int_type(ch, psbuf::traits_type::eof()))
                return psbuf::traits_type::not_eof(ch);

            return detail::generate_to(sink_, psbuf::traits_type::to_char_type(ch),
                                       CharEncoding(), Tag()) ? ch : psbuf::traits_type::eof();
        }

    private:
        OutputIterator& sink_;
    };
}

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename CharEncoding, typename Tag>
    struct any_stream_generator
      : primitive_generator<any_stream_generator<Char, CharEncoding, Tag> >
    {
        template <typename Context, typename Unused = unused_type>
        struct attribute
        {
            typedef spirit::basic_hold_any<Char> type;
        };

        // any_stream_generator has an attached attribute 
        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute
        >
        static bool generate(OutputIterator& sink, Context& context
          , Delimiter const& d, Attribute const& attr)
        {
            if (!traits::has_optional_value(attr))
                return false;

            // use existing operator<<()
            typedef typename attribute<Context>::type attribute_type;

            {
                detail::psbuf<OutputIterator, Char, CharEncoding, Tag> pseudobuf(sink);
                std::basic_ostream<Char> ostr(&pseudobuf);
                ostr << traits::extract_from<attribute_type>(attr, context) << std::flush;

                if (!ostr.good())
                    return false;
            }

            return karma::delimit_out(sink, d);   // always do post-delimiting
        }

        // this is a special overload to detect if the output iterator has been
        // generated by a format_manip object.
        template <
            typename T, typename Traits, typename Properties, typename Context
          , typename Delimiter, typename Attribute
        >
        static bool generate(
            karma::detail::output_iterator<
                karma::ostream_iterator<T, Char, Traits>, Properties
            >& sink, Context& context, Delimiter const& d
          , Attribute const& attr)
        {
            typedef karma::detail::output_iterator<
                karma::ostream_iterator<T, Char, Traits>, Properties
            > output_iterator;

            if (!traits::has_optional_value(attr))
                return false;

            // use existing operator<<()
            typedef typename attribute<Context>::type attribute_type;

            {
                detail::psbuf<output_iterator, Char, CharEncoding, Tag> pseudobuf(sink);
                std::basic_ostream<Char> ostr(&pseudobuf);
                ostr.imbue(sink.get_ostream().getloc());
                ostr << traits::extract_from<attribute_type>(attr, context) 
                     << std::flush;
                if (!ostr.good())
                    return false;
            }

            return karma::delimit_out(sink, d);  // always do post-delimiting
        }

        // this any_stream has no parameter attached, it needs to have been
        // initialized from a value/variable
        template <typename OutputIterator, typename Context
          , typename Delimiter>
        static bool
        generate(OutputIterator&, Context&, Delimiter const&, unused_type)
        {
            // It is not possible (doesn't make sense) to use stream generators
            // without providing any attribute, as the generator doesn't 'know'
            // what to output. The following assertion fires if this situation
            // is detected in your code.
            BOOST_SPIRIT_ASSERT_FAIL(OutputIterator, stream_not_usable_without_attribute, ());
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("stream");
        }
    };

    template <typename T, typename Char, typename CharEncoding, typename Tag>
    struct lit_stream_generator
      : primitive_generator<lit_stream_generator<T, Char, CharEncoding, Tag> >
    {
        template <typename Context, typename Unused>
        struct attribute 
        {
            typedef unused_type type;
        };

        lit_stream_generator(typename add_reference<T>::type t)
          : t_(t)
        {}

        // lit_stream_generator has an attached parameter

        // this overload will be used in the normal case (not called from
        // format_manip).
        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context&, Delimiter const& d
          , Attribute const&) const
        {
            detail::psbuf<OutputIterator, Char, CharEncoding, Tag> pseudobuf(sink);
            std::basic_ostream<Char> ostr(&pseudobuf);
            ostr << t_ << std::flush;             // use existing operator<<()

            if (ostr.good()) 
                return karma::delimit_out(sink, d); // always do post-delimiting
            return false;
        }

        // this is a special overload to detect if the output iterator has been
        // generated by a format_manip object.
        template <
            typename T1, typename Traits, typename Properties
          , typename Context, typename Delimiter, typename Attribute>
        bool generate(
            karma::detail::output_iterator<
                karma::ostream_iterator<T1, Char, Traits>, Properties
            >& sink, Context&, Delimiter const& d, Attribute const&) const
        {
            typedef karma::detail::output_iterator<
                karma::ostream_iterator<T1, Char, Traits>, Properties
            > output_iterator;

            {
                detail::psbuf<output_iterator, Char, CharEncoding, Tag> pseudobuf(sink);
                std::basic_ostream<Char> ostr(&pseudobuf);
                ostr.imbue(sink.get_ostream().getloc());
                ostr << t_ << std::flush;             // use existing operator<<()

                if (!ostr.good())
                    return false;
            }

            return karma::delimit_out(sink, d); // always do post-delimiting
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("any-stream");
        }

        T t_;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(lit_stream_generator& operator= (lit_stream_generator const&))
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Modifiers>
    struct make_stream
    {
        static bool const lower =
            has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;

        static bool const upper =
            has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

        typedef any_stream_generator<
            Char
          , typename spirit::detail::get_encoding_with_case<
                Modifiers, unused_type, lower || upper>::type
          , typename detail::get_casetag<Modifiers, lower || upper>::type
        > result_type;

        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    // stream
    template <typename Modifiers>
    struct make_primitive<tag::stream, Modifiers> 
      : make_stream<char, Modifiers> {};

    // wstream
    template <typename Modifiers>
    struct make_primitive<tag::wstream, Modifiers> 
      : make_stream<wchar_t, Modifiers> {};

    // any_stream_generator<char_type>
    template <typename Char, typename Modifiers>
    struct make_primitive<tag::stream_tag<Char>, Modifiers> 
      : make_stream<Char, Modifiers> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename A0, typename Modifiers>
    struct make_any_stream
    {
        static bool const lower =
            has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;

        static bool const upper =
            has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

        typedef typename add_const<A0>::type const_attribute;
        typedef lit_stream_generator<
            const_attribute, Char
          , typename spirit::detail::get_encoding_with_case<
                Modifiers, unused_type, lower || upper>::type
          , typename detail::get_casetag<Modifiers, lower || upper>::type
        > result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

    // stream(...)
    template <typename Modifiers, typename A0>
    struct make_primitive<
            terminal_ex<tag::stream, fusion::vector1<A0> >, Modifiers>
      : make_any_stream<char, A0, Modifiers> {};

    // wstream(...)
    template <typename Modifiers, typename A0>
    struct make_primitive<
            terminal_ex<tag::wstream, fusion::vector1<A0> >, Modifiers>
      : make_any_stream<wchar_t, A0, Modifiers> {};

    // any_stream_generator<char_type>(...)
    template <typename Char, typename Modifiers, typename A0>
    struct make_primitive<
            terminal_ex<tag::stream_tag<Char>, fusion::vector1<A0> >
          , Modifiers>
      : make_any_stream<Char, A0, Modifiers> {};

}}}

#endif
