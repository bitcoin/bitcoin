/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_INFO_NOVEMBER_22_2008_1132AM)
#define BOOST_SPIRIT_INFO_NOVEMBER_22_2008_1132AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/variant/variant.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/spirit/home/support/utf8.hpp>
#include <list>
#include <iterator>
#include <utility>

namespace boost { namespace spirit
{
    // info provides information about a component. Each component
    // has a what member function that returns an info object.
    // strings in the info object are assumed to be encoded as UTF8
    // for uniformity.
    struct info
    {
        struct nil_ {};

        typedef
            boost::variant<
                nil_
              , utf8_string
              , recursive_wrapper<info>
              , recursive_wrapper<std::pair<info, info> >
              , recursive_wrapper<std::list<info> >
            >
        value_type;

        explicit info(utf8_string const& tag_)
          : tag(tag_), value(nil_()) {}

        template <typename T>
        info(utf8_string const& tag_, T const& value_)
          : tag(tag_), value(value_) {}

        info(utf8_string const& tag_, char value_)
          : tag(tag_), value(utf8_string(1, value_)) {}

        info(utf8_string const& tag_, wchar_t value_)
          : tag(tag_), value(to_utf8(value_)) {}

        info(utf8_string const& tag_, ucs4_char value_)
          : tag(tag_), value(to_utf8(value_)) {}

        template <typename Char>
        info(utf8_string const& tag_, Char const* str)
          : tag(tag_), value(to_utf8(str)) {}

        template <typename Char, typename Traits, typename Allocator>
        info(utf8_string const& tag_
              , std::basic_string<Char, Traits, Allocator> const& str)
          : tag(tag_), value(to_utf8(str)) {}

        utf8_string tag;
        value_type value;
    };

    template <typename Callback>
    struct basic_info_walker
    {
        typedef void result_type;
        typedef basic_info_walker<Callback> this_type;

        basic_info_walker(Callback& callback_, utf8_string const& tag_, int depth_)
          : callback(callback_), tag(tag_), depth(depth_) {}

        void operator()(info::nil_) const
        {
            callback.element(tag, "", depth);
        }

        void operator()(utf8_string const& str) const
        {
            callback.element(tag, str, depth);
        }

        void operator()(info const& what) const
        {
            boost::apply_visitor(
                this_type(callback, what.tag, depth+1), what.value);
        }

        void operator()(std::pair<info, info> const& pair) const
        {
            callback.element(tag, "", depth);
            boost::apply_visitor(
                this_type(callback, pair.first.tag, depth+1), pair.first.value);
            boost::apply_visitor(
                this_type(callback, pair.second.tag, depth+1), pair.second.value);
        }

        void operator()(std::list<info> const& l) const
        {
            callback.element(tag, "", depth);
            for (std::list<info>::const_iterator it = l.begin(),
                                                 end = l.end(); it != end; ++it)
            {
                boost::apply_visitor(
                    this_type(callback, it->tag, depth+1), it->value);
            }
        }

        Callback& callback;
        utf8_string const& tag;
        int depth;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(basic_info_walker& operator= (basic_info_walker const&))
    };

    // bare-bones print support
    template <typename Out>
    struct simple_printer
    {
        typedef utf8_string string;

        simple_printer(Out& out_)
          : out(out_) {}

        void element(string const& tag, string const& value, int /*depth*/) const
        {
            if (value.empty())
                out << '<' << tag << '>';
            else
                out << '"' << value << '"';
        }

        Out& out;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(simple_printer& operator= (simple_printer const&))
    };

    template <typename Out>
    Out& operator<<(Out& out, info const& what)
    {
        simple_printer<Out> pr(out);
        basic_info_walker<simple_printer<Out> > walker(pr, what.tag, 0);
        boost::apply_visitor(walker, what.value);
        return out;
    }
}}

#endif
