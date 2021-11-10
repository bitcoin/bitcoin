/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_LISTS_HPP
#define BOOST_SPIRIT_LISTS_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/config.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/meta/as_parser.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>

#include <boost/spirit/home/classic/utility/lists_fwd.hpp>
#include <boost/spirit/home/classic/utility/impl/lists.ipp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  list_parser class
//
//      List parsers allow to parse constructs like
//
//          item >> *(delim >> item)
//
//      where 'item' is an auxiliary expression to parse and 'delim' is an
//      auxiliary delimiter to parse.
//
//      The list_parser class also can match an optional closing delimiter
//      represented by the 'end' parser at the end of the list:
//
//          item >> *(delim >> item) >> !end.
//
//      If ItemT is an action_parser_category type (parser with an attached
//      semantic action) we have to do something special. This happens, if the
//      user wrote something like:
//
//          list_p(item[f], delim)
//
//      where 'item' is the parser matching one item of the list sequence and
//      'f' is a functor to be called after matching one item. If we would do
//      nothing, the resulting code would parse the sequence as follows:
//
//          (item[f] - delim) >> *(delim >> (item[f] - delim))
//
//      what in most cases is not what the user expects.
//      (If this _is_ what you've expected, then please use one of the list_p
//      generator functions 'direct()', which will inhibit re-attaching
//      the actor to the item parser).
//
//      To make the list parser behave as expected:
//
//          (item - delim)[f] >> *(delim >> (item - delim)[f])
//
//      the actor attached to the 'item' parser has to be re-attached to the
//      *(item - delim) parser construct, which will make the resulting list
//      parser 'do the right thing'.
//
//      Additionally special care must be taken, if the item parser is a
//      unary_parser_category type parser as
//
//          list_p(*anychar_p, ',')
//
//      which without any refactoring would result in
//
//          (*anychar_p - ch_p(','))
//              >> *( ch_p(',') >> (*anychar_p - ch_p(',')) )
//
//      and will not give the expected result (the first *anychar_p will eat up
//      all the input up to the end of the input stream). So we have to
//      refactor this into:
//
//          *(anychar_p - ch_p(','))
//              >> *( ch_p(',') >> *(anychar_p - ch_p(',')) )
//
//      what will give the correct result.
//
//      The case, where the item parser is a combination of the two mentioned
//      problems (i.e. the item parser is a unary parser  with an attached
//      action), is handled accordingly too:
//
//          list_p((*anychar_p)[f], ',')
//
//      will be parsed as expected:
//
//          (*(anychar_p - ch_p(',')))[f]
//              >> *( ch_p(',') >> (*(anychar_p - ch_p(',')))[f] ).
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename ItemT, typename DelimT, typename EndT, typename CategoryT
>
struct list_parser :
    public parser<list_parser<ItemT, DelimT, EndT, CategoryT> > {

    typedef list_parser<ItemT, DelimT, EndT, CategoryT> self_t;
    typedef CategoryT parser_category_t;

    list_parser(ItemT const &item_, DelimT const &delim_,
        EndT const& end_ = no_list_endtoken())
    : item(item_), delim(delim_), end(end_)
    {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        return impl::list_parser_type<CategoryT>
            ::parse(scan, *this, item, delim, end);
    }

private:
    typename as_parser<ItemT>::type::embed_t item;
    typename as_parser<DelimT>::type::embed_t delim;
    typename as_parser<EndT>::type::embed_t end;
};

///////////////////////////////////////////////////////////////////////////////
//
//  List parser generator template
//
//      This is a helper for generating a correct list_parser<> from
//      auxiliary parameters. There are the following types supported as
//      parameters yet: parsers, single characters and strings (see
//      as_parser<> in meta/as_parser.hpp).
//
//      The list_parser_gen by itself can be used for parsing comma separated
//      lists without item formatting:
//
//          list_p.parse(...)
//              matches any comma separated list.
//
//      If list_p is used with one parameter, this parameter is used to match
//      the delimiter:
//
//          list_p(';').parse(...)
//              matches any semicolon separated list.
//
//      If list_p is used with two parameters, the first parameter is used to
//      match the items and the second parameter matches the delimiters:
//
//          list_p(uint_p, ',').parse(...)
//              matches comma separated unsigned integers.
//
//      If list_p is used with three parameters, the first parameter is used
//      to match the items, the second one is used to match the delimiters and
//      the third one is used to match an optional ending token sequence:
//
//          list_p(real_p, ';', eol_p).parse(...)
//              matches a semicolon separated list of real numbers optionally
//              followed by an end of line.
//
//      The list_p in the previous examples denotes the predefined parser
//      generator, which should be used to define list parsers (see below).
//
///////////////////////////////////////////////////////////////////////////////

