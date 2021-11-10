/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_BINARY_MAY_08_2007_0808AM)
#define BOOST_SPIRIT_BINARY_MAY_08_2007_0808AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/detail/endian.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/config.hpp>

#define BOOST_SPIRIT_ENABLE_BINARY(name)                                        \
    template <>                                                                 \
    struct use_terminal<qi::domain, tag::name>                                  \
      : mpl::true_ {};                                                          \
                                                                                \
    template <typename A0>                                                      \
    struct use_terminal<qi::domain                                              \
        , terminal_ex<tag::name, fusion::vector1<A0> > >                        \
      : mpl::or_<is_integral<A0>, is_enum<A0> > {};                             \
                                                                                \
    template <>                                                                 \
    struct use_lazy_terminal<qi::domain, tag::name, 1> : mpl::true_ {};         \
                                                                                \
/***/

#define BOOST_SPIRIT_ENABLE_BINARY_IEEE754(name)                              \
    template<>                                                                \
    struct use_terminal<qi::domain, tag::name>: mpl::true_ {};                \
                                                                              \
    template<typename A0>                                                     \
    struct use_terminal<qi::domain, terminal_ex<tag::name,                    \
        fusion::vector1<A0> > >: is_floating_point<A0> {};                    \
                                                                              \
    template<>                                                                \
    struct use_lazy_terminal<qi::domain, tag::name, 1>: mpl::true_ {};        \
                                                                              \
/***/

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    BOOST_SPIRIT_ENABLE_BINARY(byte_)                   // enables byte_
    BOOST_SPIRIT_ENABLE_BINARY(word)                    // enables word
    BOOST_SPIRIT_ENABLE_BINARY(big_word)                // enables big_word
    BOOST_SPIRIT_ENABLE_BINARY(little_word)             // enables little_word
    BOOST_SPIRIT_ENABLE_BINARY(dword)                   // enables dword
    BOOST_SPIRIT_ENABLE_BINARY(big_dword)               // enables big_dword
    BOOST_SPIRIT_ENABLE_BINARY(little_dword)            // enables little_dword
#ifdef BOOST_HAS_LONG_LONG
    BOOST_SPIRIT_ENABLE_BINARY(qword)                   // enables qword
    BOOST_SPIRIT_ENABLE_BINARY(big_qword)               // enables big_qword
    BOOST_SPIRIT_ENABLE_BINARY(little_qword)            // enables little_qword
#endif
    BOOST_SPIRIT_ENABLE_BINARY_IEEE754(bin_float)
    BOOST_SPIRIT_ENABLE_BINARY_IEEE754(big_bin_float)
    BOOST_SPIRIT_ENABLE_BINARY_IEEE754(little_bin_float)
    BOOST_SPIRIT_ENABLE_BINARY_IEEE754(bin_double)
    BOOST_SPIRIT_ENABLE_BINARY_IEEE754(big_bin_double)
    BOOST_SPIRIT_ENABLE_BINARY_IEEE754(little_bin_double)
}}

#undef BOOST_SPIRIT_ENABLE_BINARY
#undef BOOST_SPIRIT_ENABLE_BINARY_IEEE754

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using boost::spirit::byte_;
    using boost::spirit::word;
    using boost::spirit::big_word;
    using boost::spirit::little_word;
    using boost::spirit::dword;
    using boost::spirit::big_dword;
    using boost::spirit::little_dword;
#ifdef BOOST_HAS_LONG_LONG
    using boost::spirit::qword;
    using boost::spirit::big_qword;
    using boost::spirit::little_qword;
#endif
    using boost::spirit::bin_float;
    using boost::spirit::big_bin_float;
    using boost::spirit::little_bin_float;
    using boost::spirit::bin_double;
    using boost::spirit::big_bin_double;
    using boost::spirit::little_bin_double;
#endif

    using boost::spirit::byte_type;
    using boost::spirit::word_type;
    using boost::spirit::big_word_type;
    using boost::spirit::little_word_type;
    using boost::spirit::dword_type;
    using boost::spirit::big_dword_type;
    using boost::spirit::little_dword_type;
#ifdef BOOST_HAS_LONG_LONG
    using boost::spirit::qword_type;
    using boost::spirit::big_qword_type;
    using boost::spirit::little_qword_type;
#endif
    using boost::spirit::bin_float_type;
    using boost::spirit::big_bin_float_type;
    using boost::spirit::little_bin_float_type;
    using boost::spirit::bin_double_type;
    using boost::spirit::big_bin_double_type;
    using boost::spirit::little_bin_double_type;

    namespace detail
    {
        template <int bits>
        struct integer
        {
#ifdef BOOST_HAS_LONG_LONG
            BOOST_SPIRIT_ASSERT_MSG(
                bits == 8 || bits == 16 || bits == 32 || bits == 64,
                not_supported_binary_size, ());
#else
            BOOST_SPIRIT_ASSERT_MSG(
                bits == 8 || bits == 16 || bits == 32,
                not_supported_binary_size, ());
#endif
        };

        template <>
        struct integer<8>
        {
            enum { size = 1 };
            typedef uint_least8_t type;
        };

        template <>
        struct integer<16>
        {
            enum { size = 2 };
            typedef uint_least16_t type;
        };

        template <>
        struct integer<32>
        {
            enum { size = 4 };
            typedef uint_least32_t type;
        };

#ifdef BOOST_HAS_LONG_LONG
        template <>
        struct integer<64>
        {
            enum { size = 8 };
            typedef uint_least64_t type;
        };
#endif

        template <int bits>
        struct floating_point
        {
            BOOST_SPIRIT_ASSERT_MSG(
                bits == 32 || bits == 64,
                not_supported_binary_size, ());
        };

        template <>
        struct floating_point<32>
        {
            enum { size = 4 };
            typedef float type;
        };

        template <>
        struct floating_point<64>
        {
            enum { size = 8 };
            typedef double type;
        };

        ///////////////////////////////////////////////////////////////////////
        template <BOOST_SCOPED_ENUM(boost::endian::order) bits>
        struct what;

        template <>
        struct what<boost::endian::order::little>
        {
            static char const* is()
            {
                return "little-endian binary";
            }
        };

        template <>
        struct what<boost::endian::order::big>
        {
            static char const* is()
            {
                return "big-endian binary";
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, BOOST_SCOPED_ENUM(boost::endian::order) endian, int bits>
    struct any_binary_parser : primitive_parser<any_binary_parser<T, endian, bits> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef boost::endian::endian_arithmetic<endian, typename T::type,
                bits> type;
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr_param) const
        {
            qi::skip_over(first, last, skipper);

            typename attribute<Context, Iterator>::type attr_;
            unsigned char* bytes = attr_.data();

            Iterator it = first;
            for (unsigned int i = 0; i < sizeof(attr_); ++i)
            {
                if (it == last)
                    return false;
                *bytes++ = *it++;
            }

            first = it;
            spirit::traits::assign_to(attr_, attr_param);
            return true;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info(qi::detail::what<endian>::is());
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename V, typename T
      , BOOST_SCOPED_ENUM(boost::endian::order) endian, int bits>
    struct binary_lit_parser
      : primitive_parser<binary_lit_parser<V, T, endian, bits> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef unused_type type;
        };

        binary_lit_parser(V n_)
          : n(n_) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr_param) const
        {
            qi::skip_over(first, last, skipper);

            boost::endian::endian_arithmetic<endian, typename T::type, bits> attr_;

#if defined(BOOST_MSVC)
// warning C4244: 'argument' : conversion from 'const int' to 'foo', possible loss of data
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
            attr_ = n;
#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

            unsigned char* bytes = attr_.data();

            Iterator it = first;
            for (unsigned int i = 0; i < sizeof(attr_); ++i)
            {
                if (it == last || *bytes++ != static_cast<unsigned char>(*it++))
                    return false;
            }

            first = it;
            spirit::traits::assign_to(attr_, attr_param);
            return true;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info(qi::detail::what<endian>::is());
        }

        V n;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, BOOST_SCOPED_ENUM(boost::endian::order) endian, int bits>
    struct make_binary_parser
    {
        typedef any_binary_parser<T, endian, bits> result_type;
        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    template <typename V, typename T
      , BOOST_SCOPED_ENUM(boost::endian::order) endian, int bits>
    struct make_binary_lit_parser
    {
        typedef binary_lit_parser<V, T, endian, bits> result_type;
        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

#define BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(name, endiantype, bits)            \
    template <typename Modifiers>                                             \
    struct make_primitive<tag::name, Modifiers>                               \
      : make_binary_parser<detail::integer<bits>,                             \
        boost::endian::order::endiantype, bits> {};                           \
                                                                              \
    template <typename Modifiers, typename A0>                                \
    struct make_primitive<                                                    \
        terminal_ex<tag::name, fusion::vector1<A0> > , Modifiers>             \
      : make_binary_lit_parser<A0, detail::integer<bits>,                     \
        boost::endian::order::endiantype, bits> {};                           \
                                                                              \
    /***/

    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(byte_, native, 8)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(word, native, 16)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_word, big, 16)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_word, little, 16)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(dword, native, 32)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_dword, big, 32)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_dword, little, 32)
#ifdef BOOST_HAS_LONG_LONG
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(qword, native, 64)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_qword, big, 64)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_qword, little, 64)
#endif

#undef BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE

#define BOOST_SPIRIT_MAKE_BINARY_IEEE754_PRIMITIVE(name, endiantype, bits)    \
    template<typename Modifiers>                                              \
    struct make_primitive<tag::name, Modifiers>                               \
      : make_binary_parser<detail::floating_point<bits>,                      \
        boost::endian::order::endiantype, bits> {};                           \
                                                                              \
    template<typename Modifiers, typename A0>                                 \
    struct make_primitive<                                                    \
        terminal_ex<tag::name, fusion::vector1<A0> >, Modifiers>              \
      : make_binary_lit_parser<A0, detail::floating_point<bits>,              \
        boost::endian::order::endiantype,                                     \
        bits> {};                                                             \
                                                                              \
    /***/

    BOOST_SPIRIT_MAKE_BINARY_IEEE754_PRIMITIVE(bin_float, native, 32)
    BOOST_SPIRIT_MAKE_BINARY_IEEE754_PRIMITIVE(big_bin_float, big, 32)
    BOOST_SPIRIT_MAKE_BINARY_IEEE754_PRIMITIVE(little_bin_float, little, 32)
    BOOST_SPIRIT_MAKE_BINARY_IEEE754_PRIMITIVE(bin_double, native, 64)
    BOOST_SPIRIT_MAKE_BINARY_IEEE754_PRIMITIVE(big_bin_double, big, 64)
    BOOST_SPIRIT_MAKE_BINARY_IEEE754_PRIMITIVE(little_bin_double, little, 64)

#undef BOOST_SPIRIT_MAKE_BINARY_IEEE754_PRIMITIVE

}}}

#endif
