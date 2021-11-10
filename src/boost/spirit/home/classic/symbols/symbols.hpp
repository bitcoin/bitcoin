/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_SYMBOLS_HPP
#define BOOST_SPIRIT_SYMBOLS_HPP

///////////////////////////////////////////////////////////////////////////////
#include <string>

#include <boost/ref.hpp>

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/directives.hpp>

#include <boost/spirit/home/classic/symbols/symbols_fwd.hpp>


///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  symbols class
//
//      This class implements a symbol table. The symbol table holds a
//      dictionary of symbols where each symbol is a sequence of CharTs.
//      The template class can work efficiently with 8, 16 and 32 bit
//      characters. Mutable data of type T is associated with each
//      symbol.
//
//      The class is a parser. The parse member function returns
//      additional information in the symbol_match class (see below).
//      The additional data is a pointer to some data associated with
//      the matching symbol.
//
//      The actual set implementation is supplied by the SetT template
//      parameter. By default, this uses the tst class (see tst.ipp).
//
//      Symbols are added into the symbol table statically using the
//      construct:
//
//          sym = a, b, c, d ...;
//
//      where sym is a symbol table and a..d are strings. Example:
//
//          sym = "pineapple", "orange", "banana", "apple";
//
//      Alternatively, symbols may be added dynamically through the
//      member functor 'add' (see symbol_inserter below). The member
//      functor 'add' may be attached to a parser as a semantic action
//      taking in a begin/end pair:
//
//          p[sym.add]
//
//      where p is a parser (and sym is a symbol table). On success,
//      the matching portion of the input is added to the symbol table.
//
//      'add' may also be used to directly initialize data. Examples:
//
//          sym.add("hello", 1)("crazy", 2)("world", 3);
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename CharT, typename SetT>
class symbols
:   private SetT
,   public parser<symbols<T, CharT, SetT> >
{
public:

    typedef parser<symbols<T, CharT, SetT> > parser_base_t;
    typedef symbols<T, CharT, SetT> self_t;
    typedef self_t const& embed_t;
    typedef T symbol_data_t;
    typedef boost::reference_wrapper<T> symbol_ref_t;

    symbols();
    symbols(symbols const& other);
    ~symbols();

    symbols&
    operator=(symbols const& other);

    symbol_inserter<T, SetT> const&
    operator=(CharT const* str);

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, symbol_ref_t>::type type;
    };

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse_main(ScannerT const& scan) const
    {
        typedef typename ScannerT::iterator_t iterator_t;
        iterator_t first = scan.first;
        typename SetT::search_info result = SetT::find(scan);

        if (result.data)
            return scan.
                create_match(
                    result.length,
                    symbol_ref_t(*result.data),
                    first,
                    scan.first);
        else
            return scan.no_match();
    }

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        return impl::implicit_lexeme_parse<result_t>
            (*this, scan, scan);
    }

    template < typename ScannerT >
    T* find(ScannerT const& scan) const
    { return SetT::find(scan).data; }

    symbol_inserter<T, SetT> const add;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Symbol table utilities
//
//  add
//
//      adds a symbol 'sym' (string) to a symbol table 'table' plus an
//      optional data 'data' associated with the symbol. Returns a pointer to
//      the data associated with the symbol or NULL if add failed (e.g. when
//      the symbol is already added before).
//
//  find
//
//      finds a symbol 'sym' (string) from a symbol table 'table'. Returns a
//      pointer to the data associated with the symbol or NULL if not found
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename CharT, typename SetT>
T*  add(symbols<T, CharT, SetT>& table, CharT const* sym, T const& data = T());

template <typename T, typename CharT, typename SetT>
T*  find(symbols<T, CharT, SetT> const& table, CharT const* sym);

///////////////////////////////////////////////////////////////////////////////
//
//  symbol_inserter class
//
//      The symbols class holds an instance of this class named 'add'.
//      This can be called directly just like a member function,
//      passing in a first/last iterator and optional data:
//
//          sym.add(first, last, data);
//
//      Or, passing in a C string and optional data:
//
//          sym.add(c_string, data);
//
//      where sym is a symbol table. The 'data' argument is optional.
//      This may also be used as a semantic action since it conforms
//      to the action interface (see action.hpp):
//
//          p[sym.add]
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename SetT>
class symbol_inserter
{
public:

    symbol_inserter(SetT& set_)
    : set(set_) {}

    typedef symbol_inserter const & result_type;

    template <typename IteratorT>
    symbol_inserter const&
    operator()(IteratorT first, IteratorT const& last, T const& data = T()) const
    {
        set.add(first, last, data);
        return *this;
    }

    template <typename CharT>
    symbol_inserter const&
    operator()(CharT const* str, T const& data = T()) const
    {
        CharT const* last = str;
        while (*last)
            last++;
        set.add(str, last, data);
        return *this;
    }

    template <typename CharT>
    symbol_inserter const&
    operator,(CharT const* str) const
    {
        CharT const* last = str;
        while (*last)
            last++;
        set.add(str, last, T());
        return *this;
    }

private:

    SetT& set;
};

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#include <boost/spirit/home/classic/symbols/impl/symbols.ipp>
#endif
