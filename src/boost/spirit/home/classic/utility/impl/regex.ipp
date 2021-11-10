/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_REGEX_IPP
#define BOOST_SPIRIT_REGEX_IPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/core/primitives/impl/primitives.ipp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

namespace impl {

///////////////////////////////////////////////////////////////////////////////
//
inline const char* rx_prefix(char) { return "\\A"; }
inline const wchar_t* rx_prefix(wchar_t) { return L"\\A"; }

///////////////////////////////////////////////////////////////////////////////
//
//  rx_parser class
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT = char>
class rx_parser : public parser<rx_parser<CharT> > {

public:
    typedef std::basic_string<CharT> string_t;
    typedef rx_parser<CharT> self_t;

    rx_parser(CharT const *first, CharT const *last)
    { 
        rxstr = string_t(rx_prefix(CharT())) + string_t(first, last); 
    }

    rx_parser(CharT const *first)
    { 
        rxstr = string_t(rx_prefix(CharT())) + 
            string_t(first, impl::get_last(first)); 
    }

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        boost::match_results<typename ScannerT::iterator_t> what;
        boost::regex_search(scan.first, scan.last, what, rxstr,
            boost::match_default);

        if (!what[0].matched)
            return scan.no_match();

        scan.first = what[0].second;
        return scan.create_match(what[0].length(), nil_t(),
            what[0].first, scan.first);
    }

private:
#if BOOST_VERSION >= 013300
    boost::basic_regex<CharT> rxstr;       // regular expression to match
#else
    boost::reg_expression<CharT> rxstr;    // regular expression to match
#endif
};

}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif // BOOST_SPIRIT_REGEX_IPP