template <typename CharT = char>
struct list_parser_gen :
    public list_parser<kleene_star<anychar_parser>, chlit<CharT> >
{
    typedef list_parser_gen<CharT> self_t;

// construct the list_parser_gen object as an list parser for comma separated
// lists without item formatting.
    list_parser_gen()
    : list_parser<kleene_star<anychar_parser>, chlit<CharT> >
        (*anychar_p, chlit<CharT>(','))
    {}

// The following generator functions should be used under normal circumstances.
// (the operator()(...) functions)

    // Generic generator functions for creation of concrete list parsers, which
    // support 'normal' syntax:
    //
    //      item >> *(delim >> item)
    //
    // If item isn't given, everything between two delimiters is matched.

    template<typename DelimT>
    list_parser<
        kleene_star<anychar_parser>,
        typename as_parser<DelimT>::type,
        no_list_endtoken,
        unary_parser_category      // there is no action to re-attach
    >
    operator()(DelimT const &delim_) const
    {
        typedef kleene_star<anychar_parser> item_t;
        typedef typename as_parser<DelimT>::type delim_t;

        typedef
            list_parser<item_t, delim_t, no_list_endtoken, unary_parser_category>
            return_t;

        return return_t(*anychar_p, as_parser<DelimT>::convert(delim_));
    }

    template<typename ItemT, typename DelimT>
    list_parser<
        typename as_parser<ItemT>::type,
        typename as_parser<DelimT>::type,
        no_list_endtoken,
        typename as_parser<ItemT>::type::parser_category_t
    >
    operator()(ItemT const &item_, DelimT const &delim_) const
    {
        typedef typename as_parser<ItemT>::type item_t;
        typedef typename as_parser<DelimT>::type delim_t;
        typedef list_parser<item_t, delim_t, no_list_endtoken,
                BOOST_DEDUCED_TYPENAME item_t::parser_category_t>
            return_t;

        return return_t(
            as_parser<ItemT>::convert(item_),
            as_parser<DelimT>::convert(delim_)
        );
    }

    // Generic generator function for creation of concrete list parsers, which
    // support 'extended' syntax:
    //
    //      item >> *(delim >> item) >> !end

    template<typename ItemT, typename DelimT, typename EndT>
    list_parser<
        typename as_parser<ItemT>::type,
        typename as_parser<DelimT>::type,
        typename as_parser<EndT>::type,
        typename as_parser<ItemT>::type::parser_category_t
    >
    operator()(
        ItemT const &item_, DelimT const &delim_, EndT const &end_) const
    {
        typedef typename as_parser<ItemT>::type item_t;
        typedef typename as_parser<DelimT>::type delim_t;
        typedef typename as_parser<EndT>::type end_t;

        typedef list_parser<item_t, delim_t, end_t,
                BOOST_DEDUCED_TYPENAME item_t::parser_category_t>
            return_t;

        return return_t(
            as_parser<ItemT>::convert(item_),
            as_parser<DelimT>::convert(delim_),
            as_parser<EndT>::convert(end_)
        );
    }

// The following functions should be used, if the 'item' parser has an attached
// semantic action or is a unary_parser_category type parser and the structure
// of the resulting list parser should _not_ be refactored during parser
// construction (see comment above).

    // Generic generator function for creation of concrete list parsers, which
    // support 'normal' syntax:
    //
    //      item >> *(delim >> item)

    template<typename ItemT, typename DelimT>
    list_parser<
        typename as_parser<ItemT>::type,
        typename as_parser<DelimT>::type,
        no_list_endtoken,
        plain_parser_category        // inhibit action re-attachment
    >
    direct(ItemT const &item_, DelimT const &delim_) const
    {
        typedef typename as_parser<ItemT>::type item_t;
        typedef typename as_parser<DelimT>::type delim_t;
        typedef list_parser<item_t, delim_t, no_list_endtoken,
                plain_parser_category>
            return_t;

        return return_t(
            as_parser<ItemT>::convert(item_),
            as_parser<DelimT>::convert(delim_)
        );
    }

    // Generic generator function for creation of concrete list parsers, which
    // support 'extended' syntax:
    //
    //      item >> *(delim >> item) >> !end

    template<typename ItemT, typename DelimT, typename EndT>
    list_parser<
        typename as_parser<ItemT>::type,
        typename as_parser<DelimT>::type,
        typename as_parser<EndT>::type,
        plain_parser_category        // inhibit action re-attachment
    >
    direct(
        ItemT const &item_, DelimT const &delim_, EndT const &end_) const
    {
        typedef typename as_parser<ItemT>::type item_t;
        typedef typename as_parser<DelimT>::type delim_t;
        typedef typename as_parser<EndT>::type end_t;

        typedef
            list_parser<item_t, delim_t, end_t, plain_parser_category>
            return_t;

        return return_t(
            as_parser<ItemT>::convert(item_),
            as_parser<DelimT>::convert(delim_),
            as_parser<EndT>::convert(end_)
        );
    }
};

///////////////////////////////////////////////////////////////////////////////
//
//  Predefined list parser generator
//
//      The list_p parser generator can be used
//        - by itself for parsing comma separated lists without item formatting
//      or
//        - for generating list parsers with auxiliary parser parameters
//          for the 'item', 'delim' and 'end' subsequences.
//      (see comment above)
//
///////////////////////////////////////////////////////////////////////////////
const list_parser_gen<> list_p = list_parser_gen<>();

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif
