/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001 Daniel Nuffer
    Copyright (c) 2002 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_LIST_IPP)
#define BOOST_SPIRIT_LIST_IPP

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  operator% is defined as:
    //  a % b ---> a >> *(b >> a)
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename A, typename B>
    inline sequence<A, kleene_star<sequence<B, A> > >
    operator%(parser<A> const& a, parser<B> const& b)
    {
        return a.derived() >> *(b.derived() >> a.derived());
    }
    
    template <typename A>
    inline sequence<A, kleene_star<sequence<chlit<char>, A> > >
    operator%(parser<A> const& a, char b)
    {
        return a.derived() >> *(b >> a.derived());
    }
    
    template <typename B>
    inline sequence<chlit<char>, kleene_star<sequence<B, chlit<char> > > >
    operator%(char a, parser<B> const& b)
    {
        return a >> *(b.derived() >> a);
    }
    
    template <typename A>
    inline sequence<A, kleene_star<sequence<strlit<char const*>, A> > >
    operator%(parser<A> const& a, char const* b)
    {
        return a.derived() >> *(b >> a.derived());
    }
    
    template <typename B>
    inline sequence<strlit<char const*>,
        kleene_star<sequence<B, strlit<char const*> > > >
    operator%(char const* a, parser<B> const& b)
    {
        return a >> *(b.derived() >> a);
    }
    
    template <typename A>
    inline sequence<A, kleene_star<sequence<chlit<wchar_t>, A> > >
    operator%(parser<A> const& a, wchar_t b)
    {
        return a.derived() >> *(b >> a.derived());
    }
    
    template <typename B>
    inline sequence<chlit<wchar_t>, kleene_star<sequence<B, chlit<wchar_t> > > >
    operator%(wchar_t a, parser<B> const& b)
    {
        return a >> *(b.derived() >> a);
    }
    
    template <typename A>
    inline sequence<A, kleene_star<sequence<strlit<wchar_t const*>, A> > >
    operator%(parser<A> const& a, wchar_t const* b)
    {
        return a.derived() >> *(b >> a.derived());
    }
    
    template <typename B>
    inline sequence<strlit<wchar_t const*>,
        kleene_star<sequence<B, strlit<wchar_t const*> > > >
    operator%(wchar_t const* a, parser<B> const& b)
    {
        return a >> *(b.derived() >> a);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif
