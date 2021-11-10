/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_SYMBOLS_MARCH_11_2007_1055AM)
#define BOOST_SPIRIT_SYMBOLS_MARCH_11_2007_1055AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/string/tst.hpp>
#include <boost/spirit/home/qi/reference.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/detail/get_encoding.hpp>
#include <boost/spirit/home/support/modify.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/proto/extends.hpp>
#include <boost/proto/traits.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/shared_ptr.hpp>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4355) // 'this' : used in base member initializer list warning
#endif

namespace boost { namespace spirit { namespace qi
{
    template <
        typename Char = char
      , typename T = unused_type
      , typename Lookup = tst<Char, T>
      , typename Filter = tst_pass_through>
    struct symbols
      : proto::extends<
            typename proto::terminal<
                reference<symbols<Char, T, Lookup, Filter> >
            >::type
          , symbols<Char, T, Lookup, Filter>
        >
      , primitive_parser<symbols<Char, T, Lookup, Filter> >
    {
        typedef Char char_type; // the character type
        typedef T value_type; // the value associated with each entry
        typedef symbols<Char, T, Lookup, Filter> this_type;
        typedef reference<this_type> reference_;
        typedef typename proto::terminal<reference_>::type terminal;
        typedef proto::extends<terminal, this_type> base_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef value_type type;
        };

        symbols(std::string const& name = "symbols")
          : base_type(terminal::make(reference_(*this)))
          , add(*this)
          , remove(*this)
          , lookup(new Lookup())
          , name_(name)
        {
        }

        symbols(symbols const& syms)
          : base_type(terminal::make(reference_(*this)))
          , add(*this)
          , remove(*this)
          , lookup(syms.lookup)
          , name_(syms.name_)
        {
        }

        template <typename Filter_>
        symbols(symbols<Char, T, Lookup, Filter_> const& syms)
          : base_type(terminal::make(reference_(*this)))
          , add(*this)
          , remove(*this)
          , lookup(syms.lookup)
          , name_(syms.name_)
        {
        }

        template <typename Symbols>
        symbols(Symbols const& syms, std::string const& name = "symbols")
          : base_type(terminal::make(reference_(*this)))
          , add(*this)
          , remove(*this)
          , lookup(new Lookup())
          , name_(name)
        {
            typename range_const_iterator<Symbols>::type si = boost::begin(syms);
            while (si != boost::end(syms))
                add(*si++);
        }

        template <typename Symbols, typename Data>
        symbols(Symbols const& syms, Data const& data
              , std::string const& name = "symbols")
          : base_type(terminal::make(reference_(*this)))
          , add(*this)
          , remove(*this)
          , lookup(new Lookup())
          , name_(name)
        {
            typename range_const_iterator<Symbols>::type si = boost::begin(syms);
            typename range_const_iterator<Data>::type di = boost::begin(data);
            while (si != boost::end(syms))
                add(*si++, *di++);
        }

        symbols&
        operator=(symbols const& rhs)
        {
            name_ = rhs.name_;
            *lookup = *rhs.lookup;
            return *this;
        }

