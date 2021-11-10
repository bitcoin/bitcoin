/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001 Daniel Nuffer
    Copyright (c) 2002 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ALTERNATIVE_IPP)
#define BOOST_SPIRIT_ALTERNATIVE_IPP

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  alternative class implementation
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename A, typename B>
    inline alternative<A, B>
    operator|(parser<A> const& a, parser<B> const& b)
    {
        return alternative<A, B>(a.derived(), b.derived());
    }
    
    template <typename A>
    inline alternative<A, chlit<char> >
    operator|(parser<A> const& a, char b)
    {
        return alternative<A, chlit<char> >(a.derived(), b);
    }
    
    template <typename B>
    inline alternative<chlit<char>, B>
    operator|(char a, parser<B> const& b)
    {
        return alternative<chlit<char>, B>(a, b.derived());
    }
    
    template <typename A>
    inline alternative<A, strlit<char const*> >
    operator|(parser<A> const& a, char const* b)
    {
        return alternative<A, strlit<char const*> >(a.derived(), b);
    }
    
    template <typename B>
    inline alternative<strlit<char const*>, B>
    operator|(char const* a, parser<B> const& b)
    {
        return alternative<strlit<char const*>, B>(a, b.derived());
    }
    
    template <typename A>
    inline alternative<A, chlit<wchar_t> >
    operator|(parser<A> const& a, wchar_t b)
    {
        return alternative<A, chlit<wchar_t> >(a.derived(), b);
    }
    
    template <typename B>
    inline alternative<chlit<wchar_t>, B>
    operator|(wchar_t a, parser<B> const& b)
    {
        return alternative<chlit<wchar_t>, B>(a, b.derived());
    }
    
    template <typename A>
    inline alternative<A, strlit<wchar_t const*> >
    operator|(parser<A> const& a, wchar_t const* b)
    {
        return alternative<A, strlit<wchar_t const*> >(a.derived(), b);
    }
    
    template <typename B>
    inline alternative<strlit<wchar_t const*>, B>
    operator|(wchar_t const* a, parser<B> const& b)
    {
        return alternative<strlit<wchar_t const*>, B>(a, b.derived());
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif
