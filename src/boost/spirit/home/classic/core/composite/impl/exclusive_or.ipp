/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001 Daniel Nuffer
    Copyright (c) 2002 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_EXCLUSIVE_OR_IPP)
#define BOOST_SPIRIT_EXCLUSIVE_OR_IPP

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  exclusive_or class implementation
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename A, typename B>
    inline exclusive_or<A, B>
    operator^(parser<A> const& a, parser<B> const& b)
    {
        return exclusive_or<A, B>(a.derived(), b.derived());
    }
    
    template <typename A>
    inline exclusive_or<A, chlit<char> >
    operator^(parser<A> const& a, char b)
    {
        return exclusive_or<A, chlit<char> >(a.derived(), b);
    }
    
    template <typename B>
    inline exclusive_or<chlit<char>, B>
    operator^(char a, parser<B> const& b)
    {
        return exclusive_or<chlit<char>, B>(a, b.derived());
    }
    
    template <typename A>
    inline exclusive_or<A, strlit<char const*> >
    operator^(parser<A> const& a, char const* b)
    {
        return exclusive_or<A, strlit<char const*> >(a.derived(), b);
    }
    
    template <typename B>
    inline exclusive_or<strlit<char const*>, B>
    operator^(char const* a, parser<B> const& b)
    {
        return exclusive_or<strlit<char const*>, B>(a, b.derived());
    }
    
    template <typename A>
    inline exclusive_or<A, chlit<wchar_t> >
    operator^(parser<A> const& a, wchar_t b)
    {
        return exclusive_or<A, chlit<wchar_t> >(a.derived(), b);
    }
    
    template <typename B>
    inline exclusive_or<chlit<wchar_t>, B>
    operator^(wchar_t a, parser<B> const& b)
    {
        return exclusive_or<chlit<wchar_t>, B>(a, b.derived());
    }
    
    template <typename A>
    inline exclusive_or<A, strlit<wchar_t const*> >
    operator^(parser<A> const& a, wchar_t const* b)
    {
        return exclusive_or<A, strlit<wchar_t const*> >(a.derived(), b);
    }
    
    template <typename B>
    inline exclusive_or<strlit<wchar_t const*>, B>
    operator^(wchar_t const* a, parser<B> const& b)
    {
        return exclusive_or<strlit<wchar_t const*>, B>(a, b.derived());
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif
