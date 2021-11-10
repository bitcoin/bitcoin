/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_PARSER_IPP)
#define BOOST_SPIRIT_PARSER_IPP

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Generic parse function implementation
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename IteratorT, typename DerivedT>
    inline parse_info<IteratorT>
    parse(
        IteratorT const& first_
      , IteratorT const& last
      , parser<DerivedT> const& p)
    {
        IteratorT first = first_;
        scanner<IteratorT, scanner_policies<> > scan(first, last);
        match<nil_t> hit = p.derived().parse(scan);
        return parse_info<IteratorT>(
            first, hit, hit && (first == last), hit.length());
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Parse function for null terminated strings implementation
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharT, typename DerivedT>
    inline parse_info<CharT const*>
    parse(CharT const* str, parser<DerivedT> const& p)
    {
        CharT const* last = str;
        while (*last)
            last++;
        return parse(str, last, p);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif

