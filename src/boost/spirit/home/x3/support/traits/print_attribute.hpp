/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
================================================_==============================*/
#if !defined(BOOST_SPIRIT_X3_PRINT_ATTRIBUTE_JANUARY_20_2013_0814AM)
#define BOOST_SPIRIT_X3_PRINT_ATTRIBUTE_JANUARY_20_2013_0814AM

#include <boost/variant.hpp>
#include <boost/optional/optional.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/spirit/home/x3/support/traits/attribute_category.hpp>
#include <boost/spirit/home/x3/support/traits/is_variant.hpp>
#ifdef BOOST_SPIRIT_X3_UNICODE
# include <boost/spirit/home/support/char_encoding/unicode.hpp>
#endif

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Out, typename T>
    void print_attribute(Out& out, T const& val);

    template <typename Out>
    inline void print_attribute(Out&, unused_type) {}

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Out>
        struct print_fusion_sequence
        {
            print_fusion_sequence(Out& out)
              : out(out), is_first(true) {}

            typedef void result_type;

            template <typename T>
            void operator()(T const& val) const
            {
                if (is_first)
                    is_first = false;
                else
                    out << ", ";
                x3::traits::print_attribute(out, val);
            }

            Out& out;
            mutable bool is_first;
        };

        // print elements in a variant
        template <typename Out>
        struct print_visitor : static_visitor<>
        {
            print_visitor(Out& out) : out(out) {}

            template <typename T>
            void operator()(T const& val) const
            {
                x3::traits::print_attribute(out, val);
            }

            Out& out;
        };
    }

    template <typename Out, typename T, typename Enable = void>
    struct print_attribute_debug
    {
        // for unused_type
        static void call(Out& out, unused_type, unused_attribute)
        {
            out << "unused";
        }

        // for plain data types
        template <typename T_>
        static void call(Out& out, T_ const& val, plain_attribute)
        {
            out << val;
        }

#ifdef BOOST_SPIRIT_X3_UNICODE
        static void call(Out& out, char_encoding::unicode::char_type val, plain_attribute)
        {
            if (val >= 0 && val < 127)
            {
              if (iscntrl(val))
                out << "\\" << std::oct << int(val) << std::dec;
              else if (isprint(val))
                out << char(val);
              else
                out << "\\x" << std::hex << int(val) << std::dec;
            }
            else
              out << "\\x" << std::hex << int(val) << std::dec;
        }

        static void call(Out& out, char val, plain_attribute tag)
        {
            call(out, static_cast<char_encoding::unicode::char_type>(val), tag);
        }
#endif

        // for fusion data types
        template <typename T_>
        static void call(Out& out, T_ const& val, tuple_attribute)
        {
            out << '[';
            fusion::for_each(val, detail::print_fusion_sequence<Out>(out));
            out << ']';
        }

        // stl container
        template <typename T_>
        static void call(Out& out, T_ const& val, container_attribute)
        {
            out << '[';
            if (!traits::is_empty(val))
            {
                bool first = true;
                typename container_iterator<T_ const>::type iend = traits::end(val);
                for (typename container_iterator<T_ const>::type i = traits::begin(val);
                     !traits::compare(i, iend); traits::next(i))
                {
                    if (!first)
                        out << ", ";
                    first = false;
                    x3::traits::print_attribute(out, traits::deref(i));
                }
            }
            out << ']';
        }

        // for variant types
        template <typename T_>
        static void call(Out& out, T_ const& val, variant_attribute)
        {
            apply_visitor(detail::print_visitor<Out>(out), val);
        }

        // for optional types
        template <typename T_>
        static void call(Out& out, T_ const& val, optional_attribute)
        {
            if (val)
                x3::traits::print_attribute(out, *val);
            else
                out << "[empty]";
        }

        // main entry point
        static void call(Out& out, T const& val)
        {
            call(out, val, typename attribute_category<T>::type());
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Out, typename T>
    inline void print_attribute(Out& out, T const& val)
    {
        print_attribute_debug<Out, T>::call(out, val);
    }
}}}}

#endif