        template <typename Filter_>
        symbols&
        operator=(symbols<Char, T, Lookup, Filter_> const& rhs)
        {
            name_ = rhs.name_;
            *lookup = *rhs.lookup;
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
        operator+=(symbols& sym, Str const& str)
        {
            return sym.add(str);
        }

        template <typename Str>
        friend remover const&
        operator-=(symbols& sym, Str const& str)
        {
            return sym.remove(str);
        }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        // non-const version needed to suppress proto's += kicking in
        template <typename Str>
        friend adder const&
        operator+=(symbols& sym, Str& str)
        {
            return sym.add(str);
        }

        // non-const version needed to suppress proto's -= kicking in
        template <typename Str>
        friend remover const&
        operator-=(symbols& sym, Str& str)
        {
            return sym.remove(str);
        }
#else
        // for rvalue references
        template <typename Str>
        friend adder const&
        operator+=(symbols& sym, Str&& str)
        {
            return sym.add(str);
        }

        // for rvalue references
        template <typename Str>
        friend remover const&
        operator-=(symbols& sym, Str&& str)
        {
            return sym.remove(str);
        }
#endif
        template <typename F>
        void for_each(F f) const
        {
            lookup->for_each(f);
        }

        template <typename Str>
        value_type& at(Str const& str)
        {
            return *lookup->add(traits::get_begin<Char>(str)
                , traits::get_end<Char>(str), T());
        }

        template <typename Iterator>
        value_type* prefix_find(Iterator& first, Iterator const& last)
        {
            return lookup->find(first, last, Filter());
        }

        template <typename Iterator>
        value_type const* prefix_find(Iterator& first, Iterator const& last) const
        {
            return lookup->find(first, last, Filter());
        }

        template <typename Str>
        value_type* find(Str const& str)
        {
            return find_impl(traits::get_begin<Char>(str)
                , traits::get_end<Char>(str));
        }

        template <typename Str>
        value_type const* find(Str const& str) const
        {
            return find_impl(traits::get_begin<Char>(str)
                , traits::get_end<Char>(str));
        }

private:
        template <typename Iterator>
        value_type* find_impl(Iterator begin, Iterator end)
        {
            value_type* r = lookup->find(begin, end, Filter());
            return begin == end ? r : 0;
        }

        template <typename Iterator>
        value_type const* find_impl(Iterator begin, Iterator end) const
        {
            value_type const* r = lookup->find(begin, end, Filter());
            return begin == end ? r : 0;
        }

public:
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper, Attribute& attr_) const
        {
            qi::skip_over(first, last, skipper);

            if (value_type* val_ptr
                = lookup->find(first, last, Filter()))
            {
                spirit::traits::assign_to(*val_ptr, attr_);
                return true;
            }
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info(name_);
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
            template <typename, typename = unused_type, typename = unused_type>
            struct result { typedef adder const& type; };

            adder(symbols& sym_)
              : sym(sym_)
            {
            }

            template <typename Iterator>
            adder const&
            operator()(Iterator const& first, Iterator const& last, T const& val) const
            {
                sym.lookup->add(first, last, val);
                return *this;
            }

            template <typename Str>
            adder const&
            operator()(Str const& s, T const& val = T()) const
            {
                sym.lookup->add(traits::get_begin<Char>(s)
                  , traits::get_end<Char>(s), val);
                return *this;
            }

            template <typename Str>
            adder const&
            operator,(Str const& s) const
            {
                sym.lookup->add(traits::get_begin<Char>(s)
                  , traits::get_end<Char>(s), T());
                return *this;
            }

            symbols& sym;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(adder& operator= (adder const&))
        };

        struct remover
        {
            template <typename, typename = unused_type, typename = unused_type>
            struct result { typedef remover const& type; };

            remover(symbols& sym_)
              : sym(sym_)
            {
            }

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
                sym.lookup->remove(traits::get_begin<Char>(s)
                  , traits::get_end<Char>(s));
                return *this;
            }

            template <typename Str>
            remover const&
            operator,(Str const& s) const
            {
                sym.lookup->remove(traits::get_begin<Char>(s)
                  , traits::get_end<Char>(s));
                return *this;
            }

            symbols& sym;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(remover& operator= (remover const&))
        };

        adder add;
        remover remove;
        shared_ptr<Lookup> lookup;
        std::string name_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename T, typename Lookup
      , typename Filter, typename Modifiers>
    struct make_primitive<reference<symbols<Char, T, Lookup, Filter> >, Modifiers>
    {
        template <typename CharEncoding>
        struct no_case_filter
        {
            Char operator()(Char ch) const
            {
                return static_cast<Char>(CharEncoding::tolower(ch));
            }
        };

        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> > no_case;
        typedef reference<symbols<Char, T, Lookup, Filter> > reference_;
        typedef no_case_filter<
            typename spirit::detail::get_encoding_with_case<
                Modifiers
              , char_encoding::standard
              , no_case::value>::type>
        nc_filter;

        typedef typename mpl::if_<
            no_case
          , symbols<Char, T, Lookup, nc_filter>
          , reference_>::type
        result_type;

        result_type operator()(reference_ ref, unused_type) const
        {
            return result_type(ref.ref.get());
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename T, typename Lookup, typename Filter
      , typename Attr, typename Context, typename Iterator>
    struct handles_container<qi::symbols<Char, T, Lookup, Filter>, Attr, Context, Iterator>
      : traits::is_container<Attr> {};
}}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif
