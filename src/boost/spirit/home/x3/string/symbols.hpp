/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2013 Carl Barron

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_SYMBOLS_MARCH_11_2007_1055AM)
#define BOOST_SPIRIT_X3_SYMBOLS_MARCH_11_2007_1055AM

#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/string/tst.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/support/traits/string_traits.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <boost/spirit/home/x3/support/no_case.hpp>

#include <boost/spirit/home/support/char_encoding/ascii.hpp>
#include <boost/spirit/home/support/char_encoding/iso8859_1.hpp>
#include <boost/spirit/home/support/char_encoding/standard.hpp>
#include <boost/spirit/home/support/char_encoding/standard_wide.hpp>

#include <initializer_list>
#include <iterator> // std::begin
#include <memory> // std::shared_ptr
#include <type_traits>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4355) // 'this' : used in base member initializer list warning
#endif

namespace boost { namespace spirit { namespace x3
{
    template <
        typename Encoding
      , typename T = unused_type
      , typename Lookup = tst<typename Encoding::char_type, T> >
    struct symbols_parser : parser<symbols_parser<Encoding, T, Lookup>>
    {
        typedef typename Encoding::char_type char_type; // the character type
        typedef Encoding encoding;
        typedef T value_type; // the value associated with each entry
        typedef value_type attribute_type;

        static bool const has_attribute =
            !std::is_same<unused_type, attribute_type>::value;
        static bool const handles_container =
            traits::is_container<attribute_type>::value;

        symbols_parser(std::string const& name = "symbols")
          : add{*this}
          , remove{*this}
          , lookup(std::make_shared<Lookup>())
          , name_(name)
        {
        }

        symbols_parser(symbols_parser const& syms)
          : add{*this}
          , remove{*this}
          , lookup(syms.lookup)
          , name_(syms.name_)
        {
        }

        template <typename Symbols>
        symbols_parser(Symbols const& syms, std::string const& name = "symbols")
          : symbols_parser(name)
        {
            for (auto& sym : syms)
                add(sym);
        }

        template <typename Symbols, typename Data>
        symbols_parser(Symbols const& syms, Data const& data
              , std::string const& name = "symbols")
          : symbols_parser(name)
        {
            using std::begin;
            auto di = begin(data);
            for (auto& sym : syms)
                add(sym, *di++);
        }

        symbols_parser(std::initializer_list<std::pair<char_type const*, T>> syms
              , std::string const & name="symbols")
          : symbols_parser(name)
        {
            for (auto& sym : syms)
                add(sym.first, sym.second);
        }

        symbols_parser(std::initializer_list<char_type const*> syms
              , std::string const &name="symbols")
          : symbols_parser(name)
        {
            for (auto str : syms)
                add(str);
        }

        symbols_parser&
        operator=(symbols_parser const& rhs)
        {
            name_ = rhs.name_;
            lookup = rhs.lookup;
            return *this;
        }

        void clear()
        {
            lookup->clear();
        }

        struct adder;
        struct remover;

        template <typename Str>
        adder const&
        operator=(Str const& str)
        {
            lookup->clear();
            return add(str);
        }

        template <typename Str>
        friend adder const&
        operator+=(symbols_parser& sym, Str const& str)
        {
            return sym.add(str);
        }

        template <typename Str>
        friend remover const&
        operator-=(symbols_parser& sym, Str const& str)
        {
            return sym.remove(str);
        }

        template <typename F>
        void for_each(F f) const
        {
            lookup->for_each(f);
        }

        template <typename Str>
        value_type& at(Str const& str)
        {
            return *lookup->add(traits::get_string_begin<char_type>(str)
                , traits::get_string_end<char_type>(str), T());
        }

        template <typename Iterator>
        value_type* prefix_find(Iterator& first, Iterator const& last)
        {
            return lookup->find(first, last, case_compare<Encoding>());
        }

        template <typename Iterator>
        value_type const* prefix_find(Iterator& first, Iterator const& last) const
        {
            return lookup->find(first, last, case_compare<Encoding>());
        }

        template <typename Str>
        value_type* find(Str const& str)
        {
            return find_impl(traits::get_string_begin<char_type>(str)
                , traits::get_string_end<char_type>(str));
        }

        template <typename Str>
        value_type const* find(Str const& str) const
        {
            return find_impl(traits::get_string_begin<char_type>(str)
                , traits::get_string_end<char_type>(str));
        }

    private:

        template <typename Iterator>
        value_type* find_impl(Iterator begin, Iterator end)
        {
            value_type* r = lookup->find(begin, end, case_compare<Encoding>());
            return begin == end ? r : 0;
        }

        template <typename Iterator>
        value_type const* find_impl(Iterator begin, Iterator end) const
        {
            value_type const* r = lookup->find(begin, end, case_compare<Encoding>());
            return begin == end ? r : 0;
        }

    public:

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute& attr) const
        {
            x3::skip_over(first, last, context);

            if (value_type const* val_ptr
                = lookup->find(first, last, get_case_compare<Encoding>(context)))
            {
                x3::traits::move_to(*val_ptr, attr);
                return true;
            }
            return false;
        }

        void name(std::string const &str)
        {
            name_ = str;
        }
        std::string const &name() const
        {
            return name_;
        }

        struct adder
        {
            template <typename Iterator>
            adder const&
            operator()(Iterator first, Iterator last, T const& val) const
            {
                sym.lookup->add(first, last, val);
                return *this;
            }

            template <typename Str>
            adder const&
            operator()(Str const& s, T const& val = T()) const
            {
                sym.lookup->add(traits::get_string_begin<char_type>(s)
                  , traits::get_string_end<char_type>(s), val);
                return *this;
            }

            template <typename Str>
            adder const&
            operator,(Str const& s) const
            {
                sym.lookup->add(traits::get_string_begin<char_type>(s)
                  , traits::get_string_end<char_type>(s), T());
                return *this;
            }

            symbols_parser& sym;
        };

        struct remover
        {
            template <typename Iterator>
            remover const&
            operator()(Iterator const& first, Iterator const& last) const
            {
                sym.lookup->remove(first, last);
                return *this;
            }

            template <typename Str>
            remover const&
            operator()(Str const& s) const
            {
                sym.lookup->remove(traits::get_string_begin<char_type>(s)
                  , traits::get_string_end<char_type>(s));
                return *this;
            }

            template <typename Str>
            remover const&
            operator,(Str const& s) const
            {
                sym.lookup->remove(traits::get_string_begin<char_type>(s)
                  , traits::get_string_end<char_type>(s));
                return *this;
            }

            symbols_parser& sym;
        };

        adder add;
        remover remove;
        std::shared_ptr<Lookup> lookup;
        std::string name_;
    };

    template <typename Encoding, typename T, typename Lookup>
    struct get_info<symbols_parser<Encoding, T, Lookup>>
    {
      typedef std::string result_type;
      result_type operator()(symbols_parser< Encoding, T
                                    , Lookup
                                    > const& symbols) const
      {
         return symbols.name();
      }
    };

    namespace standard
    {
        template <typename T = unused_type>
        using symbols = symbols_parser<char_encoding::standard, T>;
    }

    using standard::symbols;

#ifndef BOOST_SPIRIT_NO_STANDARD_WIDE
    namespace standard_wide
    {
        template <typename T = unused_type>
        using symbols = symbols_parser<char_encoding::standard_wide, T>;
    }
#endif

    namespace ascii
    {
        template <typename T = unused_type>
        using symbols = symbols_parser<char_encoding::ascii, T>;
    }

    namespace iso8859_1
    {
        template <typename T = unused_type>
        using symbols = symbols_parser<char_encoding::iso8859_1, T>;
    }

}}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif
