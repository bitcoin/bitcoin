/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_BINARY_MAY_08_2007_0808AM)
#define BOOST_SPIRIT_X3_BINARY_MAY_08_2007_0808AM

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <cstdint>

#include <boost/endian/conversion.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/config.hpp>
#include <climits>

namespace boost { namespace spirit { namespace x3
{
    template <typename T, boost::endian::order endian, std::size_t bits>
    struct binary_lit_parser
      : parser<binary_lit_parser<T, endian, bits> >
    {
        static bool const has_attribute = false;
        typedef unused_type attribute_type;

        constexpr binary_lit_parser(T n_)
          : n(n_) {}

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute& attr_param) const
        {
            x3::skip_over(first, last, context);

            unsigned char const* bytes = n.data();

            Iterator it = first;
            for (unsigned int i = 0; i < sizeof(n); ++i)
            {
                if (it == last || *bytes++ != static_cast<unsigned char>(*it++))
                    return false;
            }

            first = it;
            x3::traits::move_to(n, attr_param);
            return true;
        }

        boost::endian::endian_arithmetic<endian, T, bits> n;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, boost::endian::order endian, std::size_t bits>
    struct any_binary_parser : parser<any_binary_parser<T, endian, bits > >
    {

        typedef T attribute_type;
        static bool const has_attribute =
            !is_same<unused_type, attribute_type>::value;

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute& attr_param) const
        {
            x3::skip_over(first, last, context);

            // Properly align the buffer for performance reasons
            alignas(T) unsigned char buf[sizeof(T)];
            unsigned char * bytes = buf;

            Iterator it = first;
            for (unsigned int i = 0; i < sizeof(T); ++i)
            {
                if (it == last)
                    return false;
                *bytes++ = *it++;
            }

            first = it;

            static_assert(bits % CHAR_BIT == 0,
                          "Boost.Endian supports only multiples of CHAR_BIT");
            x3::traits::move_to(
                endian::endian_load<T, bits / CHAR_BIT, endian>(buf),
                attr_param);
            return true;
        }

        constexpr binary_lit_parser<T, endian, bits> operator()(T n) const
        {
            return {n};
        }
    };

#define BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(name, endiantype, attrtype, bits)                  \
    typedef any_binary_parser< attrtype, boost::endian::order::endiantype, bits > name##type; \
    constexpr name##type name = name##type();


    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(byte_, native, uint_least8_t, 8)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(word, native, uint_least16_t, 16)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_word, big, uint_least16_t, 16)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_word, little, uint_least16_t, 16)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(dword, native, uint_least32_t, 32)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_dword, big, uint_least32_t, 32)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_dword, little, uint_least32_t, 32)
#ifdef BOOST_HAS_LONG_LONG
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(qword, native, uint_least64_t, 64)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_qword, big, uint_least64_t, 64)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_qword, little, uint_least64_t, 64)
#endif
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(bin_float, native, float, sizeof(float) * CHAR_BIT)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_bin_float, big, float, sizeof(float) * CHAR_BIT)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_bin_float, little, float, sizeof(float) * CHAR_BIT)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(bin_double, native, double, sizeof(double) * CHAR_BIT)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_bin_double, big, double, sizeof(double) * CHAR_BIT)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_bin_double, little, double, sizeof(double) * CHAR_BIT)

#undef BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, std::size_t bits>
    struct get_info<any_binary_parser<T, endian::order::little, bits>>
    {
        typedef std::string result_type;
        std::string operator()(any_binary_parser<T, endian::order::little, bits> const&) const
        {
            return "little-endian binary";
        }
    };

    template <typename T, std::size_t bits>
    struct get_info<any_binary_parser<T, endian::order::big, bits>>
    {
        typedef std::string result_type;
        std::string operator()(any_binary_parser<T, endian::order::big, bits> const&) const
        {
            return "big-endian binary";
        }
    };

    template <typename T, std::size_t bits>
    struct get_info<binary_lit_parser<T, endian::order::little, bits>>
    {
        typedef std::string result_type;
        std::string operator()(binary_lit_parser<T, endian::order::little, bits> const&) const
        {
            return "little-endian binary";
        }
    };

    template <typename T, std::size_t bits>
    struct get_info<binary_lit_parser<T, endian::order::big, bits>>
    {
        typedef std::string result_type;
        std::string operator()(binary_lit_parser<T, endian::order::big, bits> const&) const
        {
            return "big-endian binary";
        }
    };

}}}

#endif
